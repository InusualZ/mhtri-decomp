# `lib/objcompare` - Target-object vs our-object comparisons: sections, bytes, layout, symbols, relocations, undefined names, fingerprints

## Purpose

Compares a unit's split target object with our compiled one - section sizes, section bytes and relocation lists, the
permutation (layout) classes, symbol sizes, relocations by offset and by owning symbol - answers which relocated names a
flip would leave undefined (with the link-input index behind that question), audits linkage spellings, and fingerprints an
object for drift and for "touched". Every function takes a path, the bytes of an object or a `lib.binary.elf.Elf`.

## Users

`datagap` (`section_sizes`, `size_gaps`, `FLIP_DATA_SECTIONS`), `dataclosure` (`section_sizes`, `reloc_facts`,
`touch_fingerprint`, `fingerprints_equal`), `sectiongap` (`object_sections`, `sections`, `reloc_reasons`), `pairgap`
(`defined_symbols`, `symbols`, `section_kind`), `relocdiff` (`reloc_rows`, `reloc_classes`, `owner_groups`, `by_owner`),
`flipcheck` (`object_sizes`, `section_data`, `section_symbols`, the layout classes, `reloc_facts`, `link_inputs`, `link_index`,
`undefined`), `undefrefs` (`reloc_facts`, `link_index`, `undefined`, `linkage_sets`, `linkage_audit`), `verifyunit`
(`fingerprint`, `symbol_locations`, `symbol_rows`). `dataclaim` reaches `reloc_facts` through `undefrefs.load_object`.

## Public API

* Reading: `load(obj) -> Elf`, `be32(obj) -> Elf | None` (ELF32 big-endian with a section table, else None).
* Bytes: `first_difference(a, b) -> int | None`, `differing_bytes(a, b)`.
* Section sizes: `section_sizes(obj) -> {name: size}` (empty omitted), `size_gaps(target, ours, all_sections) ->
  (ours_extra, target_extra)`, `object_sizes(obj) -> {name: (size, align exponent)}` (`objdump -h`'s view), `section_names`,
  `section_data(obj, name) -> bytes | None`.
* Section content: `object_sections(obj, all_sections) -> {sections, order, relocs}`, `sections(target, ours, all_sections) ->
  [SectionGap(section, ours, target, reasons)]` (`.why`, `.to_dict()`, `.finding(unit)`), `section_reasons`, `reloc_reasons`.
* Layout: `section_symbols(obj, section) -> {name: (offset, size)}`, `mislaid_layout(mine, tgt, ours, theirs)`,
  `mislaid_order(...)`, `section_byte_reasons(name, mine, tgt, ours, theirs) -> [line]` (flipcheck's wording);
  `trailing_pad(section, ours_size, claim_size, align, target_bytes, target_symbols, start, next_aligns) -> note | None`
  (a short section whose tail is only alignment fill: never `PAD_NEVER`, align <= `PAD_MAX_ALIGN`, zero tail, no
  symbol but `gap_` labels, the next section placed at the claim's end; flipcheck's rule, see its spec).
* Symbols: `section_kind`, `wanted_kinds`, `defined_symbols(obj, kinds) -> {name: SymbolSize}`, `size_delta`,
  `symbols(target, ours, threshold, mode) -> [SymbolGap]`, `symbol_locations(obj)`, `symbol_rows(target, ours)` (a dtk-named row
  resolved by address).
* Relocations: `reloc_rows(obj) -> ({section: [(offset, symbol, type, addend)]}, None) | (None, why)`, `reloc_classes(target,
  ours)` (the four classes), `relocs(target, ours, sections)`, `owner_groups(obj)`, `by_owner(target, ours) -> (matched, total,
  lines)`, `legacy_reloc_name(type, fallback)`; `callee_diffs(target, ours) -> [{function, diffs: [{kind, ours, target,
  offset}]}]` (per `.text` function both define and ours wrote - more than `STUB_MAX_BYTES` - the relocation symbol names
  that differ: sequences aligned, moved rows cancelled by name, our `LOCAL_LABEL_RE` labels cancelling the target's
  `TARGET_LABEL_RE` pool/jump-table labels of the same type, the rest paired in order), `callee_kind(ours, target)`
  (`callee`/`mangling`/`linkage`/`extra`/`missing`, by `lib.names.owner_stem`).
* Undefined names: `reloc_facts(obj) -> {relocs, defined, refs} | None`, `provides_global(entry)`, `link_inputs(ninja)`,
  `linker_assigned(ldscript)`, `link_index(root, inputs, cache_path, rebuild) -> {providers, ref_count, inputs, refs}`,
  `external_candidates(ours, known)`, `spelling_hint(section, offset, name, target)`, `undefined(ours, target, *, map_set,
  providers, ref_count, target_rel, linker_set) -> [(name, hint)]`.
* Linkage: `linkage_sets(obj) -> (global defined, undefined) | None`, `linkage_audit(our_def, our_undef, tgt_def, tgt_undef)`.
* Fingerprints: `fingerprint(path) -> str` (target drift), `touch_fingerprint(path, symbols) -> {body, ext} | None`,
  `fingerprints_equal(a, b)` (TOUCHED), `file_sha256(path)` (the raw half the gate pairs with `fingerprint`:
  `verifyunit.target_object_hashes` / `names_only_changes`, WP4).
* Constants: `META_SECTIONS`, `DATA_SECTIONS`, `FLIP_DATA_SECTIONS`, `BOOKKEEPING_SECTIONS`, `EABI_LINKER_SYMBOLS`,
  `ENTRY_SYMBOLS`, `LINKER_SYMBOLS`, `LINK_INDEX_REL`, `RENAME_FREE_SECTIONS`, `UNSTABLE_SECTIONS`, `OBJDIFF_SIZE_GAP`.

## Invariants and rules

* **A relocation applies to the section `Elf.rela_sections` names** (`sh_info`, else the name with `.rela` dropped), and every
  relocation section is read: dtk writes two `.data` (and two `.rela.data`) into one object, and the linker applies both.
* **A repeated section name reads its last section** for sizes and bytes (`object_sizes`, `section_data`): what `objdump -h`
  sized and what `objcopy --only-section` left on top, measured on `Network/NetworkCommunityPat`'s target object. NOBITS reads
  as empty bytes, absent as None.
* **Bookkeeping relocations (`extab`, `extabindex`) are not references**: an extabindex entry covers a function and cannot
  call or root it.
* **`undefined`'s exemptions**: defined by our object (locals too), named by the map or the linker (`LINKER_SYMBOLS` plus the
  script's assignments), provided by a link input other than the target (a flip replaces it), referenced but not defined by the
  target (already unresolved), or provided by nobody while already referenced (already broken that way). The hint is the one
  other name at the same relocation slot, else the one target name with the same linkage stem.
* **`link_index`** caches per-input facts under `build/tmp/undefrefs/link-symbols.json` keyed by `[mtime_ns, size]`; only
  changed inputs are re-read and the file is written only when an input appeared, vanished or moved.
* **Two fingerprints, two questions**: `fingerprint` is rename-insensitive (string tables skipped, symbol names dropped) so a
  rename sweep is not drift; `touch_fingerprint` hashes everything but external targets and resolves those by address through
  the map, so a rename sweep is not a touch. Both hash a section's *contents* (`Section.data`), so a `SHT_NOBITS` section
  contributes only its name and size: dtk writes a `.bss`/`.sbss` header offset of 0, and its file slice is the ELF header,
  whose section-table offset moves with `.strtab`'s length - hashing that slice made a rename read as a re-range (2026-10-04,
  6 units in the L4/L2 landings). `fingerprint` values of an object with a NOBITS section changed with that fix; nothing
  compares across versions, because `.pi/land-base.json` is recorded fresh by every landing's `record-base`.
* The `symbols` size-gap threshold is strict (`>`), and gates only the size-gap class; an absent symbol is always 100 % apart.

## Absorbs (today's implementations)

`datagap.section_sizes/compare_sections/object_fingerprint/fingerprints_equal/object_reloc_sources`' reader,
`sectiongap.read_object/compare_objects/section_reasons/reloc_reasons/_fmt_offsets/_first_byte_diff/_differing_bytes`,
`flipcheck.sections/raw_section/scratch_file/differing_bytes/section_symbols/mislaid_layout/mislaid_order/
section_byte_problems/elf_sections/object_symbols/link_inputs/linker_assigned/link_reference_context` and the rule of
`undefined_reference_problems`, `pairgap.read_symbols/section_kind/wanted_kinds/size_delta/compare`,
`relocdiff.read_relocs/diff_relocs/owner_groups/compare_by_owner`, `undefrefs.link_inputs/load_object/linker_symbols/
link_symbol_index/external_candidates/spelling_hint/unresolved_names`, `relocaudit.read_symbols/object_sets/audit_sets`,
`verifyunit.target_object_fingerprint/sha256_file/symbol_locations/_same_place/raw_symbol_rows`. `flipcheck` no longer runs
`objdump`/`objcopy`; none of the tools imports `dossier.parse_elf` or `unitutil.read_elf` for these reads.

## Lib dependencies

`lib.binary.elf`, `lib.cache` (`stat_key`), `lib.names` (`linkage_stem`), `lib.findings` (`SectionGap.finding`, lazily).

## Test contract

Tier: fixture (`tools/tests/lib/test_objcompare.py`, 79 checks, every object built with `ElfBuilder`): F41 (moved relocations,
both offset sets) and F39 (a short record), the objdump/objcopy views (repeated names, NOBITS), both permutation classes, the
three symbol classes and the strict threshold, the address-resolved `pad_` row, the four relocation classes and the owner
view, both same-named relocation sections read, every `undefined` exemption and both hints, the link index and its cache, the
linkage classifier in both directions, and both fingerprints (a dtk-shaped `.bss` at header offset 0 plus a longer
`.strtab` is not drift, a `.data` byte is). Mutation-checked: seven seeded defects (last-rela-only, no
relocation reasons, target not excluded, string tables hashed, first-of-repeated, inclusive threshold, target definitions
ignored) each fail at least one check.

## Known gaps

* The comparisons return value types (`SectionGap`, `SymbolGap`) and the established dict shapes (`reloc_facts`,
  `object_sections`, `reloc_classes`), not `Finding`s, except `SectionGap.finding`: the tools' renderers read those shapes and
  the gate (WP4) is where rows become `Row`s.
* `legacy_reloc_name` keeps the six relocation names `relocdiff` and the owner view always printed; the full `binary.elf`
  table would rename `R_PPC_11` to `R_PPC_REL14` in 6 lines of `relocdiff --all` on this tree - a deliberate later change.
* `flipcheck` keeps its own spelling note (a target reference that starts with the name); `spelling_hint` is the gate's.
  Unifying them changes flipcheck's refusal text, which lanes parse.
