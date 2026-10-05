"""The gate's shared primitives: the batch path classes, the row KIND, git/run, the base file, the `Batch` context.
Spec: docs/tools/spec/landing.md. CLI: none (a module of the `land.py` gate)."""
from __future__ import annotations

import json
import os
import re
import subprocess

from dataclasses import dataclass
from dataclasses import field

from tools.lib import findings as _findings
from tools.lib import proc
from tools.lib import repo as _repo
from tools.lib.git import Git
from tools.lib.lanes import naming


#: The repository the tools layer lives in.  `land` always works on MAIN (`main_root`, walked from wherever
#: the caller stands); this is the *source* tree (the `integrate` forward and the tests read it).
SELF_REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))


ALLOWED_PREFIXES = ("src/", "include/", "docs/", "tools/", ".claude/", ".github.example/")


# Root documents and repo-config files a docs/tooling batch legitimately edits (commit categories
# `repo/readme`, `repo/license`, `repo/gitignore`, `repo/ci`): README.md, LICENSE, `.gitattributes` (line-ending
# policy) and `.flake8` (the tools' lint config). Deliberately absent: `.gitmodules` and `Add-Exclusion.ps1`.
ALLOWED_FILES = ("configure.py", "CLAUDE.md", ".gitignore", "README.md", "LICENSE", ".gitattributes", ".flake8",
                 "config/RMHE08/splits.txt", "config/RMHE08/symbols.txt")


BASE_FILE = os.path.join(".pi", "land-base.json")


#: `config.yml` - outside the path lists, admitted by content (`outside_batch`, `config_verdict`).
CONFIG_PATH = _repo.CONFIG_PATH


# Tool scratch a batch never owns, and the only thing outside `ALLOWED_*` this gate tolerates. An
# `objdiff-cli diff` run from the repo root - typically a caller that names its dump after the symbol it is
# looking at (`d910.json`, the `diff` of fn_8009A910, plus a byte-identical `t910.json`) - leaves these in the
# repo root. `65492794` put them in `.gitignore`, but a **staged** file bypasses `.gitignore`, and a stage
# happened anyway: a landing flow's own `git add -A` swept `d910.json` into the
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
KIND_GATE = _findings.KIND_GATE


KIND_BOOKKEEPING = _findings.KIND_BOOKKEEPING


KIND_TAG = _findings.KIND_TAG


KIND_REMEDY = {
    KIND_GATE: "the batch itself is bad - fix the batch; do not override the gate",
    KIND_BOOKKEEPING: ("the batch itself is fine - repair the landing's own state (the remedy above), "
                       "then re-run"),
}


def check_kind(row: tuple) -> str:
    """The KIND of a check row. A 4-tuple (an older caller, a test fixture) reads as GATE - a refusal whose
    kind is unknown must never be soft-pedalled as mere bookkeeping."""
    return _findings.Row.from_tuple(row).kind


def check_remedy(row: tuple) -> str:
    """The remedy a row carries, falling back to the generic remedy of its kind."""
    r = _findings.Row.from_tuple(row)
    return r.remedy or KIND_REMEDY[r.kind]


def failed_kinds(checks: list) -> set[str]:
    """The set of KINDs among the failed rows of `checks`."""
    return _findings.Verdict.of(checks).failed_kinds


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
    return proc.run(args, cwd=cwd)


def git(args: list[str], cwd: str, check: bool = True) -> str:
    p = Git(cwd).run(*args)
    if check and p.returncode != 0:
        raise SystemExit("git %s failed in %s: %s" % (" ".join(args), cwd, p.stderr.strip()))
    return p.stdout


def read_base(main: str) -> dict:
    path = os.path.join(main, BASE_FILE)
    if not os.path.exists(path):
        return {}
    try:
        return json.loads(open(path, encoding="utf-8").read())
    except json.JSONDecodeError:
        return {}


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
    for row in _findings.rows_of(checks):
        if not row.failed:
            continue
        note = " ".join((row.detail or row.evidence or "no detail").split())
        out.append("%s [%s]: %s (remedy: %s)"
                   % (row.name, KIND_TAG[row.kind], note[:240] or "no detail",
                      row.remedy or KIND_REMEDY[row.kind]))
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
        norm = naming.norm_unit(unit.strip("/"))
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


def worktree_root(start: str | None = None) -> str:
    """The tree the caller is in - resolved from the *cwd* (`rev-parse --show-toplevel`), never from this
    file's location (`lib.repo.worktree_root`)."""
    return _repo.worktree_root(start)


def main_root(current: str) -> str:
    """MAIN's worktree path (`lib.repo.main_checkout`: the git common dir's parent)."""
    return _repo.main_checkout(current)


@dataclass
class Batch:
    """One `verify` run: the batch's inputs, the rows so far (`checks`), and the values one row hands a later one.

    Mutable on purpose - the rows run in order and the post-build rows read what the pre-build rows recorded
    (the base snapshot, the scratch the guard tolerated, the band warnings the message carries)."""
    main: str
    units: list[str]
    unit_units: list[str]
    base: str | None                      # the batch base the rows compare against (`--base` or the recorded one)
    recorded: dict                        # `.pi/land-base.json` as `record-base` wrote it
    dry_run: bool = False
    no_build: bool = False
    branch: str | None = None
    check_outbox: bool = True
    release_claims: bool = True
    no_selftests: bool = False
    allow_regression: list[str] = field(default_factory=list)
    checks: list[_findings.Row] = field(default_factory=list)
    paths: list[str] = field(default_factory=list)
    scratch: list[str] = field(default_factory=list)
    band_warnings: list[str] = field(default_factory=list)
    warnings: list[str] = field(default_factory=list)   # `warn` rows' findings: the gate log, the message, the land log
    subject: str = ""
    extra: dict = field(default_factory=dict)

    def check(self, name: str, good: bool, detail: str = "", info: str = "", kind: str = KIND_GATE,
              remedy: str = "") -> None:
        """Append one row (`lib.findings.Row`): PASS/FAIL from `good`, `detail` shown on a failure, `info` on a
        pass, `kind` GATE or BOOKKEEPING, `remedy` what to do."""
        self.checks.append(_findings.Row.check(name, good, detail, info, kind, remedy))

    def warn(self, name: str, found: list[str], info: str = "", kind: str = KIND_BOOKKEEPING,
             remedy: str = "") -> None:
        """A WARNING row (`<name> (warning)`): always PASS, never a refusal. Each finding is printed as `WARNING:
        <name>: <finding>` and kept on `warnings`, which the commit body and the landing log carry."""
        for item in found:
            print("WARNING: %s: %s" % (name, item))
        self.warnings.extend("%s: %s" % (name, item) for item in found)
        self.check(name + " (warning)", True,
                   info=("WARNING (%d): %s%s" % (len(found), "; ".join(found[:4]),
                                                  (" - " + remedy) if remedy else "")) if found else info,
                   kind=kind, remedy=remedy)

    @property
    def ok(self) -> bool:
        return _findings.Verdict.of(self.checks).ok


def unit_owned_paths(units: list[str]) -> set[str]:
    """The source paths a batch's units own: `src/<unit>.<ext>` and any path the unit names directly."""
    owned: set[str] = set()
    for unit in units:
        unit = unit.strip("/")
        if not unit:
            continue
        owned.add(unit)
        normalized = naming.norm_unit(unit)
        owned.add("src/" + normalized)
        for ext in (".c", ".cpp", ".cp"):
            owned.add(unit + ext)
            owned.add("src/" + normalized + ext)
    return owned


def config_verdict(main: str) -> dict:
    """`lib.repo.config_change` of `main`'s working-tree `config.yml` against its HEAD copy - the decision the
    pre-commit hook (`guard.py config`) makes on the staged copy, so the gate and the hook cannot disagree."""
    p = run(["git", "show", "HEAD:" + _repo.CONFIG_PATH], main)
    old = p.stdout if p.returncode == 0 else None
    path = os.path.join(main, *_repo.CONFIG_PATH.split("/"))
    new = None
    if os.path.isfile(path):
        with open(path, encoding="utf-8", newline="") as fh:
            new = fh.read()
    return _repo.config_change(old, new)


def outside_batch(paths: list[str], allowed: tuple[str, ...] = ALLOWED_PREFIXES,
                  allowed_files: tuple[str, ...] = ALLOWED_FILES, main: str | None = None) -> list[str]:
    """Paths a batch may not touch: everything the plan keeps for the orchestrator, minus its own writes.

    `config.yml` is judged by content when `main` is given: a change of its relocation-analysis keys only
    (`config_verdict`, the hook's own rule) belongs to the batch; any other change - and every path the lists
    do not name, `build.sha1` among them - stays outside it."""
    bad = []
    verdict = None
    for path in paths:
        if path in allowed_files or path.startswith(allowed):
            continue
        if path == _repo.CONFIG_PATH and main is not None:
            verdict = verdict or config_verdict(main)
            if verdict["ok"]:
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
