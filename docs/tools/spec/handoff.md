# `handoff` - The outbox contract: print the skeleton/template and validate an outbox entry (owned symbols, percents, measured_with, config_requests schema)

<!-- generated from the module docstring of `tools/units/handoff.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

Print the report skeleton a worker fills in, and validate what comes back.

## Users

the landing gate (1); docs (11); imported by `backlog`, `brief`, `land`

## CLI

```
python tools/units/handoff.py <unit>                  # the digest skeleton (a table to fill in the reply)
python tools/units/handoff.py <unit> --template       # the outbox JSON to fill in
python tools/units/handoff.py <unit> --check FILE     # validate an outbox entry
python tools/units/handoff.py --selftest
```
Flags: `--check`, `--json`, `--selftest`, `--template`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.
* `handoff.py --check-requests FILE`: validates a lane's `<slug>-requests.json` (`lib.outbox.check_requests`, the
  `lib.requests` schema); a new-schema line's problems are errors (exit 1), a free-text line is a note saying what the
  integrator's loader read from it.

## Inputs and outputs

Inputs -> outputs: unit, outbox JSON -> verdict.

## Invariants and rules

* docs/plan.md 7.4 / §5.3. Twelve workers in twelve processes produce twelve reports; if those are prose, the orchestrator reconciles numbers by hand - which is the failure mode the protocol exists to remove. So the handoff is data: `MAIN/.pi/outbox/<slug>.json`, and this tool both emits the skeleton and refuses a bad one.
* What `--check` enforces (each is something the orchestrator would otherwise have to notice by hand):

## Lib dependencies

outbox, project, findings.

## Test contract

Tier: fixture.
Today's selftest: in-file `selftest()` (`--selftest`).
Target: `tools/tests/units/test_handoff.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

None recorded.

## History (the incidents behind the rules - keep the rule, drop the narrative when the rule is stable)

* the required fields exist and have the right types;
* every symbol it reports is a symbol the unit actually owns (a typo or a stale name would silently score 0), for the unit a single-unit outbox names *and* for every unit a batch outbox declares (`unit`, `units`, `also_changed_units`, `per_unit`) - a batch that registers or touches several units writes one outbox, and the ownership check has to read all of them;
* `unit_percent` and each `percent` are in 0-100;
* `measured_with` names the command, so a number can be reproduced and a hand-written compile spotted;
* `config_requests` entries carry the evidence the plan's §8 requires. The accepted kinds and their required fields are `CONFIG_REQUEST_SCHEMA` below - the one definition, which `brief.py` renders into the worker's brief (part 6). A kind outside the table (`tooling`, `naming`, ...) or a known kind whose structured fields are absent is still accepted when it carries its content under a free-text field (`FREE_TEXT_FIELDS`), so a real filing is never refused for spelling its field the way its lane does. `flags_probed` is a list of `{flags, effect, verdict}` objects, but a prose string or a probe filed under a lane's own keys (`flag`/`result`) is accepted as content rather than refused.
