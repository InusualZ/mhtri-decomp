# `gen_asm` - MWCC `asm` function source from a unit's target object

## Purpose

Emits `asm void f(void) { nofralloc ... }` source from the gekko disassembly of the unit's TARGET object, for the functions
a reconstruction can only state as assembly (SDK cache, context-switch, SPR and DMA code).

## Users

The decompiler and fixer profiles (SDK and runtime units); replaces the per-lane `gen_asm.py`/`mkasm.py` scripts.

## CLI

```
python tools/units/gen_asm.py <unit> [<symbol>...] [--obj PATH] [--list] [-o FILE]
```
No symbols: every function of the object. `--obj` reads another object (any `objdump`-readable ELF) instead of the unit's
`build/RMHE08/obj/<unit>.o`. `--list` prints the function names. Exit 0 ok, 1 a named function is absent, 2 no objdump or
the disassembly failed (`slots.py seed-worktree` provides `build/binutils`).

## Inputs and outputs

Unit (or `--obj`) -> stdout / `-o FILE`: one `asm void name(void)` per function; the return type and the argument list are the
author's to write.

## Invariants and rules

* The disassembler is `objdump -dr -M gekko` (`lib.asmgen.CPU`): the default core decodes paired-single opcodes as VSX/VMX.
* SPR moves: `mf<name>`/`mt<name>` become `mfspr`/`mtspr` with MWCC's name, or the number where it has none (HID2 920, HID4
  1011, WPAR 921, DMAU/DMAL 922/923); `mflr`/`mtlr`/`mfctr`/... stay.
* Branch targets inside the function get `L_<hex offset>:` labels; a REL24 branch names its symbol.
* Relocations are spelled by their symbol: `lis r3, sym@ha`, `addi r3, r3, sym@l`, `lwz r5, sym@l(r3)`.
* An SDA21 displacement is `sym(rA)` with the rA field as encoded: objdump prints a zero field as `0`, written `r0` (the linker
  adds the small-data base); `r13`/`r2` stay as they were. An SDA21 `li`/`addi` has no spelling MWCC accepts (`addi rD, r0, sym`
  is "illegal object reference"): it is kept as encoded with a `/* SDA21: sym */` comment.
* An undecodable word (`.long`) is `opword 0x..`.

## Lib dependencies

`lib.asmgen` (parse, rewrite, render), `lib.binary.objdump` (locate, run, tokenize), `units`, `repo`.

## Test contract

`tools/tests/units/test_gen_asm.py` (smoke: the round trip needs the compiler): a synthetic listing pins every rule above (13
checks); the round trip compiles an `asm` function with SDA21 (r0 and r13), HA/LO, branches, HID2/HID4/DMA SPRs and `psq_l`,
regenerates it from the object, recompiles, and compares `objdump -dr` of both objects (identical). Mutation: the SDA21 rA rule
off (every base written `r0`) fails 2 checks.

## Known gaps

* The SDA21 `li`/`addi` form and a relocation against `.text+off` of a local label are kept numeric.
* The signature is always `void f(void)`; return value, arguments, `extern` declarations and the file's `#include`s are the
  author's.
* Data in `.text` after the function's end (jump tables are not in `.text`) is not recognised.

## History

* Replaces three hand-rebuilt lane scripts (about 70 bodies); the SDA21 rule fixes their `\(\d+\)` pattern, which missed
  `0(r13)` and left a numeric operand with no relocation.
