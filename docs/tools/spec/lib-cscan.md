# `lib/cscan` - C/C++ lexical scanning for the source tools

## Purpose

C/C++ lexical scanning for the source tools.

## Users

The tools listed under this concept in `docs/tools/README.md`.

## Public API

* `strip_comments(text)` (length-preserving), `match_brace`, `match_paren`, `statements(body)`, `declarations(text)` (prototypes and `extern`, with declarator parsing), `type_definitions(text)` (struct/class/union/enum, fields, offsets, size comment), `includes(text)`, `include_closure(path, roots)`, `pragmas(text)`, `calls(text, name)`

## Absorbs (today's implementations)

`stylelint.strip/match_brace/struct_defs/iter_fields/function_declarations/_resolve_include`, `typeregistry.strip_comments/extract_decls`, `vtableaudit.type_definitions/_members`, `declclash.closure/shape`, `recordmerge.parse`, `methodize._call_sites`, `shapes.strip_comments/match_brace/split_statements`, `freshguard.source_closure`

## Test contract

Tier: fixture (a lib test never reads the live tree). the lexical rows of the stylelint, typeregistry, shapes and declclash selftests

## Known gaps

None until implemented; `migration.md` names the package.
