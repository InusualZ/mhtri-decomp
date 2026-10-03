# `lib/outbox` - The outbox and notes: the schema, tolerant loading, validation

## Purpose

The outbox and notes: the schema, tolerant loading, validation.

## Users

The tools listed under this concept in `docs/tools/README.md`.

## Public API

* `CONFIG_REQUEST_SCHEMA`, `FREE_TEXT_FIELDS`; `load_outboxes(dir)`, `load_notes(dir)`; `Entry.text(field)`, `requests()`, `units()`, `symbols()`; `validate(entry, owned) -> [Finding]`

## Absorbs (today's implementations)

`handoff.CONFIG_REQUEST_SCHEMA/validate/units_declared`, `backlog.build_items/_add_request/asstr`, `tooling.load_sources/_flatten`, `playbook.extract_findings`, `slots._merge_data_requests`, `land.outbox_units`, `splitcheck.seam_requests`

## Test contract

Tier: fixture (a lib test never reads the live tree). the handoff and backlog fixture outboxes

## Known gaps

None until implemented; `migration.md` names the package.
