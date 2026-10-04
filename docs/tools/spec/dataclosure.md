# `dataclosure` - The data closure: the census of what a unit's TARGET object references that no claim covers, the strict/span/fold verdicts and the gate's snapshot rows

## Purpose

Asks, per registered unit, which data its target object references and which claim covers each address (`own`, `other`,
`orphan`), judges the sole-owned orphans of a touched unit (refuse, or deferred with a named class), plans the exact
`splits.txt` edit to a fixpoint, and keeps the base snapshot the land gate's data-closure row is an add-only difference
against (orphan keys, claimed bytes, per-unit claims, object fingerprints), re-keyed across folds and renames.

## Users

`datagap.py` is its CLI (`--census`, `--row`, `--touched-by`, `--write-snapshot`, `--base-snapshot`) and re-exports its public
names for the importers that still spell `datagap.<name>`: `land` (`snapshot_orphans`, `batch_orphans`, `orphan_verdict`,
`orphan_key`, `git_unit_renames`, `render_touch`, `STRICT_CLASSES`), `backlog` (`census`, `load_claims`, `strict_report`,
`tree_claim_exposed`), `dataclaim` (`unit_plan`, `fixpoint_plan`, `render_plan`, `render_fixpoint`, `render_strict`,
`tree_freshness`, `render_freshness`, `build_fixture_object`), `stylelint` (`derive_absorption`, `splits_at_ref`,
`unit_claim_table`, `load_claims`), `poolseams` (`census`, `load_claims`, `load_data_symbols`, `GAME_DIR`).

## CLI

None of its own: `python tools/units/datagap.py --census [--unit U]`, `--row UNITS [--base-root R] [--base-ref REV]
[--allow-orphan ADDR] [--unit-rename OLD=NEW] [--touched-by]`, `--write-snapshot F`, `--base-snapshot F` (`spec/datagap.md`).

## Inputs and outputs

Inputs: `config/RMHE08/{splits,symbols}.txt`, `build/RMHE08/obj/**.o` (the census), `build/RMHE08/src/**.o` (pool sizes, the
touch fingerprints), `callers.py`'s reader index (`readers_index`), git (`splits_at_ref`, `git_unit_renames`). Outputs: records,
verdicts and snapshots as dicts; it writes nothing.

## Invariants and rules

* A claim is a `splits.txt` range: an address is covered exactly when some registered block's range contains it.
* **STRICT** (owner, 2026-09-29): a unit the batch really changes must claim the data only it references, pre-existing pairs
  too; a deferred pair names its class: `pool-synth`, `ambiguous-owner`, `span-blocked`, `isolated-run` (`STRICT_CLASSES`).
* **TOUCHED** (owner, 2026-09-29: "only real changes"): registered, recut, or a compiled object whose
  `lib.objcompare.touch_fingerprint` differs from the base's; a rename sweep and an edit that compiles to the same object touch
  nothing; no base record means touched.
* **CLAIM-EXPOSED** (owner, 2026-09-30): a new pair that only the batch's own newly claimed data references is deferred,
  never refused, and earns no allowance.
* **Folds** (2026-09-30): the base snapshot is keyed by unit name, so it is re-keyed onto today's names from evidence
  (`derive_absorption`: base bytes a different unit claims now, plus git's rename detection); an explicit `OLD=NEW` overrides.
* The census reads target objects; `tree_freshness` decides by content (a current object's sections are exactly its claims'
  sizes) whether the census is from current objects.

## Lib dependencies

`lib.objcompare` (`section_sizes`, `reloc_facts`, `touch_fingerprint`, `fingerprints_equal`), `lib.refs` (the census),
`lib.project` (splits, map); tool APIs: `poolseams` (the pool-sharing groups), `callers` (the reader index, lazily).

## Test contract

Tier: fixture. Its six check groups (`selftest_census`, `_strict`, `_touch`, `_exposed`, `_fold`, `_span`, 160+ checks on
temp trees of fixture objects) run from `datagap.py --selftest` through `dataclosure.selftest(eq)`.

## Known gaps

* The gate's rows are still dicts of lines, not `lib.findings.Row`s (WP4 rebuilds the rows).
* `readers_index` goes through `callers.py` until `lib.refs.RefIndex` arrives (WP3c).
* The checks live in the module; re-homing them to `tools/tests/units/test_dataclosure.py` needs their bare global references
  qualified, which is a rewrite of 700 lines of fixtures, not a move.
