# `lib/asmgen` - MWCC `asm` source from a gekko `objdump -dr` listing

## Purpose

The text transform behind `tools/units/gen_asm.py`: parse a listing into functions and render each as an `asm` function.

## Users

`tools/units/gen_asm.py`; `tools/tests/units/test_gen_asm.py`.

## Public API

* `CPU` - the `-M` value (`gekko`) the listing must be made with.
* `parse(text) -> {label: Function}` - instructions and relocations of every `.text`/`.init` label (`lib.binary.objdump.tokenize`).
* `operands_for(insn, reloc) -> (mnemonic, operands, comment)` - the relocation spelled by its symbol (`@ha`/`@h`/`@l`, `sym(rA)`
  for SDA21 with the encoded base, REL24/REL14 branch names).
* `rewrite_spr(mnemonic, operands)` - `mf<spr>`/`mt<spr>` to `mfspr`/`mtspr` by MWCC name or number; `SPR` is the table.
* `branch_targets(fn)`, `render(fn, ret, args)`, `generate(text, names) -> (source, missing)`.

## Invariants and rules

* A 16-bit relocation is found at `address + 2`; a branch relocation at `address`.
* The SDA21 base register is kept as encoded (a zero field is `r0`); the rule lives only in `operands_for`.

## Test contract

`tools/tests/units/test_gen_asm.py` (see `spec/gen_asm.md`).

## Known gaps

See `spec/gen_asm.md`.
