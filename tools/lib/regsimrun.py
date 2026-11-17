"""Run MWCC under the debugger on one unit's compile command, find or fetch the gdb, and prove the Wii/1.3 stand-in.
Spec: docs/tools/spec/lib-regsimrun.md. CLI: none (library; tools/mwcc-debugger/regsim.py `run` is the front end)."""
from __future__ import annotations

import os
import re
import shutil
import subprocess
import sys
from dataclasses import dataclass, field
from pathlib import Path

from tools.lib import objcompare, regsim, units

#: The one compiler the debugger's address tables are verified for (`tools/mwcc-debugger/README.md`).
STAND_IN = "Wii/1.3"
#: Where a slot keeps its gdb: `<root>/.pi/tmp/mwcc-dbg/mingw64/bin/gdb.exe` (`fetch_gdb.py --dest` writes `mingw64/`).
GDB_DIR = (".pi", "tmp", "mwcc-dbg")
GDB_EXE = ("mingw64", "bin", "gdb.exe")
#: Where one run writes its dumps and objects: `<root>/.pi/tmp/regsim-run/<unit>/<function>/`.
RUN_DIR = (".pi", "tmp", "regsim-run")


# --- the command line -------------------------------------------------------------------------------------------

def quote_arg(tok: str) -> str:
    """`tok` spelled so `mwcc_debugger.split_command_line` reads it back as one argument: unquoted when plain, else
    in double quotes (single quotes when it holds a double quote). `ValueError` for a token with both."""
    if tok and not re.search(r"[\s\"']", tok):
        return tok
    if '"' not in tok:
        return '"%s"' % tok
    if "'" not in tok:
        return "'%s'" % tok
    raise ValueError("cannot quote %r for the debugger's command line" % tok)


def command_line(tokens: list[str]) -> str:
    """The debugger's `-a` argument for `tokens`."""
    return " ".join(quote_arg(t) for t in tokens)


@dataclass
class Compile:
    """A unit's compile command split the way the debugger needs it: the compiler, its flags, the source."""
    compiler: str
    flags: list[str]
    source: str
    version: str | None

    def tokens(self, out_dir: str, compiler: str | None = None, flags: list[str] | None = None) -> list[str]:
        """The bare compiler invocation (no wrapper, no post-processing) writing its object into `out_dir`."""
        return [compiler or self.compiler] + list(self.flags if flags is None else flags) + [
            "-c", self.source, "-o", out_dir]


def split_compile(cmd: list[str]) -> Compile:
    """A ninja/`units.rewrite` command line (`cmd /c sjiswrap mwcceppc FLAGS -MMD -c SRC -o DIR && ...`) as a
    `Compile`. `SystemExit` when it has no compiler or no `-c`."""
    try:
        head, flags, tail = units.split_flags(cmd)
        compiler = units.compiler_token(head)
    except StopIteration:
        raise SystemExit("regsim run: the unit's command line has no mwcceppc.exe ... -MMD -c SRC part") from None
    if "-c" not in tail or tail.index("-c") + 1 >= len(tail):
        raise SystemExit("regsim run: the unit's command line has no -c SOURCE")
    return Compile(compiler, [f for f in flags if f != "-MMD"], tail[tail.index("-c") + 1],
                   units.compiler_version(head))


def stand_in_compiler(compiler: str, version: str = STAND_IN) -> str:
    """`compiler` with its version directory replaced (`.../GC/3.0a5.2/mwcceppc.exe` -> `.../Wii/1.3/...`)."""
    return units.compiler_token(units.with_compiler_version([compiler], version))


def unit_compile(spelling: str, root: str, main: str) -> tuple[Compile, list[str], list[str]]:
    """(split command, full command, notes) of the compile `recompile.py --dry-run` prints for the unit: MAIN's graph
    re-read from this tree's configure.py, pointed at this tree's source, paths absolute."""
    tokens, _where = units.unit_tokens(main, root, spelling)
    tokens, notes = units.reconcile_flags(tokens, root, spelling)
    cmd, _obj = units.rewrite(tokens, spelling, main, root)
    cmd = units.absolutize(cmd, main)
    return split_compile(cmd), cmd, notes


# --- gdb ---------------------------------------------------------------------------------------------------------

def slot_gdb(root: str | os.PathLike) -> Path:
    return Path(root).joinpath(*GDB_DIR, *GDB_EXE)


def locate_gdb(root: str | os.PathLike, explicit: str | None = None, which=shutil.which,
               fetch=None) -> tuple[str, str]:
    """(gdb path, how it was found): `--gdb`, the slot's own copy, one on `PATH`, else fetched into the slot with
    `fetch(dest)` (default `fetch_gdb`). `SystemExit` when none can be had."""
    if explicit:
        if not os.path.exists(explicit):
            raise SystemExit("regsim run: --gdb %s does not exist" % explicit)
        return os.path.abspath(explicit), "--gdb"
    mine = slot_gdb(root)
    if mine.is_file():
        return str(mine), "slot copy"
    found = which("gdb") or which("gdb.exe")
    if found:
        return found, "PATH"
    dest = Path(root).joinpath(*GDB_DIR)
    (fetch or fetch_gdb)(str(dest))
    if not mine.is_file():
        raise SystemExit("regsim run: fetching gdb into %s did not produce %s" % (dest, mine))
    return str(mine), "fetched into %s" % dest


def fetch_gdb(dest: str) -> None:
    """Fetch the mingw-w64 gdb into `dest` with `tools/mwcc-debugger/fetch_gdb.py` (needs the network once)."""
    script = Path(__file__).resolve().parents[1] / "mwcc-debugger" / "fetch_gdb.py"
    p = subprocess.run([sys.executable, str(script), "--dest", dest], capture_output=True, text=True,
                       encoding="utf-8", errors="replace")
    if p.returncode != 0:
        raise SystemExit("regsim run: no gdb and fetching one failed (rc %d):\n%s" % (p.returncode, (p.stdout + p.stderr)[-800:]))


# --- compiling and dumping --------------------------------------------------------------------------------------

def compile_into(cmd: list[str], root: str, scratch: str, compiler: str | None = None,
                 retry_unknown: bool = False) -> tuple[int, str, str, list[str]]:
    """(rc, log, object path, dropped flags): `cmd` (the full command line `unit_compile` gives) run with its object
    in `scratch` and, when given, `compiler` in place of its own. With `retry_unknown` an option the compiler does
    not know is dropped and the compile repeated (the stand-in does not know a later compiler's flags)."""
    dropped: list[str] = []
    tokens = list(cmd)
    if compiler:
        old = units.compiler_token(tokens)
        tokens = [compiler if t == old else t for t in tokens]
    while True:
        rc, log, obj = units.run_tokens(tokens, root, scratch_dir=scratch)
        if rc == 0 or not retry_unknown:
            return rc, log, obj, dropped
        head, flags, tail = units.split_flags(tokens)
        fewer = units.drop_unknown_option(flags, log)
        if fewer is None:
            return rc, log, obj, dropped
        dropped += [f for f in flags if f not in fewer]
        tokens = head + fewer + tail


def run_dump(gdb: str, tokens: list[str], function: str, out_dir: str, root: str, size: str | None = None,
             timeout: float = 900.0) -> tuple[int, str]:
    """(rc, output) of `mwcc_debugger.py` dumping `function` while it compiles `tokens` (a bare compiler
    invocation); the dumps land in `out_dir`."""
    script = Path(__file__).resolve().parents[1] / "mwcc-debugger" / "mwcc_debugger.py"
    argv = [sys.executable, str(script), "--gdb", gdb, "--timeout", str(timeout), "-a", command_line(tokens),
            "--symbol", function]
    if size:
        argv += ["--size", size]
    p = subprocess.run(argv + [out_dir], cwd=root, capture_output=True, text=True, encoding="utf-8",
                       errors="replace")
    return p.returncode, (p.stdout or "") + (p.stderr or "")


def dumped_symbol(dump_dir: str | os.PathLike) -> str | None:
    """The link name the dump's `; function` line names (what the object's symbol is called)."""
    try:
        return regsim.load(dump_dir).function or None
    except (FileNotFoundError, ValueError):
        return None


# --- is the stand-in's function the unit's own -------------------------------------------------------------------

@dataclass
class Comparison:
    """One function in two objects: bytes, and the relocation targets (pool/local `@N` names folded)."""
    symbol: str
    real_size: int | None
    other_size: int | None
    same_bytes: bool
    same_relocs: bool
    first_diff: int | None = None
    notes: list[str] = field(default_factory=list)

    @property
    def identical(self) -> bool:
        return self.same_bytes and self.same_relocs


def _function_relocs(obj: str, offset: int, size: int) -> list[tuple[int, str, int, int]]:
    rows, _why = objcompare.reloc_rows(obj)
    out = []
    for off, sym, typ, add in (rows or {}).get(".text", []):
        if offset <= off < offset + size:
            out.append((off - offset, re.sub(r"@\d+", "@N", sym), typ, add))
    return sorted(out)


def compare_function(real_obj: str, other_obj: str, symbol: str) -> Comparison:
    """Whether `symbol` of `other_obj` is `real_obj`'s: the same bytes (relocation fields are zero in an
    unlinked object) and the same relocation targets."""
    syms = [objcompare.section_symbols(o, ".text") for o in (real_obj, other_obj)]
    missing = [name for name, s in zip(("the unit's own object", "the stand-in's object"), syms) if symbol not in s]
    if missing:
        return Comparison(symbol, syms[0].get(symbol, (0, None))[1], syms[1].get(symbol, (0, None))[1], False, False,
                          None, ["%s has no %s" % (" or ".join(missing), symbol)])
    (ro, rs), (oo, os_) = syms[0][symbol], syms[1][symbol]
    rd = (objcompare.section_data(real_obj, ".text") or b"")[ro:ro + rs]
    od = (objcompare.section_data(other_obj, ".text") or b"")[oo:oo + os_]
    return Comparison(symbol, rs, os_, rd == od,
                      _function_relocs(real_obj, ro, rs) == _function_relocs(other_obj, oo, os_),
                      objcompare.first_difference(rd, od))
