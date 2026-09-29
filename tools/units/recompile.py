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
    out = subprocess.run(["git", *args], cwd=cwd, capture_output=True, text=True, encoding="utf-8", errors="replace")
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


def normalize_unit(unit: str) -> str:
    """Strip the prefixes a lane pastes (`src/`, `./`, `build/RMHE08/src/`) from a unit argument.

    Passing the real path (`src/NHTTP/NHTTP_bgnend`) used to produce a doubled `src/src/...` in the
    rewritten command; the unit argument is a path *from `src/`*, so the prefixes only ever hide it.
    """
    u = unit.replace("\\", "/").strip()
    while u.startswith("./"):
        u = u[2:]
    changed = True
    while changed:
        changed = False
        for pre in ("build/RMHE08/src/", "build/RMHE08/obj/", "src/"):
            if u.startswith(pre):
                u = u[len(pre):]
                changed = True
    return u.strip("/")


def resolve_unit_source(unit: str, wt: str = None, main: str = None, source: str = None) -> str:
    """The unit's source spelling **with its real extension** - the fix for the `.c` unit.

    `unit_source` appends `.cpp` when no extension is given, which is right for the `.cpp` units that
    make up most of the tree and wrong for every `.c` unit (`NHTTP/NHTTP_bgnend`, `RSP/runtime`, ...):
    `recompile.py` then told MWCC to compile `src/NHTTP/NHTTP_bgnend.cpp`, which does not exist, and
    four lanes fell back to `ninja build/RMHE08/src/<unit>.o` + `symdiff.py`. Resolution order:

    1. a source the caller named with `--source` (the deliberate override);
    2. `configure.py`'s own registration (`lib_block`) - the one authority for the spelling;
    3. the file that actually exists under `src/` (`.cpp` first, so the old default is byte-for-byte);
    4. `.cpp`, the pre-fix behaviour.

    The result is a unit spelling (`Pl/pl_act.cpp`), not a path, so every downstream `unit_source` call
    is the identity and the object directory / ninja target derivation is unchanged.
    """
    if source:
        s = source.replace("\\", "/").strip()
        for root in (wt, main):
            if not root:
                continue
            base = os.path.join(os.path.abspath(root), "src").replace("\\", "/") + "/"
            if os.path.normcase(s).startswith(os.path.normcase(base)):
                s = s[len(base):]
                break
        for pre in ("build/RMHE08/src/", "src/", "./"):
            if s.startswith(pre):
                s = s[len(pre):]
        return s.strip("/")
    unit = normalize_unit(unit)
    if unit.endswith(SRC_EXT):
        return unit
    stem = _unit_stem(unit)
    for root in (wt, main):
        if not root:
            continue
        try:
            _lib, names = lib_block(root, unit)
        except Exception:
            names = []
        for name in names:
            if _unit_stem(name) == stem:
                return normalize_unit(name)
    for root in (wt, main):
        if not root:
            continue
        base = os.path.join(root, "src", *unit.split("/"))
        for ext in (".cpp", ".c", ".cp", ".cxx", ".cc"):
            if os.path.isfile(base + ext):
                return unit + ext
    return unit + ".cpp"


def _ninja_compile_lines(main: str, unit: str, runner=subprocess.run):
    """(target, mwcceppc lines, completed process) for a unit, without raising.

    Empty lines is the normal case for a *proposal* unit in MAIN: the source is registered in the
    worker's worktree, so MAIN's build.ninja has no edge for `build/RMHE08/src/<unit>.o` yet.
    """
    target = "build/RMHE08/src/" + os.path.splitext(unit_source(unit))[0] + ".o"
    p = runner(["ninja", "-t", "commands", target], cwd=main, capture_output=True, text=True, encoding="utf-8",
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
            "STALE OBJECT: %s is older than its source %s (object %s, source %s) - the compile did not "
            "rewrite it; refusing to measure, because a score read from here would be last build's number "
            "dressed as this one's"
            % (object_path, source, _stamp(obj_m / 1e9), _stamp(src_m / 1e9)))
    return True, ""


def section_sizes(obj: str) -> dict:
    try:
        secs, _syms = unitutil.read_elf(obj)
    except Exception:
        return {}
    return {s["sname"]: s["size"] for s in secs if s.get("sname") and s.get("size")}


MIN_PROJECT_VERSION = unitutil.MIN_PROJECT_VERSION


def measure(target: str, base: str, symbol: str, objdiff: str, tmpdir: str,
            unit: str = None, runner=subprocess.run) -> dict:
    """The official score for one symbol, plus the instruction-level rows behind it.

    The score comes from `unitutil.report_measure`, i.e. `report generate` over a one-unit project - the
    SAME primitive `measure.py` scores a whole unit with (`unitutil.report_functions`) and the number
    `build/RMHE08/report.json`, `ledger.py` and `land.py` carry. There used to be a second copy of that
    project/report code here; deleting it is what makes `--measure` a single way to measure rather than a
    parallel implementation that can drift (the drift that once printed ~0.36 pt low on `RSO/runtime`).

    `match_percent` is deliberately the **report** metric (`fuzzy_match_percent`), so any existing consumer
    that reads `measure().match_percent` gets the number that closes a symbol. The positional objdiff value
    is kept as `diff_match_percent`; it is diagnostic only.
    """
    previous = unitutil.OBJDIFF
    unitutil.OBJDIFF = objdiff
    try:
        result = unitutil.report_measure(target, base, symbol, unit_name=unit, tmpdir=tmpdir,
                                         runner=runner)
    finally:
        unitutil.OBJDIFF = previous
    if "error" in result:
        return result
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
               capture_output=True, text=True, encoding="utf-8", errors="replace")
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
SPLITS_REL = os.path.join("config", "RMHE08", "splits.txt")
SYMBOL_LINE_RE = re.compile(r"^(\S+)\s*=\s*(.*)$")
SYMBOL_TEXT_RE = re.compile(r"^\.text:(0x[0-9A-Fa-f]+)$")
AUTO_RUN_RE = re.compile(r"^auto_\d+_([0-9A-Fa-f]{8})_text$")


def same_tree(a: str, b: str) -> bool:
    """Whether two paths name the same tree (normcase/abspath, so Windows case and slashes agree)."""
    return os.path.normcase(os.path.abspath(a)) == os.path.normcase(os.path.abspath(b))


def resolve_map(wt: str, main: str, rel: str = SYMBOLS_REL):
    """(absolute path, kind) of a config map resolved from **this invocation's** tree outward.

    `kind` is `worktree-map`, `main-map` or `missing`. This is `resolve_target`'s discipline applied to the
    *map* (F43): a branch that renamed its own symbols has edited *its* `config/RMHE08/symbols.txt`, and a
    lookup pinned to MAIN's copy cannot see a name MAIN never carried - the filed refusal "a symbol this
    branch renamed has no entry there". The invocation tree comes first, MAIN is the fallback (a fresh
    worktree may have no `config/` of its own yet), and never MAIN-only. The caller prints the result:
    which map was read is part of what makes a measurement unambiguous.

    `rel` is a parameter so the *same* order serves `splits.txt` (`SPLITS_REL`): nothing here is
    symbols-specific.
    """
    p_wt, p_main = os.path.join(wt, rel), os.path.join(main, rel)
    if not same_tree(wt, main) and os.path.exists(p_wt):
        return os.path.abspath(p_wt), "worktree-map"
    if os.path.exists(p_main):
        return os.path.abspath(p_main), "main-map"
    if os.path.exists(p_wt):
        return os.path.abspath(p_wt), "worktree-map"
    return os.path.abspath(p_main), "missing"


def text_symbol_addresses(map_path: str) -> dict:
    """{name: address} for every `.text` symbol in a symbols.txt - the only section `report generate`
    scores. `map_path` is the resolved map itself (`resolve_map`), not a tree root."""
    out: dict = {}
    if not os.path.exists(map_path):
        return out
    for line in open(map_path, encoding="utf-8", errors="replace"):
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


def symbol_addresses(wt: str, main: str):
    """({name: address}, map path, map kind) merged from this invocation's map first, MAIN's second.

    Merging both is what makes a **rename** measurable (F43): the branch's map carries the new name at the
    address while MAIN's still carries the old one - and MAIN's retired `auto_<name>_text.o` is named after
    the *old* spelling, so the by-name step needs both maps. The invocation's map wins whenever the same
    name appears in both, because it is the tree being edited.
    """
    path, kind = resolve_map(wt, main)
    out: dict = {}
    main_map = os.path.abspath(os.path.join(main, SYMBOLS_REL))
    if not same_tree(wt, main) and os.path.exists(main_map) \
            and os.path.normcase(main_map) != os.path.normcase(path):
        out.update(text_symbol_addresses(main_map))     # the older spelling first ...
    out.update(text_symbol_addresses(path))             # ... the invocation's name wins
    return out, path, kind


def retired_object_dirs(wt: str, main: str) -> list[str]:
    """The `obj/` directories a retired `auto_*_text.o` can live in: MAIN's first, then this tree's."""
    dirs = [os.path.join(main, "build", "RMHE08", "obj")]
    if not same_tree(wt, main):
        dirs.append(os.path.join(wt, "build", "RMHE08", "obj"))
    return dirs


def proposal_target(wt: str, main: str, symbol: str):
    """(target object path, note) for `symbol` in a proposal unit, or (None, why not).

    Resolution is by **address**, and the address comes from the map this invocation resolves
    (`symbol_addresses` - the branch's own map first, MAIN's second), so a branch that renamed the symbol
    still finds the object: its map knows the new name, MAIN knows the old one. Two shapes of retired
    object:

    * the single-symbol object dtk named after the symbol - `auto_<name[:20]>_text.o`. The truncation is
      dtk's; the existence test plus the name at the address is the test, not a spelling guess - and every
      name either map places at that address is tried, so a rename does not lose the object;
    * the run that covers the address - `auto_<nn>_<start>_text.o`, `start <= addr < start + code_size`.
    """
    addresses, map_path, map_kind = symbol_addresses(wt, main)
    addr = addresses.get(symbol)
    if addr is None:
        return None, ("%s is not a `.text` symbol in the map this tree resolves (%s [%s]) or in MAIN's "
                      "copy, and the fallback locates the retired split object by address"
                      % (symbol, map_path, map_kind))
    names = [symbol] + sorted(n for n, a in addresses.items() if a == addr and n != symbol)
    # 1. a single-symbol object named (by dtk, truncated to 20 chars) after a name at this address
    for objdir in retired_object_dirs(wt, main):
        for name in names:
            cand = os.path.join(objdir, "auto_%s_text.o" % name[:20])
            if os.path.exists(cand):
                return cand, ("retired single-symbol split object %s (%s at 0x%X)"
                              % (os.path.basename(cand), name, addr))
    # 2. the run whose range covers the address
    for tree in ([main] if same_tree(wt, main) else [main, wt]):
        for start, size, rel in auto_text_runs(tree):
            if start <= addr < start + size:
                path = os.path.join(tree, *rel.replace("\\", "/").split("/"))
                if os.path.exists(path):
                    return path, ("retired split object %s covers 0x%X-0x%X - the run that owns %s"
                                  % (os.path.basename(rel), start, start + size, symbol))
    return None, ("0x%X (%s) is not inside any retired `auto_*_text` object in this tree or MAIN - it "
                  "belongs to an already-registered unit or a gap, so there is no original object for it"
                  % (addr, symbol))


def object_has_symbol(obj: str, symbol: str) -> bool:
    """Whether an object defines `symbol` at all - the check that separates "nothing to pair" from a
    renamed symbol, both of which `report generate` answers with a null `fuzzy_match_percent`."""
    try:
        _secs, syms = unitutil.read_elf(obj)
    except Exception:
        return False
    return any(s[0] == symbol for s in syms)


def target_rel(unit: str) -> str:
    """The registered split object's path relative to a tree root, from the unit spelling.

    This is the worktree *and* MAIN layout: `<root>/build/RMHE08/obj/<unit>.o`.
    """
    head = os.path.join("build", "RMHE08", "obj", *unit_source(unit).split("/"))
    return os.path.splitext(head)[0] + ".o"


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


def resolve_target(wt: str, main: str, unit: str, symbol: str):
    """(target object, kind, note) for `--measure`, resolved from THIS invocation's tree outward.

    `kind` is `worktree-split`, `registered`, `auto-fallback` or `missing`. The order is the one
    `measure.py` uses, and it is the fix for the filed trap of reading MAIN's object inside a worktree
    that has its own copy:

    1. the **worktree's** split object, when this tree is not MAIN and the object exists (a proposal the
       worker has already split here - the *real* new bytes, which MAIN cannot have first);
    2. MAIN's registered split object (`registered` - the path a registered unit has always measured
       against, unchanged);
    3. the retired `auto_*_text` object that owns the symbol's address, looked up through the map this
       invocation resolves (the branch's own `symbols.txt` first, MAIN's second) and found in MAIN's
       `obj/` or this tree's - the proposal path before its registration lands on MAIN;
    4. `missing`, so the caller refuses to invent a number.

    The returned path is absolute, and the CLI prints it, so a measurement is never ambiguous about which
    tree it came from; `resolve_map` and the printed map line do the same for the map the address came
    from.  When the object is MAIN's while the invocation is a worktree - step 2 - the `note` says so and
    names the path this tree would need, because `registered` alone does not say *which* tree registered it.
    """
    rel = target_rel(unit)
    same = same_tree(wt, main)
    p_wt, p_main = os.path.join(wt, rel), os.path.join(main, rel)
    if not same and os.path.exists(p_wt):
        return p_wt, "worktree-split", ""
    if os.path.exists(p_main):
        # Run from a worktree that has no object for this unit: the score is MAIN's, and the caller must
        # be able to see that without reading the path.  `resolve_target` uses MAIN's split object where
        # MAIN is the registered tree - a plain `registered` label hides which of the two trees that is,
        # which is exactly how the filed double-take read a MAIN number as this lane's.
        note = ""
        if not same:
            note = ("this tree has no split object for %s at %s - the score is MAIN's split object; "
                    "re-split this tree (ninja build/RMHE08/config.json, or a plain ninja) to score your own"
                    % (unit, rel))
        return p_main, "registered", note
    found, note = proposal_target(wt, main, symbol)
    if found:
        return found, "auto-fallback", note
    map_path, map_kind = resolve_map(wt, main)
    return p_main, "missing", (
        "no original object for %s: no split object at %s in this tree or MAIN, and no retired "
        "`auto_*_text` object covers the address of %s (map: %s [%s])"
        % (unit, rel, symbol, map_path, map_kind))


# the name this shipped under before item B (`resolve_target` follows the invocation; this searched MAIN
# only); kept so an out-of-tree caller does not break, and so the selftest can pin the old contract.
def measure_target(main: str, unit: str, symbol: str):
    return resolve_target(main, main, unit, symbol)


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
    p = runner(cmd, cwd=main, capture_output=True, text=True, encoding="utf-8", errors="replace")
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
            result["measure"] = measure(target, result["object"], args.measure, unitutil.OBJDIFF,
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
