# `lib/binary` - The bytes: ELF objects (read and write), the DOL, disassembler text, DWARF, and the fixture builders

## Purpose

The bytes: ELF objects (read and write), the DOL, disassembler text, DWARF, and the fixture builders. Stdlib only;
the package `__init__` imports nothing, so a tool pays only for the submodule it uses.

## Users

The tools listed under this concept in `docs/tools/README.md`. Delegating today (WP1b): `elfsect`, `objalign`,
`objextab`, `dwarfmap`, `dossier`, `infer`, `linkorder`, `flipcheck`, `langcheck`,
`sectiongap`, `vtableaudit`, `dataclaim`, `datagap`, `dataseams`, `unwindcut`, `splitcheck`, `tudiscover`,
`m2cinput` (and `phantom` through it), `accessextent`, `callees`, `verify_pcode`.

## Public API

* `elf.Elf.read(path|bytes)` -> `Elf`: `sections` (`Section`: index, name, type, flags, addr, offset, size, link, info,
  align, entsize, header, name_offset, `raw` = the file slice, `data` = `raw` or empty for NOBITS), `section(name)`
  (the first of a repeated name), `section_bytes(name)`, `section_name(index)`, `symtab`, `locals_end` (`sh_info`),
  `symbols` (`Symbol`: index, name, value, size, info, other, shndx, section, header; `bind`, `type`, `defined`; the
  null symbol included), `symbol_name(i)`, `defined`/`undefined`/`globals` name sets, `rela_sections()`,
  `relocs(section=None)` (`Reloc`: offset, symbol index, symbol_name, type, addend, section, rela, rela_index;
  `type_name`), `comment` (`Comment`: version, compiler, pool_data, float_type, processor, quirks, `entries` of
  `CommentEntry(align, visibility, active_flags)`), `program_headers`, `entry`, `require_be32()`.
  `ElfError` (a `ValueError`) for bad magic or a header table outside the file; `RELOC_NAMES`, `reloc_name(t, fallback)`.
* `elf.ElfEditor(elf)`: `set_section_align`, `set_symbol_info`, `rename_symbols([(index, name)])` (appends to `.strtab`,
  splices it back, shifts later sections padded to their largest alignment), `to_bytes()`, `write(path)` - the
  `objalign`/`objextab` writer.
* `dol.Dol.read(path|bytes)`: `segments` (non-empty `Segment(name, kind, index, address, size, offset)`, text first),
  `slots` (all eighteen), `bss_address`, `bss_size`, `entry`, `bytes_at(address, n)` (wholly inside one segment or
  None), `bytes_from(address, n)` (starts inside a segment, may run on), `word`, `words`, `cstr`, `section_of`,
  `in_text`, `text_ranges`, `data_ranges`. `DolError` (a `ValueError`) under the 0x100 header.
* `objdump.tokenize(line) -> Line | None` for the three text shapes (objdump `-d[r]`, `dtk elf disasm`, the asm dump):
  `Line.kind` in insn / label / section / reloc / header / fn / endfn, with address, mnemonic (record-form dot kept),
  operands, text, raw, body, name, reloc, offset, indented. `split_text`, `locate(root)`, `run_objdump(objdump, args,
  path, check)`, `disassemble(objdump, path, sections, relocs, raw, cpu)`, `dtk_disasm(obj, dtk, cwd)`.
* `dwarf`: `uleb`, `sleb`, `debug_info(elf)` (relocations applied), `abbrevs`, `read_form`,
  `compile_unit(elf, on_die)`, `loclist`, `decode_expr`, `local_slots(obj, function) -> [LocalSlot]`; `DwarfError`.
* `pe.Pe(path)` (WP5): the one PE32 reader of the Metrowerks host tools (`mwldeppc.exe`, `mwcceppc.exe`): `machine`,
  `image_base`, `entry`, `size_image`, `dll_characteristics`, `sections` (`Section`: name, va, vsize, raw_off, raw_size,
  flags, 1-based index), `section(name)`, `rva2off`, `read(rva, n)` (None when not file-backed), `read_rva(rva, n)`
  (ValueError there), `cstring`, `data_dir(i)`, `debug_entries()`, `debug_blob()`, `codeview_symbols()` (the `NB11`
  blob: `[(rva, name, section)]`, cached), `symbol_map()`, `imports()`, `iat_slots()` (`{name: slot VA}`),
  `resources()`, `string_blocks()` (RT_STRING). Refuses (ValueError) no `MZ`, no `PE\0\0`, a non-PE32 header.
* `build.ElfBuilder` (`section`, `nobits`, `symbol`, `file_symbol`, `reloc` (a section name - its first section - or
  a 1-based section index, for a repeated name), `comment(version)`, `build`, `write`;
  locals first unless `keep_order`), `build.PeBuilder` (`section(name, data, vsize, va)`, `imports`, `strings`,
  `codeview`, `rva`, `build`, `write`) and `build.DolBuilder` (`text`, `data`, `bss`, `pad`, `entry`, `build`, `write`).
  `lib.testing.FixtureTree.add_object` accepts a builder.

## Absorbs (today's implementations)

26 ELF readers, 10 DOL readers, 13 ELF and 7 DOL fixture builders, `elfsect.sections`, `objalign/objextab.read_sections`,
`dwarfmap`, the objdump parsers in `accessextent`, `m2cinput`, `callees`, `verify_pcode`. Collapsed in WP1b: see
"Known gaps" for the ones left.

## Invariants and rules

* The reader is lenient past the header table (a string offset outside its table reads as "", a short section is
  truncated) and refuses only what it cannot walk. A delegate that must refuse more (ELF32 big-endian only) calls
  `require_be32()` or checks `ei_class`/`ei_data`, and keeps its own refusal wording.
* `.rela<sec>` pairs with `<sec>` through `sh_info`; the name is the fallback for a zero `sh_info`.
* A repeated section name is legal (dtk writes two `.data` pieces into some target objects): `section(name)` is the
  first; a relocation carries its section's index. The name-keyed delegates keep their old last-wins reading.
* Value types are frozen dataclasses; the reader builds them through a positional maker that skips the frozen
  `__setattr__` path (the same values, half the cost).
* `elf.py` is an input of every MWCC compile (the `objalign`/`objextab` steps): `tools/project.py` lists it as an
  implicit dependency, so an edit to it rebuilds every object - keep it stable and cheap to import (no `typing`).

## Test contract

Tier: fixture (`tools/tests/lib/test_binary.py`, a lib test never reads the live tree). builder -> reader round trip
for every field; `.rela<sec>` pairs with `<sec>`; locals are below `sh_info`; a repeated section name; the editor is
byte-neutral outside `.symtab`/`.strtab`; DOL reads at and across segment ends; DWARF LEB128, expressions and a
synthetic compile unit with a relocated loclist; the tokenizer on each line shape. The real split objects are parsed
by the old-vs-new equivalence run recorded in the WP1b batch, not by a pinned smoke count.
`pe` (WP5): `tools/tests/lib/test_binary_pe.py` on `PeBuilder` images - sections and the file-backed boundary, the
import directory and its IAT slots (each slot read back to its hint/name), RT_STRING resources, the CodeView blob, the
machine/image base read (not assumed), and the three refusals. The real linkers and compilers were compared old-vs-new
in the WP5 batch (`mwlink_debugger.py info/messages/anchors/phases/records/align`, `versions.detect` over the 30
`build/compilers/*/*/mwcceppc.exe`, `dissect.py`, `pass_points.py`: identical output).

## Known gaps

* WP5 delegated `mwlink_debugger.Elf`/`MwObject`/`_object_header_bits` here (as dict-shaped adapters, so the trace's
  output is unchanged) and the three PE readers (`mwlink_debugger.Pe`, `mwcc-debugger/versions.Pe`,
  `locate/dissect.Pe`) to `pe`.
* Not delegated in WP1b (left for their family lane): `promote.Elf` (retired
  in WP6), `callers.build_elf_index` and `callers.parse_dump_file`'s combined scan regex (WP3c - the scan is one regex
  over the whole dump for speed), `flipcheck.sections`/`raw_section` (binutils `objdump -h`/`objcopy -O binary`
  semantics: a separate equivalence run), `relocdiff`/`sectiongap` type-name fallbacks (`R_PPC_%d` vs `type-%d`).
* The 13 ELF and 7 DOL fixture builders in the old selftests move to `ElfBuilder`/`DolBuilder` as each tool's tests
  are re-homed (WP3); only `objextab`'s in-file builder moved here.
* `locate(root)` takes a path until `lib.repo.Tree` exists (WP1a).
