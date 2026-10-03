#!/usr/bin/env python3
"""Shared helpers for the flag tools: resolve a unit, get its real compile command, override flags.

Nothing in here is specific to a unit or a game: everything is derived from the repository
(`objdiff.json`, `build.ninja`, the `src/` tree), so the tools work for any translation unit.

A *unit spec* is any of these spellings:

    <Lib>/<file>                             project-relative, without extension
    main/<Lib>/<file>                        objdiff unit name
    src/<Lib>/<file>.c                       source path
    <file> | main/<file> | src/<file>.cpp    a top-level unit (`main`, `mh3_pad`, ...); a bare stem shared
                                             by two units is refused, listing the candidates
    build/<version>/src/<Lib>/<file>.o       a built object (the target object works too)

    (in this repo today that is `Camellia/camellia`, the only unit with source)

The compile command is taken from ninja (`ninja -t commands <obj>`), i.e. it is the *exact* command
line the build would run, including whatever `configure.py` put in that unit's `cflags`.
"""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import atexit
import os
import re
import shutil
import struct
import subprocess
import tempfile
import time
from dataclasses import dataclass

from tools.lib import proc as _proc
from tools.lib import repo as _repo
from tools.lib import report as _report
from tools.lib import units as _units
from tools.lib.binary.elf import Elf

# a process launch Windows refuses transiently (WinError 5) is retried, for every tool that imports this module
_proc.install_spawn_retry()


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
SOURCE_EXT = _units.SOURCE_EXT


def caller_worktree(start=None):
    return _repo.caller_worktree(start)


def _cwd_tree() -> str | None:
    return _repo.cwd_tree()


def repo_root(start=None):
    return _repo.repo_root(start)


def _root():
    """`ROOT`, resolved on first use (an assignment to `unitutil.ROOT` pins it) - spec lib-repo, gap 1."""
    root = globals().get("ROOT")
    if root is None:
        root = globals()["ROOT"] = repo_root()
    return root


def __getattr__(name):
    if name == "ROOT":
        return _root()
    if name == "OBJDIFF":
        return _objdiff()
    raise AttributeError("module %r has no attribute %r" % (__name__, name))


def _serves(root) -> bool:
    """Whether `root` is the tree the tools serve (`$MHTRI_MAIN` applies to it, never to a fixture)."""
    return os.path.normcase(os.path.abspath(root)) == os.path.normcase(_root())


def main_tree(root=None):
    root = root or _root()
    return _repo.main_tree(root, honour_env=_serves(root))


def resolve_input(rel, root=None, probe=os.path.exists):
    root = root or _root()
    return _repo.resolve_input(rel, root, probe, honour_env=_serves(root))


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
    if top and os.path.normcase(os.path.abspath(top)) != os.path.normcase(os.path.abspath(_root())):
        print("note: this tool compiles %s - you are in %s, whose edits it cannot see. "
              "Use tools/units/recompile.py to measure your own tree." % (_root(), top), file=sys.stderr)


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
    return _units.versions(root or _root())


def _as_unit(u: "_units.Unit") -> Unit:
    """The old mutable `Unit` shape of a `lib.units.Unit`."""
    return Unit(name=u.report_name, lib=u.module, file=u.file, version=u.version, src=u.source,
                obj_dir=u.obj_dir, obj=u.obj_ours, target=u.obj_target)


def _make(lib, file, version, root=None):
    """A `Unit`; `lib` is "" for a top-level unit (`src/<file>.cpp`, objdiff name `main/<file>`)."""
    return _as_unit(_units.Unit.make(lib + "/" + file if lib else file, root or _root(), version))


def list_units(root=None):
    """Every configured unit that has source in `root`'s `src/` (`root` defaults to `ROOT`)."""
    return [_as_unit(u) for u in _units.Unit.list(root or _root())]


def resolve_unit(spec=None, root=None):
    """Resolve a unit spec (the module docstring; `lib.units.Unit.resolve`) in `root` (default `ROOT`)."""
    return _as_unit(_units.Unit.resolve(spec, root or _root()))


def compile_command(unit):
    """The exact command line ninja would run for this unit, as a token list."""
    warn_if_foreign_worktree()
    target = os.path.relpath(unit.obj, _root())
    lines, p = _units.ninja_lines(_root(), target)
    if not lines:
        raise SystemExit("could not get the compile command for %s from ninja:\n%s%s"
                         % (target, p.stdout, p.stderr))
    return unquote(lines[-1].split())


unquote = _units.unquote


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
    """Run a compile command in `ROOT` -> (rc, output, object path) (`lib.units.run_tokens`)."""
    warn_if_foreign_worktree()
    return _units.run_tokens(tokens, _root(), expect=expect, scratch_dir=scratch_dir, src=src, verbose=verbose)


def quiet(out):
    """Drop MWCC's informational banner lines from its output."""
    return "\n".join(l for l in out.splitlines() if not l.startswith("###"))


# --- minimal ELF32 big-endian reader (enough for MWCC objects) --------------------------------

def read_elf(path):
    """(sections, symbols) of an ELF32 big-endian object; symbols are (name, value, size, type, shndx).

    A view over `lib.binary.elf.Elf`: section dicts keep `name` (the string offset), `sname`, `typ`, `off`,
    `size`, `link`, `entsize`, `data`; symbols skip the unnamed and the undefined rows."""
    elf = Elf.read(path)
    secs = [dict(name=s.name_offset, typ=s.type, off=s.offset, size=s.size, link=s.link, entsize=s.entsize,
                 data=s.raw, sname=s.name) for s in elf.sections]
    syms = [(s.name, s.value, s.size, s.type, s.shndx) for s in elf.symbols
            if s.name and s.shndx]
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

def _objdiff():
    """`OBJDIFF`: the tree's objdiff-cli, resolved on first use (an assignment to `unitutil.OBJDIFF` pins it)."""
    return globals().get("OBJDIFF") or os.path.join(_root(), "build", "tools", "objdiff-cli.exe")


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
    p = runner([_objdiff(), "diff", "-p", ".", "-u", unit.name, symbol,
                "-c", "functionRelocDiffs=none", "--format", "json", "-o", out],
               cwd=_root(), capture_output=True, text=True, encoding="utf-8", errors="replace")
    return (out if p.returncode == 0 else None), (p.stdout or "") + (p.stderr or "")


# --- official scoring: `report generate`, not `diff` (lib.report.score) -------------------------

MIN_PROJECT_VERSION = _report.MIN_PROJECT_VERSION


def session_tmpdir() -> str:
    return _repo.session_tmpdir()


def measure_project(target, base, unit_name, tmpdir):
    """Write a one-unit objdiff project for (`target`, `base`); return its directory."""
    return _report.write_project(target, base, unit_name, tmpdir)


def report_functions(target, base, unit_name=None, tmpdir=None, runner=subprocess.run):
    """{function: report entry} for one object pair, scored by `report generate` - the official metric
    (`lib.report.score_entries`); an error is `{"_error": <text>}`, never a 0.0 score."""
    return _report.score_entries(target, base, unit_name, tmpdir or session_tmpdir(), objdiff=_objdiff(),
                                 cwd=_root(), runner=runner)


def report_measure(target, base, symbol, unit_name=None, tmpdir=None, runner=subprocess.run):
    """The official score of one symbol (`lib.report.symbol_score`)."""
    return _report.symbol_score(target, base, symbol, unit_name, tmpdir or session_tmpdir(), objdiff=_objdiff(),
                                cwd=_root(), runner=runner)


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
            print("  %-32s %s" % (u.name, os.path.relpath(u.src, _root())))
        return
    u = resolve_unit(spec)
    head, flags, tail = split_flags(compile_command(u))
    print("unit    ", u.name)
    print("src     ", os.path.relpath(u.src, _root()))
    print("obj     ", os.path.relpath(u.obj, _root()))
    print("target  ", os.path.relpath(u.target, _root()))
    print("compiler", " ".join(head))
    print("flags   ", " ".join(flags))
    print("tail    ", " ".join(tail))


if __name__ == "__main__":
    import sys
    main()
