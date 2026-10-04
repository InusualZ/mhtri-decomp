# `lib/outbox` - The outbox and notes: the schema, tolerant loading, validation

## Purpose

The data a lane hands back (`.pi/outbox/<slug>.json`, `.pi/notes/<slug>.md`): the one `config_requests` schema,
the free-text fields every intake reads, the validator the gate refuses on, and tolerant loaders.

## Users

`units/handoff.py` (`--check`, the template), `units/land.py` (through `handoff.validate`), `units/brief.py`
(renders the schema, through `handoff`), `units/backlog.py` (`FREE_TEXT_FIELDS`, `asstr`), `units/playbook.py`
(`load_outboxes`).

## Public API

* `REQUIRED`, `CONFIG_REQUEST_SCHEMA`, `CONFIG_KINDS`, `CONFIG_NEEDS`, `FLAG_PROBE_FIELDS`, `FLAG_PROBE_VERDICTS`,
  `FREE_TEXT_FIELDS`; `config_schema_rows()`.
* `asstr(x)`, `request_content(req)`, `symbol_like(name)`, `units_declared(entry)`.
* `validate(entry, owned, file="") -> [Finding]` (rule `ERROR` or `WARNING`, `token` the field at fault);
  `errors_and_warnings(findings) -> (errors, warnings)`.
* `Entry(path, data, error)`: `.stem`, `text(field)`, `requests()`, `units()`, `symbols()`;
  `load_outbox(path)`, `load_outboxes(dir, warn=None, keep_bad=False)`, `load_notes(dir) -> {stem: text}`.
* `requests_path(dir, slug)`, `check_requests(path) -> (errors, notes)`: the lane's integrator requests beside its
  outbox (`lib.requests` owns the schema).

## Invariants and rules

* **One schema, and the kind carries the fields**: `range` means section/start/end, `rename` old/new/evidence. The
  brief renders this table verbatim, so it asks for exactly what the validator accepts (the 2026-09-23 18-error
  outbox came from a brief that named no schema).
* **A real filing is never refused for its spelling**: a known kind missing its structured fields, or a kind
  outside the table, is accepted when it carries content under one of `FREE_TEXT_FIELDS`
  (`evidence, why, request, what, subject, note`), and refused only when empty. Every intake reads the same list.
* `flags_probed` takes `{flags, effect, verdict}`, a prose string, or a lane's own keys (`flag`/`result`); only a
  *bad verdict* (not `reject/adopt/inconclusive/kept` as a prefix) or an empty probe is an error, a missing field
  is a warning.
* **Ownership reads every declared unit** (`unit`, `units`, `also_changed_units`, `changed_units`, `per_unit`), never
  a split of a prose `unit`; a summary row in `symbols` (a space or a `/`) is not ownership-checked.
* Loading is tolerant: an unreadable or non-object file is skipped (named on `warn` as `warn: skipping <name>
  (<why>)`) or kept with its `error`.

## Absorbs (today's implementations)

`handoff.REQUIRED/CONFIG_REQUEST_SCHEMA/CONFIG_KINDS/CONFIG_NEEDS/FLAG_PROBE_*/FREE_TEXT_FIELDS/config_schema_rows/
request_content/symbol_like/units_declared/validate`, `backlog.asstr`, `playbook.load_outboxes`.

## Lib dependencies

`lib.findings` (`Finding`).

## Test contract

Tier: fixture (`tools/tests/lib/test_outbox.py`). The handoff fixture rows (a good entry, ranges, unowned and
summary symbols, missing fields, every schema kind with and without free text, out-of-schema kinds, probe shapes),
`units_declared`, and loading a temporary outbox and notes directory with a bad and a non-object file.

## Measured (WP2c)

`handoff.py --check --json` on 51 MAIN outboxes (every seventh of 352): identical output and exit codes before and
after. `playbook.load_outboxes` on all of MAIN's outboxes: 351 entries, identical data and warnings. One behaviour
change: playbook read an outbox as strict UTF-8 (an invalid byte raised past the `json` guard); it now reads with
replacement like the other loaders - no outbox on this tree is affected.

## Known gaps

* Still reading outboxes their own way (their packages): `backlog.build_items/_add_request`, `tooling.load_sources/
  _flatten/candidates_for`, `playbook.extract_findings/extract_note_findings`, `slots._merge_data_requests`,
  `land.outbox_units`, `splitcheck.seam_requests`.
* `brief.py` and `land.py` still reach the schema through `handoff` (the brief split is 3e, the gate 4).
