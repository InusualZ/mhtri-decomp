"""The land gate: one command that runs the batch checklist and refuses to let a bad batch through.

docs/plan.md 7.5 + 7.16, §11. Six manual commands and a regression-scan heredoc were the previous version of
this, rewritten every batch - and that is where a mistake hides. It is also the place where a green
`ninja build/RMHE08/ok` can lie: the `ok` stamp file may be from an earlier run, and a `NonMatching` batch
never relinks, so `main.elf` never runs and `ok` is the only edge that re-validates anything. So `verify`

* deletes `build/RMHE08/ok` (and `main.elf` when the batch flips an object) **before** the run and requires
  them to be recreated;
* **compiles the batch's own units** (`ninja -k 0`, scoped to `build/RMHE08/src/<unit>.o` for each `--units`
  entry) before the DOL check proves anything: a `NonMatching` unit's object is never linked, so `ok` stayed
  green **twice in one session** (2026-09-26) with a unit in the tree that did not compile - once a partial
  file from a malformed cherry-pick, once a declaration moved out from under three call sites. `ok` answers
  "is the DOL still the DOL"; this answers "does our source still compile". A **foreign** dirty object that
  fails is named and tolerated - it is not this batch's defect, and the scoping is by object target, so the
  check cannot make a passing gate fail for another stream's work;
* **refuses a unit registered in name only** (2026-09-26): a source file can be committed while its
  `configure.py` `Object(...)` line and its `splits.txt` block are left behind, so the unit is absent
  from the build while `ok` stays green - and the compile gate cannot see it either, because a unit with
  no `configure.py` line has no `build/RMHE08/src/<unit>.o` target to scope to. Every batch unit must
  carry all three: an `Object(...)` line, a `splits.txt` block, and an object target in the build graph.
  `tools/units/verifyunit.py` owns the check;
* **re-measures per symbol instead of trusting `report.json`** (2026-09-26): the regression scan reads
  the same report the batch was measured against, so a stale or wrong report is invisible to it. The
  gate re-runs `objdiff report generate` over the objects, compares symbol-for-symbol against the
  report, checks a 100 % claim against the raw bytes, and reproduces the unit's `fuzzy_match_percent`
  from its listed partials (a function with no `fuzzy_match_percent` key is **0 %**, not 100 %) - the
  independent ordering the merger lane used, not the report's own consumer;
* **refuses a split target object that moved for a unit the batch does not name**: target objects come
  from the DOL split, so a `splits.txt` change that re-ranges a neighbour moves that neighbour's object.
  The gate hashes the registered units' split targets before and after the build
  (`.pi/notes/8031a6c0-fn-8031a6c0-e199.md` is the standard), rather than assuming the re-range harmless;
* checks every command's exit code, `configure.py`'s included - a failed `configure.py` leaves a stale
  `build.ninja` and every later number is a fiction;
* **refuses a batch that moves the ground truth, that moved `main` since the batch base, that touches a file
  outside the batch's expected set, or whose outbox entry does not validate**; only the batch's unit-shaped
  entries are looked up in the outbox (`unit_rows`) - a `--units` entry that names a staged *path* (a header,
  `LICENSE`, `docs`) has no outbox of its own and must not be validated as if it were a unit
  (2026-09-28, a header named in `--units` was demanded `residual`/`flags_probed` and bounced the whole
  landing); a foreign path **already in the tree** is reported - with a likely cause when it looks like lane
  scratch (`.tmp-*`, `.ws-*`, an `upstream/`
  clone) - **before** the expensive gate runs (`preflight_foreign`), not only as the refusal afterwards, so a
  mis-launched lane's leftovers cost a second, not a 5-minute build;
* **checks rule 10 like every other rule** (`vtableaudit.py`): a table of code pointers inside a unit's own
  ranges must be compiler output, so the row is add-only - exactly like the lint's `--diff`, because the
  tree already carries violations - and it PRINTS the violation set for the batch's units even when it
  passes. The rule used to be a "landing-review rule" (a habit), and
  `Network/fn_803D3CE8.cpp`'s two `self->vtable = &NetworkSessionManagerVTable;` writes survived a landing
  through it (2026-09-27); a silent pass is what that classification bought, so a silent pass is gone;
* checks rule 12 in the style lint, with a recorded allowance (`--allow-rule12 <token>`) for a lane that
  needs an unowned-data `extern` **now** while the claim is already scheduled: repeatable, printed in the
  landing log with the token it excused, and any rule-12 addition the allowance does not name - or any
  other rule at all - still refuses. Like rule 10's, it is a command-line record, never a key in a file;
* runs the style lint when it exists (7.21), reports the ledger delta, and warns when a unit improved with no
  document or header change to show for it (7.10); the lint row carries the **head** of stylelint's output -
  where the findings are - and never its trailing "not enforced: ..." legend, which on 2026-09-25 made a FAIL
  row read as a pass (`.pi/land.log`: `FAIL style lint (§6.5) adds no violation - (temporary grandfather:
  legacy scaffolding with bodies, ...)`);
* runs `tools/selftest.py` - the one runner for every tool's `--selftest` and every `*_selftest.py`, parked
  pre-existing failures aside - as the "all tool selftests pass" row, so a tool's own test that nothing runs
  cannot hide (the `measure_selftest.py` was red for weeks while 31 lanes filed "recompile.py is broken"
  incident); `--no-selftests` is the fast path.
* refreshes the baseline afterwards (7.16), so `ninja changes` compares against the batch that just landed;
* and **releases the claim of every unit it just gated** (owner's rule, "Teardown is part of landing"): a
  landed unit must not leave a worktree, a merged branch or a registry entry behind. A release that is
  incomplete (a live pane, a worktree that would not go) fails the gate and names what held it; `--no-release`
  turns the step off for an orchestrator-only batch.

    python tools/units/land.py record-base [--json]
    python tools/units/land.py land --units a,b [--base SHA] [--no-build] [--no-outbox] [--no-release]
                                  [--no-selftests] [--already-applied]
    python tools/units/land.py land --branch worker/<slug> [--units a,b] [--base SHA] [--no-build]
                                  [--no-outbox] [--no-release] [--message SUBJECT] [--no-selftests]
    python tools/units/land.py verify [--base SHA] [--units a,b] [--dry-run] [--no-build] [--no-outbox]
                                  [--no-release] [--allow-regression UNIT] [--no-selftests]
    python tools/units/land.py resolve --branch worker/<slug> [--worktree PATH] [--main PATH] [--base SHA]
                                  [--no-commit] [--json]

`resolve` is the land path's one automatic conflict resolution.  Eight of thirteen live branches conflict
with `main` on a single, safe class: sibling bands register *adjacent address ranges*, so their
`config/RMHE08/splits.txt` blocks and `configure.py` `Object(...)` lines append at the same anchor.  That is
an add/add conflict whose resolution is the pure append-union - and `resolve` only ever touches those two
paths.  It runs in the branch's worktree or a **scratch** one, **never in MAIN** (a gate applying a diff
inside MAIN is what left MAIN conflicted on 2026-09-26).  The union is gated in order by the path scope,
`unionguard` (a delete, a rename, or both sides editing one region is refused by name) and the
`unionresolve` invariants (no duplicate unit key, no duplicate `Object()` line, no overlapping
`.text`/`extab`/`extabindex` range, and no registration `main` already had is dropped); the union is
computed in memory and the invariants asserted *before* anything is written.  Staging is explicit
(`git add -- <the two paths>`), never the untracked `.pi/bin/applybranch.sh`'s `git add -A`.  The helper
branch that parks the union (`land/resolve-<slug>-<pid>`, in the scratch worktree) is deleted by the landing
that lands the branch - visibly, and only when the helper's tip is provably contained by the branch or by
`main`; one carrying a hand fix the branch never took is refused loudly and left alone, because it may be the
only copy (two helpers outlived their batches on 2026-09-26).

`land --branch` is the one-command landing, so the orchestrator never assembles it by hand again.  It
refuses a dirty tree (with the exact clean commands), records the base on the clean tree **before** the
pick, applies the branch's own delta with three-way (`git apply -3`, which - unlike a cherry-pick - does
not lose the work a merge carried), resolves a registration conflict with `resolve`'s union instead of
aborting, runs the gate above (compile gate included), then commits and releases.  It is idempotent and
loud: any refusal leaves the tree as it was (the apply is undone) and prints one `REFUSED ...` line whose
exit status is the answer.

`land` is the one command and the one you should use: it runs `verify`, stages the batch's own files, commits
**with a pathspec** (`git commit -F msg -- <paths>`, so the whole index is never taken), releases the claims,
and prints a **single answer line** (`LANDED ...` / `REFUSED ...`) whose exit status is the answer. The gate log
goes to stderr, so piping stdout cannot lose the verdict - and a failed gate can never reach `git commit` (the
old flow wrote the message unconditionally, which is how a piped `| tail -3` committed a refused batch twice).
The pathspec is the other half of that safety: without it, another stream's *staged* edit was swept into the
batch's commit twice on 2026-09-23 (`85f3d4b5`, `d50fdd32` took `tools/units/langcheck.py`). Paths the index
holds but the batch does not are left staged and named in a warning.

The pathspec still takes every *allowed* dirty path, so an unrelated edit that was already sitting in the
working tree rode the next commit twice on 2026-09-24 (a prepared `docs/plan.md` under `85ddd7b6`, a
`src/RSO/runtime.c` header under `890631e8`). `record-base` now snapshots the paths already dirty when the
batch opens (`dirty_at_base`), and `land_stageable` excludes one unless the batch names it as a unit - the
snapshot is the only way to tell "already dirty at the base" from "dirty because of this batch".

The one outside-the-batch path this gate does not refuse is **tool scratch**: `d<digits>.json` /
`t<digits>.json` in the repo root, the objdiff `diff` dumps a tool leaves behind (`65492794` ignored them, but
a staged file bypasses `.gitignore` and the landing flow's own `git add -A` staged `d910.json`). The batch
never received them, so they are never staged, the gate de-indexes a staged copy (`git reset HEAD -- <path>`,
which leaves the caller's file in the tree), and the tolerance is **named** - in the gate log, in the check's
`info`, and in the landed message - rather than silently dropped. Every other outside-the-batch path is still
refused loudly: a foreign edit to `src/`, `include/`, `config/` or `configure.py` is exactly what the guard is
for, and a `d`/`t`-shaped name elsewhere is not a permission.

The regression scan is **per symbol**. A unit's `fuzzy` is an average over its symbols, so an already-registered
unit that is *extended* - its splits range widened, a head joined to its tail - falls in average as the weaker
new bodies join it. That is not a regression. The scan compares symbol to symbol: a symbol the previous report
did not hold is NEW (a body this batch added) and never a regression, however weak, and a symbol that dropped
refuses loudly with the symbol name and both scores. The unit-average comparison survives only for a unit that
did **not** grow, where no per-symbol row can name the loss (`report_regressions`).

`verify` never commits. It writes the message to `.git/land_msg.txt` **only when every check passed**, and
removes a stale one when it refuses; committing it stays a deliberate step for the rare manual case.

`--no-outbox` and `--no-release` are **separate** opt-outs: skipping the outbox/branch checks does not skip the
claim teardown (the old `--no-worker-units` did both, and a round that passed it left 18 worktrees and 3 dead
claims behind). `--no-worker-units` remains as an alias for `--no-outbox`.

Every check is classified by **KIND**, and a refusal says which kind failed, because the two need opposite
responses. A **GATE** check refuses because the *batch* is bad - the style lint, the regression scan, the
build/DOL hash, a path outside the batch, a genuine claim conflict - so the landing must stop and the batch
must be fixed. A **BOOKKEEPING** check refuses because the *landing's own state* is stale while the batch is
fine - a base that was never recorded (or has moved), a worker branch a `--force` release parked at a rescue
ref, an outbox the worker has to rewrite, a claim whose teardown did not finish - and the refusal prints the
exact remedy and **never** says "the gate failed". On 2026-09-26 the old shared wording ("the gate failed -
nothing staged or committed") made a bookkeeping refusal read as a substantive gate failure, and the reader
nearly overrode a real gate failure on another batch; the kind is now in the refusal line and in every
per-check line (`<check> [GATE|BOOKKEEPING]: <what it printed> (remedy: ...)`).

Two bookkeeping states that cost a manual commit the same day have their own handling:

* **a released branch (case (a))** - `release --force` deletes `worker/<label>` and parks its only copy of
the work at `refs/rescue/<label>`. The gate now restores the branch from that ref automatically
(`restore_rescued_branch`) instead of refusing a batch whose work is demonstrably preserved, and names what
it did;
* **an already-applied batch (case (b))** - if `record-base` ran *after* the batch was applied, the snapshot
it took recorded the batch's own edits as pre-existing, so `land_stageable` excludes them as foreign work.
When every changed path is in that snapshot, `land` says plainly that the batch is already applied and the
ordering was wrong; `--already-applied` stages those paths anyway, so the batch lands without a hand commit.
The ordering to prefer is still `record-base` on a clean tree *before* applying the batch.
"""

from __future__ import annotations

import argparse
import contextlib
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
#: The repository the tools layer lives in.  `land` always works on MAIN (`rc.main_root`, walked from
#: wherever the caller stands); this is the *source* tree, and only the selftest's own lint reads it.
SELF_REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.dirname(HERE))
sys.path.insert(0, os.path.dirname(HERE) + os.sep + "git")
sys.path.insert(0, os.path.join(os.path.dirname(HERE), "agents"))

import unitutil  # noqa: E402
import prepcommit as pc  # noqa: E402
from units import brief as brief_mod  # noqa: E402
from units import claims  # noqa: E402
from units import handoff as handoff_mod  # noqa: E402
from units import recompile as rc  # noqa: E402
# `stylelint` owns the rule-2 machinery (the symbols.txt + splits.txt `Ownership` index and the
# `header_declarations` scanner the band rule uses); the new range-boundary check reuses it rather than
# growing a second copy of the map parser and a second declaration scanner.
from units import stylelint as sl  # noqa: E402
# `subproc` owns the one rule for reading a subprocess' text (pin UTF-8, never inherit the locale codec).
# `land.run` was the site that broke it - see the F34 fixture in the selftest and `subproc.py`'s docstring.
from units import subproc as sp  # noqa: E402
# unionguard decides whether a conflicted registration is the safe append class; unionresolve is the
# resolver + invariant assertions moved in from the untracked `.pi/bin/union.py`. `land` calls both
# directly, so the land path no longer depends on a script a fresh clone cannot see.
from units import unionguard as ug  # noqa: E402
from units import unionresolve as ur  # noqa: E402
# `verifyunit` is the independent half of the gate: registration completeness, a per-symbol re-measure
# that re-runs `report generate` instead of reading the report it audits, and split-target-object
# drift. `land` calls it directly so the gate performs the checks the SKILL documents rather than the
# landing flow trusting `build/RMHE08/report.json` (which the batch was measured against).
from units import verifyunit as vu  # noqa: E402
# `vtableaudit` owns the rule-10 check: a table of code pointers inside a unit's own ranges must be
# **compiler output** (a `virtual` class), and a hand assignment to it is the violation. Rule 10 used to
# be a "landing-review rule" - enforced by a habit nobody performed - and `Network/fn_803D3CE8.cpp`'s two
# `self->vtable = &NetworkSessionManagerVTable;` writes survived a landing through it (2026-09-27). The
# gate row below is that habit, as a check.
from units import vtableaudit as vta  # noqa: E402
# `undefrefs` owns the relocation-name row: a `bl`/pointer under a *different relocation name* scores the
# same, so a 100 % row can call a symbol no link input defines and no score-reading gate can see it
# (`quest/arenatask`'s wrong struct tag, `hud/cockpit_quest`'s C-linkage spelling). The row is the
# actionable half - does our object relocate a name nothing can provide - not a relocation diff.
from units import undefrefs as uref  # noqa: E402

ALLOWED_PREFIXES = ("src/", "include/", "docs/", "tools/", ".claude/", ".github.example/")
# Root documents and repo-config files a docs/tooling batch legitimately edits (commit categories
# `repo/readme`, `repo/license`, `repo/gitignore`, `repo/ci`): README.md, LICENSE, `.gitattributes` (line-ending
# policy) and `.flake8` (the tools' lint config). Deliberately absent: `.gitmodules` and `Add-Exclusion.ps1`.
ALLOWED_FILES = ("configure.py", "CLAUDE.md", ".gitignore", "README.md", "LICENSE", ".gitattributes", ".flake8",
                 "config/RMHE08/splits.txt", "config/RMHE08/symbols.txt")
BASE_FILE = os.path.join(".pi", "land-base.json")

# Tool scratch a batch never owns, and the only thing outside `ALLOWED_*` this gate tolerates. An
# `objdiff-cli diff` run from the repo root - typically a caller that names its dump after the symbol it is
# looking at (`d910.json`, the `diff` of fn_8009A910, plus a byte-identical `t910.json`) - leaves these in the
# repo root. `65492794` put them in `.gitignore`, but a **staged** file bypasses `.gitignore`, and a stage
# happened anyway: the landing flow's own `git add -A` (`.pi/bin/applybranch.sh`) swept `d910.json` into the
# index, land.py then refused the batch over it ("paths outside the batch appeared during the build"), and the
# batch could not be committed at all. A path the gate refuses must never have reached the index, so this
# gate now de-indexes what it tolerates instead of refusing it - and names it, every time.
SCRATCH_JSON = re.compile(r"^[dt][0-9]+\.json$")

# Every check is classified by KIND, because the two kinds need opposite responses. A GATE check refuses
# because the *batch* is bad: the style lint, the regression scan, the build/DOL hash, a path outside the
# batch, a genuine claim conflict. A BOOKKEEPING check refuses because the *landing's own state* is stale
# while the batch itself is fine: a base that was never recorded (or has moved), a branch a `--force`
# release parked at a rescue ref, an outbox the worker has to rewrite, a claim whose teardown did not
# finish. The old refusal said "the gate failed - nothing staged or committed" for both, and on 2026-09-26 a
# bookkeeping refusal (a released branch) read as a substantive gate failure - the reader reached for the
# manual-landing fallback and nearly overrode a real gate failure on another batch. The KIND and the remedy
# are now part of every refusal, and a bookkeeping-only refusal never says "the gate failed".
KIND_GATE = "gate"
KIND_BOOKKEEPING = "bookkeeping"
KIND_TAG = {KIND_GATE: "GATE", KIND_BOOKKEEPING: "BOOKKEEPING"}
KIND_REMEDY = {
    KIND_GATE: "the batch itself is bad - fix the batch; do not override the gate",
    KIND_BOOKKEEPING: ("the batch itself is fine - repair the landing's own state (the remedy above), "
                       "then re-run"),
}


def check_kind(row: tuple) -> str:
    """The KIND of a check row. A 4-tuple (an older caller, a test fixture) reads as GATE - a refusal whose
    kind is unknown must never be soft-pedalled as mere bookkeeping."""
    return row[4] if len(row) > 4 and row[4] in KIND_TAG else KIND_GATE


def check_remedy(row: tuple) -> str:
    """The remedy a row carries, falling back to the generic remedy of its kind."""
    return (row[5] if len(row) > 5 and row[5] else "") or KIND_REMEDY[check_kind(row)]


def failed_kinds(checks: list) -> set[str]:
    """The set of KINDs among the failed rows of `checks`."""
    return {check_kind(row) for row in checks if not row[1]}


def kinds_from_failures(failed: list[str]) -> set[str]:
    """The kinds named by `failing_checks` output tags (`[GATE]` / `[BOOKKEEPING]`).

    `land` reads `verify`'s out-parameter as formatted strings, so the tag is the only kind signal that
    crosses that boundary; parsing it keeps `verify`'s signature unchanged.
    """
    return {kind for kind, tag in KIND_TAG.items() if any("[%s]" % tag in f for f in failed)}


def failure_summary(checks: list, prefix: str = "REFUSING to build or stage anything") -> str:
    """`"<prefix> (<KIND>: what it means): <check> [KIND]: ... (remedy: ...)"` for a failed check list.

    The aggregate names the KIND before the per-check lines, so the first line already tells the reader
    whether the *batch* is bad or the *landing's own state* is stale. A bookkeeping-only failure says so and
    never "the gate failed".
    """
    kinds = failed_kinds(checks)
    if kinds == {KIND_BOOKKEEPING}:
        head = ("%s (BOOKKEEPING: the batch itself passed; the landing's own state is stale - this is NOT a "
                "gate failure)" % prefix)
    elif kinds == {KIND_GATE} or not kinds:
        head = "%s (GATE: the batch itself is bad, so the landing must stop)" % prefix
    else:
        head = ("%s (GATE and BOOKKEEPING both failed - GATE: the batch itself; BOOKKEEPING: the landing's "
                "own state)" % prefix)
    return head + ": " + "; ".join(failing_checks(checks))


def run(args: list[str], cwd: str) -> subprocess.CompletedProcess:
    return subprocess.run(args, cwd=cwd, capture_output=True, text=True, encoding="utf-8", errors="replace")


def git(args: list[str], cwd: str, check: bool = True) -> str:
    p = run(["git", *args], cwd)
    if check and p.returncode != 0:
        raise SystemExit("git %s failed in %s: %s" % (" ".join(args), cwd, p.stderr.strip()))
    return p.stdout


def record_base(main: str, units: list[str] | None = None) -> dict:
    head = git(["rev-parse", "HEAD"], main).strip()
    # HEAD == base here, so `changed_paths` is exactly "what was already dirty when the batch opened":
    # tracked edits and untracked files alike. `land_stageable` reads it back as the foreign-path guard.
    dirty = changed_paths(main)
    data = {"base": head, "recorded_at": time.strftime("%Y-%m-%dT%H:%M:%S"),
            "subject": git(["log", "-1", "--format=%s"], main).strip(),
            "ledger": ledger_numbers(main), "report": report_snapshot(main),
            "dirty_at_base": dirty}
    # The add-only row's set: the base tree's own unresolved references, so the gate refuses only the names
    # a batch ADDS and reports the rest as debt. The batch's units are compiled first (a handful, seconds)
    # so the snapshot is the base's own objects, not a stale build; `units is None` snapshots every object
    # already present (the manual `record-base` flow, which does not name its units).
    norm = [claims.norm_unit(u.strip("/")) for u in (units or []) if u.strip()]
    if norm and os.path.exists(os.path.join(main, "build.ninja")):
        targets = compile_targets(norm)
        if targets:
            run(["ninja"] + targets, main)       # best effort: a failed compile leaves the object missing
    data["undefrefs"] = uref.snapshot_base(main, norm or None)
    os.makedirs(os.path.join(main, ".pi"), exist_ok=True)
    with open(os.path.join(main, BASE_FILE), "w", encoding="utf-8") as fh:
        json.dump(data, fh, indent=1)
    return data


def read_base(main: str) -> dict:
    path = os.path.join(main, BASE_FILE)
    if not os.path.exists(path):
        return {}
    try:
        return json.loads(open(path, encoding="utf-8").read())
    except json.JSONDecodeError:
        return {}


def land_subject(units):
    # The gate s subject follows CLAUDE.md s commit convention: a category that mirrors the tree, then an
    # imperative message of at most 120 characters. Derived from the batch, so the two cannot drift.
    entries = [u.strip() for u in (units or []) if u.strip()]
    if not entries:
        return "repo/batch: land a batch"
    first = entries[0].replace("\\", "/")
    for pre in ("src/", "include/"):
        if first.startswith(pre):
            first = first[len(pre):]
            break
    parts = first.split("/")
    root = parts[0]
    stem = root[:-4] if root.endswith(".cpp") else (root[:-2] if root.endswith(".c") else root)
    lo = stem.lower()
    if lo == "tools":
        cat = "tools/" + (parts[1] if len(parts) > 1 else "tools")
    elif lo == "docs":
        cat = "docs/" + (parts[1][:-3] if len(parts) > 1 and parts[1].endswith(".md") else (parts[1] if len(parts) > 1 else "docs"))
    elif lo == "config":
        base = parts[-1]
        cat = "config/" + {"symbols.txt": "symbols", "splits.txt": "splits", "configure.py": "flags"}.get(base, "config")
    elif lo == ".claude":
        cat = "agents/" + (parts[2][:-3] if len(parts) > 2 and parts[2].endswith(".md") else "policy")
    elif lo in (".github", ".git"):
        cat = "repo/ci"
    elif lo == "claude.md":
        cat = "agents/policy"
    elif lo.startswith("readme") or lo.startswith("license") or lo in (".gitignore", ".gitattributes", "gitignore", "gitattributes"):
        cat = "repo/" + ("readme" if lo.startswith("readme") else ("license" if lo.startswith("license") else lo))
    else:
        cat = "game/" + lo
    tail = "" if len(entries) == 1 else (" and %d more" % (len(entries) - 1))
    msg = "land " + first + tail
    if len(msg) > 120:
        msg = msg[:117] + "..."
    return cat + ": " + msg


def land_message_path(main: str) -> str:
    """The gate's commit message. Only a green gate writes it (`write_land_message`); a red one clears it."""
    return os.path.join(main, ".git", "land_msg.txt")


def subject_lint(main: str, subject: str, runner=None, tool: str | None = None) -> tuple:
    """Lint the gate's own composed subject with `tools/git/commitlint.py` - the convention's own checker.

    Every landing is written under a subject `land_subject` composes, so a regression there (an invented
    category, a message past the limit) would land a message CLAUDE.md's convention forbids and nothing would
    notice. The row calls the tool rather than re-deriving the rules: CLAUDE.md is the convention,
    commitlint.py is its mechanical checker, and a second copy here would drift from both.

    The tool's exit codes are the contract (`--diff`'s 0/1/2): **0** clean, **1** a violation, **2** nothing
    was checked - and 2 is a *failure* here, because a lint that did not run must never read as approval.

    `runner` and `tool` are the seams the selftest uses (the real tool, pinned to a fixture `--root`); the
    gate passes neither.
    """
    tool = os.path.abspath(tool or os.path.join(main, "tools", "git", "commitlint.py"))
    if not os.path.exists(tool):
        return True, "commitlint.py not built yet - the row is skipped"
    runner = runner or (lambda args: run(args, main))
    p = runner([sys.executable, tool, "--message", subject, "--root", main])
    if p.returncode == 0:
        return True, ""
    detail = command_detail(p)
    if p.returncode == 2:
        return False, "commitlint checked nothing (exit 2), which is not a pass: %s" % detail
    return False, detail


def write_land_message(main: str, body: str) -> str:
    path = land_message_path(main)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8") as fh:
        fh.write(body)
    return path


def message_body_with_subject(body: str, subject: str) -> str:
    """Replace the gate message's subject line with a deliberate `--message`, keeping the gate's body.

    `verify` writes `land: <units>` followed by the ledger and gate summary. A caller's `--message` replaces
    only that first line, so the landed commit still records the ledger delta it was gated against.
    """
    _first, sep, rest = body.partition("\n")
    return subject + sep + rest


def clear_land_message(main: str) -> str | None:
    """Remove a stale message so a failed gate cannot be committed through `git commit -F .git/land_msg.txt`."""
    path = land_message_path(main)
    if os.path.exists(path):
        os.remove(path)
        return path
    return None


def failing_checks(checks: list) -> list[str]:
    """`"<check name> [<KIND>]: <what it printed> (remedy: ...)"` for every failed check.

    The check *names* are the assertion they make ("style lint (§6.5) adds no violation"), so a FAIL row's
    name alone can read as a pass; on 2026-09-25 the gate refused and printed only "the gate failed - nothing
    staged or committed" while the lint row showed stylelint's trailing legend (`.pi/land.log`), so the reader
    went hunting for a defect that was not there and the batch was committed by hand. Every refusal now
    carries the failing check's name, the output it printed, its KIND and its remedy.

    The KIND separates "the batch is bad" (GATE) from "the landing's own state is stale" (BOOKKEEPING) - the
    distinction the 2026-09-26 released-branch refusal lost. A 4-tuple row (an older caller, a fixture) reads
    as GATE through `check_kind`, the conservative default.
    """
    out = []
    for row in checks:
        name, good, detail, info = row[:4]
        if good:
            continue
        kind = check_kind(row)
        note = " ".join((detail or info or "no detail").split())
        out.append("%s [%s]: %s (remedy: %s)"
                   % (name, KIND_TAG[kind], note[:240] or "no detail", check_remedy(row)))
    return out


def command_detail(p: subprocess.CompletedProcess, limit: int = 300) -> str:
    """The *head* of a command's output plus its exit code - where a linter puts the reason.

    A tail is the wrong end for `stylelint.py`: it prints its finding header and its findings first and its
    "not enforced: ..." legend last, so the old `output[-300:]` showed the legend and hid the violation. The
    2026-09-25 row `FAIL style lint (§6.5) adds no violation - (temporary grandfather: legacy scaffolding with
    bodies ...)`, read on its own, looked like a pass - and the batch was then committed by hand while the
    gates looked green (`.pi/land.log`). Two lines (the header and the first finding) are the reason; the
    legend adds nothing and hides it. A linter's reason is its *first* line.

    Neither end is right for `ninja`, which is why a failing `ninja` reported only `ninja: build stopped:
    subcommand failed.` for a whole session of gate runs. Its head is progress noise (`[N/M] ...`), its tail
    is that summary line, and the reason - `FAILED: <target>` plus the compiler's own error - sits in the
    middle. So when the output names failed outputs, those ARE the reason: report them and stop, rather than
    showing either a progress line or a summary. That is the difference between "the batch is bad" being
    actionable and being a hunt (2026-09-26: `ef/fn_8030681C` refused 4 times with the reason invisible).
    """
    text = (p.stdout or "") + (p.stderr or "")
    failed = failed_compile_outputs(text)
    if failed:
        extra = " (+%d more)" % (len(failed) - 3) if len(failed) > 3 else ""
        return "exit %d: FAILED: %s%s" % (p.returncode, ", ".join(failed[:3]), extra)
    lines = [l.strip() for l in text.splitlines() if l.strip()]
    if not lines:
        return "no output (exit %d)" % p.returncode
    return "exit %d: %s" % (p.returncode, "; ".join(lines[:2])[:limit])


def selftest_detail(p: subprocess.CompletedProcess) -> str:
    """A one-line reason from `tools/selftest.py --json`, so the gate NAMES the failing tool.

    `command_detail` reports the *head* of a command's output, which for the selftest table is its header and
    first data row - the failures are printed below the table. Parse the runner's own JSON instead and name
    them, so the gate's refusal says which tool's selftest failed rather than showing a table header.
    """
    if p.returncode == 0:
        return ""
    try:
        data = json.loads(p.stdout or "")
    except ValueError:
        return command_detail(p)
    parts = []
    bad = [f["name"] for f in (data.get("failures") or [])]
    if bad:
        more = " (+%d more)" % (len(bad) - 6) if len(bad) > 6 else ""
        parts.append("failed: " + ", ".join(bad[:6]) + more)
    stale = data.get("stale_parks") or []
    if stale:
        parts.append("stale park: " + ", ".join(stale[:3]))
    if data.get("tree_clean") is False:
        parts.append("a selftest changed the tree: " + ", ".join((data.get("tree_offenders") or [])[:3]))
    return "exit %d: %s" % (p.returncode, "; ".join(parts) or command_detail(p))


def land_decision(gate_ok: bool, stageable: list[str],
                  failed: list[str] | None = None,
                  kinds: set[str] | None = None) -> tuple[str, str]:
    """What `land` does after the gate: -> (`"commit"` | `"refuse"`, reason).

    The one command has to be safe when its output is piped (the exit status is then lost): the gate's verdict
    *is* the decision, and a red gate can never reach `git commit`. A green gate with nothing to stage is also
    a refusal - there is no batch to land. A refusal on a failed gate NAMES the failing check(s) and what each
    printed (`failing_checks`): a bare "the gate failed" is not actionable, and a reader who cannot see which
    gate failed cannot tell a real defect from a passing check.

    `kinds` is the set of check KINDS that failed. A BOOKKEEPING-only refusal says so and never "the gate
    failed": the batch is fine and the fix is to the landing's own state, so a reader must not reach for the
    manual-landing fallback that a real gate failure would (and must) stop. With no `kinds` the conservative
    GATE wording is kept, so an unfurnished caller is never told a red batch is merely bookkeeping.
    """
    if not gate_ok:
        kinds = set(kinds or [KIND_GATE])
        if kinds == {KIND_BOOKKEEPING}:
            why = ("BOOKKEEPING refusal - the batch itself passed, the landing's own state is stale (this is "
                   "NOT a gate failure) - nothing staged or committed")
        elif kinds == {KIND_GATE}:
            why = "the gate failed - nothing staged or committed"
        else:
            why = ("BOTH kinds failed - GATE (the batch itself) and BOOKKEEPING (the landing's own state) - "
                   "nothing staged or committed")
        return "refuse", why + (": %s" % "; ".join(failed) if failed else "")
    if not stageable:
        return "refuse", "the gate passed but no batch path is stageable - nothing to commit"
    return "commit", ""


def message_error(subject: str | None) -> str | None:
    """Refuse an empty or whitespace-only `--message` before the gate does any work.

    The incident (2026-09-24, `71c244f9`): `--message "$(cat /tmp/msg1.txt)"`, where the shell's `/tmp` is not
    the one the file was written to, expands to the empty string. An empty override used to be dropped by
    `if subject:`, so the batch landed under the gate's fallback subject `land: <units>` instead of the message
    the worker wrote. Omitting `--message` is how a caller deliberately asks for that fallback; an empty or
    blank argument is the caller's bug, refused here - loudly, naming the argument - at the same cost as a lint
    refusal, and before `verify` runs.
    """
    if subject is None:
        return None
    if not subject.strip():
        kind = "empty" if subject == "" else "whitespace-only"
        return ("--message is %s (%r): pass the subject you meant, or omit --message to use the gate's "
                "default subject" % (kind, subject))
    return None


def branch_error(main: str) -> str | None:
    """Refuse to run the gate anywhere but `main`.

    The incident (2026-09-24): a worker told to "branch and commit there" ran
    `git checkout -b tools/stylelint-rule2-unsplit` in MAIN's checkout, so MAIN's HEAD left `main` and the
    next **14 landings** went onto that branch while the `main` ref sat at `e3ade082`. Nothing failed -
    `.pi/bin/applybranch.sh` and `land.py` both key off `main` - but a stale `main` silently changes what
    they *mean*: the merge-base slides backwards and the branch's diff starts describing already-landed
    units, re-applying them or listing them as deletions (it nearly deleted landed units the same day). The
    gate names the branch it found and refuses before any check runs.
    """
    branch = git(["rev-parse", "--abbrev-ref", "HEAD"], main).strip()
    if branch != "main":
        return ("HEAD is on %r, not main: `git checkout main` first - a batch landed off main puts its "
                "commits on the wrong ref and slides the merge-base" % branch)
    return None


def caller_branch_error(start: str | None = None) -> str | None:
    """`branch_error` asked about the tree the *caller* is in - the guard `record-base`/`verify` run under.

    Both are manual entry points on the landing path, and both read MAIN's tree. But `main` is resolved by
    `rc.main_root`, which walks the worktree list from wherever the caller stands and always answers with the
    first worktree git lists - MAIN. So a `record-base` run from inside a worker's worktree would quietly
    record MAIN's HEAD (or, if MAIN's HEAD had left `main`, a stale one) with nothing saying the wrong tree
    was asked. `land` refuses a HEAD that is not `main` through `branch_error(main)`; these two ask the same
    helper about the caller's own tree (`rc.worktree_root`), so a call from any worktree but MAIN is refused
    and names the branch it found.
    """
    return branch_error(rc.worktree_root(start))


def changed_status(main: str) -> list[tuple[str, str]]:
    """[(status, path)] for every change git reports, untracked files listed individually (`-uall`)."""
    out = git(["status", "--porcelain", "-uall"], main)
    rows = []
    for line in out.splitlines():
        if len(line) < 4:
            continue
        code, path = line[:2].strip(), line[3:].strip()
        if " -> " in path:
            # a rename is two paths, and both belong to the pathspec: passing only the new one leaves the
            # deletion staged, so the commit records an add where the batch meant a move
            old, path = path.split(" -> ")
            rows.append((code, old.strip('"')))
        rows.append((code, path.strip('"')))
    return rows


def changed_paths(main: str) -> list[str]:
    return [path for _code, path in changed_status(main)]


def conflict_marker_files(main: str, paths: list[str]) -> list[tuple[str, int, str]]:
    """The batch's own files that carry a git conflict marker, as `(path, line, marker)`.

    A committed conflict marker is the cheapest defect to catch and one of the more expensive ones to find
    late: the build reports it as a syntax error in whichever file carries it, so a full compile buys one
    line's worth of news, and the marker survives review because it looks like ordinary text.  Only the two
    markers a conflict writes are looked for - `=======` on its own is a legal banner comment, so a file
    full of those is not evidence of anything.
    """
    found: list[tuple[str, int, str]] = []
    for rel in paths:
        path = os.path.join(main, rel)
        if not os.path.isfile(path):
            continue
        try:
            with open(path, "r", encoding="utf-8", errors="replace") as fh:
                for number, line in enumerate(fh, 1):
                    head = line.lstrip()
                    for marker in ("<<<<<<<", ">>>>>>>"):
                        if head.startswith(marker + " ") or head.rstrip() == marker:
                            found.append((rel, number, marker))
        except OSError:
            continue
    return found


def _exists_at(main: str, base: str | None, entry: str) -> bool:
    """True when `entry` is a file or directory in `base`'s tree (`git cat-file -e <base>:<entry>`)."""
    if not base:
        return False
    try:
        return subprocess.run(["git", "cat-file", "-e", "%s:%s" % (base, entry)], cwd=main,
                              stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode == 0
    except OSError:
        return False


def is_batch_path(main: str, entry: str, base: str | None = None) -> bool:
    """True when a `--units` entry names a repo PATH the batch stages, not a translation unit.

    A unit is named at an extensionless path (`<module>/<name>`, or `src/<module>/<name>` - `norm_unit`
    strips the source extension) whose source file is `<name>.c`/`.cpp`, so the bare name is **not** a file
    in the tree.  Three signals therefore say "path": the entry still carries a file extension, the tree
    has something at that exact path - which `.gitignore`, `LICENSE`, `Makefile` and a directory like `docs`
    do, and a unit never does - or the batch BASE's tree has it.  The last is for a batch that DELETES (or
    renames away) an extension-less file such as `tools/git/hooks/post-commit`: the gate runs after the
    apply, where the file no longer exists, so the tree test alone read it as a unit and demanded an
    `Object(...)` line and an outbox for it (2026-09-28, the hook removal).
    """
    if os.path.splitext(entry)[1]:
        return True
    if os.path.exists(os.path.join(main, entry)):
        return True
    # A source-looking file OUTSIDE `src/` (a playbook demo `docs/matching/043-x.cpp`, a fixture) is a path too:
    # `norm_unit` strips its `.cpp` before it reaches here, but a unit's source always lives under `src/`.
    if not entry.startswith("src/"):
        for ext in (".cpp", ".cp", ".c"):
            if os.path.isfile(os.path.join(main, entry + ext)) or _exists_at(main, base, entry + ext):
                return True
    return _exists_at(main, base, entry)


def unit_rows(main: str, units: list[str], base: str | None = None) -> list[str]:
    """The unit-shaped subset of a `--units` list: translation units, not paths the batch stages.

    `is_batch_path` draws the line for every unit-shaped gate row (outbox, branch, registration, compile,
    target drift), so this is the one place that decision is made.  The outbox row used to run over the raw
    `--units` list instead, so a batch whose `--units` named a HEADER
    (`include/Network/network_state.h`) had `outbox_units` look for that path as if it were a unit and demand
    `residual`/`flags_probed` from a non-unit record - a BOOKKEEPING refusal while every real gate row passed
    (2026-09-28, cost one round-trip and was landed with `--no-outbox`).
    """
    return [u for u in units if not is_batch_path(main, u, base)]


def unit_owned_paths(units: list[str]) -> set[str]:
    """The source paths a batch's units own: `src/<unit>.<ext>` and any path the unit names directly."""
    owned: set[str] = set()
    for unit in units:
        unit = unit.strip("/")
        if not unit:
            continue
        owned.add(unit)
        normalized = claims.norm_unit(unit)
        owned.add("src/" + normalized)
        for ext in (".c", ".cpp", ".cp"):
            owned.add(unit + ext)
            owned.add("src/" + normalized + ext)
    return owned


def base_dirty_paths(main: str) -> set[str]:
    """The paths already dirty when the batch base was recorded - another stream's in-flight work.

    `record_base` snapshots them while HEAD == base, so a path in this set was dirty *before* the batch
    touched anything. A base recorded before this snapshot existed has none, and the guard is off for that
    batch (re-record the base to arm it). `--base` only asserts HEAD; the recorded snapshot still names the
    batch's foreign work.
    """
    return set(read_base(main).get("dirty_at_base") or [])


def land_stageable(units: list[str], rows: list[tuple[str, str]],
                   base_dirty: set[str] | None = None) -> list[str]:
    """The paths `land` stages: the batch's own files, never another stream's in-flight work.

    A *tracked* change inside the allowed set is part of the batch (the cherry-pick, the shared-file edits) -
    except under `tools/`: that tree is where every worker keeps its own in-flight tools, so a tracked
    `tools/` change belongs to the batch only when the batch *names* that path as one of its units. Without
    this, `tools/units/langcheck.py` was a tracked change inside the allowed set, `land` staged it, and the
    batch's commit carried another stream's work (`85f3d4b5`, `d50fdd32`). An **untracked** file is staged
    when it is a named unit's own path or lives under `src/`/`include/` (source is the batch's), but not
    otherwise.

    `base_dirty` is the set `record_base` snapshotted when the batch opened: a path that was already dirty
    then is foreign, not batch material, even inside the allowed set (`docs/plan.md` under `85ddd7b6`,
    `src/RSO/runtime.c` under `890631e8`). The batch still stages a path it *names* as one of its units, so a
    unit the batch is genuinely working on keeps its existing behaviour.

    Tool scratch (`is_scratch`) is never staged - the batch did not receive it (the `d910.json` refusal).
    """
    owned = unit_owned_paths(units)
    foreign = base_dirty or set()
    stageable = []
    for code, path in rows:
        if outside_batch([path]):
            continue
        if is_scratch(path):
            continue          # an objdiff dump the batch never received, staged or not (the crossing point)
        if path in foreign and path not in owned:
            continue          # already dirty at the batch base: another stream's work, leave it alone
        if code.startswith("??") and path not in owned and not path.startswith(("src/", "include/")):
            continue
        if path.startswith("tools/") and path not in owned:
            continue
        stageable.append(path)
    return stageable


def looks_already_applied(rows: list[tuple[str, str]], base_dirty: set[str] | None) -> bool:
    """True when the batch was applied to the tree *before* `record-base` ran.

    The signature: every changed path the batch guard allows is in the base's `dirty_at_base` snapshot, so
    `land_stageable` excludes them all as another stream's work and there is nothing left to stage. That is
    exactly what `record-base` running *after* the apply looks like - the snapshot it took recorded the
    batch's own edits as pre-existing. The batch is fine; the *ordering* was wrong (the 2026-09-26 case (b),
    where `land` refused with "the gate passed but no batch path is stageable" straight after "READY: every
    check passed", and the round had to commit by hand).

    A batch with no changed path at all is not this: there is genuinely nothing to commit, and the plain
    "nothing to commit" refusal is the right one. Neither is a batch that adds a path the base did not
    already hold - only a *wholly* pre-existing dirty set has the signature.
    """
    allowed = [p for _code, p in rows if not outside_batch([p]) and not is_scratch(p)]
    if not allowed:
        return False
    return set(allowed) <= (base_dirty or set())


def staged_elsewhere(main: str, stageable: list[str]) -> list[str]:
    """Paths already in the index that are not part of this batch - another stream's in-flight work.

    `land` commits with a pathspec, so these are never swept in; naming them is the warning that keeps the
    accident visible. The 2026-09-23 collision (a staged `tools/units/langcheck.py` landed under two unrelated
    unit commits, `85f3d4b5` and `d50fdd32`) happened because the commit had no pathspec and took the whole
    index.
    """
    staged = git(["diff", "--cached", "--name-only"], main).splitlines()
    return [p for p in staged if p and p not in stageable]


def foreign_warning(foreign: list[str]) -> str:
    """The warning `land` prints when the index holds paths outside the batch (a note, never a refusal)."""
    noun = "path" if len(foreign) == 1 else "paths"
    return ("WARNING: the index holds %d %s outside this batch - left staged, not committed: %s"
            % (len(foreign), noun, ", ".join(foreign)))


def commit_pathspec(main: str, msg_file: str, stageable: list[str]) -> subprocess.CompletedProcess:
    """Commit exactly the batch's paths. `git commit` with no pathspec commits the whole index; with one it
    commits the named paths (read from the working tree) and leaves every other staged path staged."""
    return run(["git", "commit", "-F", msg_file, "--", *stageable], main)


def stage_batch(main: str, stageable: list[str]) -> None:
    """`git add` the batch's paths. Deleted paths are left to the commit's pathspec: `git add` refuses a
    pathspec that matches no working-tree file, while `git commit -- <path>` records the deletion (staged or
    not) on its own. So a rename's source path can stay in the pathspec without breaking the staging step."""
    existing = [p for p in stageable if os.path.exists(os.path.join(main, p))]
    if existing:
        git(["add", "--", *existing], main)


# --------------------------------------------------------------------------------------------------
# The registration append-conflict resolver (docs/plan.md 7.5; the 2026-09-26 merger class).
#
# Eight of thirteen live branches conflict with `main` on exactly one class: sibling bands claim adjacent
# address ranges, so both sides append a block at the same anchor in `config/RMHE08/splits.txt` and a line
# in `configure.py`'s `Object(...)` list.  The resolution is the pure append-union - and it is the one
# union that is safe: `unionguard` proves both sides only *inserted*, and `unionresolve` asserts the
# invariants a wrong union breaks silently.
#
# `resolve` runs in the batch's worktree or a **scratch tree**, never in MAIN.  `land --branch` applies the
# branch's own delta inside MAIN (that is the landing) but uses the same union, which is why the union
# lives in `_union_conflicts` with the MAIN guard in `resolve_conflicts` around it.
#
# Staging is explicit (`git add -- <the two scoped paths>`), never `git add -A`: that sweep - the tail of
# the untracked `.pi/bin/applybranch.sh` this replaces - staged `d910.json` and has bitten the campaign
# twice.
# --------------------------------------------------------------------------------------------------

UNION_SCOPE = ur.UNION_SCOPE


def _resolve_result(ok: bool, reason: str, **extra) -> dict:
    out = {"ok": ok, "reason": reason}
    out.update(extra)
    return out


def _tree_text(tree: str, ref: str, rel: str) -> str:
    """`git show <ref>:<rel>` read as text, or "" when the path is absent at that ref."""
    p = run(["git", "show", "%s:%s" % (ref, rel)], tree)
    return p.stdout if p.returncode == 0 else ""


def _union_conflicts(tree: str, branch: str, base: str | None = None, paths: list[str] | None = None,
                     commit: bool = False, runner=subprocess.run) -> dict:
    """Union the in-scope registration conflict in `tree` (the MAIN guard is in `resolve_conflicts`).

    Three ordered gates, any of which refuses: **scope** (every conflicted path must be `configure.py` or
    `config/RMHE08/splits.txt`), **unionguard** (a disjoint addition, not a delete/rename/overlap), and
    the **invariants** (`ur.check_union`).  The union is computed in memory and the invariants asserted
    *before* anything is written, so a union that duplicates a key, overlaps a range or drops a
    registration never reaches the tree.  `ur.union_text_full` also reports a **prose** hunk whose two
    sides neither carried the other (a comment paragraph both sides rewrote, with no superset): unioning
    it would duplicate the prose and reintroduce generated names, so it is refused here rather than
    written - the scope gate already keeps the registration class (`configure.py`/`splits.txt`) a pure
    declaration union.
    """
    try:
        stages = ug.unmerged(tree)
    except RuntimeError as exc:
        return _resolve_result(False, "%s is not a git tree: %s" % (tree, exc))
    conflicted = sorted(paths if paths is not None else stages)
    if not conflicted:
        return _resolve_result(False, "no unmerged path in %s - nothing to resolve" % tree)
    foreign = [p for p in conflicted if p not in UNION_SCOPE]
    if foreign:
        return _resolve_result(
            False,
            "conflict outside the registration scope: %s - a header or `src/**` conflict is a real "
            "content conflict; unioning it stacks `#ifdef`/`#endif` and shifts struct offsets. Resolve "
            "it by hand." % ", ".join(foreign), scope=list(UNION_SCOPE))
    if base:
        ours_renames = ug.rename_sets(tree, base, branch)
        theirs_renames = ug.rename_sets(tree, base, "main")
    else:
        ours_renames = theirs_renames = (set(), set())
    unsafe = []
    for path in conflicted:
        finding = ug.classify(tree, path, stages.get(path, {}), ours_renames, theirs_renames)
        if finding["unsafe"]:
            unsafe.append("%s (%s)" % (path, ", ".join(finding["reasons"])))
    if unsafe:
        return _resolve_result(
            False,
            "unionguard refused, the conflict is not a disjoint addition: %s - resolve by hand, do not "
            "union them" % "; ".join(unsafe))

    # Compute every union in memory first; a violation must not touch the tree.
    merged_texts: dict[str, str] = {}
    for path in conflicted:
        work = os.path.join(tree, *path.replace("/", os.sep).split(os.sep))
        try:
            with open(work, encoding="utf-8", newline="") as fh:
                text = fh.read()
        except OSError as exc:
            return _resolve_result(False, "cannot read %s: %s" % (path, exc))
        merged, hunks, decisions = ur.union_text_full(text, path)
        if hunks == 0 or "<<<<<<<" in merged or ">>>>>>>" in merged:
            return _resolve_result(False, "%s carries no conflict block to union" % path)
        blocked = [d for d in decisions if d.get("blocked")]
        if blocked:
            return _resolve_result(
                False,
                "%s carries a prose conflict hunk with no superset (hunk %d: %s) - unioning it would "
                "duplicate the prose and reintroduce generated names; resolve it by hand"
                % (path, blocked[0]["hunk"], blocked[0]["why"]))
        merged_texts[path] = merged

    main_splits = _tree_text(tree, "main", "config/RMHE08/splits.txt")
    main_configure = _tree_text(tree, "main", "configure.py")
    merged_splits = merged_texts.get("config/RMHE08/splits.txt", main_splits)
    merged_configure = merged_texts.get("configure.py", main_configure)
    violations = ur.check_union(main_splits, merged_splits, main_configure, merged_configure)
    if violations:
        # The union looked plausible but broke an invariant - the case a green build cannot see. Refuse
        # and name every violation; nothing was written.
        return _resolve_result(False, "the union would break %d invariant(s): %s"
                               % (len(violations), "; ".join(violations)), violations=violations)

    for path, merged in merged_texts.items():
        work = os.path.join(tree, *path.replace("/", os.sep).split(os.sep))
        with open(work, "w", encoding="utf-8", newline="\n") as fh:
            fh.write(merged)
    # the two scoped paths, explicitly - never `git add -A`, the untracked applybranch.sh's sweep.
    git(["add", "--", *conflicted], tree)
    committed = None
    if commit:
        p = runner(["git", "commit", "-q", "-m",
                    "resolve: union the registration append-conflict on %s" % branch], cwd=tree,
                   capture_output=True, text=True, encoding="utf-8", errors="replace")
        if p.returncode != 0:
            tail = ((p.stdout or "") + (p.stderr or "")).strip().splitlines()
            return _resolve_result(False, "the union is sound but the commit failed: %s"
                                   % (tail[-1] if tail else "unknown error"), paths=conflicted)
        committed = git(["rev-parse", "--short", "HEAD"], tree).strip()
    return _resolve_result(True, "union-resolved %d path(s)" % len(conflicted), paths=conflicted,
                           commit=committed)


def resolve_conflicts(tree: str, main_tree: str, branch: str, base: str | None = None,
                      paths: list[str] | None = None, commit: bool = False,
                      runner=subprocess.run) -> dict:
    """`_union_conflicts`, but it refuses to run inside MAIN - the batch's worktree or a scratch tree only.

    MAIN must stay clean while a branch is resolved: the merger lane's whole advantage is that resolving
    two registrations never touches the gate's own tree, and a gate that applied a diff inside MAIN is what
    left MAIN mid-conflict on 2026-09-26.
    """
    if os.path.abspath(tree) == os.path.abspath(main_tree):
        return _resolve_result(False, "refusing to resolve inside MAIN - use the batch's worktree or a "
                                     "scratch tree (`git worktree add`), never the gate's own tree")
    return _union_conflicts(tree, branch, base, paths=paths, commit=commit, runner=runner)


def scratch_resolve(main_tree: str, branch: str, base: str | None = None, commit: bool = True,
                    runner=subprocess.run) -> dict:
    """`resolve_conflicts` on a fresh scratch worktree: `git worktree add` + `git merge main`.

    A temporary worktree on a temporary branch at the branch's tip (so the branch itself is not disturbed
    and `main` stays clean), then `git merge --no-commit main` - exactly the merger lane's hand operation,
    with ours = the branch.  A clean merge has no conflict to resolve and is reported as such; a conflicted
    one goes through `resolve_conflicts`.  The scratch worktree is removed on refusal and kept on success,
    so the caller can fast-forward the branch (`git branch -f <branch> <scratch-branch>`).
    """
    if git(["rev-parse", "--verify", "-q", branch], main_tree, check=False).strip() == "":
        return _resolve_result(False, "no such branch: %s" % branch)
    base = base or git(["merge-base", "main", branch], main_tree).strip()
    tmp = tempfile.mkdtemp(prefix="land-resolve-")
    slug = re.sub(r"[^A-Za-z0-9._-]+", "-", branch.split("/", 1)[-1])
    scratch_branch = "land/resolve-%s-%d" % (slug, os.getpid())
    suffix = 1
    while git(["rev-parse", "--verify", "-q", scratch_branch], main_tree, check=False).strip():
        suffix += 1
        scratch_branch = "land/resolve-%s-%d-%d" % (slug, os.getpid(), suffix)
    p = runner(["git", "worktree", "add", "-b", scratch_branch, tmp, branch], cwd=main_tree,
               capture_output=True, text=True, encoding="utf-8", errors="replace")
    if p.returncode != 0:
        return _resolve_result(False, "git worktree add failed: %s"
                               % ((p.stderr or p.stdout or "").strip().splitlines() or [""])[-1])

    def discard(reason: str, **extra) -> dict:
        runner(["git", "worktree", "remove", "--force", tmp], cwd=main_tree, capture_output=True)
        runner(["git", "branch", "-D", scratch_branch], cwd=main_tree, capture_output=True)
        return _resolve_result(False, reason, **extra)

    merge = runner(["git", "merge", "--no-commit", "main"], cwd=tmp, capture_output=True, text=True, encoding="utf-8",
                   errors="replace")
    if merge.returncode == 0 and not ug.unmerged(tmp):
        return discard("the branch merges `main` cleanly - no registration conflict to resolve")
    if not ug.unmerged(tmp):
        tail = ((merge.stdout or "") + (merge.stderr or "")).strip().splitlines()
        return discard("`git merge main` failed without a conflict: %s"
                       % (tail[-1] if tail else "unknown error"))
    result = resolve_conflicts(tmp, main_tree, branch, base=base, commit=commit, runner=runner)
    if not result.get("ok"):
        return discard(result.get("reason"), **{k: v for k, v in result.items()
                                                if k not in ("ok", "reason")})
    result["worktree"] = tmp
    result["scratch_branch"] = scratch_branch
    return result


# --------------------------------------------------------------------------------------------------
# The resolve helper's teardown: `scratch_resolve` parks a registration union on a
# `land/resolve-<slug>-<pid>` helper branch (in its own scratch worktree) so the caller can fast-forward
# the worker branch onto it.  Once the branch lands, the helper is debris - and a `land/*` ref left
# behind is a false statement about the branch's state (two were left from 2026-09-26).  The landing
# deletes the helpers it can *prove* redundant - the helper's tip is already contained by the branch it
# resolved or by `main` - and refuses loudly on any other, because a hand fix made on the helper
# (2026-09-26: the `u32 mode` repair lived only on `land/resolve-8030681c-...-31048`) would be the only
# copy.  Nothing is deleted until the landing has succeeded.
# --------------------------------------------------------------------------------------------------

def resolve_helper_slug(branch: str) -> str:
    """The slug `scratch_resolve` names a branch's helper with (`worker/x` -> `x`)."""
    return re.sub(r"[^A-Za-z0-9._-]+", "-", branch.split("/", 1)[-1])


def resolve_helper_refs(main: str, branch: str) -> list[str]:
    """Every `land/resolve-<slug>-<pid>[-<n>]` helper ref that `branch` owns, matched by exact slug.

    The trailing `-<pid>[-<n>]` is what keeps `worker/foo` from claiming `worker/foo-bar`'s helper: a
    bare prefix match would sweep a sibling's resolution.
    """
    pat = re.compile(r"^refs/heads/land/resolve-%s-[0-9]+(-[0-9]+)?$" % re.escape(resolve_helper_slug(branch)))
    out = git(["for-each-ref", "--format=%(refname)", "refs/heads/land/"], main, check=False)
    return [line.strip() for line in out.splitlines() if pat.match(line.strip())]


def _is_ancestor(main: str, ancestor: str, descendant: str) -> bool:
    """True when `ancestor` is reachable from `descendant` - the one containment proof the sweep uses."""
    return run(["git", "merge-base", "--is-ancestor", ancestor, descendant], main).returncode == 0


def resolve_helper_state(main: str, branch: str) -> tuple[list[str], list[str]]:
    """Classify `branch`'s resolve helpers -> (provably redundant, refused).

    Redundant: the helper's tip is contained by `branch` or by `main`, so deleting it cannot lose the
    work.  Refused: anything else - the helper may hold a hand fix the branch never took.
    """
    redundant: list[str] = []
    refused: list[str] = []
    for ref in resolve_helper_refs(main, branch):
        tip = git(["rev-parse", ref], main, check=False).strip()
        if tip and (_is_ancestor(main, tip, branch) or _is_ancestor(main, tip, "main")):
            redundant.append(ref)
        else:
            refused.append(ref)
    return redundant, refused


def _resolve_helper_worktree(main: str, ref: str) -> str | None:
    """The worktree a helper ref is checked out in, or None."""
    out = git(["worktree", "list", "--porcelain"], main, check=False)
    wt = None
    for line in out.splitlines():
        if line.startswith("worktree "):
            wt = line[len("worktree "):].strip()
        elif line.startswith("branch ") and wt and line[len("branch "):].strip() == ref:
            return wt
    return None


def delete_resolve_helper(main: str, ref: str) -> str:
    """Remove a redundant helper's scratch worktree and delete its branch; -> the branch name deleted."""
    branch = ref[len("refs/heads/"):]
    wt = _resolve_helper_worktree(main, ref)
    if wt and os.path.isdir(wt):
        try:
            claims.safe_worktree_remove(wt, main)
        except SystemExit:
            git(["worktree", "remove", "--force", wt], main, check=False)
    git(["worktree", "prune"], main, check=False)
    git(["branch", "-D", branch], main)
    return branch


def _sweep_resolve_helpers(main: str, branch: str, redundant: list[str], refused: list[str]) -> None:
    """Delete the provably-redundant helper(s) and name every refusal, in the landing output."""
    for ref in redundant:
        try:
            delete_resolve_helper(main, ref)
            print("    resolve helper %s deleted (its resolution landed with %s)" % (ref, branch))
        except SystemExit as exc:
            print("    REFUSING to delete resolve helper %s: %s" % (ref, exc), file=sys.stderr)
    for ref in refused:
        print("    REFUSING to delete resolve helper %s: its tip is not contained by %s or main - it may "
              "hold the only copy; fast-forward %s to it and re-run" % (ref, branch, branch),
              file=sys.stderr)


# --------------------------------------------------------------------------------------------------
# The one-command landing: `land.py land --branch worker/<slug>`.
# --------------------------------------------------------------------------------------------------

def claim_unit_for_branch(main: str, branch: str) -> str | None:
    """The registry key whose claim records `branch`, or None.

    A unit renamed at registration keeps the *pre-registration* claim key while the name the gate compiles
    and the outbox lookup use differ.  The branch is the claim's identity, so the key is read from the
    branch rather than re-derived from the unit path - which is what makes the release of a renamed unit
    work.
    """
    try:
        registry = claims.load_registry(main)
    except Exception:
        return None
    for key, record in registry.items():
        if isinstance(record, dict) and record.get("branch") == branch:
            return key
    return None


def require_clean_tree(main: str) -> str | None:
    """None when MAIN's tree is clean; else the exact commands to clean it.

    One implementation and one message for every entry point, because a dirty tree blocks the gate twice
    over: `record-base` records the dirt as foreign work (so `land_stageable` excludes the batch's own
    paths), and the pick aborts on a file the dirt already touched.  Every row is dirt, CLAUDE.md included.
    """
    rows = changed_status(main)
    dirty = ["%s %s" % (code or "??", path) for code, path in rows]
    if not dirty:
        return None
    paths = " ".join(path for _code, path in rows)
    return ("main's tree is not clean: %s\n  a dirty tree is recorded as foreign work at `record-base` "
            "and aborts the pick, so clean it first, e.g.:\n"
            "    git -C %s stash push --include-untracked -- %s\n"
            "  (or `git -C %s checkout -- <path>` for a tracked edit and `git -C %s clean -fd` for "
            "untracked files), then re-run" % (", ".join(dirty), main, paths, main, main))


def units_from_branch(main: str, branch: str, base: str) -> list[str]:
    """The units a branch registers, read from the branch's own registration diff.

    `land --branch` must know which objects to compile, and a registration rename means the branch name
    cannot always be turned back into the registered unit (the outbox slug is the pre-registration name).
    The branch's diff is authoritative: its added `Object(...)` lines and `splits.txt` unit headers name the
    units it registers, in file order.  Returns normalised (extensionless) unit names, de-duplicated.
    """
    p = run(["git", "diff", base, branch, "--", "configure.py", "config/RMHE08/splits.txt"], main)
    found: list[str] = []
    for line in (p.stdout or "").splitlines():
        if not line.startswith("+") or line.startswith("+++"):
            continue
        body = line[1:]
        found.extend(claims.norm_unit(n) for n in ur.object_names(body))
        found.extend(claims.norm_unit(u) for u in ur.split_units(body))
    seen: set[str] = set()
    out: list[str] = []
    for unit in found:
        unit = unit.strip("/")
        if unit and unit not in seen:
            seen.add(unit)
            out.append(unit)
    return out


def apply_branch(main: str, branch: str, base: str | None = None
                 ) -> tuple[bool, str, str]:
    """Apply the branch's own delta to MAIN with three-way; resolve a registration conflict.

    -> (ok, reason, base).  `git diff --binary <merge-base> <branch>` is exactly the branch's own work (a
    merged-in `main` cancels out), which is why it is used instead of a cherry-pick: cherry-picking a
    branch that merged `main` silently drops the work the merge carried (the untracked
    `.pi/bin/applybranch.sh`'s own note).  A clean apply is done; a conflict is sent to the scoped union,
    and a refusal undoes the apply so the tree is exactly as it was found.
    """
    base = base or git(["merge-base", "main", branch], main).strip()
    patch = subprocess.run(["git", "diff", "--binary", base, branch], cwd=main,
                           capture_output=True).stdout
    if not patch.strip():
        return False, "the branch has no diff against %s - nothing to land" % base[:8], base
    ap = subprocess.run(["git", "apply", "-3", "-"], cwd=main, input=patch, capture_output=True)
    stages = ug.unmerged(main)
    if not stages:
        if ap.returncode == 0:
            return True, "applied cleanly", base
        tail = ap.stderr.decode("utf-8", "replace").strip().splitlines()
        return False, "git apply failed: %s" % (tail[-1] if tail else "unknown error"), base
    result = _union_conflicts(main, branch, base, paths=sorted(stages))
    if not result.get("ok"):
        ug.cleanup_applied(main, base, branch)
        return False, result.get("reason"), base
    return True, "applied with the registration union (%s)" % result.get("reason"), base


def land_branch(main: str, branch: str, units: list[str] | None = None, base: str | None = None,
                no_build: bool = False, allow_regression: list[str] | None = None,
                check_outbox: bool = True, release_claims: bool = True,
                subject: str | None = None, no_selftests: bool = False) -> int:
    """The one-command landing: clean tree -> record-base -> apply+union -> gate -> commit -> release.

    Idempotent and loud: every refusal prints one `REFUSED <branch> | <reason>` line (stdout) and leaves
    MAIN exactly as it was found - the apply is undone whenever the landing did not reach a commit, so a
    refused landing is never a half-landing.  The registered unit(s) come from the branch's registration
    diff when `--units` is not given, so a unit renamed at registration is landed under its real name.
    """
    norm = [claims.norm_unit(u.strip("/")) for u in (units or []) if u.strip()]
    bad_message = message_error(subject)
    if bad_message:
        clear_land_message(main)
        print("REFUSED %s | %s" % (branch, bad_message))
        return 1
    bad_branch = branch_error(main)
    if bad_branch:
        clear_land_message(main)
        print("REFUSED %s | %s" % (branch, bad_branch))
        return 1
    if not claims.branch_exists(main, branch):
        clear_land_message(main)
        print("REFUSED %s | no such branch" % branch)
        return 1
    dirty = require_clean_tree(main)
    if dirty:
        clear_land_message(main)
        print("REFUSED %s | %s" % (branch, dirty))
        return 1
    merge_base = base or git(["merge-base", "main", branch], main).strip()
    if not norm:
        norm = units_from_branch(main, branch, merge_base)
    if not norm:
        clear_land_message(main)
        print("REFUSED %s | could not read the branch's registered unit(s) from its configure.py/"
              "splits.txt diff; pass --units explicitly" % branch)
        return 1
    # Read the resolve-helper state now, while `branch` still exists: the landing below may release and
    # delete it, and the containment proof is against `branch`'s pre-land tip.
    helper_redundant, helper_refused = resolve_helper_state(main, branch)
    record_base(main, norm)                # on the clean tree, BEFORE the pick; snapshots the base refs
    head_before = git(["rev-parse", "HEAD"], main).strip()
    ok, why, applied_base = apply_branch(main, branch, base=merge_base)
    if not ok:
        clear_land_message(main)
        print("REFUSED %s | %s (the apply was undone; main is unchanged)" % (branch, why))
        return 1
    code = land(main, norm, None, no_build, allow_regression, check_outbox=check_outbox,
                release_claims=release_claims, subject=subject, branch=branch,
                no_selftests=no_selftests)
    if code != 0 and git(["rev-parse", "HEAD"], main).strip() == head_before:
        # the gate refused before committing: undo the apply so a refused landing is not a half-landing
        ug.cleanup_applied(main, applied_base, branch)
        print("NOTE: the apply was undone - main is back at %s" % head_before[:8], file=sys.stderr)
    if code == 0:
        # the branch landed: its resolve helper (if any) is debris now. Delete only what is provably
        # contained; refuse loudly on anything else rather than leaving a `land/*` ref and hoping.
        _sweep_resolve_helpers(main, branch, helper_redundant, helper_refused)
    return code


def outside_batch(paths: list[str], allowed: tuple[str, ...] = ALLOWED_PREFIXES,
                  allowed_files: tuple[str, ...] = ALLOWED_FILES) -> list[str]:
    """Paths a batch may not touch: everything the plan keeps for the orchestrator, minus its own writes."""
    bad = []
    for path in paths:
        if path in allowed_files or path.startswith(allowed):
            continue
        bad.append(path)
    return bad


def is_scratch(path: str) -> bool:
    """True for tool scratch output in the repo root (`d<digits>.json` / `t<digits>.json`).

    Deliberately narrow: only the repo root's `d`/`t` + digits `.json` shape. A file with that name anywhere
    else, or any other path - `src/`, `include/`, `config/`, `configure.py`, a header, a `tools/` script - is
    the batch guard's business and is still refused loudly (a name is not a permission).
    """
    return bool(SCRATCH_JSON.match(path))


def scratch_paths(paths: list[str]) -> list[str]:
    """The tolerated subset of `paths`, in order."""
    return [p for p in paths if is_scratch(p)]


def scratch_note(paths: list[str]) -> str:
    """The line that NAMES tolerated scratch - a tolerated path is never silently dropped."""
    noun = "path" if len(paths) == 1 else "paths"
    return ("NOTE: %d tool scratch %s outside this batch - named here, never staged, never a refusal: %s"
            % (len(paths), noun, ", ".join(paths)))


def unstage_scratch(main: str, paths: list[str]) -> tuple[list[str], list[str]]:
    """De-index tolerated scratch; -> (paths whose index entry is gone, paths still staged).

    A staged copy is possible (the landing flow's `git add -A`, or a hand `git add`), and a staged file
    bypasses `.gitignore` - which is exactly how `d910.json` reached a gate that then refused it. `git reset
    HEAD -- <path>` drops the index entry and leaves the file in the worktree untouched, so the caller's dump
    is not destroyed. A `git reset` that fails is reported, never claimed: the caller has to unstage by hand
    before a `git commit` without a pathspec.
    """
    staged = set(git(["diff", "--cached", "--name-only"], main).splitlines())
    victims = [p for p in paths if p in staged]
    if not victims:
        return [], []
    p = run(["git", "reset", "-q", "HEAD", "--", *victims], main)
    return (victims, []) if p.returncode == 0 else ([], victims)


def tolerate_scratch(main: str, paths: list[str], act: bool = True) -> str:
    """Note (and, unless `act` is False, de-index) tolerated scratch; return the note line.

    `verify --dry-run` touches nothing, so it passes `act=False` and only names what it would have removed.
    """
    note = scratch_note(paths)
    if act:
        removed, held = unstage_scratch(main, paths)
        if removed:
            note += " (a staged copy was removed from the index)"
        if held:
            note += (" (WARNING: %s is staged and `git reset` failed - unstage it by hand before any "
                     "`git commit` without a pathspec)" % ", ".join(held))
    return note


# A foreign path whose *name* looks like a lane's scratch names the likely cause, so the pre-flight's report
# is actionable the moment it is printed - not after a 5-minute build.  A lane launched with its cwd set to
# MAIN leaves exactly these behind (`.tmp-mwcc/upstream/` was the 2026-09-27 case).
LANE_SCRATCH_MARKERS = (".tmp-", ".ws-", "tmp-")


def likely_cause(path: str) -> str | None:
    """A named cause for a foreign path that looks like lane scratch - `None` when the name says nothing.

    Deliberately name-based and conservative: this only *suggests* a cause in the pre-flight report, it never
    changes the verdict.  What it names is the failure this lane exists for - a lane launched in MAIN rather
    than in its slot.
    """
    parts = path.replace("\\", "/").split("/")
    if any(p.startswith(m) for p in parts for m in LANE_SCRATCH_MARKERS):
        return ("looks like lane scratch in MAIN - a lane was launched with its cwd set to MAIN (or cloned "
                "its upstream there) instead of working in its slot; delete it and relaunch the lane with "
                "the slot as its cwd (`queue.py next` prints that line)")
    if any(p.startswith(".slot") for p in parts):
        return ("a slot directory sits inside MAIN - slots are siblings of MAIN, never inside it")
    if "upstream" in parts:
        return ("a cloned upstream repository left in MAIN - typically the same mis-launched lane")
    return None


def preflight_foreign(main: str) -> list[dict]:
    """Foreign paths already in `main` **before** the build, each with a likely cause.  Read-only.

    The same information the post-build refusal prints (`land`'s `paths outside the batch appeared during the
    build`), delivered *before* the expensive work: a refusal that arrives after a 5-minute build is the same
    information, late.  It does not change the verdict - a path that appears during the build is still refused
    afterwards - it just makes the common case (a foreign path already there) cost a second, not minutes.
    """
    rows = changed_status(main)
    outside = outside_batch([path for _code, path in rows])
    scratch = set(scratch_paths(outside))
    return [{"path": p, "cause": likely_cause(p)} for p in outside if p not in scratch]


def preflight_report(main: str, foreign: list[dict] | None = None) -> str | None:
    """The pre-flight report line(s), or `None` when the tree already holds no foreign path."""
    foreign = preflight_foreign(main) if foreign is None else foreign
    if not foreign:
        return None
    lines = ["PRE-FLIGHT | %d path(s) outside this batch are already in %s BEFORE the build:"
             % (len(foreign), main)]
    for row in foreign:
        lines.append("  %s%s" % (row["path"], "  <- %s" % row["cause"] if row["cause"] else ""))
    lines.append("  the post-build gate would refuse these too; they are another lane's or a mis-launch's "
                 "scratch, not this batch's - remove them first")
    return "\n".join(lines)


# --------------------------------------------------------------------------------------------------
# rule 2 at the range boundary: a batch that REGISTERS a range is what makes the symbols inside it owned
# (docs/plan.md 6.5 rule 2), so any declaration of one of them still living in `include/unsplit/<band>.h`
# has just become a violation - and, when the new unit's own definition disagrees, the `(10505) illegal
# overloading` that costs a full build (`-maxerrors 1` only shows it one symbol at a time).
#
# stylelint `--diff` cannot see it: the band header is NOT a file the batch changed, so it is not in the
# diff's file set at all, and the registration edit itself (splits.txt + configure.py) carries no
# declaration to flag. The 2026-09-26 incident: a batch registered `fn_8019E9AC`'s cluster, the band header
# still declared it `s32` while the new unit defined it `u32`, and the gate only found out after a full
# build (`(10505) illegal overloading 'fn_8019E9AC(_ENEMY_WORK *, long)'`).
#
# The check is deliberately a WARNING, never a refusal: a redeclaration whose signature happens to agree
# builds cleanly, and the range can own data or functions the band legitimately spelled the same way - so
# a refusal would not be provably safe (it could block a batch that builds). An extra signal that names the
# symbol, the owner and the stale header is the whole point; the build remains the arbiter.
#
# Every rule-2 warning that says "move the declaration into the owner's header" carries this caveat, because
# that instruction is not always true and following it blindly broke `main` on 2026-09-26: commit `756023c4e`
# moved `fn_80335CE8` into its owner's header (hud/fn_80334568), whose prototype is THREE parameters, while
# the three call sites in `Pl/fn_80273B14.cpp` pass TWO - so they lost their declaration ((10140) undefined
# identifier) and the unit stopped compiling, with `ok` green because it is `NonMatching`. There was no
# malformed input, only an instruction that is not always true.
# --------------------------------------------------------------------------------------------------

RULE2_CALLSITE_CAVEAT = ("after checking every call site: moving a declaration changes its arity if the "
                         "owner's prototype differs")


def _split_rows(text: str) -> list[tuple[str, str, int, int]]:
    """`(unit, section, start, end)` rows of a `splits.txt` text.

    The same parse `sharedfiles.parse_ranges`/`stylelint._parse_splits` use: a unit header is an
    unindented `name:` line, every range is an indented `start:0x.. end:0x..` line. Kept local so the
    check can parse the *base* text `git show` returns without a temp file.
    """
    rows: list[tuple[str, str, int, int]] = []
    cur = None
    for line in text.splitlines():
        if line.startswith("Sections:"):
            continue
        if line[:1] not in (" ", "\t") and line.rstrip().endswith(":"):
            cur = line.strip()[:-1]
            continue
        m = re.match(r"^\s+(\S+)\s+start:(0x[0-9A-Fa-f]+)\s+end:(0x[0-9A-Fa-f]+)", line)
        if m and cur:
            rows.append((cur, m.group(1), int(m.group(2), 16), int(m.group(3), 16)))
    return rows


def _merge_intervals(spans: list[tuple[int, int]]) -> list[tuple[int, int]]:
    """Sorted, merged, non-overlapping `(start, end)` spans."""
    out: list[tuple[int, int]] = []
    for s, e in sorted(spans):
        if s >= e:
            continue
        if out and s <= out[-1][1]:
            out[-1] = (out[-1][0], max(out[-1][1], e))
        else:
            out.append((s, e))
    return out


def _subtract_intervals(span: tuple[int, int], coverage: list[tuple[int, int]]
                        ) -> list[tuple[int, int]]:
    """The parts of `span` no merged `coverage` interval covers."""
    s, e = span
    out: list[tuple[int, int]] = []
    cur = s
    for cs, ce in coverage:
        if ce <= cur:
            continue
        if cs >= e:
            break
        if cs > cur:
            out.append((cur, cs))
        cur = max(cur, ce)
        if cur >= e:
            break
    if cur < e:
        out.append((cur, e))
    return out


def added_split_ranges(old_rows: list[tuple[str, str, int, int]],
                       new_rows: list[tuple[str, str, int, int]]) -> list[tuple[str, str, int, int]]:
    """Ranges `new_rows` claims that `old_rows` did not - i.e. what the batch newly registers.

    A brand-new unit's whole block is added; an existing unit whose `.text` was *widened* contributes only
    the strip the old range did not already cover (the range the symbols newly inside are owned by). A
    range the batch did not touch is covered by the old rows and contributes nothing, so a clean batch
    yields no added range and the check stays silent.
    """
    old_cov: dict[str, list[tuple[int, int]]] = {}
    for _unit, section, s, e in old_rows:
        old_cov.setdefault(section, []).append((s, e))
    for section in old_cov:
        old_cov[section] = _merge_intervals(old_cov[section])
    out: list[tuple[str, str, int, int]] = []
    for unit, section, s, e in new_rows:
        for cs, ce in _subtract_intervals((s, e), old_cov.get(section, [])):
            out.append((unit, section, cs, ce))
    return out


def _ranges_by_section(rows: list[tuple[str, str, int, int]]) -> dict:
    """`Stylelint.Ownership`'s ranges shape: {section: [(start, end, unit)]}."""
    out: dict = {}
    for unit, section, s, e in rows:
        out.setdefault(section, []).append((s, e, unit))
    return out


def _band_header_paths(main: str) -> list[str]:
    """Every `include/unsplit/` header, as an absolute path (the whole band, changed or not)."""
    base = os.path.join(main, "include", "unsplit")
    out: list[str] = []
    for dirpath, _dirnames, filenames in os.walk(base):
        for name in sorted(filenames):
            if name.endswith((".h", ".hpp", ".hh")):
                out.append(os.path.join(dirpath, name))
    return sorted(out)


def _changed_band_headers(main: str, base: str) -> list[str]:
    """The `include/unsplit/` headers this batch added or modified (a delete is not a declaration site)."""
    p = run(["git", "diff", "--name-status", "-M", "--diff-filter=d", base, "--", "include/unsplit"],
            main)
    if p.returncode != 0:
        return []
    out: list[str] = []
    for line in (p.stdout or "").splitlines():
        parts = line.split("\t")
        if len(parts) >= 2 and parts[-1].endswith((".h", ".hpp", ".hh")):
            out.append(parts[-1])
    return out


def _declared_names(main: str, rel: str) -> set[str]:
    """The file-scope declaration names of `rel` (absolute path), read from the worktree."""
    path = os.path.join(main, *rel.replace("/", os.sep).split(os.sep))
    try:
        text = open(path, encoding="utf-8", errors="replace", newline="").read()
    except OSError:
        return set()
    return {name for name, _line in sl.header_declarations(sl.Source(path, rel, text))}


def _declared_names_at(main: str, base: str, rel: str) -> set[str]:
    """The file-scope declaration names of `rel` at `base`; empty when it did not exist there."""
    p = run(["git", "show", "%s:%s" % (base, rel)], main)
    if p.returncode != 0:
        return set()
    return {name for name, _line in sl.header_declarations(sl.Source(rel, rel, p.stdout))}


def _defer_count(text: str) -> int:
    """How many `rule 7 deferred: <reason>` declarations a file carries.

    The spelling is still matched by `stylelint.RULE7_DEFER_RE`, but the lint no longer honours it (the
    no-exemption ruling deleted the key); this row is a second, narrower guard on the escape's growth.
    """
    return len(sl.RULE7_DEFER_RE.findall(text))


# a file name (or a symbol) that is a generated stem rather than a name: rule 7's defect class.
_GENERATED_STEM_RE = re.compile(r"^(?:fn|lbl|unk)_[0-9A-Fa-f]{8}$")
_GENERATED_FN_RE = re.compile(r"\bfn_[0-9A-Fa-f]{8}\s*\(")


def generated_fn_definitions(text: str) -> list[str]:
    """The generated `fn_XXXXXXXX` names `text` *defines* (a body, not a prototype) - the file's own.

    Rule 7's escape defers the `fn_` half of a file, and the ruling is that it may only defer names the
    file does not own: `void fn_802B2978(void);` is a *reference* to another unit's symbol and is
    tolerated, while `void fn_802B2978(void) { ... }` is this file's own name left generated. The test
    is syntactic, on stylelint's comment-and-literal-blanked `code` view: the `(` after a generated
    name must close on a `{` rather than on a `;`.
    """
    code = sl.strip(text)[0]
    out: list[str] = []
    for m in _GENERATED_FN_RE.finditer(code):
        depth, i = 0, m.end() - 1
        while i < len(code):
            if code[i] == "(":
                depth += 1
            elif code[i] == ")":
                depth -= 1
                if depth == 0:
                    break
            i += 1
        j = i + 1
        while j < len(code) and code[j] in " \t\r\n":
            j += 1
        if j < len(code) and code[j] == "{":
            out.append(m.group(0).rstrip(" \t("))
    return sorted(set(out))


def rule7_defer_growth(main: str, base: str | None) -> list[str]:
    """The batch's OWN symbols left generated behind a `rule 7 deferred` escape - the naming refusal.

    The owner's ruling (2026-09-26) is a hybrid, and this row enforces the batch's own half only: a
    generated name is never a resting place for a unit being written, so a file that *grows* an escape
    may not (1) be registered at a generated file name (`src/enemy/fn_8033041C.cpp`) or (2) leave a
    generated `fn_` name **defined** by it.

    References to *other* units' unrenamed symbols are tolerated - they are not this batch's to fix,
    and in a wholly unnamed region they are most of rule 7's findings (the 803253bc lane: 42 of 68
    findings were callee names owned by ~10 other units, with 0 rename candidates anywhere in the
    band). The escape also stays available for the files registered before the rule, so this is a
    check on GROWTH, not on presence: rewriting a header that already carries the comment is safe
    (1 -> 1 passes); adding one to a file that did not have it is examined. `grep -rn "rule 7
    deferred" src/` remains the complete list of files that use it.

    A *bodyless* file is not examined even when its escape is new: this row only judges a written unit,
    and the file's name is provisional - `enemy/fn_8033041C.cpp` is the seam re-draw's second half, whose
    band has no name evidence at all. The *lint* is not so lenient - its key 2 is gone, so a bodyless file's
    generated names are `--diff` additions that refuse a batch on their own. When the unit is written it
    grows bodies and this row then demands the name too (`ef/fn_803432B4.cpp`, 33 generated definitions, is
    refused for exactly that).

    Returns one line per offender, sorted. `[]` when `base` is unknown, no source file changed, or the
    growth is in files that name their own symbols.
    """
    if not base:
        return []
    touched = run(["git", "diff", "--name-only", base, "--", "src", "include"], main)
    if touched.returncode != 0 or not (touched.stdout or "").strip():
        return []
    offenders: list[str] = []
    for rel in touched.stdout.split():
        if not rel.endswith((".c", ".cpp", ".h", ".hpp", ".cc")):
            continue
        new_text = ""
        path = os.path.join(main, rel)
        if os.path.exists(path):
            new_text = open(path, encoding="utf-8", errors="replace", newline="").read()
        old = run(["git", "show", "%s:%s" % (base, rel)], main)
        if _defer_count(new_text) <= _defer_count(old.stdout if old.returncode == 0 else ""):
            continue
        # a bodyless unit is not "being written" yet: it is rule-7-exempt anyway (key 2, stylelint's own
        # `text_has_bodies`) and its file name is provisional - `enemy/fn_8033041C.cpp` is the seam
        # re-draw's second half, whose band has no name evidence at all. Once it carries bodies the row
        # demands the name (the 803432b4 unit is the case: 33 generated definitions, refused).
        if not brief_mod.text_has_bodies(new_text):
            continue
        if _GENERATED_STEM_RE.match(os.path.splitext(os.path.basename(rel))[0]):
            offenders.append("%s: registered at a generated file name" % rel)
            continue
        defs = generated_fn_definitions(new_text)
        if defs:
            # The list must be COMPLETE: a worker who renames only the names it was shown is refused again
            # on the next run, and that loop cost two lanes a full unit of work each (the 803432b4 unit
            # defines 33 generated names, the eft053 batch 8 - both messages showed only three). The count
            # goes first so the scale of the job is visible in one line.
            offenders.append("%s: defines %d generated name(s): %s"
                             % (rel, len(defs), ", ".join(defs)))
    return sorted(offenders)


ALLOW_RULE10: list[str] = []


def set_allow_rule10(keys: list[str] | None) -> None:
    """Record the rule-10 keys *this invocation* accepts deliberately - the command's own audit trail.

    The row lives in `verify()`, which `land()` and the CLI's `--dry-run` path both reach, so the value
    travels on the module rather than through two more signatures. It is set only from the command line and
    printed by the row; nothing in a file can grant it, which is the difference between this and the
    `rule 7 deferred` key the no-exemption ruling removed.
    """
    global ALLOW_RULE10                                                   # noqa: PLW0603 - one invocation
    ALLOW_RULE10 = [a.strip() for a in (keys or []) if a.strip()]


def rule10_violations(main: str, text_ref: str | None = None) -> dict | None:
    """`{key: {"unit", "where", "kind"}}` for every rule-10 violation in the tree as it stands.

    The key shape is `vtableaudit.violation_rows`'s, and it is **rename-stable**: `run:<section>:<addr>`
    for a code-pointer run inside the unit's own registered ranges that our object neither emits nor
    references, and `ref:<file>:<line>:<symbol>` for a source assignment to a `+0x00` function-pointer-table
    member whose table the unit itself owns (with `<file>` translated through the batch's renames when
    `text_ref` is the base). `text_ref` judges the text half as of that revision - the gate passes the
    batch base for its BEFORE snapshot, so a unit the batch re-homed (plan §12) must not read as seven
    added violations. `None` when the audit cannot read the tree at all (a missing DOL, a broken
    `configure.py`) - the row then says so instead of refusing every batch.
    """
    try:
        sweep = vta.sweep(main, text_ref=text_ref)
    except Exception as exc:                                   # noqa: BLE001 - the row must never crash
        print("rule 10: vtableaudit could not read this tree (%s)" % exc, file=sys.stderr)
        return None
    rename = vta.rename_map(main, text_ref) if text_ref else {}
    rows = {}
    for key, row in vta.violation_rows(sweep, rename).items():
        rows[key] = {"unit": claims.norm_unit(row["unit"]), "kind": row["kind"],
                     "where": row["where"]}
    return rows


def rule10_growth(before: dict, after: dict, units: list[str]) -> tuple[list[str], list[str]]:
    """`(added_keys, rows_for_the_batch_units)` - the rule-10 row's decision, as a pure function.

    ADD-only, like the lint's `--diff`: a key present before the batch is grandfathered, a key the batch
    introduced is a refusal. The second element is the report the row prints even when it passes - the
    violation set for the units the batch touches, so a silent pass (rule 10's old "landing review"
    classification) cannot happen again.
    """
    grew = sorted(set(after) - set(before))
    mine = {claims.norm_unit(u) for u in units}
    touched = [after[k]["where"] for k in sorted(after) if after[k]["unit"] in mine]
    return grew, touched


ALLOW_RULE12: list[str] = []


def set_allow_rule12(tokens: list[str] | None) -> None:
    """Record the rule-12 tokens *this invocation* accepts deliberately - the command's own audit trail.

    Mirrors `set_allow_rule10`: the value travels on the module (the row lives in `verify`, which both
    `land()` and the CLI's `verify` path reach), it is set **only** from a command line - never from a key
    in a file, which is what the no-exemption ruling removed - and the row prints it, so the landing log
    carries the token it excused. The token is the at-fault symbol name rule 12 fires on (the `extern`'s
    identifier), the rename-stable key `stylelint --list-added` prints.
    """
    global ALLOW_RULE12                                                   # noqa: PLW0603 - one invocation
    ALLOW_RULE12 = [t.strip() for t in (tokens or []) if t and t.strip()]


def rule12_verdict(added: list[dict], detail: list[dict],
                   allowed: list[str]) -> tuple[bool, list[str], list[str], list[dict]]:
    """The rule-12 half of the style-lint row, as a pure function.

    `added`/`detail` are `stylelint --diff --json`'s: the `(rule, file)` count deltas and the added
    occurrences (each carrying its `token`). Only rule-12 additions can ever be excused by
    `--allow-rule12 <token>`; a single added violation of any other rule refuses the row. Returns
    `(ok, excused, refused, other_rules)`. An allowance that matches nothing is not an error by itself -
    it simply excuses nothing, and the refusal it was meant for stands unless its own token is named.
    """
    other = [a for a in added if a.get("rule") != 12]
    if other:
        return False, [], [], other
    sanctioned = set(allowed)
    tokens = [d.get("token") for d in detail if d.get("rule") == 12]
    excused = sorted({t for t in tokens if t and t in sanctioned})
    refused = sorted({t for t in tokens if not t or t not in sanctioned})
    rule12_added = sum(int(a.get("added") or 0) for a in added if a.get("rule") == 12)
    if len(tokens) < rule12_added:
        # the JSON named fewer added occurrences than the counts: the named tokens cannot be proven to
        # cover them, so it refuses. A silent pass on an unverifiable delta is the failure this guards.
        refused.append("<unnamed rule-12 occurrence>")
    return (not other and not refused), excused, refused, []


def rule12_lint_row(p, allowed: list[str] | None) -> tuple[bool, str, str, list[str]]:
    """The style-lint row's `(ok, detail, info, excused)` with `--allow-rule12` in play.

    Rule 12 is refused inside the style lint (it is part of its `--diff`), so the allowance has to be
    applied to *its* decision. `--json` carries the added `(rule, file)` counts and the added
    occurrences; a clean run passes, anything the allowance does not cover - any other rule, or a
    rule-12 token not named - is the refusal, with what stylelint printed. A non-zero exit whose JSON
    carries no `added` (a refusal that is not a delta: no map, no dump) is a failure too, never a pass.
    """
    if p.returncode == 0:
        return True, "", "", []
    try:
        payload = json.loads(p.stdout or "{}")
    except ValueError:
        return False, command_detail(p), "", []
    added = payload.get("added") or []
    if not added:
        return False, command_detail(p), "", []
    detail = payload.get("detail") or []
    ok, excused, refused, _other = rule12_verdict(added, detail, allowed or [])
    if ok:
        return True, "", ("rule 12: %d addition(s) authorised by --allow-rule12" % len(excused)
                           if excused else ""), excused
    lines = ["+%d rule %d %s (%d -> %d)" % (a.get("added", 0), a.get("rule"), a.get("file"),
                                            a.get("before", 0), a.get("after", 0)) for a in added]
    if refused:
        lines.append("unexcused rule 12 (add --allow-rule12 <token> to accept deliberately): %s"
                     % ", ".join(refused))
    return False, "; ".join(lines[:6]) or command_detail(p), "", excused


def band_ownership_warnings(main: str, base: str | None) -> list[str]:
    """Rule-2 warnings a batch introduces at the registration boundary. Never a refusal.

    Two directions, both keyed on the batch's own diff:

    1. the batch REGISTERS a range (splits.txt/configure.py changed): every symbol of `symbols.txt` the
       range newly covers is now owned, so any `include/unsplit/<band>.h` that still *declares* one is a
       rule-2 violation and a candidate `illegal overloading` - whether that header was changed or not;
    2. the batch ADDS a declaration to a band header of a symbol an already-registered unit owns
       (ownership unchanged): the band is a fallback, not the owner.

    Returns one warning line per finding, sorted. `[]` when the map is absent, `base` is unknown, the batch
    touches neither registration file, or nothing is newly owned - so a clean batch is silent.
    """
    if not base:
        return []
    spl_rel = "config/RMHE08/splits.txt"
    sym_path = os.path.join(main, "config", "RMHE08", "symbols.txt")
    spl_path = os.path.join(main, "config", "RMHE08", "splits.txt")
    if not (os.path.exists(sym_path) and os.path.exists(spl_path)):
        return []
    touched = run(["git", "diff", "--name-only", base, "--", spl_rel, "configure.py"], main)
    if touched.returncode != 0 or not (touched.stdout or "").strip():
        return []
    old = run(["git", "show", "%s:%s" % (base, spl_rel)], main)
    if old.returncode != 0:
        return []
    new_text = open(spl_path, encoding="utf-8", errors="replace", newline="").read()
    added = added_split_ranges(_split_rows(old.stdout), _split_rows(new_text))
    changed_band = _changed_band_headers(main, base)
    if not added and not changed_band:
        # no new ownership and no band header the batch touched: nothing this check reasons about
        return []

    symbols = sl._parse_symbols(sym_path)  # the 4.5 MB map, parsed here and never printed
    old_own = sl.Ownership(symbols, _ranges_by_section(_split_rows(old.stdout)))

    # 1. symbols the batch's new ranges now cover (via their address), grouped by the owner the range names.
    newly: dict[str, str] = {}
    if added:
        for name, entries in symbols.items():
            if len(entries) != 1:
                continue
            section, address, _type = entries[0]
            for unit, asec, s, e in added:
                if asec == section and s <= address < e:
                    newly[name] = unit
                    break
    # A band header may declare a C++ callee by its clean spelling (`em_act_ck(...)`) while symbols.txt
    # carries the compiler mangling (`em_act_ck__FP11_ENEMY_WORKUcUc`). Match the mangled symbol's base
    # name too, so the rule-9-correct spelling is not a blind spot; the message names the full symbol.
    newly_bases: dict[str, list[str]] = {}
    for name in newly:
        # NOTE: this must not be called `base` - that is this function's own parameter (the batch's
        # base ref, used further down for `git show`), and the earlier shadowing made every
        # pre-existing band-header declaration look newly added (46 spurious warnings on one fold).
        symbol_stem = name.split("__", 1)[0]
        if symbol_stem != name and sl.RULE9_MANGLED_RE.match(name):
            newly_bases.setdefault(symbol_stem, []).append(name)

    warnings: list[str] = []
    if newly:
        for path in _band_header_paths(main):
            rel = os.path.relpath(path, main).replace(os.sep, "/")
            try:
                text = open(path, encoding="utf-8", errors="replace", newline="").read()
            except OSError:
                continue
            src = sl.Source(path, rel, text)
            for name, line in sl.header_declarations(src):
                owner = newly.get(name)
                if owner is not None:
                    warnings.append(
                        ("WARNING: %s:%d declares `%s`, which this batch's registration now makes owned "
                         "by `src/%s` - move the declaration into that unit's header and #include it, "
                         + RULE2_CALLSITE_CAVEAT +
                         " (docs/plan.md 6.5 rule 2); a mismatched signature is the `(10505) illegal "
                         "overloading` that only a full build would show") % (rel, line, name, owner))
                    continue
                for full in sorted(newly_bases.get(name, [])):
                    warnings.append(
                        ("WARNING: %s:%d declares `%s`, the C++ spelling of `%s`, which this batch's "
                         "registration now makes owned by `src/%s` - declare it in the owner's header "
                         "and #include it, " + RULE2_CALLSITE_CAVEAT +
                         " (docs/plan.md 6.5 rule 2)")
                        % (rel, line, name, full, newly[full]))

    # 2. a declaration this batch ADDS to a band header, of a symbol a unit already owned before the batch.
    for rel in changed_band:
        now_names = _declared_names(main, rel)
        before_names = _declared_names_at(main, base, rel)
        if not now_names:
            continue
        path = os.path.join(main, *rel.replace("/", os.sep).split(os.sep))
        try:
            text = open(path, encoding="utf-8", errors="replace", newline="").read()
        except OSError:
            continue
        src = sl.Source(path, rel, text)
        for name, line in sl.header_declarations(src):
            if name in before_names or name in newly:
                continue
            res = old_own.resolve(name)
            if res is None or res.get("kind") != "owned":
                continue
            warnings.append(
                ("WARNING: %s:%d newly declares `%s`, already owned by `src/%s` - the band is a "
                 "fallback, not the owner: declare it in the owner's header and #include it, "
                 + RULE2_CALLSITE_CAVEAT + " (docs/plan.md 6.5 rule 2)") % (rel, line, name, res["unit"]))

    return sorted(dict.fromkeys(warnings))


def flips_objects(main: str) -> bool:
    """True when configure.py gains `Object(Matching, ...)` relative to HEAD - the batch flips something."""
    p = run(["git", "diff", "HEAD", "--", "configure.py"], main)
    return bool(re.search(r"^\+.*Object\(\s*Matching", p.stdout or "", re.M))


def regression_rows(changes_json: str) -> list[tuple[str, str, float, float]]:
    """(unit, measure, before, after) for every measure that went down, from a report_changes.json."""
    if not os.path.exists(changes_json):
        return []
    data = json.loads(open(changes_json, encoding="utf-8").read())
    rows = []

    def walk(node):
        if isinstance(node, dict):
            name = node.get("name") or ""
            for key, value in (node.get("measures") or {}).items():
                if isinstance(value, dict) and "old" in value and "new" in value:
                    old, new = value["old"], value["new"]
                    if isinstance(old, (int, float)) and isinstance(new, (int, float)) and new < old - 1e-9:
                        rows.append((name, key, old, new))
            for key in ("units", "children", "categories"):
                for child in (node.get(key) or []):
                    walk(child)
        elif isinstance(node, list):
            for child in node:
                walk(child)

    walk(data)
    return [r for r in rows if "auto_" not in r[0]]


def report_snapshot(main: str) -> dict:
    """Per-unit measures and the sub-100 % symbols - the evidence a batch's delta is judged against.

    `build/RMHE08/report_changes.json` only carries DOL-level totals, and `ninja baseline` (which this tool runs
    at the end of a batch) rewrites the very baseline the comparison would need: two consecutive verifies of the
    same tree therefore both report "no regression" while the ledger says matched 231 -> 228. So the snapshot is
    taken at `record-base` and kept in `.pi/`, where nothing overwrites it.
    """
    path = os.path.join(main, "build", "RMHE08", "report.json")
    if not os.path.exists(path):
        return {}
    data = json.loads(open(path, encoding="utf-8").read())
    out = {}
    for unit in data.get("units", []):
        name = unit.get("name") or ""
        measures = unit.get("measures") or {}
        symbols = {}
        for fn in unit.get("functions") or []:
            pct = fn.get("fuzzy_match_percent", fn.get("match_percent"))
            if fn.get("name") and isinstance(pct, (int, float)) and pct < 100.0:
                symbols[fn["name"]] = round(float(pct), 4)
        if measures or symbols:
            out[name] = {
                "fuzzy": measures.get("fuzzy_match_percent"),
                "matched_code": measures.get("matched_code"),
                "symbols": symbols,
            }
    return out


def unit_grew(prior: dict, after: dict) -> bool:
    """True when a unit only **gained** bodies/symbols - an extension, not a loss.

    A widened splits range (or a head joined to its tail) adds functions to an already-registered unit; the
    unit's average `fuzzy` then falls because the weaker new bodies joined it, which is exactly what an
    extension does and is not a regression. Two signals say "grew" and either is enough: the unit's sub-100 %
    symbol set gained a name the previous report did not hold, or its matched-byte count rose. Both are read
    from the `record-base` snapshot, so a unit re-measured identically is not "grown" and the unit-average
    comparison still sees it (`report_regressions`).
    """
    prior_syms = prior.get("symbols") or {}
    after_syms = after.get("symbols") or {}
    if len(after_syms) > len(prior_syms) or any(s not in prior_syms for s in after_syms):
        return True
    bm, am = prior.get("matched_code"), after.get("matched_code")
    return isinstance(bm, (int, float)) and isinstance(am, (int, float)) and am > bm + 1e-9


def report_regressions(before: dict, after: dict, allow: list[str]) -> tuple[list[tuple], list[tuple]]:
    """-> (unauthorised, authorised) regressions as (unit, what, before, after).

    A regression is measured **per symbol**: a symbol the previous report held whose own score dropped is a
    regression, and the row names the symbol and both numbers. A symbol the previous report did not hold is
    NEW - a body this batch added - and is never a regression, however weak; that is what extending an
    already-registered unit does, and refusing it blocks every legitimate extension (the gate did exactly
    that to the g3d_resshp head-plus-tail join and to the 800997e0 extension before 2026-09-25).

    The unit's own `fuzzy` is an average over its symbols, so it can fall while every symbol holds or improves
    - the weaker new symbols joined. `unit_grew` catches that case and the unit-average row is skipped; it
    fires only for a unit that did **not** grow, where something really dropped and no per-symbol row would
    name it, and it names the unit and both numbers. `allow` names units whose measured regression an explicit
    rule authorised (rule 8 of §6.5 costs score, and that cost is measured, not hidden).
    """
    unauthorised, authorised = [], []
    for unit, after_vals in after.items():
        prior = before.get(unit)
        if not prior or "auto_" in unit and "/auto/" not in unit:
            continue          # the auto_* scaffold losing symbols to a real unit is bookkeeping, not a regression
        hit_allowed = any(a in unit for a in allow)
        rows = []
        for sym, bpct in (prior.get("symbols") or {}).items():
            apct = (after_vals.get("symbols") or {}).get(sym)
            if apct is None:
                continue          # reached 100 %: not a regression
            if isinstance(apct, (int, float)) and apct < bpct - 1e-9:
                rows.append((unit, sym, bpct, apct))
        # the average only speaks when no symbol does, and never for a unit that merely grew: `unit_grew`
        # already said the fall is the weaker new bodies joining, which is not a regression to name.
        if not rows and not unit_grew(prior, after_vals):
            af, bf = prior.get("fuzzy"), after_vals.get("fuzzy")
            if isinstance(af, (int, float)) and isinstance(bf, (int, float)) and bf < af - 1e-9:
                rows.append((unit, "unit fuzzy", af, bf))
        (authorised if hit_allowed else unauthorised).extend(rows)
    return unauthorised, authorised


def ledger_numbers(main: str) -> dict:
    """The ledger's totals, mapped to the names the message uses (`ledger.py --json` nests them)."""
    p = run([sys.executable, os.path.join("tools", "units", "ledger.py"), "--json"], main)
    if p.returncode != 0:
        return {}
    try:
        data = json.loads(p.stdout)
    except json.JSONDecodeError:
        return {}
    totals = data.get("totals") or data
    return {
        "covered": totals.get("claimed_functions"),
        "closed": totals.get("closed"),
        "partial": totals.get("partial"),
        "unclaimed": totals.get("unclaimed"),
        "matched": totals.get("matched_functions"),
        "bytes": totals.get("matched_code"),
        "total_code": totals.get("total_code"),
        "matched_percent": totals.get("fuzzy_match_percent"),
    }


def summary(before: dict, after: dict) -> str:
    def num(value):
        try:
            return int(value)
        except (TypeError, ValueError):
            return value if isinstance(value, (int, float)) else None

    keys = ("covered", "closed", "partial", "matched", "bytes")
    parts = []
    for key in keys:
        b, a = num(before.get(key)), num(after.get(key))
        if isinstance(b, (int, float)) and isinstance(a, (int, float)):
            parts.append("%s %s -> %s" % (key, b, a))
    return ", ".join(parts) or "(ledger numbers unavailable)"


def branch_commits(main: str, unit: str) -> int:
    """How many commits the unit's worker branch has that main does not already have.

    The batch base is deliberately *not* part of this. A worker's branch is cut when the unit is claimed,
    and the orchestrator records a fresh base for every batch it lands, so `base..branch` is only
    meaningful while that base is main's HEAD; in a multi-batch round the recorded base is stale, which
    makes the count report 0 for work that *is* committed (or miss it entirely). The check's intent is
    simply "the worker's work exists as commits of its own on its branch that landing has not taken yet" -
    `main..branch`. A branch that predates the base but carries its own commit passes; a branch with
    nothing beyond main fails.

    The branch comes from the claim (`claims.claim_branch`), not from re-deriving it here: the unit may be
    spelled with or without its source extension, and the stored branch is the lock.
    """
    unit = claims.norm_unit(unit.strip("/"))
    branch = claims.claim_branch(main, unit)
    if not claims.branch_exists(main, branch):
        return 0
    p = run(["git", "rev-list", "--count", "main..%s" % branch], main)
    return int(p.stdout.strip()) if p.returncode == 0 and p.stdout.strip().isdigit() else 0


def outbox_units(main: str, units: list[str], branch: str | None = None) -> tuple[list[str], list[str]]:
    """-> (units whose outbox validates, problems).

    `branch`, when given (`land --branch`), locates the outbox by the branch's slug - the name `brief.py`
    writes - instead of re-deriving it from the unit path.  That is what covers a unit **renamed at
    registration**: its outbox keeps the pre-registration slug, so the unit-derived path misses it while
    the branch-derived one finds it.  When the branch-derived path is missing too, the failure names
    `--no-outbox` as the remedy, because a missing record must be stated plainly, not hidden.

    The outbox is validated against the **batch's** owned symbols, not one unit's: a branch that registers or
    touches several units writes one outbox naming all of them, and a per-unit check flagged every other
    unit's symbols as "not owned".
    """
    ok, problems = [], []
    branch_slug = claims.slug_of_branch(branch) if branch else None
    # The ownership check reads the *batch's* units as one set: a branch that registers or touches several
    # units writes one outbox naming all of them, and validating it against a single unit flags every symbol
    # of the others as "not owned" - a refusal whose only documented escape is `--no-outbox`, which turns the
    # outbox check off entirely. A symbol owned by no unit in the batch is still an error, and a single-unit
    # batch is unchanged.
    batch_owned = handoff_mod.owned_symbols(main, [claims.norm_unit(u.strip("/")) for u in units])
    for unit in units:
        unit = claims.norm_unit(unit.strip("/"))
        if branch_slug:
            path = os.path.join(main, ".pi", "outbox", branch_slug + ".json")
        else:
            path = handoff_mod.outbox_path(main, unit)
        if not os.path.exists(path):
            hint = (" (a unit renamed at registration keeps its outbox under the pre-registration slug; "
                    "if the record is demonstrably fine, --no-outbox is the remedy)")
            problems.append("%s: no outbox at %s%s" % (unit, path, hint))
            continue
        entry = json.loads(open(path, encoding="utf-8").read())
        errors, _warnings = handoff_mod.validate(entry, batch_owned)
        if errors:
            problems.extend("%s: %s" % (unit, e) for e in errors)
        else:
            ok.append(unit)
    return ok, problems


def release_plan(checks: list[tuple], units: list[str], release_claims: bool,
                 check_outbox: bool = True) -> list[str]:
    """The units whose claim `verify` releases: every gated unit, but only once every check so far passed.

    Releasing is a side effect, so it must not run behind a failed gate - a refused batch has to leave its
    worker's branch and worktree exactly as they are, or the retry has nothing to re-run. `--no-release`
    turns the step off entirely.

    `check_outbox` is an input that **does not affect the answer**, and that is the point: the old
    `--no-worker-units` flag skipped the outbox check *and* the release, so a round that passed it to quiet
    an outbox problem silently stopped tearing its workers down (18 worktrees and 3 dead claims left behind,
    2026-09-23). Outbox checking and release are separate opt-outs now.
    """
    del check_outbox  # independent of the release decision by design
    if not release_claims or not units:
        return []
    if any(not row[1] for row in checks):
        return []
    return list(units)


def restore_rescued_branch(main: str, unit: str) -> str | None:
    """Put a `--force`-released worker's branch back from its rescue ref. Returns the branch, or None.

    A `release --force` deletes the branch (it is the lock and must go) and parks its only copy of the work
    at `refs/rescue/<slug>`. What the gate cares about is that the work *exists as commits*, so rather than
    refuse a missing branch whose work is demonstrably preserved - the 2026-09-26 dead end, where land.py's
    own refusal named the rescue ref and the exact `git branch` command and the round still had to run it by
    hand before a manual commit - the gate restores the branch itself and carries on. Nothing is destroyed:
    the branch points at the rescue ref's commit, and a later teardown sees it exactly as the normal flow
    would (merged into main after the landing, so the release removes it).
    """
    branch = claims.claim_branch(main, unit)
    if claims.branch_exists(main, branch):
        return None
    rescue = claims.rescue_exists(main, unit)
    if not rescue:
        return None
    p = run(["git", "update-ref", "refs/heads/%s" % branch, rescue], main)
    return branch if p.returncode == 0 else None


def commits_ahead_of_main(main: str, ref: str) -> bool:
    """True when `ref` carries commits `main` does not already have - the gate's "the work exists" test."""
    p = run(["git", "rev-list", "--count", "main..%s" % ref], main)
    return p.returncode == 0 and p.stdout.strip().isdigit() and int(p.stdout.strip()) > 0


def branch_problems(main: str, units: list[str], branch: str | None = None) -> list[str]:
    """One line per unit whose worker branch does not carry its work as commits.

    A missing branch is the 2026-09-23 shape: `release --force` on an unreported worker deleted the branch
    (its only copy of the work) and left it at `refs/rescue/<slug>`. What the gate actually needs is that the
    work *exists as commits*, so a rescue ref that carries commits `main` does not is accepted as the
    branch's work - the 2026-09-26 report's case (a). The landing path additionally restores the real branch
    from that ref (`restore_rescued_branch`, and only when not `--dry-run`, which touches nothing), so the
    teardown still has a branch to release. A missing branch with no rescue ref behind it, or a rescue ref
    with no commits of its own, is still reported.

    `branch`, when given (`land --branch`), is the branch being landed and is checked directly - a unit
    renamed at registration is not reachable through its registered name, but its branch is what was named.
    """
    if branch:
        if claims.branch_exists(main, branch) and commits_ahead_of_main(main, branch):
            return []
        return ["%s (branch %s has no commits of its own)" % (branch, branch)]
    problems = []
    for u in units:
        branch = claims.claim_branch(main, u)
        if not claims.branch_exists(main, branch):
            rescue = claims.rescue_exists(main, u)
            if rescue and commits_ahead_of_main(main, rescue):
                continue          # the work exists as commits at the rescue ref: that is what the gate wants
            if rescue:
                problems.append("%s (rescue ref %s has no commits of its own; no branch %s)"
                                % (u, rescue, branch))
            else:
                problems.append("%s (no branch %s)" % (u, branch))
        elif branch_commits(main, u) == 0:
            problems.append("%s (branch %s has no commits of its own)" % (u, branch))
    return problems


# --------------------------------------------------------------------------------------------------
# the compile gate: `ninja build/RMHE08/ok` is structurally blind to a unit that does not compile. A
# `NonMatching` unit's object is never linked, so the DOL hash stays green with any number of uncompilable
# units in the tree - measured twice in one session (2026-09-26): a partial file left by a malformed
# cherry-pick, and a declaration moved out from under three call sites, both reached `main` with `ok` green
# and were found much later, by a full build failing elsewhere. The failing unit compiles in about a second,
# and `ninja -k 0` is seconds rather than minutes because only changed objects rebuild.
#
# The scoping is the point. Other dirty work in MAIN can be uncompilable for reasons that are not this
# batch's, so the check must not fail for it. `ninja -k 0` builds every dirty object once; only a `FAILED:`
# output that IS one of the batch's own object targets (`build/RMHE08/src/<unit>.o` for a `--units` entry) is
# counted. A foreign object's failure is named and tolerated. The unit object is what "our source still
# compiles" means, and the object is exactly what `ok` never links.
# --------------------------------------------------------------------------------------------------

_OBJECT_TARGET = "build/RMHE08/src/%s.o"
_FAILED_LINE = re.compile(r"^FAILED:\s+(\S+)", re.M)


def compile_targets(units: list[str]) -> list[str]:
    """The ninja object targets of the batch's own units (`--units`), normalised and de-duplicated."""
    out: list[str] = []
    for unit in units:
        norm = claims.norm_unit(unit.strip("/"))
        if norm:
            out.append(_OBJECT_TARGET % norm)
    return list(dict.fromkeys(out))


def failed_compile_outputs(output: str) -> list[str]:
    """Every output path a ninja run reported as `FAILED: <path>`, de-duplicated, in order."""
    seen: list[str] = []
    for m in _FAILED_LINE.finditer((output or "").replace("\\", "/")):
        if m.group(1) not in seen:
            seen.append(m.group(1))
    return seen


def batch_compile_failures(units: list[str], output: str) -> tuple[list[str], list[str]]:
    """-> (the batch's units whose object FAILED, foreign FAILED outputs).

    Scoping: only a failed output that is one of the batch's own `build/RMHE08/src/<unit>.o` targets is a
    failure of *this* batch. Every other failed output is another stream's dirty work in MAIN, which the
    batch did not touch and must not answer for - named, never counted, so a compile gate cannot make a
    passing batch fail for a reason that is not its own.
    """
    want: dict[str, str] = {}
    for unit in units:
        norm = claims.norm_unit(unit.strip("/"))
        if norm:
            want[_OBJECT_TARGET % norm] = norm
    bad: list[str] = []
    foreign: list[str] = []
    for path in failed_compile_outputs(output):
        unit = want.get(path)
        if unit is None:
            foreign.append(path)
        elif unit not in bad:
            bad.append(unit)
    return bad, foreign


def compile_check(main: str, units: list[str], runner=None) -> tuple[bool, str]:
    """Do the batch's own units still compile? -> (ok, detail) after one `ninja -k 0`.

    `ninja build/RMHE08/ok` is deliberately not asked (that is the DOL check); this runs the compile itself,
    once: `-k 0` keeps going past the first error, so every failure is visible in one run and a foreign
    failure cannot hide a batch unit's. The "did it compile" verdict is scoped by object target through
    `batch_compile_failures`, so another stream's broken dirty unit is reported, not counted.

    `runner` is a `run([...])`-shaped callable and exists for the selftest: it lets the scoping be exercised
    in both directions without a real build tree.
    """
    if not units:
        return True, "no batch unit named"
    if not os.path.exists(os.path.join(main, "build.ninja")):
        return True, "no build.ninja - the configure.py gate owns that"
    run_fn = runner or (lambda args: run(args, main))
    p = run_fn(["ninja", "-k", "0"])
    output = (p.stdout or "") + (p.stderr or "")
    bad, foreign = batch_compile_failures(units, output)
    if bad:
        return False, "FAILED to compile: %s" % ", ".join(bad)
    if p.returncode != 0 and not foreign:
        # ninja failed without naming an output: not a compile failure of a unit we can scope, so it is not
        # silently read as a pass - the anomaly is the detail.
        tail = [l.strip() for l in output.splitlines() if l.strip()]
        return False, "ninja -k 0 exited %d without a FAILED target: %s" % (p.returncode, tail[-1] if tail else "")
    detail = "all %d batch unit object(s) compiled" % len(compile_targets(units))
    if foreign:
        detail += (" (tolerated: %d FAILED foreign dirty target(s), not this batch's: %s)"
                   % (len(foreign), ", ".join(foreign[:3])))
    return True, detail


def verify(main: str, units: list[str], base: str | None, dry_run: bool, no_build: bool,
           allow_regression: list[str] | None = None, check_outbox: bool = True,
           release_claims: bool = True, problems: list[str] | None = None,
           branch: str | None = None, no_selftests: bool = False) -> int:
    # `problems` is the out-parameter an automated caller (`land`) reads: `"<failing check> [<KIND>]: <what
    # it printed> (remedy: ...)"` per failed check, so its refusal can name the gate and its kind instead of
    # saying only "the gate failed". `verify`'s
    # own stdout keeps the check table; the exit status stays the answer.
    # a unit's *name* is its path without the source extension (`claims.norm_unit`): `Camellia/camellia` and
    # `Camellia/camellia.c` are one batch, and the gate must key its outbox, branch and splits the same way
    # whichever the orchestrator typed.
    units = [claims.norm_unit(u.strip("/")) for u in units]
    # A `--units` entry is a batch PATH rather than a unit when it is a file the batch stages rather than a
    # translation unit - see `is_batch_path` and `unit_rows`: it carries a file extension, or the tree has
    # something at that exact path (`.gitignore`, `LICENSE`, `docs`).  A path has no `Object(...)` line, no splits.txt
    # block and no `build/RMHE08/src/<unit>.o` target, so the unit-shaped rows below (outbox, branch, registration,
    # compile, target drift) must not assert unit properties about it. It stays in `units` for the staging and
    # ledger rows, which is how the path is committed. (2026-09-27: a tool-only batch could not land at all
    # before the extension signal - the registration row refused every `tools/` path.)
    unit_units = unit_rows(main, units, base or read_base(main).get("base"))
    allow_regression = [a.strip() for a in (allow_regression or []) if a.strip()]
    checks: list[tuple[str, bool, str, str, str, str]] = []

    def check(name: str, good: bool, detail: str = "", info: str = "",
              kind: str = KIND_GATE, remedy: str = "") -> None:
        checks.append((name, good, detail, info, kind, remedy))

    # 1. ground truth
    truth = pc.ground_truth_error()
    check("ground truth (build.sha1 == the DOL's hash)", not truth, "; ".join(truth))

    # 2. the batch base: main must not have moved since the batch opened
    recorded = read_base(main)
    want_base = base or recorded.get("base")
    head = git(["rev-parse", "HEAD"], main).strip()
    if want_base:
        check("main has not moved since the batch base", head == want_base,
              "HEAD %s != base %s - a worker committed to main, or another stream landed"
              % (head[:8], want_base[:8]), info="base %s" % (want_base or "?")[:8],
              kind=KIND_BOOKKEEPING,
              remedy="re-record the batch base (`python tools/units/land.py record-base`) once main is the "
                     "tree the batch applies to; if a worker committed to main, undo that first")
    else:
        check("batch base recorded", False, "no base: run `land.py record-base` when the batch opens",
              kind=KIND_BOOKKEEPING,
              remedy="run `python tools/units/land.py record-base` when the batch opens, then re-run the "
                     "landing")

    # 3. the tree guard + the outbox of every unit in the batch
    paths = changed_paths(main)
    bad = outside_batch(paths)
    scratch = scratch_paths(bad)
    bad = [p for p in bad if p not in scratch]
    if scratch:
        # tolerated, but NAMED: the guard's intent is a loud refusal for foreign work, and a path it refuses
        # must never have been staged by this gate - so tolerated scratch is reported, not swallowed.
        print(tolerate_scratch(main, scratch, act=not dry_run), file=sys.stderr)
    check("every changed path belongs to a batch", not bad, "not allowed in a batch: %s" % ", ".join(bad),
          info=("tool scratch tolerated (not staged): %s" % ", ".join(scratch)) if scratch else "")
    # 3b. the cheapest defect to catch early: a committed conflict marker. The build reports it as a syntax
    # error in whichever file carries it, so a full compile buys one line's worth of news, and a marker
    # survives review because it looks like ordinary text. Scoped to the batch's own files - another
    # stream's dirty file is not this batch's to fix - and to the two markers a conflict writes.
    markers = conflict_marker_files(main, land_stageable(units, changed_status(main),
                                                        base_dirty_paths(main)))
    check("no batch file carries a git conflict marker", not markers,
          "; ".join("%s:%d %s" % (p, n, m) for p, n, m in markers[:6]),
          remedy="resolve the conflict in that file and re-commit it - a marker is not source, and the build "
                 "only reports it as a syntax error, in a file that need not be the one the merge touched")
    if unit_units and check_outbox:
        # NOTE: a fresh name for the outbox problems. Reusing the `problems` out-parameter here rebound it
        # locally and the failed-check list never reached the caller's `land` refusal (2026-09-26).
        ok_units, outbox_problems = outbox_units(main, unit_units, branch=branch)
        check("every unit's outbox validates", not outbox_problems, "; ".join(outbox_problems[:4]),
              kind=KIND_BOOKKEEPING,
              remedy="the source is fine - have the worker re-run brief.py to rewrite its outbox, or re-run "
                     "with --no-outbox for an orchestrator-only batch")
        # a `--force` release leaves the work at refs/rescue/<slug>. `branch_problems` already accepts that
        # ref as the branch's work, and the landing path (never `--dry-run`, which touches nothing) restores
        # the real branch from it so the teardown still has a branch to release (2026-09-26 case (a)).
        if not dry_run:
            for u in unit_units:
                restored = restore_rescued_branch(main, u)
                if restored:
                    print("NOTE: %s's branch %s was gone but its work is preserved at %s - restored the "
                          "branch from the rescue ref (a `--force` release had parked it there)"
                          % (u, restored, claims.rescue_ref_name(u)), file=sys.stderr)
        uncommitted = branch_problems(main, unit_units, branch=branch)
        check("every unit's branch carries its work as commits", not uncommitted,
              "no commits of its own on the branch (work left uncommitted in the worktree?): %s"
              % ", ".join(uncommitted),
              kind=KIND_BOOKKEEPING,
              remedy="if the commits are at refs/rescue/<slug> land.py restores the branch for you; "
                     "otherwise have the worker commit its work on the branch, then re-run")
    elif units:
        check("orchestrator-only batch (no worker outboxes to check)", True,
              info="%d unit(s): %s" % (len(units), ", ".join(units)))
    else:
        check("batch units named", False, "pass --units (or --no-outbox for an orchestrator-only batch)",
              kind=KIND_BOOKKEEPING,
              remedy="pass --units a,b (or --no-outbox for an orchestrator-only batch)")

    # 4. the style lint (7.21), when it exists
    lint = os.path.join(main, "tools", "units", "stylelint.py")
    if os.path.exists(lint):
        # `--json` carries the added (rule, file) counts and the added occurrences, so rule 12's own
        # allowance (`--allow-rule12 <token>`) can be applied to the lint's decision: a token the command
        # line sanctioned is excused and printed here; anything else the delta added still refuses.
        p = run([sys.executable, lint, "--diff", want_base or "HEAD", "--json"], main)
        lint_ok, lint_detail, lint_info, lint_excused = rule12_lint_row(p, ALLOW_RULE12)
        if lint_excused:
            print("rule 12: %d authorised by --allow-rule12 (recorded, not a file-level exemption): %s"
                  % (len(lint_excused), "; ".join(lint_excused)))
        # the head of the output, never the tail: stylelint prints its findings first and its "not enforced"
        # legend last, so a tail hides the violation the batch has to fix (2026-09-25, `.pi/land.log`).
        check("style lint (§6.5) adds no violation", lint_ok, lint_detail, info=lint_info,
              remedy="rule 12 refuses an `extern` of data no registered `splits.txt` range covers: claim "
                     "the range into the unit (the whole map symbol extent, `end:` 4-aligned), or "
                     "register a named data-only unit when several units read the pool - "
                     "`python tools/units/dataclaim.py --unit <unit>` prints the claim. A rule-12 "
                     "addition a landing must take now, with the claim already scheduled, is accepted "
                     "by `--allow-rule12 <token>` (recorded in the landing log). Rule 13 refuses a "
                     "`<Type>_<name>(<Type>* self, ...)` free function (a member spelled the C way): "
                     "declare `name` in the class, define `Type::name`, rename the map row to the "
                     "mangling and sweep the call sites (`python tools/units/methodize.py <Type>` prints "
                     "the plan), or mark a genuine C function `/* free: <retail C linkage evidenced|SDK "
                     "C struct> */` on the declaration.")
    else:
        check("style lint (§6.5)", True, info="not built yet (roadmap 7.21) - skipped")

    # 4b. every tool's own selftest, except the explicitly parked pre-existing failures. The 2026-09-27
    # incident: `measure_selftest.py` was red for weeks while 31 lanes filed "recompile.py is broken" - the
    # tool's own test said so and nothing ran it. `tools/selftest.py` runs both shapes (`*_selftest.py` and
    # `<tool> --selftest`), in parallel with a per-test timeout, and reports "green except N parked" against
    # `tools/selftests-known-failures.json`, so one old red cannot hide every new one.
    selftests = os.path.join(main, "tools", "selftest.py")
    if no_selftests:
        check("all tool selftests pass (except the parked list)", True,
              info="--no-selftests (fast path) - the suite did not run")
    elif os.path.exists(selftests):
        p = run([sys.executable, selftests, "--json"], main)
        check("all tool selftests pass (except the parked list)", p.returncode == 0,
              selftest_detail(p),
              remedy="fix the named tool's selftest, or park a *pre-existing* failure in "
                     "tools/selftests-known-failures.json with a reason and a date (explicit and greppable, "
                     "never a silent skip); `python tools/selftest.py --changed` is a lane's fast loop")
    else:
        check("all tool selftests pass (except the parked list)", True,
              info="tools/selftest.py not built yet")

    # 4c. the registration boundary (Backlog #1): a range this batch registers makes the symbols inside it
    # owned, so a declaration of one of them still sitting in `include/unsplit/<band>.h` is now a rule-2
    # violation - and the `(10505) illegal overloading` a mismatched signature costs a full build to show.
    # This is an ADDITIONAL signal: it is a WARNING, never a failed check, because a compatible
    # redeclaration is legal and no refusal here is provably safe (see `band_ownership_warnings`).
    band_warnings = band_ownership_warnings(main, want_base)
    for warning in band_warnings:
        print(warning, file=sys.stderr)
    check("rule 2 registration boundary (warning)", True,
          info=("%d newly-owned symbol declaration(s) still in include/unsplit/*.h - see the WARNING "
                "lines above" % len(band_warnings)) if band_warnings
               else "no newly-owned symbol left declared in include/unsplit/*.h")

    # A second, narrower naming guard on top of stylelint's (the no-exemption ruling, 2026-09-27, deleted
    # rule 7's keys; this row keeps its own growth test on the escape's spelling). It refuses a batch that
    # grows a `rule 7 deferred` escape for a name the batch's own unit defines, or at a generated file stem.
    # See `rule7_defer_growth` for the two conditions and why a bodyless file is skipped here.
    defer_growth = rule7_defer_growth(main, want_base)
    check("no batch leaves its own symbols generated behind a `rule 7 deferred` escape",
          not defer_growth,
          detail="%d file(s) grew a `rule 7 deferred` escape for a name the batch owns: %s"
                 % (len(defer_growth), "; ".join(defer_growth[:4])),
          remedy="name the symbols the batch's own unit defines - derive a name from context and mark "
                 "the guess in the unit header - and register the unit at a named path; an added reference "
                 "to another unit's unrenamed symbol is now a stylelint `--diff` finding too, so name it as "
                 "well",
          info="no file in the batch grew a `rule 7 deferred` escape")

    # 4d. the gate's own subject follows the convention (CLAUDE.md's commit-message convention). The gate
    # composes every landing's subject through `land_subject`; the row runs `commitlint.py` over it, so the
    # convention and its checker cannot drift: a category `land_subject` invents, or a message past 120
    # characters, fails the batch here instead of landing. The tool is called, never re-implemented - and its
    # exit 2 ("nothing checked") is a failure, so a lint that did not run cannot pass this row.
    subject = land_subject(units)
    ok_subject, subject_detail = subject_lint(main, subject)
    check("the gate's own subject follows the convention", ok_subject, subject_detail, info=subject,
          remedy="the composed subject `%s` is a commitlint violation, so the batch would land a message "
                 "CLAUDE.md's convention forbids; fix `land_subject` (or the batch's unit names) so the "
                 "category is a known member and the message is at most 120 characters, then re-run - "
                 "`python tools/git/commitlint.py --message \"<subject>\"` reproduces it" % subject)

    before = recorded.get("ledger") or ledger_numbers(main)
    flip = flips_objects(main)
    ok_file = os.path.join(main, "build", "RMHE08", "ok")
    elf_file = os.path.join(main, "build", "RMHE08", "main.elf")

    if dry_run:
        for row in checks:
            name, good, detail, info = row[:4]
            note = (detail if not good else "") or info
            print("%s %s%s" % ("PASS" if good else "FAIL", name, (" - " + note) if note else ""))
        print("\nwould then: delete build/RMHE08/ok%s, run configure.py -> registration gate (configure.py "
              "+ splits.txt + build graph) -> compile gate (ninja -k 0, scoped to the batch's own "
              "objects) -> ninja -> report.json -> target-object drift + independent per-symbol "
              "re-measure -> regression scan -> ok -> ledger -> baseline"
              % (" and main.elf (this batch flips an object)" if flip else ""))
        if problems is not None:
            problems.extend(failing_checks(checks))
        return 0 if all(row[1] for row in checks) else 1

    failures = [row[0] for row in checks if not row[1]]
    if failures:
        for row in checks:
            name, good, detail, info = row[:4]
            note = (detail if not good else "") or info
            print("%s %s%s" % ("PASS" if good else "FAIL", name, (" - " + note) if note else ""))
        if problems is not None:
            problems.extend(failing_checks(checks))
        print("\n" + failure_summary(checks))
        return 1
    if problems is not None:
        problems.extend(failing_checks(checks))

    def gate(name: str, args: list[str]) -> bool:
        p = run(args, main)
        check(name + " (exit %d)" % p.returncode, p.returncode == 0, command_detail(p))
        return p.returncode == 0

    # 5. the build, with the proof that the `ok` we read is this run's
    for stale in [ok_file] + ([elf_file] if flip else []):
        if os.path.exists(stale):
            os.remove(stale)
    started = time.time_ns()
    # the split target objects as the gate finds them, i.e. before configure.py/ninja re-split the
    # batch's ranges: compared after the build, a unit the batch does not name whose object moved is a
    # `splits.txt` change that re-ranged a neighbour (the merger's strongest form, .pi/notes/8031a6c0).
    before_targets = vu.target_object_snapshot(main)
    # rule 10 (vtable ownership), the pre-build half: the batch's working tree already carries the head
    # text, and `configure.py` below re-splits and overwrites the objects, so the BEFORE snapshot has to be
    # taken here - text from the batch base (`want_base`) and objects as the base built them.
    rule10_before = rule10_violations(main, text_ref=want_base)
    built = gate("configure.py", [sys.executable, "configure.py"])
    # the split, before the registration check (2026-09-26, .pi/notes/8030681c-gate-finding.md): the
    # per-unit rules are generated from build/RMHE08/config.json, and build.ninja itself depends on it
    # (`build build.ninja objdiff.json: configure | build\RMHE08\config.json`), so a configure.py run
    # that precedes the split regenerates the graph from the *previous* analyzed config. A batch whose
    # splits.txt block is new is then absent from build.ninja and the registration check below fails on
    # the first attempt and passes on the retry, because the `ninja` row further down is what ran the
    # split. Run the split (a no-op when it is current), then configure.py again so the graph the check
    # reads carries the batch's own units.
    if built and os.path.exists(os.path.join(main, "build.ninja")):
        gate("split (config.json)", ["ninja", "build/RMHE08/config.json"])
        built = gate("configure.py (after the split)", [sys.executable, "configure.py"]) and built
    # the registration gate (2026-09-26): a unit can be committed as a *source file* with its
    # `configure.py` `Object(...)` line and its `splits.txt` block left behind, and then it is
    # registered in name only and absent from the build - `ok` stays green (`NonMatching` is never
    # linked) and the compile gate has no `build/RMHE08/src/<unit>.o` target to scope to. Assert all
    # three axes after configure.py has regenerated the graph.
    if built:
        ok_reg, reg_detail = vu.registration_check(main, unit_units)
        check("every batch unit is registered (configure.py + splits.txt + build graph)", ok_reg,
              reg_detail,
              remedy="commit the unit's `Object(...)` line in configure.py and its splits.txt block, "
                     "then re-run configure.py so build.ninja carries build/RMHE08/src/<unit>.o - a "
                     "source file alone is registered in name only and never enters the build")
    # the compile gate, before the link proves anything: `ok` cannot see a `NonMatching` unit's object (it is
    # never linked), so this is the only check that answers "does our source still compile". Scoped to the
    # batch's own `build/RMHE08/src/<unit>.o` targets - a foreign dirty object that fails is named and
    # tolerated (`compile_check`). One `ninja -k 0`; the full `ninja` below then only links.
    if built:
        ok_compile, compile_detail = compile_check(main, unit_units)
        check("every batch unit compiles (compile gate)", ok_compile, compile_detail,
              remedy="make the batch unit's source compile (`ninja -k 0` names the error above); "
                     "`ninja build/RMHE08/ok` cannot see this because a `NonMatching` unit is never linked")
        # the relocation-name row (2026-09-28): the object is built, so its relocations can be read. A
        # different relocation *name* scores the same, so refuse a unit that ADDS a call to a symbol no
        # link input defines - the flip would answer `undefined: '<name>'` even at 100 %. Add-only, like
        # the style lint's `--diff`: 61 landed units already carry such a reference, and refusing that
        # pre-existing debt would block a batch for what it did not create. The base's own unresolved set
        # was snapshotted at `record-base`; the pre-existing remainder is reported, never refused. Cheap:
        # the batch's own objects plus a cached link-symbol index (`undefrefs.link_symbol_index`).
        result = uref.check_units(main, unit_units, base_snapshot=recorded.get("undefrefs") or {},
                                  base=want_base)
        check("every batch unit's relocations resolve against the link (no new undefined reference)",
              not result["problems"], "; ".join(result["problems"][:4]),
              info=("; ".join(result["pre_existing"][:3]) if result["pre_existing"]
                    else "no batch unit adds a name the link cannot provide"),
              remedy="our object ADDS a relocation to a name no `symbols.txt` row and no other link input "
                     "defines, so a flip would answer `undefined: '<name>'` even when the row reads 100 %. "
                     "The refusal names the target's own spelling where it records a different one - match "
                     "that spelling and its map row; `python tools/units/undefrefs.py <unit>` prints the "
                     "detail. A pre-existing wrong reference is reported, not refused (the `--census` "
                     "register is where it is worked down)")
        for line in result["pre_existing"]:
            print("note: %s" % line, file=sys.stderr)
        if result["missing"]:
            check("the batch base carries an unresolved-reference snapshot for every unit", False,
                  "record-base did not snapshot: %s" % ", ".join(result["missing"][:4]),
                  kind=KIND_BOOKKEEPING,
                  remedy="re-run `python tools/units/land.py record-base --units <batch units>` at the base "
                         "(it compiles the base objects and caches their unresolved references)")
    built = gate("ninja", ["ninja"]) and built
    gate("report.json", ["ninja", "build/RMHE08/report.json"])
    # rule 10 (vtable ownership), the row: a table of code pointers inside a unit's own ranges must be
    # compiler output (a `virtual` class emits it and the store), so a batch that ADDS a violation is
    # refused. The comparison is add-only, exactly like the lint's `--diff`: the tree already carries
    # violations (`fn_80429B94.cpp`'s seven, `fn_80423E74.cpp`, `ai/fn_802CC794.cpp`, and the units that
    # hand-assign their own unclaimed vtable, `Network/fn_803D3CE8.cpp` among them before this batch), and
    # refusing those would refuse every batch forever. The set for the batch's own units is printed even
    # when the row passes - a silent pass is what rule 10's old "landing review" classification bought.
    rule10_after = rule10_violations(main)
    if rule10_before is None or rule10_after is None:
        check("rule 10 (vtable ownership) adds no violation", True,
              info="vtableaudit could not read this tree - the row is skipped (see the note above)")
    else:
        grew, touched_rows = rule10_growth(rule10_before, rule10_after, unit_units)
        # A sanctioned claim is accepted by an explicit, recorded allowance (`--allow-rule10 <key>`) - never
        # by a key in a file, the same reasoning that removed `rule 7 deferred` - and the acceptance is
        # printed here so the landing's own log carries the exception **and** the key it excused. An
        # allowance that matches nothing is visible too: it stays out of `accepted` and the row still
        # refuses, so a stale allowance cannot quietly keep excusing a key that no longer exists.
        authorised = set(ALLOW_RULE10)
        accepted = sorted(k for k in grew if k in authorised)
        grew = sorted(k for k in grew if k not in authorised)
        if accepted:
            print("rule 10: %d authorised by --allow-rule10 (recorded, not a file-level exemption): %s"
                  % (len(accepted), "; ".join(accepted)))
        check("rule 10 (vtable ownership) adds no violation", not grew,
              detail="%d added: %s" % (len(grew), "; ".join(grew[:4])),
              info=("rule 10 report for this batch: %s" % "; ".join(touched_rows)) if touched_rows
                   else "no owned-but-unemitted code-pointer run or own-range vtable write in the batch's "
                        "units",
              remedy="declare the class with its `virtual` methods and let MWCC emit the table and the "
                     "store (rule 10 / playbook 52), or claim the `.data` range and emit it; run "
                     "`python tools/units/vtableaudit.py --unit <unit>` for the detail")
    # the split target objects after the re-split: a unit the batch does not name must be byte-identical.
    after_targets = vu.target_object_snapshot(main)
    drift = vu.target_drift_problems(before_targets, after_targets, unit_units)
    check("no unit's split target object moved under the batch (a neighbour re-ranged)", not drift,
          "; ".join(drift[:4]),
          remedy="the batch's splits.txt re-ranged a unit it does not name; include that unit in the "
                 "batch or fix its registration anchor, then re-run")
    # the independent per-symbol re-measure (2026-09-26): the regression scan reads the same
    # report.json the batch was measured against, so a stale or wrong report is invisible to it. This
    # re-runs `objdiff report generate` itself over the objects and compares symbol-for-symbol, then
    # checks the score against the raw bytes and the unit's own arithmetic.
    ok_ind, ind_detail, ind_warn = vu.verify_units(main, units)
    check("per-symbol re-measure reproduces the report from the objects", ok_ind, ind_detail,
          info="; ".join(ind_warn[:3]) if ind_warn else "",
          remedy="a symbol's report score is not reproducible from the objects (a stale report.json, "
                 "a measuring-tool lie, or a 100% claim whose bytes differ) - rebuild and re-read, or "
                 "fix the symbol, before landing. A row dtk named itself (`pad_*`/`auto_*`, a range "
                 "with no function prologue) is resolved to our symbol at the same section+offset "
                 "and judged by its bytes, because objdiff cannot pair such a row by name at all - "
                 "so a dtk-generated row name is never the reason on its own")
    # the regression scan reads build/RMHE08/report_changes.json, which only `ninja changes` writes: without
    # this the scan reads the PREVIOUS batch's file and passes for the wrong reason (the first real run did
    # exactly that, while the ledger showed matched 231 -> 228).
    gate("ninja changes (DOL-level totals, informational)", ["ninja", "changes"])
    before_report = recorded.get("report") or {}
    after_report = report_snapshot(main)
    unauthorised, authorised = report_regressions(before_report, after_report, allow_regression)
    for row in authorised:
        print("note: regression ALLOWED by --allow-regression: %s %s %.2f -> %.2f" % row)
    check("no symbol or unit regressed", not unauthorised,
          "; ".join("%s %s %.2f -> %.2f" % r for r in unauthorised[:5]),
          info=("%d authorised regression(s)" % len(authorised)) if authorised else "")
    used = {a for a in allow_regression if any(a in row[0] for row in authorised)}
    stale_allow = [a for a in allow_regression if a not in used]
    check("every --allow-regression was actually needed", not stale_allow,
          "stale allowance(s), remove them: %s" % ", ".join(stale_allow),
          kind=KIND_BOOKKEEPING,
          remedy="remove the stale --allow-regression flag(s) named above and re-run")
    if not before_report:
        check("the batch base carries a report snapshot", False,
              "record-base did not snapshot report.json (rebuild it and re-record the base)",
              kind=KIND_BOOKKEEPING,
              remedy="run `ninja build/RMHE08/report.json` then `python tools/units/land.py record-base`")
    all_regressed = [("", "", 0.0, 0.0)][:0] + [(u, w, b, a) for u, w, b, a in unauthorised + authorised]
    gate("ok (main.dol verified)", ["ninja", "build/RMHE08/ok"])
    fresh = os.path.exists(ok_file) and os.stat(ok_file).st_mtime_ns >= started
    check("ok was recreated by THIS run", fresh, "the ok stamp predates the run - it proves nothing")

    # 6. the ledger delta + the knowledge delta
    after = ledger_numbers(main)
    improved = any(isinstance(before.get(k), (int, float)) and isinstance(after.get(k), (int, float))
                   and after[k] > before[k] for k in ("closed", "matched", "bytes"))
    docs_changed = any(p.startswith(("docs/", "CLAUDE.md")) for p in paths)
    headers_changed = any(p.startswith("src/") and p.endswith((".c", ".cpp", ".cp")) for p in paths)
    check("knowledge delta present if the batch improved something",
          (not improved) or docs_changed or headers_changed,
          "a unit improved and no docs/CLAUDE.md/unit header changed in this batch (7.10)",
          remedy="record the improvement: touch the unit's header or a docs/ / CLAUDE.md file in the batch")

    # 7. the baseline, so the next batch's `ninja changes` compares against this one (7.16)
    if not no_build:
        gate("ninja baseline", ["ninja", "baseline"])

    # 8. the teardown (owner's rule): release the claim of every unit just gated. This runs after the build
    # checks and only when every one of them passed, so a refused batch keeps its branch and worktree for the
    # retry; an incomplete teardown is itself a failed check (a live pane is named, not silently kept).
    to_release = release_plan(checks, units, release_claims, check_outbox)
    for unit_name in to_release:
        out = claims.release(unit_name, main, force=False, dry_run=False)
        failed_step = next((s for s in out["steps"] if s["status"] == "failed"), None)
        note = out.get("refused") or (("failed: %s - %s" % (failed_step["label"], failed_step["why"]))
                                      if failed_step else "")
        info = "branch %s" % out["branch"]
        if out.get("release_ref"):
            info += "; un-merged commits rescued to %s" % out["release_ref"]
        check("claim released: %s" % unit_name, bool(out.get("complete")), note, info=info,
              kind=KIND_BOOKKEEPING,
              remedy="the batch is committed - finish the teardown by hand (close the pane / remove the "
                     "worktree), then re-run")
    if release_claims and units and not to_release:
        check("claim release deferred", True, info="a check above failed - the claim is left alone")
    elif units and not release_claims:
        check("claim release skipped", True, info="--no-release")

    print("%-58s %s" % ("check", "result"))
    for row in checks:
        name, good, detail, info = row[:4]
        note = (detail if not good else "") or info
        print("%-58s %s%s" % (name[:58], "PASS" if good else "FAIL", ("  " + note[:80]) if note else ""))

    body = [subject,
            "",
            "ledger: %s" % summary(before, after),
            "gates: ground truth ok, base %s, %d check(s), ok recreated=%s%s%s"
            % ((want_base or "?")[:8], len(checks), fresh, ", main.elf relinked" if flip else "",
               ", authorised regressions: %s" % ", ".join(sorted(allow_regression)) if allow_regression else ""),
            ""]
    if scratch:
        # the tolerance is part of the record, not a silent drop: the commit says what was left alone
        body.append("scratch: tool scratch outside the batch, not staged, not committed: %s"
                    % ", ".join(scratch))
        body.append("")
    if band_warnings:
        # the warning is part of the record too, so a batch that landed with one is greppable from the log
        body.append("rule 2 boundary: %d newly-owned symbol declaration(s) left in include/unsplit/*.h"
                    % len(band_warnings))
        for warning in band_warnings[:10]:
            body.append("  " + warning)
        if len(band_warnings) > 10:
            body.append("  ... (%d more)" % (len(band_warnings) - 10))
        body.append("")
    failed = [row[0] for row in checks if not row[1]]
    if failed:
        # a failed gate must not leave a message a `git commit -F .git/land_msg.txt` could pick up: the old
        # flow wrote it unconditionally, so a piped `| tail -3` read a green-looking summary and committed a
        # batch whose gate had failed (twice, 2026-09-23). No message exists unless every check passed.
        stale = clear_land_message(main)
        if stale:
            print("removed the stale %s (a failed gate has no committable message)" % os.path.relpath(stale, main))
        print("\nledger: %s" % summary(before, after))
        print(failure_summary(checks, prefix="FAILED"))
        return 1
    message = write_land_message(main, "\n".join(body))
    print("\nledger: %s" % summary(before, after))
    print("message written to %s - review it, then `git commit -F .git/land_msg.txt`" % message)
    print("READY: every check passed")
    return 0


def land(main: str, units: list[str], base: str | None, no_build: bool,
         allow_regression: list[str] | None = None, check_outbox: bool = True,
         release_claims: bool = True, subject: str | None = None,
         already_applied: bool = False, branch: str | None = None,
         no_selftests: bool = False, allow_rule10: list[str] | None = None) -> int:
    """The one command: gate -> stage the batch's files -> commit -> release, one answer line on stdout.


    The failure mode this closes: `verify`'s output was piped (`| tail -3`), the exit status was lost, and a
    batch whose gate had *failed* was committed by hand - twice, leaving a partial source on `main` while
    `ok` stayed green (the unit is `NonMatching`). So the gate's verdict is now the decision, not a report:

    * a red gate never reaches `git commit` (`land_decision`), and `verify` removes any stale message. A
      refusal NAMES the failing check and what it printed - a bare `the gate failed` is not actionable, and a
      reader who cannot see which gate failed cannot tell a real defect from a passing check (2026-09-25);
    * the refusal also names each check's KIND and its remedy. A GATE failure says "the gate failed"; a
      BOOKKEEPING failure (the batch is fine, the landing's own state is stale) says so and never "the gate
      failed", so a reader does not mistake a released branch or a stale base for a bad batch;
    * a worker branch a `--force` release parked at `refs/rescue/<slug>` is restored by the gate
      (`restore_rescued_branch`), because the gate only needs the work to exist as commits;
    * a batch applied before `record-base` (so its paths are in the base's dirty snapshot) is named as such,
      and `--already-applied` lands it rather than making the caller revert and re-record;
    * an empty or whitespace-only `--message` is refused before the gate runs (`message_error`): the empty
      shell substitution that expanded `$(cat /tmp/msg1.txt)` must not silently land the fallback subject;
    * a HEAD that is not `main` is refused before the gate runs (`branch_error`): a land run off `main` moves
      the wrong ref, and every later merge-base and branch diff is computed against a stale `main`;
    * the commit uses the gate's own message, so there is no separate `git commit -F` to get wrong;
    * the commit is `git commit -F msg -- <the batch's paths>`: no pathspec means the whole index, which
      swept another stream's staged edit into a land twice on 2026-09-23 (`85f3d4b5`, `d50fdd32`). Paths the
      index holds but the batch does not are left staged, and a warning names them;
    * the gate log goes to **stderr** and stdout carries exactly one answer line, so `tail -1` is the answer
      whether or not the exit status survived the pipe;
    * the exit status *is* the answer: 0 landed, 1 refused (or landed with an incomplete teardown).

    Releasing runs after the commit, never before: until `main` has the commits, the worker's branch is their
    only copy.
    """
    norm_units = [claims.norm_unit(u.strip("/")) for u in units]
    bad_message = message_error(subject)
    if bad_message:
        # a refusal must not leave a message `git commit -F .git/land_msg.txt` could pick up (2026-09-23)
        clear_land_message(main)
        print("REFUSED %s | %s" % (",".join(norm_units), bad_message))
        return 1
    bad_branch = branch_error(main)
    if bad_branch:
        # same rule as the message guard: a refusal leaves no committable message and touches nothing
        clear_land_message(main)
        print("REFUSED %s | %s" % (",".join(norm_units), bad_branch))
        return 1
    # pre-flight: a foreign path ALREADY in the tree is reported - with a likely cause - before the gate's
    # expensive build.  The same information the post-build refusal prints, delivered a build earlier; a
    # foreign path cannot disappear during the build, so an early refusal loses nothing and saves minutes.
    foreign_now = preflight_foreign(main)
    if foreign_now:
        clear_land_message(main)
        print(preflight_report(main, foreign_now), file=sys.stderr)
        print("REFUSED %s | %d path(s) outside the batch are already in the tree before the build; the "
              "post-build gate would refuse them too - clear the tree first"
              % (",".join(norm_units), len(foreign_now)))
        return 1
    gate_failures: list[str] = []
    with contextlib.redirect_stdout(sys.stderr):
        gate_ok = verify(main, norm_units, base, dry_run=False, no_build=no_build,
                         allow_regression=allow_regression, check_outbox=check_outbox,
                         release_claims=False, problems=gate_failures, branch=branch,
                         no_selftests=no_selftests) == 0
    rows = changed_status(main)
    outside = outside_batch([path for _code, path in rows])
    scratch = scratch_paths(outside)
    foreign = [p for p in outside if p not in scratch]
    if scratch:
        # the fix for the `d910.json` self-contradiction: the gate used to refuse this path and (via the
        # landing flow's `git add -A`) have staged it, so its own refusal was its own doing. Tolerated scratch
        # is named, de-indexed and left in the tree; the batch lands.
        print(tolerate_scratch(main, scratch), file=sys.stderr)
    if foreign:
        clear_land_message(main)
        print("REFUSED %s | paths outside the batch appeared during the build: %s"
              % (",".join(norm_units), ", ".join(foreign)))
        return 1
    base_dirty = base_dirty_paths(main)
    stageable = land_stageable(norm_units, rows, base_dirty)
    if gate_ok and looks_already_applied(rows, base_dirty):
        # the batch was applied to the tree *before* `record-base` ran, so the base's dirty snapshot recorded
        # its own edits as foreign and `land_stageable` excluded them (all of them, or just the shared files).
        # This is the 2026-09-26 case (b): the gate printed "READY: every check passed" and `land` then
        # refused with "no batch path is stageable", and the batch had to be committed by hand. Name the
        # ordering and the two ways out instead.
        if already_applied:
            print("NOTE: --already-applied: the batch was applied before `record-base`, so the base's dirty "
                  "snapshot recorded its own edits as foreign - staging the batch's paths anyway.",
                  file=sys.stderr)
            stageable = land_stageable(norm_units, rows, set())
        else:
            clear_land_message(main)
            print("REFUSED %s | the batch is already applied: every changed path was dirty when "
                  "`record-base` ran, so the gate excluded it as foreign work and has nothing to stage. "
                  "Revert the batch and re-record the base (`land.py record-base`) with a clean tree, or "
                  "land it as-is with --already-applied." % ",".join(norm_units))
            return 1
    action, why = land_decision(gate_ok, stageable, gate_failures, kinds_from_failures(gate_failures))
    if action != "commit":
        clear_land_message(main)
        print("REFUSED %s | %s" % (",".join(norm_units), why))
        return 1
    msg_file = land_message_path(main)
    if subject is not None:
        # the guard above means subject is a real one here, never the empty shell substitution
        write_land_message(main, message_body_with_subject(open(msg_file, encoding="utf-8").read(), subject))
    stage_batch(main, stageable)
    foreign = staged_elsewhere(main, stageable)
    if foreign:
        print(foreign_warning(foreign), file=sys.stderr)
    # A pathspec, never a bare `git commit`: that takes the whole index and is how another stream's staged
    # edit landed under the batch's message twice on 2026-09-23.
    p = commit_pathspec(main, msg_file, stageable)
    if p.returncode != 0:
        clear_land_message(main)
        tail = ((p.stderr or p.stdout) or "").strip().splitlines()
        print("REFUSED %s | git commit failed: %s" % (",".join(norm_units), tail[-1] if tail else ""))
        return 1
    sha = git(["rev-parse", "--short", "HEAD"], main).strip()
    teardown, incomplete = [], []
    if release_claims:
        # A unit renamed at registration keeps the pre-registration claim key while the gate compiled its
        # registered name; the branch is the claim's identity, so release the key the branch records.
        release_key = claim_unit_for_branch(main, branch) if branch else None
        for u in norm_units:
            out = claims.release(release_key or u, main, force=False, dry_run=False)
            if out.get("complete"):
                teardown.append(u)
            else:
                incomplete.append("%s (%s)" % (u, out.get("refused") or "a teardown step failed"))
    ledger = summary(read_base(main).get("ledger") or {}, ledger_numbers(main))
    tail = ""
    if teardown:
        tail += " | teardown %s" % ",".join(teardown)
    if incomplete:
        tail += " | TEARDOWN INCOMPLETE: %s" % "; ".join(incomplete)
    print("LANDED %s %s | %s%s" % (sha, ",".join(norm_units), ledger, tail))
    return 1 if incomplete else 0


def selftest() -> int:
    import tempfile
    fails, checks = [], 0

    def check(name, got, want):
        nonlocal checks
        checks += 1
        if got != want:
            fails.append("%s: got %r want %r" % (name, got, want))

    check("inside the batch: a unit source", outside_batch(["src/Pl/pl_act.cpp"]), [])
    check("inside the batch: splits.txt", outside_batch(["config/RMHE08/splits.txt"]), [])
    check("outside the batch: orig", outside_batch(["orig/RMHE08/sys/main.dol"]), ["orig/RMHE08/sys/main.dol"])
    check("outside the batch: build", outside_batch(["build/RMHE08/main.dol"]), ["build/RMHE08/main.dol"])
    check("outside the batch: ground truth", outside_batch(["config/RMHE08/build.sha1"]),
          ["config/RMHE08/build.sha1"])
    check("outside the batch: the campaign state files", outside_batch([".pi/claims.json"]), [".pi/claims.json"])

    # a batch that DELETES an extension-less file: after the apply the tree no longer has it, so only the
    # BASE's tree can say it was a path (`tools/git/hooks/post-commit`, 2026-09-28). Real temporary repo.
    with tempfile.TemporaryDirectory() as tmp:
        def _g(*a):
            subprocess.run(["git", "-c", "user.name=t", "-c", "user.email=t@t", *a], cwd=tmp, check=True,
                           stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        _g("init", "-q")
        os.makedirs(os.path.join(tmp, "tools", "git", "hooks"))
        open(os.path.join(tmp, "tools", "git", "hooks", "post-commit"), "w").write("#!/bin/sh\n")
        open(os.path.join(tmp, "tools", "git", "hooks", "pre-commit"), "w").write("#!/bin/sh\n")
        _g("add", "-A")
        _g("commit", "-q", "-m", "base")
        base = git(["rev-parse", "HEAD"], tmp).strip()
        os.remove(os.path.join(tmp, "tools", "git", "hooks", "post-commit"))
        hook = "tools/git/hooks/post-commit"
        check("a deleted extension-less file is gone from the tree", os.path.exists(os.path.join(tmp, hook)), False)
        check("... and without the base it is misread as a unit (the bug)", is_batch_path(tmp, hook), False)
        check("... with the base it is a batch path", is_batch_path(tmp, hook, base), True)
        check("unit_rows drops the deleted hook given the base",
              unit_rows(tmp, [hook, "Pl/pl_act"], base), ["Pl/pl_act"])
        check("a unit name that is not a path is still a unit at the base",
              is_batch_path(tmp, "menu/arena_result", base), False)
        check("an extension-less file that exists is still a path",
              is_batch_path(tmp, "tools/git/hooks/pre-commit", base), True)
        check("a directory at the base is a path", is_batch_path(tmp, "tools/git", base), True)
        check("an unresolvable base changes nothing", is_batch_path(tmp, hook, "0" * 40), False)

    # ALLOWED_FILES: the root documents a docs/tooling batch edits are allowed, an unknown root file is not
    check("inside the batch: README.md", outside_batch(["README.md"]), [])
    check("inside the batch: LICENSE", outside_batch(["LICENSE"]), [])
    check("inside the batch: .gitattributes and .flake8", outside_batch([".gitattributes", ".flake8"]), [])
    check("inside the batch: the CI example", outside_batch([".github.example/workflows/build.yml"]), [])
    check("outside the batch: an unknown root file", outside_batch(["notes.txt"]), ["notes.txt"])
    check("outside the batch: .gitmodules", outside_batch([".gitmodules"]), [".gitmodules"])

    # a `--units` entry that names a repo PATH is not a unit: it has no `Object(...)` line, no splits.txt
    # block and no `build/RMHE08/src/<unit>.o` target, so the unit-shaped rows must skip it. The extension
    # signal alone missed the extension-less ones - a batch naming `.gitignore` or `LICENSE` was refused by
    # the registration row with a message about a source file registered in name only (2026-09-27).
    with tempfile.TemporaryDirectory() as tmp:
        open(os.path.join(tmp, ".gitignore"), "w").write("build/\n")
        open(os.path.join(tmp, "LICENSE"), "w").write("CC0\n")
        open(os.path.join(tmp, "Makefile"), "w").write("all:\n")
        os.makedirs(os.path.join(tmp, "docs"))
        check("a hidden file is a batch path, not a unit", is_batch_path(tmp, ".gitignore"), True)
        check("an extension-less file at the root is a batch path", is_batch_path(tmp, "LICENSE"), True)
        check("a Makefile is a batch path", is_batch_path(tmp, "Makefile"), True)
        check("a directory the batch stages is a batch path", is_batch_path(tmp, "docs"), True)
        check("a tool script is a batch path", is_batch_path(tmp, "tools/units/land.py"), True)
        check("a unit is not a path - the bare name is not a file in the tree",
              is_batch_path(tmp, "menu/arena_result"), False)
        check("... with or without the `src/` prefix",
              is_batch_path(tmp, "src/menu/arena_result"), False)
        check("... and a deeper unit path is still a unit",
              is_batch_path(tmp, "Network/initNetworkSessionStable"), False)
        os.makedirs(os.path.join(tmp, "docs", "matching"))
        open(os.path.join(tmp, "docs", "matching", "043-demo.cpp"), "w").write("int f();")
        os.makedirs(os.path.join(tmp, "src", "Network"))
        open(os.path.join(tmp, "src", "Network", "unit_x.cpp"), "w").write("int g();")
        check("a demo .cpp outside src/ (its extension stripped by norm_unit) is a batch path",
              is_batch_path(tmp, "docs/matching/043-demo"), True)
        check("... a unit whose source is under src/ is still a unit", is_batch_path(tmp, "Network/unit_x"), False)
        check("... including the src/ spelling", is_batch_path(tmp, "src/Network/unit_x"), False)

        # a committed conflict marker: the build only reports it as a syntax error, so it is worth a row of
        # its own. `=======` alone is a banner, not evidence.
        open(os.path.join(tmp, "conflicted.c"), "w").write(
            "int a;\n<<<<<<< HEAD\nint b;\n=======\nint c;\n>>>>>>> other\n")
        open(os.path.join(tmp, "banner.c"), "w").write("/* ======= */\nint d;\n")
        open(os.path.join(tmp, "clean.c"), "w").write("int e;\n")
        check("a conflict marker is found, with its line and spelling",
              conflict_marker_files(tmp, ["conflicted.c"]),
              [("conflicted.c", 2, "<<<<<<<"), ("conflicted.c", 6, ">>>>>>>")])
        check("a banner of `=======` alone is not a marker", conflict_marker_files(tmp, ["banner.c"]), [])
        check("a clean file has none", conflict_marker_files(tmp, ["clean.c"]), [])
        check("a path that is not a file is skipped, not an error",
              conflict_marker_files(tmp, ["gone.c", "docs"]), [])
        check("... and the scan covers the batch's files together",
              len(conflict_marker_files(tmp, ["clean.c", "conflicted.c", "banner.c"])), 2)

    # the pre-flight names the likely cause of a foreign path whose name looks like lane scratch (the
    # `.tmp-mwcc/upstream` incident); an ordinary foreign file gets no invented cause
    check("preflight: `.tmp-*` names the mis-launch cause",
          "cwd set to MAIN" in (likely_cause(".tmp-mwcc/upstream/x.c") or ""), True)
    check("preflight: `.ws-*` also names lane scratch",
          "cwd set to MAIN" in (likely_cause(".ws-foo/f.txt") or ""), True)
    check("preflight: a bare `upstream` names a leftover clone",
          "cloned upstream" in (likely_cause("upstream/x.c") or ""), True)
    check("preflight: an ordinary foreign file gets no invented cause", likely_cause("NOTES.md"), None)

    # `land` stages the batch's own files only: a tracked change is the batch, but an untracked file that is
    # not a named unit's source is another stream's in-flight work (the round's `tools/units/playbook.py`)
    rows = [(" M", "src/Pl/pl_act.cpp"), ("??", "src/Pl/pl_act.cpp"), ("??", "tools/units/playbook.py"),
            ("??", "tools/units/land.py"), ("??", "include/Foo.h"), ("??", "src/Other/other.cpp"),
            (" M", "configure.py"), ("??", "build/RMHE08/main.dol")]
    check("a tracked batch file is staged", "src/Pl/pl_act.cpp" in land_stageable(["Pl/pl_act"], rows), True)
    check("an untracked unit source is staged", "src/Pl/pl_act.cpp" in land_stageable(["Pl/pl_act"], rows), True)
    check("another worker's untracked tool is not",
          "tools/units/playbook.py" in land_stageable(["Pl/pl_act"], rows), False)
    check("an untracked source or header is staged",
          ("include/Foo.h" in land_stageable(["Pl/pl_act"], rows)
           and "src/Other/other.cpp" in land_stageable(["Pl/pl_act"], rows)), True)
    check("a unit named by its own path is staged",
          "tools/units/land.py" in land_stageable(["tools/units/land.py"], rows), True)
    check("a shared-file edit is staged", "configure.py" in land_stageable(["Pl/pl_act"], rows), True)
    check("build output is never staged", "build/RMHE08/main.dol" in land_stageable(["Pl/pl_act"], rows), False)
    # the incident: a *tracked* `tools/` change is another stream's work unless the batch names it
    check("another worker's tracked tool edit is not staged",
          "tools/units/langcheck.py" in land_stageable(["Pl/pl_act"], [(" M", "tools/units/langcheck.py")]), False)
    check("a tool the batch names is still staged",
          "tools/units/land.py" in land_stageable(["tools/units/land.py"], [(" M", "tools/units/land.py")]), True)

    # the 2026-09-24 hazard: a path that was already dirty when the batch base was recorded is another
    # stream's work, not batch material, even though it sits inside the allowed set - a prepared `docs/plan.md`
    # rode `85ddd7b6` and a `src/RSO/runtime.c` header rode `890631e8`. The snapshot comes from `record_base`.
    foreign_rows = [(" M", "docs/plan.md"), (" M", "src/RSO/runtime.c"), (" M", "src/Pl/pl_act.cpp"),
                    (" M", "configure.py")]
    dirty_at_base = {"docs/plan.md", "src/RSO/runtime.c"}
    check("a path dirty at the batch base is not staged",
          "docs/plan.md" in land_stageable(["Pl/pl_act"], foreign_rows, dirty_at_base), False)
    check("... and neither is a foreign unit header",
          "src/RSO/runtime.c" in land_stageable(["Pl/pl_act"], foreign_rows, dirty_at_base), False)
    check("a path the batch names is staged even if dirty at the base",
          "src/Pl/pl_act.cpp" in land_stageable(["Pl/pl_act"], foreign_rows, {"src/Pl/pl_act.cpp"}), True)
    check("a path clean at the base is still batch material",
          "configure.py" in land_stageable(["Pl/pl_act"], foreign_rows, dirty_at_base), True)
    check("without the base snapshot the old sweep is reproduced (the failure mode)",
          "docs/plan.md" in land_stageable(["Pl/pl_act"], foreign_rows), True)

    # the one-command path: the gate's verdict is the decision, so a red gate can never reach `git commit`
    check("a failed gate refuses the commit", land_decision(False, ["src/Pl/pl_act.cpp"]),
          ("refuse", "the gate failed - nothing staged or committed"))
    check("a green gate with nothing to stage refuses", land_decision(True, []),
          ("refuse", "the gate passed but no batch path is stageable - nothing to commit"))
    check("a green gate with a batch commits", land_decision(True, ["src/Pl/pl_act.cpp"]),
          ("commit", ""))

    # the 2026-09-24 incident: a `--message "$(cat /tmp/msg1.txt)"` whose file lived at a different `/tmp`
    # expanded to the empty string, `if subject:` dropped the override, and the batch landed under the gate's
    # fallback subject instead of the one the worker wrote (`71c244f9`). An empty or blank argument is refused
    # before the gate runs; omitting --message still asks for the gate's default subject; a real one lands.
    check("no --message uses the gate's default subject", message_error(None), None)
    empty_err = message_error("")
    check("an empty --message is refused", empty_err is not None, True)
    check("... and the refusal names --message", "--message" in (empty_err or ""), True)
    check("... and names it empty", "is empty" in (empty_err or ""), True)
    blank_err = message_error("  \t\n ")
    check("a whitespace-only --message is refused", blank_err is not None, True)
    check("... and the refusal names --message", "--message" in (blank_err or ""), True)
    check("... and names it whitespace-only", "whitespace-only" in (blank_err or ""), True)
    check("a real --message is accepted", message_error("ef: land fn_800FAE08 (41/41 symbols)"), None)
    check("a real --message replaces the gate subject and keeps its body",
          message_body_with_subject("land: Pl/pl_act\n\nledger: closed 1 -> 2\n",
                                    "ef: land fn_800FAE08 (41/41 symbols)"),
          "ef: land fn_800FAE08 (41/41 symbols)\n\nledger: closed 1 -> 2\n")

    # 4d. the gate's own subject row (CLAUDE.md's commit convention). `land_subject` composes the subject
    # every landing is written under, and the row runs `commitlint.py` over it - the tool, never a second copy
    # of the rules. The demonstration uses the REAL tool against a fixture tree (`--root` pins the member set
    # to the fixture, not this checkout), so its verdict is the convention's, not a stub's restatement.
    check("a unit under src/ composes a game/<module> subject",
          land_subject(["Network/network_transport"]), "game/network: land Network/network_transport")
    check("a tool path composes the grouping category",
          land_subject(["tools/units/land.py"]), "tools/units: land tools/units/land.py")
    check("no units still yields a conventional subject",
          land_subject([]), "repo/batch: land a batch")

    with tempfile.TemporaryDirectory() as lint_root:
        for rel in ("tools/units/land.py", "tools/units/stylelint.py", "tools/git/commitlint.py"):
            path = os.path.join(lint_root, *rel.split("/"))
            os.makedirs(os.path.dirname(path), exist_ok=True)
            open(path, "w", encoding="utf-8").close()
        cl_tool = os.path.normpath(os.path.join(HERE, "..", "git", "commitlint.py"))
        check("the real commitlint.py exists for the row to call", os.path.exists(cl_tool), True)

        # a good composed subject passes: `tools/land` is a member now (the script is `tools/units/land.py`)
        good_ok, good_detail = subject_lint(lint_root, "tools/land: land tools/units/land.py", tool=cl_tool)
        check("the row PASSES a good composed subject", good_ok, True)
        check("... and a pass carries no detail", good_detail, "")

        # ... and a bad one is refused. `land_subject` reads the first segment after `tools/`, so a unit
        # under an invented grouping composes a category no member matches.
        bad_subject = land_subject(["tools/land2/tool.py"])
        check("a unit under an invented grouping composes a bad subject",
              bad_subject, "tools/land2: land tools/land2/tool.py")
        bad_ok, bad_detail = subject_lint(lint_root, bad_subject, tool=cl_tool)
        check("the row REFUSES a bad composed subject", bad_ok, False)
        check("... and the refusal carries commitlint's own finding, not a restatement",
              "not a known `tools` member" in bad_detail, True)

        # exit 2 is "nothing checked" - the row treats it as a failure, never a pass
        class _NothingChecked:
            returncode, stdout, stderr = 2, "commitlint: nothing was checked", ""
        stub_ok, stub_detail = subject_lint(lint_root, "tools/land: x", tool=cl_tool,
                                            runner=lambda args: _NothingChecked())
        check("the row treats exit 2 (nothing checked) as a failure", stub_ok, False)
        check("... and says the lint did not run", "nothing was checked" in stub_detail, True)

    # the warning that keeps a foreign staged edit visible: `land` leaves it alone and names it
    check("the foreign-index warning names the path",
          foreign_warning(["tools/units/langcheck.py"]),
          "WARNING: the index holds 1 path outside this batch - left staged, not committed: "
          "tools/units/langcheck.py")
    check("... and pluralises two", foreign_warning(["a", "b"]).count("paths"), 1)

    # the gate's "all tool selftests pass" row: `tools/selftest.py --json` is parsed so the refusal NAMES the
    # failing tool - `command_detail`'s head would show the table header, not the reason.
    def _stp(code, stdout):
        return subprocess.CompletedProcess(["selftest"], code, stdout, "")

    check("a green selftest row is silent", selftest_detail(_stp(0, "")), "")
    _red = json.dumps({"failures": [{"name": "tools/flags/infer"}], "stale_parks": [],
                       "tree_clean": True})
    check("a red selftest row names the failing tool",
          "tools/flags/infer" in selftest_detail(_stp(1, _red)), True)
    _stale = json.dumps({"failures": [], "stale_parks": ["tools/units/wtsafe"], "tree_clean": True})
    check("... and a stale park", "stale park: tools/units/wtsafe" in selftest_detail(_stp(1, _stale)), True)
    _dirty = json.dumps({"failures": [], "stale_parks": [], "tree_clean": False,
                         "tree_offenders": ["?? tools/x"]})
    check("... and a selftest that changed the tree",
          "changed the tree" in selftest_detail(_stp(1, _dirty)), True)
    check("unparseable output falls back to the head",
          "boom" in selftest_detail(_stp(1, "boom")), True)

    # the 2026-09-25 `d910.json` defect. An objdiff `diff` dump from the repo root is outside the guard's
    # allowed set, so `outside_batch` still classifies it - the tolerance is a deliberate carve-out *at the
    # tolerance site*, never a loosening of the guard - and the carve-out is narrow: the `d`/`t` + digits +
    # `.json` shape in the repo root only. A `d`-shaped name elsewhere, or any other path, is still refused.
    check("scratch: an objdiff dump is scratch", is_scratch("d910.json"), True)
    check("scratch: the target-side dump too", is_scratch("t910.json"), True)
    check("scratch: a single digit", is_scratch("d0.json"), True)
    check("scratch: uppercase is not the measured shape", is_scratch("D910.json"), False)
    check("scratch: not a suffix of another name", is_scratch("d910.json.bak"), False)
    check("scratch: not a letter-only name", is_scratch("dx.json"), False)
    check("scratch: no letter", is_scratch("910.json"), False)
    check("scratch: no dot", is_scratch("d.json"), False)
    check("scratch: not under a subdirectory", is_scratch("src/d910.json"), False)
    check("scratch: a source file is not scratch", is_scratch("src/Pl/pl_act.cpp"), False)
    check("scratch: the shared file is not scratch", is_scratch("configure.py"), False)
    check("scratch: the guard still classifies it outside the batch",
          outside_batch(["d910.json"]), ["d910.json"])
    check("scratch: it is named in the note", "d910.json" in scratch_note(["d910.json"]), True)
    check("scratch: the note says it is not a refusal",
          "never a refusal" in scratch_note(["d910.json"]), True)
    check("scratch: the note pluralises", scratch_note(["d910.json", "t910.json"]).count("paths"), 1)
    check("scratch: the subset keeps order",
          scratch_paths(["d910.json", "src/a.c", "t12.json"]), ["d910.json", "t12.json"])
    # the crossing point itself: the gate must never stage a path it did not receive from the batch
    check("scratch: a scratch path is never stageable",
          "d910.json" in land_stageable(["Pl/pl_act"], [("??", "d910.json")]), False)
    check("scratch: the batch's own file in the same rows still is",
          land_stageable(["Pl/pl_act"], [("??", "d910.json"), (" M", "src/Pl/pl_act.cpp")]),
          ["src/Pl/pl_act.cpp"])
    check("scratch: a staged scratch path is not stageable either",
          "d910.json" in land_stageable(["Pl/pl_act"], [("A ", "d910.json")]), False)

    # the compile gate (2026-09-26): `ninja build/RMHE08/ok` is structurally blind to a `NonMatching` unit's
    # object (it is never linked), so a unit that does not compile reached `main` twice in one session with
    # `ok` green. The check is scoped by object target: only a `FAILED:` output that is one of the batch's own
    # `build/RMHE08/src/<unit>.o` targets refuses. A foreign dirty object failing passes and is named.
    check("compile: a unit's object target is build/RMHE08/src/<unit>.o",
          compile_targets(["Pl/fn_80273B14"]), ["build/RMHE08/src/Pl/fn_80273B14.o"])
    check("compile: a unit spelled with its extension still yields one target",
          compile_targets(["Pl/fn_80273B14.cpp"]), ["build/RMHE08/src/Pl/fn_80273B14.o"])
    check("compile: duplicates collapse", compile_targets(["Pl/pl_act", "Pl/pl_act.cpp"]),
          ["build/RMHE08/src/Pl/pl_act.o"])
    check("compile: no units yields no targets", compile_targets([]), [])
    check("compile: FAILED outputs are read from ninja's stderr", failed_compile_outputs(
        "FAILED: build/RMHE08/src/Pl/pl_act.o\nrun 1\nFAILED: build/RMHE08/src/RSO/runtime.o\nrun 2"),
        ["build/RMHE08/src/Pl/pl_act.o", "build/RMHE08/src/RSO/runtime.o"])
    check("compile: a target failed twice is named once", failed_compile_outputs(
        "FAILED: build/RMHE08/src/Pl/pl_act.o\nFAILED: build/RMHE08/src/Pl/pl_act.o"),
        ["build/RMHE08/src/Pl/pl_act.o"])
    check("compile: a Windows path separator is normalised", failed_compile_outputs(
        "FAILED: build\\RMHE08\\src\\Pl\\pl_act.o"), ["build/RMHE08/src/Pl/pl_act.o"])
    check("compile: a batch unit's own failure is the batch's", batch_compile_failures(
        ["Pl/pl_act"], "FAILED: build/RMHE08/src/Pl/pl_act.o"), (["Pl/pl_act"], []))
    check("compile: a foreign dirty object's failure is NOT the batch's", batch_compile_failures(
        ["Pl/pl_act"], "FAILED: build/RMHE08/src/RSO/runtime.o"),
        ([], ["build/RMHE08/src/RSO/runtime.o"]))
    check("compile: a mixed run names both, refuses on the batch's unit", batch_compile_failures(
        ["Pl/pl_act"], "FAILED: build/RMHE08/src/RSO/runtime.o\nFAILED: build/RMHE08/src/Pl/pl_act.o"),
        (["Pl/pl_act"], ["build/RMHE08/src/RSO/runtime.o"]))
    check("compile: a target that only shares a prefix is foreign", batch_compile_failures(
        ["Pl/pl_act"], "FAILED: build/RMHE08/src/Pl/pl_act2.o"),
        ([], ["build/RMHE08/src/Pl/pl_act2.o"]))

    class FakeProc:
        """A `subprocess.CompletedProcess`-shaped stand-in for the selftest's fake ninja."""

        def __init__(self, code, out="", err=""):
            self.returncode, self.stdout, self.stderr = code, out, err

    with tempfile.TemporaryDirectory() as tmp:
        open(os.path.join(tmp, "build.ninja"), "w").close()
        calls = []

        def fake_runner(args):
            calls.append(args)
            return FakeProc(1, "", "FAILED: build/RMHE08/src/Pl/pl_act.o\n(10248) does not match")

        ok, detail = compile_check(tmp, ["Pl/pl_act"], runner=fake_runner)
        check("compile gate: a batch unit that does not compile is REFUSED", ok, False)
        check("... and the batch unit is named", "Pl/pl_act" in detail, True)
        check("... and it ran one `ninja -k 0`", calls, [["ninja", "-k", "0"]])

        ok, detail = compile_check(
            tmp, ["Pl/pl_act"],
            runner=lambda args: FakeProc(1, "FAILED: build/RMHE08/src/RSO/runtime.o\n"))
        check("compile gate: it PASSES when only FOREIGN dirty work is broken", ok, True)
        check("... and the foreign failure is named, not counted", "foreign" in detail, True)
        check("... and it never reads as the batch's", "Pl/pl_act" in detail, False)

        ok, detail = compile_check(tmp, ["Pl/pl_act"], runner=lambda args: FakeProc(0))
        check("compile gate: a clean compile passes", ok, True)

        ok, detail = compile_check(
            tmp, ["Pl/pl_act"], runner=lambda args: FakeProc(2, "ninja: error: unknown target\n"))
        check("compile gate: an unattributable ninja failure is not read as a pass", ok, False)
        check("... and the reason is in the detail", "unknown target" in detail, True)

    with tempfile.TemporaryDirectory() as no_build:
        ok, detail = compile_check(no_build, ["Pl/pl_act"], runner=lambda args: FakeProc(1, "x"))
        check("compile gate: without build.ninja the configure.py gate owns it", ok, True)

    # the registration gate (2026-09-26): a source file committed without its `configure.py` line and
    # `splits.txt` block is registered in name only and absent from the build; `ok` stays green because a
    # `NonMatching` object is never linked, and the compile gate cannot see it because there is no
    # `build/RMHE08/src/<unit>.o` target to scope to. `vu.registration_problems` owns the three axes.
    check("registration: a unit in all three places passes",
          vu.registration_problems(
              ["Pl/pl_act"],
              'Object(NonMatching, "Pl/pl_act.cpp")\n',
              "Sections:\nPl/pl_act.cpp:\n\t\t.text start:0x1 end:0x2\n",
              "build build\\RMHE08\\src\\Pl\\pl_act.o: mwcc_sjis\n"),
          [])
    check("registration: a source-only unit REFUSES",
          vu.registration_problems(["Pl/pl_act"], "", "Sections:\n", "build a.o: rule\n") != [],
          True)
    check("registration: it names all three axes",
          len(vu.registration_problems(["Pl/pl_act"], "", "Sections:\n", "build a.o: rule\n")), 3)
    check("registration: an Object line with no splits block still refuses",
          vu.registration_problems(["Pl/pl_act"],
                                   'Object(NonMatching, "Pl/pl_act.cpp")\n',
                                   "Sections:\n",
                                   "build build\\RMHE08\\src\\Pl\\pl_act.o: mwcc_sjis\n") != [],
          True)
    check("registration: a unit missing from a stale build.ninja still refuses",
          vu.registration_problems(["Pl/pl_act"],
                                   'Object(NonMatching, "Pl/pl_act.cpp")\n',
                                   "Sections:\nPl/pl_act.cpp:\n\t\t.text start:0x1 end:0x2\n",
                                   "") != [],
          True)

    # the split-target drift (the merger's strongest form): target objects come from the DOL split, so a
    # unit the batch does not name whose object moved is a `splits.txt` change that re-ranged a neighbour.
    check("drift: an unchanged tree has no drift",
          vu.target_drift_problems({"a": "1", "b": "2"}, {"a": "1", "b": "2"}, []), [])
    check("drift: the batch's own unit may move",
          vu.target_drift_problems({"a": "1"}, {"a": "2"}, ["a"]), [])
    check("drift: a re-ranged neighbour REFUSES",
          any("re-ranged" in p
              for p in vu.target_drift_problems({"a": "1", "n": "2"}, {"a": "1", "n": "3"}, ["a"])),
          True)

    # the independent per-symbol re-measure: a `fuzzy_match_percent`-absent function is 0%, not 100%, and
    # the unit arithmetic that proves the reading must refuse when the two disagree (SKILL 5.2/5.3).
    check("re-measure: an absent fuzzy key is 0%, so the arithmetic reproduces",
          vu.arithmetic_crosscheck(
              {"total_code": 200, "fuzzy_match_percent": 50.0},
              {"a": {"size": "100", "fuzzy_match_percent": 100.0}, "b": {"size": "100"}})[0],
          True)
    check("re-measure: a unit fuzzy that only reproduces if absent=100 REFUSES",
          vu.arithmetic_crosscheck(
              {"total_code": 200, "fuzzy_match_percent": 100.0},
              {"a": {"size": "100", "fuzzy_match_percent": 100.0}, "b": {"size": "100"}})[0],
          False)
    check("re-measure: a 100% claim whose bytes differ REFUSES",
          any("not identical" in p for p in vu.symbol_problems(
              {"a": {"fuzzy_match_percent": 100.0}}, {"a": {"fuzzy_match_percent": 100.0}},
              {"a": {"target_size": 16, "candidate_size": 16, "in_target": True,
                     "in_candidate": True, "identical": False}})[0]),
          True)
    check("re-measure: a report not reproducible from a fresh generate REFUSES",
          any("not reproducible" in p for p in vu.symbol_problems(
              {"a": {"fuzzy_match_percent": 100.0}}, {"a": {"fuzzy_match_percent": 50.0}},
              {"a": {"target_size": 16, "candidate_size": 16, "in_target": True,
                     "in_candidate": True, "identical": True}})[0]),
          True)

    with tempfile.TemporaryDirectory() as tmp:
        os.makedirs(os.path.join(tmp, ".git"), exist_ok=True)
        stale = write_land_message(tmp, "land: old batch\n")
        check("a green gate writes the message", os.path.exists(stale), True)
        check("a failed gate removes it", clear_land_message(tmp), stale)
        check("... and it is gone", os.path.exists(stale), False)
        check("clearing a missing message is a no-op", clear_land_message(tmp), None)

    with tempfile.TemporaryDirectory() as tmp:
        # regression_rows() reads a report_changes.json fixture
        fixture = os.path.join(tmp, "changes.json")
        json.dump({"units": [
            {"name": "src/Pl/pl_act.cpp", "measures": {"matched_code": {"old": 100, "new": 90}}},
            {"name": "src/Pl/pl_skill.cpp", "measures": {"matched_code": {"old": 90, "new": 90}}},
            {"name": "main/auto_eft004_set_pl__FP4_PLWUcfffUl_text", "measures": {"matched_code": {"old": 10, "new": 1}}},
        ]}, open(fixture, "w"))
        rows = regression_rows(fixture)
        check("a regression is caught", len(rows), 1)
        check("the auto_ scaffold is ignored", any("auto_" in r[0] for r in rows), False)
        check("a registered auto/* unit is still tracked",
              regression_rows(fixture) != [] and all("main/auto/" not in r[0] for r in rows), True)
        check("a flat measure is not a regression", any(r[1] == "matched_code" and r[2] == 90 for r in rows), False)
        check("a missing changes file is not a crash", regression_rows(os.path.join(tmp, "nope.json")), [])

    # the per-SYMBOL regression rule (2026-09-25). A unit's `fuzzy` is an average over its symbols, so an
    # already-registered unit that is EXTENDED - a widened splits range, a head joined to its tail - falls in
    # average as weaker new bodies join it. The old gate called that a regression and refused two legitimate
    # extensions in one day (g3d_resshp: `no symbol or unit regressed: ... unit fuzzy 99.96 -> 99.26`). The
    # rule is now symbol by symbol: only a symbol the previous report HELD can regress, and a symbol it did
    # not hold is NEW and never refuses a batch.
    def old_unit_avg_regression(before, after):
        """The pre-2026-09-25 rule, kept as the oracle the fix must beat: any unit average or symbol drop."""
        rows = []
        for unit, av in after.items():
            pv = before.get(unit)
            if not pv:
                continue
            af, bf = pv.get("fuzzy"), av.get("fuzzy")
            if isinstance(af, (int, float)) and isinstance(bf, (int, float)) and bf < af - 1e-9:
                rows.append((unit, "unit fuzzy", af, bf))
            for sym, bp in (pv.get("symbols") or {}).items():
                ap = (av.get("symbols") or {}).get(sym)
                if isinstance(ap, (int, float)) and ap < bp - 1e-9:
                    rows.append((unit, sym, bp, ap))
        return rows

    # the shape the gate refused: the 21-symbol head at 99.96 % widened to the 58-function TU at 99.26 %,
    # every head symbol re-measured unchanged and 37 weaker bodies added (fn_8009A1E0 among them at 70.28).
    head = {"main/g3d/g3d_resshp": {"fuzzy": 99.95968, "matched_code": 964,
                                    "symbols": {"fn_80099724": 98.5714}}}
    extended = {"main/g3d/g3d_resshp": {"fuzzy": 99.26, "matched_code": 2108,
                                        "symbols": {"fn_80099724": 98.5714, "fn_8009A1E0": 70.28,
                                                    "fn_8009A244": 55.0, "fn_8009A2D0": 31.5}}}
    check("a unit average that fell only because it grew is not a regression",
          report_regressions(head, extended, []), ([], []))
    check("... but the old unit-average rule did fire on it",
          old_unit_avg_regression(head, extended) != [], True)
    check("... and unit_grew reads the new symbols as growth",
          unit_grew(head["main/g3d/g3d_resshp"], extended["main/g3d/g3d_resshp"]), True)

    # an existing symbol's own score dropping must still refuse, naming the symbol and both numbers
    regressed = {"main/g3d/g3d_resshp": {"fuzzy": 98.5, "matched_code": 2108,
                                         "symbols": {"fn_80099724": 91.25, "fn_8009A1E0": 70.28}}}
    check("an existing symbol that dropped is a regression",
          report_regressions(extended, regressed, []),
          ([("main/g3d/g3d_resshp", "fn_80099724", 98.5714, 91.25)], []))
    check("... and the new symbol beside it is never named",
          report_regressions(extended, regressed, [])[0][0][1], "fn_80099724")
    # growth plus a real drop is still a refusal: the symbol row wins, the average row does not double it
    grew_and_dropped = {"main/g3d/g3d_resshp": {"fuzzy": 95.0, "matched_code": 2400,
                                                "symbols": {"fn_80099724": 90.0, "fn_NEW": 20.0}}}
    check("an extension that ALSO regressed a held symbol still refuses, once, on the symbol",
          report_regressions(extended, grew_and_dropped, []),
          ([("main/g3d/g3d_resshp", "fn_80099724", 98.5714, 90.0)], []))

    # a unit that did NOT grow and whose average fell (matched bytes lost, no held symbol dropped) still
    # refuses at the unit level - the one place a per-symbol row cannot name the loss
    shrunk = {"main/g3d/g3d_resshp": {"fuzzy": 90.0, "matched_code": 900,
                                      "symbols": {"fn_80099724": 98.5714}}}
    check("a unit that shrank and lost its average still refuses",
          report_regressions(head, shrunk, []),
          ([("main/g3d/g3d_resshp", "unit fuzzy", 99.95968, 90.0)], []))
    check("... and unit_grew is false for it",
          unit_grew(head["main/g3d/g3d_resshp"], shrunk["main/g3d/g3d_resshp"]), False)

    # the happy paths: unchanged measurements, and a unit the previous report never held, are not regressions
    check("an unchanged unit is not a regression", report_regressions(head, head, []), ([], []))
    check("a brand-new unit is not a regression", report_regressions({}, extended, []), ([], []))
    check("an empty snapshot on both sides is not a regression", report_regressions({}, {}, []), ([], []))

    # --allow-regression still authorises exactly the unit it names, and nothing else
    check("an authorised drop leaves the unauthorised list empty",
          report_regressions(extended, regressed, ["g3d_resshp"])[0], [])
    check("... and is reported as authorised",
          report_regressions(extended, regressed, ["g3d_resshp"])[1],
          [("main/g3d/g3d_resshp", "fn_80099724", 98.5714, 91.25)])
    check("an allowance that does not name the unit does not authorise",
          len(report_regressions(extended, regressed, ["SomeOther/file.cpp"])[0]), 1)
    check("... and nothing is authorised by it",
          report_regressions(extended, regressed, ["SomeOther/file.cpp"])[1], [])

    # branch_commits() counts `main..branch`, not `<batch base>..branch`: a worker's branch is cut when the
    # unit is claimed, so in a multi-batch round it predates the base the orchestrator later records. Real
    # temp repos, because the check is entirely about git reachability. The unit is spelled extensionless and
    # then called with the extension: the branch must be found under either spelling (the 2026-09-23 fix).
    unit = "Pl/pl_act"
    branch = claims.branch_for(unit)

    def repo_git(path, *args):
        p = subprocess.run(["git", "-c", "user.email=selftest@example.invalid", "-c", "user.name=selftest",
                            "-c", "commit.gpgsign=false", *args], cwd=path, capture_output=True, text=True, encoding="utf-8", errors="replace")
        if p.returncode != 0:
            raise RuntimeError("git %s: %s" % (" ".join(args), p.stderr.strip()))
        return p.stdout.strip()

    def repo_commit(path, msg):
        with open(os.path.join(path, "f.txt"), "a", encoding="utf-8") as fh:
            fh.write(msg + "\n")
        repo_git(path, "add", "-A")
        repo_git(path, "commit", "-q", "-m", msg)

    # `land` commits with a pathspec. `git commit` with none takes the whole index, which is how another
    # stream's staged `tools/units/langcheck.py` landed under two unrelated unit commits on 2026-09-23
    # (`85f3d4b5`, `d50fdd32`). The proof is a real repo, run through the real `land_stageable` and the real
    # commit helper: the batch's paths are committed, the foreign edit is not, and it is still staged
    # afterwards. A rename's deletion is part of the batch, so both of its paths go in the pathspec.
    with tempfile.TemporaryDirectory() as tmp:
        repo_git(tmp, "init", "-q")
        repo_git(tmp, "checkout", "-q", "-b", "main")
        for name in ("src/batch.c", "src/old.c", "tools/units/langcheck.py"):
            os.makedirs(os.path.dirname(os.path.join(tmp, name)), exist_ok=True)
            with open(os.path.join(tmp, name), "w", encoding="utf-8") as fh:
                fh.write("base\n")
        repo_git(tmp, "add", "-A")
        repo_git(tmp, "commit", "-q", "-m", "base")
        with open(os.path.join(tmp, "src/batch.c"), "w", encoding="utf-8") as fh:
            fh.write("the batch\n")
        repo_git(tmp, "mv", "src/old.c", "src/new.c")   # a rename: its deletion belongs to the batch too
        with open(os.path.join(tmp, "tools/units/langcheck.py"), "w", encoding="utf-8") as fh:
            fh.write("another stream\n")
        repo_git(tmp, "add", "tools/units/langcheck.py")  # foreign, staged by another worker
        rows = changed_status(tmp)
        check("a rename reports both of its paths",
              ("src/old.c" in [p for _c, p in rows] and "src/new.c" in [p for _c, p in rows]), True)
        stageable = land_stageable(["Pl/pl_act"], rows)
        check("the tracked tool edit is not part of the batch", "tools/units/langcheck.py" in stageable, False)
        check("the foreign staged edit is named", staged_elsewhere(tmp, stageable), ["tools/units/langcheck.py"])
        stage_batch(tmp, stageable)
        msg_file = os.path.join(tmp, "msg.txt")
        with open(msg_file, "w", encoding="utf-8") as fh:
            fh.write("land: the batch\n")
        p = commit_pathspec(tmp, msg_file, stageable)
        check("the pathspec commit succeeds", p.returncode, 0)
        check("the batch's file is committed", repo_git(tmp, "show", "HEAD:src/batch.c"), "the batch")
        check("the rename's new path is committed",
              run(["git", "cat-file", "-e", "HEAD:src/new.c"], tmp).returncode, 0)
        check("the rename's deletion is committed",
              run(["git", "cat-file", "-e", "HEAD:src/old.c"], tmp).returncode != 0, True)
        check("the foreign edit is NOT committed",
              repo_git(tmp, "show", "HEAD:tools/units/langcheck.py"), "base")
        check("... and is still staged", repo_git(tmp, "diff", "--cached", "--name-only"),
              "tools/units/langcheck.py")
        check("... and still uncommitted", repo_git(tmp, "status", "--porcelain").startswith("M"), True)

    # the `d910.json` end-to-end: the real `land` on a real repo, with a `verify` stand-in that writes files
    # DURING the "build" (the only way to reproduce "appeared during the build"). Four cases: tool scratch
    # lands the batch without being staged, a scratch file the index already holds is de-indexed and still not
    # committed, a path outside the batch's expected set is still refused loudly (even with scratch beside it),
    # and a gate that fails names itself in the refusal.
    import io
    import unittest.mock as mock
    module = sys.modules[__name__]
    # the rule-7 row still judges GROWTH of the `rule 7 deferred` escape (the escape itself is inert to the
    # lint since the 2026-09-27 no-exemption ruling): an existing escape is tolerated, a grown one is refused
    # for a name the batch's own unit defines, and for a file registered at a generated stem. The cases are
    # the ones the 803253bc and 8032c920 lanes produced.
    def defer_repo(rel, base_text, text):
        tmp = tempfile.mkdtemp(prefix="land-defer-")
        repo_git(tmp, "init", "-q")
        repo_git(tmp, "checkout", "-q", "-b", "main")
        path = os.path.join(tmp, rel)
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, "w", encoding="utf-8") as fh:
            fh.write(base_text)
        repo_git(tmp, "add", "-A")
        repo_git(tmp, "commit", "-q", "-m", "base")
        base_sha = repo_git(tmp, "rev-parse", "HEAD")
        with open(path, "w", encoding="utf-8") as fh:
            fh.write(text)
        return tmp, base_sha

    defer = "/* rule 7 deferred: the map has only fn_XXXXXXXX for this range */\n"
    d1, sha1 = defer_repo("src/enemy/em_action.cpp", "void em_act_dispatch(void) {}\n",
                          defer + "void fn_802B2978(void) {}\n")
    check("rule7: a file that DEFINES its own generated name REFUSES",
          module.rule7_defer_growth(d1, sha1) != [], True)
    d2, sha2 = defer_repo("src/enemy/em_action.cpp", "void em_act_dispatch(void) {}\n",
                          defer + "void fn_802B2978(void);\nvoid em_act_dispatch(void) {}\n")
    check("rule7: a prototype of another unit's generated name is tolerated",
          module.rule7_defer_growth(d2, sha2), [])
    d3, sha3 = defer_repo("src/enemy/fn_8033041C.cpp", "",
                          defer + "void em_act_dispatch(void) {}\n")
    check("rule7: a unit registered at a generated file name REFUSES",
          module.rule7_defer_growth(d3, sha3) != [], True)
    d4, sha4 = defer_repo("src/enemy/em_action.cpp", defer + "void em_act_dispatch(void) {}\n",
                          defer + "/* rewritten header */\nvoid em_act_dispatch(void) {}\n")
    check("rule7: rewriting a file that already had the escape passes",
          module.rule7_defer_growth(d4, sha4), [])
    d5, sha5 = defer_repo("src/enemy/fn_8033041C.cpp", "",
                          defer + "void em_act_dispatch(void);\n/* no bodies yet: the name is provisional */\n")
    check("rule7: a BODYLESS unit at a generated file name is tolerated",
          module.rule7_defer_growth(d5, sha5), [])
    for d in (d1, d2, d3, d4, d5):
        shutil.rmtree(d, ignore_errors=True)

    # --- rule 10: the row is ADD-only, like the lint's `--diff` ------------------------------------
    # The keys are `vtableaudit.violation_rows`'s rename-stable shape: a run by its range (an address is
    # unique in the DOL and a re-home keeps it), a `ref:` by the path the tree now spells.
    existing = {"run:.data:805D4D38": {"unit": "ai/fn_802CC794",
                                       "where": "ai/fn_802CC794.cpp .data"},
                "ref:src/old/unit.cpp:9:OldVTable": {"unit": "old/unit",
                                                      "where": "src/old/unit.cpp:9 assigns OldVTable"}}
    same = dict(existing, **{"ref:src/new/unit.cpp:3:NewVTable":
                             {"unit": "new/unit", "where": "src/new/unit.cpp:3 assigns NewVTable"}})
    check("rule10: an unchanged set (existing violations grandfathered) adds nothing",
          module.rule10_growth(existing, existing, ["new/unit"]), ([], []))
    added, touched = module.rule10_growth(existing, same, ["new/unit"])
    check("rule10: a batch that ADDS a violation is refused",
          added, ["ref:src/new/unit.cpp:3:NewVTable"])
    check("... and the row prints the batch units' violations even when it passes",
          touched, ["src/new/unit.cpp:3 assigns NewVTable"])
    check("rule10: a violation that disappears is not an addition",
          module.rule10_growth(same, existing, ["new/unit"]), ([], []))
    check("rule10: a batch touching a file that already has one passes",
          module.rule10_growth(existing, existing, ["ai/fn_802CC794"]),
          ([], ["ai/fn_802CC794.cpp .data"]))

    # --- rule 12: the style-lint row's allowance, applied to stylelint's own --diff JSON -----------
    # Rule 12 is refused inside the style lint, so `--allow-rule12 <token>` is applied to that row's
    # `added`/`detail` delta. The token is the at-fault symbol name `--list-added` prints.
    added12 = [{"rule": 12, "file": "src/Pl/pl_act_step.cpp", "added": 1, "before": 0, "after": 1}]
    detail12 = [{"rule": 12, "file": "src/Pl/pl_act_step.cpp", "line": 9,
                 "token": "pl_frame_window_44", "detail": "unowned data"}]
    check("rule12: an addition with no allowance is refused",
          module.rule12_verdict(added12, detail12, []), (False, [], ["pl_frame_window_44"], []))
    check("rule12: the named token is excused",
          module.rule12_verdict(added12, detail12, ["pl_frame_window_44"]),
          (True, ["pl_frame_window_44"], [], []))
    check("rule12: an allowance that matches nothing keeps the refusal",
          module.rule12_verdict(added12, detail12, ["some_other_symbol"]),
          (False, [], ["pl_frame_window_44"], []))
    check("rule12: a count with fewer named tokens refuses",
          module.rule12_verdict([{"rule": 12, "file": "a.cpp", "added": 2, "before": 0, "after": 2}],
                                detail12, ["pl_frame_window_44"])[0], False)
    check("rule12: another rule's addition is never excusable",
          module.rule12_verdict([{"rule": 2, "file": "a.cpp", "added": 1, "before": 0, "after": 1}],
                                detail12, ["pl_frame_window_44"])[0], False)
    check("rule12: a clean delta has nothing to excuse", module.rule12_verdict([], [], []),
          (True, [], [], []))
    module.set_allow_rule12([" pl_frame_window_44 ", "", None])
    check("rule12: set_allow_rule12 trims and drops blanks", module.ALLOW_RULE12,
          ["pl_frame_window_44"])
    module.set_allow_rule12([])


    def fake_verify_with(write, gate_code=0, gate_problems=()):
        """A `verify` stand-in: `write(main)` is "the build", then the gate's verdict and its problems."""

        def fake_verify(main, units, base, dry_run, no_build, allow_regression=None, check_outbox=True,
                        release_claims=True, problems=None, branch=None, no_selftests=False):
            write(main)
            if problems is not None:
                problems.extend(gate_problems)
            write_land_message(main, "land: batch\n\nledger: (fixture)\n")
            return gate_code

        return fake_verify

    def land_fixture(tmp, verify_stand_in, stage_scratch=False):
        """A repo on `main` with one batch file changed; run the real `land` under the stand-in."""
        repo_git(tmp, "init", "-q")
        repo_git(tmp, "checkout", "-q", "-b", "main")
        os.makedirs(os.path.join(tmp, "src"), exist_ok=True)
        with open(os.path.join(tmp, "src", "batch.c"), "w", encoding="utf-8") as fh:
            fh.write("base\n")
        repo_git(tmp, "add", "-A")
        repo_git(tmp, "commit", "-q", "-m", "base")
        base_sha = repo_git(tmp, "rev-parse", "HEAD")
        with open(os.path.join(tmp, "src", "batch.c"), "w", encoding="utf-8") as fh:
            fh.write("the batch\n")
        if stage_scratch:
            # exactly what the landing flow's own `git add -A` was measured doing: the scratch in the index
            with open(os.path.join(tmp, "d910.json"), "w", encoding="utf-8") as fh:
                fh.write("staged by another step\n")
            repo_git(tmp, "add", "-A")
        buf, err = io.StringIO(), io.StringIO()
        with mock.patch.object(module, "verify", verify_stand_in), \
                contextlib.redirect_stdout(buf), contextlib.redirect_stderr(err):
            code = land(tmp, ["Pl/pl_act"], None, no_build=True, check_outbox=False,
                        release_claims=False, subject="x")
        return code, buf.getvalue(), err.getvalue(), base_sha

    def write_scratch(main):
        with open(os.path.join(main, "d910.json"), "w", encoding="utf-8") as fh:
            fh.write("during the build\n")

    def write_foreign(main):
        # outside the batch's expected set, and the file the gate exists to protect: a foreign SOURCE edit may
        # live under `src/`/`include/`/`configure.py` (those are the batch's own paths by design) - what this
        # guard refuses is work the batch could not have produced, e.g. the ground truth.
        os.makedirs(os.path.join(main, "config", "RMHE08"), exist_ok=True)
        with open(os.path.join(main, "config", "RMHE08", "build.sha1"), "w", encoding="utf-8") as fh:
            fh.write("someone else's edit\n")

    def write_foreign_root(main):
        with open(os.path.join(main, "NOTES.md"), "w", encoding="utf-8") as fh:
            fh.write("a foreign file the batch did not receive\n")

    def write_both(main):
        write_scratch(main)
        write_foreign(main)

    with tempfile.TemporaryDirectory() as tmp:
        # the pre-flight: a foreign path ALREADY in the tree (not during the build) is named - with a likely
        # cause when it looks like lane scratch - and refused BEFORE the expensive gate runs.
        repo_git(tmp, "init", "-q")
        repo_git(tmp, "checkout", "-q", "-b", "main")
        os.makedirs(os.path.join(tmp, "src"), exist_ok=True)
        with open(os.path.join(tmp, "src", "batch.c"), "w", encoding="utf-8") as fh:
            fh.write("base\n")
        repo_git(tmp, "add", "-A")
        repo_git(tmp, "commit", "-q", "-m", "base")
        os.makedirs(os.path.join(tmp, ".tmp-mwcc", "upstream"), exist_ok=True)
        with open(os.path.join(tmp, ".tmp-mwcc", "upstream", "x.c"), "w", encoding="utf-8") as fh:
            fh.write("a clone inside MAIN\n")
        called = []

        def recording_verify(*_a, **_k):
            called.append(True)
            return 0

        buf, err = io.StringIO(), io.StringIO()
        with mock.patch.object(module, "verify", recording_verify), \
                contextlib.redirect_stdout(buf), contextlib.redirect_stderr(err):
            code = land(tmp, ["Pl/pl_act"], None, no_build=True, check_outbox=False,
                        release_claims=False, subject="x")
        check("preflight: a foreign path before the build refuses early", code, 1)
        check("... and the expensive gate never ran", called, [])
        check("... the answer line is a REFUSED", buf.getvalue().startswith("REFUSED"), True)
        check("... the pre-flight names the planted path", ".tmp-mwcc/upstream/x.c" in err.getvalue(), True)
        check("... and the likely cause (a lane launched in MAIN)", "cwd set to MAIN" in err.getvalue(), True)
        check("... and nothing was committed", repo_git(tmp, "show", "HEAD:src/batch.c"), "base")

    with tempfile.TemporaryDirectory() as tmp:
        # (a) scratch appears during the build: the batch lands, the scratch is never staged or committed, and
        # the caller's dump is left in the tree - the tolerance is named, not silent
        code, out, err, _base = land_fixture(tmp, fake_verify_with(write_scratch))
        check("scratch during the build: the landing is not refused", code, 0)
        check("... the answer line says LANDED", out.startswith("LANDED"), True)
        check("... the batch is committed", repo_git(tmp, "show", "HEAD:src/batch.c"), "the batch")
        check("... the scratch is NOT committed",
              run(["git", "cat-file", "-e", "HEAD:d910.json"], tmp).returncode != 0, True)
        check("... nothing is left staged", repo_git(tmp, "diff", "--cached", "--name-only"), "")
        check("... and the caller's dump is left alone",
              open(os.path.join(tmp, "d910.json"), encoding="utf-8").read(), "during the build\n")
        check("... the gate log names it", "d910.json" in err, True)
        check("... and says the tolerance is not a refusal", "never a refusal" in err, True)

    with tempfile.TemporaryDirectory() as tmp:
        # (b) the index already holds the scratch (the landing flow's `git add -A` did it): `land` de-indexes
        # it, so the commit can never carry a path the batch did not receive
        code, out, err, _base = land_fixture(tmp, fake_verify_with(lambda main: None), stage_scratch=True)
        check("a staged scratch does not block the landing", code, 0)
        check("... the answer line says LANDED", out.startswith("LANDED"), True)
        check("... the batch is committed", repo_git(tmp, "show", "HEAD:src/batch.c"), "the batch")
        check("... the staged scratch is de-indexed and never committed",
              run(["git", "cat-file", "-e", "HEAD:d910.json"], tmp).returncode != 0, True)
        check("... and the index is left otherwise empty",
              repo_git(tmp, "diff", "--cached", "--name-only"), "")
        check("... the file itself survives",
              open(os.path.join(tmp, "d910.json"), encoding="utf-8").read(), "staged by another step\n")
        check("... and the de-indexing is named", "removed from the index" in err, True)

    with tempfile.TemporaryDirectory() as tmp:
        # a `git reset` that fails must be REPORTED, never claimed as removed: the guarantee is "the batch can
        # never carry a path it did not receive", and a silent failure would leave that guarantee a fiction
        repo_git(tmp, "init", "-q")
        repo_git(tmp, "checkout", "-q", "-b", "main")
        with open(os.path.join(tmp, "d910.json"), "w", encoding="utf-8") as fh:
            fh.write("scratch\n")
        repo_git(tmp, "add", "-A")
        real_run = run

        def failing_reset(args, cwd):
            if args[:2] == ["git", "reset"]:
                return subprocess.CompletedProcess(args, 1, "", "index.lock exists")
            return real_run(args, cwd)

        with mock.patch.object(module, "run", failing_reset):
            note = tolerate_scratch(tmp, ["d910.json"])
        check("a failed unstage warns", "WARNING" in note, True)
        check("... and names the path", "d910.json" in note, True)
        check("... and does not claim it was removed", "removed from the index" in note, False)
        check("... the path really is still staged",
              repo_git(tmp, "diff", "--cached", "--name-only"), "d910.json")
        # and the successful path: `git reset` drops the entry and leaves the file in the tree
        check("a successful unstage reports what it removed",
              tolerate_scratch(tmp, ["d910.json"]), scratch_note(["d910.json"])
              + " (a staged copy was removed from the index)")
        check("... and the file survives",
              open(os.path.join(tmp, "d910.json"), encoding="utf-8").read(), "scratch\n")
        check("... and is no longer staged", repo_git(tmp, "diff", "--cached", "--name-only"), "")

    with tempfile.TemporaryDirectory() as tmp:
        # (c) a foreign path outside the batch that appears during the build is refused loudly - the tolerance is
        # tool scratch only, and it does not swallow the refusal when scratch appears beside it
        code, out, _err, base_sha = land_fixture(tmp, fake_verify_with(write_foreign_root))
        check("a foreign root file during the build is refused", code, 1)
        check("... the refusal names it", "appeared during the build: NOTES.md" in out, True)
        check("... the refused batch is not committed", repo_git(tmp, "rev-parse", "HEAD"), base_sha)
        check("... and its message is cleared", os.path.exists(land_message_path(tmp)), False)

    with tempfile.TemporaryDirectory() as tmp:
        code, out, _err, base_sha = land_fixture(tmp, fake_verify_with(write_foreign))
        check("a foreign ground-truth edit is refused", code, 1)
        check("... the refusal names it",
              "appeared during the build: config/RMHE08/build.sha1" in out, True)
        check("... the refused batch is not committed", repo_git(tmp, "rev-parse", "HEAD"), base_sha)

    with tempfile.TemporaryDirectory() as tmp:
        code, out, err, base_sha = land_fixture(tmp, fake_verify_with(write_both))
        check("foreign + scratch together is still refused", code, 1)
        check("... and the foreign path is the one named",
              "appeared during the build: config/RMHE08/build.sha1" in out, True)
        check("... the refused batch is not committed", repo_git(tmp, "rev-parse", "HEAD"), base_sha)
        check("... and the tolerated scratch is still named", "d910.json" in err, True)

    with tempfile.TemporaryDirectory() as tmp:
        # (d) a failing gate NAMES itself: the 2026-09-25 `proposal/800916FC` refusal printed only "the gate
        # failed - nothing staged or committed" while its lint row showed stylelint's trailing legend, so the
        # reader hunted for a defect that was not there. Both halves are asserted here - the check's name, what
        # it printed, and that a passing gate lands.
        stylelint_row = "style lint (§6.5) adds no violation"
        failing = [("ground truth (build.sha1 == the DOL's hash)", True, "", ""),
                   (stylelint_row, False,
                    "exit 1: the batch adds 3 section 6.5 violation(s) over 2 changed file(s):; "
                    "+3 rule 2  src/ef/eft029.cpp  (0 -> 3)", "")]
        named = failing_checks(failing)
        check("a failing gate produces one line per failed check", len(named), 1)
        check("... it names the check", named[0].startswith(stylelint_row + " [GATE]:"), True)
        check("... and what the check printed", "+3 rule 2 src/ef/eft029.cpp" in named[0], True)
        check("... a passing check is not reported", "ground truth" in named[0], False)
        check("... a gate failure carries its KIND", "[GATE]" in named[0], True)
        check("... and its remedy", "remedy:" in named[0], True)
        green = [("ground truth (build.sha1 == the DOL's hash)", True, "", "")]
        check("an all-green gate reports nothing", failing_checks(green), [])
        check("a bare failed gate keeps the old wording",
              land_decision(False, ["src/Pl/pl_act.cpp"]),
              ("refuse", "the gate failed - nothing staged or committed"))
        named_reason = land_decision(False, ["src/Pl/pl_act.cpp"], named,
                                     kinds_from_failures(named))[1]
        check("a failed gate refuses and names the check",
              land_decision(False, ["src/Pl/pl_act.cpp"], named)[0], "refuse")
        check("... the refusal carries the check's name", stylelint_row in named_reason, True)
        check("... and what it printed", "+3 rule 2" in named_reason, True)
        check("... and names the kind it failed as", "the gate failed" in named_reason, True)
        check("... and the per-check line carries [GATE]", "[GATE]" in named_reason, True)
        check("kinds_from_failures reads the gate tag", kinds_from_failures(named), {KIND_GATE})
        check("a full set of passing gates lands",
              land_decision(True, ["src/Pl/pl_act.cpp"], failing_checks(green)), ("commit", ""))
        code, out, _err, _base = land_fixture(tmp, fake_verify_with(lambda main: None, gate_code=1,
                                                                  gate_problems=named))
        check("a red gate never reaches git commit", code, 1)
        check("... the answer line is a refusal", out.startswith("REFUSED"), True)
        check("... and it names the failing check", stylelint_row in out, True)
        check("... and what the check printed", "+3 rule 2" in out, True)
        check("... and the batch file is still uncommitted",
              repo_git(tmp, "status", "--porcelain").startswith("M"), True)

    # a BOOKKEEPING failure (the batch is fine, the landing's own state is stale) must read differently:
    # its remedy is printed, and the refusal never says "the gate failed", so a reader does not treat a
    # released branch or a stale base as a defective batch (2026-09-26 case (a)).
    bk_row = ("every unit's branch carries its work as commits", False,
              "no commits of its own on the branch", "", KIND_BOOKKEEPING,
              "restore the branch from refs/rescue/<slug>, then re-run")
    bk_named = failing_checks([bk_row])
    check("a bookkeeping failure names its KIND", "[BOOKKEEPING]" in bk_named[0], True)
    check("... and prints its remedy", "restore the branch from refs/rescue" in bk_named[0], True)
    check("... and does not call the batch bad", "the batch itself is bad" in bk_named[0], False)
    check("kinds_from_failures reads the bookkeeping tag", kinds_from_failures(bk_named), {KIND_BOOKKEEPING})
    check("... and both tags together", kinds_from_failures(named + bk_named), {KIND_GATE, KIND_BOOKKEEPING})
    bk_reason = land_decision(False, ["src/Pl/pl_act.cpp"], bk_named,
                              kinds_from_failures(bk_named))[1]
    check("a bookkeeping refusal still refuses", bk_reason.startswith("BOOKKEEPING refusal"), True)
    check("... says the batch itself passed", "the batch itself passed" in bk_reason, True)
    check("... never says the gate failed", "the gate failed" in bk_reason, False)
    check("... and names the failing check and its remedy",
          "every unit's branch carries its work as commits" in bk_reason and "remedy:" in bk_reason, True)
    mixed_reason = land_decision(False, ["src/Pl/pl_act.cpp"], named + bk_named,
                                 kinds_from_failures(named + bk_named))[1]
    check("a mixed refusal names both kinds",
          "GATE" in mixed_reason and "BOOKKEEPING" in mixed_reason, True)
    check("failure_summary names the gate kind for a gate-only failure",
          "(GATE:" in failure_summary(failing), True)
    check("... and never says 'the gate failed' for a bookkeeping-only failure",
          "the gate failed" in failure_summary([bk_row]), False)
    check("... and a bookkeeping-only summary says the batch passed",
          "the batch itself passed" in failure_summary([bk_row]), True)

    # case (a) end to end: a `--force` release deleted `worker/<label>` and parked its only copy of the work
    # at refs/rescue/<slug>. The gate cares that the work exists as commits, so it restores the branch from
    # the rescue ref itself (`restore_rescued_branch`) instead of refusing (2026-09-26). A real temp repo,
    # because the whole point is git reachability, then the real `verify` on top.
    with tempfile.TemporaryDirectory() as tmp:
        repo_git(tmp, "init", "-q")
        repo_git(tmp, "checkout", "-q", "-b", "main")
        os.makedirs(os.path.join(tmp, "src", "Pl"), exist_ok=True)
        with open(os.path.join(tmp, "src", "Pl", "pl_act.c"), "w", encoding="utf-8") as fh:
            fh.write("base\n")
        with open(os.path.join(tmp, ".gitignore"), "w", encoding="utf-8") as fh:
            fh.write(".pi/\n")
        repo_git(tmp, "add", "-A")
        repo_git(tmp, "commit", "-q", "-m", "base")
        repo_git(tmp, "branch", branch)
        repo_git(tmp, "checkout", "-q", branch)
        with open(os.path.join(tmp, "src", "Pl", "pl_act.c"), "w", encoding="utf-8") as fh:
            fh.write("the worker's work\n")
        repo_git(tmp, "add", "-A")
        repo_git(tmp, "commit", "-q", "-m", "the worker's own work")
        rescue = claims.rescue_ref_name(unit)
        repo_git(tmp, "update-ref", rescue, repo_git(tmp, "rev-parse", branch))
        repo_git(tmp, "checkout", "-q", "main")
        repo_git(tmp, "branch", "-D", branch)          # the `release --force` shape
        check("case (a): the released branch is really gone", claims.branch_exists(tmp, branch), False)
        check("case (a): its work is preserved at the rescue ref", claims.rescue_exists(tmp, unit), rescue)
        check("case (a): the rescue ref is accepted as the branch's work", branch_problems(tmp, [unit]), [])
        restored = restore_rescued_branch(tmp, unit)
        check("case (a): the landing path restores the branch from the rescue ref", restored, branch)
        check("... the branch exists again", claims.branch_exists(tmp, branch), True)
        check("... and carries its work as commits", branch_commits(tmp, unit) > 0, True)
        check("... so branch_problems is still empty", branch_problems(tmp, [unit]), [])
        check("restoring an existing branch is a no-op", restore_rescued_branch(tmp, unit), None)
        # the real `verify`, dry-run: it restores the branch during its own check and reports the gate green
        os.makedirs(os.path.join(tmp, ".pi", "outbox"), exist_ok=True)
        with open(claims.outbox_path(tmp, unit), "w", encoding="utf-8") as fh:
            json.dump({"unit": unit, "worker": "a", "finished_at": "2026-01-01T00:00:00",
                       "unit_percent": 50.0, "symbols": [{"name": "fn_1", "percent": 50.0}],
                       "residual": "none", "measured_with": "recompile.py", "config_requests": [],
                       "flags_probed": [], "blockers": []}, fh)
        repo_git(tmp, "branch", "-D", branch)           # back to the released shape for the verify run
        check("case (a): the branch is gone again", claims.branch_exists(tmp, branch), False)
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf), contextlib.redirect_stderr(io.StringIO()):
            code = verify(tmp, [unit], repo_git(tmp, "rev-parse", "HEAD"), dry_run=True, no_build=True)
        check("case (a): the real verify passes on the rescued branch", code, 0)
        check("... and --dry-run touched nothing (no branch created)",
              claims.branch_exists(tmp, branch), False)
        # the out-parameter plumbing that the classification depends on: before the fix, reusing the
        # `problems` name for the outbox results rebound it locally and NO failed check reached the caller's
        # list (so `land`'s refusal could not name anything). A real verify must populate it.
        problems = []
        repo_git(tmp, "update-ref", "-d", rescue)      # no rescue ref: the branch is genuinely gone
        with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
            code = verify(tmp, [unit], repo_git(tmp, "rev-parse", "HEAD"), dry_run=True, no_build=True,
                          check_outbox=True, problems=problems)
        check("case (a): a failed check reaches verify's problems out-param", code, 1)
        check("... and it carries the BOOKKEEPING tag",
              any("[BOOKKEEPING]" in p for p in problems), True)
        check("... naming the branch check",
              any(p.startswith("every unit's branch carries its work as commits") for p in problems), True)

    # case (b) end to end: the batch was applied to the working tree *before* `record-base` ran, so the base's
    # dirty snapshot recorded the batch's own edits as foreign and `land_stageable` had nothing to stage. The
    # old `land` refused "the gate passed but no batch path is stageable" straight after "READY: every check
    # passed" and the batch was committed by hand. The new `land` names the ordering, and --already-applied
    # stages the paths instead (2026-09-26).
    check("case (b): an applied-before-base batch is detected",
          looks_already_applied([(" M", "src/batch.c")], {"src/batch.c"}), True)
    check("case (b): a clean batch is not", looks_already_applied([(" M", "src/batch.c")], set()), False)
    check("case (b): a batch with no changed path is not", looks_already_applied([], {"src/batch.c"}), False)
    check("case (b): a path outside the batch is not the signature",
          looks_already_applied([("??", "orig/RMHE08/sys/main.dol")], {"orig/RMHE08/sys/main.dol"}), False)

    def already_applied_repo(tmp):
        repo_git(tmp, "init", "-q")
        repo_git(tmp, "checkout", "-q", "-b", "main")
        os.makedirs(os.path.join(tmp, "src", "Pl"), exist_ok=True)
        with open(os.path.join(tmp, "src", "Pl", "pl_act.c"), "w", encoding="utf-8") as fh:
            fh.write("base\n")
        with open(os.path.join(tmp, ".gitignore"), "w", encoding="utf-8") as fh:
            fh.write(".pi/\n")
        repo_git(tmp, "add", "-A")
        repo_git(tmp, "commit", "-q", "-m", "base")
        with open(os.path.join(tmp, "src", "Pl", "pl_act.c"), "w", encoding="utf-8") as fh:
            fh.write("the batch, applied before record-base\n")
        record_base(tmp)          # the ordering mistake: the batch is already in the tree
        return repo_git(tmp, "rev-parse", "HEAD")

    with tempfile.TemporaryDirectory() as tmp:
        base_sha = already_applied_repo(tmp)
        check("case (b): record-base snapshotted the applied path as dirty",
              "src/Pl/pl_act.c" in base_dirty_paths(tmp), True)
        buf = io.StringIO()
        with mock.patch.object(module, "verify", fake_verify_with(lambda main: None)), \
                contextlib.redirect_stdout(buf), contextlib.redirect_stderr(io.StringIO()):
            code = land(tmp, ["Pl/pl_act"], None, no_build=True, check_outbox=False,
                        release_claims=False, subject="x")
        check("case (b): without --already-applied land refuses", code, 1)
        check("... and says plainly the batch is already applied", "already applied" in buf.getvalue(), True)
        check("... and never commits", repo_git(tmp, "rev-parse", "HEAD"), base_sha)
    with tempfile.TemporaryDirectory() as tmp:
        base_sha = already_applied_repo(tmp)
        buf = io.StringIO()
        with mock.patch.object(module, "verify", fake_verify_with(lambda main: None)), \
                contextlib.redirect_stdout(buf), contextlib.redirect_stderr(io.StringIO()):
            code = land(tmp, ["Pl/pl_act"], None, no_build=True, check_outbox=False,
                        release_claims=False, subject="x", already_applied=True)
        check("case (b): --already-applied lands the batch (no manual commit)", code, 0)
        check("... the answer line says LANDED", buf.getvalue().startswith("LANDED"), True)
        check("... the batch file is committed", repo_git(tmp, "show", "HEAD:src/Pl/pl_act.c"),
              "the batch, applied before record-base")
        check("... and the base commit is the commit before it", repo_git(tmp, "rev-parse", "HEAD~1"), base_sha)

    # `command_detail` is the detail a FAILED lint row carries: stylelint prints its findings FIRST and its
    # "not enforced: ..." legend LAST, so the old `output[-300:]` showed the legend and hid the violation.
    stylelint_out = ("stylelint: the batch adds 3 section 6.5 violation(s) over 2 changed file(s):\n"
                     "  +3 rule 2  src/ef/eft029.cpp  (0 -> 3)\n"
                     "  not enforced: rule 7 under src/auto/ (temporary grandfather: legacy scaffolding\n"
                     "       with bodies, until the auto/ migration lands)\n"
                     "  not enforced: rule 7 for a file with no bodies yet (a stub has nothing to name)\n")
    detail = command_detail(subprocess.CompletedProcess([], 1, stdout=stylelint_out, stderr=""))
    check("a failed lint row carries the violation", "+3 rule 2  src/ef/eft029.cpp" in detail, True)
    check("... and its exit code", "exit 1" in detail, True)
    check("... not just the legend", "temporary grandfather" in detail, False)
    check("a clean lint row says so",
          command_detail(subprocess.CompletedProcess([], 0, stdout="ok\n", stderr="")), "exit 0: ok")
    check("an empty output is not a crash",
          command_detail(subprocess.CompletedProcess([], 3, stdout="", stderr="")), "no output (exit 3)")

    # A failing `ninja` reported only `ninja: build stopped: subcommand failed.` for a whole session of gate
    # runs (2026-09-26, `ef/fn_8030681C` refused four times). Its head is `[N/M]` progress and its tail is
    # that summary, so neither end names the reason - the `FAILED: <target>` line in the middle does. The
    # detail must therefore prefer the failed outputs over either end of the output.
    ninja_out = ("[91/240] cxx src/ef/fn_8030681C.cpp\n"
                 "FAILED: build/RMHE08/src/ef/fn_8030681C.o\n"
                 "src/ef/fn_8030681C.cpp(12): error: identifier \"EftWork\" is undefined\n"
                 "ninja: build stopped: subcommand failed.\n")
    ninja_detail = command_detail(subprocess.CompletedProcess([], 1, stdout=ninja_out, stderr=""))
    check("a failed ninja row names the FAILED target",
          "FAILED: build/RMHE08/src/ef/fn_8030681C.o" in ninja_detail, True)
    check("... not the progress line", "[91/240]" in ninja_detail, False)
    check("... and not just 'build stopped'", ninja_detail.rstrip().endswith("subcommand failed."), False)

    # the snapshot the guard reads: `record_base` must capture what was dirty when it ran, so a path the
    # batch edits afterwards is batch material and one that was dirty before it is foreign. CLAUDE.md is an
    # ordinary tracked file here: an edit to it is foreign when dirty at the base and stageable when named.
    with tempfile.TemporaryDirectory() as tmp:
        repo_git(tmp, "init", "-q")
        repo_git(tmp, "checkout", "-q", "-b", "main")
        os.makedirs(os.path.join(tmp, "src"), exist_ok=True)
        with open(os.path.join(tmp, "src", "a.c"), "w", encoding="utf-8") as fh:
            fh.write("base\n")
        with open(os.path.join(tmp, "CLAUDE.md"), "w", encoding="utf-8") as fh:
            fh.write("base agents\n")
        repo_git(tmp, "add", "-A")
        repo_git(tmp, "commit", "-q", "-m", "base")
        with open(os.path.join(tmp, "src", "a.c"), "w", encoding="utf-8") as fh:
            fh.write("foreign\n")
        data = record_base(tmp)
        check("record_base snapshots the dirty set", data.get("dirty_at_base"), ["src/a.c"])
        check("the snapshot is read back", base_dirty_paths(tmp), {"src/a.c"})
        with open(os.path.join(tmp, "CLAUDE.md"), "w", encoding="utf-8") as fh:
            fh.write("base agents\nedited\n")
        check("a CLAUDE.md edit dirty at the base is foreign, like any file",
              "CLAUDE.md" in (record_base(tmp).get("dirty_at_base") or []), True)
        check("... and a CLAUDE.md edit the batch names is stageable with no special casing",
              "CLAUDE.md" in land_stageable(["CLAUDE.md"], changed_status(tmp), base_dirty_paths(tmp)), True)
        check("... and one the batch does not name, dirty at the base, is left alone",
              "CLAUDE.md" in land_stageable(["src/a.c"], changed_status(tmp), base_dirty_paths(tmp)), False)
        check("... and the clean-tree gate treats it as dirt", "CLAUDE.md" in (require_clean_tree(tmp) or ""), True)

    with tempfile.TemporaryDirectory() as tmp:
        repo_git(tmp, "init", "-q")
        repo_git(tmp, "checkout", "-q", "-b", "main")
        repo_commit(tmp, "claim-time main")
        repo_git(tmp, "branch", branch)          # the worker branch is cut here
        repo_git(tmp, "checkout", "-q", branch)
        repo_commit(tmp, "the worker's own work")  # committed on the branch
        repo_git(tmp, "checkout", "-q", "main")
        check("a branch with commits ahead of main passes", branch_commits(tmp, unit) > 0, True)
        check("the .cpp spelling finds the same branch", branch_commits(tmp, unit + ".cpp"),
              branch_commits(tmp, unit))

    with tempfile.TemporaryDirectory() as tmp:
        repo_git(tmp, "init", "-q")
        repo_git(tmp, "checkout", "-q", "-b", "main")
        repo_commit(tmp, "claim-time main")
        repo_git(tmp, "branch", branch)          # cut, but nothing committed on it
        check("a branch with no commits ahead of main fails", branch_commits(tmp, unit), 0)

    with tempfile.TemporaryDirectory() as tmp:
        repo_git(tmp, "init", "-q")
        repo_git(tmp, "checkout", "-q", "-b", "main")
        repo_commit(tmp, "claim-time main")
        repo_git(tmp, "branch", branch)          # cut before the batch base
        repo_commit(tmp, "the batch base")        # main moves on while the worker works
        repo_git(tmp, "checkout", "-q", branch)
        repo_commit(tmp, "the worker's own work")
        repo_git(tmp, "checkout", "-q", "main")
        check("a branch cut before the batch base still passes with commits ahead",
              branch_commits(tmp, unit) > 0, True)

    with tempfile.TemporaryDirectory() as tmp:
        repo_git(tmp, "init", "-q")
        repo_git(tmp, "checkout", "-q", "-b", "main")
        repo_commit(tmp, "claim-time main")
        check("a missing worker branch fails", branch_commits(tmp, unit), 0)

    # a `--force` release deletes the branch and parks the work at refs/rescue/<slug>. The gate's need is that
    # the work exists as commits, so a rescue ref carrying commits is ACCEPTED as the branch's work (2026-09-26
    # case (a)); a rescue ref with no commits of its own, or none at all, is still reported.
    with tempfile.TemporaryDirectory() as tmp:
        repo_git(tmp, "init", "-q")
        repo_git(tmp, "checkout", "-q", "-b", "main")
        repo_commit(tmp, "claim-time main")
        rescue = claims.rescue_ref_name(unit)
        repo_git(tmp, "update-ref", rescue, repo_git(tmp, "rev-parse", "HEAD"))
        problems = branch_problems(tmp, [unit])
        check("a rescue ref with no commits of its own is reported", len(problems), 1)
        check("... and names the rescue ref", rescue in problems[0], True)
        check("... and names the missing branch", claims.branch_for(unit) in problems[0], True)
        # a rescue ref that carries the worker's commits IS the branch's work
        repo_git(tmp, "checkout", "-q", "-b", "tmpwork")
        repo_commit(tmp, "the worker's own work")
        repo_git(tmp, "update-ref", rescue, repo_git(tmp, "rev-parse", "tmpwork"))
        repo_git(tmp, "checkout", "-q", "main")
        repo_git(tmp, "branch", "-D", "tmpwork")
        check("a rescue ref carrying commits is accepted", branch_problems(tmp, [unit]), [])
        check("... and restore_rescued_branch puts the real branch back",
              restore_rescued_branch(tmp, unit), claims.branch_for(unit))
        check("... which then carries the work", branch_commits(tmp, unit) > 0, True)
        check("a missing branch with no rescue ref is still reported", len(branch_problems(tmp, ["Nope/none"])), 1)

    # outbox_units() must look where brief.py wrote: the claim's branch minus worker/, not slug(unit)
    entry = {"unit": "Pl/pl_act", "worker": "a", "finished_at": "2026-01-01T00:00:00", "unit_percent": 50.0,
             "symbols": [{"name": "fn_1", "percent": 50.0}], "residual": "none",
             "measured_with": "recompile.py", "config_requests": [], "flags_probed": [], "blockers": []}
    with tempfile.TemporaryDirectory() as tmp:
        os.makedirs(os.path.join(tmp, ".pi", "outbox"), exist_ok=True)
        claims.save_registry(tmp, {"Pl/pl_act": {"branch": claims.branch_for("Pl/pl_act") + "-dd6e"}})
        json.dump(entry, open(os.path.join(tmp, ".pi", "outbox", claims.slug("Pl/pl_act") + "-dd6e.json"), "w"))
        check("a branch-derived outbox is found", outbox_units(tmp, ["Pl/pl_act"]), (["Pl/pl_act"], []))
        # the same entry under the unit-path slug is not what brief.py wrote, so the gate must not accept it
        os.remove(os.path.join(tmp, ".pi", "outbox", claims.slug("Pl/pl_act") + "-dd6e.json"))
        json.dump(entry, open(os.path.join(tmp, ".pi", "outbox", claims.slug("Pl/pl_act") + ".json"), "w"))
        ok_units, problems = outbox_units(tmp, ["Pl/pl_act"])
        check("an outbox under the unit-path slug is not found", ok_units, [])
        check("and is reported as missing", "no outbox" in (problems[0] if problems else ""), True)

    # the case that failed: a registry keyed by the extensionless name, the gate asked for the .c spelling.
    # `land.py verify --units Camellia/camellia.c` must read the same outbox as `Camellia/camellia`.
    with tempfile.TemporaryDirectory() as tmp:
        os.makedirs(os.path.join(tmp, ".pi", "outbox"), exist_ok=True)
        claims.save_registry(tmp, {"Camellia/camellia": {"branch": "worker/camellia-67ed"}})
        json.dump(dict(entry, unit="Camellia/camellia"),
                  open(os.path.join(tmp, ".pi", "outbox", "camellia-67ed.json"), "w"))
        check("the extensionless spelling finds the outbox", outbox_units(tmp, ["Camellia/camellia"])[1], [])
        check("the .c spelling finds the same outbox", outbox_units(tmp, ["Camellia/camellia.c"])[1], [])
        check("both spellings resolve to one unit",
              outbox_units(tmp, ["Camellia/camellia"])[0] == outbox_units(tmp, ["Camellia/camellia.c"])[0], True)
        check("the outbox path ignores the spelling",
              claims.outbox_path(tmp, "Camellia/camellia.c"), claims.outbox_path(tmp, "Camellia/camellia"))

    # a batch whose `--units` names a HEADER: the path carries a file extension, so `is_batch_path` marks it and
    # `unit_rows` drops it from every unit-shaped row. The outbox row used to run over the raw `--units` list,
    # so the header was validated as a unit and demanded `residual`/`flags_probed` - a BOOKKEEPING refusal while
    # every real gate row passed (2026-09-28, landed with `--no-outbox`). The fixture gives the header a
    # non-unit outbox deliberately: if the row ever runs on it again, this check fails.
    with tempfile.TemporaryDirectory() as tmp:
        os.makedirs(os.path.join(tmp, "include", "Network"))
        open(os.path.join(tmp, "include", "Network", "network_state.h"), "w").write("/* h */\n")
        os.makedirs(os.path.join(tmp, ".pi", "outbox"), exist_ok=True)
        header = "include/Network/network_state.h"
        json.dump({"unit": header, "worker": "a", "finished_at": "2026-01-01T00:00:00",
                   "unit_percent": 1.0, "symbols": [{"name": "x", "percent": 1.0}],
                   "measured_with": "n/a", "config_requests": [], "blockers": []},
                  open(claims.outbox_path(tmp, header), "w"))
        json.dump(entry, open(claims.outbox_path(tmp, "Pl/pl_act"), "w"))
        check("a header is a batch path, not a unit", is_batch_path(tmp, header), True)
        _ok, raw_problems = outbox_units(tmp, [header, "Pl/pl_act"])
        check("validating the raw list treats the header as a unit (the bug)", bool(raw_problems), True)
        check("unit_rows drops the header from the unit-shaped rows", unit_rows(tmp, [header, "Pl/pl_act"]),
              ["Pl/pl_act"])
        check("the outbox row then validates only the unit",
              outbox_units(tmp, unit_rows(tmp, [header, "Pl/pl_act"])), (["Pl/pl_act"], []))

    # a multi-unit batch: one outbox names both units' symbols. Validating it against one unit at a time made
    # every symbol of the other unit "not owned", so the batch could only land with --no-outbox (which turns
    # the outbox check off entirely). The row now reads the batch's whole owned set.
    with tempfile.TemporaryDirectory() as tmp:
        cfg = os.path.join(tmp, "config", "RMHE08")
        os.makedirs(cfg)
        with open(os.path.join(cfg, "splits.txt"), "w", encoding="utf-8") as fh:
            fh.write("A/a.c:\n    .text start:0x100 end:0x200\nB/b.c:\n    .text start:0x200 end:0x300\n")
        with open(os.path.join(cfg, "symbols.txt"), "w", encoding="utf-8") as fh:
            fh.write("fn_a = .text:0x100; // type:function size:0x10\n"
                     "fn_b = .text:0x200; // type:function size:0x10\n")
        os.makedirs(os.path.join(tmp, ".pi", "outbox"), exist_ok=True)
        branch = "worker/two-units-abcd"
        obx = os.path.join(tmp, ".pi", "outbox", claims.slug_of_branch(branch) + ".json")
        json.dump(dict(entry, unit="A/a.c + B/b.c",
                       symbols=[{"name": "fn_a", "percent": 50.0}, {"name": "fn_b", "percent": 50.0}]),
                  open(obx, "w"))
        check("a multi-unit outbox validates against the batch's whole owned set",
              outbox_units(tmp, ["A/a", "B/b"], branch=branch), (["A/a", "B/b"], []))
        # typo/stale detection stays: a symbol no batch unit owns is still refused
        json.dump(dict(entry, unit="A/a.c + B/b.c", symbols=[{"name": "fn_zzz", "percent": 50.0}]),
                  open(obx, "w"))
        check("... but a symbol no batch unit owns is still refused",
              bool(outbox_units(tmp, ["A/a", "B/b"], branch=branch)[1]), True)

    # the teardown step (owner's rule): a green gate releases the batch's claims, a failed one leaves them
    green = [("ground truth", True, "", ""), ("ok", True, "", "")]
    red = [("ground truth", True, "", ""), ("ok was recreated by THIS run", False, "stale stamp", "")]
    check("a green gate releases the batch's claims", release_plan(green, ["Pl/pl_act"], True), ["Pl/pl_act"])
    check("a failed gate leaves the claims alone", release_plan(red, ["Pl/pl_act"], True), [])
    check("--no-release turns the teardown off", release_plan(green, ["Pl/pl_act"], False), [])
    check("an empty batch releases nothing", release_plan(green, [], True), [])
    # the 2026-09-23 bug: `--no-worker-units` skipped the outbox check *and* the release, so teardowns stopped
    # for a dozen landings. The two opt-outs are independent now: skipping the outbox check must not skip this.
    check("release runs when the outbox check is off",
          release_plan(green, ["Pl/pl_act"], True, check_outbox=False), ["Pl/pl_act"])
    check("... and is unchanged by it", release_plan(green, ["Pl/pl_act"], True, check_outbox=True),
          release_plan(green, ["Pl/pl_act"], True, check_outbox=False))
    check("--no-release still stops it with the outbox check off",
          release_plan(green, ["Pl/pl_act"], False, check_outbox=False), [])
    check("summary delta", summary({"closed": 284, "matched": 217}, {"closed": 290, "matched": 223}),
          "closed 284 -> 290, matched 217 -> 223")
    check("summary tolerates a missing side", summary({}, {}), "(ledger numbers unavailable)")

    # the branch guard: `land` runs on `main`, never on a worker's branch. The incident (2026-09-24): a worker
    # told to "branch and commit there" ran `git checkout -b tools/stylelint-rule2-unsplit` in MAIN's checkout,
    # so MAIN's HEAD left `main` and the next 14 landings went onto that branch while the `main` ref sat at
    # `e3ade082`. `applybranch.sh` and `land.py` both key off `main`, so a stale `main` does not fail - it
    # silently changes what their diff means. The gate must refuse before any check runs.
    with tempfile.TemporaryDirectory() as tmp:
        repo_git(tmp, "init", "-q")
        repo_git(tmp, "checkout", "-q", "-b", "main")
        repo_commit(tmp, "claim-time main")
        check("landing on main passes the branch guard", branch_error(tmp), None)
        repo_git(tmp, "checkout", "-q", "-b", "throwaway-check")
        err = branch_error(tmp)
        check("a land off main is refused", err is not None, True)
        check("... the refusal names the branch it found", "throwaway-check" in (err or ""), True)
        check("... and tells the caller to checkout main", "git checkout main" in (err or ""), True)
        # the refusal path itself: exit 1, no gate, and the stale message is cleared like every other refusal
        import io
        write_land_message(tmp, "land: stale batch\n")
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            code = land(tmp, ["Pl/pl_act"], None, no_build=True, subject="x")
        check("a land off main refuses with exit 1", code, 1)
        check("... the refusal names the branch", "throwaway-check" in buf.getvalue(), True)
        check("... and the stale message is cleared", os.path.exists(land_message_path(tmp)), False)

    # `record-base` and `verify` are the landing path's two other manual entry points. They read MAIN's tree,
    # but MAIN is resolved from wherever the caller stands (`rc.main_root`), so both must refuse a caller that
    # is not on `main` - a worker's worktree is on the claim's branch, not main, and a MAIN left on a throwaway
    # branch would record the wrong HEAD. `caller_branch_error` is `branch_error` asked about the caller's tree;
    # the entry points themselves are exercised through `main()` with the worktree root pointed at a temp repo.
    import io
    import unittest.mock
    with tempfile.TemporaryDirectory() as tmp:
        repo_git(tmp, "init", "-q")
        repo_git(tmp, "checkout", "-q", "-b", "main")
        repo_commit(tmp, "claim-time main")
        check("record-base/verify on main pass the caller guard", caller_branch_error(tmp), None)
        repo_git(tmp, "checkout", "-q", "-b", "throwaway-check")
        err = caller_branch_error(tmp)
        check("a record-base/verify off main is refused", err is not None, True)
        check("... the refusal names the branch it found", "throwaway-check" in (err or ""), True)
        check("... and tells the caller to checkout main", "git checkout main" in (err or ""), True)
        # the normal path: `record-base` from a tree on `main` still records that HEAD
        repo_git(tmp, "checkout", "-q", "main")
        with unittest.mock.patch.object(rc, "worktree_root", return_value=tmp), \
                unittest.mock.patch.object(sys, "argv", ["land.py", "record-base", "--json"]):
            buf = io.StringIO()
            with contextlib.redirect_stdout(buf):
                code = main()
        check("record-base on main still runs", code, 0)
        check("... and records the base it read", read_base(tmp).get("base"), repo_git(tmp, "rev-parse", "HEAD"))
        # both entry points refuse a caller off `main`, before reading or writing anything
        repo_git(tmp, "checkout", "-q", "throwaway-check")
        for cmd in ("record-base", "verify"):
            with unittest.mock.patch.object(rc, "worktree_root", return_value=tmp), \
                    unittest.mock.patch.object(sys, "argv", ["land.py", cmd]):
                buf = io.StringIO()
                with contextlib.redirect_stdout(buf):
                    code = main()
            check("land.py %s off main refuses" % cmd, code, 1)
            check("... names the branch it found", "throwaway-check" in buf.getvalue(), True)

    # --- rule 2 at the registration boundary (Backlog #1): a range this batch registers makes the
    # symbols inside it owned, so a declaration of one of them still in include/unsplit/<band>.h is now a
    # rule-2 violation - and a candidate `(10505) illegal overloading`. The old stylelint `--diff` could
    # not see it: the band header is not a changed file, so it is not in the diff's file set at all. The
    # checks below are the guard's own bar - delete `band_ownership_warnings` (or the range diff) and they
    # go red on the fixture, because the warning has to FIRE for the checks to pass.
    check("range diff: an unchanged range adds nothing",
          added_split_ranges([("a", ".text", 0x1000, 0x1100)], [("a", ".text", 0x1000, 0x1100)]), [])
    check("range diff: a widened range contributes only the new strip",
          added_split_ranges([("a", ".text", 0x1000, 0x1100)], [("a", ".text", 0x1000, 0x1200)]),
          [("a", ".text", 0x1100, 0x1200)])
    check("range diff: a brand-new unit contributes its whole block",
          added_split_ranges([("a", ".text", 0x1000, 0x1100)],
                             [("a", ".text", 0x1000, 0x1100), ("b", ".text", 0x2000, 0x2100)]),
          [("b", ".text", 0x2000, 0x2100)])
    check("range diff: another section is not coverage",
          added_split_ranges([("a", ".sdata", 0x1000, 0x1100)], [("a", ".text", 0x1000, 0x1100)]),
          [("a", ".text", 0x1000, 0x1100)])
    check("range diff: two old spans leave the hole between them",
          added_split_ranges([("a", ".text", 0x1000, 0x1100), ("b", ".text", 0x1200, 0x1300)],
                             [("c", ".text", 0x1000, 0x1300)]),
          [("c", ".text", 0x1100, 0x1200)])

    def _write_tree(tmp, files):
        for rel, text in files.items():
            path = os.path.join(tmp, *rel.split("/"))
            os.makedirs(os.path.dirname(path), exist_ok=True)
            with open(path, "w", encoding="utf-8", newline="\n") as fh:
                fh.write(text)
        repo_git(tmp, "add", "-A")
        repo_git(tmp, "commit", "-q", "-m", "write tree")

    band_fixture = {
        "config/RMHE08/symbols.txt": (
            "fn_8019E9AC = .text:0x801993E0; // type:function size:0x10\n"
            "em_act_ck__FP11_ENEMY_WORKUcUc = .text:0x801993E4; // type:function size:0x8\n"
            "fn_ALREADY = .text:0x80010000; // type:function size:0x10\n"
            "fn_WIDENED = .text:0x80010014; // type:function size:0x8\n"
            "fn_OTHER = .text:0x80020000; // type:function size:0x8\n"),
        "config/RMHE08/splits.txt": (
            "Sections:\n\t.text       type:code align:32\n\n"
            "existing/unit.cpp:\n\t.text       start:0x80010000 end:0x80010010\n"),
        "configure.py": "config.libs = [\n]\n",
        "include/unsplit/enemy.h": (
            "#ifndef B1\n#define B1\n"
            "void fn_8019E9AC(void);\n"
            "void fn_OTHER(void);\n"
            "void fn_ALREADY(void);\n"
            "#endif\n"),
        "include/unsplit/widen.h": "#ifndef B2\n#define B2\nvoid fn_WIDENED(void);\n#endif\n",
        # a C++ callee declared by its clean spelling while symbols.txt holds the mangling
        "include/unsplit/cxx.h": "#ifndef B4\n#define B4\nu32 em_act_ck(struct _ENEMY_WORK*, u8);\n#endif\n",
    }

    with tempfile.TemporaryDirectory() as tmp:
        repo_git(tmp, "init", "-q")
        repo_git(tmp, "checkout", "-q", "-b", "main")
        _write_tree(tmp, band_fixture)
        base_sha = repo_git(tmp, "rev-parse", "HEAD")
        # the batch: register a new unit's range and WIDEN an existing one; add a band header that declares
        # a symbol an already-registered unit owns. It does NOT touch enemy.h or widen.h.
        _write_tree(tmp, {
            "config/RMHE08/splits.txt": (
                "Sections:\n\t.text       type:code align:32\n\n"
                "existing/unit.cpp:\n\t.text       start:0x80010000 end:0x80010020\n\n"
                "new/fn_801993E0.cpp:\n\t.text       start:0x801993E0 end:0x801993F0\n"),
            "configure.py": ("config.libs = [\n    Object(NonMatching, \"new/fn_801993E0.cpp\"),\n]\n"),
            "include/unsplit/band_added.h": "#ifndef B3\n#define B3\nvoid fn_ALREADY(void);\n#endif\n",
        })
        warns = band_ownership_warnings(tmp, base_sha)
        blob = "\n".join(warns)
        check("band: a newly-registered range that the band still declares warns",
              any("fn_8019E9AC" in w and "enemy.h" in w and "src/new/fn_801993E0.cpp" in w
                  for w in warns), True)
        check("band: a WIDENED range that the band declares warns too",
              any("fn_WIDENED" in w and "widen.h" in w and "src/existing/unit.cpp" in w
                  for w in warns), True)
        check("band: a declaration the batch ADDS of an already-owned symbol warns",
              any("fn_ALREADY" in w and "band_added.h" in w and "src/existing/unit.cpp" in w
                  for w in warns), True)
        check("band: a clean C++ spelling of a newly-owned mangled symbol warns",
              any("em_act_ck" in w and "cxx.h" in w
                  and "em_act_ck__FP11_ENEMY_WORKUcUc" in w for w in warns), True)
        check("band: exactly the four findings fire", len(warns), 4)
        # the 2026-09-26 main breakage: the rule-2 instruction to move the declaration broke `main` when it
        # was followed blindly (`756023c4e` moved `fn_80335CE8` into an owner header of a different arity and
        # three call sites lost their declaration). Every rule-2 warning now carries the call-site caveat.
        check("band: the newly-registered warning tells the reader to check the call sites first",
              any("fn_8019E9AC" in w and "after checking every call site" in w
                  and "changes its arity if the owner's prototype differs" in w for w in warns), True)
        check("band: ... and so does the added-declaration warning",
              any("fn_ALREADY" in w and "after checking every call site" in w for w in warns), True)
        check("band: ... and the C++ spelling warning",
              any("em_act_ck" in w and "after checking every call site" in w for w in warns), True)
        # a pre-existing band declaration of an already-owned symbol is NOT this batch's defect: a clean
        # batch must not be spammed with the band's whole backlog.
        check("band: a pre-existing owned declaration does not warn",
              any("enemy.h" in w and "fn_ALREADY" in w for w in warns), False)
        check("band: an unowned symbol in the band is left alone", "fn_OTHER" in blob, False)
        # the guard's own failure mode: with no base there is no diff to reason about, so it is silent
        check("band: no base is silent", band_ownership_warnings(tmp, None), [])

    with tempfile.TemporaryDirectory() as tmp:
        # a CLEAN batch - source only, no registration edit - must say nothing
        repo_git(tmp, "init", "-q")
        repo_git(tmp, "checkout", "-q", "-b", "main")
        _write_tree(tmp, dict(band_fixture, **{"src/new/fn_801993E0.cpp": "int f(void) { return 0; }\n"}))
        base_sha = repo_git(tmp, "rev-parse", "HEAD")
        _write_tree(tmp, {"src/new/fn_801993E0.cpp": "int f(void) { return 1; }\n"})
        check("band: a source-only batch is silent", band_ownership_warnings(tmp, base_sha), [])

    # --- the registration append-conflict resolver (the 2026-09-26 merger class) -------------------
    SECTION = "Sections:\n\t.text       type:code align:32\n\n"
    ANCHOR_SPLIT = "anchor.cpp:\n\t.text       start:0x80000000 end:0x80000800\n"
    TAIL_SPLIT = "tail.cpp:\n\t.text       start:0x80010000 end:0x80011000\n"
    CONF_HEAD = 'config.libs = [\n    {\n        "lib": "menu",\n        "objects": [\n'
    CONF_ANCHOR = '            Object(NonMatching, "anchor.cpp"),\n'
    CONF_TAIL = '            Object(NonMatching, "tail.cpp"),\n'
    CONF_FOOT = '        ],\n    },\n]\n'

    def _registration_files(extra_splits="", extra_conf="", anchor_split=ANCHOR_SPLIT,
                            anchor_conf=CONF_ANCHOR):
        return {
            "config/RMHE08/splits.txt": SECTION + anchor_split + extra_splits + "\n" + TAIL_SPLIT,
            "configure.py": CONF_HEAD + anchor_conf + extra_conf + CONF_TAIL + CONF_FOOT,
            "src/anchor.cpp": "int a(void) { return 0; }\n",
            "src/tail.cpp": "int t(void) { return 0; }\n",
            ".gitignore": ".pi/\n",
        }

    def _resolve_fixture(tmp, branch_splits="", main_splits="", branch_conf="", main_conf="",
                         branch_anchor_conf=CONF_ANCHOR, main_anchor_conf=CONF_ANCHOR):
        """`main` and `worker/x` both append a registration at one anchor; -> the merge-base sha."""
        repo_git(tmp, "init", "-q")
        repo_git(tmp, "checkout", "-q", "-b", "main")
        _write_tree(tmp, _registration_files())
        base = repo_git(tmp, "rev-parse", "HEAD")
        repo_git(tmp, "checkout", "-q", "-b", "worker/x")
        _write_tree(tmp, _registration_files(branch_splits, branch_conf, ANCHOR_SPLIT, branch_anchor_conf))
        repo_git(tmp, "checkout", "-q", "main")
        _write_tree(tmp, _registration_files(main_splits, main_conf, ANCHOR_SPLIT, main_anchor_conf))
        return base

    def _conflicted_worktree(tmp, branch="worker/x", name="scratch"):
        """A scratch worktree on `branch`, with `git merge main` left mid-conflict (ours=branch)."""
        wt = os.path.join(tmp, name)
        repo_git(tmp, "worktree", "add", "-b", name, wt, branch)
        subprocess.run(["git", "merge", "--no-commit", "main"], cwd=wt,
                       capture_output=True, text=True, encoding="utf-8", errors="replace")
        return wt

    with tempfile.TemporaryDirectory() as tmp:
        base = _resolve_fixture(
            tmp,
            branch_splits="menu/branch.cpp:\n\t.text       start:0x80000800 end:0x80001000\n",
            main_splits="menu/main.cpp:\n\t.text       start:0x80001000 end:0x80002000\n",
            branch_conf='            Object(NonMatching, "menu/branch.cpp"),\n',
            main_conf='            Object(NonMatching, "menu/main.cpp"),\n')
        wt = _conflicted_worktree(tmp)
        result = resolve_conflicts(wt, tmp, "worker/x", base=base)
        check("resolve: the append conflict is union-resolved", result.get("ok"), True)
        text = open(os.path.join(wt, "config", "RMHE08", "splits.txt"), encoding="utf-8").read()
        check("resolve: both bands are kept",
              "menu/branch.cpp" in text and "menu/main.cpp" in text, True)
        check("resolve: main's own unit is not dropped", "anchor.cpp" in text, True)
        check("resolve: the branch's block is first (address order)",
              text.index("menu/branch.cpp") < text.index("menu/main.cpp"), True)
        check("resolve: no conflict marker survives",
              "<<<<<<<" in text or ">>>>>>>" in text, False)
        check("resolve: exactly the two scoped paths are staged",
              sorted(p for p in repo_git(wt, "diff", "--cached", "--name-only").splitlines() if p),
              ["config/RMHE08/splits.txt", "configure.py"])
        check("resolve: no unmerged path is left", ug.unmerged(wt), {})
        repo_git(tmp, "worktree", "remove", "--force", wt)
        repo_git(tmp, "branch", "-D", "scratch")

    with tempfile.TemporaryDirectory() as tmp:
        # An UNSAFE union: both sides changed the same existing line - unionguard must refuse, and the
        # tree must keep its markers for a hand resolution.
        base = _resolve_fixture(
            tmp,
            branch_anchor_conf='            Object(NonMatching, "anchor_branch.cpp"),\n',
            main_anchor_conf='            Object(Matching, "anchor.cpp"),\n')
        wt = _conflicted_worktree(tmp)
        result = resolve_conflicts(wt, tmp, "worker/x", base=base)
        check("unsafe union: land refuses", result.get("ok"), False)
        check("... and names unionguard", "unionguard refused" in result.get("reason", ""), True)
        check("... naming the overlap reason", "same region" in result.get("reason", ""), True)
        conf = open(os.path.join(wt, "configure.py"), encoding="utf-8").read()
        check("... leaving the conflict for a hand resolution", "<<<<<<<" in conf, True)
        repo_git(tmp, "worktree", "remove", "--force", wt)
        repo_git(tmp, "branch", "-D", "scratch")

    with tempfile.TemporaryDirectory() as tmp:
        # The invariant unionguard CANNOT see: both sides append disjointly (empty base), but their brand
        # bands claim the *same* address range. The union is textually safe and would overlap - the
        # assertion is what refuses it, and nothing is written.
        base = _resolve_fixture(
            tmp,
            branch_splits="menu/dup.cpp:\n\t.text       start:0x80000900 end:0x80000940\n",
            main_splits="menu/dup2.cpp:\n\t.text       start:0x80000920 end:0x80000960\n",
            branch_conf='            Object(NonMatching, "menu/dup.cpp"),\n',
            main_conf='            Object(NonMatching, "menu/dup2.cpp"),\n')
        wt = _conflicted_worktree(tmp)
        result = resolve_conflicts(wt, tmp, "worker/x", base=base)
        check("invariant: a textually-safe but overlapping union is refused",
              result.get("ok"), False)
        check("... the reason names the overlap", "overlapping" in result.get("reason", ""), True)
        check("... and carries the violation list", bool(result.get("violations")), True)
        text = open(os.path.join(wt, "config", "RMHE08", "splits.txt"), encoding="utf-8").read()
        check("... nothing was written (markers still present)", "<<<<<<<" in text, True)
        repo_git(tmp, "worktree", "remove", "--force", wt)
        repo_git(tmp, "branch", "-D", "scratch")

    with tempfile.TemporaryDirectory() as tmp:
        # A conflict outside the scope (a header) is a content conflict, never a union.
        repo_git(tmp, "init", "-q")
        repo_git(tmp, "checkout", "-q", "-b", "main")
        _write_tree(tmp, {"include/shared.h": "int shared = 0;\n", "src/anchor.cpp": "int a;\n"})
        repo_git(tmp, "checkout", "-q", "-b", "worker/x")
        _write_tree(tmp, {"include/shared.h": "int shared = 1;\n"})
        repo_git(tmp, "checkout", "-q", "main")
        _write_tree(tmp, {"include/shared.h": "int shared = 2;\n"})
        wt = _conflicted_worktree(tmp)
        result = resolve_conflicts(wt, tmp, "worker/x", base=repo_git(tmp, "merge-base", "main", "worker/x"))
        check("scope: a header conflict is refused", result.get("ok"), False)
        check("... naming it as outside the registration scope",
              "outside the registration scope" in result.get("reason", ""), True)
        check("... and the header is left alone",
              "<<<<<<<" in open(os.path.join(wt, "include", "shared.h"), encoding="utf-8").read(), True)
        repo_git(tmp, "worktree", "remove", "--force", wt)
        repo_git(tmp, "branch", "-D", "scratch")

    with tempfile.TemporaryDirectory() as tmp:
        repo_git(tmp, "init", "-q")
        repo_git(tmp, "checkout", "-q", "-b", "main")
        _write_tree(tmp, {"src/a.cpp": "int a;\n"})
        result = resolve_conflicts(tmp, tmp, "main", base=None)
        check("MAIN is refused as a resolution tree", result.get("ok"), False)
        check("... naming MAIN", "inside MAIN" in result.get("reason", ""), True)

    with tempfile.TemporaryDirectory() as tmp:
        # scratch_resolve: the merger lane's operation - a temp worktree, `git merge main`, resolve,
        # commit - with MAIN's HEAD provably untouched.
        base = _resolve_fixture(
            tmp,
            branch_splits="menu/branch.cpp:\n\t.text       start:0x80000800 end:0x80001000\n",
            main_splits="menu/main.cpp:\n\t.text       start:0x80001000 end:0x80002000\n",
            branch_conf='            Object(NonMatching, "menu/branch.cpp"),\n',
            main_conf='            Object(NonMatching, "menu/main.cpp"),\n')
        main_head = repo_git(tmp, "rev-parse", "HEAD")
        result = scratch_resolve(tmp, "worker/x")
        check("scratch: the branch resolves in a scratch worktree", result.get("ok"), True)
        check("... a scratch branch is named", bool(result.get("scratch_branch")), True)
        check("... MAIN's HEAD is untouched", repo_git(tmp, "rev-parse", "HEAD"), main_head)
        text = open(os.path.join(result["worktree"], "config", "RMHE08", "splits.txt"),
                    encoding="utf-8").read()
        check("... the merge commit carries both bands",
              "menu/branch.cpp" in text and "menu/main.cpp" in text, True)
        check("... the merge is committed on the scratch branch",
              repo_git(result["worktree"], "rev-parse", "HEAD") != main_head, True)
        repo_git(tmp, "worktree", "remove", "--force", result["worktree"])
        repo_git(tmp, "branch", "-D", result["scratch_branch"])

    with tempfile.TemporaryDirectory() as tmp:
        _resolve_fixture(
            tmp,
            branch_anchor_conf='            Object(NonMatching, "anchor_branch.cpp"),\n',
            main_anchor_conf='            Object(Matching, "anchor.cpp"),\n')
        result = scratch_resolve(tmp, "worker/x")
        check("scratch: an unsafe merge is refused", result.get("ok"), False)
        check("... and the scratch worktree is removed",
              repo_git(tmp, "worktree", "list", "--porcelain").count("worktree "), 1)

    # --- the one-command landing (`land --branch`) -------------------------------------------------
    with tempfile.TemporaryDirectory() as tmp:
        repo_git(tmp, "init", "-q")
        repo_git(tmp, "checkout", "-q", "-b", "main")
        _write_tree(tmp, {"src/a.cpp": "int a;\n", "CLAUDE.md": "base\n"})
        check("clean-tree: a committed tree is clean", require_clean_tree(tmp), None)
        open(os.path.join(tmp, "src", "a.cpp"), "w", encoding="utf-8").write("dirty\n")
        dirty = require_clean_tree(tmp)
        check("clean-tree: a tracked edit is refused", dirty is not None, True)
        check("... naming the path", "src/a.cpp" in (dirty or ""), True)
        check("... and the exact clean command", "stash push --include-untracked" in (dirty or ""), True)
        repo_git(tmp, "checkout", "--", "src/a.cpp")
        with open(os.path.join(tmp, "src", "a.cpp"), "w", encoding="utf-8") as fh:
            fh.write("real dirt\n")
        message = require_clean_tree(tmp) or ""
        check("clean-tree: the dirt is refused", "main's tree is not clean: M src/a.cpp" in message, True)
        check("... and the stash command names it", "stash push --include-untracked -- src/a.cpp" in message,
              True)
        repo_git(tmp, "checkout", "--", "src/a.cpp")

    def _land_verify_ok(main, units, base, dry_run, no_build, allow_regression=None,
                        check_outbox=True, release_claims=True, problems=None, branch=None,
                        no_selftests=False):
        write_land_message(main, "land: %s\n\nledger: (fixture)\n" % ",".join(units))
        return 0

    with tempfile.TemporaryDirectory() as tmp:
        base = _resolve_fixture(
            tmp,
            branch_splits="menu/branch.cpp:\n\t.text       start:0x80000800 end:0x80001000\n",
            main_splits="menu/main.cpp:\n\t.text       start:0x80001000 end:0x80002000\n",
            branch_conf='            Object(NonMatching, "menu/branch.cpp"),\n',
            main_conf='            Object(NonMatching, "menu/main.cpp"),\n')
        check("units-from-branch: reads the branch's registration",
              units_from_branch(tmp, "worker/x", base), ["menu/branch"])
        main_head = repo_git(tmp, "rev-parse", "HEAD")
        buf = io.StringIO()
        with mock.patch.object(module, "verify", _land_verify_ok), contextlib.redirect_stdout(buf):
            code = land_branch(tmp, "worker/x", no_build=True, check_outbox=False,
                               release_claims=False, subject="selftest")
        check("land --branch: lands the branch in one command", code, 0)
        check("... the answer line says LANDED", buf.getvalue().startswith("LANDED"), True)
        check("... main advanced past the base", repo_git(tmp, "rev-parse", "HEAD") != main_head, True)
        text = open(os.path.join(tmp, "config", "RMHE08", "splits.txt"), encoding="utf-8").read()
        check("... both bands are in the committed file",
              "menu/branch.cpp" in text and "menu/main.cpp" in text, True)
        check("... and the tree is clean afterwards", require_clean_tree(tmp), None)
        check("... and a conflict-free landing leaves no land/* ref",
              repo_git(tmp, "for-each-ref", "refs/heads/land/"), "")

    with tempfile.TemporaryDirectory() as tmp:
        # A landing that goes through the conflict-resolution path: `scratch_resolve` parked the union on
        # a `land/resolve-*` helper (and its scratch worktree), the caller fast-forwarded the branch onto
        # it, and the landing deletes the now-redundant helper - visibly.
        _resolve_fixture(
            tmp,
            branch_splits="menu/branch.cpp:\n\t.text       start:0x80000800 end:0x80001000\n",
            main_splits="menu/main.cpp:\n\t.text       start:0x80001000 end:0x80002000\n",
            branch_conf='            Object(NonMatching, "menu/branch.cpp"),\n',
            main_conf='            Object(NonMatching, "menu/main.cpp"),\n')
        resolved = scratch_resolve(tmp, "worker/x")
        helper = resolved["scratch_branch"]
        check("resolve helper: the union is parked on a land/resolve-* ref",
              repo_git(tmp, "for-each-ref", "--format=%(refname)", "refs/heads/land/").endswith(helper),
              True)
        repo_git(tmp, "branch", "-f", "worker/x", helper)      # the caller's documented fast-forward
        buf = io.StringIO()
        with mock.patch.object(module, "verify", _land_verify_ok), contextlib.redirect_stdout(buf), \
                contextlib.redirect_stderr(buf):
            code = land_branch(tmp, "worker/x", no_build=True, check_outbox=False, release_claims=False,
                               subject="selftest helper")
        check("resolve helper: the branch lands", code, 0)
        check("resolve helper: the landing leaves no land/* ref behind",
              repo_git(tmp, "for-each-ref", "refs/heads/land/"), "")
        check("resolve helper: the deletion is visible in the landing output",
              "resolve helper refs/heads/%s deleted" % helper in buf.getvalue(), True)

    with tempfile.TemporaryDirectory() as tmp:
        # The helper carries a hand fix the branch never took: deleting it would drop the only copy, so
        # the landing must refuse loudly and leave it alone.
        _resolve_fixture(
            tmp,
            branch_splits="menu/branch.cpp:\n\t.text       start:0x80000800 end:0x80001000\n",
            main_splits="menu/main.cpp:\n\t.text       start:0x80001000 end:0x80002000\n",
            branch_conf='            Object(NonMatching, "menu/branch.cpp"),\n',
            main_conf='            Object(NonMatching, "menu/main.cpp"),\n')
        resolved = scratch_resolve(tmp, "worker/x")
        helper = resolved["scratch_branch"]
        _write_tree(resolved["worktree"], {"src/hand_fix.cpp": "int hand_fix;\n"})
        buf = io.StringIO()
        with mock.patch.object(module, "verify", _land_verify_ok), contextlib.redirect_stdout(buf), \
                contextlib.redirect_stderr(buf):
            code = land_branch(tmp, "worker/x", no_build=True, check_outbox=False, release_claims=False,
                               subject="selftest helper fix")
        check("resolve helper fix: the branch still lands", code, 0)
        check("resolve helper fix: the helper is NOT deleted",
              repo_git(tmp, "for-each-ref", "--format=%(refname)", "refs/heads/land/").endswith(helper),
              True)
        check("resolve helper fix: the refusal is named",
              "REFUSING to delete resolve helper" in buf.getvalue(), True)
        repo_git(tmp, "worktree", "remove", "--force", resolved["worktree"])
        repo_git(tmp, "branch", "-D", helper)

    with tempfile.TemporaryDirectory() as tmp:
        _resolve_fixture(
            tmp,
            branch_anchor_conf='            Object(NonMatching, "anchor_branch.cpp"),\n',
            main_anchor_conf='            Object(Matching, "anchor.cpp"),\n')
        main_head = repo_git(tmp, "rev-parse", "HEAD")
        buf = io.StringIO()
        with mock.patch.object(module, "verify", _land_verify_ok), contextlib.redirect_stdout(buf):
            code = land_branch(tmp, "worker/x", no_build=True, check_outbox=False, release_claims=False)
        check("land --branch: an unresolvable conflict is refused", code, 1)
        check("... the answer line says REFUSED", buf.getvalue().startswith("REFUSED"), True)
        check("... naming unionguard", "unionguard refused" in buf.getvalue(), True)
        check("... and the tree is left clean (the apply was undone)", require_clean_tree(tmp), None)
        check("... with main unchanged", repo_git(tmp, "rev-parse", "HEAD"), main_head)

    with tempfile.TemporaryDirectory() as tmp:
        repo_git(tmp, "init", "-q")
        repo_git(tmp, "checkout", "-q", "-b", "main")
        _write_tree(tmp, {"src/a.cpp": "int a;\n"})
        repo_git(tmp, "branch", "worker/x")
        open(os.path.join(tmp, "src", "a.cpp"), "w", encoding="utf-8").write("dirty\n")
        buf = io.StringIO()
        with mock.patch.object(module, "verify", _land_verify_ok), contextlib.redirect_stdout(buf):
            code = land_branch(tmp, "worker/x", no_build=True, check_outbox=False, release_claims=False)
        check("land --branch: a dirty tree is refused before anything", code, 1)
        check("... the refusal names the clean command", "stash push --include-untracked" in buf.getvalue(), True)
        check("... and main did not move", repo_git(tmp, "rev-parse", "HEAD"), repo_git(tmp, "rev-parse", "main"))

    with tempfile.TemporaryDirectory() as tmp:
        # A unit renamed at registration: the outbox keeps the pre-registration branch slug. The
        # branch-derived lookup finds it; the unit-derived one does not - and says --no-outbox is the
        # remedy.
        os.makedirs(os.path.join(tmp, ".pi", "outbox"), exist_ok=True)
        renamed = "worker/old-proposal-name-abcd"
        slug = claims.slug_of_branch(renamed)
        json.dump(dict(entry, unit="hud/fn_80334568"),
                  open(os.path.join(tmp, ".pi", "outbox", slug + ".json"), "w"))
        ok, problems = outbox_units(tmp, ["hud/fn_80334568"])
        check("renamed unit: the unit-derived outbox path misses", ok, [])
        check("... and the problem names --no-outbox", "--no-outbox" in (problems[0] if problems else ""), True)
        ok, problems = outbox_units(tmp, ["hud/fn_80334568"], branch=renamed)
        check("renamed unit: the branch-derived path finds the outbox", ok, ["hud/fn_80334568"])
        check("... with no problems", problems, [])
        check("renamed unit: the claim key is read from the branch",
              claim_unit_for_branch(tmp, renamed), None)   # no registry entry: None, not a wrong key

    with tempfile.TemporaryDirectory() as tmp:
        # the CLI wiring for the one command, through `main()` with MAIN pointed at the fixture
        _resolve_fixture(
            tmp,
            branch_splits="menu/branch.cpp:\n\t.text       start:0x80000800 end:0x80001000\n",
            main_splits="menu/main.cpp:\n\t.text       start:0x80001000 end:0x80002000\n",
            branch_conf='            Object(NonMatching, "menu/branch.cpp"),\n',
            main_conf='            Object(NonMatching, "menu/main.cpp"),\n')
        with mock.patch.object(rc, "worktree_root", return_value=tmp), \
                mock.patch.object(rc, "main_root", return_value=tmp), \
                mock.patch.object(module, "verify", _land_verify_ok), \
                mock.patch.object(sys, "argv", ["land.py", "land", "--branch", "worker/x",
                                                "--no-build", "--no-outbox", "--no-release"]):
            buf = io.StringIO()
            with contextlib.redirect_stdout(buf):
                code = main()
        check("land --branch runs through main()", code, 0)
        check("... and prints the one answer line", buf.getvalue().startswith("LANDED"), True)

    # --- F34: the gate's own decode, and the lint that keeps it that way ---------------------------------
    # `land.run` used `text=True` with no codec, so git's UTF-8 stdout was decoded with the host's locale
    # codec (`cp1252` here) while a file was compared against it read as UTF-8.  One em dash in CLAUDE.md's
    # prose made the two spellings differ, and the landing gate refused *every* landing with "main's tree is
    # not clean: M CLAUDE.md" - a false refusal.  The fixture is that em dash at byte level: a check that
    # decodes through the locale is exactly the failure it is here to catch, so it must not itself be
    # host-dependent.
    dash = "\u2014"
    probe = run([sys.executable, "-c", "import sys; sys.stdout.buffer.write(bytes.fromhex('%s'))"
                 % dash.encode("utf-8").hex()], SELF_REPO)
    check("F34: `run` decodes a subprocess' UTF-8 stdout as UTF-8", probe.stdout, dash)
    with tempfile.TemporaryDirectory() as tmp:
        repo_git(tmp, "init", "-q")
        repo_git(tmp, "checkout", "-q", "-b", "main")
        prose = "prose with an em dash %s in it\n" % dash
        _write_tree(tmp, {"CLAUDE.md": prose})
        head_text = run(["git", "show", "HEAD:CLAUDE.md"], tmp).stdout
        head_bytes = subprocess.run(["git", "show", "HEAD:CLAUDE.md"], cwd=tmp,
                                    capture_output=True).stdout
        check("F34: `run` returns HEAD's em dash, not its cp1252 spelling", dash in head_text, True)
        check("... where a locale decode of those same bytes would not have matched",
              dash in head_bytes.decode("cp1252", "replace"), False)
        check("F34: an unchanged non-ASCII CLAUDE.md is clean", require_clean_tree(tmp), None)
        with open(os.path.join(tmp, "CLAUDE.md"), "w", encoding="utf-8", newline="") as fh:
            fh.write("prose with an em dash %s and an EDIT\n" % dash)
        check("... while a real edit is dirt, naming CLAUDE.md", "CLAUDE.md" in (require_clean_tree(tmp) or ""), True)

    strays = sp.trap_sites(SELF_REPO)
    check("every text-mode subprocess call in tools/ pins its codec (F34's rule)", strays, [])

    if fails:
        print("FAIL (%d)" % len(fails))
        for f in fails:
            print("  " + f)
        return 1
    print("ok - %d checks" % checks)
    return 0


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--selftest", action="store_true")
    sub = ap.add_subparsers(dest="cmd")
    rb = sub.add_parser("record-base", help="record main's HEAD as the batch base")
    rb.add_argument("--json", action="store_true")
    rb.add_argument("--units", default=None,
                    help="comma-separated batch units to compile and snapshot (the add-only row's base)")
    v = sub.add_parser("verify", help="run the batch checklist (never commits; use `land` for that)")
    v.add_argument("--base", default=None, help="expected main HEAD (default: the recorded base)")
    v.add_argument("--units", default=None, help="comma-separated units in this batch")
    v.add_argument("--dry-run", action="store_true", help="run the cheap checks only; touch nothing")
    v.add_argument("--no-build", action="store_true", help="skip the split/link/ok/baseline steps")
    v.add_argument("--no-outbox", "--no-worker-units", action="store_true", dest="no_outbox",
                   help="skip the outbox/branch checks (an orchestrator-only batch); release still runs, "
                        "add --no-release to skip that too. `--no-worker-units` is the old alias and no "
                        "longer turns the teardown off")
    v.add_argument("--no-release", action="store_true", dest="no_release",
                   help="do not release the batch's claims")
    v.add_argument("--allow-regression", action="append", default=[],
                   help="unit whose measured regression is authorised by a rule (recorded in the message); repeatable")
    v.add_argument("--allow-rule12", action="append", default=[], metavar="TOKEN",
                   help="rule-12 token (the unowned data symbol an `extern` names) a landing accepts "
                        "deliberately, with the claim already scheduled; repeatable, recorded in the "
                        "landing log, never a key in a file")
    v.add_argument("--no-selftests", action="store_true", dest="no_selftests",
                   help="skip the all-tool-selftests row (the fast path; `python tools/selftest.py "
                        "--changed` is the narrower lane loop)")
    v.add_argument("--json", action="store_true")
    ld = sub.add_parser("land", help="gate + stage + commit + release; one answer line, exit status is the answer")
    ld.add_argument("--base", default=None, help="expected main HEAD (default: the recorded base)")
    ld.add_argument("--units", default=None, help="comma-separated units in this batch")
    ld.add_argument("--branch", default=None,
                    help="land a whole branch: refuse a dirty tree, record-base, apply the branch with the "
                         "registration union, gate, commit and release (derives --units from the branch's "
                         "registration diff when --units is omitted)")
    ld.add_argument("--no-build", action="store_true", help="skip the baseline step")
    ld.add_argument("--no-outbox", "--no-worker-units", action="store_true", dest="no_outbox",
                    help="skip the outbox/branch checks (release still runs)")
    ld.add_argument("--no-release", action="store_true", dest="no_release",
                    help="commit without releasing the batch's claims")
    ld.add_argument("--allow-regression", action="append", default=[],
                    help="unit whose measured regression is authorised by a rule; repeatable")
    ld.add_argument("--allow-rule10", action="append", default=[],
                    help="rule-10 violation key a landing accepts deliberately, e.g. "
                         "`run:.data:805FB0F8` for the Pat vtable the owner ruled stays claimed while "
                         "its slots are written; repeatable, recorded in the landing log, never a key "
                         "in a file")
    ld.add_argument("--allow-rule12", action="append", default=[], metavar="TOKEN",
                    help="rule-12 token (the unowned data symbol an `extern` names) a landing accepts "
                         "deliberately, with the claim already scheduled; repeatable, recorded in the "
                         "landing log, never a key in a file")
    ld.add_argument("--no-selftests", action="store_true", dest="no_selftests",
                    help="skip the all-tool-selftests row (the fast path; `python tools/selftest.py "
                         "--changed` is the narrower lane loop)")
    ld.add_argument("--message", default=None, help="override the gate message's subject line")
    ld.add_argument("--already-applied", action="store_true", dest="already_applied",
                    help="the batch was applied before `record-base` ran, so its paths are in the base's "
                         "dirty snapshot; stage them anyway (otherwise `land` refuses and says so)")
    rs = sub.add_parser("resolve", help="union-resolve a branch's registration append-conflict in a "
                                         "scratch tree (never in MAIN); exit status is the answer")
    rs.add_argument("--branch", required=True, help="the worker branch to resolve")
    rs.add_argument("--worktree", default=None,
                    help="a worktree mid-merge on --branch to resolve in (default: a fresh scratch "
                         "worktree, `git merge main`)")
    rs.add_argument("--main", default=None, help="MAIN worktree (default: resolved with git)")
    rs.add_argument("--base", default=None, help="the merge base (default: git merge-base main <branch>)")
    rs.add_argument("--no-commit", action="store_true",
                    help="resolve and stage the scoped paths but do not commit")
    rs.add_argument("--json", action="store_true")
    args = ap.parse_args()

    if args.selftest:
        return selftest()
    main = rc.main_root(rc.worktree_root())
    if args.cmd == "record-base":
        # manual entry point: it reads MAIN's tree, so it must refuse a caller that is not in MAIN on main
        bad_branch = caller_branch_error()
        if bad_branch:
            print("REFUSED record-base | %s" % bad_branch)
            return 1
        units = [u.strip() for u in (args.units or "").split(",") if u.strip()]
        data = record_base(main, units)
        print(json.dumps(data, indent=2) if args.json else "base %s (%s)" % (data["base"], data["subject"]))
        return 0
    if args.cmd == "verify":
        bad_branch = caller_branch_error()
        if bad_branch:
            print("REFUSED verify | %s" % bad_branch)
            return 1
        units = [u.strip() for u in (args.units or "").split(",") if u.strip()]
        set_allow_rule12(args.allow_rule12)
        return verify(main, units, args.base, args.dry_run, args.no_build, args.allow_regression,
                      check_outbox=not args.no_outbox, release_claims=not args.no_release,
                      no_selftests=args.no_selftests)
    if args.cmd == "land":
        if args.branch:
            units = [u.strip() for u in (args.units or "").split(",") if u.strip()]
            set_allow_rule10(args.allow_rule10)
            set_allow_rule12(args.allow_rule12)
            return land_branch(main, args.branch, units=units, base=args.base, no_build=args.no_build,
                               allow_regression=args.allow_regression,
                               check_outbox=not args.no_outbox, release_claims=not args.no_release,
                               subject=args.message, no_selftests=args.no_selftests)
        if not args.units:
            print("REFUSED | land needs --units a,b or --branch worker/<slug>")
            return 1
        units = [u.strip() for u in args.units.split(",") if u.strip()]
        set_allow_rule10(args.allow_rule10)
        set_allow_rule12(args.allow_rule12)
        return land(main, units, args.base, args.no_build, args.allow_regression,
                    allow_rule10=args.allow_rule10,
                    check_outbox=not args.no_outbox, release_claims=not args.no_release,
                    subject=args.message, already_applied=args.already_applied,
                    no_selftests=args.no_selftests)
    if args.cmd == "resolve":
        main = args.main or rc.main_root(rc.worktree_root())
        if args.worktree:
            base = args.base or git(["merge-base", "main", args.branch], main).strip()
            result = resolve_conflicts(args.worktree, main, args.branch, base=base,
                                       commit=not args.no_commit)
        else:
            result = scratch_resolve(main, args.branch, base=args.base, commit=not args.no_commit)
        if args.json:
            print(json.dumps(result, indent=2))
        elif result.get("ok"):
            tail = ""
            if result.get("scratch_branch"):
                tail = (" | scratch branch %s in %s (fast-forward the worker branch with `git branch -f "
                        "%s %s` when it is not checked out)"
                        % (result["scratch_branch"], result.get("worktree"), args.branch,
                           result["scratch_branch"]))
            print("RESOLVED %s | %s%s" % (args.branch, result.get("reason"), tail))
        else:
            print("REFUSED %s | %s" % (args.branch, result.get("reason")))
        return 0 if result.get("ok") else 1
    ap.print_help()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
