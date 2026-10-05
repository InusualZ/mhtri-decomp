# `backlog` - The ranked register of everything filed and not done (outbox config_requests, tooling register, lint/undefrefs/dataclaim debt) with the credit ledger `queue.py next` spends; `triage` proves items resolved/stale

<!-- generated from the module docstring of `tools/units/backlog.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

One ranked register of everything the campaign filed but has not done - the *backlog*.

## Users

profiles (`.claude/agents`) (2); skills (1); CLAUDE.md (1); docs (16); imported by `queue`

## CLI

```
python tools/units/backlog.py                       # regenerate MAIN/.pi/backlog.json + summary
python tools/units/backlog.py --print [--top N]     # human summary, write nothing
python tools/units/backlog.py --json                # the register as JSON on stdout (no write)
python tools/units/backlog.py --check               # exit 1 when the register is missing or stale
python tools/units/backlog.py --set-status KEY done # open / done / parked, then regenerate
python tools/units/backlog.py --selftest
python tools/units/queue.py debt                    # claim the top naming/band-header item
```
Flags: `--apply`, `--check`, `--json`, `--main`, `--notes`, `--outbox`, `--print`, `--ratio`, `--register`, `--selftest`, `--set-status`, `--tooling-register`, `--top`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: .pi/outbox, .pi/notes, docs/tooling-requests.md, lint -> .pi/backlog.json.

## Invariants and rules

* **The ledger can never drift.** Earnings are *derived from the item statuses* - the count of items a lane resolved (`done` that is not the source's default) - not from a stored counter, so enforcement never depends on the file surviving a clean checkout. `parked` earns **no** credit: parking removes a ghost, it does not buy a claim; only `done` does. The spent side (the claims handed out) is the one thing that cannot be derived from the statuses, so it is persisted alongside them in `.pi/backlog.json`; losing that file is fail-open by design (the statuses are lost with it, so the register restarts at 1 credit).
* **`triage` is evidence-based and never guesses.** Many "open" items were filed weeks ago against code that has since moved, so `python tools/units/backlog.py triage` classifies every open item as `resolved`, `stale` or `open` and prints the check that proved each. `--apply` writes the verdicts (`resolved` -> `done`, `stale` -> `parked`) and is idempotent, respecting any status a human already set. Anything that cannot be proved from the repository stays `open (no check)` - a triage that guesses is worse than the pile it is triaging. A lint-derived item follows the same rule: a `naming` / `band-header` item is `resolved` when the file no longer carries that rule's findings (re-linted, not remembered), `stale` when the file is gone, and `open` with the live count otherwise. An `undefrefs` item is `resolved` only when the one rule no longer fires for its unit (re-run over the unit's object, not remembered); with no compiled object to judge it stays `open` rather than guess.
* Five sources, one register:
* This is also the owner's "do not revoke committed progress - put the mounted naming debt in a backlog and work on it slowly" half: the stylelint items are ordinary open items, so the credit ratio rations new claims against them exactly as it rations against the rest.
* The pile is not 703 problems. The same defect is filed by several lanes over weeks, so items are keyed on `(kind, target, normalised-defect)`: for `shared-file` the header path plus a normalised summary of the defect, for `range` the address span, for `flag` the lib plus the flag. Repeats collapse into one item that records **how many lanes filed it and which** - a defect three lanes independently hit is a priority signal.
* `shared-file` mixes *records* with *requests*. A lane that used the shared-file exception files an entry describing what it **did** ("Added one union member to the +0x328 union ..."); those changes rode the branch that landed, so they are done by definition. Only an entry that states a live defect ("line 67 declares X while ... declares Y, so any TU that includes both fails with MWCC"; "should take one argument") is open. A small classifier splits them; every item keeps the raw filing text, so a wrong classification is visible rather than hidden.
* Statuses and the credit ledger live in `MAIN/.pi/backlog.json` (gitignored, the way `claims.json` is), so they survive a regeneration and can be set with `--set-status KEY STATUS`. The register is regenerated after each change.
* Ranking is by what predicts value: the number of independent filers, then an item's weight (a `naming`/`band-header` item's **distinct at-fault name** count - one name repeated 500 times is nearly no work, 50 names are 50 renames - an `untyped` item's `void *` count, or an `undefrefs` unit's reference count, so the file with the most work outstanding leads), then recency, then `tooling.py`'s votes. The register also publishes the summed weight per rule (naming, band-header, untyped, undefrefs) so "is the debt shrinking?" is a number rather than a memory; `--print` shows it. An item also shows how long it has been open; an item filed in an early phase may be stale because the code moved on, and that is exactly what the register is for - it is surfaced, never silently dropped, and a human or lane parks it.
* The published JSON is `version: 2`: the shape is backward-compatible (new keys only), but an item's `weight` for `naming`/`band-header` is now the distinct-name count rather than the occurrence count. The old occurrence number is preserved on the item as `count`, and each such item also carries its `names` list - the one place the meaning changed, so a reader that treated `weight` as occurrences should read `count` instead.

## Lib dependencies

outbox, findings, text, project.

## Test contract

Tier: fixture.
Today's selftest (`tools/units/backlog_selftest.py`): No repository state and no build: every outbox is a fixture written into a temp directory, so the contract is pinned - the `rename` kind is never carried; a repeated `shared-file` defect from two lanes becomes **one** item with a filer count of 2; a record ("Added one union member ...") defaults to `done` while a defect ("... illegal function overloading") defaults to `open`; one entry carrying two defects in one header becomes two items; a `range` admission is done while a re-draw is open; a `flag` whose change is "none" is done; a status survives regeneration; and `refusal()` names the top item while an empty register does not refuse.
Target: `tools/tests/units/test_backlog.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

None recorded.

## History (the incidents behind the rules - keep the rule, drop the narrative when the rule is stable)

* The owner's rule (2026-09-27, revised): **one resolved backlog item per new proposal claim.** While the backlog has open items, `queue.py next` does not hand out an unbounded stream of new proposal claims: it keeps a **credit ledger**. A `done` earns 1 credit; a claim spends 1 credit (`--ratio K` makes a claim cost K, i.e. K backlog items per claim); the register starts with 1 credit so the campaign can begin. The rule was unenforceable because the backlog was invisible: every lane ends with an outbox `.pi/outbox/*.json` whose `config_requests` list records what it found but was not allowed to change, and nothing tracked whether any of it was ever done (266 outboxes / 703 requests on 2026-09-27). This tool makes it one register with a lifecycle and a ledger, and `queue.py next` reads it.
* **`config_requests`** in `.pi/outbox/*.json`. A `range` (a seam re-draw, or a data run to claim) is open until a proposal pass re-draws it; a `shared-file` (a defect in a header a worker may not touch) is open until fixed; a `flag` (a compiler flag for a lib) is open until measured and adopted or rejected. A **`rename` is not backlog** - the landing applies it, so it is done when its batch lands - and is not carried at all.
* **`tools/units/tooling.py`**'s own register (`docs/tooling-requests.md`): the ranked tooling/environment requests with their open/done/parked statuses. It is read, never duplicated.
* **`tools/units/stylelint.py`**'s findings, aggregated **per file**: one `naming` item per file carrying rule-7 findings (`fn_XXXXXXXX` / `lbl_XXXXXXXX` / `loc_XXXXXXXX` / bare `unk*`), one `band-header` item per file carrying rule-2 findings (an `extern` that belongs in the owner's header or `src/unsplit/`), and one `untyped` item per file carrying rule-11 findings (a `void *` parameter or return type with no `/* untyped: <reason> */` marker). The item's ask names the file, the rule and the **distinct at-fault names** still outstanding, and that count - not the occurrence count - is the item's rank weight, because one name repeated 500 times is one rename while 50 distinct names are 50 (`lint_index`). The names come from `stylelint.finding_identity`'s token, the same predicate the landing gate's `--diff` refuses a *new* token on, so the register and the gate can never disagree about what "a name" is. A lint item's key is the (kind, file) pair, so a partial fix keeps its status; the item is carried forward from the published register even after the findings are gone, so `triage` can prove the file clean and close it (and so a resolved item stays in the register and earns its credit). This is the "do not revoke committed progress
* work the debt slowly" half of the owner's naming ruling (2026-09-27): hundreds of such open items against the campaign's balance keep naming and typing work interleaved with new claims through the ratio, with no special-casing.
* A `naming`/`band-header` item is also **claimable** (`queue.py debt`): it carries the file and its distinct name list, so a lane can be handed "clean the N names in this file" through the same `claims.py` worktree/branch lock a unit proposal uses, spending one credit through `record_claims` - one resolved item still earns exactly one. The debt becomes scheduled work instead of something a lane pays down incidentally while passing through.
* **`tools/units/undefrefs.py`**'s pre-existing-debt register (`--census`): one `undefrefs` item per unit whose object relocates a name no link input can define - the flip blocker a score cannot see. The unit's reference count is the item's rank weight, so the worst unit leads. Like a lint item it is an ordinary open item, so the credit ratio rations new claims against it exactly as against the rest, with no special-casing; its key is the unit, and it is carried forward from the published register so `triage` can re-run the one rule (`undefrefs.unresolved_names`) and close it once no undefined reference remains.
* **`tools/units/datagap.py`**'s strict data-claim rule (`strict_report`): one `data-claim` item per unit that still has refusable sole-owned orphan data (data only that unit's object references, claimable on its own). The pair count is the item's rank weight; the item is an ordinary open item the credit ratio rations against, keyed by the unit, carried forward from the published register, and `triage` re-runs the one rule and closes it once the unit has none left - the same lifecycle as an `undefrefs` item. A second `data-claim` defect (owner, 2026-09-30) is `claim-exposed:<section>:<start>-<end>`: one item per (unit, pair-run) of pairs only the unit's own claimed data references - the pairs the land gate defers (`datagap.claim_exposed_pairs`) because claiming data exposes the pairs its relocations name. Weight = the run's pairs (the `sole-owned` count leaves them out); `triage` closes it when no such pair remains in the range.
