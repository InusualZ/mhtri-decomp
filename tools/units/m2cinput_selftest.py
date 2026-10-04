#!/usr/bin/env python3
"""Fast, deterministic self-test for tools/units/m2cinput.py.

    python tools/units/m2cinput_selftest.py

No object, no objdump and no build: it feeds the converter real objdump text (copied from
`build/RMHE08/obj/**` output, tabs included) through the pure functions and checks the m2c input it
produces. That covers the rewrites that are easy to get subtly wrong - an `@`-named pooled constant, an
sda21 access whose base register objdump prints as `0`, a `bdnz-` hint, a mid-function tail call, a
data-only `gap_*` blob - without depending on a split object being present.

The rows are the contract: when a rewrite changes on purpose, the row changes with it.
"""
from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import contextlib
import io
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))


from tools.units import m2cinput as m2c

# One `.text` section, verbatim in the shape `objdump -dr --no-show-raw-insn` prints it.
TEXT_OBJECT = "\n".join(
    [
        "build/RMHE08/obj/auto_00_80004000_init.o:     file format elf32-powerpc",
        "",
        "",
        "Disassembly of section .text:",
        "",
        "00000000 <fn_80004000>:",
        "   0:\tlbz     r3,0(0)",
        "\t\t\t0: R_PPC_EMB_SDA21\tglob",
        "   4:\tlis     r4,0",
        "\t\t\t6: R_PPC_ADDR16_HA\tjumptable_805CEF78",
        "   8:\tbdnz-   c <fn_80004000+0xc>",
        "   c:\tbl      0 <fn_80004000>",
        "\t\t\tc: R_PPC_REL24\t_savegpr_15",
        "  10:\tb       18 <other_fn>",
        "  14:\tblr",
        "",
        "00000018 <other_fn>:",
        "  18:\tli      r3,0",
        "  1c:\tblr",
        "",
        "00000020 <gap_00_80004020_text>:",
        "  20:\t.long 0x0",
        "",
    ]
)

TEXT_EXPECTED = "\n".join(
    [
        ".text",
        "glabel fn_80004000",
        "lbz     r3,glob@sda21(r13)",
        "lis     r4,jumptable_805CEF78@ha",
        "bdnz-   loc_c",
        "loc_c:",
        "bl      _savegpr_15",
        "bl other_fn",
        "blr",
        "blr",
        "",
        "glabel other_fn",
        "li      r3,0",
        "blr",
        "",
    ]
)

# dtk names pooled constants `@…`, and `@` is m2c's relocation separator, so it has to be respelled.
POOL_OBJECT = "\n".join(
    [
        "Disassembly of section .text:",
        "",
        "0000001c <fn_8010D6A4>:",
        "  1c:\tlis     r19,0",
        "\t\t\t1e: R_PPC_ADDR16_HA\t@1841_80629B90",
        "  20:\tblr",
        "",
    ]
)

# A relocation the converter has no spelling for must stop the run, not produce a wrong operand.
UNKNOWN_OBJECT = "\n".join(
    [
        "Disassembly of section .text:",
        "",
        "00000000 <fn_8000>:",
        "   0:\tlwz     r3,0(r3)",
        "\t\t\t0: R_PPC_EMB_SDA2REL\tglob",
        "   4:\tblr",
        "",
    ]
)

# `.data` is not read unless it is asked for, and a data-only symbol is never emitted.
DATA_OBJECT = "\n".join(
    [
        "Disassembly of section .text:",
        "",
        "00000000 <fn_8000>:",
        "   0:\tblr",
        "",
        "Disassembly of section .data:",
        "",
        "00000000 <lbl_8000>:",
        "   0:\t.long 0x0",
        "",
    ]
)

# A `bctr` switch: the table is loaded through `jumptable_80007000`, then indexed and jumped through.
SWITCH_OBJECT = "\n".join(
    [
        "Disassembly of section .text:",
        "",
        "00000000 <fn_80006000>:",
        "   0:\tcmplwi  r3,2",
        "   4:\tbgt     20 <fn_80006000+0x20>",
        "   8:\tlis     r4,0",
        "\t\t\ta: R_PPC_ADDR16_HA\tjumptable_80007000",
        "   c:\taddi    r4,r4,0",
        "\t\t\te: R_PPC_ADDR16_LO\tjumptable_80007000",
        "  10:\tslwi    r0,r3,2",
        "  14:\tlwzx    r4,r4,r0",
        "  18:\tmtctr   r4",
        "  1c:\tbctr",
        "  20:\tli      r3,0",
        "  24:\tblr",
        "",
    ]
)
SWITCH_ENTRIES = [0x80006008, 0x80006020, 0x80006020]
SWITCH_EXPECTED = "\n".join(
    [
        ".text",
        "glabel fn_80006000",
        "cmplwi  r3,2",
        "bgt     loc_80006020",
        "loc_80006008:",
        "lis     r4,jumptable_80007000@ha",
        "addi    r4,r4,jumptable_80007000@l",
        "slwi    r0,r3,2",
        "lwzx    r4,r4,r0",
        "mtctr   r4",
        "bctr",
        "loc_80006020:",
        "li      r3,0",
        "blr",
        "",
        ".data",
        "jumptable_80007000:",
        ".long loc_80006008",
        ".long loc_80006020",
        ".long loc_80006020",
        "",
    ]
)


# `-M gekko` makes objdump print the Gekko paired-single forms correctly: the frame saves are `psq_st`
# (a *displacement* form, already handled) and the restores are an indexed form when MWCC's peephole is
# off (the retail shape of docs/matching.md row 39). MWCC addresses the slot with a register it just
# computed - `addi rX,r1,N` + `psq_stx fX,r0,rX` in the prologue, `li r0,N` + `psq_lx fX,r1,r0` in the
# epilogue. m2c has no load/store entry for either, so the converter folds both pairs to the displacement
# form it handles; without the fold the frame save/restore comes out as `M2C_ERROR(unknown instruction:
# psq_stx/psq_lx ...)` woven into every path.
PSQ_OBJECT = "\n".join(
    [
        "Disassembly of section .text:",
        "",
        "00000000 <fn_psq>:",
        "   0:\tstwu    r1,-48(r1)",
        "   4:\tstfd    f31,8(r1)",
        "   8:\tpsq_st  f31,16(r1),0,0",
        "   c:\taddi    r3,r1,24",
        "  10:\tpsq_stx f31,r0,r3,0,0",
        "  14:\tli      r0,16",
        "  18:\tpsq_lx  f31,r1,r0,0,0",
        "  1c:\tpsq_lx  f30,r3,r4,0,0",
        "  20:\tlfd     f31,8(r1)",
        "  24:\taddi   r1,r1,48",
        "  28:\tblr",
        "",
    ]
)
PSQ_EXPECTED = "\n".join(
    [
        ".text",
        "glabel fn_psq",
        "stwu    r1,-48(r1)",
        "stfd    f31,8(r1)",
        "psq_st  f31,16(r1),0,0",
        "addi    r3,r1,24",
        "psq_st f31,24(r1),0,0",
        "li      r0,16",
        "psq_l f31,16(r1),0,0",
        "psq_lx  f30,r3,r4,0,0",
        "lfd     f31,8(r1)",
        "addi   r1,r1,48",
        "blr",
        "",
    ]
)

# A `psq_*x` indexed by a live register (no `li` setting the index) is *not* folded: m2c must report it
# rather than be handed a guessed displacement. The row also pins the `li`->register match.
PSQ_LIVE_INDEX_OBJECT = "\n".join(
    [
        "Disassembly of section .text:",
        "",
        "00000000 <fn_psq_live>:",
        "   0:\tli      r0,16",
        "   4:\tpsq_stx f31,r1,r4,0,0",
        "   8:\tblr",
        "",
    ]
)


class FakeImage:
    """A `Dol` stand-in: a table's worth of 32-bit entries at one address."""

    def __init__(self, address: int, entries: list[int]) -> None:
        import struct as _struct

        self.address = address
        self.raw = _struct.pack(f">{len(entries)}I", *entries)

    def read(self, address: int, size: int) -> bytes | None:
        if address == self.address:
            return self.raw[:size]
        return None


def make_dol(path: str, address: int, payload: bytes) -> None:
    """A minimal DOL: one text section, at `address`, holding `payload`."""
    import struct as _struct

    header = bytearray(0x100)
    _struct.pack_into(">I", header, 0x00, 0x100)  # first text section offset
    _struct.pack_into(">I", header, 0x48, address)  # ... its address
    _struct.pack_into(">I", header, 0x90, len(payload))  # ... and its size
    with open(path, "wb") as fh:
        fh.write(bytes(header) + payload)


def rows():
    asm, emitted = m2c.convert(TEXT_OBJECT, (".text",))
    yield "text section", asm, TEXT_EXPECTED
    yield "emitted functions", [f.name for f in emitted], ["fn_80004000", "other_fn"]

    asm, _ = m2c.convert(POOL_OBJECT, (".text",))
    yield "pooled constant", asm, "\n".join(
        [".text", "glabel fn_8010D6A4", "lis     r19,_1841_80629B90@ha", "blr", ""]
    )

    asm, _ = m2c.convert(DATA_OBJECT, (".text",))
    yield "data section ignored", asm, "\n".join([".text", "glabel fn_8000", "blr", ""])

    asm, _ = m2c.convert(DATA_OBJECT, (".text", ".data"))
    yield "data-only symbol dropped", asm, "\n".join([".text", "glabel fn_8000", "blr", ""])

    asm, emitted = m2c.convert(TEXT_OBJECT, (".text",), addresses=True)
    yield "address comments", "/* 0000000C */ bl      _savegpr_15" in asm, True

    # `--list` facts: the third symbol is a data blob, and the converter knows.
    symbols, functions = m2c.parse(TEXT_OBJECT, (".text",))
    yield "data-only detected", [f.code for f in functions], [True, True, False]
    yield "sda21 base register", "glob@sda21(r13)" in asm, True
    yield "symbol table", sorted(symbols[".text"].values()), ["fn_80004000", "gap_00_80004020_text", "other_fn"]

    # The tail-call rewrite is what keeps a dispatcher (Camellia_Ekeygen, `b fn` mid-function) readable.
    asm, _ = m2c.convert(TEXT_OBJECT, (".text",))
    yield "tail call spelled out", "bl other_fn\nblr\nblr" in asm, True
    yield "trailing tail call left alone", asm.count("bl other_fn"), 1

    # A jump table: the bytes come from the DOL, the name from the symbol map, and the case labels from
    # the table itself - which is the only way a `bctr` switch can be recovered at all.
    table_symbols = {"jumptable_80007000": {"address": 0x80007000, "size": 4 * len(SWITCH_ENTRIES)}}
    asm, _ = m2c.convert(SWITCH_OBJECT, (".text",), 0x80006000, image=FakeImage(0x80007000, SWITCH_ENTRIES), table_symbols=table_symbols)
    yield "jump table recovered", asm, SWITCH_EXPECTED

    # The same table under a name m2c would not look at (dtk calls an anonymous local `@1845`).
    anonymous = {"@1845": {"address": 0x80007000, "size": 4 * len(SWITCH_ENTRIES)}}
    asm, _ = m2c.convert(SWITCH_OBJECT, (".text",), 0x80006000, image=FakeImage(0x80007000, SWITCH_ENTRIES), table_symbols=anonymous)
    yield "anonymous table renamed", "jumptable_80007000:" in asm and "@1845" not in asm, True

    # A refusal and a missing DOL both print a warning, which is the point - but not the test's output.
    quiet = contextlib.redirect_stderr(io.StringIO())
    with quiet:
        stray = FakeImage(0x80007000, [0x80006008, 0x12345678, 0x80006020])
        asm, _ = m2c.convert(SWITCH_OBJECT, (".text",), 0x80006000, image=stray, table_symbols=table_symbols)
    yield "foreign table refused", ".data" in asm, False

    # No image at all (a missing DOL): the code is emitted, m2c reports the switch itself.
    with quiet:
        asm, _ = m2c.convert(SWITCH_OBJECT, (".text",), 0x80006000)
    yield "no DOL still converts", ".data" in asm, False

    # The object's link address, taken from its own symbols when the name does not carry one.
    _, functions = m2c.parse(SWITCH_OBJECT, (".text",))
    yield "base from symbols", m2c.derive_base(functions, {"fn_80006000": {"address": 0x80006000}}, 0), 0x80006000

    # The paired-single path: `-M gekko` decodes the Wii forms, and the indexed restore is folded to the
    # displacement form m2c's own load/store table has (m2c/arch_ppc.py: `psq_l`/`psq_st`).
    asm, _ = m2c.convert(PSQ_OBJECT, (".text",))
    yield "psq indexed restore folded", asm, PSQ_EXPECTED
    asm, _ = m2c.convert(PSQ_LIVE_INDEX_OBJECT, (".text",))
    yield "psq live index left alone", "psq_stx f31,r1,r4,0,0" in asm, True

    # objdump has to be told the Gekko core, or the pinned binutils renders `psq_lx` as `vmrghb` and the
    # `psq_l`/`psq_st` displacement forms as VSX (`xxsel`, `xscmpgedp`, ...). The CPU flag is the fix.
    yield "objdump gets the Gekko CPU", disasm_command()[1:3], ["-M", m2c.DISASM_CPU]

    for label, text in (("unknown relocation", UNKNOWN_OBJECT),):
        try:
            m2c.convert(text, (".text",))
        except SystemExit as exc:
            yield label, "R_PPC_EMB_SDA2REL" in str(exc), True
        else:
            yield label, "no error raised", "SystemExit"


def disasm_command() -> list[str]:
    """The argv `disassemble` hands objdump, captured without running the real one."""
    calls: list[list[str]] = []

    class Done:
        returncode = 0
        stdout = ""
        stderr = ""

    original = m2c.subprocess.run
    m2c.subprocess.run = lambda cmd, **kw: (calls.append(cmd), Done())[1]
    try:
        m2c.disassemble("objdump", "x.o")
    finally:
        m2c.subprocess.run = original
    return calls[0]


def main() -> int:
    failures = 0
    for label, got, want in rows():
        if got == want:
            print(f"ok    {label}")
        else:
            failures += 1
            print(f"FAIL  {label}\n        got:  {got!r}\n        want: {want!r}")
    failures += dol_rows()
    print(f"{'FAILED' if failures else 'passed'}: {failures} failure(s)")
    return 1 if failures else 0


def dol_rows() -> int:
    """The DOL header is the one piece of arithmetic a table reading cannot be tested without."""
    import tempfile

    failures = 0
    payload = b"\x80\x00\x60\x00" * 3
    with tempfile.TemporaryDirectory() as tmp:
        path = os.path.join(tmp, "fake.dol")
        make_dol(path, 0x80006000, payload)
        image = m2c.Dol(path)
        checks = (
            ("dol read", image.read(0x80006000, 12), payload),
            ("dol read unaligned", image.read(0x80006004, 4), payload[4:8]),
            ("dol read outside", image.read(0x80100000, 4), None),
        )
    for label, got, want in checks:
        if got == want:
            print(f"ok    {label}")
        else:
            failures += 1
            print(f"FAIL  {label}\n        got:  {got!r}\n        want: {want!r}")
    return failures


if __name__ == "__main__":
    sys.exit(main())
