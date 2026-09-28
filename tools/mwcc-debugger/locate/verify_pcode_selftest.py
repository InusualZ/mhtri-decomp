#!/usr/bin/env python3
"""Self-test for tools/mwcc-debugger/locate/verify_pcode.py.

    python tools/mwcc-debugger/locate/verify_pcode_selftest.py

No compiler, no gdb and no debugger run: the fixtures are hand-written PCode
dumps plus one object assembled with the repository's own `powerpc-eabi-as`, so
the contract is pinned instead of being re-derived from whatever
`build/mwcc-debug/` happens to hold today:

* which pass a dump is, and therefore whether the comparison is strict;
* that the strict path still FAILS when the final dump disagrees (the check is
  not vacuous - that is the property the tool exists for);
* what a PASS-DELTA says about an early dump: the instruction-count delta, the
  concrete change, and the pass that made it;
* that `--json` stays machine-readable and the exit codes mean what the docstring
  says.

The objdump-dependent rows are skipped (reported, not faked) when
`build/binutils` has not been downloaded; the pure classification rows always run.
"""
from __future__ import annotations

import contextlib
import io
import json
import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
if str(HERE) not in sys.path:
    sys.path.insert(0, str(HERE))

import verify_pcode as vp  # noqa: E402  (imported through the sys.path shim above)

AS = ROOT / "build/binutils/powerpc-eabi-as.exe"
OBJDUMP = ROOT / "build/binutils/powerpc-eabi-objdump.exe"

SKIP = object()

# The object: a function whose address computation the peephole pass fuses.
# Written with the repository's assembler so the objdump side is real.
FIXTURE_ASM = """\
    .text
    .globl fn_fixture
    .type fn_fixture, @function
fn_fixture:
    stwu 1,-16(1)
    mflr 0
    stw 0,20(1)
    stw 31,12(1)
    mr 31,3
    li 0,0
    stb 0,0(5)
    clrlwi 0,4,16
    mulli 0,0,12
    add 7,3,0
    lbzu 0,3584(7)
    cmplwi 0,15
    bgt .Ldone
    li 3,0
.Ldone:
    lwz 31,12(1)
    lwz 0,20(1)
    mtlr 0
    addi 1,1,16
    blr
"""

# The final PCode of the function above: 19 instructions.
FUSED = """\
B0: line=1 pcode=4 (successors/predecessors/labels not dumped: not derived)
     1  stwu     r1,r1,-0x10
     1  mflr     r0,lr
     1  stw      r0,r1,0x14
     1  stw      r31,r1,0xc

B1: line=1 pcode=13 (successors/predecessors/labels not dumped: not derived)
     1  mr       r31,r3
     1  li       r0,0
     1  stb      r0,r5,0
     1  rlwinm   r0,r4,0,0x10,0x1f
     1  mulli    r0,r0,0xc
     1  add      r7,r3,r0
     1  lbzu     r0,r7,0xe00
     1  cmpli    cr0,r0,0xf
     1  bt       cr0,1,@L713
     1  li       r3,0

B2: line=1 pcode=5 (successors/predecessors/labels not dumped: not derived)
     1  lwz      r31,r1,0xc
     1  lwz      r0,r1,0x14
     1  mtlr     r0,lr
     1  addi     r1,r1,0x10
     1  blr
"""

# The same function one pass earlier: the address computation is not fused yet,
# so `add`/`addi`/`lbz` are three instructions where the object has two.
UNFUSED = FUSED.replace(
    "     1  add      r7,r3,r0\n     1  lbzu     r0,r7,0xe00\n",
    "     1  add      r6,r3,r0\n"
    "     1  addi     r7,r6,0xe00\n"
    "     1  lbz      r0,r7,0\n",
)
assert UNFUSED != FUSED and UNFUSED.count("\n") == FUSED.count("\n") + 1

FUSION_CHANGE = (
    "[3->2] add r6,r3,r0 / addi r7,r6,0xe00 / lbz r0,r7,0  ->  "
    "add r7,r3,r0 / lbzu r0,3584(r7)"
)
FUSION_ATTRIBUTED = (
    "[3->2] add r6,r3,r0 / addi r7,r6,0xe00 / lbz r0,r7,0  ->  "
    "add r7,r3,r0 / lbzu r0,r7,0xe00"
)


def write_dumps(directory, **named):
    Path(directory).mkdir(parents=True, exist_ok=True)
    for name, text in named.items():
        (Path(directory) / name.replace("__", "-")).write_text(text, encoding="utf-8")


def run_cli(argv):
    """Run the checker in-process; returns (exit code, stdout)."""
    buffer = io.StringIO()
    with contextlib.redirect_stdout(buffer):
        code = vp.main([str(a) for a in argv])
    return code, buffer.getvalue()


def fixture_object(tmp) -> Path:
    """Assemble FIXTURE_ASM with the repository's own binutils."""
    src = Path(tmp) / "fixture.s"
    obj = Path(tmp) / "fixture.o"
    src.write_text(FIXTURE_ASM, encoding="utf-8")
    subprocess.run([str(AS), "-o", str(obj), str(src)], check=True, capture_output=True)
    return obj


def rows(tmp):
    have_binutils = AS.exists() and OBJDUMP.exists()
    obj = fixture_object(tmp) if have_binutils else None

    # -- pure classification (no object needed) -----------------------------
    yield (
        "final pass of Wii/1.3 is after-code-labels",
        vp.final_pass_name(vp.version_rows().row("Wii/1.3")),
        "after-code-labels",
    )
    same_a = vp.normalise([("rlwinm", "r0,r4,0,0x10,0x1f")])[0]
    same_b = vp.normalise([("clrlwi", "r0,r4,16")])[0]
    yield "a spelling difference is not a change", vp.same(same_a, same_b), True
    real_a = vp.normalise([("add", "r6,r3,r0")])[0]
    real_b = vp.normalise([("add", "r7,r3,r0")])[0]
    yield "a register change is a change", vp.same(real_a, real_b), False

    if not have_binutils:
        yield "objdump-dependent rows (build/binutils is not downloaded)", SKIP, SKIP
        return

    unfused_insns = vp.normalise(vp.read_dump(_as_file(tmp, "unfused", UNFUSED)))
    obj_insns = vp.normalise(vp.read_objdump(obj, OBJDUMP))
    hunks = vp.align(unfused_insns, obj_insns)
    yield "align finds the fusion as one hunk", len(hunks), 1
    yield (
        "the hunk is the three instructions the object has two of",
        (hunks[0][1] - hunks[0][0], hunks[0][3] - hunks[0][2]),
        (3, 2),
    )

    # -- the strict path: a dump that claims to be final --------------------
    final_dir = Path(tmp) / "final"
    write_dumps(final_dir, **{"backend-00-after-code-labels.txt": FUSED})
    code, out = run_cli([final_dir / "backend-00-after-code-labels.txt", obj])
    yield "final dump that reproduces the object: exit 0", code, 0
    yield "final dump that reproduces the object: MATCH", "MATCH:" in out, True

    bad_dir = Path(tmp) / "bad"
    write_dumps(bad_dir, **{"backend-00-after-code-labels.txt": UNFUSED})
    code, out = run_cli([bad_dir / "backend-00-after-code-labels.txt", obj])
    yield "final dump that disagrees still FAILS (exit 1)", code, 1
    yield "final dump that disagrees says FAIL", "FAIL:" in out, True
    yield (
        "the failure names the first divergence",
        "first divergence: " in out,
        True,
    )

    # -- the delta path: an early dump --------------------------------------
    early_dir = Path(tmp) / "early"
    write_dumps(
        early_dir,
        **{
            "backend-30-after-prologue-epilogue.txt": UNFUSED,
            "backend-31-after-peephole.txt": FUSED,
        },
    )
    code, out = run_cli([early_dir / "backend-30-after-prologue-epilogue.txt", obj])
    yield "an early dump does not FAIL (exit 0)", code, 0
    yield "an early dump reports PASS-DELTA", "PASS-DELTA:" in out, True
    yield "the delta names the pass and its count delta", "dump 20, object 19 (-1)" in out, True
    yield "the delta prints the concrete change", FUSION_CHANGE in out, True
    yield (
        "the delta attributes the change to the pass that made it",
        "first reached at dump 31 'after-peephole'" in out and FUSION_ATTRIBUTED in out,
        True,
    )

    code, out = run_cli([early_dir / "backend-30-after-prologue-epilogue.txt", obj, "--strict"])
    yield "--strict turns a PASS-DELTA into exit 1", code, 1

    code, out = run_cli([early_dir / "backend-31-after-peephole.txt", obj])
    yield "an early dump that already is the object's MATCHes", "MATCH:" in out, True
    yield (
        "and says it is not the last dump written",
        "this is not the final pass ('after-code-labels')" in out,
        True,
    )

    # -- an interrupted run, and a dump whose pass cannot be identified ------
    partial_dir = Path(tmp) / "partial"
    write_dumps(partial_dir, **{"backend-30-after-prologue-epilogue.txt": UNFUSED})
    code, out = run_cli([partial_dir / "backend-30-after-prologue-epilogue.txt", obj])
    yield (
        "an interrupted run is reported as incomplete, not as a failure",
        "the run looks incomplete" in out and code == 0,
        True,
    )

    odd_dir = Path(tmp) / "odd"
    write_dumps(odd_dir, **{"dump.txt": UNFUSED})
    code, out = run_cli([odd_dir / "dump.txt", obj])
    yield "an unidentifiable file name deltas, never FAILs", (code, "PASS-DELTA:" in out), (0, True)
    yield (
        "and says why it cannot be classified",
        "unidentified" in out,
        True,
    )
    code, out = run_cli([odd_dir / "dump.txt", obj, "--final"])
    yield "--final forces the strict comparison on it", code, 1

    # -- machine-readable output --------------------------------------------
    code, out = run_cli([early_dir / "backend-30-after-prologue-epilogue.txt", obj, "--json"])
    report = json.loads(out)
    yield "--json is valid JSON and only JSON", isinstance(report, dict), True
    yield "--json status", report["status"], "pass-delta"
    yield "--json change list", [c["dump"] for c in report["changes"]], [
        ["add r6,r3,r0", "addi r7,r6,0xe00", "lbz r0,r7,0"]
    ]
    yield "--json attribution pass", report["attribution"]["pass"], "after-peephole"
    yield "--json counts", (report["dump_instructions"], report["object_instructions"]), (20, 19)

    code, out = run_cli([final_dir / "backend-00-after-code-labels.txt", obj, "--json"])
    yield "--json on a match", json.loads(out)["status"], "match"


def _as_file(tmp, name, text) -> Path:
    path = Path(tmp) / f"{name}.txt"
    path.write_text(text, encoding="utf-8")
    return path


def main() -> int:
    failures = skipped = checks = 0
    with tempfile.TemporaryDirectory(prefix="verify_pcode-selftest-") as tmp:
        for label, got, want in rows(tmp):
            if want is SKIP:
                skipped += 1
                print(f"skip  {label}")
                continue
            checks += 1
            if got == want:
                print(f"ok    {label}")
            else:
                failures += 1
                print(f"FAIL  {label}\n        got:  {got!r}\n        want: {want!r}")
    print(
        f"{'FAILED' if failures else 'passed'}: {failures} failure(s), "
        f"{checks} checks, {skipped} skipped"
    )
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
