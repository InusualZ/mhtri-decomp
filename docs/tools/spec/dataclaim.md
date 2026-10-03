# `dataclaim` - Decide whether a proposed data run may be claimed (overlap, target bytes, what ours emits, ledger effect); `--unit U` lists rule-12 references with the exact claim to paste

<!-- generated from the module docstring of `tools/units/dataclaim.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

Decide, mechanically, whether a proposed data run may be claimed - before `splits.txt` is edited.

## Users

the landing gate (4); profiles (`.claude/agents`) (5); skills (1); docs (23)

## CLI

```
python tools/units/dataclaim.py                   # verdicts over the whole queue + summary
python tools/units/dataclaim.py --risky 10        # the riskiest runs, with their reason
python tools/units/dataclaim.py --queue-unit Pl/pl_act   # one unit's proposed runs
python tools/units/dataclaim.py --json            # machine-readable entries
python tools/units/dataclaim.py --out FILE        # write the verdicts (atomic)
python tools/units/dataclaim.py --selftest
python tools/units/dataclaim.py --unit Pl/pl_act_step [--dry-run] [--json]
rule 12, the other direction: every data symbol that unit references but does not own,
with who else reads it, where the declaration actually sits (`declared in:` - the
address's owner and the declaration's home can disagree), and the exact `splits.txt`
claim (or named data-only unit) to fix it.
Read-only - it never writes `splits.txt`; `--dry-run` just says so explicitly.
It ends with the land gate's STRICT data-claim view of that unit (`datagap.strict_report`: the
data only that unit references, refusable or deferred with its class) and the exact `splits.txt`
edit that claims the refusable blocks (lines to ADD in section order inside the unit's block, or the
spanning range that REPLACES its existing line, with a partial-run note) - a touched unit must claim it.
```
Flags: `--dry-run`, `--fixpoint`, `--json`, `--limit`, `--out`, `--queue`, `--queue-unit`, `--risky`, `--root`, `--selftest`, `--unit`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: data-queue.json, obj/src objects, DOL, report, callers census -> verdicts.

## Invariants and rules

* docs/plan.md 7.8 / §6.1. Claiming a data range is the campaign's riskiest edit class: three of them were decided by hand and two were "do not claim" (playbook 23 - the RSO string pool cost 1.35 % on `fn_804DABF0`, `Gecko_ExceptionPPC.cp`'s `.bss fragmentinfo` would have collapsed `matched_data`). Those three decisions reduce to one sentence: **claim data the object emits; a range our object does not emit must not be claimed**, because a target section paired against nothing adds target bytes that nothing reproduces, so the unit's `matched_data` ratio falls.
* This is the reader for `tools/units/data-queue.json` (written by `dataqueue.py`, 7.17; read by `brief.py`). For every proposed run it answers, with evidence, without compiling and without touching a shared file:
* **the range next to what is claimed today.** The run's section/start/end is compared against every range in `config/RMHE08/splits.txt`; an intersection is a **refusal** (`overlap`), not a warning - the queue is a snapshot and `attribute.py apply` can have moved since it was written.
* **the target's bytes.** Read from the split object that currently covers the address (`build/RMHE08/obj/**/*.o`; the object's base address comes from `build/RMHE08/config.json`) and cross-checked against `orig/RMHE08/sys/main.dol`. The DOL is the fallback when no object covers the address. A covering object whose bytes disagree with the DOL is itself a refusal - the DOL already has those bytes right.
* **what our source emits.** Our object for the run's unit (`build/RMHE08/src/<unit>.o`) is read at the offset the run would land on: the unit's lowest claimed range in that section, or the run's own start when the claim would create the section. Equal for the whole run -> `safe` (+run size `matched_data`); absent, short or different -> `lowers-score`, naming the unit, symbol and worst function it affects.
* **the expected effect** on the ledger's matched bytes, from the unit's own numbers in `build/RMHE08/report.json`.
* Verdicts: `safe` | `lowers-score` (the symbol/function it would affect is named) | `overlap` (the run intersects a claimed range) | `unowned` (nothing to claim it into: an `auto/*` placeholder with no registered source, or the queue's own `never`/`owner-held`/`not claimed` refusal).
* Read-only by design: no `ninja`, no compile, no link, no write to `splits.txt`. `land.py` owns the batch that acts on these verdicts.

## Lib dependencies

project, refs, binary, report, text, findings.

## Test contract

Tier: fixture.
Today's selftest: in-file `selftest()` (`--selftest`).
Target: `tools/tests/units/test_dataclaim.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

None recorded.
