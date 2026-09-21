#!/usr/bin/env python3
"""Turn a target object's disassembly into the assembly `m2c` reads.

    python tools/units/m2cinput.py build/RMHE08/obj/<unit>.o [-f name]... [-o out.s] [--list]

`m2c` (the `tools/m2c` submodule) recovers C from GNU-as style assembly, which makes it a second shape
oracle next to the Ghidra decompiler - and the only one that works offline. Our target objects are
`build/RMHE08/obj/<unit>.o`, and `powerpc-eabi-objdump -dr` prints them in a shape m2c rejects (a
`00000000 <LocateObject>:` header per symbol, and an address in front of every instruction), so this
translates one into the other:

    glabel fn_804DA7E4
    lwz r6, 12(r3)
    bl RSORelocate
    beq loc_2c8
    ...

    python tools/units/m2cinput.py build/RMHE08/obj/RSO/runtime.o -f fn_804DA7E4 -o build/tmp/x.s
    python tools/m2c/m2c.py -t ppc-mwcc-c --no-cache -f fn_804DA7E4 build/tmp/x.s

What it rewrites, and why each one has to happen:

* Relocations become the spelling m2c expects: `R_PPC_REL24` the target symbol, `R_PPC_ADDR16_HA`/`_HI`/
  `_LO` `sym@ha`/`sym@h`/`sym@l`, `R_PPC_EMB_SDA21` `sym@sda21(r13)`. m2c asserts on any other spelling
  (`@hi` crashes it), and an unapplied relocation left as a raw immediate would decompile silently wrong.
  An sda21 access is encoded with RA=0 and that *is* the sda base register, so it is printed as `(r13) -
  the same object form mwcc itself emits (checked against `mwcceppc` on a small-data access).
* A branch to an address that is a symbol in this object keeps that name; any other target inside the
  function becomes a `loc_<address>` label - the `loc_` prefix is what stops m2c treating it as the start
  of a new function.
* A tail call in the middle of a function (`b <function>`) becomes `bl <function>` + `blr`. m2c only
  accepts that shape when the branch is the function's *last* instruction (`TailCallPattern` in
  `m2c/arch_ppc.py`) and otherwise fails the function with "Cannot find branch target". It is m2c's own
  rewrite, applied early, so the C is the shape it would have produced for a trailing one.
* A symbol m2c cannot spell gets a readable alias: dtk names pooled constants `@1841_80629B90`, and `@` is
  m2c's relocation separator, so it becomes `_1841_80629B90` - definitions and references alike.
* A symbol whose body is only data (`.long`, `...`, the `gap_*` blobs) is dropped: m2c aborts the whole run
  with "Function ... contains no instructions" if one of those reaches it.
* The section is always emitted as `.text`. m2c only accepts a label as a function inside `.text`, so a
  `.init` function (memset, boot code) has to be handed over under that name; `--section` picks what to
  read out of the object.
* A `bctr` switch is handed its jump table: the table is the nearest symbol loaded before the `bctr`, its
  bytes are read out of the original DOL (`orig/RMHE08/sys/main.dol`, read-only - the table lives in a
  different section than the code, often a different split object) and emitted as `.data` with
  `.long loc_<address>` entries. A table m2c could not recognize by name (an anonymous local like `@1845`,
  or an SDK name) is written as `jumptable_<address>` so m2c looks at it at all. A table whose entries are
  not this function's case labels - a relative table, a wrong size, a function pointer read - is refused
  with a warning instead of guessed at, and then m2c reports it itself. The table's address comes from
  `config/RMHE08/symbols.txt` (through its own parser), which is also what tells this script where a unit
  object is linked (`--base` overrides both).

Not every instruction survives the trip: m2c has no `mfcr` or `cmpwi cr1, ...` and prints `M2C_ERROR(...)`
inline where it meets one, which is visible in its output. The file written is throwaway - `build/tmp/` is
gitignored. Nothing here is codegen evidence: the disassembly is the arbiter (playbook 4).

    --list       the object's functions with size and instruction count, data-only ones marked `data` and
                 the ones that switch through a jump table marked `switch`
    --no-tables  do not pull jump tables in (no DOL, no symbol map)
    --dol PATH   original DOL to read them out of (default `orig/RMHE08/sys/main.dol`)
    --base       address used for `loc_` labels and `--addresses` comments (default: the address in an
                 `auto_<nn>_<address>_<section>.o` name, else 0, i.e. object-relative)
    --section    section to read (default `.text`; repeatable, or `all`)
    --addresses  comment each instruction with its address
    -f           keep only these functions (repeatable)
"""

from __future__ import annotations

import argparse
import os
import re
import shutil
import struct
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
EXE = ".exe" if os.name == "nt" else ""
DEFAULT_OBJDUMP = os.path.join(ROOT, "build", "binutils", f"powerpc-eabi-objdump{EXE}")
ALL_SECTIONS = (".text", ".init", ".rodata", ".data", ".bss", ".sdata", ".sdata2", ".ctors", ".dtors")

# objdump's relocation type -> the macro m2c names it. m2c accepts exactly h/ha/l/sda2/sda21.
RELOC_SUFFIX = {
    "R_PPC_ADDR16_HI": "h",
    "R_PPC_ADDR16_HA": "ha",
    "R_PPC_ADDR16_LO": "l",
    "R_PPC_EMB_SDA21": "sda21",
}
RELOC_BRANCH = "R_PPC_REL24"
# An EMB_SDA21 access is encoded with RA=0, and that is the sda base register, per the EABI.
SDA_BASE = "r13"

SECTION_RE = re.compile(r"^Disassembly of section (?P<name>.+):$")
LABEL_RE = re.compile(r"^(?P<addr>[0-9a-f]{8}) <(?P<name>.+)>:$")
INSTR_RE = re.compile(r"^\s+(?P<addr>[0-9a-f]+):\t(?P<text>.*?)\s*$")
RELOC_RE = re.compile(r"^\s+(?P<addr>[0-9a-f]+):\s+(?P<type>R_PPC_\S+)\s*(?P<symbol>.*?)\s*$")
# The branch target objdump prints last: "2c8 <LocateObject+0x2c8>", "cr1,2c8 <...>", or a bare address.
TARGET_RE = re.compile(r"(?<![\w])(?P<addr>[0-9a-f]+)(?:\s+<(?P<sym>[^>]*)>)?$")
BRANCH_RE = re.compile(r"^b[a-z]*[.+-]?$")
# `@` starts a relocation macro in m2c, so a symbol containing one cannot be referenced as it stands.
UNSPELLABLE_RE = re.compile(r"[@]")
# A number, but not the digits inside a register name (r13) or a symbol (Debug_BBA_807953C8).
NUMBER_RE = re.compile(r"(?<![A-Za-z0-9_])(?:-?[0-9]+|0x[0-9a-fA-F]+)")
ADDRESS_MODE_RE = re.compile(r"\((?P<base>[^()]*)\)\s*$")
DATA_RE = re.compile(r"^(?:\.\w+|\.\.\.)$")

# The jump tables live in another section than the code that dispatches through them, so their bytes come
# from the original DOL (read-only, and always present). m2c only recognizes a table by the *name* it is
# loaded through, hence TABLE_PREFIXES below, which mirrors m2c/flow_graph.py.
DEFAULT_DOL = os.path.join(ROOT, "orig", "RMHE08", "sys", "main.dol")
DEFAULT_SYMBOLS = os.path.join(ROOT, "config", "RMHE08", "symbols.txt")
TABLE_PREFIXES = ("jtbl", "jpt_", "lbl_", "jumptable_")
MAX_TABLE_ENTRIES = 256
# How many symbol references before a `bctr` to try as its table: the nearest one is the table in the
# MWCC idiom, and a `lbl_*` reachable from an earlier case body can sit closer to a later one.
TABLE_CANDIDATES = 3


class Instr:
    """One disassembled line, with the relocations objdump printed under it."""

    def __init__(self, addr: int, text: str) -> None:
        self.addr = addr
        self.text = text
        self.relocs: list[tuple[str, str]] = []

    @property
    def mnemonic(self) -> str:
        return self.text.split(None, 1)[0] if self.text else ""

    def is_data(self) -> bool:
        """`.long 0x0` / `...` - bytes objdump printed as data rather than as an instruction."""
        return bool(DATA_RE.match(self.mnemonic))


class Function:
    """One symbol in the object: its instructions, and whether it holds code at all."""

    def __init__(self, name: str, addr: int, section: str) -> None:
        self.name = name
        self.addr = addr
        self.section = section
        self.instrs: list[Instr] = []
        self.end = addr

    @property
    def code(self) -> bool:
        """m2c refuses a function with no instructions, so data-only blobs are never emitted."""
        return any(not instr.is_data() for instr in self.instrs)

    @property
    def size(self) -> int:
        return self.end - self.addr

    def __repr__(self) -> str:  # pragma: no cover - debugging aid
        return f"<Function {self.name} {self.addr:#x} {len(self.instrs)} instrs>"


def find_objdump(explicit: str | None) -> str:
    for candidate in (explicit, DEFAULT_OBJDUMP, shutil.which("powerpc-eabi-objdump")):
        if candidate and os.path.exists(candidate):
            return candidate
    sys.exit(
        f"powerpc-eabi-objdump not found (looked for {DEFAULT_OBJDUMP}).\n"
        "Run `ninja tools` to download the pinned binutils, or pass --objdump PATH."
    )


def disassemble(objdump: str, obj: str) -> str:
    out = subprocess.run(
        [objdump, "-dr", "--no-show-raw-insn", obj], capture_output=True, text=True, errors="replace"
    )
    if out.returncode != 0:
        sys.exit(f"{objdump} failed on {obj}:\n{out.stderr.strip()[:500]}")
    return out.stdout


def spell(name: str) -> str:
    """A name m2c can parse. `@` starts a relocation macro there, and dtk names pooled constants `@…`.

    Applied when a name is *written*, never when it is looked up: the symbol map spells those symbols
    `@1845`, so the raw name is what finds them.
    """
    return UNSPELLABLE_RE.sub("_", name)


def parse(text: str, sections: tuple[str, ...]) -> tuple[dict[str, dict[int, str]], list[Function]]:
    """(symbol starts per section, the functions found in the wanted sections).

    Every section's symbols are collected - a branch into another section still has a name to use - but
    instructions are only kept for the wanted sections.
    """
    symbols: dict[str, dict[int, str]] = {}
    functions: list[Function] = []
    section = ""
    current: Function | None = None

    for line in text.splitlines():
        match = SECTION_RE.match(line)
        if match:
            section, current = match.group("name"), None
            continue

        match = LABEL_RE.match(line)
        if match:
            addr, name = int(match.group("addr"), 16), match.group("name")
            symbols.setdefault(section, {})[addr] = name
            current = None
            if section in sections and not name.startswith("."):
                current = Function(name, addr, section)
                functions.append(current)
            continue

        match = RELOC_RE.match(line)
        if match and current is not None and current.instrs:
            # objdump prints the relocation under its instruction, at the address of the relocated
            # field - for @ha/@l that is instruction+2, so find the instruction it belongs to.
            addr = int(match.group("addr"), 16)
            for instr in reversed(current.instrs):
                if instr.addr <= addr < instr.addr + 4:
                    instr.relocs.append((match.group("type"), match.group("symbol")))
                    break
            continue

        match = INSTR_RE.match(line)
        if match and current is not None:
            addr = int(match.group("addr"), 16)
            current.instrs.append(Instr(addr, match.group("text")))
            current.end = max(current.end, addr + 4)

    return symbols, functions


class Dol:
    """The original DOL as an addressable image: `read(vaddr, size)`, or None when it is not mapped.

    Its header is 7 text and 11 data section descriptors, each an offset/address/size triple, which is
    all that is needed to turn a data address into a file offset. Nothing here writes to it.
    """

    def __init__(self, path: str) -> None:
        with open(path, "rb") as fh:
            self.data = fh.read()
        self.sections: list[tuple[int, int, int]] = []
        for count, bases in ((7, (0x00, 0x48, 0x90)), (11, (0x1C, 0x64, 0xAC))):
            offsets, addresses, sizes = (
                [self.word(base + 4 * i) for i in range(count)] for base in bases
            )
            self.sections += list(zip(offsets, addresses, sizes))

    def word(self, at: int) -> int:
        return struct.unpack_from(">I", self.data, at)[0]

    def read(self, address: int, size: int) -> bytes | None:
        for offset, start, length in self.sections:
            if start <= address and address + size <= start + length:
                return self.data[offset + address - start : offset + address - start + size]
        return None


class JumpTable:
    """A jump table: the symbol it is loaded through, where it is, and its entries as code addresses.

    `emit_name` is the name it is written under, which is the symbol's own name when m2c can recognize
    that (`jumptable_*`, `jtbl*`, `lbl_*`, `jpt_*`) and a synthesized one otherwise - an anonymous local
    table (`@1845`) or a table with a name m2c has no reason to look at.
    """

    def __init__(self, name: str, address: int, entries: list[int], emit_name: str) -> None:
        self.name = name
        self.address = address
        self.entries = entries
        self.emit_name = emit_name

    def __repr__(self) -> str:  # pragma: no cover - debugging aid
        return f"<JumpTable {self.name} {self.address:#x} {len(self.entries)} entries>"


def load_dol(path: str) -> Dol | None:
    """The original DOL, or None when it is not there - the caller reports what that costs."""
    return Dol(path) if os.path.exists(path) else None


def load_symbols(path: str) -> dict[str, dict]:
    """name -> symbol-map entry, through that map's own parser (`tools/symbols/symedit.py`).

    Only used to place a jump table and to find the address an object is linked at, so a missing map costs
    the tables, not the decompilation.
    """
    if not os.path.exists(path):
        return {}
    symbols_dir = os.path.join(ROOT, "tools", "symbols")
    if symbols_dir not in sys.path:
        sys.path.insert(0, symbols_dir)
    import symedit  # noqa: PLC0415  (only needed when a jump table is in play)

    return {entry["name"]: entry for entry in symedit.entries(path)}


def symbol_address(name: str, symbols: dict[str, dict]) -> int | None:
    entry = symbols.get(name)
    if entry:
        return entry["address"]
    # dtk names most data after the address it sits at, so a name that carries one is enough on its own.
    match = re.search(r"([0-9A-Fa-f]{7,8})$", name)
    return int(match.group(1), 16) if match else None


def build_table(
    name: str,
    function: Function,
    base: int,
    end: int,
    symbols: dict[str, dict],
    image: Dol | None,
    verbose: bool = False,
) -> JumpTable | None:
    """Read the table `name` out of the original DOL, if its bytes really are this function's case labels.

    The check is the point: a table is only usable when every entry is an instruction address inside this
    function, which is exactly what m2c needs to turn it back into `switch` cases. Anything else - a
    relative table, a default case pointing elsewhere, a function pointer read, a wrong size - would
    either fail the function or, worse, produce confident nonsense, so it is refused instead.
    """
    def explain(reason: str) -> None:
        if verbose:
            print(f"warning: {function.name}: {name} is not usable as its jump table: {reason}", file=sys.stderr)

    if image is None:
        explain("the original DOL is not available")
        return None
    if not base:
        # Every absolute address in the table would be meaningless without this.
        explain("the object's link address is unknown (pass --base)")
        return None
    address = symbol_address(name, symbols)
    if address is None:
        explain("no address for it in the symbol map")
        return None
    start, stop = base + function.addr, base + end
    size = symbols.get(name, {}).get("size", 0)
    raw = image.read(address, size if size >= 4 else MAX_TABLE_ENTRIES * 4)
    if raw is None:
        explain(f"its bytes at {address:#x} are not in the DOL")
        return None
    entries: list[int] = []
    for value in struct.unpack(f">{len(raw) // 4}I", raw):
        if start <= value < stop and value % 4 == 0:
            entries.append(value)
        elif size >= 4:
            explain(f"entry {len(entries)} is {value:#x}, outside {start:#x}-{stop:#x}")
            return None
        else:
            break
    if len(entries) < 2:
        explain(f"only {len(entries)} of its entries look like case labels")
        return None
    spelled = spell(name)
    return JumpTable(name, address, entries, spelled if spelled.startswith(TABLE_PREFIXES) else f"jumptable_{address:x}")


def find_tables(
    function: Function, base: int, end: int, symbols: dict[str, dict], image: Dol | None
) -> list[JumpTable]:
    """One table per `bctr` in the function, from the nearest symbol load before it that reads as one."""
    tables: list[JumpTable] = []
    candidates: list[str] = []
    for instr in function.instrs:
        candidates.extend(symbol for _reloc_type, symbol in instr.relocs if symbol)
        if instr.mnemonic != "bctr":
            continue
        for depth, name in enumerate(reversed(candidates[-TABLE_CANDIDATES:])):
            table = build_table(name, function, base, end, symbols, image, verbose=depth == 0)
            if table:
                tables.append(table)
                break
        candidates = []
    return tables


def replace_operand(text: str, expr: str, *, sda: bool = False) -> str | None:
    """Rewrite the last immediate of `text` to `expr`, keeping an address-mode base register.

    `lis r3,-21845` -> `lis r3,sym@ha`; `lwz r3,4(r3)` -> `lwz r3,sym@l(r3)`; `lbz r3,0(0)` (an sda21
    access, whose base register is encoded as 0) -> `lbz r3,sym@sda21(r13)`. The base register is peeled
    off first: objdump prints r0 as a bare `0` there, so it is a number token of its own.
    """
    suffix = ""
    base = ADDRESS_MODE_RE.search(text)
    if base:
        register = base.group("base")
        if sda and register in ("0", "r0"):
            register = SDA_BASE
        elif register == "0":
            register = "r0"
        suffix = f"({register})"
        text = text[: base.start()].rstrip()
    matches = list(NUMBER_RE.finditer(text))
    if not matches:
        return None
    last = matches[-1]
    return f"{text[:last.start()]}{expr}{suffix}{text[last.end():]}"


def rewrite(
    instr: Instr,
    func: Function,
    known: dict[int, str],
    end: int,
    base: int,
    last: bool,
    renames: dict[str, str],
) -> tuple[list[str], set[int]]:
    """The m2c-ready text of one instruction, plus the in-function addresses it jumps to.

    Two lines come back when a mid-function tail call had to be spelled out as `bl` + `blr`. `renames`
    carries the jump tables m2c has to recognize by name.
    """
    text = instr.text
    labels: set[int] = set()
    reloc_type, reloc_symbol = instr.relocs[0] if instr.relocs else (None, None)
    if reloc_symbol:
        reloc_symbol = spell(renames.get(reloc_symbol, reloc_symbol))

    if reloc_type in (RELOC_BRANCH, None) and BRANCH_RE.match(instr.mnemonic):
        match = TARGET_RE.search(text)
        if not match:  # blr / bctr: no target
            return [text], labels
        addr = int(match.group("addr"), 16)
        external = False
        if reloc_type == RELOC_BRANCH:
            if not reloc_symbol:
                raise SystemExit(f"{RELOC_BRANCH} at {instr.addr:#x} in {func.name} names no symbol: {text!r}")
            operand, external = reloc_symbol, True
        elif addr in known:
            operand = spell(known[addr])
            # A name defined inside this function is one of its own labels; anything else is another
            # function (m2c's own test: is the name a label of this function).
            external = not func.addr <= addr < end
        else:
            operand = f"loc_{base + addr:x}"
            labels.add(addr)
            if not func.addr <= addr < end:
                print(
                    f"warning: {func.name}: branch at {instr.addr:#x} targets {addr:#x}, which is not a "
                    f"symbol here - emitted as {operand}. A data blob read as code does this.",
                    file=sys.stderr,
                )
        if external and not last and instr.mnemonic.rstrip("+-.") == "b":
            # A tail call that is not the function's last instruction. m2c only accepts that shape when
            # the branch is last (TailCallPattern in m2c/arch_ppc.py), and otherwise fails the whole
            # function with "Cannot find branch target <name>". This is that rewrite, applied early.
            return [f"bl {operand}", "blr"], labels
        return [text[: match.start()] + operand], labels

    if reloc_type in RELOC_SUFFIX:
        expr = f"{reloc_symbol}@{RELOC_SUFFIX[reloc_type]}"
        rewritten = replace_operand(text, expr, sda=reloc_type == "R_PPC_EMB_SDA21")
        if rewritten is None:
            raise SystemExit(
                f"cannot attach {reloc_type} {reloc_symbol} to {text!r} at {instr.addr:#x} in "
                f"{func.name}: no immediate operand to replace"
            )
        return [rewritten], labels

    if reloc_type is not None:
        raise SystemExit(
            f"unhandled relocation {reloc_type} ({reloc_symbol}) at {instr.addr:#x} in {func.name}: "
            f"{text!r}\nRefusing to guess - an unapplied relocation would decompile to the wrong C. "
            "Report it to whoever maintains tools/units/m2cinput.py."
        )

    return [text], labels


def convert(
    text: str,
    sections: tuple[str, ...],
    base: int = 0,
    addresses: bool = False,
    wanted: set[str] | None = None,
    image: Dol | None = None,
    table_symbols: dict[str, dict] | None = None,
) -> tuple[str, list[Function]]:
    """objdump output -> m2c input, plus the functions the output was made from.

    `image` and `table_symbols` are what a `bctr` switch needs: the original DOL to read the table out of,
    and the symbol map to find where it lives. Without them a `bctr` function is emitted as it stands and
    m2c says so itself.
    """
    symbols, functions = parse(text, sections)
    groups: dict[str, list[Function]] = {}
    for function in functions:
        groups.setdefault(function.section, []).append(function)

    out: list[str] = [".text"]
    emitted: list[Function] = []
    recovered: list[JumpTable] = []
    unresolved: list[str] = []
    lookup = table_symbols or {}
    for section, group in groups.items():
        known = symbols.get(section, {})
        for index, function in enumerate(group):
            if not function.code or (wanted is not None and function.name not in wanted):
                continue
            end = group[index + 1].addr if index + 1 < len(group) else function.end
            code = [instr for instr in function.instrs if not instr.is_data()]
            if any(instr.mnemonic == "bctr" for instr in code):
                tables = find_tables(function, base, end, lookup, image)
                if not tables:
                    unresolved.append(function.name)
                recovered.extend(tables)
            else:
                tables = []
            renames = {table.name: table.emit_name for table in tables if table.name != table.emit_name}
            # A case reachable only through the table has no branch to it, so its label comes from here.
            labels: dict[int, str] = {
                target - base: f"loc_{target:x}"
                for table in tables
                for target in table.entries
                if any(instr.addr == target - base for instr in code)
            }
            body: list[str] = []
            for position, instr in enumerate(code):
                lines, targets = rewrite(
                    instr, function, known, end, base, position == len(code) - 1, renames
                )
                for addr in targets:
                    labels.setdefault(addr, f"loc_{base + addr:x}")
                comment = f"/* {base + instr.addr:08X} */ " if addresses else ""
                body.extend((instr.addr, comment + line) for line in lines)
            emitted.append(function)
            out.append(f"glabel {spell(function.name)}")
            for addr, line in body:
                if addr in labels:
                    out.append(f"{labels[addr]}:")
                out.append(line)
            out.append("")
    if recovered:
        out.append(".data")
        for table in recovered:
            out.append(f"{table.emit_name}:")
            out.extend(f".long loc_{target:x}" for target in table.entries)
            out.append("")
    if unresolved:
        print(
            f"warning: {len(unresolved)} function(s) jump through a table (`bctr`) that could not be "
            f"placed: {', '.join(unresolved)}.\nm2c aborts the whole file on those - keep the others "
            "with `-f <name>`, or pass --base/--dol.",
            file=sys.stderr,
        )
    return "\n".join(out).rstrip("\n") + "\n", emitted


def base_from_name(path: str) -> int:
    """`auto_00_80004000_init.o` names the address its section starts at; everything else is 0."""
    match = re.match(r"^auto_\d+_([0-9a-fA-F]{8})_", os.path.basename(path))
    return int(match.group(1), 16) if match else 0


def derive_base(functions: list[Function], symbols: dict[str, dict], fallback: int) -> int:
    """Where this object's section is linked: the first function it defines, asked of the symbol map.

    The addresses objdump prints are section-relative, and the jump tables hold absolute ones, so the two
    have to be related before a table can be used - and a `loc_` label named after the retail address is
    easier to check against `symbols.txt` than one named after an offset.
    """
    for function in functions:
        entry = symbols.get(function.name)
        if entry and entry["address"] >= function.addr:
            return entry["address"] - function.addr
    return fallback


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("object", help="object to read (build/RMHE08/obj/<unit>.o)")
    parser.add_argument("-f", "--function", action="append", default=[], help="keep this function (repeatable)")
    parser.add_argument("-o", "--output", help="write here (default stdout)")
    parser.add_argument("--list", action="store_true", help="list the object's functions and stop")
    parser.add_argument("--section", action="append", default=[], help="section to read (default .text)")
    parser.add_argument("--base", type=lambda v: int(v, 0), help="address for loc_ labels (default from name)")
    parser.add_argument("--addresses", action="store_true", help="comment each instruction with its address")
    parser.add_argument("--objdump", help="path to powerpc-eabi-objdump")
    parser.add_argument("--dol", default=DEFAULT_DOL, help="original DOL to read jump tables out of")
    parser.add_argument("--symbols", default=DEFAULT_SYMBOLS, help="symbol map, for jump-table addresses")
    parser.add_argument("--no-tables", action="store_true", help="do not pull jump tables in")
    args = parser.parse_args()

    sections = tuple(args.section) if args.section else (".text",)
    if "all" in sections:
        sections = ALL_SECTIONS

    text = disassemble(find_objdump(args.objdump), args.object)
    _, functions = parse(text, sections)

    if args.list:
        for function in functions:
            count = sum(1 for instr in function.instrs if not instr.is_data())
            kind = "code" if count else "data"
            switch = " switch" if any(instr.mnemonic == "bctr" for instr in function.instrs) else ""
            print(f"{function.addr:08X} {function.size:6d} {count:5d}  {kind:<4}{switch:<8}{function.name}")
        return 0

    wanted = set(args.function) or None
    if wanted:
        known = {function.name for function in functions}
        missing = sorted(wanted - known)
        if missing:
            sys.exit(f"not found in {args.object}: {', '.join(missing)}")
        dataless = sorted(name for name in wanted if name in known and not _code(functions, name))
        if dataless:
            sys.exit(f"no instructions (m2c would abort on it): {', '.join(dataless)}")

    symbols = {} if args.no_tables else load_symbols(args.symbols)
    image = None if args.no_tables else load_dol(args.dol)
    base = args.base if args.base is not None else derive_base(functions, symbols, base_from_name(args.object))
    asm, emitted = convert(text, sections, base, args.addresses, wanted, image, symbols)
    if wanted and not emitted:
        sys.exit(f"nothing emitted for: {', '.join(sorted(wanted))}")

    if args.output:
        parent = os.path.dirname(os.path.abspath(args.output))
        os.makedirs(parent, exist_ok=True)
        with open(args.output, "w", encoding="utf-8", newline="\n") as fh:
            fh.write(asm)
        print(f"{args.output}: {len(emitted)} function(s), {len(asm.splitlines())} lines")
    else:
        sys.stdout.write(asm)
    return 0


def _code(functions: list[Function], name: str) -> bool:
    return any(function.name == name and function.code for function in functions)


if __name__ == "__main__":
    sys.exit(main())
