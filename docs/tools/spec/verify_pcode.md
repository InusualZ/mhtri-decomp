# `verify_pcode` - Classify a PCode dump (final vs earlier pass) and compare it to the object: MATCH / FAIL / PASS-DELTA naming the pass

<!-- generated from the module docstring of `tools/mwcc-debugger/locate/verify_pcode.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

Classify a PCode dump, then either verify it against the object or measure its delta.

## Users

profiles (`.claude/agents`) (1); skills (1); CLAUDE.md (1); docs (5)

## CLI

```
python locate/verify_pcode.py <backend-NN-....txt> <file.o> [--json]
```
Flags: `--all`, `--final`, `--json`, `--objdump`, `--strict`, `--version`.
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
* Exit status: 0 for MATCH and for PASS-DELTA, 1 for a FAIL, 2 for an error (a missing file, no objdump, an unknown build). `--strict` makes a PASS-DELTA exit 1 as well, for a caller that requires the final code.

## Lib dependencies

proc, binary.objdump.

## Test contract

Tier: fixture.
Today's selftest (`tools/mwcc-debugger/locate/verify_pcode_selftest.py`): No compiler, no gdb and no debugger run: the fixtures are hand-written PCode dumps plus one object assembled with the repository's own `powerpc-eabi-as`, so the contract is pinned instead of being re-derived from whatever `build/mwcc-debug/` happens to hold today: * which pass a dump is, and therefore whether the comparison is strict; * that the strict path still FAILS when the final dump disagrees (the check is not vacuous - that is the property the tool exists for); * what a PASS-DELTA says about an early dump: the instruction-count delta, the concrete change, and the pass that made it; * that `--json` stays machine-readable and the exit codes mean what the docstring says.
Target: `tools/tests/mwcc-debugger/test_verify_pcode.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

None recorded.
