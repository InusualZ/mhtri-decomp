"""Compile one unit **without ninja**, from any worktree, and prove the object is fresh.

The problem this solves (docs/plan.md 7.1 + 7.15): a worker lives in its own git worktree, which has no
`build.ninja`, no `objdiff.json` and no `build/RMHE08/` — and `unitutil` resolves all three from its own
location, so `ninja build/RMHE08/src/<unit>.o` and `mt.py` simply do not work there. Two further traps it
closes: MWCC writes the object into the *directory* named by `-o` (a hand-written command can silently
compile nothing), and the filesystem's one-second mtime granularity makes a recompile of an unchanged file
look like "no change" to anything that caches on (mtime, size).

What it does instead:

* resolves **MAIN** with `git worktree list --porcelain` and takes the toolchain, the include path and the
  *target* object from there;
* takes the real command line from MAIN's ninja (`ninja -t commands`), rewrites the three paths that must
  change (the source, the `-o` directory, the `-i` search path), and runs it with MAIN as cwd;
* puts the worktree's `-i` directories **first** and points every one of MAIN's at the worktree's copy of
  that directory, so a worker's edit to an existing shared header is the header that gets compiled (see
  `order_includes`; appending them - the old behaviour - left MAIN's copy first and silently measured the
  wrong source, which is what the `eft004` round had to work around with a scratch measurer);
* deletes the object first, then asserts the file exists and its mtime moved — a stale object is impossible;
* prints the two object paths, the section sizes, and can measure a symbol **with the same objdiff code
  path the official report uses** (`report generate` on a one-unit project), so the number equals
  `build/RMHE08/report.json`'s `fuzzy_match_percent` for the same object.

    python tools/units/recompile.py <unit> [--measure <symbol>] [--json] [--dry-run] [--selftest]

The measurement trap this closes (`--measure` used to lie by ~0.36 points on `RSO/runtime`, which sent a
worker chasing a regression that did not exist): objdiff-cli's explicit `diff` mode is **not** the
report's metric. Two differences compound - `diff` defaults `functionRelocDiffs` to `data_value` while
`report generate` defaults to `none` (so relocation-only differences count as mismatches), and even at the
same setting the diff JSON's per-symbol `match_percent` is a different normalisation from the report's
`fuzzy_match_percent`. `report generate` over a one-unit project is the only path that is the report by
construction, and it costs ~0.04 s. `--measure` calls the *same* `unitutil.report_measure` primitive
`tools/units/measure.py` scores a whole unit with, so there is one implementation of the metric and the
two fronts cannot drift.

**A proposal unit measures too** (CLAUDE.md, "the proposal-unit measurement gap"). A worker registers a
fresh proposal in its own worktree first (`configure.py` + `splits.txt`, per the brief) and MAIN has
neither a ninja rule nor a split object for the range until that registration lands. That used to be the
end of `--measure`; three workers hand-built a harness each (borrow a sibling's command line, score
against the retired `auto_*_text.o`, one spent 87 turns on it). Both halves are now the tool's own path,
and neither MAIN's config nor the worktree is written:

* the **command line** comes from MAIN's ninja (registered), the worktree's ninja (if the worker
generated one), or - last - a registered sibling in the *same `config.libs` block* of the worktree's
`configure.py`, with only the source, the `-o` directory and the `-lang` token pointed at this unit.
Those are the flags `project.py` emits for that lib, not a hand-rolled approximation;
* the **target object** is resolved from the invocation's own tree outward (`resolve_target`): the
worktree's split object first, then MAIN's, then the retired `auto_*_text.o` that owns the symbol's address
in whichever tree has it - the same original bytes the split will put in the registered object
(`auto_<symbol[:20]>_text.o` for a single symbol, else the `auto_<nn>_<address>_text` run that covers it).

A registered unit run from MAIN takes exactly the path it took before (MAIN's rule, MAIN's object). A unit
run from a worktree that has its own copy - the filed double-take - takes **that** copy, and the CLI prints
the resolved absolute path with its kind (`[worktree-split]`, `[registered]`, `[auto-fallback]`) **and the
tree it came from**, so a measurement is never ambiguous about which tree it came from. The score is still
`report generate`'s `fuzzy_match_percent`.

**`--measure` prints the provenance of the number it reports.** The tree the invocation resolved in (its
cwd), the target object it compared against (**path and mtime**), the object it compiled (**path and
mtime**), and the map - then a `WARNING` when the target is MAIN's while the cwd is a worktree, because
that score is MAIN's and a reader must not have to infer it from an absolute path. `--json` carries the same
facts under `provenance`. This is the half a reader can check *after* the fact; `split_staleness` is the half
that refuses before it. It re-derives nothing: `compile_unit` already deletes the object before compiling
and asserts it reappears, so the printed `compiled_mtime` is a provably fresh file.

**A stale split is refused, not silently measured (F40).** Preferring this tree's object is only safe while
this tree's split actually reflects its own `symbols.txt`/`splits.txt`/DOL. A lane that edits its `splits.txt`
(a seam re-draw, a new registration) and has **not** re-split still has the previous build's object on disk,
so the "this tree's copy" the resolution just preferred is the *old range's* bytes - and MAIN's copy is the
same old range, so falling back is not a fix either. `split_staleness` reuses the seeder's own guard
(`claims._build_is_current`, the one `slots.verify` uses) and then asks which split input is both newer than
this tree's `build/RMHE08/config.json` **and** an uncommitted edit to this tree (`git diff --quiet HEAD`),
and the CLI refuses with the file and both mtimes named. The dirty test is load-bearing, not decoration: a
fresh worktree's tracked files are all written at checkout time while `build/` keeps MAIN's mtimes, so
`_build_is_current` is False in **every** fresh worktree and a pure-mtime rule would refuse every measurement.
`--allow-stale-split` is the deliberate override.

**The map follows the invocation too (F43).** The fallback locates the retired `auto_*text.o` by *address*,
and the address comes from `config/RMHE08/symbols.txt`. Reading MAIN's copy alone made the tool refuse a
branch that had renamed a symbol - "a symbol this branch renamed has no entry there" - because MAIN has
never carried the new spelling. `resolve_map` applies `resolve_target`'s discipline to the map: the
invocation tree's copy first, MAIN's as the fallback, and the address lookup merges both (the branch
supplies the new name, MAIN the old one that the retired object is named after). The CLI prints the map it
read with its kind, so a measurement is unambiguous about the map as well as the object.

`<unit>` is the path from the repository root, e.g. `Pl/pl_act`, `main.cpp`, `auto/80040598_fn_80040598`.
"""

from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import argparse
import json
import os
import subprocess
import sys
import time
from tools.lib.git import Git

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))

import unitutil  # noqa: E402
from tools.lib import report as _report  # noqa: E402  (the metric and the diagnostic rows)
from tools.lib import units as _units  # noqa: E402  (spellings, the compile command, the compile, the target)

SRC_EXT = _units.SOURCE_EXT


def git(args: list[str], cwd: str, check: bool = True) -> str:
    out = Git(cwd).run(*args)
    if check and out.returncode != 0:
        raise SystemExit("git %s failed in %s: %s" % (" ".join(args), cwd, out.stderr.strip()))
    return out.stdout


def worktree_root(start: str | None = None) -> str:
    """The tree the caller is in - resolved from the *cwd*, never from this file's location.

    Invoking `MAIN/tools/units/recompile.py` from inside a worktree must compile the worktree's source, not
    MAIN's; that is the whole point of 7.15.
    """
    return git(["rev-parse", "--show-toplevel"], start or os.getcwd()).strip()


def main_root(current: str) -> str:
    """MAIN's worktree path - the tree that owns the toolchain, the ninja graph and the split objects.

    Callers that genuinely need MAIN: `recompile.py`/`measure.py` take the compile **command line**, the
    toolchain and the *target* split object from MAIN (a worktree has no `build.ninja` of its own), while
    the source, `-o` directory and `-i` order are the caller's tree. The gate (`land.py`) runs from MAIN
    and does not call this.

    Resolution is by `git rev-parse --git-common-dir`, whose parent is MAIN by construction, **not** the
    first `git worktree list` entry - that order is registration order, and a tool that pinned MAIN by it
    could hand a lane a slot's tree (or, worse, read a tree it was not editing). Falls back to the first
    worktree entry only when git cannot answer, so a non-git copy still works.
    """
    common = git(["rev-parse", "--path-format=absolute", "--git-common-dir"], current, check=False)
    common = (common or "").strip()
    if common and os.path.basename(common.replace("\\", "/")) == ".git":
        main = os.path.dirname(os.path.abspath(common))
        if os.path.exists(os.path.join(main, "configure.py")):
            return main
    out = git(["worktree", "list", "--porcelain"], current)
    paths = [line.split(" ", 1)[1] for line in out.splitlines() if line.startswith("worktree ")]
    return paths[0] if paths else current


def main_worktree_list(current: str) -> list[dict]:
    out = git(["worktree", "list", "--porcelain"], current)
    entries, cur = [], None
    for line in out.splitlines():
        if line.startswith("worktree "):
            cur = {"path": line.split(" ", 1)[1], "branch": None, "head": None}
            entries.append(cur)
        elif line.startswith("branch ") and cur is not None:
            cur["branch"] = line.split(" ", 1)[1].replace("refs/heads/", "")
        elif line.startswith("HEAD ") and cur is not None:
            cur["head"] = line.split(" ", 1)[1]
    return entries


unit_source = _units.with_ext
normalize_unit = _units.normalize


def resolve_unit_source(unit: str, wt: str = None, main: str = None, source: str = None) -> str:
    """The unit's source spelling with its real extension (`lib.units.source_spelling`)."""
    return _units.source_spelling(unit, (wt, main), source)


def _ninja_compile_lines(main: str, unit: str, runner=subprocess.run):
    """(target, mwcceppc lines, completed process) of MAIN's ninja for a unit, without raising."""
    target = _units.ninja_target(unit)
    lines, p = _units.ninja_lines(main, target, runner)
    return target, lines, p


ninja_command = _units.ninja_command
_unit_stem = _units.stem
retarget = _units.retarget
lib_block = _units.lib_block
sibling_for = _units.sibling_for
unit_tokens = _units.unit_tokens
OBJECT_HELPERS = _units.OBJECT_HELPERS
retarget_object_helpers = _units.retarget_object_helpers


# kept for callers outside this module (the name the fix first shipped under)
retarget_objalign = retarget_object_helpers


rewrite = _units.rewrite
INCLUDE_FLAG = _units.INCLUDE_FLAG
include_pairs = _units.include_pairs
order_includes = _units.order_includes
source_path = _units.source_path


def _stamp(seconds: float) -> str:
    """A local wall-clock stamp for a file's mtime - the form the refusal messages name."""
    return time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(seconds))


def object_stamp(path: str) -> str:
    """A file's mtime as the wall-clock stamp the provenance block prints, or `MISSING`."""
    try:
        return _stamp(os.path.getmtime(path))
    except OSError:
        return "MISSING"


def provenance(wt: str, main: str, result: dict) -> dict:
    """The facts a `--measure` number rests on: the tree it was resolved in and the two objects behind it.

    A score is only as good as the tree it came from and the objects it compared.  The failure this closes
    cost a lane hours: standing in a slot, `--measure` still returned a number read from **MAIN's** object
    (before `2c10d0473`); the number looked like a measurement and was not one, and nothing in the output
    said which tree it came from.  `target_tree` was the first half of that fix; this is the half a reader
    can check *after the fact* - the absolute invocation tree, the target's path **and mtime**, and the
    object this run compiled, **and its mtime**.

    This deliberately re-derives nothing.  `compile_unit` already deletes the object before compiling and
    asserts it reappears, so `compiled_mtime` describes a provably fresh file; `split_staleness` already
    refuses a split older than this tree's own edited map/splits; `resolve_target` already prefers this
    tree's object.  Provenance only *prints* those outcomes so they can be quoted with the score.
    """
    return {
        # `worktree_root()` resolves from the cwd (`git rev-parse --show-toplevel`), so this IS the tree the
        # invocation is in - printing it is what makes "the number is this tree's" checkable.
        "invoked_tree": wt,
        "main": main,
        "target_tree": result.get("target_tree"),
        "compiled_object": result.get("object"),
        "compiled_mtime": object_stamp(result.get("object") or ""),
        "target_object": result.get("target"),
        "target_mtime": object_stamp(result.get("target") or ""),
        "symbol_map": result.get("symbol_map"),
        "symbol_map_tree": result.get("symbol_map_kind"),
    }


def provenance_lines(prov: dict) -> list[str]:
    """The paste-able provenance block - every value measured here, none promised.

    The `WARNING` line is the one that matters: when the invocation is a worktree but the target object is
    MAIN's, the score is MAIN's, and the reader must not have to infer that from an absolute path.
    """
    where = {"worktree": "this tree", "main": "MAIN"}.get(prov.get("target_tree"), prov.get("target_tree"))
    lines = ["  provenance (the tree, and the two objects this number came from)"]
    lines.append("    invoked    %s   (this invocation's cwd; MAIN is %s)"
                 % (prov["invoked_tree"], prov["main"]))
    lines.append("    compiled   %s   (mtime %s)" % (prov["compiled_object"], prov["compiled_mtime"]))
    lines.append("    target     %s   (mtime %s%s)" % (prov["target_object"], prov["target_mtime"],
                                                        "; %s" % where if where else ""))
    lines.append("    map        %s   [%s]" % (prov["symbol_map"], prov["symbol_map_tree"]))
    if prov.get("target_tree") == "main" and os.path.normcase(prov["invoked_tree"]) != os.path.normcase(prov["main"]):
        lines.append("    WARNING: the target object is MAIN's, not this tree's - the score is MAIN's; "
                     "re-split this tree (`ninja build/RMHE08/config.json`) to score your own")
    return lines


object_is_fresh = _units.object_is_fresh
section_sizes = _units.section_sizes


MIN_PROJECT_VERSION = unitutil.MIN_PROJECT_VERSION


def measure(target: str, base: str, symbol: str, objdiff: str, tmpdir: str,
            unit: str = None, runner=subprocess.run) -> dict:
    """The official score of one symbol (`lib.report.symbol_score`) plus the diagnostic rows behind it;
    the positional diff value is kept as `diff_match_percent` and is never the score."""
    result = _report.symbol_score(target, base, symbol, unit, tmpdir, objdiff=objdiff, cwd=unitutil.ROOT,
                                  runner=runner)
    if "error" in result:
        return result
    rows = diff_rows(target, base, symbol, objdiff, tmpdir, runner=runner)
    if "error" in rows:
        result["rows_error"] = rows["error"]
        return result
    result["diff_match_percent"] = rows.get("diff_match_percent")
    if result.get("target_size") is None:
        result["target_size"] = rows.get("target_size")
    result["candidate_size"] = rows.get("candidate_size")
    result["paired"] = rows.get("paired")
    result["json"] = rows.get("json")
    return result


diff_rows = _report.diff_rows
SWITCH_PREFIXES = _units.SWITCH_PREFIXES
is_switch = _units.is_switch
absolutize = _units.absolutize


# ---------------------------------------------------------------------------------------------------
# The target object for a proposal unit: MAIN's retired per-symbol/per-run `auto_*_text.o`
# ---------------------------------------------------------------------------------------------------
# A registered unit measures against MAIN's `build/RMHE08/obj/<unit>.o` (the split object). A proposal has
# no such object until the registration lands, but MAIN *does* still build the range as the `auto_*_text`
# split objects its previous split produced, with the same original bytes. That is the honest fallback, and
# it is what each stuck worker re-derived by hand.

SYMBOLS_REL = _units.SYMBOLS_REL
SPLITS_REL = _units.SPLITS_REL
AUTO_RUN_RE = _units.AUTO_RUN_RE
same_tree = _units.same_tree
resolve_map = _units.resolve_map
text_symbol_addresses = _units.text_symbol_addresses
auto_text_runs = _units.auto_text_runs
symbol_addresses = _units.symbol_addresses
retired_object_dirs = _units.retired_object_dirs
proposal_target = _units.proposal_target


def object_has_symbol(obj: str, symbol: str) -> bool:
    """Whether an object defines `symbol` at all - the check that separates "nothing to pair" from a
    renamed symbol, both of which `report generate` answers with a null `fuzzy_match_percent`."""
    try:
        _secs, syms = unitutil.read_elf(obj)
    except Exception:
        return False
    return any(s[0] == symbol for s in syms)


target_rel = _units.target_rel


# ---------------------------------------------------------------------------------------------------
# The invocation tree's split must postdate the tree's own map/splits/DOL, or every object it holds
# (and MAIN's for the same range) is the previous build's.
# ---------------------------------------------------------------------------------------------------
# The inputs dtk's split reads.  Kept in step with `claims._build_is_current` (the seeder's own guard,
# which `slots.verify` uses) - the selftest asserts claims reacts to each of them, so a change there that is
# not mirrored here is caught rather than silently leaving the refusal message short one file.  Relative
# paths, so the same tuple reads both roots.
SPLIT_INPUTS = (os.path.join("config", "RMHE08", "config.yml"),
                os.path.join("config", "RMHE08", "symbols.txt"),
                os.path.join("config", "RMHE08", "splits.txt"),
                os.path.join("orig", "RMHE08", "sys", "main.dol"),
                os.path.join("orig", "RMHE08", "files", "mh3.sel"))

# The deliberate override for `--measure` when the caller knows the stale split cannot touch its unit.
STALE_SPLIT_FLAG = "--allow-stale-split"


def _claims():
    """`claims` imported late: it imports this module at import time, so a top-level import would cycle."""
    from units import claims
    return claims


def git_dirty(wt: str, rel: str, runner=subprocess.run) -> bool:
    """Whether this tree carries an **uncommitted** change to the tracked path `rel` (worktree vs HEAD).

    A split input being newer than the split is by itself not evidence of a doubt: a fresh worktree writes
    every tracked file at checkout time, so all of them are newer than the `build/` tree seeded from MAIN.
    This is what separates "the checkout wrote the file" from "this lane edited the file", and it is the
    only signal that survives MAIN moving under a lane (the branch's own files stay byte-equal to its base,
    while a lane's edit does not).  `git diff --quiet` answers 0 (clean) or 1 (differs); any other status
    means git could not answer, which is not evidence of a difference - the guard then keeps today's
    behaviour rather than manufacturing a doubt out of a question git never answered.
    """
    p = runner(["git", "-C", wt, "diff", "--quiet", "HEAD", "--", rel],
               capture_output=True, text=True, encoding="utf-8", errors="replace")
    return p.returncode == 1


def split_staleness(wt: str, main: str, dirty=None):
    """(stale, lines) - whether this tree's split can back a `--measure` number, and why not.

    Three gates, cheapest first, and every one of them must agree before the refusal fires:

    1. **the trees differ** - run from MAIN the resolved target is MAIN's own object, which is the path a
       registered unit has always taken; nothing changes there.
    2. **the seeder's guard** - `claims._build_is_current(wt, wt)`.  This is the existing staleness rule
       (`slots.verify`, `seed_worktree_build`) rather than a second one that can drift from it.
    3. **this tree's own edit** - `config.json` predates a split input that `git diff` says this tree has
       changed.  Gate 2 alone is not enough: it is False in every fresh worktree (checkout mtimes vs the
       seeded `build/`), and pure mtimes cannot tell a checkout artefact from a lane's edit or from MAIN
       moving under the lane.  The dirty test can, so it is what the refusal actually rests on.

    No `build/RMHE08/config.json` at all means this tree has no split of its own to be stale - that is a
    fresh worktree's normal state and the resolution below deliberately falls back to MAIN there.
    """
    if dirty is None:
        dirty = git_dirty
    if same_tree(wt, main):
        return False, []
    cfg = os.path.join(wt, "build", "RMHE08", "config.json")
    if not os.path.isfile(cfg):
        return False, []
    if _claims()._build_is_current(wt, wt):
        return False, []
    try:
        cfg_m = os.path.getmtime(cfg)
    except OSError:
        return False, []
    lines = []
    for rel in SPLIT_INPUTS:
        try:
            m = os.path.getmtime(os.path.join(wt, rel))
        except OSError:
            continue
        if m > cfg_m and dirty(wt, rel):
            lines.append("%s (edited %s, split %s)" % (rel, _stamp(m), _stamp(cfg_m)))
    return (True, lines) if lines else (False, [])


def refuse_if_split_stale(wt: str, main: str, allow_stale: bool = False, dirty=None):
    """Raise with the evidence unless this tree's split provably reflects its own map/splits/DOL.

    Returns (False, []) when the split is usable (or `allow_stale` skips the gate).  A refusal names every
    input that is both newer than the split and an uncommitted edit here, so the remedy is one command
    (`ninja build/RMHE08/config.json`) and never a guess about which file moved.
    """
    if allow_stale:
        return False, []
    stale, lines = split_staleness(wt, main, dirty=dirty)
    if stale:
        raise SystemExit(
            "REFUSED: %s/build/RMHE08/config.json is older than map/split input(s) this tree has edited, "
            "so the split object here is the *previous* range - measuring against it (or falling back to "
            "MAIN, which holds the same previous split) would print a number that looks like a "
            "measurement and is not:\n  %s\n"
            "  re-split this tree first: ninja build/RMHE08/config.json\n"
            "  or measure deliberately against the stale object: %s"
            % (wt, "\n  ".join(lines), STALE_SPLIT_FLAG))
    return stale, lines


def target_tree(path: str, wt: str, main: str) -> str:
    """Which tree a resolved target object lives in - what the CLI labels the `target` line with."""
    p = os.path.normcase(os.path.abspath(path))
    for name, root in (("worktree", wt), ("main", main)):
        root = os.path.normcase(os.path.abspath(root))
        if p == root or p.startswith(root + os.sep):
            return name
    return os.path.dirname(os.path.abspath(path))


resolve_target = _units.resolve_target


# the name this shipped under before item B (`resolve_target` follows the invocation; this searched MAIN
# only); kept so an out-of-tree caller does not break, and so the selftest can pin the old contract.
def measure_target(main: str, unit: str, symbol: str):
    return resolve_target(main, main, unit, symbol)


compile_unit = _units.compile


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("unit", nargs="?", help="unit path from the repository root, e.g. Pl/pl_act")
    ap.add_argument("--main", default=None, help="main worktree (default: resolved with git)")
    ap.add_argument("--source", default=None,
                    help="override the unit's source path (repo-relative, e.g. src/NHTTP/NHTTP_bgnend.c)")
    ap.add_argument("--measure", default=None, help="symbol to diff against the target object afterwards")
    ap.add_argument(STALE_SPLIT_FLAG, action="store_true", dest="allow_stale_split",
                    help="measure even when this tree's split is older than its own edited map/splits")
    ap.add_argument("--json", action="store_true")
    ap.add_argument("--dry-run", action="store_true", help="print the command, compile nothing")
    ap.add_argument("--selftest", action="store_true", help="run the self-test and exit")
    args = ap.parse_args()
    if args.selftest:
        import recompile_selftest
        return recompile_selftest.main()
    if not args.unit:
        ap.error("a unit is required (or --selftest)")

    wt = worktree_root()
    main_wt = args.main or main_root(wt)
    unit = resolve_unit_source(args.unit.strip("/"), wt, main_wt, args.source)
    if args.measure and not args.dry_run:
        # before the compile: a stale split is refused in ~0 s rather than after a wasted one.  Only a
        # real measurement needs it - a plain recompile does not read a target object, and `--dry-run`
        # prints a command without producing a number.
        refuse_if_split_stale(wt, main_wt, allow_stale=args.allow_stale_split)
    map_path, map_kind = resolve_map(wt, main_wt)
    tokens, cmd_source = unit_tokens(main_wt, wt, unit)
    result = compile_unit(unit, main_wt, wt, dry_run=args.dry_run, tokens=tokens)
    result.update({"unit": unit, "worktree": wt, "main": main_wt,
                   "symbol_map": map_path, "symbol_map_kind": map_kind})
    if cmd_source != "main":
        # the registered path must read exactly as it did before; a proposal says where its flags came from
        result["command_source"] = cmd_source
    result["target"] = os.path.join(main_wt, target_rel(unit))

    if args.measure and result.get("compiled"):
        # resolution follows THIS invocation's tree first, then MAIN; `measure.py` calls the same function
        target, target_kind, target_note = resolve_target(wt, main_wt, unit, args.measure)
        result["target"] = target
        result["target_kind"] = target_kind
        result["target_note"] = target_note
        result["target_tree"] = target_tree(target, wt, main_wt)
        if target_kind == "missing":
            result["measure"] = {"symbol": args.measure, "error": target_note}
        else:
            result["measure"] = measure(target, result["object"], args.measure, _report.objdiff_cli(wt, main_wt),
                                        unitutil.session_tmpdir(), unit=unit)
            m = result["measure"]
            if target_kind == "auto-fallback" and "error" not in m and m.get("match_percent") is None:
                # report pairs by name and answers a null (not an error) when pairing fails; say which of
                # the two causes it is, because the message is what tells the worker where to look
                if not object_has_symbol(result["object"], args.measure):
                    m["error"] = ("%s does not define %s (nothing to pair) - the retired object defines it "
                                  "at that address, so the unit's own source is what is short"
                                  % (os.path.basename(result["object"]), args.measure))
                else:
                    m["error"] = ("no pairing: %s defines %s, but %s spells that address differently - "
                                  "the report pairs symbols by name, which a renamed symbol breaks"
                                  % (os.path.basename(result["object"]), args.measure,
                                     os.path.basename(target)))
                result.pop("match_percent", None)

    if args.measure:
        # the provenance of the number (tree + the two objects + mtimes); recorded for `--json` too, so a
        # caller that quotes a score quotes what it was measured against.
        result["provenance"] = provenance(wt, main_wt, result)

    if args.json:
        print(json.dumps(result, indent=2))
        return 0
    if args.dry_run:
        print("would run (cwd %s):\n  %s" % (main_wt, " ".join('"%s"' % t if " " in t else t for t in result["command"])))
        print("object  -> %s" % result["object"])
        return 0
    if not result.get("compiled"):
        print("FAILED: %s\n%s" % (result["unit"], result.get("error", "")))
        return 1
    print("compiled %s" % result["unit"])
    print("  object  %s  (%d bytes, fresh=%s)" % (result["object"], result["bytes"], result["fresh"]))
    kind = result.get("target_kind")
    # name the tree explicitly: the path already does, but a lane reading `[registered]` cannot tell
    # whose registered tree it is, which is the filed F40 double-take.
    where = {"worktree": "this tree", "main": "MAIN"}.get(result.get("target_tree"),
                                                        result.get("target_tree"))
    print("  target  %s%s%s" % (result["target"], "  [%s]" % kind if kind else "",
                                 "  (%s)" % where if where else ""))
    if result.get("target_note"):
        print("          %s" % result["target_note"])
    # the map the address lookup used - without this line a measurement is ambiguous about its map, which
    # is the second half of F43 (the object's tree was already printed above)
    print("  map     %s  [%s]" % (result["symbol_map"], result["symbol_map_kind"]))
    if args.measure:
        # only `--measure` sets this key; printing it unconditionally crashed a plain recompile (exit 1,
        # the code a failed compile also returns) *after* the object had been written
        for line in provenance_lines(result["provenance"]):
            print(line)
    if kind == "auto-fallback":
        print("  [fallback] MAIN has no split object for %s yet; the score is the one the registered unit"
              " will report (same original bytes)" % result["unit"])
    elif kind == "missing":
        print("  [no target] %s" % result["target_note"])
    if result.get("command_source"):
        print("  command  %s" % result["command_source"])
    for name, size in sorted((result.get("sections") or {}).items()):
        print("  %-12s 0x%X" % (name, size))
    if "measure" in result:
        m = result["measure"]
        if "error" in m:
            print("  measure %s: ERROR %s" % (m.get("symbol"), m["error"][:200]))
        else:
            print("  measure %s: %s%% (official report metric; target %s B, ours %s B, paired=%s)"
                  % (m["symbol"], m.get("match_percent"), m.get("target_size"), m.get("candidate_size"),
                     m.get("paired")))
            dmp = m.get("diff_match_percent")
            if isinstance(dmp, (int, float)) and isinstance(m.get("match_percent"), (int, float)) \
                    and abs(dmp - m["match_percent"]) > 1e-6:
                # objdiff-cli's explicit diff mode is a different normalisation; say so, so nobody quotes it
                print("    (objdiff's positional diff reports %s%% for the same object - not the report metric)"
                      % dmp)
    print("\nnext: python .claude/skills/mwcc-unit-matching/scripts/mt.py diff -u %s <symbol>   (in MAIN)"
          "\n      or: python tools/units/recompile.py %s --measure <symbol>" % (result["unit"], result["unit"]))
    if not result.get("fresh"):
        print("WARNING: the object's mtime did not move - treat any measurement as stale", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
