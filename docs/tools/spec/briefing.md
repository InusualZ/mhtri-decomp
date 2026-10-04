# `briefing` - The brief, split by concern: what it is made of, how it reads, the pre-written pool

<!-- WP3e (2026-10-04): the split of brief.py (design.md section 5); brief.py stays the CLI and re-exports every name -->

## Purpose

`tools/units/briefing/` is `brief.py`'s implementation, one module per concern:

* `sources.py` - everything a brief is made of: registration (`registered_objects`, `pool_units`), the claim that
  holds a unit (`claim_for`, `handoff_paths`), ranges and symbols (`splits_range`, `symbols_in_range`), scores,
  the header comment, the flags (`lib.units.split_flags` of MAIN's ninja command), shared headers, rule sections
  (`plan_section`), and `build` (the dict the six parts render). **It is the only briefing module that reads
  another tool** (`langcheck`, `typeregistry`, `dossier`, `handoff`, each imported where it is used).
* `render.py` - the text: the six parts (`render`), the shared steps (C++ class rule, data step, pre-commit check,
  asm hint, measurement loop), the stamp (`unit_stamp`, `brief_stamp`), the outbox schema table, the integrator
  contract, `brief_for`, and `cluster_index` (the one file a cluster lane gets). The "your tree" block and the
  release ban are `lib.lanes.launch.your_tree_lines`/`teardown_lines` - one copy, shared with `slots.py spawn`.
* `pool.py` - `briefs/pool/` (`pool`, idempotent by stamp, pruned to the pool) and the promoted litter check.

## Users

`tools/units/brief.py` (the CLI; `import tools.units.briefing as briefing`).

## Invariants and rules

* `render` never imports a tool; it reaches `langcheck`/`dossier`/`handoff` through `sources`
  (`language_cell`, `language_paragraph`, `render_dossier`, `config_schema_rows`), so the layering edges sit on one
  module.
* `briefing/__init__.py` imports `sources`, `render`, `pool` in that order (render and pool read sources).

## Test contract

Through `brief.py --selftest` (fixture trees; `plan_section`/`splits_range` also read the live tree).

## Known gaps

None known.
