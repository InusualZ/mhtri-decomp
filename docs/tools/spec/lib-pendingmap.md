# `lib/pendingmap` - Pending `symbols.txt` edits applied to a split target object, so a unit scores as the map will read

## Purpose

A lane whose source already defines the new names reads low until the map half of its renames lands: the report pairs
symbols by name, so every renamed row is unpaired (`g3d/g3d_state` read 73.06 on the map, 99.30 with the 29 renames of
`g3d-state#4`). This module applies the pending edits - renames and row splits - to a **copy** of the split target's
symbol table, so `measure.py --rename-map` and `tryvar.py --rename-map` score the unit as it will read after the landing,
without touching `symbols.txt`, the split or a re-split.

## Users

`tools/units/measure.py --rename-map`, `tools/flags/tryvar.py --rename-map` (every mode: variants, `--permdecl`, `--hill`).

## Public API

* `parse(text) -> [Op]` / `load(path)`: one op per line - `old new` (the `symedit.py rename-batch` shape), `rename old new`,
  or `split <row> <offset> <new-name> [--scope global|local|weak]` (`symedit.py split`'s arguments); `#` starts a comment.
  Anything else raises `ValueError` naming the line.
* `Op(kind, old, new, offset, scope, line)`.
* `apply(raw, ops) -> (bytes, Outcome)`: the object with the ops applied in order. `Outcome`: `renamed` `(old, new,
  entries)`, `split` `(row, new, offset)`, `absent` (names the object neither defines nor references - normal for a rename
  of another unit's symbol), `refused` `(line, reason)`; `summary()`, `to_json()`.
* `apply_file(src, dst, ops) -> Outcome`: the same from a path to another path (refuses `dst == src`).

## Invariants and rules

* A rename renames **every** `.symtab` entry called `old`, defined or undefined, so a relocation to a renamed callee follows
  it (the symbol index is unchanged). It is refused when `old` and `new` are both defined in the object.
* A split shrinks the one defined `row` to `offset` bytes and appends `new` at `row + offset` with the remainder, in the same
  section and of the same type; its binding is `--scope`'s, else the row's. Refused: the row defined more than once, an
  offset not strictly inside the row, `new` already defined.
* Section contents and relocations are never touched. The grown `.strtab` and `.symtab` are written at the end of the file and
  their two section headers re-pointed, so no other offset moves (no splice, unlike `ElfEditor`); appended symbols keep every
  existing index. An object no op touches is returned byte-identical.
* The split target is never rewritten in place: the callers write the copy under their own scratch
  (`build/tmp/measure/<unit>.pending.o`, `build/tmp/tryvar/<unit>.pending.o`).

## Test contract

Tier: fixture (`tools/tests/lib/test_pendingmap.py`, `ElfBuilder` objects). The parser's three line shapes and four
refusals; renames of a defined row and an undefined callee with the relocation following (renaming only defined symbols
fails "a relocation follows its renamed symbol"); a split's two rows and `--scope`; every non-symbol section byte-identical;
the three refusals; `apply_file` never in place. `tools/units/measure_selftest.py` (`rename_map_rows`) pins that the report
scores the pending copy and leaves the split object untouched; `tools/tests/flags/test_tryvar.py` covers tryvar's use.
Live check (2026-10-21, objdiff-cli on the real object): `g3d/g3d_state` with the `g3d-state#4` renames reverted in the map
read 73.05624; `measure.py --rename-map` with the 29 renames read 99.30054, the unit's score once the map landed.

## Known gaps

* A split does not add the relocation `dtk` would emit for a branch from the first half into the second (the target keeps a
  plain intra-function branch), so a split row whose code branches across the cut can read slightly lower than after a real
  re-split.
* An appended `local` symbol sits after the globals (`sh_info` is not moved); objdiff does not mind, a strict ELF reader
  would.
