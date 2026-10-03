# `lib/binary` - The bytes: ELF objects (read and write), the DOL, disassembler text, DWARF, and the fixture builders

## Purpose

The bytes: ELF objects (read and write), the DOL, disassembler text, DWARF, and the fixture builders.

## Users

The tools listed under this concept in `docs/tools/README.md`.

## Public API

* `Elf.read(path|bytes)`: `sections` (name, type, addr, offset, size, data, align), `symbols` (name, section, value, size, bind, type, index), `relocs(section)` (offset, symbol, type, type_name, addend), `comment`, `section_bytes`, `defined`, `undefined`, `globals`; `Elf.write(path)` for `objalign`/`objextab`
* `Dol.read(path)`: `segments`, `bytes_at(address, n)`, `section_of(address)`, `text_ranges`, `data_ranges`, `word(address)`
* `objdump.locate(tree)`, `objdump.disassemble(obj|elf, sections)`, `dtk_disasm(obj)`, `tokenize(line) -> (address, mnemonic, operands, reloc)` for the three text shapes
* `dwarf.local_slots(obj, function)` (the `dwarfmap` core)
* `ElfBuilder` (sections, symbols, relocs, comment, version) and `DolBuilder` for fixtures

## Absorbs (today's implementations)

26 ELF readers, 10 DOL readers, 13 ELF and 7 DOL fixture builders, `elfsect.sections`, `objalign/objextab.read_sections`, `dwarfmap`, the objdump parsers in `accessextent`, `m2cinput`, `callees`, `verify_pcode`

## Test contract

Tier: fixture (a lib test never reads the live tree). builder -> reader round trip for every field; the real split objects parse (smoke); `.rela<sec>` pairs with `<sec>`; locals are below `sh_info`

## Known gaps

None until implemented; `migration.md` names the package.
