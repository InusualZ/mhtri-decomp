# `land` - The land gate: `record-base`, `verify` (the ~30 PASS/FAIL rows), `land` (apply/stage/commit/release) and `resolve` for a branch

<!-- generated from the module docstring of `tools/units/land.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

The land gate: one command that runs the batch checklist and refuses to let a bad batch through.
Since WP4 `land.py` is the CLI shim and the gate is the package `tools/units/landing/` (layout: `landing.md`).

## Users

the selftest runner (1); profiles (`.claude/agents`) (22); CLAUDE.md (5); docs (72); imported by `mergebranch`, `prepcommit`

## CLI

```
python tools/units/land.py record-base [--json]
python tools/units/land.py land --units a,b [--base SHA] [--no-build] [--no-outbox] [--no-release]
[--no-selftests] [--already-applied] [--manifest PATH|SLUG]
python tools/units/land.py land --branch worker/<slug> [--units a,b] [--base SHA] [--no-build]
[--no-outbox] [--no-release] [--message SUBJECT] [--no-selftests] [--manifest PATH|SLUG]
python tools/units/land.py verify [--base SHA] [--units a,b] [--dry-run] [--no-build] [--no-outbox]
[--no-release] [--allow-regression UNIT] [--no-selftests] [--manifest PATH|SLUG]
python tools/units/land.py resolve --branch worker/<slug> [--worktree PATH] [--main PATH] [--base SHA]
[--no-commit] [--json]
```
Subcommands: `record-base`, `verify`, `land`, `resolve`.
Flags: `--allow-orphan`, `--allow-regression`, `--allow-rule10`, `--allow-rule12`, `--already-applied`, `--base`, `--branch`, `--dry-run`, `--json`, `--main`, `--manifest`, `--message`, `--no-build`, `--no-commit`, `--no-outbox`, `--no-release`, `--no-selftests`, `--selftest`, `--unit-rename`, `--units`, `--worktree`.
Exit codes: `verify` 0 when every row passed, 1 otherwise; `land` prints one `LANDED ...` / `REFUSED ...` line on stdout (the gate log goes to stderr) and its exit status is that answer; a refusal never reaches `git commit`. Every refusal line carries the row's KIND: `GATE` (the batch is bad) or `BOOKKEEPING` (the landing's own state is stale; the remedy is printed).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.
* `land.py integrate ARGS...` forwards to `tools/units/integrate.py ARGS` unchanged (by subprocess); applying requests
  is not landing - the branch it builds lands through `land --branch`.

## Inputs and outputs

Inputs -> outputs: main tree, branch, .pi/land-base.json -> rows, commit.

## The rows of `verify` (the order is the run order: `landing/gate.py` `PRE_BUILD`, then the build; one function per row in `landing/rows/`)

1. ground truth: `build.sha1` equals the DOL's hash, both read in the tree the gate judges (GATE; before WP4 they
   were read in the tree `prepcommit.py` lives in - the same tree for a gate run in MAIN).
2. `main` has not moved since the batch base; the base was recorded (`record-base`) (BOOKKEEPING). `record-base`
   rebuilds `build/RMHE08/report.json` (`ninja`) before it snapshots it, so the base's scores are the base tree's; a
   rebuild that failed or changed the tree adds the row "the batch base's report.json was rebuilt at record-base"
   (BOOKKEEPING; no row otherwise, and none for a base recorded before WP4).
3. every changed path belongs to the batch (tool scratch `d<digits>.json`/`t<digits>.json` is tolerated and named), and none
   is added or changed under the retired `include/` root (a deletion or a rename out of it is fine: `common.retired_paths`); no batch file carries a conflict marker (GATE).
   With `--manifest PATH|SLUG` only: the batch touches only the lane's manifest - every changed path is inside its
   `owns` globs and outside its `read_only` ones (GATE); an unreadable or malformed manifest is BOOKKEEPING; no row
   without the flag (`rows/manifest.py`, `lane-manifest.md`).
4. every unit's branch carries its work as commits (BOOKKEEPING refusal - the gate's only "the work exists" test); batch units named (or `--no-outbox` for an orchestrator-only batch) (BOOKKEEPING); every unit's outbox validates (`handoff.validate`) - a **WARNING** since 2026-10-05 (`every unit's outbox validates (warning)`, always PASS: each problem is printed as `WARNING: ...`, written as a `warning:` line in the commit body and into the landing log's `warnings`); it is skipped with a printed note for `--branch` naming a branch that is not `worker/<slug>` (only a claim's lane writes an outbox).
5. style lint (section 6.5) adds no violation (`stylelint --diff <base>`, add-only, with rename/move credits; rule 15's advisory classes never count) (GATE).
6. all tool selftests pass except the parked list (`tools/selftest.py`, ~30 s); `--no-selftests` skips (GATE).
7. rule 2 registration boundary (a batch registers a range a band header still declares) - a warning row.
8. *(deleted 2026-10-05)* the `rule 7 deferred` escape-growth row: rule 7 is row 5's (the lint fires on every added
   generated name; the escape is inert since 2026-09-27). Replay: over the 313 gated commits since it was added
   (`373c93732`) the escape grew in 19 and the row refused none.
   7b. *(added 2026-10-05)* no newly registered unit has a generated name (rule 7) - a **REFUSAL** (GATE) for a unit
   the batch registers (a `splits.txt` block or an `Object(...)` row the base did not carry) whose path spells a
   generated component (`lib.names.generated_path_components`: a `fn_`/`lbl_`/`loc_`/`dtor_`/`zz_` stem or a DOL
   address, in the stem or a directory). A GUESS name passes; a `--unit-rename OLD=NEW` is credited when NEW adds no
   generated component OLD lacked (a rename to a named stem, a move that keeps one). It protects new registrations
   only and is reported only when a new registration or a declared rename spells a generated component, so a batch
   registering named units shows the same table as before (`objects.new_unit_name_row`, pre-build).
9. the gate's own subject follows the convention (`commitlint`) (GATE).
10. `configure.py`, then `ninja -k 0` scoped to the batch's objects (the compile gate; a foreign dirty object that fails is named and tolerated), then `ninja build/RMHE08/ok` - each command's exit code is a row (GATE).
11. every batch unit is registered on three axes: `Object(...)` line, `splits.txt` block, build-graph target (`verifyunit`) (GATE).
12. every batch unit's relocations resolve against the link - no *new* undefined reference against the base snapshot (`undefrefs`) (GATE); the base must carry the snapshot (BOOKKEEPING).
13. every unit the batch flips to `Matching` is `flipcheck` READY (GATE).
14. rule 10 (vtable ownership) adds no violation (`vtableaudit --diff`, add-only, decided by `vtableaudit.diff_rows`: a
    `run:` key whose start and end each moved by at most one word against a removed run is SHIFTED, not added, and the
    shifted/removed keys are printed in the row's evidence; a run a recut re-owned from another unit is credited and
    printed `rule 10: N re-owned ... <key> <- <old owner>`; `ref:` keys are a set difference; `--allow-rule10` records an
    allowance) (GATE).
15. data closure: no batch unit's target object references data no claim covers, and no unit the batch really changes leaves data only it references unclaimed (`datagap` snapshot rows; `--allow-orphan`) (GATE); the base must carry the snapshot (BOOKKEEPING).
16. no unit's split target object moved under the batch (a neighbour re-ranged) (GATE) - "moved" is the
    rename-insensitive `objcompare.fingerprint`; a unit the batch does not name whose bytes changed by names only (a
    map rename rewrites its symbol table) is not drift and is listed in the row's evidence and the gate log
    ("name them in --units"), never auto-added: the replay of the last 15 report-affecting landings found such units in 7 of them (79 registered units),
    and adding them would put them under the unit-scoped rows (undefined references, the strict data closure) a
    replay of reports cannot judge - a proposal, not a change.
17. per-symbol re-measure reproduces the report from the objects (`verifyunit.verify_units`): fresh `report generate`, 100 % claims checked against bytes, the unit percent reproduced from its partials (GATE).
18. no symbol or unit regressed (per symbol, every symbol - one at 100 % that fell or lost its score included; a NEW symbol is never a regression; the unit average only for a unit that did not grow, `matched_code` read as a number) (GATE); every `--allow-regression` was actually needed - a **WARNING** since
    2026-10-05 (a stale allowance is printed, logged and written in the commit body; it no longer costs a rerun).
19. `ok` was recreated by THIS run (the stamp is deleted before the run) (GATE).
20. *(deleted 2026-10-05)* the knowledge-delta row (it passed whenever a `src/*.c*` file changed, and was coded GATE
    while this list called it a warning). Replay: of the 572 gated commits, 270 improved the ledger and none did so
    without a `docs/`, `CLAUDE.md` or `src/*.c*` change - it never fired. The ledger delta is still reported (the
    `ledger:` line of the gate log and the commit body).
21. claim released for every gated unit (`claims.release`; deferred when a row above failed; `--no-release` skips) (BOOKKEEPING).

## Invariants and rules

* **A WARNING row never refuses** (2026-10-05; `Batch.warn`): it is a PASS row named `<check> (warning)`, its findings
  are printed as `WARNING: <check>: <finding>`, handed to `land` through `verify(..., warnings=)` and recorded in the
  commit body (`warning:` lines) and the landing log (`warnings`). The outbox row and the stale-`--allow-regression`
  row are the two; the rule-2 band row is a warning of the older kind (printed, never logged). Replay: no attempt of
  the landing log was refused by either row, so no recorded verdict changes; the golden changes only those two rows.

* docs/plan.md 7.5 + 7.16, §11. Six manual commands and a regression-scan heredoc were the previous version of this, rewritten every batch - and that is where a mistake hides. It is also the place where a green `ninja build/RMHE08/ok` can lie: the `ok` stamp file may be from an earlier run, and a `NonMatching` batch never relinks, so `main.elf` never runs and `ok` is the only edge that re-validates anything. So `verify`
* `land --branch` is the one-command landing, so the orchestrator never assembles it by hand again. It refuses a dirty tree (with the exact clean commands), records the base on the clean tree **before** the pick, applies the branch's own delta with three-way (`git apply -3`, which - unlike a cherry-pick - does not lose the work a merge carried), resolves a registration conflict with `resolve`'s union instead of aborting, runs the gate above (compile gate included), then commits and releases. It is idempotent and loud: any refusal leaves the tree as it was (the apply is undone) and prints one `REFUSED ...` line whose exit status is the answer.
* The one outside-the-batch path this gate does not refuse is **tool scratch**: `d<digits>.json` / `t<digits>.json` in the repo root, the objdiff `diff` dumps a tool leaves behind (`65492794` ignored them, but a staged file bypasses `.gitignore` and the landing flow's own `git add -A` staged `d910.json`). The batch never received them, so they are never staged, the gate de-indexes a staged copy (`git reset HEAD -- <path>`, which leaves the caller's file in the tree), and the tolerance is **named** - in the gate log, in the check's `info`, and in the landed message - rather than silently dropped. Every other outside-the-batch path is still refused loudly: a foreign edit to `src/`, `config/` or `configure.py` is exactly what the guard is for, and a `d`/`t`-shaped name elsewhere is not a permission.
* The regression scan is **per symbol**. A unit's `fuzzy` is an average over its symbols, so an already-registered unit that is *extended* - its splits range widened, a head joined to its tail - falls in average as the weaker new bodies join it. That is not a regression. The scan compares symbol to symbol: a symbol the previous report did not hold is NEW (a body this batch added) and never a regression, however weak, and a symbol that dropped refuses loudly with the symbol name and both scores. The unit-average comparison survives only for a unit that did **not** grow, where no per-symbol row can name the loss (`report_regressions`).
* `verify` never commits. It writes the message to `.git/land_msg.txt` **only when every check passed**, and removes a stale one when it refuses; committing it stays a deliberate step for the rare manual case.
* `--no-outbox` and `--no-release` are **separate** opt-outs: skipping the outbox/branch checks does not skip the claim teardown (the old `--no-worker-units` did both, and a round that passed it left 18 worktrees and 3 dead claims behind). `--no-worker-units` remains as an alias for `--no-outbox`.
* **a released branch (case (a))** - `release --force` deletes `worker/<label>` and parks its only copy of the work at `refs/rescue/<label>`. The gate now restores the branch from that ref automatically (`restore_rescued_branch`) instead of refusing a batch whose work is demonstrably preserved, and names what it did;
* **an already-applied batch (case (b))** - if `record-base` ran *after* the batch was applied, the snapshot it took recorded the batch's own edits as pre-existing, so `land_stageable` excludes them as foreign work. When every changed path is in that snapshot, `land` says plainly that the batch is already applied and the ordering was wrong; `--already-applied` stages those paths anyway, so the batch lands without a hand commit. The ordering to prefer is still `record-base` on a clean tree *before* applying the batch.

## Lib dependencies

findings, git, lanes, project, proc, repo, report (the per-module list is `landing.md`).

## Test contract

Tier: fixture. `tools/tests/units/test_land.py` (the re-homed in-file selftest, 467 checks; `land.py --selftest`
forwards to it and to `tools/tests/units/landing/`) and `tools/tests/units/landing/test_gate_golden.py` (the table on
eleven fixture batches, against `gate_golden.py` recorded from the monolith). The live-tree comparison (the same rows
on replayed landings) is a measurement in the WP4 report, not a suite test: a live `verify` runs the lint and the
suite.

## Known gaps

the gate shells out to flipcheck/stylelint/ledger/selftest/commitlint and parses their output (the tools have no
row API yet); `linkorder` is not a row; `verify --json` is accepted and ignored; `land --branch` (with `resolve`) is
the one landing path (questions.md 2, ruled)

## History (the incidents behind the rules - keep the rule, drop the narrative when the rule is stable)

* deletes `build/RMHE08/ok` (and `main.elf` when the batch flips an object) **before** the run and requires them to be recreated;
* **compiles the batch's own units** (`ninja -k 0`, scoped to `build/RMHE08/src/<unit>.o` for each `--units` entry) before the DOL check proves anything: a `NonMatching` unit's object is never linked, so `ok` stayed green **twice in one session** (2026-09-26) with a unit in the tree that did not compile - once a partial file from a malformed cherry-pick, once a declaration moved out from under three call sites. `ok` answers "is the DOL still the DOL"; this answers "does our source still compile". A **foreign** dirty object that fails is named and tolerated - it is not this batch's defect, and the scoping is by object target, so the check cannot make a passing gate fail for another stream's work;
* **refuses a unit registered in name only** (2026-09-26): a source file can be committed while its `configure.py` `Object(...)` line and its `splits.txt` block are left behind, so the unit is absent from the build while `ok` stays green - and the compile gate cannot see it either, because a unit with no `configure.py` line has no `build/RMHE08/src/<unit>.o` target to scope to. Every batch unit must carry all three: an `Object(...)` line, a `splits.txt` block, and an object target in the build graph. `tools/units/verifyunit.py` owns the check;
* **re-measures per symbol instead of trusting `report.json`** (2026-09-26): the regression scan reads the same report the batch was measured against, so a stale or wrong report is invisible to it. The gate re-runs `objdiff report generate` over the objects, compares symbol-for-symbol against the report, checks a 100 % claim against the raw bytes, and reproduces the unit's `fuzzy_match_percent` from its listed partials (a function with no `fuzzy_match_percent` key is **0 %**, not 100 %) - the independent ordering the merger lane used, not the report's own consumer;
* **refuses a split target object that moved for a unit the batch does not name**: target objects come from the DOL split, so a `splits.txt` change that re-ranges a neighbour moves that neighbour's object. The gate hashes the registered units' split targets before and after the build (`.pi/notes/8031a6c0-fn-8031a6c0-e199.md` is the standard), rather than assuming the re-range harmless;
* checks every command's exit code, `configure.py`'s included - a failed `configure.py` leaves a stale `build.ninja` and every later number is a fiction;
* **refuses a batch that moves the ground truth, that moved `main` since the batch base, that touches a file outside the batch's expected set, or whose unit branch carries no commits of its own** (an outbox entry that does not validate is a warning since 2026-10-05); `config/RMHE08/config.yml` is judged by **content**, not by path: a change of its relocation-analysis keys only (`block_relocations`/`add_relocations`, decided by `lib.repo.config_change` - the pre-commit hook's own rule, read as `landing.common.config_verdict`) belongs to the batch and is staged with it, any other key refuses at pre-flight with the hook's reason, and `build.sha1` stays outside every batch (2026-10-04: the L2 round-2 batch carrying `block_relocations` had to land in two commits); only the batch's unit-shaped entries are looked up in the outbox (`unit_rows`) - a `--units` entry that names a staged *path* (a header, `LICENSE`, `docs`) has no outbox of its own and must not be validated as if it were a unit (2026-09-28, a header named in `--units` was demanded `residual`/`flags_probed` and bounced the whole landing); a foreign path **already in the tree** is reported - with a likely cause when it looks like lane scratch (`.tmp-*`, `.ws-*`, an `upstream/` clone) - **before** the expensive gate runs (`preflight_foreign`), not only as the refusal afterwards, so a mis-launched lane's leftovers cost a second, not a 5-minute build;
* **checks rule 10 like every other rule** (`vtableaudit.py`): a table of code pointers inside a unit's own ranges must be compiler output, so the row is add-only - exactly like the lint's `--diff`, because the tree already carries violations - and it PRINTS the violation set for the batch's units even when it passes. The rule used to be a "landing-review rule" (a habit), and `Network/fn_803D3CE8.cpp`'s two `self->vtable = &NetworkSessionManagerVTable;` writes survived a landing through it (2026-09-27); a silent pass is what that classification bought, so a silent pass is gone;
* checks rule 12 in the style lint, with a recorded allowance (`--allow-rule12 <token>`) for a lane that needs an unowned-data `extern` **now** while the claim is already scheduled: repeatable, printed in the landing log with the token it excused, and any rule-12 addition the allowance does not name - or any other rule at all - still refuses. Like rule 10's, it is a command-line record, never a key in a file;
* runs the style lint when it exists (7.21) and reports the ledger delta (the 7.10 knowledge-delta warning was row 20, deleted 2026-10-05; `prepcommit.py` still carries it); the lint row carries the **head** of stylelint's output - where the findings are - and never its trailing "not enforced: ..." legend, which on 2026-09-25 made a FAIL row read as a pass (`.pi/land.log`: `FAIL style lint (§6.5) adds no violation - (temporary grandfather: legacy scaffolding with bodies, ...)`);
* runs `tools/selftest.py` - the one runner for every tool's `--selftest` and every `*_selftest.py`, parked pre-existing failures aside - as the "all tool selftests pass" row, so a tool's own test that nothing runs cannot hide (the `measure_selftest.py` was red for weeks while 31 lanes filed "recompile.py is broken" incident); `--no-selftests` is the fast path.
* refreshes the baseline afterwards (7.16), so `ninja changes` compares against the batch that just landed;
* and **releases the claim of every unit it just gated** (owner's rule, "Teardown is part of landing"): a landed unit must not leave a worktree, a merged branch or a registry entry behind. A release that is incomplete (a live pane, a worktree that would not go) fails the gate and names what held it; `--no-release` turns the step off for an orchestrator-only batch.
* `resolve` is the land path's one automatic conflict resolution. Eight of thirteen live branches conflict with `main` on a single, safe class: sibling bands register *adjacent address ranges*, so their `config/RMHE08/splits.txt` blocks and `configure.py` `Object(...)` lines append at the same anchor. That is an add/add conflict whose resolution is the pure append-union - and `resolve` only ever touches those two paths. It runs in the branch's worktree or a **scratch** one, **never in MAIN** (a gate applying a diff inside MAIN is what left MAIN conflicted on 2026-09-26). The union is gated in order by the path scope, `unionguard` (a delete, a rename, or both sides editing one region is refused by name) and the `unionresolve` invariants (no duplicate unit key, no duplicate `Object()` line, no overlapping `.text`/`extab`/`extabindex` range, and no registration `main` already had is dropped); the union is computed in memory and the invariants asserted *before* anything is written. Staging is explicit (`git add -- <the two paths>`), never `git add -A`. The helper branch that parks the union (`land/resolve-<slug>-<pid>`, in the scratch worktree) is deleted by the landing that lands the branch - visibly, and only when the helper's tip is provably contained by the branch or by `main`; one carrying a hand fix the branch never took is refused loudly and left alone, because it may be the only copy (two helpers outlived their batches on 2026-09-26).
* `land` is the one command and the one you should use: it runs `verify`, stages the batch's own files, commits **with a pathspec** (`git commit -F msg -- <paths>`, so the whole index is never taken), releases the claims, and prints a **single answer line** (`LANDED ...` / `REFUSED ...`) whose exit status is the answer. The gate log goes to stderr, so piping stdout cannot lose the verdict - and a failed gate can never reach `git commit` (the old flow wrote the message unconditionally, which is how a piped `| tail -3` committed a refused batch twice). The pathspec is the other half of that safety: without it, another stream's *staged* edit was swept into the batch's commit twice on 2026-09-23 (`85f3d4b5`, `d50fdd32` took `tools/units/langcheck.py`). Paths the index holds but the batch does not are left staged and named in a warning.
* The pathspec still takes every *allowed* dirty path, so an unrelated edit that was already sitting in the working tree rode the next commit twice on 2026-09-24 (a prepared `docs/plan.md` under `85ddd7b6`, a `src/RSO/runtime.c` header under `890631e8`). `record-base` now snapshots the paths already dirty when the batch opens (`dirty_at_base`), and `land_stageable` excludes one unless the batch names it as a unit - the snapshot is the only way to tell "already dirty at the base" from "dirty because of this batch".
* Every check is classified by **KIND**, and a refusal says which kind failed, because the two need opposite responses. A **GATE** check refuses because the *batch* is bad - the style lint, the regression scan, the build/DOL hash, a path outside the batch, a genuine claim conflict - so the landing must stop and the batch must be fixed. A **BOOKKEEPING** check refuses because the *landing's own state* is stale while the batch is fine - a base that was never recorded (or has moved), a worker branch a `--force` release parked at a rescue ref, a claim whose teardown did not finish - and the refusal prints the exact remedy and **never** says "the gate failed". On 2026-09-26 the old shared wording ("the gate failed - nothing staged or committed") made a bookkeeping refusal read as a substantive gate failure, and the reader nearly overrode a real gate failure on another batch; the kind is now in the refusal line and in every per-check line (`<check> [GATE|BOOKKEEPING]: <what it printed> (remedy: ...)`).
* Two bookkeeping states that cost a manual commit the same day have their own handling:
