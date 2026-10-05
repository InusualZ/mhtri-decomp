"""Compile one unit without ninja from any worktree with MAIN's real command line, prove the object fresh, and
`--measure` one symbol with the official metric. Spec: docs/tools/spec/recompile.md.
CLI: python tools/units/recompile.py <unit> [--measure <symbol>] [--source <path>] [--main <tree>] [--allow-stale-split] [--json] [--dry-run] | --selftest."""

from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import argparse
import json
import os
import subprocess
import sys
import time

from tools.lib import artifacts as _artifacts
from tools.lib import repo as _repo
from tools.lib import report as _report  # the metric and the diagnostic rows
from tools.lib import units as _units  # spellings, the compile command, the compile, the target
from tools.lib.binary.elf import Elf
from tools.lib.git import Git

SRC_EXT = _units.SOURCE_EXT


def git(args: list[str], cwd: str, check: bool = True) -> str:
    out = Git(cwd).run(*args)
    if check and out.returncode != 0:
        raise SystemExit("git %s failed in %s: %s" % (" ".join(args), cwd, out.stderr.strip()))
    return out.stdout


def worktree_root(start: str | None = None) -> str:
    """The tree the caller is in - resolved from the *cwd*, never from this file's location.

    Invoking `MAIN/tools/units/recompile.py` from inside a worktree must compile the worktree's source, not
    MAIN's; that is the whole point of 7.15 (`lib.repo.worktree_root`).
    """
    return _repo.worktree_root(start)


def main_root(current: str) -> str:
    """MAIN's worktree path - the tree that owns the toolchain, the ninja graph and the split objects
    (`lib.repo.main_checkout`: the git common dir's parent, never the first worktree entry unless git
    cannot answer)."""
    return _repo.main_checkout(current)


def main_worktree_list(current: str) -> list[dict]:
    """`[{path, branch, head}]` of `git worktree list` (`lib.git.Git.worktree_list`)."""
    return [{"path": w.path, "branch": w.branch, "head": w.head}
            for w in Git(current).worktree_list()]


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


MIN_PROJECT_VERSION = _report.MIN_PROJECT_VERSION


def measure(target: str, base: str, symbol: str, objdiff: str, tmpdir: str,
            unit: str = None, runner=subprocess.run, cwd: str | None = None) -> dict:
    """The official score of one symbol (`lib.report.symbol_score`) plus the diagnostic rows behind it;
    the positional diff value is kept as `diff_match_percent` and is never the score."""
    result = _report.symbol_score(target, base, symbol, unit, tmpdir, objdiff=objdiff, cwd=cwd,
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
        return any(s.name == symbol and s.shndx for s in Elf.read(obj).symbols)
    except Exception:
        return False


target_rel = _units.target_rel


# ---------------------------------------------------------------------------------------------------
# The invocation tree's split must postdate the tree's own map/splits/DOL, or every object it holds
# (and MAIN's for the same range) is the previous build's.
# ---------------------------------------------------------------------------------------------------
# The inputs dtk's split reads: the registry's one list (`lib.artifacts.SPLIT_INPUTS`, which
# `lib.lanes.seed.build_is_current` and `slots.verify` read too). Relative paths, so the same tuple reads both roots.
SPLIT_INPUTS = _artifacts.SPLIT_INPUTS

# The deliberate override for `--measure` when the caller knows the stale split cannot touch its unit.
STALE_SPLIT_FLAG = "--allow-stale-split"


def _seed():
    """The seeder's staleness guard (`lib.lanes.seed.build_is_current`), imported where it is used."""
    from tools.lib.lanes import seed
    return seed


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
    2. **the seeder's guard** - `lib.lanes.seed.build_is_current(wt, wt)`.  This is the existing staleness rule
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
    if _seed().build_is_current(wt, wt):
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


def refuse_if_split_stale(wt: str, main: str, allow_stale: bool = False, dirty=None, runner=None):
    """Raise with the evidence unless this tree's split provably reflects its own map/splits/DOL.

    Returns (False, []) when the split is usable (or `allow_stale` skips the gate).  A refusal names every
    input that is both newer than the split and an uncommitted edit here, so the remedy is one command
    (`ninja build/RMHE08/config.json`) and never a guess about which file moved.
    """
    if allow_stale:
        return False, []
    # the freshness policy (`lib.artifacts`): refuse by default, FRESH=warn skips the gate like the flag, and
    # FRESH=auto re-splits this tree (the registry's `split` refresh, ~2 s) and re-checks
    policy = _artifacts.effective_policy(None, "refuse", runner)
    if policy == "warn":
        return False, []
    stale, lines = split_staleness(wt, main, dirty=dirty)
    if stale and policy == "auto":
        _artifacts.refresh("split", _artifacts.Context(wt, runner=runner))
        stale, lines = split_staleness(wt, main, dirty=dirty)
    if stale:
        raise SystemExit(
            "REFUSED: %s/build/RMHE08/config.json is older than map/split input(s) this tree has edited, "
            "so the split object here is the *previous* range - measuring against it (or falling back to "
            "MAIN, which holds the same previous split) would print a number that looks like a "
            "measurement and is not:\n  %s\n"
            "  re-split this tree first: ninja build/RMHE08/config.json (or FRESH=auto lets this tool do it)\n"
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
        from tools.units import recompile_selftest
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
    # the graph's flags are only as new as its last `python configure.py`; this tree's configure.py decides
    tokens, flag_notes = _units.reconcile_flags(tokens, wt, unit)
    result = dict(compile_unit(unit, main_wt, wt, dry_run=args.dry_run, tokens=tokens))
    result.update({"unit": unit, "worktree": wt, "main": main_wt,
                   "symbol_map": map_path, "symbol_map_kind": map_kind})
    if flag_notes:
        result["flags_reread"] = flag_notes
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
                                        _repo.session_tmpdir(), unit=unit, cwd=wt)
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
        for note in result.get("flags_reread") or ():
            print("[flags] %s" % note)
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
    for note in result.get("flags_reread") or ():
        print("  [flags] %s" % note)
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
