# `lib/rawsame` - A function's raw bytes, target against ours, with only the relocated operand bits masked

## Purpose

The raw per-function compare behind `rawsame.py`: words identical outside the bits each relocation owns, and the same
relocation type at the same word on both sides.

## Users

`tools/objdiff/rawsame.py`.

## Public API

* `MASKS` - `{relocation type: bits of the 32-bit word the linker rewrites}`; SDA21 is `0xFFFF` (its `rA` is compared).
* `functions(elf) -> {name: (section index, offset, size)}` - defined `STT_FUNC` symbols, first definition of a name.
* `word_relocs(elf) -> {section index: {word offset: type}}` - an offset is rounded down to its word.
* `compare_function(name, t, o, tloc, oloc, trel, orel) -> Row`, `compare(target, ours, names) -> [Row]` (paths, bytes or
  `Elf`s); `Row.status` is `same`, `size`, `bytes`, `missing-target`, `missing-ours`; `Row.diffs` is `[(offset, kind,
  target, ours)]`, `kind` `word` or `reloc`; `Row.line(unit)` is the printed row.

## Invariants and rules

* A relocation present on one side only, or of another type, is a `reloc` difference and the word is not byte-compared.
* The mask is a table, one entry per relocation type; an unknown type masks nothing.

## Test contract

Tier: fixture, `tools/tests/objdiff/test_rawsame.py`.

## Known gaps

No symbol or addend comparison (`lib.objcompare.by_owner` / `relocdiff.py`); no data sections.
