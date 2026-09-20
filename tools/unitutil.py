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
import json
import os
import re
import struct
import subprocess
import time
from dataclasses import dataclass

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


def repo_root(start=None):
    """Walk up from `start` (default: this file) until the directory that holds configure.py."""
    d = os.path.abspath(start or os.path.dirname(os.path.abspath(__file__)))
    while True:
        if os.path.exists(os.path.join(d, "configure.py")):
            return d
        parent = os.path.dirname(d)
        if parent == d:
            raise SystemExit("repo root (configure.py) not found above %s" % start)
        d = parent


ROOT = repo_root()


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


def _versions():
    build = os.path.join(ROOT, "build")
    return sorted(d for d in os.listdir(build)
                  if os.path.isdir(os.path.join(build, d, "obj"))) if os.path.isdir(build) else []


def _find_src(lib, file):
    for ext in SOURCE_EXT:
        p = os.path.join(ROOT, "src", lib, file + ext)
        if os.path.exists(p):
            return p
    return None


def _make(lib, file, version):
    src = _find_src(lib, file)
    if src is None:
        raise SystemExit("no source for unit %s/%s under src/" % (lib, file))
    return Unit(name="main/%s/%s" % (lib, file), lib=lib, file=file, version=version, src=src,
                obj_dir=os.path.join(ROOT, "build", version, "src", lib),
                obj=os.path.join(ROOT, "build", version, "src", lib, file + ".o"),
                target=os.path.join(ROOT, "build", version, "obj", lib, file + ".o"))


def list_units():
    """Every configured unit that has source in src/ (so the flag tools can work on it)."""
    out = []
    for version in _versions():
        for lib in sorted(os.listdir(os.path.join(ROOT, "src"))):
            d = os.path.join(ROOT, "src", lib)
            if not os.path.isdir(d):
                continue
            for entry in sorted(os.listdir(d)):
                if entry.endswith(SOURCE_EXT):
                    out.append(_make(lib, os.path.splitext(entry)[0], version))
    return out


def resolve_unit(spec=None):
    """Resolve a unit spec (see the module docstring). With no spec, use the only unit there is."""
    units = list_units()
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
    if len(parts) >= 3 and parts[0] in _versions():      # build/<ver>/{src,obj}/<lib>/<file>.o
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
    return _make(lib, file, units[0].version if units else _versions()[0])


def compile_command(unit):
    """The exact command line ninja would run for this unit, as a token list."""
    target = os.path.relpath(unit.obj, ROOT)
    p = subprocess.run(["ninja", "-t", "commands", target], cwd=ROOT,
                       capture_output=True, text=True, errors="replace")
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
    p = subprocess.run(tokens, cwd=ROOT, capture_output=True, text=True, errors="replace")
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


def objdiff(unit, symbol, out=None):
    """Run `objdiff-cli diff` for a unit in project mode; returns (json path, output).

    A symbol argument is required for symbol/instruction level output in the pinned objdiff-cli.
    """
    out = out or os.path.join(ROOT, "build", "tmp", "%s_%s_diff.json"
                              % (unit.lib, unit.file))
    os.makedirs(os.path.dirname(out), exist_ok=True)
    p = subprocess.run([OBJDIFF, "diff", "-p", ".", "-u", unit.name, symbol,
                        "--format", "json", "-o", out],
                       cwd=ROOT, capture_output=True, text=True, errors="replace")
    return (out if p.returncode == 0 else None), (p.stdout or "") + (p.stderr or "")


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
    """Same command line, but with the compiler version component replaced."""
    out = []
    for t in head:
        if t.endswith("mwcceppc.exe"):
            parts = re.split(r"[\\/]", t)
            parts[-2] = version
            sep = "\\" if "\\" in t else "/"
            out.append(sep.join(parts))
        else:
            out.append(t)
    return out


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
