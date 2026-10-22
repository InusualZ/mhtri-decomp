# `names` - the name registry: first-proposal-wins names for symbols another unit owns

## Purpose

Records the names lanes give foreign callees (symbols another unit owns) before those names reach the map, so two
lanes never name one address differently: the first recorded proposal wins and later lanes reuse it. The orchestrator
turns the pending winners into a map-only rename batch (`docs/pipeline.md` 16.2).

## Users

Lanes (before naming a foreign symbol: `lookup`, then `propose`); the orchestrator (`list --pending`, `export` into
`symedit.py rename-batch`, landed as a small `config/symbols` batch); the gate admits the registry file as batch
material (`landing.common.ALLOWED_FILES`, subject category `config/symbols`).

## CLI

```
python tools/symbols/names.py lookup ADDR|NAME [ADDR|NAME ...]
python tools/symbols/names.py propose ADDR NAME --lane L --evidence TEXT [--dry-run]
python tools/symbols/names.py list [--lane L] [--pending]
python tools/symbols/names.py export [-o FILE]
common: [--ref REF (main)] [--registry FILE] [--root DIR] [--json]
```

* `lookup`: the map rows at the address / of that name in main's map and the tree's (owner unit from the claims,
  `[generated]` marked) and every registry entry for it with its status. Exit 0.
* `propose`: `PROPOSED ...` (one line appended), `ALREADY ...` (this name is the winner or the map's name: nothing
  written), exit 0; `REFUSED ...` naming the winner or the reason, exit 1; a malformed address exits 2.
* `list`: every entry with its status (`winner`, `superseded`, `duplicate`, `name-taken`); `--pending` only the winners
  main's map does not carry yet.
* `export`: `old new` lines (stdout or `-o`) for every pending winner whose row in main's map still has a generated
  name; a winner whose row main named meanwhile, or whose name main uses elsewhere, is a `note:` on stderr.
* `--json`: `lookup` -> `[{query, map: [{name, section, address, size, generated, in, owner}], registry: [...]}]`;
  `propose` -> `{verdict, message, entry, dry_run}`; `list` -> entries; `export` -> `{pairs, notes}`.

## Inputs and outputs

Reads `<ref>:config/RMHE08/symbols.txt`, `splits.txt` and `name_registry.jsonl` (the landed state, through the shared
`main` ref, so a lane in a slot sees what landed after its cut), the tree's `symbols.txt` (a lane's own-row renames)
and the tree's registry file (its own unlanded proposals). Writes only `propose`'s one appended line to the tree's
registry file, and `export -o FILE`.

A registry line: `{"address": "0x8011D448", "section": ".text", "name": "...", "old": "<the map's name then>",
"lane": "...", "evidence": "...", "at": "<ISO time>"}`.

### Why this path

`config/RMHE08/name_registry.jsonl`, **tracked**, beside the map it feeds:

* every slot reads the landed registry through the `main` ref without a write into MAIN (lanes may not write MAIN's
  `.pi/`), and a lane's own proposals travel on its branch with the code that uses them;
* append-only JSON lines plus `.gitattributes` `merge=union` (`ATTRIBUTE_RULE`) make two lanes' appends a clean
  merge in `git merge` and in `land.py`'s `git apply -3` (measured in the test), so the registry never blocks a landing;
* a clash that the union lets through (two lanes proposing one address before either landed) is decided by the
  reader, `resolve`: first by `at`, then by read order - deterministic on every tree.

## Invariants and rules

* A proposal is refused for: a name the map cannot carry (`symbols.VALID_NAME_RE`), a generated or address-named name
  (section 6.5 rule 7: `lib.names` `map` scheme, `RULE7_STEM_RE`, `ADDRESS_RUN_RE`), no `--lane`/`--evidence`, an
  address with no map row, a row main's or the tree's map already named (the reply names it: use it), a name the map
  uses at another address, an address the registry already gave another name (first proposal wins: the reply names
  the winner), a name the registry already gave another address.
* The file is append-only: `propose` never rewrites a line; a stale or losing entry stays and is reported by status.
* `export` never emits a rename of a non-generated map name and never a name main uses elsewhere.
* Nothing outside the tree is read except through git refs; nothing is written except the registry line and `-o`.

## Lib dependencies

`lib.names` (generated-name predicates), `lib.project.symbols` (`parse_line`, `VALID_NAME_RE`), `lib.project.splits`
(the owner), `lib.git` (`show` at the ref), `lib.repo` (the invocation tree).

## Test contract

Tier fixture: `tools/tests/symbols/test_names.py` - the propose rules on a fixture repository (a first proposal, the
idempotent repeat, first-proposal-wins, a name taken in the registry or the map, a row main named, rule-7 names, no
row, `--dry-run`), two lanes in their own worktrees reading main's registry through the ref and their appends merged by
`git apply -3` under `ATTRIBUTE_RULE` with the earlier proposal winning, `resolve`'s four statuses, `export`/`list
--pending`/`lookup` after main named one row itself (19 checks). Tier smoke: `tools/tests/smoke/test_names_live.py` -
the live `.gitattributes` carries `ATTRIBUTE_RULE`, every live winner names a map row.

## Known gaps

* Two lanes that propose one address before either lands both see their own proposal as the winner until the union
  lands; the loser learns it from `lookup`/`list` after main moves (`superseded`), and its source spelling must follow.
* `export` is the rename half only: the reference sweep is `symedit.py rename-batch`'s, and declarations still move
  through the integrator.
