# `infer` - Infer compiler flags from a target object's bytes with evidence and confidence (peephole, fp_contract, pool, str, lmw/stmw, func_align, inline, extab)

<!-- generated from the module docstring of `tools/flags/infer.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

Infer the compiler flags a target object was built with, from the object's own bytes.

## Users

the landing gate (2); configure.py / the build (2); skills (1); CLAUDE.md (1); docs (2)

## CLI

```
python tools/flags/infer.py <unit|object>          # one unit
python tools/flags/infer.py --all                  # every registered unit
python tools/flags/infer.py --accuracy             # + the known-case accuracy table
python tools/flags/infer.py <unit> --json          # machine-readable
```
Flags: `--accuracy`, `--all`, `--json`, `--markdown`, `--selftest`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: target object -> flags + evidence.

## Invariants and rules

* A *unit* is spelled as in `configure.py` (`Camellia/camellia.c`, `auto/80073398_fn_80073398.cpp`) or as a path to a split target object (`build/RMHE08/obj/...o`). The object is the *target* object under `build/RMHE08/obj/` (the original split out of the DOL), never our `build/RMHE08/src/` build.
* Fingerprints implemented ------------------------
* record forms (`rlwinm.`/`clrlwi.`/`and.`/`add.`/`extsb.`/...) - only the peephole pass emits them, so their presence proves `-opt peephole` on; their absence proves nothing.
* fused multiply-adds (`fmadds`/`fmsubs`/...) vs an unfused `fmuls`+`fadds` chain - the contraction.
* pooled literal addressing: a single `lis` base register reused across several data symbols (pool on) vs one `lis`+`addi` pair per symbol (pool off).
* the string pool's section (`.rodata` = `-str ...,readonly`, `.data` = not readonly).
* the FPR/`GPR` save/restore shape: `stmw`/`lmw` (`-use_lmw_stmw on`) vs the EABI `_savegpr_*`/ `_restgpr_*` helpers (off).
* function start alignment and inter-function `gap_*` padding: a start off a 16-byte boundary proves `-func_align 4`; every start 16-byte aligned *with* `gap_*` padding is only consistent with 16 (`-O4,p` implies it, and a `#pragma function_align 16` restores it), because a `-func_align 4` unit can land all-16-aligned too, so that direction is a hint.
* a kept `bl` to a tiny function defined in the same object - consistent with `-inline noauto`, but a hint only: `-inline auto` is a heuristic, a source `#pragma dont_inline` also keeps the call, and in a multi-TU split object the callee may be a different original translation unit.
* `extab`/`extabindex` presence, `.ctors`/`.dtors` fragments, and the `.comment` byte (reported as evidence; the split objects' `.comment` is synthesized from `config.yml` and is uniform here).
* Instruction decoding is done on the raw big-endian words, not through `objdump`: GNU objdump in the pinned binutils mis-decodes the Gekko paired-single instructions (primary opcode 4) as VMX, which would hide the indexed `psq_lx`/`psq_stx` epilogue that row 39 is about.
* Nothing here compiles, links, re-splits or writes to the repository.

## Lib dependencies

binary, ppc, units, project.configure.

## Test contract

Tier: fixture (synthetic ELF per fingerprint); smoke: the accuracy table (parked).
Today's selftest (`tools/flags/infer_selftest.py`): Two layers: * pure unit tests of the instruction decoder, the cflags/pragma ground-truth parser and the `Insn` predicates - no files, no build; * one synthetic ELF32 big-endian object per fingerprint, written here, so the contract is pinned without depending on `build/RMHE08/obj/` (which only exists after a `dol split`).
Target: `tools/tests/flags/test_infer.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

the `pool` detector false positive (park list)

## History (the incidents behind the rules - keep the rule, drop the narrative when the rule is stable)

* Why this exists --------------- The matching playbook (rows 39-46) records eight levers that are derivable from the target bytes alone - `-opt nopeephole`, `-fp_contract off`, `-pool off`, `-str ...` (not `readonly`), `-use_lmw_stmw off`, `-func_align 4`, `-inline noauto`, the C++ front-end - and every one of them was rediscovered independently by a worker who had to notice the same byte pattern. This tool applies those rules automatically and reports, per flag, the evidence and a confidence. It is a *pre-flight*, not an oracle: it reads one side of the diff (the target), so a flag whose effect is source-dependent (the peephole pass, the optimizer level) can only be inferred one way, and the tool says so instead of guessing.
