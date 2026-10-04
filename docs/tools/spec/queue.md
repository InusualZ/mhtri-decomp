# `queue` - Hand the next registered unit (or a header/module cluster of them, or a backlog debt item) to a worker: claim, render the brief, print the spawn line; refuses while unlanded work exists

<!-- rewritten by WP3e (2026-10-04): the registered units are the queue; the proposal/attribution paths and the address stride are gone -->

## Purpose

Hand the next registered unit (or a cluster of them) to a worker: claim it, render its brief, print the spawn line.

## Users

the landing gate (1); CLAUDE.md (11); docs (29); imported by `claims` (the claim CLI's brief + spawn)

## CLI

```
python tools/units/queue.py next [--count N | --cluster MODULE|HEADER [--cross-module]] [--kind K] [--profile P] [--worker W]
                                 [--dry-run] [--json] [--ignore-backlog] [--allow-unlanded B]... [--no-slots]
python tools/units/queue.py list [--json]
python tools/units/queue.py debt [--worker W] [--dry-run] [--json] [--ignore-backlog]
python tools/units/queue.py --selftest
```
Subcommands: `next`, `list`, `debt`.
Flags: `--allow-unlanded`, `--cluster`, `--count`, `--cross-module`, `--dry-run`, `--ignore-backlog`, `--json`, `--kind`, `--no-slots`, `--profile`, `--ratio`, `--selftest`, `--worker`.
`list --json` keeps its shape `{dir, counts, next}`: `dir` is the brief pool directory, `counts` the registered units by
state, `next` the first five ready units.

## Inputs and outputs

Inputs -> outputs: `configure.py` (the `Object(flag, path)` rows), `splits.txt`, the claim registry and the `worker/`
branches, the backlog -> claim + brief + spawn line.

## Invariants and rules

* **The registered units are the queue** (owner, 2026-10-04): every `Object(...)` row whose flag is not `Matching` and
  whose source exists. States: `ready`, `claimed` (a registry row, a cluster row's `units`, or the claim branch / the
  throwaway worktree - the lock - holds it), `matching` (done; never handed out), `nosource`. The pool used to be the
  body-less units only, which hid every NonMatching unit with a body (measured: 81 body-less of 331 NonMatching; the
  queue now offers all 331).
* `next` takes the ready unit with the lowest `.text` address (data-only units last, then by name), claims it first
  (`claims.claim`: branch + slot or worktree), then renders the brief at the claim's slug path against the claim's own
  worktree (`promote`) - a brief is never copied from `briefs/pool/`, and the outbox it names always exists.
* `next --cluster NAME` claims **one lane for every ready unit of a cluster**: a module (`--cluster Network`: every
  ready unit under `src/Network/`) or a header (`--cluster include/Network/net.h`: every ready unit **of the header's
  own module directory** whose include closure contains it - `--cross-module` adds the units of other modules that
  merely include it, and a top-level header has no module bound). Measured 2026-10-04 on
  `include/Network/network_transport.h`: 35 units over 8 modules before the bound (enemy, lobby, Pl, stage, hud, ef,
  quest pulled in through a hub include), 17 `Network` units after; the reason line names how many it left out. The claim is keyed `cluster/<name>` and records `units`; every member then reads `claimed`.
  The lane gets `briefs/<slug>.md` (the index: the members, the cluster's ack key and its one outbox) plus one brief
  per member under `briefs/<slug>/`.
* `next --count N` claims up to N lanes at once and **no two share an owner header or a module** (`disjoint_picks`,
  in address order). Owner headers are the closure's headers under `include/`/`src/` minus the shared base and
  fallbacks every unit sees (`include/types.h`, `include/dolphin/`, `include/MSL*`, `include/nw4r/`,
  `include/unsplit/`). A wave that cannot fill reports its shortfall; `--cluster` gives the rest one lane. (The
  address stride this replaces put header-sharing neighbours in concurrent lanes.)
* The guards run before any claim, in this order: MAIN's HEAD is `main`, a slot is free (the concurrency cap), the
  backlog credit covers the claim (`--ignore-backlog` spends nothing), and no branch holds work main lacks
  (`--allow-unlanded B` parks one on purpose). A real claim is recorded in the backlog ledger.
* `debt` hands out the register's top `naming`/`band-header` item on its file through the same lock and the same
  credit (one resolved item earns exactly one).
* The spawn's agent comes from `lib.lanes.launch.KIND_PROFILE` (`--profile` overrides, validated against
  `PROFILES`); a spawn whose cwd resolves to MAIN is refused.

## Lib dependencies

lanes (launch, naming, pool, registry), cscan (the include closure).

## Test contract

Tier: fixture (temp trees, an injected claim function).
Today's selftest: in-file `selftest()` (`--selftest`; the `queue_selftest.py` delegator was deleted in WP3e): the
state machine over registered units, the address order, the cluster selection by header and by module, the
no-shared-header wave and its shortfall, the cluster claim's index and member briefs, the branch lock, the MAIN-branch
guard, the credit ledger and `debt`.

## Known gaps

The cluster brief tells the lane to ack with the cluster key, but each member brief still prints its own unit's ack
line. `brief.py --pool` now pre-renders 331 briefs (a few seconds each); `next` never needs it.
