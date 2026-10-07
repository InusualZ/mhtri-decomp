"""gen_asm / lib.asmgen: asm source from an `objdump -dr -M gekko` listing - SPR numbers, branch labels, relocation operands,
the SDA21 rA rule - on a synthetic listing; and (smoke) a round trip through the repo's compiler: an asm function
compiled, regenerated from its object, recompiled gives the same bytes and relocations.
Mutation: the SDA21 `sym(rA)` rule off (a plain `sym(r0)`) fails the r13 check and the round trip."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import os
import subprocess
import tempfile

from tools.lib import asmgen, testing
from tools.lib.binary import objdump

TIER = "smoke"

LISTING = """\
f.o:     file format elf32-powerpc

Disassembly of section .text:

00000000 <probe>:
   0:\t7c 08 02 a6 \tmflr    r0
   4:\t80 80 00 00 \tlwz     r4,0(0)
\t\t\t4: R_PPC_EMB_SDA21\tg_flag
   8:\t81 2d 00 00 \tlwz     r9,0(r13)
\t\t\t8: R_PPC_EMB_SDA21\tg_other
   c:\t2c 04 00 00 \tcmpwi   r4,0
  10:\t41 82 00 08 \tbeq     18 <probe+0x18>
  14:\t48 00 00 01 \tbl      14 <probe+0x14>
\t\t\t14: R_PPC_REL24\thelper
  18:\t3c 60 00 00 \tlis     r3,0
\t\t\t1a: R_PPC_ADDR16_HA\tg_table
  1c:\t38 63 00 00 \taddi    r3,r3,0
\t\t\t1e: R_PPC_ADDR16_LO\tg_table
  20:\t80 a3 00 00 \tlwz     r5,0(r3)
\t\t\t22: R_PPC_ADDR16_LO\tg_table
  24:\t7c d8 e2 a6 \tmfhid2  r6
  28:\t7c db e3 a6 \tmthid4  r6
  2c:\t7c fa e2 a6 \tmfdmau  r7
  30:\t7c 08 03 a6 \tmtlr    r0
  34:\t42 00 ff e4 \tbdnz    18 <probe+0x18>
  38:\t4e 80 00 20 \tblr
  3c:\t00 00 00 07 \t.long   0x7

00000040 <second>:
  40:\t38 60 00 00 \tli      r3,0
\t\t\t42: R_PPC_EMB_SDA21\tg_flag
  44:\t4e 80 00 20 \tblr
"""


def test_render(c):
    out, missing = asmgen.generate(LISTING, ["probe"])
    lines = [l.strip() for l in out.splitlines()]
    c.check("header and nofralloc", lines[:3], ["asm void probe(void)", "{", "nofralloc"])
    c.check("an SDA21 load with a zero rA field is sym(r0)", "lwz r4, g_flag(r0)" in lines, True)
    c.check("... and keeps the base register it was encoded with (r13)", "lwz r9, g_other(r13)" in lines, True)
    c.check("branch targets get one label, placed before the target", lines.index("L_18:") + 1, lines.index("lis r3, g_table@ha"))
    c.check("a conditional and a bdnz branch name the label", ("beq L_18" in lines, "bdnz L_18" in lines), (True, True))
    c.check("a REL24 branch names its symbol", "bl helper" in lines, True)
    c.check("@ha / @l operands, the displacement form too",
            ("addi r3, r3, g_table@l" in lines, "lwz r5, g_table@l(r3)" in lines), (True, True))
    c.check("numbered SPRs (HID2 920, HID4 1011, DMAU 922), the named LR move kept",
            ("mfspr r6, 920" in lines, "mtspr 1011, r6" in lines, "mfspr r7, 922" in lines, "mtlr r0" in lines),
            (True, True, True, True))
    c.check("an undecodable word becomes opword", "opword 0x7" in lines, True)
    c.check("an SDA21 li has no source form: kept, the symbol in a comment",
            asmgen.generate(LISTING, ["second"])[0].count("li r3, 0 /* SDA21: g_flag */"), 1)
    c.check("a missing function is named, the rest still emitted", asmgen.generate(LISTING, ["nope", "second"])[1], ["nope"])
    c.check("no names: every function in order", list(asmgen.parse(LISTING)), ["probe", "second"])


def compile_obj(root, src, out):
    cc = os.path.join(root, "build", "compilers", "Wii", "1.3", "mwcceppc.exe")
    flags = "-nodefaults -proc gekko -align powerpc -enum int -fp hardware -Cpp_exceptions off -O4,p -inline auto -lang=c".split()
    p = subprocess.run([cc] + flags + ["-c", src, "-o", out], capture_output=True, text=True, encoding="utf-8", errors="replace")
    return p.returncode == 0 and os.path.exists(out)


def test_round_trip(c):
    root = str(testing.LIVE_ROOT)
    tool = objdump.locate(root)
    if tool is None or not os.path.exists(os.path.join(root, "build", "compilers", "Wii", "1.3", "mwcceppc.exe")):
        c.skip("round trip", "no build/compilers or build/binutils in this tree (slots.py seed-worktree)")
        return
    decls = "extern int g_flag;\nextern int g_table[];\nextern void helper(void);\n"
    source = decls + (
        "asm void probe(void)\n{\n    nofralloc\n    mflr r0\n    stw r0, 4(r1)\n    lwz r4, g_flag(r0)\n"
        "    lwz r9, g_flag(r13)\n    cmpwi r4, 0\n    beq L_skip\n    bl helper\nL_skip:\n"
        "    lis r3, g_table@ha\n    addi r3, r3, g_table@l\n    lwz r5, g_table@l(r3)\n    mfspr r6, 920\n"
        "    mtspr 1011, r6\n    mfspr r7, 922\n    psq_l f1, 8(r3), 0, 0\n    bdnz L_skip\n    lwz r0, 4(r1)\n"
        "    mtlr r0\n    blr\n}\n")
    with tempfile.TemporaryDirectory() as tmp:
        a_c, a_o, b_c, b_o = (os.path.join(tmp, n) for n in ("a.c", "a.o", "b.c", "b.o"))
        with open(a_c, "w", newline="\n") as fh:
            fh.write(source)
        if not compile_obj(root, a_c, a_o):
            c.check("the probe compiles", False, True)
            return
        text_a = objdump.disassemble(tool, a_o, relocs=True, cpu=asmgen.CPU)
        generated, missing = asmgen.generate(text_a)
        with open(b_c, "w", newline="\n") as fh:
            fh.write(decls + generated)
        ok = compile_obj(root, b_c, b_o)
        c.check("the generated source compiles", (ok, missing), (True, []))
        if ok:
            body = lambda t: [l for l in t.splitlines() if l.strip() and "file format" not in l]
            c.check("recompiled: the same instructions, bytes and relocations (objdump -dr identical)",
                    body(objdump.disassemble(tool, b_o, relocs=True, cpu=asmgen.CPU)), body(text_a))


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
