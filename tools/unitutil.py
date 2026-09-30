#!/usr/bin/env python3
"""Shared helpers for the flag tools: resolve a unit, get its real compile command, override flags.

Nothing in here is specific to a unit or a game: everything is derived from the repository
(`objdiff.json`, `build.ninja`, the `src/` tree), so the tools work for any translation unit.

A *unit spec* is any of these spellings:

    <Lib>/<file>                             project-relative, without extension
    main/<Lib>/<file>                        objdiff unit name
    src/<Lib>/<file>.c                       source path
    build/<version>/src/<Lib>/<file>.o       a built object (the target object works too)

    (in this repo today that is `Camellia/camellia`, the only unit with source)

The compile command is taken from ninja (`ninja -t commands <obj>`), i.e. it is the *exact* command
line the build would run, including whatever `configure.py` put in that unit's `cflags`.
"""
import atexit
import json
import os
import re
import shutil
import struct
import subprocess
import sys
import tempfile
import time
from dataclasses import dataclass

import spawnretry

# a process launch Windows refuses transiently (WinError 5) is retried, for every tool that imports this module
spawnretry.install()


def rmtree_retry(path: str, attempts: int = 8) -> None:
    """`shutil.rmtree` that survives Windows holding a file for a moment (a scanner, a just-exited git).

    Retries `PermissionError`/`OSError` with a growing backoff, clears the read-only bit git sets on
    `.git/objects`, and finally gives up silently: a leftover temp directory must never fail a test.
    """
    def _onerror(func, p, _exc):
        try:
            os.chmod(p, 0o700)
            func(p)
        except OSError:
            pass
    for attempt in range(attempts):
        if not os.path.exists(path):
            return
        try:
            if sys.version_info >= (3, 12):
                shutil.rmtree(path, onexc=_onerror)
            else:
                shutil.rmtree(path, onerror=_onerror)
        except OSError:
            pass
        if not os.path.exists(path):
            return
        time.sleep(0.1 * (attempt + 1))
    shutil.rmtree(path, ignore_errors=True)


def isolate_live_state() -> str:
    """Point `CLAUDE_CONFIG_DIR` at an empty temp dir for the rest of this process; returns it.

    `slots.live_runs()` reads `<config dir>/sessions/*.json` - the REAL live lanes - so a selftest that calls
    a slot guard without a fixture registry sees whichever lanes happen to be running when the gate runs, and
    passes or fails with them. A selftest calls this first: no live sessions exist for it.
    """
    path = tempfile.mkdtemp(prefix="claude-config-")
    os.environ["CLAUDE_CONFIG_DIR"] = path
    atexit.register(rmtree_retry, path)
    return path


class temp_dir:
    """`tempfile.TemporaryDirectory()` with `rmtree_retry` cleanup: `with unitutil.temp_dir() as tmp:`.

    Selftests that build a temp git repo or worktree used `TemporaryDirectory`, whose cleanup raises
    `PermissionError [WinError 5]` when Windows still holds a file - failing a test that had passed.
    """

    def __init__(self, prefix: str = "mhtri-"):
        self.name = tempfile.mkdtemp(prefix=prefix)

    def __enter__(self) -> str:
        return self.name

    def __exit__(self, *exc) -> None:
        rmtree_retry(self.name)
        return None

    def cleanup(self) -> None:
        rmtree_retry(self.name)

# Options that take a following value token, as used by this project's configure.py. Used only to
# remove a conflicting earlier occurrence when `--flags-extra` overrides the same option.
VALUED = {
    "-proc": 1, "-align": 1, "-enum": 1, "-fp": 1, "-Cpp_exceptions": 1, "-inline": 1,
    "-pragma": 1, "-maxerrors": 1, "-RTTI": 1, "-fp_contract": 1, "-str": 1, "-i": 1, "-ir": 1,
    "-I": 1, "-use_lmw_stmw": 1, "-common": 1, "-lang": 1, "-opt": 1, "-pool": 1, "-schedule": 1,
    "-sdata": 1, "-sdata2": 1, "-model": 1, "-abi": 1, "-encoding": 1, "-D": 0, "-U": 0,
    "-gccinc": 0, "-nodefaults": 0, "-nosyspath": 0, "-multibyte": 0, "-gcc": 0, "-rostr": 0,
}
SOURCE_EXT = (".c", ".cc", ".cp", ".cpp", ".cxx", ".c++")


def caller_worktree(start=None):
    """The git worktree the *caller* is in, or None when git cannot say."""
    try:
        p = subprocess.run(["git", "rev-parse", "--show-toplevel"], cwd=start or os.getcwd(),
                           capture_output=True, text=True, encoding="utf-8", errors="replace")
    except OSError:
        return None
    return (p.stdout or "").strip() or None if p.returncode == 0 else None


def _cwd_tree() -> str | None:
    """`cwd` when it is itself a tree (has `configure.py`), else None - the non-git invocation case."""
    try:
        cwd = os.getcwd()
    except OSError:
        return None
    return cwd if os.path.exists(os.path.join(cwd, "configure.py")) else None


def repo_root(start=None):
    """The tree the tools should read: the **caller's** tree, else the tree this file lives in.

    This used to be only the file's location, which silently read MAIN from inside a lane's worktree: a
    tool invoked as `python <MAIN>/tools/objdiff/symdiff.py -u <unit>` with cwd in a worktree scored
    MAIN's objects and printed MAIN's numbers - 0.91743 for a symbol the worktree's own build had at
    100.0, which read as "the merge destroyed 67 functions". The invocation's tree is what the caller
    means (`git rev-parse --show-toplevel`), and it is the same tree `recompile.py`/`measure.py` already
    take their source, `-o` and `-i` order from. When there is no git worktree (a temp dir, a packaged
    copy) the walk up to `configure.py` is unchanged, and an explicit `start` still roots the walk (the
    lane/teardown helpers pass one) so a caller can name a tree unambiguously.

    **A tree that is not a git worktree is named with `start`, or with `cwd`.**  A *fixture* (a fake
    repository under the system temp, the shape every `*_selftest.py` here uses) is not a git worktree,
    so `git rev-parse` answers nothing; before this, the walk then began at *this file's* directory and
    silently resolved the real repository - the fixture was scored against the real build, which is why
    `unitscore_selftest` had to `git init` its tree. Without `start`, the invocation's own tree is tried
    first (its git worktree, else `cwd` **when `cwd` is a tree at all**), and only a cwd that is not a
    tree falls back to this file's directory, so the packaged-copy case is unchanged. With `start`, the
    walk begins there and never reaches this file's directory.
    """
    if start is not None:
        d = os.path.abspath(start)
    else:
        top = caller_worktree() or _cwd_tree()
        d = top or os.path.dirname(os.path.abspath(__file__))
    while True:
        if os.path.exists(os.path.join(d, "configure.py")):
            return d
        parent = os.path.dirname(d)
        if parent == d:
            raise SystemExit("repo root (configure.py) not found above %s" % (start or d))
        d = parent


ROOT = repo_root()


def warn_if_foreign_worktree() -> None:
    """Say so when the caller sits in a worktree other than ROOT - a silent wrong-source measurement.

    With `repo_root` now resolving the caller's worktree this is mostly a safety net: it fires only when
    ROOT was pinned to a tree other than the caller's (an explicit import, or a `repo_root(start)`), which
    is the case where compiling here would measure a tree the caller cannot see. Everything here resolves
    the unit, its source and its object from ROOT and runs the compiler with `cwd=ROOT`; if that is not
    the caller's tree the number is not the caller's. `tools/units/recompile.py` is the worktree-aware
    path - it rewrites the source, the `-o` directory and the `-i` order to the caller's tree.
    """
    top = caller_worktree()
    if top and os.path.normcase(os.path.abspath(top)) != os.path.normcase(os.path.abspath(ROOT)):
        print("note: this tool compiles %s - you are in %s, whose edits it cannot see. "
              "Use tools/units/recompile.py to measure your own tree." % (ROOT, top), file=sys.stderr)


@dataclass
class Unit:
    name: str        # objdiff unit name, e.g. "main/<Lib>/<file>"
    lib: str         # library directory under src/
    file: str        # source file name without extension
    version: str     # build/<version>, e.g. the game id
    src: str         # absolute path to the source file
    obj_dir: str     # absolute output directory (MWCC's -o is a directory)
    obj: str         # absolute path of our object
    target: str      # absolute path of the original (split) object


def _versions(root=None):
    build = os.path.join(root or ROOT, "build")
    return sorted(d for d in os.listdir(build)
                  if os.path.isdir(os.path.join(build, d, "obj"))) if os.path.isdir(build) else []


def _find_src(lib, file, root=None):
    for ext in SOURCE_EXT:
        p = os.path.join(root or ROOT, "src", lib, file + ext)
        if os.path.exists(p):
            return p
    return None


def _make(lib, file, version, root=None):
    root = root or ROOT
    src = _find_src(lib, file, root)
    if src is None:
        raise SystemExit("no source for unit %s/%s under src/" % (lib, file))
    return Unit(name="main/%s/%s" % (lib, file), lib=lib, file=file, version=version, src=src,
                obj_dir=os.path.join(root, "build", version, "src", lib),
                obj=os.path.join(root, "build", version, "src", lib, file + ".o"),
                target=os.path.join(root, "build", version, "obj", lib, file + ".o"))


def list_units(root=None):
    """Every configured unit that has source in `root`'s `src/` (`root` defaults to `ROOT`)."""
    root = root or ROOT
    out = []
    for version in _versions(root):
        for lib in sorted(os.listdir(os.path.join(root, "src"))):
            d = os.path.join(root, "src", lib)
            if not os.path.isdir(d):
                continue
            for entry in sorted(os.listdir(d)):
                if entry.endswith(SOURCE_EXT):
                    out.append(_make(lib, os.path.splitext(entry)[0], version, root))
    return out


def resolve_unit(spec=None, root=None):
    """Resolve a unit spec (see the module docstring). With no spec, use the only unit there is.

    `root` names the tree to resolve against and defaults to `ROOT` (the invocation's tree).  Passing it
    is the same rule `repo_root(start=)` documents - the caller names the tree it means - and it is the
    only way to resolve a unit in a tree that is **not a git worktree**, i.e. a fixture: without it the
    fixture silently reads this module's own `ROOT` and refuses (or, worse, resolves the real unit).  The
    returned `Unit`'s `src`/`obj`/`target` are therefore absolute paths *under that root*.
    """
    units = list_units(root)
    if spec is None:
        if len(units) == 1:
            return units[0]
        raise SystemExit("--unit is required; candidates:\n  " +
                         "\n  ".join(u.name for u in units))
    s = spec.replace("\\", "/").strip()
    for pre in ("build/", "src/"):
        if s.startswith(pre):
            s = s[len(pre):]
    parts = [p for p in s.split("/") if p not in ("", ".")]
    if parts and parts[0] == "main":
        parts = parts[1:]
    if len(parts) >= 3 and parts[0] in _versions(root):      # build/<ver>/{src,obj}/<lib>/<file>.o
        parts = parts[2:]
    if not parts:
        raise SystemExit("cannot parse unit spec %r" % spec)
    file = os.path.splitext(parts[-1])[0]
    if len(parts) == 1:
        for u in units:
            if u.file == file:
                return u
        raise SystemExit("no unit with file name %r" % file)
    lib = parts[-2]
    for u in units:
        if u.lib == lib and u.file == file:
            return u
    return _make(lib, file, units[0].version if units else _versions(root)[0], root)


def compile_command(unit):
    """The exact command line ninja would run for this unit, as a token list."""
    warn_if_foreign_worktree()
    target = os.path.relpath(unit.obj, ROOT)
    p = subprocess.run(["ninja", "-t", "commands", target], cwd=ROOT,
                       capture_output=True, text=True, encoding="utf-8", errors="replace")
    lines = [l for l in (p.stdout or "").splitlines() if "mwcceppc" in l]
    if not lines:
        raise SystemExit("could not get the compile command for %s from ninja:\n%s%s"
                         % (target, p.stdout, p.stderr))
    return unquote(lines[-1].split())


def unquote(tokens):
    """Merge `"cats off"` (split into two tokens by .split()) back into one token, without quotes."""
    out, i = [], 0
    while i < len(tokens):
        t = tokens[i]
        if t.startswith('"') and not t.endswith('"'):
            j = i
            while j < len(tokens) and not tokens[j].endswith('"'):
                j += 1
            out.append(" ".join(tokens[i:j + 1]).strip('"'))
            i = j + 1
        else:
            out.append(t.strip('"'))
            i += 1
    return out


def split_flags(tokens):
    """(head, flags, tail): head = wrapper + compiler, tail = `-MMD -c <src> -o <dir>`."""
    head_end = next(i for i, t in enumerate(tokens) if t.startswith("-") and i > 0)
    tail_start = next(i for i, t in enumerate(tokens) if t == "-MMD")
    return tokens[:head_end], tokens[head_end:tail_start], tokens[tail_start:]


def family(tok):
    """The option family a flag token belongs to (`-O3`, `-O4,p` -> `-O`), or None."""
    if re.match(r"^-O\d", tok):
        return "-O"
    name = tok.split("=", 1)[0]
    return name if name in VALUED else None


def _value_len(tokens, i):
    """How many tokens after `tokens[i]` make up its value (handles quoted values with spaces)."""
    if VALUED.get(tokens[i], 0) == 0:
        return 0
    j = i + 1
    if j < len(tokens) and tokens[j].startswith('"') and not tokens[j].endswith('"'):
        while j < len(tokens) and not tokens[j].endswith('"'):
            j += 1
        return j - i
    return VALUED[tokens[i]]


def override_flags(flags, extra):
    """Apply `--flags-extra` to a flag list: drop earlier flags of the same family, then append.

    Dropping matters: `-O3` must replace the project's `-O4,p`, not sit next to it.
    """
    extras = extra.split()
    families = {f for f in (family(t) for t in extras) if f}
    out = []
    i = 0
    while i < len(flags):
        if family(flags[i]) in families:
            i += 1 + _value_len(flags, i)
            continue
        out.append(flags[i])
        i += 1
    return out + extras


def run_compile(tokens, expect=None, scratch_dir=None, src=None, verbose=False):
    """Run a compile command. Returns (rc, output, object path).

    * `expect`  - object path that must exist afterwards (staleness check).
    * `scratch_dir` - redirect MWCC's `-o` there, so the unit's real object is not clobbered.
    * `src`     - replace the `-c` source argument (used for source-rewrite experiments).
    """
    tokens = list(tokens)
    warn_if_foreign_worktree()
    obj = expect
    if src is not None:
        i = tokens.index("-c")
        tokens[i + 1] = src
        obj = os.path.join(scratch_dir, os.path.splitext(os.path.basename(src))[0] + ".o") \
            if scratch_dir else obj
    if scratch_dir is not None:
        i = tokens.index("-o")
        tokens[i + 1] = scratch_dir
        os.makedirs(scratch_dir, exist_ok=True)
        if src is None:
            j = tokens.index("-c")
            obj = os.path.join(scratch_dir,
                               os.path.splitext(os.path.basename(tokens[j + 1]))[0] + ".o")
    if obj and os.path.exists(obj):
        os.remove(obj)
    # The filesystem reports whole-second mtimes and objdiff caches on (mtime, size): make sure this
    # compile lands in a later second than the previous one.
    time.sleep(1.05)
    p = subprocess.run(tokens, cwd=ROOT, capture_output=True, text=True, encoding="utf-8", errors="replace")
    out = (p.stdout or "") + (p.stderr or "")
    if verbose:
        print("$ " + " ".join(tokens))
        print(out)
    if obj and not os.path.exists(obj):
        out += "\n!! object was NOT regenerated (%s) -- check the -o argument" % obj
        return 2, out, obj
    return p.returncode, out, obj


def quiet(out):
    """Drop MWCC's informational banner lines from its output."""
    return "\n".join(l for l in out.splitlines() if not l.startswith("###"))


# --- minimal ELF32 big-endian reader (enough for MWCC objects) --------------------------------

def read_elf(path):
    """(sections, symbols) of an ELF32 big-endian object; symbols are (name, value, size, type)."""
    data = open(path, "rb").read()
    assert data[:4] == b"\x7fELF", "%s is not an ELF file" % path
    (shoff,) = struct.unpack_from(">I", data, 0x20)
    (shentsize, shnum, shstrndx) = struct.unpack_from(">HHH", data, 0x2E)
    secs = []
    for i in range(shnum):
        o = shoff + i * shentsize
        name, typ, flags, addr, off, size, link, info, align, entsize = struct.unpack_from(
            ">IIIIIIIIII", data, o)
        secs.append(dict(name=name, typ=typ, off=off, size=size, link=link, entsize=entsize,
                         data=data[off:off + size]))
    shstr = secs[shstrndx]["data"]
    for s in secs:
        e = shstr.find(b"\0", s["name"])
        s["sname"] = shstr[s["name"]:e].decode()
    symtab = next(s for s in secs if s["typ"] == 2)
    strtab = secs[symtab["link"]]["data"]
    syms = []
    for o in range(0, symtab["size"], symtab["entsize"] or 16):
        nm, val, size, info, other, shndx = struct.unpack_from(">IIIBBH", symtab["data"], o)
        if nm == 0 or shndx == 0:
            continue
        e = strtab.find(b"\0", nm)
        syms.append((strtab[nm:e].decode(), val, size, info & 0xF, shndx))
    return secs, syms


def text_size(obj):
    secs, _ = read_elf(obj)
    return next(s for s in secs if s["sname"] == ".text")["size"]


def frames(obj):
    """[(name, size, frame)] for every function, in address order; frame is negative (or None)."""
    secs, syms = read_elf(obj)
    out = []
    for name, val, size, typ, shndx in sorted([s for s in syms if s[3] == 2], key=lambda x: x[1]):
        sec = secs[shndx]
        frame = None
        if len(sec["data"]) >= val + 4:
            word = struct.unpack_from(">I", sec["data"], val)[0]
            if word >> 26 == 37:                       # stwu r1, -N(r1)
                frame = -struct.unpack_from(">h", sec["data"], val + 2)[0]   # positive magnitude
        out.append((name, size, frame))
    return out


def function_names(obj):
    return [f[0] for f in frames(obj)]


# --- objdiff -----------------------------------------------------------------------------------

OBJDIFF = os.path.join(ROOT, "build", "tools", "objdiff-cli.exe")


def objdiff(unit, symbol, out=None, runner=subprocess.run):
    """Run `objdiff-cli diff` for a unit in project mode; returns (json path, output).

    A symbol argument is required for symbol/instruction level output in the pinned objdiff-cli.

    `out` defaults to this process's unique `session_tmpdir()` - the same helper `report_functions`/
    `report_measure` use. It used to be the shared per-file `build/tmp/<lib>_<file>_diff.json`, so two
    concurrent lanes measuring the same unit wrote and read the *same* file: the loser quoted the other
    run's rows (or hit a `PermissionError` while the file was held). A per-process directory cannot
    collide, and it lives in the system temp, not under the tree the selftest's dirty-guard watches.

    `-c functionRelocDiffs=none` is passed explicitly: `report generate`'s default is `none` while
    `diff`'s is `data_value`, so without it the rows a tool sees disagree with the official
    classification (relocation-only differences show up as `DIFF_ARG_MISMATCH`).

    The per-symbol `match_percent` in this JSON is objdiff's **positional** value, not the campaign's
    metric - it is neither the reloc-independent numbers nor the report's normalisation. Use
    `report_measure()` / `report_functions()` for a score, and this only for row detail.
    """
    out = out or os.path.join(session_tmpdir(), "%s_%s_diff.json" % (unit.lib, unit.file))
    os.makedirs(os.path.dirname(out), exist_ok=True)
    p = runner([OBJDIFF, "diff", "-p", ".", "-u", unit.name, symbol,
                "-c", "functionRelocDiffs=none", "--format", "json", "-o", out],
               cwd=ROOT, capture_output=True, text=True, encoding="utf-8", errors="replace")
    return (out if p.returncode == 0 else None), (p.stdout or "") + (p.stderr or "")


# --- official scoring: `report generate`, not `diff` --------------------------------------------
#
# objdiff-cli's `diff` mode is not the metric that closes a symbol, and two differences compound:
#   * `diff` defaults `functionRelocDiffs` to `data_value` while `report generate` defaults to `none`,
#     so relocation-only differences are counted as mismatches by the former (measured on this repo:
#     `pl_skill` fn_80270018 reads 99.88 % in diff mode and **100.0 %** in the report);
#   * even at the same setting, the diff JSON's `match_percent` is a different normalisation from the
#     report's `fuzzy_match_percent` (`RSOStaticLocateObject` 99.38461 vs 99.64103; the gap reaches
#     1.25 pt on `main`'s fn_8003F730).
# `report generate` over a one-unit project is the report code path by construction - the number
# `build/RMHE08/report.json`, `ledger.py`, `brief.py` and `land.py` read - and it costs ~0.04 s.
# `tools/units/recompile.py` carries the same primitive for the no-ninja worker path; keep them in sync.

MIN_PROJECT_VERSION = "2.0.0-beta.5"


_TMPDIR = None


def session_tmpdir() -> str:
    """A **unique** scratch directory for this process, removed at exit.

    The default used to be the shared `build/tmp/unitutil`, and every tool that did not pass `tmpdir`
    (`tryvar.py`, `slotmap.py`, `recompile.py`'s single-symbol path, `report_measure` called with no
    tmpdir) wrote its project/report there. Two concurrent invocations raced on
    `unitutil_report.json`, and the loser saw a report the *other* run had just written (or a
    `PermissionError [WinError 5]` while the file was held) - the same collision that cost `symdiff.py` a
    measurement round (see `tools/objdiff/symdiff.py`) and was fixed there with a per-invocation directory.
    `objdiff()`'s per-file `build/tmp/<lib>_<file>_diff.json` was the same shape of race and now routes
    through this helper too.

    One directory per process (not per call) keeps `report_measure`'s returned `report_json` path equal to
    the file `report_functions` just wrote, while two processes never share one. It lives in the system
    temp, not under the repo, so a measurement cannot dirty the tree the selftest's dirty-guard watches.
    """
    global _TMPDIR
    if _TMPDIR is None:
        _TMPDIR = tempfile.mkdtemp(prefix="unitutil-")
        atexit.register(shutil.rmtree, _TMPDIR, ignore_errors=True)
    return _TMPDIR


def measure_project(target, base, unit_name, tmpdir):
    """Write a one-unit objdiff project for (`target`, `base`); return its directory.

    `report generate` resolves `target_path`/`base_path` against the project directory, and on Windows
    only a backslash-rooted path counts as absolute (`C:/...` is joined and mangled into `C:...`), so
    both are absolutised with `os.path.abspath` - that shape on Windows, a plain absolute path elsewhere.
    """
    proj = os.path.join(tmpdir, "unitutil_project")
    os.makedirs(proj, exist_ok=True)
    with open(os.path.join(proj, "objdiff.json"), "w", encoding="utf-8") as fh:
        json.dump({"min_version": MIN_PROJECT_VERSION,
                   "units": [{"name": unit_name or "measure",
                              "target_path": os.path.abspath(target),
                              "base_path": os.path.abspath(base)}]}, fh, indent=2)
    return proj


def report_functions(target, base, unit_name=None, tmpdir=None, runner=subprocess.run):
    """{function: report entry} for one object pair, scored by `report generate` - the official metric.

    Each entry carries `fuzzy_match_percent` and `size` exactly as `build/RMHE08/report.json` does, so
    a consumer that wants the official score reads `[name]["fuzzy_match_percent"]`. An error is returned
    as `{"_error": <text>}` (never as a 0.0 score); the one-unit project's report is left in `tmpdir`,
    which defaults to this process's unique `session_tmpdir()` so concurrent tools cannot collide.
    """
    tmpdir = tmpdir or session_tmpdir()
    os.makedirs(tmpdir, exist_ok=True)
    proj = measure_project(target, base, unit_name, tmpdir)
    out = os.path.join(tmpdir, "unitutil_report.json")
    if os.path.exists(out):
        os.remove(out)
    p = runner([OBJDIFF, "report", "generate", "-p", proj, "-o", out],
               cwd=ROOT, capture_output=True, text=True, encoding="utf-8", errors="replace")
    if p.returncode != 0 or not os.path.exists(out):
        return {"_error": "objdiff report generate failed: " + (p.stdout or "") + (p.stderr or "")}
    data = json.load(open(out, encoding="utf-8"))
    units = data.get("units") or []
    return {f.get("name"): f for f in ((units[0].get("functions") if units else []) or [])}


def report_measure(target, base, symbol, unit_name=None, tmpdir=None, runner=subprocess.run):
    """The official (`fuzzy_match_percent`) score for one symbol of an object pair.

    Returns `{"symbol", "match_percent", "target_size", "report_json"}` or `{"error": ...}`.
    `match_percent` is deliberately the **report** metric so that any consumer reading it gets the
    number that closes a symbol; the positional objdiff value is not exposed here (use `objdiff()` if
    row detail is what is wanted). `tmpdir` defaults to this process's unique `session_tmpdir()`.
    """
    tmpdir = tmpdir or session_tmpdir()
    entries = report_functions(target, base, unit_name=unit_name, tmpdir=tmpdir, runner=runner)
    if "_error" in entries:
        return {"symbol": symbol, "error": entries["_error"]}
    fn = entries.get(symbol)
    if fn is None:
        return {"symbol": symbol,
                "error": "symbol is not in the target object (renamed? not in this unit?)"}
    report_json = os.path.join(tmpdir, "unitutil_report.json")
    return {"symbol": symbol, "match_percent": fn.get("fuzzy_match_percent"),
            "target_size": fn.get("size"), "report_json": report_json}


def any_function(obj):
    """A function name from an object, for the objdiff symbol argument."""
    names = function_names(obj)
    if not names:
        raise SystemExit("no function symbols in %s" % obj)
    return names[0]


# --- compiler selection ------------------------------------------------------------------------

def compiler_token(head):
    """The MWCC executable token of a split command line (e.g. build\\compilers\\Wii\\1.3\\mwcceppc.exe)."""
    return next(t for t in head if t.endswith("mwcceppc.exe"))


def with_compiler_version(head, version):
    """Same command line, but with the compiler version component replaced.

    `version` is a bare version ("1.3", the unit's own family) or a cross-family spec
    ("GC/3.0a3").  The cross-family form is how a unit's compiler family is verified: a unit can come
    from a different toolchain than the rest of the game (prebuilt SDK libraries in particular), and
    the target object's `.comment` cannot answer that because it is synthesized from `config.yml`
    (see docs/matching.md 17).
    """
    family, _, ver = version.partition("/")
    out = []
    for t in head:
        if t.endswith("mwcceppc.exe"):
            parts = re.split(r"[\\/]", t)
            parts[-2] = ver if ver else family
            if ver:
                parts[-3] = family
            sep = "\\" if "\\" in t else "/"
            out.append(sep.join(parts))
        else:
            out.append(t)
    return out


def drop_unknown_option(flags, log):
    """Remove the option MWCC rejected, so a matrix can span compiler generations.

    Older compilers do not know every option the unit's command line uses (GC 1.x rejects
    `-gccinc`, for instance), which would otherwise abort the whole sweep.
    """
    m = re.search(r"Unknown option '([^']+)'", log)
    if not m:
        return None
    bad = m.group(1)
    out = list(flags)
    for i, f in enumerate(out):
        if f == bad or (f.startswith("-") and bad in f):
            del out[i]
            if i < len(out) and not out[i].startswith("-"):
                del out[i]
            return out
    return None


def available_versions(head):
    """Compiler versions installed next to the one this unit uses."""
    d = os.path.dirname(compiler_token(head))
    return sorted(os.listdir(d)) if os.path.isdir(d) else []


def main():
    """`python tools/unitutil.py [-u] [spec]` - list units, or show what the tools would resolve."""
    args = sys.argv[1:]
    spec = None
    for flag in ("-u", "--unit"):
        if flag in args:
            i = args.index(flag)
            spec = args[i + 1] if i + 1 < len(args) else None
            del args[i:i + 2]
    if spec is None and args:
        spec = args[0]
    units = list_units()
    if spec is None:
        print("%d unit(s) with source in this repo:" % len(units))
        for u in units:
            print("  %-32s %s" % (u.name, os.path.relpath(u.src, ROOT)))
        return
    u = resolve_unit(spec)
    head, flags, tail = split_flags(compile_command(u))
    print("unit    ", u.name)
    print("src     ", os.path.relpath(u.src, ROOT))
    print("obj     ", os.path.relpath(u.obj, ROOT))
    print("target  ", os.path.relpath(u.target, ROOT))
    print("compiler", " ".join(head))
    print("flags   ", " ".join(flags))
    print("tail    ", " ".join(tail))


if __name__ == "__main__":
    import sys
    main()
