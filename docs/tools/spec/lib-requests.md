# `lib/requests` - integrator requests: schema, legacy loader, classification, owner resolution, status

## Purpose

The one definition of an integrator request (a lane's ask for a change in a unit it does not own), the loader that
also reads the pilot's free-text lines, the static classification, the owner resolution against the map, the status
sidecar and the STOPGAP marker.

## Users

`tools/units/integrate.py` (all of it), `tools/units/stylelint.py` (open ids, STOPGAP blocks), `tools/units/brief.py`
(renders `SCHEMA`), `tools/lib/outbox.py` + `tools/units/handoff.py --check-requests` (validation).

## Public API

* `SCHEMA`, `KINDS` (`rename`, `decl`, `decl-move`, `field`, `seam`, `unit-rename`, `config`, `info`), `CONFIDENCES`
  (`certain`, `evidence`, `guess`), `CLASSES` (`mechanical`, `semi`, `judgement`), `STATUSES`, `ID_RE` (`<slug>#<n>`).
* `validate(entry) -> [problem]` - a new-schema object: id, kind, evidence, `symbol|address`, `proposed_name` for a
  rename and for a `decl` of a generated name, a one-line `prototype` that declares the name, `stopgap {file, id}`.
* `Request` / `Target`; `load_file(path)` (JSON lines or a list; a new-schema line keeps its id, a free-text line
  becomes `<slug>#<line>` through `normalise_legacy`, an unparsable line an `info` request - nothing is dropped);
  `request_files(dir)`, `slug_of(path)`.
* `normalise_legacy(raw, slug, n)`: kind mapping (`move` -> `decl-move` / `seam` / `info`, `unit stem` -> `unit-rename`),
  targets split on `,` and ` / `, addresses from `fn_`/`lbl_` names, `(.text:0x...)` and a leading `0x...`, a mangled
  alias in parentheses, names from `proposed` (`first_name`: prose is never a name), a strict `prototype_in` (at most
  four type tokens, none prose), `header_in`, GUESS -> `guess`.
* `classify(req, decisions) -> (class, reason)` - static (never reads the tree). Seams, unit renames, info and field
  changes are judgement; a `decl` asking for a class/linkage/type change is judgement, one asking for a return or
  parameter tweak is semi; a rename to a member/ctor/dtor mangling is judgement; everything else is mechanical (a GUESS
  needs a decision at apply time).
* `load_decisions(path)` (`old new` lines; `old` is a spelling or `0xADDR`), `decision_for(target, decisions)`.
* `resolve(req, ownership, decisions) -> [Resolved]`, `resolve_target`: by name, by a C++ function's one mangled row
  (`ck_option_cfg` -> `ck_option_cfg__FUc`), else by address (a lane's `fn_X` the map has renamed is `stale`); the owner
  is `Ownership.owner_of` (registered unit, else the band); `owner_header(unit|band)`; the request's `owner_unit` is
  cross-checked (the map wins, noted). `source_name`, `is_member`, `mangled`, `live_owner(resolved, units)`.
* `status_path`, `load_status`, `write_status(path, {id: {status, note}})` (the sidecar
  `<slug>-requests.status.json`; the request file is never edited), `open_ids(dirs)`.
* `stopgap_blocks(text)`, `unpaired_stopgaps(text)`, `STOPGAP_*_RE`.

## Invariants and rules

* One schema: `brief.py` renders `SCHEMA`, `handoff.py`/`outbox.check_requests` validate with `validate`, integrate reads
  `load_file` - no second copy of a field list.
* Classification is static and conservative: a request becomes mechanical only when nothing beyond a name decision is
  needed; whether it really applies is integrate's business (it reads the tree and builds).
* A free-text line is never refused, only interpreted; `handoff.py --check-requests` prints what it read.

## Lib dependencies

`lib.names`, `lib.text`; `lib.project.Ownership` is passed in (no import cycle).

## Test contract

Tier fixture: `tools/tests/lib/test_requests.py` - verbatim pilot lines through the loader, the classes of a fixture
set, resolution on a fixture map (renamed-by-address, mangled stem, member, band, cross-check, live owner), the sidecar
and `open_ids`, the STOPGAP pairing.

## Known gaps

The legacy loader reads the four pilot files' phrasings; a new phrasing may classify as judgement until the lane files
the new schema. Classification counts on the four files: 38 mechanical, 3 semi, 35 judgement (the analysis read 35/3/38).
