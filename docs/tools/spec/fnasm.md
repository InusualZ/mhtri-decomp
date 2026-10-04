# `fnasm` - One function's disassembly from the target object, ours, or both side by side

## Purpose

Prints one function's instructions out of the unit's split target object (`build/RMHE08/obj/<unit>.o`), our compiled
object (`build/RMHE08/src/<unit>.o`), or both in two columns with a marker per row, by symbol or by DOL address. It
replaces the lanes' scratch `fnasm.py`/`asm.py` (a regex over a saved `objdump -dr` dump, every lane its own copy).

## Users

Decompiler and fixer lanes reading a function before writing it, or a residual after measuring it; `vary.py`'s
divergence reading is the same pairing (`pair`).

## CLI

```
python tools/objdiff/fnasm.py -u Network/NetworkResolverWii __ct__18NetworkResolverWiiFv   # both sides
python tools/objdiff/fnasm.py 0x803CB420                    # an address: the map names the symbol, splits.txt the unit
python tools/objdiff/fnasm.py putOnSendPool --side target   # a name with no -u: the unit comes from its address
python tools/objdiff/fnasm.py -u main/mh3_pad fn_800417F0 --raw   # the raw instruction bytes too
python tools/objdiff/fnasm.py --object a.o [--object b.o] f     # arbitrary objects (first = target, second = ours)
python tools/objdiff/fnasm.py ... --json
```
Flags: `-u/--unit`, `--object` (twice at most), `--side target|ours|both` (default both), `--raw`, `--json`.
Exit codes: 0 at least one side printed, 2 nothing printable (no symbol, no unit owning the address, no object, no objdump).
`--json`: `{symbol, resolved?: {address, symbol, offset}, sides: {target|ours: {object, section, offset, size,
instructions: [{offset, mnemonic, operands, raw, relocs: [{type, symbol}], text}]} | {object, error}}, pair?: {same,
rows, first_divergence, markers}}`.

## Inputs and outputs

Reads the objects (ELF symbol table for the extent, `objdump -dr -j <section>` for the instructions), and for an
address or a unit-less name `config/RMHE08/symbols.txt` and `splits.txt`. Writes nothing.

## Invariants and rules

* A function's extent is its ELF symbol (`value`, `size`) in the object, never the next label in the listing: a
  split object carries local labels inside functions, which ended the lanes' regex blocks early.
* A relocation belongs to the instruction whose four bytes contain its offset (`@ha`/`@l` sit at +2) and is folded into
  the operand (`lis r3,sym@ha`, `addi r3,r3,sym@l`, `lwz r3,sym@sda21(r13)`, `bl sym`); an operand it cannot fold keeps
  the text and appends `# TYPE sym`. A local branch target prints as `+0xNN` from the function start, so the same code
  at a different object offset renders identically on both sides.
* `--side both` pairs rows by index (` ` same text, `|` different, `<`/`>` one side only) and names the first
  divergence; it is a reading aid, not a score - the aligned diff and the official percent are `symdiff.py -u` and
  `unitscore.py`.
* An address resolves through the tree's map (the code symbol covering it, `+0xNN` printed when it is inside); with no
  `-u`, the unit is the `splits.txt` range covering the address.

## Lib dependencies

`lib.binary.objdump` (`locate`, `disassemble`, `tokenize`), `lib.binary.elf`, `lib.units` (`Unit.resolve`),
`lib.project` (`SymbolMap`, `Splits`), `lib.repo`.

## Test contract

Tier: fixture (`tools/tests/objdiff/test_fnasm.py`). A canned `objdump -dr` listing of a two-function `ElfBuilder` object
pins the extent cut, the reloc folding (branch, `@ha`/`@l` at +2, `@sda21`), the relative branch target, the pairing and
its first divergence, and the CLI over a `FixtureTree` (unit by `-u`, by address through map + splits, `--side`,
`--object`, the exit-2 cases) with the objdump call injected (`main(runner=)`). Attaching a relocation only at its exact
instruction address fails 3 checks.

## Known gaps

* Index pairing does not realign after an inserted instruction (objdiff does); the first divergence is exact, rows
  after an insertion read as different.
