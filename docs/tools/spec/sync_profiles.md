# `sync_profiles` - Inject the section 6.5 rule block (generated from docs/plan.md) into every profile; `--check` is a docs-batch gate row

<!-- generated from the module docstring of `tools/agents/sync_profiles.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

Inject the canonical section 6.5 rule table into the subagent profiles.

## Users

the selftest runner (6); profiles (`.claude/agents`) (16); CLAUDE.md (2); docs (10)

## CLI

```
python tools/agents/sync_profiles.py            # regenerate every profile's block in place
python tools/agents/sync_profiles.py --check    # exit 1 when a profile is stale (writes nothing)
python tools/agents/sync_profiles.py --print    # show the block it would write, without touching a file
python tools/agents/sync_profiles.py --selftest # run tools/agents/sync_profiles_selftest.py
```
Flags: `--check`, `--print`, `--repo`, `--selftest`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: docs/plan.md -> .claude/agents/*.md.

## Invariants and rules

* Between a BEGIN/END marker pair in each profile, this tool writes a block derived from the rule table of `docs/plan.md` section 6.5 - the canonical table - and nothing else:
* one line per rule: its number, the table's bold rule title, and the table's own meaning cell (verbatim, so no operative exception can be summarised away);
* the section's enforcement sentences, selected from its enforcement paragraph.
* It is idempotent (`python tools/agents/sync_profiles.py` writes the block only when it differs), and `--check` exits non-zero when a profile's block does not match what the plan says today - so a rule change that forgets the profiles fails the check instead of silently leaving the prompts stale.
* The same tripwire covers the profile *set*, not only its text: a file in `.claude/agents/` whose frontmatter carries a `name:` (i.e. a profile, unlike `TESTS.md`, which is prose) that is **not** in `PROFILES` is an error that names the file. `surveyor.md` was added to the directory but not to `PROFILES`, and `--check` went on printing "all profiles in sync" while the new profile carried none of the rules - the exact failure this tool exists to prevent, one level up. A new profile is only covered once it is listed here.
* The block is generated from the *table*, so a new rule (rule 12) and every exception clause (rule 2's unowned extern -> `src/unsplit/`; rule 7's "no exemption and no deferral"; rule 11's `/* untyped: <reason> */`) reach every generated profile the moment the plan does. If the plan's section-6.5 shape changes enough that the table cannot be parsed, this tool refuses loudly instead of writing a stale or empty block - that refusal is the next rule change's tripwire. The block's title says `rules 1-N` from the table's row count (N = 12 today), so no rule count is written down here to drift.
* Run it from MAIN (`docs/plan.md` is the authority there); the selftest runs the same code on fixtures and on the real tree.

## Lib dependencies

text, repo.

## Test contract

Tier: fixture; smoke: every profile in sync (strict, by design).
Today's selftest (`tools/agents/sync_profiles_selftest.py`): The point of the tool is that the profiles' section 6.5 text cannot drift from `docs/plan.md` section 6.5. The test pins that in three ways: * on fixtures - the block carries every table row (a new rule appears without a code change, rule 11's `/* untyped: <reason> */` marker and rule 12's claim-the-unowned-range row included), the enforcement paragraph is filtered to its enforcement sentences (the audit table and the "apply immediately" sentence stay out), the markers are refused when duplicated or reversed, and a deliberately stale profile is reported stale and then in sync; * on the real tree - every generated profile is in sync with the real plan, and none still teaches the deleted `rule 7 deferred` escape (that is the drift this tool exists to end); * against `brief.plan_section` - the brief and the profiles read section 6.5 through the same bytes.
Target: `tools/tests/agents/test_sync_profiles.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

the docstring says `N = 12 today`; the table has 13 rows

## History (the incidents behind the rules - keep the rule, drop the narrative when the rule is stable)

* WHY THIS EXISTS (the drift it fixes, measured 2026-09-27):
* The campaign's brief is machine-generated - it reads `docs/plan.md` section 6.5 verbatim through `brief.plan_section` - so it cannot drift. The subagent profiles are hand-written prose, and `.claude/agents/decompiler.md` and `.claude/agents/fixer.md` each *duplicated* the rule text (the decompiler inlined it across ~110 lines). Duplicated policy drifts, and nothing detected it: commit `4f3cb4ea1` deleted the `rule 7 deferred: <reason>` escape (rule 7 now fires on every `fn_` / `lbl_` / `loc_` / `unk` identifier in `src/`, whoever owns it - no exemption, no deferral; the land gate's `--diff` is the only grandfather), and both profiles went on teaching the deleted key as legal. The same rot was waiting for rule 11 (`void *`), which neither profile knew about - and again for rule 12 (unowned data the unit must claim), which is why the block's rule count is computed from the table rather than written down here.
* THE MECHANISM: the rules are generated, the prose around them is hand-written.
