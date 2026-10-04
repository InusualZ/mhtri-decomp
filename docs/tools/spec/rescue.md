# `rescue` - Audit `refs/rescue/*`: derive the units each ref registers, classify redundant/landed-with-drift/unlanded/unknown, prune only redundant

<!-- generated from the module docstring of `tools/units/rescue.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

Audit the `refs/rescue/*` safety net and prune only what is provably redundant.

## Users

docs (7); the classifier is `lib.lanes.rescue` (`claims.release` reads a ref's verdict from the lib; this tool is the audit CLI)

## CLI

```
python tools/units/rescue.py audit [--prune] [--json] [--full-diff] [--ref REF]...
[--repo PATH] [--main REF]
python tools/units/rescue.py --selftest
```
Flags: `--full-diff`, `--json`, `--main`, `--prefix`, `--prune`, `--ref`, `--repo`, `--selftest`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: git refs -> report.

## Invariants and rules

* For every ref the audit reports
* the ref name and the tip's date and subject;
* **the unit(s) the ref registers**, derived from the ref's registration **diff against its merge-base with `main`** - the `Object(...)` rows and `splits.txt` unit headers that ref *added*. A whole-file string match against `main` ('every name in the ref's configure.py that also appears in main') matches every unit in the file and is useless; the diff is what names the ref's own registration. When the registration diff is empty (a ref that only *edits* a registration main already had) the units are read from the `src/**` paths the ref touched and the derivation is labelled `touched-path`;
* whether each unit is registered on `main` today (an `Object(...)` row **and** a `splits.txt` block - the two halves `verifyunit.registration_problems` asserts);
* the content diff of those touched paths against `main`.
* and classifies it:
* ``redundant`` - every unit is on `main` and every touched path matches `main`; nothing is missing.
* ``landed-with-drift``- every unit is on `main` but the paths differ: `main` has moved on since.
* ``unlanded`` - at least one unit is **not** on `main`; this ref may hold the only copy. Never pruned.
* ``unknown`` - no merge-base with `main`, or nothing parseable to derive a unit from. Never pruned.
* `--prune` deletes **only** `redundant` refs and prints each deletion; without it the audit is strictly read-only. `landed-with-drift`, `unlanded` and `unknown` are never touched, prune or not.
* The classification is deliberately conservative: a unit *renamed* on `main` (the `auto/` placeholders that migrated to their final homes) still fails the by-name registration check and lands in `unlanded`, where it is surfaced and kept - the safe direction. The point is to stop a future teardown from ever deleting the last copy of work, not to reclaim disk.

## Lib dependencies

lanes.rescue, git, project.configure.

## Test contract

Tier: fixture (GitFixture).
`tools/tests/units/test_rescue.py` (GitFixture; `--selftest` forwards to it): all four verdicts, both derivations, the read-only audit, `--prune` deleting only `redundant`, `--ref`, the CLI's `--json`. The registration readers are `lib.project.object_calls`/`Splits` and `lib.units.stem` (the `unionresolve`/`verifyunit` imports are gone).

## Known gaps

None recorded.

## History (the incidents behind the rules - keep the rule, drop the narrative when the rule is stable)

* `claims.py release` / `timeout` copy a branch's unlanded commits to `refs/rescue/<slug>` **before** deleting the branch, because the branch is the lock and the work it held must not vanish with it. The refs are a safety net, not debris: some of them hold the only copy of work no landing ever took. They also accumulate - 193 of them by 2026-09-27 - and nothing in the campaign had ever looked at them.
