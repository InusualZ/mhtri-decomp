# `lib/objcompare` - Target-object vs our-object comparisons, each returning findings

## Purpose

Target-object vs our-object comparisons, each returning findings.

## Users

The tools listed under this concept in `docs/tools/README.md`.

## Public API

* `sections(target, ours, all_sections=False) -> [SectionGap]` (sizes, first byte, count, permutation class, metadata excluded)
* `symbols(target, ours, threshold) -> size-gap / missing / extra`; `relocs(target, ours, by_owner) -> four classes`; `undefined(ours, target, providers, map, linker) -> hits with spelling hint`; `fingerprint(obj)`

## Absorbs (today's implementations)

`datagap.compare_sections`, `sectiongap.compare_objects`, `flipcheck.section_byte_problems/mislaid_*/undefined_reference_problems`, `pairgap.compare`, `relocdiff.diff_relocs/compare_by_owner`, `undefrefs.unresolved_names`, `relocaudit.audit_sets`, `datagap.object_fingerprint`, `verifyunit.target_object_fingerprint`

## Test contract

Tier: fixture (a lib test never reads the live tree). the ELF fixtures of the six tools' selftests, re-expressed on `ElfBuilder`

## Known gaps

None until implemented; `migration.md` names the package.
