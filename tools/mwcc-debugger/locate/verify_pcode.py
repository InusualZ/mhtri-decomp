#!/usr/bin/env python3
"""Cross-check a PCode dump against the object the same compiler emitted.

The point of the port is that the dumps describe *our* compiler's real
behaviour, so this compares the last PCode dump of a run (the state just before
assembly) with `powerpc-eabi-objdump -d` of the object the same command line
produces.

    python locate/verify_pcode.py <backend-NN-....txt> <file.o>

Mnemonics are compared after folding the spelling differences between MWCC's
PCode and the disassembler (`clrlwi` is an `rlwinm`, `cmplwi` is a `cmpli`,
`bgt` is a `bt` on a condition bit, ...), and registers are compared as sets
per instruction, because the dump prints a memory operand as `rX,rY,disp`
where the disassembler prints `disp(rY)`.

Exit status is 0 when nothing mismatches, 1 otherwise.
"""
from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path

OBJDUMP = (
    Path(__file__).resolve().parents[3] / "build/binutils/powerpc-eabi-objdump.exe"
)

# MWCC PCode mnemonic -> the disassembler's spelling for the same encoding.
ALIASES = {
    "clrlwi": "rlwinm",
    "clrrwi": "rlwinm",
    "extlwi": "rlwinm",
    "extrwi": "rlwinm",
    "inslwi": "rlwimi",
    "insrwi": "rlwimi",
    "rotlwi": "rlwinm",
    "rotrwi": "rlwinm",
    "slwi": "rlwinm",
    "srwi": "rlwinm",
    "cmplwi": "cmpli",
    "cmpwi": "cmpi",
    "subi": "addi",
    "subic": "addic",
    "subis": "addis",
}

# bt/bf on cr0 map onto the disassembler's named conditions.
CR0_BRANCHES = {
    ("bt", 0): "blt",
    ("bt", 1): "bgt",
    ("bt", 2): "beq",
    ("bt", 3): "bso",
    ("bf", 0): "bge",
    ("bf", 1): "ble",
    ("bf", 2): "bne",
    ("bf", 3): "bns",
}

# Mnemonics whose immediate *fields* are laid out differently by each side
# (the disassembler prints `clrlwi r0,r4,16` for what the PCode record holds as
# `rlwinm r0,r4,0,16,31`), or whose operand is a raw address in the object and
# a label id in the dump.  Immediates are not compared for these; mnemonics and
# registers always are.
NO_IMM_COMPARE = {
    "rlwinm",
    "rlwimi",
    "cmpi",
    "cmpli",
    "b",
    "bl",
    "bc",
    "bdnz",
    "blt",
    "bgt",
    "beq",
    "bso",
    "bge",
    "ble",
    "bne",
    "bns",
}

DUMP_LINE = re.compile(r"^\s*\d+\s+([a-z0-9_.]+)\s*(.*)$")
OBJDUMP_LINE = re.compile(r"^\s*[0-9a-f]+:\s+([a-z0-9_.]+)\s*(.*)$")
REG = re.compile(r"\b([rf])(\d+)\b")
IMM = re.compile(r"(?<![\w])(-?0x[0-9a-f]+|-?\d+)\b")


def read_dump(path):
    insns = []
    for line in Path(path).read_text(encoding="utf-8").splitlines():
        m = DUMP_LINE.match(line)
        if m:
            insns.append((m.group(1), m.group(2)))
    return insns


def read_objdump(obj_path):
    out = subprocess.run(
        [str(OBJDUMP), "-d", "--no-show-raw-insn", str(obj_path)],
        capture_output=True,
        text=True,
        check=True,
    ).stdout
    insns = []
    for line in out.splitlines():
        m = OBJDUMP_LINE.match(line)
        if m:
            insns.append((m.group(1), m.group(2)))
    return insns


def normalise(insns):
    out = []
    for mnemonic, operands in insns:
        text = operands.replace(" ", "")
        text = re.sub(r"<[^>]*>", "", text)  # symbolic branch targets
        if mnemonic in ("bt", "bf"):
            cr, bit = 0, None
            for part in text.split(","):
                cm = re.match(r"cr(\d+)", part)
                if cm:
                    cr = int(cm.group(1))
                if re.fullmatch(r"-?\d+", part):
                    bit = int(part)
            named = CR0_BRANCHES.get((mnemonic, (bit or 0) + 4 * cr))
            if named:
                mnemonic = named
        mnemonic = ALIASES.get(mnemonic, mnemonic)
        regs = {f"{a}{b}" for a, b in REG.findall(text)}
        imms = []
        for value in IMM.findall(text):
            try:
                imms.append(int(value, 0))
            except ValueError:
                pass
        out.append((mnemonic, regs, imms))
    return out


def main():
    dump_insns = normalise(read_dump(sys.argv[1]))
    obj_insns = normalise(read_objdump(sys.argv[2]))

    problems = []
    if len(dump_insns) != len(obj_insns):
        problems.append(
            f"instruction count: dump {len(dump_insns)} vs object {len(obj_insns)}"
        )
    for i, (a, b) in enumerate(zip(dump_insns, obj_insns)):
        if a[0] != b[0]:
            problems.append(f"#{i}: mnemonic {a[0]} (dump) vs {b[0]} (object)")
        elif a[1] != b[1]:
            problems.append(
                f"#{i} {a[0]}: registers {sorted(a[1])} vs {sorted(b[1])}"
            )
        elif (
            a[2]
            and b[2]
            and a[2] != b[2]
            and a[0] not in NO_IMM_COMPARE
            and not all(x in b[2] or x == 0 for x in a[2])
        ):
            problems.append(f"#{i} {a[0]}: immediates {a[2]} vs {b[2]}")

    print(f"dump:   {len(dump_insns)} instructions")
    print(f"object: {len(obj_insns)} instructions")
    if problems:
        print(f"\n{len(problems)} difference(s):")
        for p in problems[:40]:
            print("  " + p)
        return 1
    print("MATCH: every mnemonic, register and compared immediate agrees")
    return 0


if __name__ == "__main__":
    sys.exit(main())
