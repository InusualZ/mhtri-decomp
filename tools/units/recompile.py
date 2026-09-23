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

    python tools/units/recompile.py <unit> [--measure <symbol>] [--json] [--dry-run] [--print-command]

The measurement trap this closes (`--measure` used to lie by ~0.36 points on `RSO/runtime`, which sent a
worker chasing a regression that did not exist): objdiff-cli's explicit `diff` mode is **not** the
report's metric. Two differences compound - `diff` defaults `functionRelocDiffs` to `data_value` while
`report generate` defaults to `none` (so relocation-only differences count as mismatches), and even at the
same setting the diff JSON's per-symbol `match_percent` is a different normalisation from the report's
`fuzzy_match_percent`. `report generate` over a one-unit project is the only path that is the report by
construction, and it costs ~0.04 s.

`<unit>` is the path from the repository root, e.g. `Pl/pl_act`, `main.cpp`, `auto/80040598_fn_80040598`.
"""

from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))

import unitutil  # noqa: E402

SRC_EXT = (".c", ".cpp", ".cp", ".cxx", ".cc")


def git(args: list[str], cwd: str, check: bool = True) -> str:
    out = subprocess.run(["git", *args], cwd=cwd, capture_output=True, text=True, errors="replace")
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
    """The first worktree git lists - the main one. `git worktree list --porcelain` prints it first."""
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


def unit_source(unit: str) -> str:
    return unit if unit.endswith(SRC_EXT) else unit + ".cpp"


def ninja_command(main: str, unit: str, runner=subprocess.run) -> list[str]:
    """The exact compile command MAIN's ninja would run, as tokens."""
    target = "build/RMHE08/src/" + os.path.splitext(unit_source(unit))[0] + ".o"
    p = runner(["ninja", "-t", "commands", target], cwd=main, capture_output=True, text=True, errors="replace")
    lines = [l for l in (p.stdout or "").splitlines() if "mwcceppc" in l]
    if not lines:
        raise SystemExit("could not get the compile command for %s from ninja in %s:\n%s%s"
                         % (target, main, p.stdout, p.stderr))
    return unitutil.unquote(lines[-1].split())


def rewrite(tokens: list[str], unit: str, main: str, wt: str) -> tuple[list[str], str]:
    """Point the command at the worktree's source and object, and at its own headers first.

    Returns (tokens, object_path). The three rewrites are the source path, the `-o` directory (MWCC's `-o`
    is a directory - the object's *name* comes from the source) and the include search path
    (`order_includes`).
    """
    rel_src = os.path.join("src", *unit_source(unit).split("/"))
    wt_src = os.path.join(wt, rel_src)
    out: list[str] = []
    obj_dir = None
    i = 0
    while i < len(tokens):
        tok = tokens[i]
        if tok in ("-o",):
            out.append(tok)
            i += 1
            if i < len(tokens):
                obj_dir = os.path.join(wt, tokens[i].lstrip("./\\"))
                out.append(obj_dir)
        elif tok == "-c":
            out.append(tok)
            i += 1
            if i < len(tokens):
                out.append(wt_src)
        else:
            out.append(tok)
        i += 1
    if obj_dir is None:
        raise SystemExit("no -o in the command line - refusing to guess where the object goes")
    out = order_includes(out, main, wt)
    obj_path = os.path.join(obj_dir, os.path.splitext(os.path.basename(wt_src))[0] + ".o")
    return out, obj_path


# The flag configure.py's cflags use for a search directory; the include *values* are relative to MAIN
# (`-i include`, `-i build/RMHE08/include`), which is exactly why the order matters here.
INCLUDE_FLAG = "-i"


def include_pairs(tokens: list[str]) -> list[tuple[int, str]]:
    """[(index of the flag, its directory)] for every `-i <dir>` in a command line."""
    pairs: list[tuple[int, str]] = []
    i = 0
    while i < len(tokens):
        if tokens[i] == INCLUDE_FLAG and i + 1 < len(tokens):
            pairs.append((i, tokens[i + 1]))
            i += 2
        else:
            i += 1
    return pairs


def order_includes(tokens: list[str], main: str, wt: str) -> list[str]:
    """Rebuild the command line's `-i` list so the worktree's headers always win.

    MWCC searches `-i` directories **in the order given**, and the command line ninja hands over carries
    MAIN's directories as relative paths (`-i include -i build/RMHE08/include`) which the compile resolves
    against its cwd - MAIN. Appending the worktree's `include/` (what this used to do) therefore left MAIN's
    copy of every shared header first: a worker's edit to an *existing* header (`include/nw4r/math.h`,
    `include/ef.h`) was shadowed, the compile succeeded, and the measurement was silently of MAIN's source -
    a lower score that reads as a matching problem.

    Two changes: the worktree's own include directories go **first**, and every one of MAIN's directories is
    pointed at the worktree's copy of it when the worktree has one, so the worktree is self-sufficient
    (`docs/plan.md` §5.1) and no MAIN path can shadow a worktree file. A directory only MAIN has (the
    generated `build/RMHE08/include`, the toolchain) keeps MAIN's absolute path.

    Pure in the sense that matters for testing: it only reads the two trees' directory listings, never the
    compiler, so the ordering can be asserted without a build.
    """
    pairs = include_pairs(tokens)
    dirs: list[str] = []
    seen: set[str] = set()

    def add(path: str) -> None:
        key = os.path.normcase(os.path.abspath(path))
        if key not in seen:
            seen.add(key)
            dirs.append(path)

    # 1. the worktree's own headers, before anything MAIN carries - the whole point
    for rel in ("include", os.path.join("build", "RMHE08", "include")):
        cand = os.path.join(wt, rel)
        if os.path.isdir(cand):
            add(cand)
    # 2. MAIN's own list, each entry redirected to the worktree's copy when it has one
    for _idx, value in pairs:
        rel = os.path.normpath(value)
        wt_copy = os.path.join(wt, rel)
        if os.path.isdir(wt_copy):
            add(wt_copy)
            continue
        main_copy = os.path.join(main, rel)
        add(main_copy if os.path.isdir(main_copy) else value)

    block = [t for d in dirs for t in (INCLUDE_FLAG, d)]
    if pairs:
        at = pairs[0][0]
        skip = {i for idx, _v in pairs for i in (idx, idx + 1)}
    else:
        # no `-i` at all: put the worktree's own directories at the head of the flag list
        at = next((k for k, t in enumerate(tokens) if t.startswith("-")), len(tokens))
        skip = set()
    kept = [t for k, t in enumerate(tokens) if k not in skip]
    pos = at - sum(1 for k in skip if k < at)
    return kept[:pos] + block + kept[pos:]


def section_sizes(obj: str) -> dict:
    try:
        secs, _syms = unitutil.read_elf(obj)
    except Exception:
        return {}
    return {s["sname"]: s["size"] for s in secs if s.get("sname") and s.get("size")}


MIN_PROJECT_VERSION = "2.0.0-beta.5"


def measure_project(target: str, base: str, unit: str, tmpdir: str) -> tuple[str, str]:
    """Write a one-unit objdiff project pointing at the two objects; return (project dir, config path).

    `report generate` resolves `target_path`/`base_path` against the project directory, and on Windows it
    only treats a **backslash**-rooted path as absolute (`C:/...` is joined and mangled into `C:...`), so
    the paths are absolutised with `os.path.abspath` - which yields exactly that shape on Windows and a
    plain absolute path elsewhere.
    """
    proj = os.path.join(tmpdir, "measure_project")
    os.makedirs(proj, exist_ok=True)
    cfg_path = os.path.join(proj, "objdiff.json")
    config = {
        "min_version": MIN_PROJECT_VERSION,
        "units": [{
            "name": unit or "measure",
            "target_path": os.path.abspath(target),
            "base_path": os.path.abspath(base),
        }],
    }
    with open(cfg_path, "w", encoding="utf-8") as fh:
        json.dump(config, fh, indent=2)
    return proj, cfg_path


def report_measure(target: str, base: str, symbol: str, objdiff: str, tmpdir: str,
                   unit: str = None, runner=subprocess.run) -> dict:
    """Score one symbol with `report generate` - the exact path behind build/RMHE08/report.json.

    The returned `fuzzy_match_percent` is the official metric: `ledger.py`, `brief.py` and `land.py` all
    read it from the project report, so a worker must not be handed anything else.
    """
    os.makedirs(tmpdir, exist_ok=True)
    proj, _cfg = measure_project(target, base, unit, tmpdir)
    out = os.path.join(tmpdir, "recompile_report_%s.json" % re.sub(r"\W", "_", symbol))
    p = runner([objdiff, "report", "generate", "-p", proj, "-o", out],
               capture_output=True, text=True, errors="replace")
    if p.returncode != 0 or not os.path.exists(out):
        return {"symbol": symbol, "error": (p.stdout or "") + (p.stderr or "")}
    data = json.loads(open(out, encoding="utf-8").read())
    units = data.get("units") or []
    functions = (units[0].get("functions") if units else []) or []
    fn = next((f for f in functions if f.get("name") == symbol), None)
    if fn is None:
        return {"symbol": symbol,
                "error": "symbol is not in the target object (renamed? not in this unit?)"}
    return {"symbol": symbol, "fuzzy_match_percent": fn.get("fuzzy_match_percent"),
            "target_size": fn.get("size"), "report_json": out}


def diff_rows(target: str, base: str, symbol: str, objdiff: str, tmpdir: str,
              runner=subprocess.run) -> dict:
    """Instruction-level diff for the row detail the report does not carry.

    `-c functionRelocDiffs=none` is passed explicitly because `report generate`'s default is `none` while
    `diff`'s is `data_value`: without it the rows disagree with the official classification (relocation-only
    differences show up as `DIFF_ARG_MISMATCH`). The metric in this JSON (`match_percent`) is *not* the
    report's - it is exposed as `diff_match_percent` and must never be quoted as the score.
    """
    out = os.path.join(tmpdir, "recompile_%s.json" % re.sub(r"\W", "_", symbol))
    os.makedirs(tmpdir, exist_ok=True)
    p = runner([objdiff, "diff", "-1", target, "-2", base, symbol,
                "-c", "functionRelocDiffs=none", "--format", "json", "-o", out],
               capture_output=True, text=True, errors="replace")
    if p.returncode != 0 or not os.path.exists(out):
        return {"symbol": symbol, "error": (p.stdout or "") + (p.stderr or "")}
    data = json.loads(open(out, encoding="utf-8").read())
    if isinstance(data, dict) and "left" in data:
        sides = (data.get("left") or {}, data.get("right") or {})
    elif isinstance(data, dict) and "symbols" in data:
        sides = (data, data)
    else:
        return {"symbol": symbol, "error": "unrecognised objdiff output"}

    def entry(side):
        for sym in side.get("symbols") or []:
            if sym.get("name") == symbol:
                return sym
        return None

    tgt, cand = entry(sides[0]), entry(sides[1])
    if tgt is None and cand is None:
        return {"symbol": symbol, "error": "symbol is in neither object (renamed? unpaired?)"}
    return {
        "symbol": symbol,
        "diff_match_percent": (cand or tgt or {}).get("match_percent"),
        "target_size": (tgt or {}).get("size"),
        "candidate_size": (cand or {}).get("size"),
        "paired": tgt is not None and cand is not None,
        "json": out,
    }


def measure(target: str, base: str, symbol: str, objdiff: str, tmpdir: str,
            unit: str = None, runner=subprocess.run) -> dict:
    """The official score for one symbol, plus the instruction-level rows behind it.

    `match_percent` is deliberately the **report** metric (`fuzzy_match_percent`), so any existing consumer
    that reads `measure().match_percent` gets the number that closes a symbol. The positional objdiff value
    is kept as `diff_match_percent`; it is diagnostic only.
    """
    result = report_measure(target, base, symbol, objdiff, tmpdir, unit=unit, runner=runner)
    if "error" in result:
        return result
    result["match_percent"] = result.pop("fuzzy_match_percent")
    rows = diff_rows(target, base, symbol, objdiff, tmpdir, runner=runner)
    if "error" in rows:
        # the score stands on its own; only the row detail is unavailable
        result["rows_error"] = rows["error"]
        return result
    result["diff_match_percent"] = rows.get("diff_match_percent")
    if result.get("target_size") is None:
        result["target_size"] = rows.get("target_size")
    result["candidate_size"] = rows.get("candidate_size")
    result["paired"] = rows.get("paired")
    result["json"] = rows.get("json")
    return result


def absolutize(tokens: list[str], main: str) -> list[str]:
    """Resolve any token that names a file *in MAIN* to an absolute path.

    Windows resolves a relative executable path against the parent process's directory, not against the
    `cwd=` handed to the child, so `build/tools/sjiswrap.exe` fails with WinError 2 even when the child's cwd
    is MAIN. Absolutizing the driver (and the compiler sjiswrap is told to run) is what makes the command
    work from any worktree.
    """
    out = []
    for tok in tokens:
        if not tok.startswith("-") and ("\\" in tok or "/" in tok):
            candidate = os.path.join(main, tok.replace("\\", os.sep))
            if os.path.exists(candidate):
                out.append(os.path.abspath(candidate))
                continue
        out.append(tok)
    return out


def compile_unit(unit: str, main: str, wt: str, dry_run: bool = False, runner=subprocess.run) -> dict:
    tokens = ninja_command(main, unit, runner=runner)
    cmd, obj = rewrite(tokens, unit, main, wt)
    cmd = absolutize(cmd, main)
    os.makedirs(os.path.dirname(obj), exist_ok=True)
    existed = os.path.exists(obj)
    before = os.stat(obj).st_mtime_ns if existed else None
    if dry_run:
        return {"command": cmd, "object": obj, "dry_run": True}
    if existed:
        os.remove(obj)
    started = time.time_ns()
    p = runner(cmd, cwd=main, capture_output=True, text=True, errors="replace")
    log = (p.stdout or "") + (p.stderr or "")
    if p.returncode != 0:
        return {"object": obj, "compiled": False, "error": log}
    if not os.path.exists(obj):
        return {"object": obj, "compiled": False,
                "error": "the compiler returned 0 but wrote no object - MWCC's -o is a DIRECTORY; "
                         "digest:\n" + log}
    after = os.stat(obj).st_mtime_ns
    fresh = after >= started and after != before
    return {"object": obj, "compiled": True, "fresh": fresh, "bytes": os.path.getsize(obj),
            "sections": section_sizes(obj), "log": log}


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("unit", help="unit path from the repository root, e.g. Pl/pl_act")
    ap.add_argument("--main", default=None, help="main worktree (default: resolved with git)")
    ap.add_argument("--measure", default=None, help="symbol to diff against the target object afterwards")
    ap.add_argument("--json", action="store_true")
    ap.add_argument("--dry-run", action="store_true", help="print the command, compile nothing")
    args = ap.parse_args()

    wt = worktree_root()
    main_wt = args.main or main_root(wt)
    unit = args.unit.strip("/")
    result = compile_unit(unit, main_wt, wt, dry_run=args.dry_run)
    result.update({"unit": unit, "worktree": wt, "main": main_wt})
    target = os.path.join(main_wt, "build", "RMHE08", "obj", *unit_source(unit).split("/"))
    target = os.path.splitext(target)[0] + ".o"
    result["target"] = target

    if args.measure and result.get("compiled") and os.path.exists(target):
        result["measure"] = measure(target, result["object"], args.measure, unitutil.OBJDIFF,
                                    os.path.join(wt, "build", "tmp"), unit=unit)

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
    print("  target  %s" % result["target"])
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
    print("\nnext: python .agents/skills/mwcc-unit-matching/scripts/mt.py diff -u %s <symbol>   (in MAIN)"
          "\n      or: python tools/units/recompile.py %s --measure <symbol>" % (result["unit"], result["unit"]))
    if not result.get("fresh"):
        print("WARNING: the object's mtime did not move - treat any measurement as stale", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
