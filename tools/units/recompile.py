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
  change (the source, the `-o` directory, the worktree's own `include/`), and runs it with MAIN as cwd;
* deletes the object first, then asserts the file exists and its mtime moved — a stale object is impossible;
* prints the two object paths, the section sizes, and can measure a symbol with objdiff-cli's explicit
  `-1 <target> -2 <base>` mode, which needs no project files at all.

    python tools/units/recompile.py <unit> [--measure <symbol>] [--json] [--dry-run] [--print-command]

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
    is a directory - the object's *name* comes from the source) and the include path order.
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
    # the worktree's own headers must win over MAIN's copy of the same path
    inc = []
    for p in (os.path.join(wt, "build", "RMHE08", "include"), os.path.join(wt, "include")):
        if os.path.isdir(p):
            inc += ["-i", p]
    if inc:
        first_compile = next((k for k, t in enumerate(out) if t in ("-c",)), len(out))
        out = out[:first_compile] + inc + out[first_compile:]
    obj_path = os.path.join(obj_dir, os.path.splitext(os.path.basename(wt_src))[0] + ".o")
    return out, obj_path


def section_sizes(obj: str) -> dict:
    try:
        secs, _syms = unitutil.read_elf(obj)
    except Exception:
        return {}
    return {s["sname"]: s["size"] for s in secs if s.get("sname") and s.get("size")}


def measure(target: str, base: str, symbol: str, objdiff: str, tmpdir: str) -> dict:
    """Per-symbol diff with objdiff-cli's explicit-object mode - no project files, so it works in a worktree.

    The JSON is `{left: {symbols: [...]}, right: {...}}`; each symbol entry carries `name`, `size`,
    `match_percent`. `left` is the target object, `right` the candidate.
    """
    out = os.path.join(tmpdir, "recompile_%s.json" % re.sub(r"\W", "_", symbol))
    os.makedirs(tmpdir, exist_ok=True)
    p = subprocess.run([objdiff, "diff", "-1", target, "-2", base, symbol, "--format", "json", "-o", out],
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
        "match_percent": (cand or tgt or {}).get("match_percent"),
        "target_size": (tgt or {}).get("size"),
        "candidate_size": (cand or {}).get("size"),
        "paired": tgt is not None and cand is not None,
        "json": out,
    }


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
                                    os.path.join(wt, "build", "tmp"))

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
            print("  measure %s: %s%% (target %s B, ours %s B, paired=%s)"
                  % (m["symbol"], m.get("match_percent"), m.get("target_size"), m.get("candidate_size"),
                     m.get("paired")))
    print("\nnext: python .agents/skills/mwcc-unit-matching/scripts/mt.py diff -u %s <symbol>   (in MAIN)"
          "\n      or: python tools/units/recompile.py %s --measure <symbol>" % (result["unit"], result["unit"]))
    if not result.get("fresh"):
        print("WARNING: the object's mtime did not move - treat any measurement as stale", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
