# `verify_pcode` - Classify a PCode dump (final vs earlier pass) and compare it to the object: MATCH / FAIL / PASS-DELTA naming the pass

<!-- generated from the module docstring of `tools/mwcc-debugger/locate/verify_pcode.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

Classify a PCode dump, then either verify it against the object or measure its delta.

## Users

profiles (`.claude/agents`) (1); skills (1); CLAUDE.md (1); docs (5)

## CLI

```
python locate/verify_pcode.py <backend-NN-....txt> <file.o> [--symbol NAME] [--json]
```
Flags: `--all`, `--final`, `--json`, `--objdump`, `--strict`, `--symbol`, `--version`.
Exit codes: Exit status: 0 for MATCH and for PASS-DELTA, 1 for a FAIL, 2 for an error (a missing file, no objdump, an unknown build).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: dump + object -> verdict.

## Invariants and rules

* The debugger writes one dump per optimizer pass (`backend-NN-<pass>.txt`), so the question "does this dump reproduce the object?" only has an answer for the *last* one - the state just before assembly. An earlier dump is not claiming to be the final code; what it is worth is the difference between it and the final code, which is exactly what names the pass responsible for a residual.
* So this tool classifies the dump first:
* **the final dump** (its pass name is the last dump point of the build's codegen driver, see `final_pass_name`) is compared against `powerpc-eabi-objdump -d` of the object the same command line produces, and reports MATCH or FAIL with the first divergence;
* **any earlier dump** reports a PASS-DELTA: the instruction-count delta and the concrete instruction changes from that pass forward, with the pass that first reaches the object's stream named when the sibling dumps are still on disk;
* a dump whose pass cannot be identified from its file name reports its delta too (never a bare FAIL: it does not claim to be final), and `--final` forces the strict comparison when the caller knows better.
* Mnemonics are compared after folding the spelling differences between MWCC's PCode and the disassembler (`clrlwi` is an `rlwinm`, `cmplwi` is a `cmpli`, `bgt` is a `bt` on a condition bit, ...), and registers are compared as sets per instruction, because the dump prints a memory operand as `rX,rY,disp` where the disassembler prints `disp(rY)`. The delta uses the same comparison, so a pure operand reordering is not reported as a change.
* **One function, not the object** (2026-10-27): `--symbol NAME` compares that function's code only (its label to
  the next, trailing zero `.long` fill dropped); without it the function the dump's first line names
  (`; function NAME, dump NN pass`) is used when the object defines it, else the whole object, and the report says
  which. A missing `--symbol` exits 2.
* objdump runs with `-M gekko`, so `psq_*` decode as what MWCC emitted. Folded too: `cmplw`/`cmpw` (the dump's
  `cmpl`/`cmp`), `crclr`/`crset` (`crxor`/`creqv`, CR-bit immediates not compared), `bt`/`bf` on any CR field (the
  field compared like a register, cr0 implicit). Not compared, and printed under every MATCH: `fixup[...]` operands
  (relocated - their kind is not a displacement: `lfs f0,r1,fixup[0x0004]` is `lfs f0,104(r1)`), the SDA21 base `r0`,
  `bl`'s register list (the call's clobbers), a `psq_*`'s W/I fields.
* **Branch targets**: in a final dump (block headers carry `offset=`), each branch's target block offset is compared
  with the object's target when the instruction counts agree. Measured on ten real functions (lobby/lb_npc x2,
  lobby/lb_pane_ui, camera/camera_main, NAND/NANDCheck, NAND/NANDLogging, quest/arenatask, RVLGX/GXTev, GXAttr,
  GXTransform): all ten MATCH with 434 branch targets compared; the previous checker FAILed all ten on the same
  dumps (it compared each with the whole object: 357 vs 23068 instructions for `lb_arm_wrestle_step`).
* Exit status: 0 for MATCH and for PASS-DELTA, 1 for a FAIL, 2 for an error (a missing file, no objdump, an unknown build). `--strict` makes a PASS-DELTA exit 1 as well, for a caller that requires the final code.

## Lib dependencies

proc, binary.objdump.

## Test contract

Tier: fixture.
Today's selftest (`tools/mwcc-debugger/locate/verify_pcode_selftest.py`): No compiler, no gdb and no debugger run: the fixtures are hand-written PCode dumps plus one object assembled with the repository's own `powerpc-eabi-as`, so the contract is pinned instead of being re-derived from whatever `build/mwcc-debug/` happens to hold today: * which pass a dump is, and therefore whether the comparison is strict; * that the strict path still FAILS when the final dump disagrees (the check is not vacuous - that is the property the tool exists for); * what a PASS-DELTA says about an early dump: the instruction-count delta, the concrete change, and the pass that made it; * that `--json` stays machine-readable and the exit codes mean what the docstring says.
`tools/tests/mwcc-debugger/test_verify_pcode.py` (`TIER='smoke'`: it assembles with the repository's
`build/binutils`, skipped when absent): a two-function object FAILs whole and MATCHes under `--symbol`/the header, a
moved branch target, a register, a CR field and a `psq` displacement each still FAIL, `-M gekko` decodes `psq_st`,
and the normalisation rows (aliases, SDA21 base, fixup, `bl`) hold.

## Known gaps

None recorded.
