# `tooling` - Cluster the reports' tooling/environment requests into the ranked `docs/tooling-requests.md`

<!-- generated from the module docstring of `tools/units/tooling.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

Turn the `## Tooling and environment` sections of worker reports into a ranked register.

## Users

profiles (`.claude/agents`) (2); docs (7); imported by `backlog`

## CLI

```
python tools/units/tooling.py                 # scan, write docs/tooling-requests.md
python tools/units/tooling.py --json          # the same data as JSON on stdout
python tools/units/tooling.py --check         # exit 1 when the register is stale (for a commit hook)
python tools/units/tooling.py --print         # human summary, write nothing
python tools/units/tooling.py --set-status seed-worktree done
python tools/units/tooling.py --outbox DIR --notes DIR --register PATH
python tools/units/tooling.py --selftest
```
Flags: `--check`, `--json`, `--notes`, `--outbox`, `--print`, `--register`, `--selftest`, `--set-status`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: .pi/outbox, .pi/notes -> docs/tooling-requests.md.

## Invariants and rules

* How a suggestion is found (all optional - the report schemas drift between rounds):
* an outbox key whose name contains `tool` or `environment` (string, list or object) - the structured channel the harness populates (`tooling`, `tools_wanted`, ...). A **list** value is the structured channel proper: every element is one request the lane filed and becomes **its own row**, keyed by the tool/path it names (`target_of`), and the rows it did not become are reported (`skipped_tooling`, printed by `--print`/ `--json`). A lane that files two bullets gets two rows; they are never agglomerated with each other.
* a `.pi/notes/*.md` section whose heading names tooling/environment/worktree/setup/reproduction (the section variants workers actually wrote: `## Tooling notes for the next worker`, `## Environment note`, `## Worktree setup`, `## How to reproduce a measurement`, `## Tooling worth keeping`, ...);
* a signal line anywhere in a note or an outbox: a tool/env noun (`worktree`, `orig/`, `compilers`, `ninja`, `configure.py`, `recompile`, `objdiff`, `m2c`, `stylelint`, `junction`, ...) within a line that also carries a friction verb (`no`, `without`, `lacks`, `needs`, `had to`, `copied`, `by hand`, `cost`, `minutes`, `fails`, `unsafe`, `declines`, ...).
* Clustering is the point. Two workers who hit the same wall phrase it differently, so the register does not key on wording:
* a small curated `TOPICS` registry (the walls already known by name, e.g. the empty-worktree split) matches a whole *source*, and every distinct worker that mentions the wall is one vote;
* everything else is agglomerated by token overlap (Jaccard, with an overlap-coefficient escape hatch for a short phrasing of a longer one), so two novel phrasings of one request become **one entry with two votes**. A **structured** `tooling` list element is exempt from this: it is one filed request, so it never merges with another bullet (only the identical bullet from another lane is a second vote).
* A `**Status.**` line per entry (`open` / `done` / `parked`) is carried across regenerations - the only hand-editable part of the file - so the list stays honest as things get fixed.

## Lib dependencies

outbox, text.

## Test contract

Tier: fixture.
Today's selftest (`tools/units/tooling_selftest.py`): No build, no `ninja` and no repository state: every outbox and note is a fixture written into a temp directory, so the contract is pinned - a structured outbox key and a prose `## Tooling and environment` section both yield suggestions; two workers phrasing the same wall differently become **one** entry with a count of 2; two novel phrasings of one request are clustered by similarity; a duplicate outbox from one worker does not double-vote; the ranking is votes-descending then cost-descending; and a `**Status.**` line survives regeneration (the register's only hand-edited field). One more, and it is the reason this file was touched (2026-09-28): a **structured `tooling` list** is one row per bullet, keyed by the target it names - `.init` filed two bullets and the register carried one, so neither bullet may be agglomerated with the other, the identical bullet from a second lane is a second *vote*, and a bullet that was skipped (too short, or already owned by a curated topic) is reported.
Target: `tools/tests/units/test_tooling.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

None recorded.

## History (the incidents behind the rules - keep the rule, drop the narrative when the rule is stable)

* Every `decompiler` report now ends with one to three *tooling/environment* entries: the wall that cost the most wall-clock time, phrased as a capability ("the worktree needs `orig/RMHE08/**` + `build/compilers`, without the DOL nothing splits") rather than a complaint. Those entries used to die in the report; this tool collects them from the same two sources `tools/units/playbook.py` reads - `.pi/outbox/*.json` and `.pi/notes/*.md` - **clusters the ones that ask for the same thing**, and writes a single tracked register (`docs/tooling-requests.md`) sorted by demand.
