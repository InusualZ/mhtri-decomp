# `playbook` - Turn outbox findings into playbook idea drafts under `.pi/playbook-drafts/` (run by hand; the gate's knowledge-delta row that called it was deleted 2026-10-05)

<!-- generated from the module docstring of `tools/units/playbook.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

Turn worker outboxes into ready-to-land playbook drafts (docs/matching/NNN-slug.md idea stubs).

## Users

the orchestrator, by hand (the landing gate's row 20 called it until 2026-10-05)

## CLI

```
python tools/units/playbook.py                 # scan .pi/outbox, write .pi/playbook-drafts/
python tools/units/playbook.py --print         # the same report, without writing
python tools/units/playbook.py --outbox DIR --notes DIR --drafts DIR
python tools/units/playbook.py --selftest
```
Flags: `--agents`, `--drafts`, `--json`, `--matching`, `--notes`, `--outbox`, `--print`, `--selftest`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: .pi/outbox, .pi/notes -> drafts.

## Invariants and rules

* This tool closes that loop. It reads every outbox, classifies each finding against a small registry of *levers* and *traps*, groups duplicates (five identical findings become one row), and writes a draft `docs/matching.md` section in the house style (Problem / Why try it / Result / Example) plus the matching index row (informational: `tools/agents/sync_playbook_index.py` generates the real one), numbered from the current highest. The drafts go to `.pi/playbook-drafts/` - this tool **never edits `docs/**` or `CLAUDE.md`**; the orchestrator lands the drafts in one commit.
* Where a finding can come from (all optional, the schemas drift between rounds):
* `playbook_candidate` (object: title/problem/result) and `suggested_playbook_row` (string) - a worker's explicit request;
* `config_requests` naming `docs/matching.md`/`CLAUDE.md` (kind `shared-file`) or a `flag` change whose evidence matches a registered lever;
* `flags_probed` / `flag_probes` - an `adopt` verdict is a lever, a `reject`/`inconclusive` verdict can be a registered trap;
* `.pi/notes/*.md` for the traps a worker records in prose rather than in the outbox.
* A finding is only drafted when it carries evidence: a before/after number for a lever, or a measured probe for a trap. Everything else is reported under "not drafted" with the reason - a finding with no numbers is not a row.
* The registry (`LEVERS`) is the one place the curated framing lives; extending it is how a new lever becomes a row. It is deliberately data, so the next round's workers can be pointed at it.

## Lib dependencies

outbox, text.

## Test contract

Tier: fixture.
Today's selftest (`tools/units/playbook_selftest.py`): No build, no `ninja` and no repository state: every outbox, note, CLAUDE.md and matching.md is a fixture written into a temp directory, so the contract is pinned - a finding is classified against the registry, five identical adopt probes become one group, an already-landed idea is skipped by number, a finding with no numbers is refused, and `run()` writes the drafts without touching `docs/**` or `CLAUDE.md`.
Target: `tools/tests/units/test_playbook.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

writes drafts only; the registry `LEVERS` is data to extend

## History (the incidents behind the rules - keep the rule, drop the narrative when the rule is stable)

* The round's problem: many workers independently found the same levers - `#pragma peephole off` (five or more units), `#pragma fp_contract off` (four or more), and the trap that `#pragma optimization_level 1` does *not* turn the peephole off (two) - and two filed a `config_requests` entry asking for a row. Nothing wrote those rows automatically: they sat in `.pi/outbox/*.json` until the orchestrator got to them, and a lever rediscovered by five workers is a lever that was not written down in time.
