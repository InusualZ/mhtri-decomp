"""MWCC `asm` source from a gekko `objdump -dr` listing: SPR names, branch labels, relocation operands.
Spec: docs/tools/spec/lib-asmgen.md. CLI: none (library)."""
from __future__ import annotations

import re
from dataclasses import dataclass

from tools.lib.binary import objdump

#: The disassembler's CPU: the default core decodes the paired-single opcodes as VSX/VMX.
CPU = "gekko"

#: Special-purpose registers MWCC's assembler takes by a name or a number, keyed by objdump's `mf<name>`/`mt<name>`
#: spelling. HID2 (920), HID4 (1011), WPAR (921) and the DMA pair (922/923) have no MWCC name: the number is written.
SPR = {name: name.upper() for name in (
    "xer", "dsisr", "dar", "dec", "sdr1", "srr0", "srr1", "sprg0", "sprg1", "sprg2", "sprg3", "ear", "pvr", "hid0",
    "hid1", "dabr", "iabr", "l2cr", "mmcr0", "mmcr1", "pmc1", "pmc2", "pmc3", "pmc4", "sia", "ummcr0", "ummcr1",
    "upmc1", "upmc2", "upmc3", "upmc4", "usia", "thrm1", "thrm2", "thrm3", "tbl", "tbu", "ictc")}
SPR.update({"hid2": "920", "hid4": "1011", "wpar": "921", "dmau": "922", "dmal": "923"})
for _i in range(8):
    SPR["gqr%d" % _i] = "GQR%d" % _i
    SPR["ibat%du" % _i] = "IBAT%dU" % _i
    SPR["ibat%dl" % _i] = "IBAT%dL" % _i
    SPR["dbat%du" % _i] = "DBAT%dU" % _i
    SPR["dbat%dl" % _i] = "DBAT%dL" % _i
#: The forms with their own mnemonic that stay as they are.
KEEP = ("mfcr", "mtcr", "mflr", "mtlr", "mfctr", "mtctr", "mfxer", "mtxer", "mfmsr", "mtmsr", "mftb", "mftbu")

_SPR_MN = re.compile(r"^(mf|mt)([a-z0-9]+)$")
_TARGET = re.compile(r"([0-9a-f]+) <")
_NUMBER_TAIL = re.compile(r"-?\d+$")
_DISPLACEMENT = re.compile(r"-?\d+\(")
_DISP_BASE = re.compile(r"-?\d+\((r?\d+)\)")


@dataclass(frozen=True)
class Insn:
    address: int
    mnemonic: str
    operands: str
    raw: str = ""


@dataclass
class Function:
    name: str
    insns: list[Insn]
    relocs: dict[int, tuple[str, str]]       # address -> (type, symbol)


def parse(text: str) -> dict[str, Function]:
    """`{function label: Function}` from an `objdump -dr` listing (only `.text`-style code sections)."""
    out: dict[str, Function] = {}
    cur: Function | None = None
    section = ""
    for raw in text.splitlines():
        line = objdump.tokenize(raw)
        if line is None:
            continue
        if line.kind == "section":
            section = line.name
        elif line.kind == "label" and section.startswith((".text", ".init")):
            cur = out.setdefault(line.name, Function(line.name, [], {}))
        elif line.kind == "label":
            cur = None
        elif cur is not None and line.kind == "insn":
            cur.insns.append(Insn(line.address, line.mnemonic, line.operands, line.raw))
        elif cur is not None and line.kind == "reloc":
            cur.relocs[line.address] = (line.reloc, line.name)
    return out


def rewrite_spr(mn: str, ops: str) -> tuple[str, str]:
    """`mfhid2 r3` -> `mfspr r3, 920`; `mtgqr 3,r4`-style forms too; the named-register moves are kept."""
    m = _SPR_MN.match(mn)
    if m and mn not in KEEP and m.group(2) in SPR:
        return ("mfspr", "%s,%s" % (ops, SPR[m.group(2)])) if m.group(1) == "mf" else ("mtspr", "%s,%s" % (SPR[m.group(2)], ops))
    m = re.match(r"^(mf|mt)gqr$", mn)
    if m and "," in ops:
        a, b = ops.split(",", 1)
        return ("mfspr", "%s,GQR%s" % (a, b)) if m.group(1) == "mf" else ("mtspr", "GQR%s,%s" % (a, b))
    return mn, ops


def branch_targets(fn: Function) -> set[int]:
    """In-function addresses some non-relocated branch jumps to."""
    out = set()
    for ins in fn.insns:
        if ins.address in fn.relocs or not ins.mnemonic.startswith("b") or ins.mnemonic.startswith(("blr", "bctr", "bclr", "bcctr")):
            continue
        m = _TARGET.search(ins.operands)
        if m:
            out.add(int(m.group(1), 16))
    return out


def operands_for(ins: Insn, rel: tuple[str, str] | None) -> tuple[str, str, str]:
    """`(mnemonic, operands, trailing comment)` with the relocation spelled by its symbol: `bl sym`, `lis r3, sym@ha`,
    `addi r3, r3, sym@l`, `lwz r3, sym@l(r4)`. An SDA21 displacement is written `sym(rA)` with the rA field as encoded
    (objdump prints a zero field as `0`, which is `r0`: the linker adds the small-data base). An SDA21 `li`/`addi`
    has no source spelling MWCC accepts, so it is kept as encoded and the symbol goes in a comment."""
    mn, ops = ins.mnemonic, ins.operands
    if rel is None:
        m = _TARGET.search(ops)
        if m and mn.startswith("b"):
            ops = re.sub(r"[0-9a-f]+ <.*>", "L_%x" % int(m.group(1), 16), ops)
        return mn, ops, ""
    kind, sym = rel
    if mn.startswith("b") and kind == "R_PPC_REL24":
        return mn, sym, ""
    if kind == "R_PPC_REL14":
        return mn, re.sub(r"[0-9a-f]+ <.*>", sym, ops), ""
    if kind == "R_PPC_ADDR16_HA":
        return mn, _NUMBER_TAIL.sub(sym + "@ha", ops), ""
    if kind == "R_PPC_ADDR16_HI":
        return mn, _NUMBER_TAIL.sub(sym + "@h", ops), ""
    if kind == "R_PPC_ADDR16_LO":
        return mn, (_DISPLACEMENT.sub(sym + "@l(", ops) if "(" in ops else _NUMBER_TAIL.sub(sym + "@l", ops)), ""
    if kind in ("R_PPC_EMB_SDA21", "R_PPC_SDAREL16"):
        m = _DISP_BASE.search(ops)
        if m:
            base = "r0" if m.group(1) == "0" else m.group(1)
            return mn, ops[:m.start()] + "%s(%s)" % (sym, base) + ops[m.end():], ""
        return mn, ops, " /* SDA21: %s */" % sym
    return mn, ops, ""


def render(fn: Function, ret: str = "void", args: str = "void") -> str:
    """The `asm` function source for one parsed function."""
    targets = branch_targets(fn)
    lines = ["asm %s %s(%s)" % (ret, fn.name, args), "{", "    nofralloc"]
    for ins in fn.insns:
        if ins.address in targets:
            lines.append("L_%x:" % ins.address)
        rel = fn.relocs.get(ins.address)
        if rel is None:                       # a 16-bit relocation sits on the low half of the word
            rel = fn.relocs.get(ins.address + 2)
        if ins.mnemonic.startswith("."):      # an undecodable word: .long 0x...
            lines.append("    opword %s" % ins.operands.split()[0] if ins.operands else "    opword 0")
            continue
        mn, ops, note = operands_for(ins, rel)
        mn, ops = rewrite_spr(mn, ops)
        lines.append(("    %s %s" % (mn, ops.replace(",", ", ")) if ops else "    %s" % mn) + note)
    lines.append("}")
    return "\n".join(lines)


def generate(text: str, names: list[str] | None = None, **kw) -> tuple[str, list[str]]:
    """`(source, missing)`: the `asm` functions of an `objdump -dr` listing, all of them or the named ones in order."""
    funcs = parse(text)
    want = names or list(funcs)
    missing = [n for n in want if n not in funcs]
    return "\n\n".join(render(funcs[n], **kw) for n in want if n in funcs) + "\n", missing
