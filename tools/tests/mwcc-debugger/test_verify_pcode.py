"""verify_pcode.py on a two-function object: `--symbol` (or the dump's header) compares one function, objdump decodes
Gekko `psq_*`, the extended spellings (`cmplw`, `cmpw`, `crclr`) and a CR field other than cr0 compare, the SDA21 r0
base, `fixup[...]` operands and `bl`'s register list are not compared, and the branch targets a final dump's block
offsets give are compared with the object's."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import contextlib
import importlib
import io
import subprocess
import tempfile
from pathlib import Path

from tools.lib import testing

TIER = "smoke"  # it runs the repository's own build/binutils assembler and objdump
ROOT = Path(__file__).resolve().parents[3]
AS = ROOT / "build/binutils/powerpc-eabi-as.exe"
OBJDUMP = ROOT / "build/binutils/powerpc-eabi-objdump.exe"
vp = importlib.import_module("tools.mwcc-debugger.locate.verify_pcode")

ASM = """\
    .text
    .globl first
first:
    li 3,0
    blr
    .globl second
second:
    stwu 1,-16(1)
    mflr 0
    cmplw 3,4
    cmpwi 1,5,0
    ble 1,.Lb
    bl first
    lfs 1,0(0)
    psq_st 31,200(1),0,0
    crclr 6
.Lb:
    lfs 0,104(1)
    blr
"""

# `second`'s final PCode as the Wii/1.3 port writes it (block offsets assigned at assembly).
DUMP = """\
; function second, dump 00 after-code-labels
B0: Successors = { B2 B1 }  Predecessors = {  }  Labels = { L0 }  line=1 pcode=5 offset=0x0
     1  stwu     r1,r1,-0x10
     1  mflr     r0,lr
     1  cmpl     cr0,r3,r4
     1  cmpi     cr1,r5,0
     1  bf       cr1,1,B2

B1: Successors = { B2 }  Predecessors = { B0 }  Labels = { L1 }  line=1 pcode=4 offset=0x14
     1  bl       fixup[0x0000],r3,r4,r5
     1  lfs      f1,r0,fixup[0x0000]
     1  psq_st   f31,r1,0xc8,0,1
     1  crxor    cr1,2,cr1,2,cr1,2

B2: Successors = {  }  Predecessors = { B0 B1 }  Labels = { L2 }  line=1 pcode=2 offset=0x24
     1  lfs      f0,r1,fixup[0x0004]
     1  blr
"""


def run_cli(argv):
    buffer = io.StringIO()
    with contextlib.redirect_stdout(buffer), contextlib.redirect_stderr(io.StringIO()):
        code = vp.main([str(a) for a in argv])
    return code, buffer.getvalue()


def test_verify_pcode(c):
    if not (AS.exists() and OBJDUMP.exists()):
        c.skip("verify_pcode on an assembled object", "build/binutils is not downloaded")
        return
    with tempfile.TemporaryDirectory() as td:
        td = Path(td)
        (td / "f.s").write_text(ASM, encoding="utf-8")
        subprocess.run([str(AS), "-mgekko", "-o", str(td / "f.o"), str(td / "f.s")], check=True,
                       capture_output=True)
        obj = td / "f.o"
        run = td / "run"
        run.mkdir()

        def dump(text, name="backend-00-after-code-labels.txt"):
            (run / name).write_text(text, encoding="utf-8")
            return run / name

        plain = dump(DUMP.split("\n", 1)[1])
        code, out = run_cli([plain, obj, "--objdump", OBJDUMP])
        c.check("without a symbol the whole object is compared: the 2 extra instructions FAIL", (code, "FAIL:" in out,
                "dump 11, object 13" in out), (1, True, True))
        code, out = run_cli([plain, obj, "--symbol", "second", "--objdump", OBJDUMP])
        c.check("--symbol compares one function: MATCH", (code, "MATCH:" in out), (0, True))
        c.contains("... with its branch target compared", out, "and 1 branch target(s)")
        c.contains("... and says what it did not compare", out, "not compared: relocated operands")
        code, out = run_cli([dump(DUMP), obj, "--objdump", OBJDUMP])
        c.check("the dump's header names the function when --symbol is absent", (code, "from the dump's header" in out),
                (0, True))
        code, out = run_cli([dump(DUMP.replace("function second", "function third")), obj, "--objdump", OBJDUMP])
        c.contains("a header naming a function the object lacks falls back to the whole object", out,
                   "which the object does not define")
        code, out = run_cli([dump(DUMP.replace("offset=0x24", "offset=0x28")), obj, "--objdump", OBJDUMP])
        c.check("a branch whose target block sits elsewhere FAILs", (code, "branch target +0x28 (dump) vs +0x24" in out),
                (1, True))
        code, out = run_cli([dump(DUMP.replace("cmpl     cr0,r3,r4", "cmpl     cr0,r3,r5")), obj, "--objdump",
                             OBJDUMP])
        c.check("a register change still FAILs", (code, "registers" in out), (1, True))
        code, out = run_cli([dump(DUMP.replace("bf       cr1,1,B2", "bf       cr2,1,B2")), obj, "--objdump",
                             OBJDUMP])
        c.check("a branch on another CR field FAILs", code, 1)
        code, out = run_cli([dump(DUMP.replace("0xc8,0,1", "0xc0,0,1")), obj, "--objdump", OBJDUMP])
        c.check("a psq displacement change FAILs", code, 1)
        c.contains("objdump decodes paired singles (-M gekko)",
                   [m for m, _o in vp.read_objdump(obj, OBJDUMP, "second")], "psq_st")
        code, _out = run_cli([dump(DUMP), obj, "--symbol", "missing", "--objdump", OBJDUMP])
        c.check("an absent --symbol cannot run (exit 2)", code, 2)


def test_normalise(c):
    n = vp.normalise
    c.check("cmplw is cmpl", vp.same(n([("cmpl", "cr0,r3,r4")])[0], n([("cmplw", "r3,r4")])[0]), True)
    c.check("cmpw is cmp", vp.same(n([("cmp", "cr0,r3,r4")])[0], n([("cmpw", "r3,r4")])[0]), True)
    c.check("crclr is crxor", vp.same(n([("crxor", "cr1,2,cr1,2,cr1,2")])[0], n([("crclr", "4*cr1+eq")])[0]), True)
    c.check("the SDA21 r0 base is not a register",
            vp.same(n([("lfs", "f1,r0,fixup[0x0000]")])[0], n([("lfs", "f1,0(0)")])[0]), True)
    c.check("a fixup's kind is not an immediate (lfs fN,x(r1) of a local)",
            vp.same(n([("lfs", "f0,r1,fixup[0x0004]")])[0], n([("lfs", "f0,104(r1)")])[0]), True)
    c.check("bl's register list is not compared",
            vp.same(n([("bl", "fixup[0x0000],r31,r30")])[0], n([("bl", "8 <x+0x8>")])[0]), True)
    c.check("bf on cr1 is ble cr1", vp.same(n([("bf", "cr1,1,B16")])[0], n([("ble", "cr1,1c00 <f+0x130>")])[0]), True)
    c.check("... and not ble on cr0", vp.same(n([("bf", "cr1,1,B16")])[0], n([("ble", "1c00 <f+0x130>")])[0]), False)


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
