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

**A proposal unit measures too** (AGENTS.md, "the proposal-unit measurement gap"). A worker registers a
fresh proposal in its own worktree first (`configure.py` + `splits.txt`, per the brief) and MAIN has
neither a ninja rule nor a split object for the range until that registration lands. That used to be the
end of `--measure`; three workers hand-built a harness each (borrow a sibling's command line, score
against the retired `auto_*_text.o`, one spent 87 turns on it). Both halves are now the tool's own path,
and neither MAIN's config nor the worktree is written:

* the **command line** comes from MAIN's ninja (registered), the worktree's ninja (if the worker
generated one), or - last - a registered sibling in the *same `config.libs` block* of the worktree's
`configure.py`, with only the source, the `-o` directory and the `-lang` token pointed at this unit.
Those are the flags `project.py` emits for that lib, not a hand-rolled approximation;
* the **target object** is MAIN's retired `auto_*_text.o` that owns the symbol's address - the same
original bytes the split will put in the registered object (`auto_<symbol[:20]>_text.o` for a single
symbol, else the `auto_<nn>_<address>_text` run that covers it).

The score is still `report generate`'s `fuzzy_match_percent`, and the output names the target object and
says `[fallback]`, so a worker can tell a real measurement from one against the retired split. A
*registered* unit takes exactly the path it took before (MAIN's rule, MAIN's object, same output).

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


def unit_source(unit: str) -> str:
    return unit if unit.endswith(SRC_EXT) else unit + ".cpp"


def _ninja_compile_lines(main: str, unit: str, runner=subprocess.run):
    """(target, mwcceppc lines, completed process) for a unit, without raising.

    Empty lines is the normal case for a *proposal* unit in MAIN: the source is registered in the
    worker's worktree, so MAIN's build.ninja has no edge for `build/RMHE08/src/<unit>.o` yet.
    """
    target = "build/RMHE08/src/" + os.path.splitext(unit_source(unit))[0] + ".o"
    p = runner(["ninja", "-t", "commands", target], cwd=main, capture_output=True, text=True,
               errors="replace")
    return target, [l for l in (p.stdout or "").splitlines() if "mwcceppc" in l], p


def ninja_command(main: str, unit: str, runner=subprocess.run) -> list[str]:
    """The exact compile command MAIN's ninja would run, as tokens."""
    target, lines, p = _ninja_compile_lines(main, unit, runner)
    if not lines:
        raise SystemExit("could not get the compile command for %s from ninja in %s:\n%s%s"
                         % (target, main, p.stdout, p.stderr))
    return unitutil.unquote(lines[-1].split())


def _unit_stem(name: str) -> str:
    """`ef/effect.cpp` / `src/ef/effect.c` / `ef/effect` -> `ef/effect` - registration names and unit
    spellings differ by prefix and extension, so compare on this."""
    n = name.replace("\\", "/").strip().lstrip("./")
    if n.startswith("src/"):
        n = n[len("src/"):]
    return os.path.splitext(n)[0]


def retarget(tokens: list[str], unit: str) -> list[str]:
    """Point a borrowed command line at *this* unit's object directory and language.

    MWCC's `-o` is a directory and `project.py` sets it to the source's own directory, and it inserts the
    `-lang` token from the extension; those are the only two things a same-lib sibling's line gets wrong.
    Nothing else is touched - the flags stay exactly what ninja printed for the lib.
    """
    obj_dir = os.path.join("build", "RMHE08", "src", *unit_source(unit).split("/")[:-1])
    lang = "-lang=c" if unit_source(unit).lower().endswith(".c") else "-lang=c++"
    out, i = [], 0
    while i < len(tokens):
        tok = tokens[i]
        if tok == "-o" and i + 1 < len(tokens):
            out += [tok, obj_dir]
            i += 2
            continue
        if tok.startswith("-lang="):
            out.append(lang)
            i += 1
            continue
        out.append(tok)
        i += 1
    return out


# One `config.libs` block: its name up to the object list, then the list. The brace-free runs on either
# side let the block's own comments through - the real `configure.py` puts a long comment between the `{`
# and the `"lib"` line, and requiring only whitespace there (what `brief.lib_for` does) silently matches
# just the handful of blocks that have no comment, so those lookups report the lib as `(unknown)`.
LIB_BLOCK_RE = re.compile(r"\{[^{}]*?\"lib\": \"([^\"]+)\"[^{}]*?\"objects\": \[(.*?)\]\s*,\s*\n\s*\}",
                          re.S)
OBJECT_RE = re.compile(r"Object\(\s*\w+\s*,\s*\"([^\"]+)\"")


def lib_block(wt: str, unit: str):
    """(lib name, [object source names]) for the `config.libs` block that registers `unit`.

    Read from the **worktree's** `configure.py` - the registration is the worker's, and it is exactly what
    MAIN does not have yet. (None, []) when the unit is not registered there.
    """
    path = os.path.join(wt, "configure.py")
    if not os.path.exists(path):
        return None, []
    text = open(path, encoding="utf-8", errors="replace").read()
    want = _unit_stem(unit)
    for m in LIB_BLOCK_RE.finditer(text):
        names = OBJECT_RE.findall(m.group(2))
        if any(_unit_stem(n) == want for n in names):
            return m.group(1), names
    return None, []


def sibling_for(main: str, wt: str, unit: str, runner=subprocess.run):
    """(sibling, tokens) - a registered unit in the worktree's lib for `unit`, and its command line.

    Every object in one `config.libs` block shares the `mw_version` and `cflags` `project.py` builds the
    command from, so a sibling's line IS this unit's flags. Preference is the same module directory and
    the same extension, so the borrow usually needs no correction at all. Raises with the registration
    step to run when the unit is not in `configure.py`, or when its lib has no unit MAIN can build.
    """
    lib, names = lib_block(wt, unit)
    want = _unit_stem(unit)
    if not lib:
        raise SystemExit(
            "%s is not registered in %s/configure.py, and MAIN has no compile command for it - a proposal "
            "unit is measurable only once its own `Object(...)` line and `splits.txt` block are there "
            "(docs/plan.md: registration comes before the bodies)" % (unit, wt))
    want_dir, want_ext = os.path.dirname(want), os.path.splitext(unit_source(unit))[1].lower()

    def rank(name: str):
        stem = _unit_stem(name)
        return (0 if os.path.dirname(stem) == want_dir else 1,
                0 if os.path.splitext(name)[1].lower() == want_ext else 1, stem)

    for name in sorted(names, key=rank):
        stem = _unit_stem(name)
        if stem == want:
            continue
        _target, lines, _p = _ninja_compile_lines(main, stem, runner)
        if lines:
            return stem, unitutil.unquote(lines[-1].split())
    raise SystemExit(
        "no unit in lib %r (the one %s/configure.py registers %s in) has a compile command in MAIN's "
        "ninja - a brand-new lib cannot be measured until its registration lands on MAIN"
        % (lib, wt, unit))


def unit_tokens(main: str, wt: str, unit: str, runner=subprocess.run):
    """(tokens, source) - the real compile command for `unit`, and where it came from.

    In order: MAIN's ninja (the registered unit, unchanged), the worktree's own ninja (a worker who
    regenerated `build.ninja` after registering), then a registered sibling in the same lib (the proposal
    path). Only the sibling's line is rewritten, and only its `-c`/`-o`/`-lang` - the flags are the ones
    ninja printed.
    """
    _target, lines, _p = _ninja_compile_lines(main, unit, runner)
    if lines:
        return unitutil.unquote(lines[-1].split()), "main"
    _target, lines, _p = _ninja_compile_lines(wt, unit, runner)
    if lines:
        return unitutil.unquote(lines[-1].split()), "worktree"
    sibling, tokens = sibling_for(main, wt, unit, runner)
    return retarget(tokens, unit), "sibling %s (same lib)" % sibling


# The post-compile helpers project.py chains after MWCC; each takes the just-written object as its
# positional argument. Kept as data so a new helper is one entry, not another special case.
OBJECT_HELPERS = ("objalign.py", "objextab.py")


def retarget_object_helpers(tokens: list[str], obj_path: str) -> list[str]:
    """Point every chained `<helper>.py <object>` argument at the worktree's object.

    Every `.o` rule ends `... && python tools\\elf\\objalign.py build\\RMHE08\\src\\<unit>.o && python
    tools\\elf\\objextab.py build\\RMHE08\\src\\<unit>.o`, and those arguments are neither `-o` nor `-c`,
    so `rewrite` used to leave them **relative** while `compile_unit` runs the whole line with `cwd=MAIN`.
    The helper then resolved MAIN's copy of the object - absent for a unit MAIN has not registered -
    raised `FileNotFoundError`, and the tool reported `FAILED` even though MWCC had compiled the worktree
    object fine (the exact measurement `docs/plan.md`'s landing recipe names; its `--dry-run` showed the
    good command and hid the mismatch). `objextab` was the same latent failure `objalign` already had:
    with a worktree object and an unregistered unit it silently rewrote MAIN's object instead of this
    tree's. A borrowed sibling's line is worse still: its helper carries the *sibling's* object name, so
    the fix replaces the token after the helper rather than matching on the object's name.
    """
    out = list(tokens)
    changed = False
    for i, tok in enumerate(out):
        if any(tok.replace("\\", "/").endswith(helper) for helper in OBJECT_HELPERS) \
                and i + 1 < len(out):
            out[i + 1] = os.path.abspath(obj_path)
            changed = True
    return out if changed else tokens


# kept for callers outside this module (the name the fix first shipped under)
retarget_objalign = retarget_object_helpers


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
    # the chained objalign/objextab arguments are not `-o`/`-c` values, so they were left relative to
    # MAIN; absolutise them here, after the object path is known, so each helper touches the object MWCC
    # just wrote rather than MAIN's.
    out = retarget_object_helpers(out, obj_path)
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


def source_path(wt: str, unit: str) -> str:
    """The source a unit's object must be newer than, resolved the way `rewrite` resolves it."""
    return os.path.join(wt, "src", *unit_source(unit).split("/"))


def object_is_fresh(object_path: str, source: str) -> tuple[bool, str]:
    """(fresh, reason) - the stale-object guard every measurement path must pass before reading an object.

    The failure this refuses: a compile fails (or is a no-op) and leaves the **previous** object on disk,
    and the scorer reads it and reports the old score as this run's number. That is worse than no scorer,
    because the number looks like progress. Two layers protect against it - `compile_unit` deletes the
    object before it compiles and requires it to reappear, and this last check refuses any object whose
    mtime predates the source it claims to be built from, so a scorer handed an object path directly
    cannot be fooled either.

    An object with no source to compare against is not rejected *here* (`compile_unit` already failed the
    compile if the source is missing); this is about the ordering, not existence.
    """
    if not os.path.exists(object_path):
        return False, ("STALE OBJECT: no object at %s - the compile wrote nothing, so there is no score to "
                       "read" % object_path)
    if not os.path.exists(source):
        return True, ""
    obj_m, src_m = os.stat(object_path).st_mtime_ns, os.stat(source).st_mtime_ns
    if obj_m < src_m:
        return False, (
            "STALE OBJECT: %s is older than its source %s (%d ns) - the compile did not rewrite it; "
            "refusing to measure, because a score read from here would be last build's number dressed as "
            "this one's" % (object_path, source, src_m - obj_m))
    return True, ""


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


# Tokens that are switches, not paths. `cmd /c` is the one that bit: `os.path.join(main, "/c")` is
# `C:/c` - a leading separator resets to the drive root - and on a host where `C:\c` exists (this one
# does) the switch was rewritten to that path, so the child became an *interactive* `cmd`, printed its
# banner, and never wrote an object. A `-` or `/` prefix is always a switch in these command lines
# (`-o`, `-i`, `-c`, `-lang=`, `cmd /c`), never a MAIN-relative file.
SWITCH_PREFIXES = ("-", "/")


def is_switch(tok: str) -> bool:
    """Whether a command token is a flag/switch, never a MAIN-relative path.

    The one predicate that decides `absolutize`'s classification; kept separate so the `cmd /c` contract
    can be asserted without depending on whether this host happens to have a `C:/c` artifact.
    """
    return tok.startswith(SWITCH_PREFIXES)


def absolutize(tokens: list[str], main: str) -> list[str]:
    """Resolve any token that names a file *in MAIN* to an absolute path, without touching switches.

    Windows resolves a relative executable path against the parent process's directory, not against the
    `cwd=` handed to the child, so `build/tools/sjiswrap.exe` fails with WinError 2 even when the child's cwd
    is MAIN. Absolutizing the driver (and the compiler sjiswrap is told to run) is what makes the command
    work from any worktree.

    The guard that matters is the switch prefix: a token beginning with `-` or `/` is a flag, and
    `os.path.join` would resolve a rooted one (`/c`) against the *drive root* rather than MAIN. With
    `C:/c` present that turned `cmd /c ...` into `cmd C:\\c ...` - an interactive shell, no compile, no
    object. A rooted token that is a real path (`C:\\...`) is still absolutised by `os.path.join`
    discarding `main`, so absolute paths keep working.
    """
    out = []
    for tok in tokens:
        if is_switch(tok):
            out.append(tok)
            continue
        if "\\" in tok or "/" in tok:
            candidate = os.path.join(main, tok.replace("\\", os.sep))
            if os.path.exists(candidate):
                out.append(os.path.abspath(candidate))
                continue
        out.append(tok)
    return out


# ---------------------------------------------------------------------------------------------------
# The target object for a proposal unit: MAIN's retired per-symbol/per-run `auto_*_text.o`
# ---------------------------------------------------------------------------------------------------
# A registered unit measures against MAIN's `build/RMHE08/obj/<unit>.o` (the split object). A proposal has
# no such object until the registration lands, but MAIN *does* still build the range as the `auto_*_text`
# split objects its previous split produced, with the same original bytes. That is the honest fallback, and
# it is what each stuck worker re-derived by hand.

SYMBOLS_REL = os.path.join("config", "RMHE08", "symbols.txt")
SYMBOL_LINE_RE = re.compile(r"^(\S+)\s*=\s*(.*)$")
SYMBOL_TEXT_RE = re.compile(r"^\.text:(0x[0-9A-Fa-f]+)$")
AUTO_RUN_RE = re.compile(r"^auto_\d+_([0-9A-Fa-f]{8})_text$")


def text_symbol_addresses(main: str) -> dict:
    """{name: address} for every `.text` symbol in MAIN's map - the only section `report generate` scores."""
    out: dict = {}
    path = os.path.join(main, SYMBOLS_REL)
    if not os.path.exists(path):
        return out
    for line in open(path, encoding="utf-8", errors="replace"):
        m = SYMBOL_LINE_RE.match(line.rstrip("\n"))
        if not m:
            continue
        for part in m.group(2).split(";"):
            a = SYMBOL_TEXT_RE.match(part.strip())
            if a:
                out[m.group(1)] = int(a.group(1), 16)
                break
    return out


def auto_text_runs(main: str) -> list:
    """[(start, size, object)] for MAIN's retired *run* split objects, ascending by start address.

    dtk names a run `auto_<nn>_<address>_text` (its start in the name) and records its `code_size` in MAIN's
    `build/RMHE08/config.json`; a run can hold several functions (`fn_80041304` and `fn_8004132C` share
    `auto_03_80041304_text.o`). A single symbol gets `auto_<symbol[:20]>_text` instead, found by name.
    """
    path = os.path.join(main, "build", "RMHE08", "config.json")
    if not os.path.exists(path):
        return []
    try:
        units = json.load(open(path, encoding="utf-8")).get("units") or []
    except (ValueError, OSError):
        return []
    out = []
    for u in units:
        m = AUTO_RUN_RE.match(u.get("name") or "")
        if m:
            out.append((int(m.group(1), 16), u.get("code_size") or 0, u.get("object") or ""))
    out.sort()
    return out


def proposal_target(main: str, symbol: str):
    """(target object path, note) for `symbol` in a proposal unit, or (None, why not).

    Resolution is by **address**, so a branch that renamed the symbol still finds the object: MAIN's map
    gives the one name it knows at that address. Two shapes of retired object:

    * the single-symbol object dtk named after the symbol - `auto_<name[:20]>_text.o`. The truncation is
      dtk's; the existence test plus the name at the address is the test, not a spelling guess;
    * the run that covers the address - `auto_<nn>_<start>_text.o`, `start <= addr < start + code_size`.
    """
    addresses = text_symbol_addresses(main)
    addr = addresses.get(symbol)
    if addr is None:
        return None, ("%s is not a `.text` symbol in MAIN's %s, and the fallback locates the retired split "
                      "object by address - a symbol this branch renamed has no entry there"
                      % (symbol, SYMBOLS_REL))
    objdir = os.path.join(main, "build", "RMHE08", "obj")
    # 1. a single-symbol object named (by dtk, truncated to 20 chars) after the symbol at this address
    for name in [symbol] + [n for n, a in addresses.items() if a == addr and n != symbol]:
        cand = os.path.join(objdir, "auto_%s_text.o" % name[:20])
        if os.path.exists(cand):
            return cand, ("retired single-symbol split object %s (%s at 0x%X)"
                          % (os.path.basename(cand), name, addr))
    # 2. the run whose range covers the address
    for start, size, rel in auto_text_runs(main):
        if start <= addr < start + size:
            path = os.path.join(main, *rel.replace("\\", "/").split("/"))
            if os.path.exists(path):
                return path, ("retired split object %s covers 0x%X-0x%X - the run that owns %s"
                              % (os.path.basename(rel), start, start + size, symbol))
    return None, ("0x%X (%s) is not inside any retired `auto_*_text` object in MAIN - it belongs to a "
                  "already-registered unit or a gap, so MAIN has no original object for it"
                  % (addr, symbol))


def object_has_symbol(obj: str, symbol: str) -> bool:
    """Whether an object defines `symbol` at all - the check that separates "nothing to pair" from a
    renamed symbol, both of which `report generate` answers with a null `fuzzy_match_percent`."""
    try:
        _secs, syms = unitutil.read_elf(obj)
    except Exception:
        return False
    return any(s[0] == symbol for s in syms)


def measure_target(main: str, unit: str, symbol: str):
    """(target object, kind, note) for `--measure`. `kind` is `registered`, `auto-fallback` or `missing`.

    `registered` is the path a registered unit has always measured against and is decided first, so that
    path cannot change.
    """
    head = os.path.join(main, "build", "RMHE08", "obj", *unit_source(unit).split("/"))
    registered = os.path.splitext(head)[0] + ".o"
    if os.path.exists(registered):
        return registered, "registered", ""
    found, note = proposal_target(main, symbol)
    if found is None:
        return registered, "missing", note
    return found, "auto-fallback", note


def compile_unit(unit: str, main: str, wt: str, dry_run: bool = False, runner=subprocess.run,
                 tokens: list[str] = None) -> dict:
    if tokens is None:
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
    ok, why = object_is_fresh(obj, source_path(wt, unit))
    if not ok:
        # the compile returned 0 and wrote *an* object, but it is not this source's - refuse it
        return {"object": obj, "compiled": False, "error": why}
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
    tokens, cmd_source = unit_tokens(main_wt, wt, unit)
    result = compile_unit(unit, main_wt, wt, dry_run=args.dry_run, tokens=tokens)
    result.update({"unit": unit, "worktree": wt, "main": main_wt})
    if cmd_source != "main":
        # the registered path must read exactly as it did before; a proposal says where its flags came from
        result["command_source"] = cmd_source
    target = os.path.join(main_wt, "build", "RMHE08", "obj", *unit_source(unit).split("/"))
    target = os.path.splitext(target)[0] + ".o"
    result["target"] = target

    if args.measure and result.get("compiled"):
        target, target_kind, target_note = measure_target(main_wt, unit, args.measure)
        result["target"] = target
        if target_kind != "registered":
            result["target_kind"] = target_kind
            result["target_note"] = target_note
        if target_kind == "missing":
            result["measure"] = {"symbol": args.measure, "error": target_note}
        else:
            result["measure"] = measure(target, result["object"], args.measure, unitutil.OBJDIFF,
                                        os.path.join(wt, "build", "tmp"), unit=unit)
            m = result["measure"]
            if target_kind == "auto-fallback" and "error" not in m and m.get("match_percent") is None:
                # report pairs by name and answers a null (not an error) when pairing fails; say which of
                # the two causes it is, because the message is what tells the worker where to look
                if not object_has_symbol(result["object"], args.measure):
                    m["error"] = ("%s does not define %s (nothing to pair) - MAIN's retired object defines it "
                                  "at that address, so the unit's own source is what is short"
                                  % (os.path.basename(result["object"]), args.measure))
                else:
                    m["error"] = ("no pairing: %s defines %s, but MAIN's %s spells that address differently - "
                                  "the report pairs symbols by name, which a renamed symbol breaks"
                                  % (os.path.basename(result["object"]), args.measure,
                                     os.path.basename(target)))
                result.pop("match_percent", None)

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
    if result.get("target_kind") == "auto-fallback":
        print("  [fallback] MAIN has no split object for %s yet; %s\n             same original bytes, so the"
              " score is the one the registered unit will report" % (result["unit"], result["target_note"]))
    elif result.get("target_kind") == "missing":
        print("  [no target] %s" % result["target_note"])
    if result.get("command_source"):
        print("  command %s - real flags from MAIN's ninja" % result["command_source"])
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
