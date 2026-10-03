#!/usr/bin/env python3
"""One runner for every tool's selftest - discovery, parallel execution, aggregation, a park list.

    python tools/selftest.py                 # run every discovered selftest
    python tools/selftest.py --changed       # only the selftests of tools this diff touches
    python tools/selftest.py --changed main  # ... of everything since `main`
    python tools/selftest.py --json          # machine-readable summary only (no table)
    python tools/selftest.py --list          # the inventory, without running anything
    python tools/selftest.py --no-dedupe     # run both halves of every wrapper pair
    python tools/selftest.py --selftest      # this runner's own checks (it is discovered like any other)

**The incident this closes.** `tools/units/measure_selftest.py` was red for weeks while 31 lanes filed
"`recompile.py` is broken": the tool's own test said so and nothing ran it. A selftest nobody runs is
decoration, and the land gate only ran `land.py --selftest` - the gate's own tests, never the suite. This is
the one command that runs them all, so a stale tool cannot hide behind a green gate.

**Two shapes, one inventory.** Every tool is tested one of two ways:

* a standalone `tools/**/<name>_selftest.py`, run as `python <file>`; and
* a tool exposing `--selftest`, run as `python <tool.py> --selftest`.

They are one *entry* per tested tool (keyed by the module path with `_selftest` stripped), because many are
wrappers of each other: a tool's `--selftest` can import and run its `<tool>_selftest.py`, and
`dossier_selftest.py` calls `dossier.selftest()`. Running both would run the same checks twice and waste the
gate's time, so a delegating pair is collapsed to its **tool** entry (the documented contract); a genuine
pair that does *not* delegate - `ledger.py --selftest` covers the per-0x10000 view, `ledger_selftest.py` the
older views - keeps **both**, because neither is a duplicate of the other. `--no-dedupe` runs everything.

**`--changed` maps sources, not only `tools/` (F37).** A batch that edits only docs can still break an
invariant, and no `tools/**` selftest covers it: `docs/plan.md` is the source of the section-6.5 block
generated into `.claude/agents/*.md` (`tools/agents/sync_profiles.py`), and `docs/matching/` (one file per
playbook idea) is the source of the skill's `references/matching/` copy
(`.claude/skills/mwcc-unit-matching/scripts/sync_reference.py --check`) and of the generated
`docs/matching/index.md` (`tools/agents/sync_playbook_index.py --check`). Both
are selected when the diff touches those sources, so a docs batch verifies itself instead of reporting
"GREEN, 0 selftests". The mapping is `SOURCE_ENTRIES`/`SOURCE_CHECKS` below.

**Parallel, bounded, and never able to hang the gate.** Each selftest runs in a bounded worker pool with a
per-test timeout; on timeout the whole process tree is killed (several tests shell out to `ninja` and the
compiler, so a wedged child is a real risk, not a formality). The aggregate table names every tool, its check
count and its duration; a failure carries the head of its output.

**The tree-dirty guard.** `git status --porcelain` is captured before and after the whole run and must be
identical - a selftest that writes into the real repository is a defect, and it is invisible unless something
checks. If it moved, the offender is named with the exact before/after rows.  **The live slot manifest**
(`.pi/slots/pool.json`) is guarded the same way but by **bytes**, because `.pi/` is gitignored and the dirty
guard cannot see it: that file is the campaign's concurrency cap, and a run that rewrites it shrinks the pool
for every live lane.

**The park list** (`tools/selftests-known-failures.json`) records each *pre-existing* failure with a reason
and a date, so one old red cannot hide every new one: the summary reads "green except N parked". Parking is
explicit and greppable, never a silent skip, and a park whose test now **passes** is itself reported as
`STALE` (and fails the run) so a debt cannot rot unnoticed.

**A failure is re-run once, alone (2026-09-30).** The suite runs `--jobs` tests at once, on a machine that is
also building (live lanes), and a test that touches git, a temp tree or the clock can lose that race without
being wrong: four landings were refused by `claims`, `ideas` and `slots` selftests that passed by hand a
minute later. So every fail/timeout is re-run ONCE, serially, after the pool has drained. Passing the second
time is a **flake**: it passes the row, prints `flaky: ... passed on isolated re-run` loudly on stderr, lands
in the JSON summary (`flaky`) and is appended to `.pi/selftest-flakes.jsonl` (tool, time, the first failure's
last lines) so flakes are counted, not silently eating landings. A test that fails twice fails the row, and
the refusal carries the last 40 lines of its output (`FAIL_TAIL_LINES`).

Exit status is the answer: 0 only when nothing failed, nothing moved the tree, and no park is stale.
"""
from __future__ import annotations

import argparse
import concurrent.futures
import json
import os
import re
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
PARK_FILE = os.path.join(HERE, "selftests-known-failures.json")
#: prepended to every selftest's PYTHONPATH: its `sitecustomize.py` installs `tools/spawnretry.py`
SITE_DIR = os.path.join(HERE, "selftest_site")

# a tool "exposes --selftest" when it registers the flag (not when a docstring merely mentions it)
SELFTEST_FLAG = re.compile(r"""add_argument\(\s*['"]--selftest['"]""")
# the count shapes the tools print: `ok - 12 checks`, `9/9 checks passed`, `all 6 checks passed`,
# `12 checks` - anything else is reported as `-` and the exit status is still the verdict.
COUNT_PATTERNS = (
    re.compile(r"ok\s*-\s*(\d+)\s+checks?", re.I),
    re.compile(r"\d+\s*/\s*(\d+)\s+checks?\s+passed", re.I),
    re.compile(r"all\s+(\d+)\s+checks?\s+passed", re.I),
    re.compile(r"(\d+)\s+checks?\s+passed", re.I),
    re.compile(r"(\d+)\s+checks?\b", re.I),
)

#: how much of a failing selftest's output the runner keeps and prints (and the gate's refusal carries)
FAIL_TAIL_LINES = 40
#: one JSON line per flake (a selftest that failed, then passed alone), relative to the root
FLAKE_LOG_REL = os.path.join(".pi", "selftest-flakes.jsonl")

SR_REL = ".claude/skills/mwcc-unit-matching/scripts/sync_reference.py"

# **A diff outside `tools/` still owns invariants (F37).** `--changed` used to map only `tools/**` diffs, so
# a docs/profiles batch reported "GREEN, 0 selftests" while the invariant it can break was exactly the one
# nobody ran: the *generated* copies of those docs drifting from the source. Two mappings, because the two
# shapes differ - `docs/plan.md` is the source of the section-6.5 block generated into
# `.claude/agents/*.md`, and `tools/agents/sync_profiles.py`'s own selftest checks the real tree for that
# drift (`sync_profiles.check_profile`, i.e. what its `--check` does); `docs/matching.md` is the source of
# the skill's `references/`, and `sync_reference.py` lives under `.claude/`, so `discover()` (which walks
# `tools/`) never sees it and there is no entry to select - its `--check` runs as a synthetic entry.
#
# `SOURCE_ENTRIES`: source path -> selftest entry keys it must select.
# `SOURCE_CHECKS`:  source path -> ((tool path relative to the root, extra argv), ...) to run as `--check`.
# A key ending in `/` is a PREFIX: every changed path under it selects the entry (the playbook is a directory
# of idea files, so an exact-path key cannot name them).
SKILL_MATCHING = ".claude/skills/mwcc-unit-matching/references/matching/"
SOURCE_ENTRIES = {
    "docs/plan.md": ("tools/agents/sync_profiles",),
    "docs/matching.md": ("tools/agents/sync_playbook_index",),
    "docs/matching/": ("tools/agents/sync_playbook_index", "tools/agents/ideas"),
    SKILL_MATCHING: ("tools/agents/sync_playbook_index", "tools/agents/ideas"),
    ".claude/skills/mwcc-unit-matching/SKILL.md": ("tools/agents/sync_playbook_index",),
}
SOURCE_CHECKS = {
    "docs/matching.md": ((SR_REL, ("--check",)), ("tools/agents/sync_playbook_index.py", ("--check",))),
    "docs/matching/": ((SR_REL, ("--check",)), ("tools/agents/sync_playbook_index.py", ("--check",)), ("tools/agents/ideas.py", ("check",)),
                       ("tools/agents/ideas.py", ("demo-check", "--changed", "{ref}"))),
    SKILL_MATCHING: ((SR_REL, ("--check",)), ("tools/agents/sync_playbook_index.py", ("--check",)), ("tools/agents/ideas.py", ("check",))),
    ".claude/skills/mwcc-unit-matching/SKILL.md": ((SR_REL, ("--check",)),),
}


def _source_lookup(table: dict, src: str):
    """The table's value for `src`: the exact key, then every `dir/` prefix key that contains it."""
    src = src.replace("\\", "/")
    out = list(table.get(src, ()))
    for key, val in table.items():
        if key.endswith("/") and src.startswith(key):
            out.extend(val)
    return out


class Entry:
    """One tested tool. `key` is stable across whichever half of a wrapper pair is run."""

    def __init__(self, key: str, tool: str | None, standalone: str | None,
                 dedupe_note: str = ""):
        self.key = key                      # e.g. "tools/units/queue"
        self.tool = tool                    # relative path of the tool exposing --selftest
        self.standalone = standalone        # relative path of the standalone selftest
        self.dedupe_note = dedupe_note
        # the target actually executed, and the mechanism
        self.target = tool or standalone
        self.kind = "tool" if tool else "standalone"
        self.check_argv: list[str] | None = None   # set for a synthetic `--check` entry (F37)

    def as_check(self, argv: list[str]) -> "Entry":
        """Turn this entry into a `--check` run of a tool outside `tools/` (F37).

        `target` becomes the tool path, so the table, the park list and the failure line all name a real
        file; `argv` is the whole command, because a check has no `--selftest` flag to fall back on. The
        check is added whatever the filesystem says: a check that cannot run is a failure to verify, not a
        silent skip.
        """
        self.kind = "check"
        self.check_argv = list(argv)
        if len(argv) > 1:
            self.target = argv[1]
        return self

    @property
    def name(self) -> str:
        return self.key

    @property
    def argv(self) -> list[str]:
        if self.check_argv is not None:
            return self.check_argv
        if self.kind == "tool":
            return [sys.executable, self.target, "--selftest"]
        return [sys.executable, self.target]

    def both_paths(self) -> list[str]:
        return [p for p in (self.tool, self.standalone) if p]


def _rel(path: str, root: str = ROOT) -> str:
    return os.path.relpath(path, root).replace("\\", "/")


def _key_for(path: str, root: str = ROOT) -> str:
    """`tools/units/queue_selftest.py` and `tools/units/queue.py` -> `tools/units/queue`."""
    rel = _rel(path, root)
    base = os.path.basename(rel)[:-3]  # strip .py
    if base.endswith("_selftest"):
        base = base[: -len("_selftest")]
    return os.path.join(os.path.dirname(rel), base).replace("\\", "/")


def _read(path: str) -> str:
    try:
        with open(path, encoding="utf-8", errors="replace") as fh:
            return fh.read()
    except OSError:
        return ""


def _tool_delegates_to_standalone(tool_text: str, key: str) -> bool:
    base = os.path.basename(key)
    pats = (
        r"(?:^|\n)\s*(?:import|from)\s+%s_selftest\b" % re.escape(base),
        r"\b%s_selftest\.selftest\s*\(" % re.escape(base),
        r"\b%s_selftest\.main\s*\(" % re.escape(base),
        r"['\"]%s_selftest\.py['\"]" % re.escape(base),   # a subprocess call
    )
    return any(re.search(p, tool_text) for p in pats)


def _standalone_delegates_to_tool(st_text: str, key: str) -> bool:
    base = os.path.basename(key)
    return bool(re.search(r"\b%s\.selftest\s*\(" % re.escape(base), st_text))


def discover(root: str = ROOT) -> tuple[list[Entry], list[str]]:
    """Every tested tool, deduped, plus the notes describing each collapsed pair."""
    notes: list[str] = []
    tools: dict[str, str] = {}       # key -> tool path
    standalones: dict[str, str] = {}  # key -> selftest path
    for dirpath, dirnames, filenames in os.walk(os.path.join(root, "tools")):
        dirnames[:] = [d for d in dirnames if d != "__pycache__"]
        for fn in filenames:
            if not fn.endswith(".py"):
                continue
            full = os.path.join(dirpath, fn)
            if fn.endswith("_selftest.py"):
                standalones[_key_for(full, root)] = _rel(full, root)
            elif SELFTEST_FLAG.search(_read(full)):
                tools[_key_for(full, root)] = _rel(full, root)

    entries: list[Entry] = []
    for key in sorted(set(tools) | set(standalones)):
        tool, st = tools.get(key), standalones.get(key)
        if tool and st:
            tt, stt = _read(os.path.join(root, tool)), _read(os.path.join(root, st))
            if _tool_delegates_to_standalone(tt, key):
                note = "%s --selftest delegates to %s - ran the tool, skipped the standalone" % (tool, st)
                entries.append(Entry(key, tool, st, note))
                notes.append(note)
            elif _standalone_delegates_to_tool(stt, key):
                note = "%s delegates to %s --selftest - ran the tool, skipped the standalone" % (st, tool)
                entries.append(Entry(key, tool, st, note))
                notes.append(note)
            else:
                note = ("%s and %s pin different checks (neither delegates) - ran both"
                        % (tool, st))
                entries.append(Entry(key, tool, st, note))
                notes.append(note)
                # a complementary pair is two entries: the standalone on its own key
                entries.append(Entry(st[:-3].replace("\\", "/"), None, st, "complementary half of " + note))
        elif tool:
            entries.append(Entry(key, tool, None))
        else:
            entries.append(Entry(key, None, st))
    entries.sort(key=lambda e: e.name)
    return entries, notes


def parse_checks(text: str) -> int | None:
    for pat in COUNT_PATTERNS:
        m = pat.search(text or "")
        if m:
            try:
                return int(m.group(1))
            except ValueError:
                pass
    # fallback: tools that print one `ok`/`FAIL` line per check but no total (`metric_selftest.py`,
    # `flipcheck_selftest.py`, `shapes_selftest.py`).  The explicit summary above always wins, so a tool
    # that prints `ok - N checks` is never counted line-by-line.
    per_line = sum(1 for ln in (text or "").splitlines()
                   if re.match(r"^\s*(ok|FAIL)\b", ln, re.I))
    return per_line or None


def _kill_tree(proc: subprocess.Popen) -> None:
    if os.name == "nt":
        subprocess.run(["taskkill", "/F", "/T", "/PID", str(proc.pid)], capture_output=True)
    else:
        try:
            os.killpg(os.getpgid(proc.pid), 9)
        except OSError:
            proc.kill()


def run_one(entry: Entry, timeout: float, root: str = ROOT) -> dict:
    """Run one selftest with a hard timeout; kill the whole tree on expiry."""
    started = time.time()
    kwargs: dict = dict(stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
                        encoding="utf-8", errors="replace", cwd=root)
    # every child Python loads `selftest_site/sitecustomize.py`: a launch refused with WinError 5 is retried
    env = dict(os.environ)
    env["PYTHONPATH"] = os.pathsep.join(p for p in (SITE_DIR, env.get("PYTHONPATH")) if p)
    kwargs["env"] = env
    if os.name != "nt":
        kwargs["start_new_session"] = True
    else:
        kwargs["creationflags"] = subprocess.CREATE_NEW_PROCESS_GROUP
    proc = subprocess.Popen(entry.argv, **kwargs)
    timed_out = False
    try:
        out, _ = proc.communicate(timeout=timeout)
        rc = proc.returncode
    except subprocess.TimeoutExpired:
        timed_out = True
        _kill_tree(proc)
        try:
            out, _ = proc.communicate(timeout=30)
        except subprocess.TimeoutExpired:
            out = ""
        rc = -1
    return {
        "name": entry.name,
        "target": entry.target,
        "kind": entry.kind,
        "argv": " ".join(entry.argv[1:]),
        "returncode": rc,
        "status": "timeout" if timed_out else ("pass" if rc == 0 else "fail"),
        "checks": None if timed_out else parse_checks(out),
        "duration_s": round(time.time() - started, 2),
        "output": out or "",
    }


def tail_lines(text: str, n: int = FAIL_TAIL_LINES) -> str:
    """The last `n` lines of `text` (a traceback and a `FAIL` line are at the end, not the head)."""
    return "\n".join((text or "").splitlines()[-n:])


def log_flake(path: str, r: dict, retry: dict) -> None:
    """Append one flake record; never let a logging problem change the verdict."""
    rec = {"tool": r["name"], "time": time.strftime("%Y-%m-%dT%H:%M:%S"), "first_status": r["status"],
           "first_returncode": r["returncode"], "first_duration_s": r["duration_s"],
           "retry_duration_s": retry["duration_s"], "first_failure_tail": tail_lines(r["output"])}
    try:
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, "a", encoding="utf-8") as fh:
            fh.write(json.dumps(rec) + "\n")
    except OSError as exc:
        print("selftest: could not record the flake in %s: %s" % (path, exc), file=sys.stderr)


def retry_failed(results: list[dict], entries: list[Entry], timeout: float, root: str = ROOT,
                 flake_log: str | None = None, runner=None) -> list[dict]:
    """Re-run every failed/timed-out selftest ONCE, serially (alone: the pool has drained).

    Passing on the re-run marks the result `flaky` (status pass, the first failure kept in `first_failure`);
    failing again keeps the *second* run's output (the isolated one) and records `attempts: 2`. A parked
    tool is re-run too - the park is decided later, from the final status - at the cost of one extra run.
    """
    runner = runner or run_one
    by_name = {e.name: e for e in entries}
    for i, r in enumerate(results):
        if r["status"] not in ("fail", "timeout"):
            continue
        again = runner(by_name[r["name"]], timeout, root)
        again["attempts"] = 2
        again["first_failure"] = tail_lines(r["output"])
        if again["status"] == "pass":
            again["flaky"] = True
            again["first_status"] = r["status"]
            print("flaky: %s passed on isolated re-run (first run: %s, exit %s) - recorded in %s"
                  % (r["name"], r["status"], r["returncode"], FLAKE_LOG_REL.replace(os.sep, "/")),
                  file=sys.stderr)
            for ln in tail_lines(r["output"], 12).splitlines():
                print("      " + ln, file=sys.stderr)
            log_flake(flake_log or os.path.join(root, FLAKE_LOG_REL), r, again)
        results[i] = again
    return results


def git_status(root: str) -> list[str]:
    try:
        p = subprocess.run(["git", "status", "--porcelain"], cwd=root, capture_output=True,
                           text=True, encoding="utf-8", errors="replace")
    except OSError:
        return []
    if p.returncode != 0:
        return []
    return [ln for ln in p.stdout.splitlines() if ln.strip()]


#: The one file that IS the slot concurrency cap, and is invisible to `git status` (`.pi/` is gitignored).
POOL_MANIFEST_REL = os.path.join(".pi", "slots", "pool.json")


def pool_manifest_bytes(root: str) -> bytes | None:
    """The live slot manifest's bytes, or None when there is none.

    Captured before and after the whole run and required to be identical: a selftest that rewrites
    `.pi/slots/pool.json` silently changes the campaign's concurrency cap while live lanes sit in the pool,
    which is exactly the phantom "the pool shrank" this guard turns into a one-line failure.
    """
    try:
        with open(os.path.join(root, POOL_MANIFEST_REL), "rb") as fh:
            return fh.read()
    except OSError:
        return None


def load_parks(path: str = PARK_FILE) -> tuple[list[dict], str | None]:
    if not os.path.exists(path):
        return [], None
    try:
        data = json.load(open(path, encoding="utf-8"))
    except (OSError, ValueError) as exc:
        return [], "park list %s is unreadable: %s" % (_rel(path), exc)
    parks = data.get("parked", data) if isinstance(data, dict) else data
    if not isinstance(parks, list):
        return [], "park list %s has no `parked` array" % _rel(path)
    return parks, None


def _park_matches(park: dict, entry: Entry) -> bool:
    for field in ("target", "name"):
        v = park.get(field)
        if not v:
            continue
        v = v.replace("\\", "/")
        if v in (entry.name, entry.target) or v in entry.both_paths():
            return True
    return False


def mapped_entries(changed: list[str]) -> list[str]:
    """Selftest entry keys a diff's non-`tools/` sources select (F37), in table order.

    Pure - no git, no filesystem - so `--selftest` can pin the mapping without a repository.
    """
    out: list[str] = []
    for src in changed:
        for key in _source_lookup(SOURCE_ENTRIES, src):
            if key not in out:
                out.append(key)
    return out


def mapped_checks(changed: list[str], ref: str = "HEAD") -> list[tuple[str, list[str]]]:
    """`(entry name, argv tail)` for every non-`tools/` source that owns a `--check` (F37).

    The name is what the table prints and the park list matches (`<tool> --check`); the argv tail is
    relative to the root the run uses as cwd. A `{ref}` in the argv is the diff base the run compares against
    (`ideas.py demo-check --changed {ref}` re-compiles exactly the demos that diff touches).
    """
    out, seen = [], set()
    for src in changed:
        for rel, extra in _source_lookup(SOURCE_CHECKS, src):
            extra = tuple(x.replace("{ref}", ref) for x in extra)
            name = "%s %s" % (rel, " ".join(extra))
            if name in seen:
                continue
            seen.add(name)
            out.append((name, [rel, *extra]))
    return out


def changed_entries(entries: list[Entry], ref: str | None, root: str) -> tuple[list[Entry], list[str]]:
    """The selftests of the tools a diff touches, plus the checks a non-`tools/` source owns (F37); also
    the paths no selftest claims."""
    rng = ref or "HEAD"
    p = subprocess.run(["git", "diff", "--name-only", rng], cwd=root, capture_output=True,
                       text=True, encoding="utf-8", errors="replace")
    if p.returncode != 0:
        print("selftest: `git diff --name-only %s` failed: %s" % (rng, (p.stderr or "").strip()),
              file=sys.stderr)
        return [], []
    changed = [ln.strip().replace("\\", "/") for ln in p.stdout.splitlines() if ln.strip()]
    # a new tool file is not in `git diff` until it is staged: include untracked files too, so a lane's
    # brand-new tool or selftest is covered by the fast loop before its first commit.
    u = subprocess.run(["git", "ls-files", "--others", "--exclude-standard"], cwd=root,
                       capture_output=True, text=True, encoding="utf-8", errors="replace")
    if u.returncode == 0:
        changed += [ln.strip().replace("\\", "/") for ln in u.stdout.splitlines() if ln.strip()]
    changed = sorted(set(changed))
    changed_set = set(changed)
    changed_mods = {os.path.basename(c)[:-3] for c in changed if c.endswith(".py")}

    picked: list[Entry] = []
    claimed: set[str] = set()
    for entry in entries:
        files = entry.both_paths() or [entry.target]
        # 1. the diff touches the tool's or the standalone's own file
        if changed_set & set(files):
            picked.append(entry)
            claimed |= set(files)
            continue
        # 2. the diff touches a module one of the entry's files imports
        if changed_mods:
            text = "".join(_read(os.path.join(root, f)) for f in files)
            imports = set(re.findall(r"(?:^|\n)\s*(?:import|from)\s+([A-Za-z_][\w]*)", text))
            if imports & changed_mods:
                picked.append(entry)
                claimed |= set(files)

    # 3. a diff outside `tools/` whose generated copies this runner would otherwise never check (F37):
    #    `docs/plan.md` selects the tool whose selftest validates the generated block against it, and a
    #    source with no selftest entry of its own runs that tool's `--check` as a synthetic entry.
    names = {e.name for e in picked}
    by_key = {e.key: e for e in entries}
    for key in mapped_entries(changed):
        entry = by_key.get(key)
        if entry is not None and entry.name not in names:
            picked.append(entry)
            names.add(entry.name)
    for name, tail in mapped_checks(changed, rng):
        if name in names:
            continue
        picked.append(Entry(name, None, None).as_check([sys.executable, *tail]))
        names.add(name)
    picked.sort(key=lambda e: e.name)
    unclaimed = [c for c in changed if c not in claimed and c.endswith(".py")]
    return picked, unclaimed


def format_table(results: list[dict]) -> str:
    width = max([len(r["name"]) for r in results] + [4])
    lines = ["%-6s %-*s %6s %9s" % ("STATUS", width, "TOOL", "CHECKS", "TIME")]
    order = {"fail": 0, "timeout": 0, "stale": 1, "parked": 2, "pass": 3}
    for r in sorted(results, key=lambda r: (order.get(r["status"], 9), r["name"])):
        checks = "-" if r["checks"] is None else str(r["checks"])
        lines.append("%-6s %-*s %6s %8ss" % (r["status"].upper(), width, r["name"], checks,
                                             r["duration_s"]))
    return "\n".join(lines)


def selftest() -> int:
    """The runner's own checks - discovery/dedupe, the count parser, park matching, the `--changed`
    mapping - on fixtures and this tree's inventory.

    `tools/selftest.py` exposes `--selftest`, so `discover` finds it and the suite runs these checks as one
    more entry. They never **run** git and never spawn a repository test: the discovery fixtures are a temp
    `tools/` tree, the `--changed` composition replaces `subprocess.run`, and the only processes started are the
    two throwaway scripts of the re-run fixture, inside a temp dir.
    """
    import io
    import tempfile
    fails, checks = [], 0

    def check(name, got, want):
        nonlocal checks
        checks += 1
        if got != want:
            fails.append("%s: got %r want %r" % (name, got, want))

    # the check-count parser covers every shape the suite prints
    check("`ok - N checks`", parse_checks("ok - 12 checks"), 12)
    check("`N/M checks passed`", parse_checks("declclash: 7/9 checks passed"), 9)
    check("`all N checks passed`", parse_checks("all 6 checks passed"), 6)
    check("per-line ok/FAIL fallback", parse_checks("ok  a\n  ok   b\nFAIL c"), 3)
    check("an unnumbered `selftest: OK` has no count", parse_checks("selftest: OK"), None)

    with tempfile.TemporaryDirectory() as tmp:
        def w(rel, text=""):
            p = os.path.join(tmp, rel)
            os.makedirs(os.path.dirname(p), exist_ok=True)
            with open(p, "w", encoding="utf-8") as fh:
                fh.write(text)

        # a: the tool delegates to the standalone -> one tool entry
        w("tools/units/alpha.py", 'ap.add_argument("--selftest")\nimport alpha_selftest\n')
        w("tools/units/alpha_selftest.py", "def selftest():\n    return 0\n")
        # b: the standalone delegates to the tool -> one tool entry
        w("tools/units/beta.py", 'ap.add_argument("--selftest")\ndef selftest():\n    return 0\n')
        w("tools/units/beta_selftest.py", "import beta\nbeta.selftest()\n")
        # g: neither delegates -> both entries (complementary, e.g. ledger)
        w("tools/units/gamma.py", 'ap.add_argument("--selftest")\ndef selftest():\n    return 0\n')
        w("tools/units/gamma_selftest.py", "def selftest():\n    return 0\n")
        # d: tool only, e: standalone only
        w("tools/units/delta.py", 'ap.add_argument("--selftest")\n')
        w("tools/units/eps_selftest.py", "def selftest():\n    return 0\n")
        entries, notes = discover(tmp)
        by = {e.name: e for e in entries}
        check("a tool->standalone pair collapses", sorted(by),
              ["tools/units/alpha", "tools/units/beta", "tools/units/delta", "tools/units/eps",
               "tools/units/gamma", "tools/units/gamma_selftest"])
        check("... and keeps the tool, not the standalone", by["tools/units/alpha"].kind, "tool")
        check("a standalone->tool pair keeps the tool", by["tools/units/beta"].kind, "tool")
        check("a complementary pair keeps both", by["tools/units/gamma_selftest"].kind, "standalone")
        check("a tool-only entry is a standalone run of itself", by["tools/units/delta"].kind, "tool")
        check("a standalone-only entry has no tool", by["tools/units/eps"].kind, "standalone")
        check("a tool entry runs with --selftest", by["tools/units/alpha"].argv[-1], "--selftest")
        check("a standalone entry runs the file", by["tools/units/eps"].argv[-1],
              "tools/units/eps_selftest.py")

        # the park list matches either the tool or its standalone sibling
        alpha = by["tools/units/alpha"]
        check("a park on the standalone matches the tool entry",
              _park_matches({"target": "tools/units/alpha_selftest.py"}, alpha), True)
        check("a park on the tool matches", _park_matches({"target": "tools/units/alpha.py"}, alpha), True)
        check("a park on an unrelated target does not match",
              _park_matches({"target": "tools/units/zeta_selftest.py"}, alpha), False)

    # --- F37: a diff outside `tools/` still has to select the check that owns its invariant --------------
    check("docs/plan.md selects the sync_profiles selftest", mapped_entries(["docs/plan.md"]),
          ["tools/agents/sync_profiles"])
    check("a tools-only diff maps to no extra entry", mapped_entries(["tools/units/measure.py"]), [])
    check("a Windows-style path maps identically", mapped_entries(["docs\\plan.md"]),
          ["tools/agents/sync_profiles"])
    md_checks = mapped_checks(["docs/matching.md"])
    check("docs/matching.md selects the skill's sync_reference check and the playbook-index check",
          [name for name, _tail in md_checks], [SR_REL + " --check", "tools/agents/sync_playbook_index.py --check"])
    check("docs/matching.md also selects the playbook-index selftest", mapped_entries(["docs/matching.md"]),
          ["tools/agents/sync_playbook_index"])
    check("a skill SKILL.md edit selects the reference check and the index selftest",
          ([n for n, _t in mapped_checks([".claude/skills/mwcc-unit-matching/SKILL.md"])],
           mapped_entries([".claude/skills/mwcc-unit-matching/SKILL.md"])),
          ([SR_REL + " --check"], ["tools/agents/sync_playbook_index"]))
    check("an edit under docs/matching/ selects the index selftest, the index check and the skill copy check",
          ([n for n, _t in mapped_checks(["docs/matching/043-pool-off-string-addressing.md"])],
           mapped_entries(["docs/matching/index.md"])),
          ([SR_REL + " --check", "tools/agents/sync_playbook_index.py --check", "tools/agents/ideas.py check",
            "tools/agents/ideas.py demo-check --changed HEAD"],
           ["tools/agents/sync_playbook_index", "tools/agents/ideas"]))
    check("the demo check is handed the diff base it runs against",
          [t for n, t in mapped_checks(["docs/matching/034-switch-tail-negated-arms.cpp"], "main")
           if "demo-check" in n], [["tools/agents/ideas.py", "demo-check", "--changed", "main"]])
    check("an edit under the skill's references/matching/ selects all three checks",
          [n for n, _t in mapped_checks([SKILL_MATCHING + "index.md"])],
          [SR_REL + " --check", "tools/agents/sync_playbook_index.py --check", "tools/agents/ideas.py check"])
    check("a file next to docs/matching/ (not under it) selects nothing", mapped_checks(["docs/matching-other.md"]), [])
    check("... and the argv runs that tool with --check",
          md_checks[0][1] if md_checks else None, [SR_REL, "--check"])
    check("a source with no mapped check maps to nothing", mapped_checks(["docs/plan.md"]), [])

    # the composition, with git replaced: a `docs/*.md`-only diff must select a runnable entry each
    entries, _ignore = discover(ROOT)
    real_run = subprocess.run

    def fake_git(changed):
        def run(argv, **kwargs):
            if list(argv[:3]) == ["git", "diff", "--name-only"]:
                return subprocess.CompletedProcess(argv, 0, changed + "\n", "")
            if list(argv[:2]) == ["git", "ls-files"]:
                return subprocess.CompletedProcess(argv, 0, "", "")
            return real_run(argv, **kwargs)
        return run

    try:
        for source, want in (("docs/plan.md", [("tools/agents/sync_profiles", "tool", "--selftest")]),
                             ("docs/matching/README.md", [(SR_REL + " --check", "check", "--check"),
                                               ("tools/agents/ideas", "tool", "--selftest"),
                                               ("tools/agents/ideas.py check", "check", "check"),
                                               ("tools/agents/ideas.py demo-check --changed HEAD", "check", "HEAD"),
                                               ("tools/agents/sync_playbook_index", "tool", "--selftest"),
                                               ("tools/agents/sync_playbook_index.py --check", "check", "--check")]),
                             ("docs/matching.md", [(SR_REL + " --check", "check", "--check"),
                                               ("tools/agents/sync_playbook_index", "tool", "--selftest"),
                                               ("tools/agents/sync_playbook_index.py --check", "check", "--check")])):
            subprocess.run = fake_git(source)
            picked, unclaimed = changed_entries(entries, "HEAD", ROOT)
            check("a %s-only diff selects %s" % (source, want[0][0]),
                  [(e.name, e.kind, e.argv[-1]) for e in picked], want)
            check("... and claims it (nothing unclaimed)", unclaimed, [])
    finally:
        subprocess.run = real_run

    # --- the isolated re-run (2026-09-30): a fixture tool that fails once then passes is a FLAKE (row passes,
    # warning + log line); one that always fails stays red and its last lines are kept. Two tiny scripts
    # run for real (a subprocess each), in a temp dir, with the flake log pointed there too.
    with tempfile.TemporaryDirectory() as tmp:
        marker = os.path.join(tmp, "ran-once")
        flaky_py = os.path.join(tmp, "flaky_tool.py")
        with open(flaky_py, "w", encoding="utf-8") as fh:
            fh.write("import os, sys\nm = %r\nif not os.path.exists(m):\n    open(m, 'w').close()\n"
                     "    print('first run boom')\n    sys.exit(1)\nprint('ok - 3 checks')\n" % marker)
        red_py = os.path.join(tmp, "red_tool.py")
        with open(red_py, "w", encoding="utf-8") as fh:
            fh.write("import sys\nfor i in range(60):\n    print('line %d' % i)\nprint('FAIL always')\nsys.exit(1)\n")
        flaky_e = Entry("fixture/flaky", None, None).as_check([sys.executable, flaky_py])
        red_e = Entry("fixture/red", None, None).as_check([sys.executable, red_py])
        log = os.path.join(tmp, ".pi", "selftest-flakes.jsonl")
        first = [run_one(flaky_e, 60, tmp), run_one(red_e, 60, tmp)]
        check("the fixture flaky tool fails the first time", first[0]["status"], "fail")
        real_stderr, sys.stderr = sys.stderr, io.StringIO()
        try:
            res = retry_failed(first, [flaky_e, red_e], 60, tmp, flake_log=log)
            warned = sys.stderr.getvalue()
        finally:
            sys.stderr = real_stderr
        check("a fail-once-then-pass tool passes the row", res[0]["status"], "pass")
        check("... and is marked flaky", res[0].get("flaky"), True)
        check("... with a loud warning", "flaky: fixture/flaky passed on isolated re-run" in warned, True)
        check("... that shows the first failure", "first run boom" in warned, True)
        with open(log, encoding="utf-8") as fh:
            rows = [json.loads(ln) for ln in fh if ln.strip()]
        check("... and one flake line is logged with tool, time and the first failure's tail",
              [(r["tool"], bool(r["time"]), "first run boom" in r["first_failure_tail"]) for r in rows],
              [("fixture/flaky", True, True)])
        check("an always-failing tool stays red after the re-run", res[1]["status"], "fail")
        check("... is not flaky and records two attempts", (res[1].get("flaky"), res[1]["attempts"]), (None, 2))
        check("... and keeps only the last FAIL_TAIL_LINES lines, ending on the failure",
              tail_lines(res[1]["output"]).splitlines()[-1], "FAIL always")
        check("... exactly that many", len(tail_lines(res[1]["output"]).splitlines()), FAIL_TAIL_LINES)
        check("a passing tool is never re-run", retry_failed([dict(res[0], status="pass", flaky=None)],
                                                             [flaky_e], 60, tmp, flake_log=log,
                                                             runner=lambda *a: 1 / 0)[0]["status"], "pass")

    for f in fails:
        print("FAIL " + f)
    print("ok - %d checks" % checks)
    return 1 if fails else 0


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0],
                                 formatter_class=argparse.RawDescriptionHelpFormatter,
                                 epilog="Exit status is the answer: 0 = green (parks aside).")
    ap.add_argument("--selftest", action="store_true",
                    help="run this runner's own checks (it is discovered, so the suite tests itself too)")
    ap.add_argument("--json", action="store_true", help="machine-readable summary on stdout")
    ap.add_argument("--changed", nargs="?", const="HEAD", default=None, metavar="REF",
                    help="only the selftests of tools this diff touches (default REF: HEAD)")
    ap.add_argument("--list", action="store_true", help="print the inventory and exit")
    ap.add_argument("--no-dedupe", action="store_true",
                    help="run both halves of every wrapper pair (the standalone checks too)")
    ap.add_argument("--timeout", type=float, default=300.0,
                    help="per-test timeout in seconds (default 300)")
    ap.add_argument("--jobs", type=int, default=min(8, (os.cpu_count() or 4)),
                    help="parallel workers (default min(8, cpus))")
    ap.add_argument("--root", default=ROOT, help="repository root (default: this file's parent)")
    ap.add_argument("--park-file", default=PARK_FILE, help="known-failures list")
    a = ap.parse_args(argv)

    if a.selftest:
        return selftest()

    root = os.path.abspath(a.root)
    entries, notes = discover(root)
    if a.no_dedupe:
        # re-expand: keep every tool entry and add the standalone sibling back on its own key
        expanded: list[Entry] = []
        seen: set[str] = set()
        for e in entries:
            if e.key not in seen:
                expanded.append(e)
                seen.add(e.key)
            if e.standalone and e.kind == "tool" and e.standalone not in seen:
                expanded.append(Entry(e.standalone[:-3].replace("\\", "/"), None, e.standalone,
                                      "explicit --no-dedupe half of " + e.key))
                seen.add(e.standalone)
        entries = sorted(expanded, key=lambda e: e.name)

    if a.changed is not None:
        entries, unclaimed = changed_entries(entries, a.changed, root)
        for path in unclaimed:
            print("note: %s changed but no selftest targets it" % path, file=sys.stderr)

    if a.list:
        for e in entries:
            note = ("  (%s)" % e.dedupe_note) if e.dedupe_note else ""
            print("%-58s %-10s %s%s" % (e.name, e.kind, e.target, note))
        print("%d selftest(s)" % len(entries))
        return 0

    parks, park_err = load_parks(a.park_file)
    if park_err:
        print("FAIL park list: %s" % park_err, file=sys.stderr)
        return 2

    dirty_before = git_status(root)
    pool_before = pool_manifest_bytes(root)
    started = time.time()
    with concurrent.futures.ThreadPoolExecutor(max_workers=max(1, a.jobs)) as pool:
        results = list(pool.map(lambda e: run_one(e, a.timeout, root), entries))
    results = retry_failed(results, entries, a.timeout, root)
    wall = time.time() - started
    dirty_after = git_status(root)
    pool_after = pool_manifest_bytes(root)

    entry_by_name = {e.name: e for e in entries}
    # classify failures against the park list
    parked_count = 0
    stale_parks: list[str] = []
    failed: list[dict] = []
    matched_parks: set[int] = set()
    for r in results:
        if r["status"] in ("fail", "timeout"):
            hit = [i for i, p in enumerate(parks) if _park_matches(p, entry_by_name[r["name"]])]
            if hit:
                r["status"] = "parked"
                r["reason"] = parks[hit[0]].get("reason", "")
                matched_parks.update(hit)
                parked_count += 1
            else:
                failed.append(r)
        elif r["status"] == "pass":
            hit = [i for i, p in enumerate(parks) if _park_matches(p, entry_by_name[r["name"]])]
            if hit:
                r["status"] = "stale"
                r["reason"] = "parked %s but now passes - unpark it" % (
                    parks[hit[0]].get("date", "?"))
                matched_parks.update(hit)
                stale_parks.append(r["name"])

    unmatched_parks = [p for i, p in enumerate(parks) if i not in matched_parks]
    # a park for a test a `--changed` subset did not run is not stale - only the full run can say a tool's
    # test has vanished. A park whose test ran and PASSED is always stale, subset or not.
    if a.changed is None:
        for p in unmatched_parks:
            stale_parks.append("park entry %s (%s) matches no discovered selftest"
                               % (p.get("target") or p.get("name"), p.get("date", "?")))

    tree_ok = dirty_before == dirty_after
    pool_ok = pool_before == pool_after
    offenders = []
    if not tree_ok:
        before, after = set(dirty_before), set(dirty_after)
        offenders = sorted(after - before) + ["removed: " + x for x in sorted(before - after)]
    if not pool_ok:
        offenders.append("the live slot manifest %s changed during the run (it IS the concurrency cap)"
                         % POOL_MANIFEST_REL)
        tree_ok = False

    passed = sum(1 for r in results if r["status"] == "pass")
    total_checks = sum(r["checks"] or 0 for r in results)
    green = not failed and tree_ok and not stale_parks

    if a.json:
        payload = {
            "root": root,
            "total": len(results),
            "passed": passed,
            "failed": len(failed),
            "parked": parked_count,
            "stale_parks": stale_parks,
            "checks": total_checks,
            "wall_s": round(wall, 2),
            "timeout_s": a.timeout,
            "jobs": a.jobs,
            "tree_clean": tree_ok,
            "tree_offenders": offenders,
            "pool_manifest_unchanged": pool_ok,
            "green": green,
            "results": [{k: v for k, v in r.items() if k != "output"} for r in results],
            "dedupe_notes": notes,
            "failures": [{"name": r["name"], "status": r["status"], "returncode": r["returncode"],
                          "head": "\n".join((r["output"] or "").splitlines()[:25]),
                          "tail": tail_lines(r["output"])} for r in failed],
            "flaky": [{"name": r["name"], "first_status": r.get("first_status"),
                       "first_failure": r.get("first_failure", "")} for r in results if r.get("flaky")],
        }
        print(json.dumps(payload, indent=2))
        return 0 if green else 1

    print(format_table(results))
    print("")
    for r in failed:
        print("FAIL %s (%s, exit %s)" % (r["name"], r["target"], r["returncode"]))
        for line in tail_lines(r["output"]).splitlines():
            print("    " + line)
        print("")
    if not tree_ok:
        print("FAIL the tree changed under the run - a selftest wrote into the repository:")
        for row in offenders[:20]:
            print("    " + row)
        print("")
    for name in stale_parks:
        print("STALE %s" % name)

    summary = "%d selftest(s): %d passed, %d failed" % (len(results), passed, len(failed))
    flaky_n = sum(1 for r in results if r.get("flaky"))
    if flaky_n:
        summary += ", %d FLAKY (passed on isolated re-run)" % flaky_n
    if parked_count:
        summary += ", %d parked (known)" % parked_count
    if stale_parks:
        summary += ", %d stale park(s)" % len(stale_parks)
    summary += " in %.1fs" % wall
    print(summary)
    if green:
        print("GREEN" + (" except %d parked" % parked_count if parked_count else ""))
    else:
        print("RED")
    return 0 if green else 1


if __name__ == "__main__":
    sys.exit(main())
