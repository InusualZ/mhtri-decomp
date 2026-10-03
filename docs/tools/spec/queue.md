# `queue` - Hand the next pooled unit/proposal (or backlog debt) to a worker: claim, re-render the brief, print the spawn line; `--count N` strides a wave; refuses while unlanded work exists

<!-- generated from the module docstring of `tools/units/queue.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

Hand a pooled brief to a worker: claim the unit, promote the brief, print the spawn line.

## Users

the landing gate (1); CLAUDE.md (11); docs (29); imported by `claims`

## CLI

```
python tools/units/queue.py next [--count N] [--worker NAME] [--dry-run] [--json] [--ignore-backlog]
python tools/units/queue.py list [--json]
python tools/units/queue.py debt [--worker NAME] [--dry-run] [--json] [--ignore-backlog]
python tools/units/queue.py --selftest
```
Subcommands: `next`, `list`, `debt`.
Flags: `--allow-unlanded`, `--count`, `--dry-run`, `--ignore-backlog`, `--json`, `--kind`, `--no-slots`, `--profile`, `--ratio`, `--selftest`, `--worker`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: pool, queue JSON, backlog, claims -> claim + brief + spawn line.

## Invariants and rules

* `debt` hands out the register's `naming`/`band-header` debt the same way: it claims the top open item on its file (`claims.claim`, the same worktree/branch lock), writes a brief naming every distinct at-fault name, and spends one credit through `backlog.record_claims` - one resolved item still earns exactly one. The register therefore both rations new proposal claims against the debt *and* lets a lane be tasked with paying it down.
* `next --count N` claims a **wave** of N: a stride of N through the address-ordered queue, never N neighbours. Adjacency is the vector for almost every clash this campaign has had - the two halves of one translation unit are two adjacent proposals (`proposal/8007270C`+`proposal/80073180`, both `g3d_calcvtx.cpp`), a rule-2 boundary artefact appears when a neighbour registers a symbol you declare, and neighbouring units share owner headers and types by construction - so a wave takes proposals `i, i+N, i+2N, ...` instead. That is a **guarantee**, not a probability: two adjacent proposals can share a wave only if both indices are congruent mod N, which is impossible for N > 1. Random sampling would still put both halves of one TU in a wave about once in N tries. The stride is taken in the queue's own address order and never re-sorted, so a wave is spread across the address bands for free; the cost is cross-unit knowledge reuse (adjacent proposals tend to share a TU, a header, a type), so a wave is spread *within* a band rather than scattered for its own sake.
* A pooled brief is claim-independent by construction: it is rendered against the worktree the claim *will* create (`claims.worktree_for`) and against the branch `claims.py claim` *will* make (`worker/<slug(unit)>`), so `--pool` can prepare it before a claim exists. The claim path re-renders rather than copies (see `promote`), so the brief a worker gets always matches the entry the queue holds at claim time, including a manually named branch whose slug (and therefore outbox path) is its own. The unit is claimed **before** the brief is written, so a worker never gets a brief whose outbox does not exist.
* `list` shows the pool's state: briefs written, ready (unclaimed, no bodies), claimed (in flight), written (a unit that has gained a body - `brief.py --pool` prunes those), covered (its range is registered already, under whatever name - never handed out) and stale (no longer registered), plus the next few ready candidates in address order.

## Lib dependencies

lanes, outbox, project, report.

## Test contract

Tier: fixture (injected claim function).
Today's selftest (`tools/units/queue_selftest.py`): The checks run against a temp fixture with an injected claim function, so no git worktree, no `ninja` and no repository state are touched. What they pin: the pool's state machine (`ready` / `claimed` / `written` / `stale` / `unreadable`), the address order `next` picks, that `--dry-run` claims nothing, that the real flow claims the unit *before* promoting the brief, that the promoted brief is a copy of the pooled one at the claim's own slug, that a suffixed branch is re-rendered so its outbox path is the claim's, and that the printed spawn carries the cwd, name and task the orchestrator pastes. The wave selection is pinned too: a `next --count N` wave strides the address order, so no two picks are adjacent; `N=1` is the single pick `next_entry` makes; a claimed or covered stride position is skipped without breaking the stride; and fewer than N ready claims what exists and reports the shortfall instead of failing. `queue.selftest()` holds the checks so `queue.py --selftest` and this entry point cannot drift.
Target: `tools/tests/units/test_queue.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

a claimed proposal is still `ready` for selection (CLAUDE.md known bug)

## History (the incidents behind the rules - keep the rule, drop the narrative when the rule is stable)

* `brief.py --pool` prepares the brief for every registered unit that has no bodies yet (owner's ask, 2026-09-23: "prepare briefs in advance and queue new work right away"), so the orchestrator can start a worker the instant a slot frees without deriving anything. This is the other half:
* `next` picks the pooled unit with the lowest `.text` address that is still **unclaimed**, takes the claim (`claims.py claim` creates the worktree and the branch), renders the brief **from the current queue entry or `splits.txt` range**, writes it to the claim's own slug path, and prints the exact spawn line - cwd, name and task text - to paste. The brief is never copied from the pool: `brief.py --pool` skips a brief that already exists, so a queue regeneration can leave every pooled file describing the old range, and copying one handed a worker the wrong scope (`proposal/80119DEC`, 2026-09-25). The pool still decides *which* unit is next; it is not the source of the worker's brief.
