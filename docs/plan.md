# Campaign plan: from 217 matched functions to a byte-identical `main.dol`

**This file is the contract.** The orchestrator follows it as written — 100 %, not "mostly" — and the end goal is
not "a lot of matched functions": it is **`main.dol` rebuilt from `src/` alone, byte-identical to
`config/RMHE08/build.sha1`** (`BF4850739478CAAEDFE675949EB7C28595A7FDE9`), with every function symbol in
`symbols.txt` belonging to a named translation unit that has source.

Version 2.3 (2026-09-22), after **three rounds of subagent peer review** (findings kept in
`.pi/reviews/plan-r{1,2,3}-*.md`). Round 1 corrected the flip candidates, the per-module numbers, the overloaded
word "batch", the worktree measurement gap and the tools that do not exist yet; round 2 added the ground-truth
guard, the verify-then-commit order, the submodule trap and the honest size of the campaign; round 3 fixed the
remaining contradictions (the `.ninja_log` proof, the completion test, section 12, the cross-references).

---

## 1. The end goal, and what "done" means

| level | done means |
| --- | --- |
| a **symbol** | attributed to its unit, decompiled, measured ≥ 80 % per-symbol `fuzzy_match_percent`, committed |
| a **unit** | every symbol it owns is closed or explicitly `partial` with its residual in the file header; its ranges measure no worse than before; its **object** is byte-identical if it is to be flipped |
| a **batch** | every symbol it set out to close is closed (or recorded `partial`), the ranges it claimed measure no worse, its **knowledge delta** is written, `ninja build/RMHE08/ok` is green, and it is committed |
| the **campaign** | (1) `python tools/units/ledger.py` reports **no unclaimed *function*** - the ledger counts functions, and the TRK vector table of §10 item 2 is a label that stays unowned by decision; (2) **no `auto/*` placeholder unit remains** - every unit is named from evidence or is listed as a named exception in §10 with a reason; and (2b) **no unit's source has zero scored symbols** — a header-only stub is a note, not a unit, so `ledger.py` must show every unit contributing at least one measured function; (3) `rm -rf build/RMHE08 && python configure.py && ninja build/RMHE08/ok` passes **from a clean build tree**, so no stale split object can be carrying original bytes; (4) the DOL hash equals `config/RMHE08/build.sha1`, and that file's own sha1 still matches the value in `AGENTS.md` (it is ground truth and is never edited - §8.1) |

**Why clause (3) is not paranoia.** Today `ninja build/RMHE08/ok` is green while 65 128 of 5 437 392 bytes are matched, because the 13 565 `auto_*` objects carry the original DOL bytes. A green `ok` therefore proves *nothing* about our source until the tree is rebuilt from scratch - and that is exactly the check the campaign must end on.

Two rules keep the bar honest and are **repository** rules, not campaign policy (non-negotiables 3 and 4):

* `Matching` means **byte-identical** — every symbol 100 %, sections and relocations equal, proven per the
  `objdiff-verify` skill. A unit closed at 80 % stays `Object(NonMatching, …)`, its bytes are not linked, and
  that is what keeps `ok` green for the whole campaign.
* The **80 % bar closes a symbol**; it never changes what "matching" means.

The last mile is a *linking* campaign: units flip to `Matching` one at a time, each flip verified alone, because
the DOL hash only starts depending on our objects at the first flip. **Start that early** (§7.6) — the linking
questions (`.ctors$10` placement, pool sharing across units, section order, `extab` pairing) are cheapest to find
while the unit count is small.

**A flip needs a byte-identical *object*, not a byte-identical library.** `Runtime.PPCEABI.H` is 19 of its 20
symbols at 100 % — `Gecko_ExceptionPPC.cp`'s `__register_fragment` is 93.68 % (a loop-preheader scheduling
tie-break, §10 item 6) — so the flip candidates are the six **objects** that are byte-identical: `memset.c`,
`memcpy.c`, `__start.c`, `__ppc_eabi_init.cpp`, `__init_cpp_exceptions.cpp`, `global_destructor_chain.c`. Each is
proven per `objdiff-verify` before it is flipped. Gecko's object is not a candidate until that residual is settled.

### 1.1 The phases, and the honest size of this

The campaign is not one loop; it is four phases with different economics, and confusing them is how a plan misallocates its effort:

| phase | what it does | cost per unit of progress | measured so far |
| --- | --- | --- | --- |
| **A. attribute** | claim ranges, name units, register, write stub sources | ~1 min per unit, no source work | 19 units, 295 functions |
| **B. decompile** | write bodies to >= 80 % | a worker round (10-40 functions) per 4 workers | 60 functions from a 7-worker round, before the 4-worker cap existed |
| **C. second pass** | push the 80-99.9 % band to 100 % | the playbook's idea list, per unit | 67 symbols sit in that band today, ~26 % of each batch's output |
| **D. flip** | prove byte-identity per object, link it, keep `ok` green | minutes per object, but the DOL hash starts depending on us | 0 objects flipped |

**The honest arithmetic.** 20 224 unclaimed functions at 10-40 functions per 4-worker round is **500-2 000 worker-rounds**; the 284 closed so far are the *easiest* 1.2 % of the bytes. The plan's job is therefore not to promise a date - it is to keep each round cheap (the protocol), to keep the method compounding (the knowledge delta) and to make attribution, which is mechanical, run at machine speed. **Phases C and D are on the critical path for the DOL**, not optional polish: a unit with any residual cannot flip, and the DOL cannot be byte-identical until every unit can.

## 2. Where we are (measured, 2026-09-22, commit `9d6b351`)

| | |
| --- | --- |
| symbol map | **20 519 functions**, 45 176 data symbols (22 418 of them `extab`/`extabindex`/`.ctors`/`.dtors` fragments that travel with their code unit) |
| attributed | **295 functions in 19 registered units** (19 configured, 13 584 split objects) |
| closed | **284 ≥ 80 %** (objdiff counts **217 matched**), 65 128 of 5 437 392 `.text` bytes (1.2 %) |
| unclaimed | **20 224 functions** |
| per module | `Runtime.PPCEABI.H` 19 of 20 symbols at 100 % (`__register_fragment` 93.68 %); `Pl` 3 units, 150+ of 189 closed (`pl_master` 99.99 %, `pl_skill` 96.07 %, `pl_act` 93.75 %); `main.cpp` 47 functions, 37 at 100 %; `sys_mem.cpp` complete; `auto/80040598_fn_80040598` 97.19 %; `Camellia` 99.97 % and `RSO` 99.71 % (both with named residuals), `g3d`, `OS`, `Network` at 100 % |
| flags landed | `cflags_main` (`-O3 -inline noauto`), `cflags_pl` (`-O3 -inline noauto -opt nopeephole -Cpp_exceptions on`), `cflags_ppceabi` (`cflags_runtime` + `-func_align 4`) — each with its instruction-level evidence in `configure.py` |
| tooling that exists | `ledger.py`, `attribute.py` (+ selftest), `symbolpreflight.py`, `tudiscover.py`, `dump_asm.py`, `m2cinput.py` (+ selftest), `symedit.py`, `symdiff.py`, `mt.py`, `prepcommit.py`, `localonly.py`, `tools/m2c` (submodule) |

Everything above is *derived*, never remembered: `python tools/units/ledger.py` reads `symbols.txt` (through
`symedit.py`), `splits.txt`, `configure.py` and `build/RMHE08/report.json`. A stale `report.json` lies — the
ledger warns when it predates `splits.txt`/`configure.py`, and regenerating costs 1.5–3.5 s.

## 3. What a batch costs — and the batching law

Per **edge**, from `.ninja_log` (the first draft of the review used wall clocks taken under contention from a
second stream; these are the measured per-edge costs, and `docs/build-performance.md` has the breakdown):

| edge | measured | why it matters |
| --- | --- | --- |
| `dtk dol split` (`build/RMHE08/config.json`) | **16.8 / 16.8 / 27.1 / 46.5 s**; 200–400 s if `write_asm: true` | re-runs on *any* edit to `symbols.txt`/`splits.txt`/the DOL — it is an input of itself, so renames ride the batch |
| the link (`main.elf`) | **66.8 – 130.9 s** over 13 584 objects | the largest edge, paid once per batch whether one object changed or fifty |
| one source unit (`mwcceppc`) | **0.1 – 24 s** (our own log shows 0.13–1.20 s for the units rebuilt lately; `Pl/pl_skill.o` is the worst measured) | what a worker pays per iteration — and a worker pays *no* split and *no* link |
| `report.json` / `baseline.json` | 1.5 – 3.5 s | cheap enough to regenerate before believing anything |
| `main.dol` + `ok` | < 0.5 s | the real test, effectively free |
| the asm dump (`python tools/splits/dump_asm.py`) | **200 - 400 s** | not a build step (`write_asm: false`), but `tudiscover` reads it and **every batch's own `splits.txt` edit invalidates it** - which is a second, independent reason to batch attribution: run it once per attribution batch, check it with `--check` |

**Three different things get called "batch" — keep them apart:**

| term | what it is | size |
| --- | --- | --- |
| **registration batch** | the ranges and `configure.py` entries claimed in one go (one split, one ledger check) | **≤ 0.5 MB of `.text`** (owner's call) — ≈ 2 000 functions, a blast-radius ceiling, not a target |
| **work round** | what one group of ≤ 4 workers does in parallel | 10–40 functions, bounded by worker throughput |
| **land batch** | everything that goes into one commit on `main` (one split, one link, one `ok`) | one or more work rounds, plus the renames/merges that ride them |

**The law, in five clauses:**

1. **One split and one link per land batch.** A batch boundary costs ~1.5–2.5 min of machine time; a rename-only
   batch that changes no object is ~20 s.
2. **Cap a registration batch at 0.5 MB of `.text`** (owner's call) — it binds the *attribution* pass, where one
   command can claim a whole region. Until `attribute.py` grows `--max-total-bytes` (§7.14) the cap is enforced
   with `--limit N`, using the byte total `attribute.py plan` prints.
3. **Four workers to one orchestrator** (owner's call). The machine already has the parallelism (4 processes);
   what it lacks without §5 is an interface. My verification time, not the machine's, is the scarce resource.
4. **Renames, phantom merges and range claims ride the same batch** as the source work, because each is a split
   dirty-check input. `symedit.py rename-batch <file>` collects renames; verify every renamed symbol *after* the
   batch's split, before the commit.
5. **Source-only changes never wait for a split** — they ride the next one for free, and an already-registered
   unit is pure source work (`main.cpp` was registered once and filled over three batches).

## 4. Roles and responsibilities

> **Tool status — read this before following §4–§6 literally.** Sections 4–6 describe the **target** state. Of the
> tools they name, `recompile.py`, `claims.py`, `brief.py`, `handoff.py` and `land.py` **do not exist yet**
> (§7.1–7.5, first in §12). Until they do, the orchestrator runs their *commands* by hand — that is what §11's
> checklist is — and workers get their briefs as inline text with the same six parts (§5.2) instead of as a file.
> Everything else these sections name already exists.

**The four shared files (`config/RMHE08/splits.txt`, `configure.py`, `config/RMHE08/symbols.txt`, `AGENTS.md`)
have exactly one writer: the orchestrator.**

| responsibility | orchestrator (me) | worker (external `pi` agent) | owner | tooling |
| --- | --- | --- | --- | --- |
| pick the batch (§12), order, size cap | **owns** | — | — | `ledger.py next`, `attribute.py plan` |
| attribute: ranges, unit name, `configure.py` entry | **owns** (one writer) | proposes only | — | `symbolpreflight.py`, `tudiscover.py`, `attribute.py apply` |
| write a unit's source | reviews, integrates | **owns one unit, one file, one worktree** | — | `m2cinput.py`+`m2c`, Ghidra dump, `DumpSymbols.map` |
| measure a symbol | **re-measures every claim** | measures its own unit | — | `recompile.py`, `mt.py diff/info`, `symdiff.py` |
| land a compiler flag | **owns** — decides, and the decision needs §8.2's evidence | probes, reports numbers; may put a **one-unit** deviation in the source as a pragma (it owns that file) | — | the `configure.py` comment is the evidence; the pragma is the exception |
| claim a data range | **owns**, measured before/after | proposes with numbers | — | `attribute.py` data pass, playbook 23 |
| run `ninja` / the split / the link / `ok` | **sole runner** | **never** (uses `recompile.py`) | — | `land.py verify` |
| git: branches, merges, conflicts | **owns** (cherry-pick to `main`) | commits only on its branch | — | `git worktree`, `git cherry-pick` |
| commit on `main` | **owns** | never | grants standing approval | `prepcommit.py` |
| knowledge delta (playbook row, skill line, unit header) | **owns the gate** | reports what it learned | — | `sync_reference.py --check` |
| type and naming discipline (§6.5) | **owns the gate**: lints every branch at the land step and refuses one that adds a violation | **writes to the rules** — its brief embeds them verbatim | — | `stylelint.py` (7.21) |
| decisions only the owner can make | escalates, does not guess | — | **owns** | §10 |
| DOL hash safety | **owns** | cannot touch it (branch, no `Matching`) | — | `prepcommit.py`, `land.py` |
| keep `main` free of worker commits | **owns**: the batch records `main`'s HEAD as its base, and a moved HEAD before the land step aborts the batch and resets `main` to that base (nothing was pushed) | must not commit, rebase or amend on `main` | — | the base sha in the brief, `land.py verify` |

**What the orchestrator must not do:** read source files and disassembly as its default mode. It reads the
ledger, worker digests, `git diff --stat` and numbers; every read-heavy question is cheaper as a worker's answer.
It is the *librarian* of the compounding asset (§9), not the fastest decompiler.

## 5. The coordinator protocol (worktrees, branches, briefs, outbox)

Workers are separate processes in a tmux window (max 4). They inherit nothing from my context and I inherit
nothing from theirs, so **everything that used to be a prompt or a habit becomes a file with a format.**

### 5.1 The worktree layer — the branch *is* the claim

```sh
# orchestrator: claim = create the branch (fails loudly if it exists - atomic for free)
git worktree add -b worker/pl-act-13 ../mhtri-dtk.ws-pl-act main

# worker, inside its worktree: source only, commits on its own branch
tools/units/recompile.py main/Pl/pl_act        # direct compiler, no ninja, mtime asserted
python .agents/skills/mwcc-unit-matching/scripts/mt.py diff -u main/Pl/pl_act fn_8027C208
git commit --amend -am "Pl/pl_act: fn_8027C208 body"   # ONE commit per unit, on worker/pl-act-13

# orchestrator, on main: integrate one unit at a time, then verify the whole batch
git cherry-pick --no-commit worker/pl-act-13   # or <merge-base>..worker/pl-act-13 if it left several
python tools/units/land.py verify              # split + link + ok + regressions + ledger + delta
git commit -F .git/land_msg.txt                # only after verify passes; --abort otherwise
git worktree remove ../mhtri-dtk.ws-pl-act     # and delete the branch
```

* **No build tree per worktree — and the tooling must know that.** `build/` is ~1 GB and the split tree is a
  shared *read-only* input, so a worktree must not get a copy (4 GB and a re-split each). A fresh worktree has
  **no** `build.ninja`, `objdiff.json` or `build/RMHE08/`, and today's `unitutil` resolves all three from its own
  location — so a worker in a worktree cannot measure with `mt.py` as it stands. `recompile.py` therefore
  resolves `MAIN` with `git worktree list --porcelain` (first entry) and uses `MAIN/build/tools`,
  `MAIN/build/binutils`, `MAIN/build/compilers` and the **target** object `MAIN/build/RMHE08/obj/<unit>.o`;
  `mt.py` is given the target object and cflags explicitly (`--target`, `--flags`) instead of reading the main
  worktree's `build.ninja`. Roadmap 7.1 + 7.15 owns this; it is a **prerequisite for the first worktree round**.
* **The worker's object path is its own.** A worker writes `<worktree>/build/RMHE08/src/<unit>.o` and never
  anything under `MAIN/build/` — otherwise an unverified branch object could be read as `main`'s and a green `ok`
  would be meaningless. `recompile.py` prints the path it wrote and the target it compared against.
* **A worker's diff is never committed to `main` before it is verified.** `git cherry-pick --no-commit` stages it,
  `land.py verify` runs, and only then is the commit written; if verify fails, `git cherry-pick --abort` or
  `git reset --hard` restores `main` untouched (this is not "rewriting history": nothing was published).
  Verify-then-commit, never commit-then-verify.
* **`main` does not move while a batch is open.** The batch records `main`'s HEAD as its base; if HEAD has moved
  before the land step (a worker committed to `main` instead of its branch, or another stream landed), the batch
  aborts and `main` is reset to that base — recovery, not history rewriting, because nothing was pushed.
  `land.py verify` checks the base before it does anything else.
* **One commit per unit.** The brief requires a single commit for the unit (`git commit --amend` while
  iterating), because `git cherry-pick worker/<slug>` lands only the tip. If a worker leaves several commits,
  the orchestrator picks the range (`git cherry-pick <merge-base>..worker/<slug>`) rather than silently
  dropping all but the last.
* **Evidence lives in `MAIN`, not in the worktree.** `.pi/` is a directory inside each worktree, so an outbox or
  notes file written there is destroyed by `git worktree remove` (or blocks it as untracked). A worker writes
  `MAIN/.pi/outbox/<unit>.json` and `MAIN/.pi/notes/<unit>.md` — `MAIN` it already resolved — so the evidence
  outlives the worktree.
* **`tools/m2c` is a submodule - do not initialise it in a worktree.** `git worktree remove` refuses to remove a
  worktree that contains a checked-out submodule ("working trees containing submodules cannot be moved or
  removed"), which would leave the branch - the lock - alive forever. The brief points the worker at
  `MAIN/tools/m2c`, and `claims.py release` does `git worktree remove --force` + `git branch -D` (a cherry-picked
  branch is *not* "merged" from git's point of view, so `-d` refuses) + `git worktree prune`.
* **A worker resolves the unit from its own tree, the toolchain from `MAIN`.** `recompile.py` reads the source and
  writes the object in the **current worktree** (its `cwd`) and takes only the compiler, binutils and the *target*
  object from `MAIN`. Running `MAIN/tools/units/recompile.py` from a worktree must not silently compile
  `MAIN`'s source - the tool prints the source path it compiled and the object path it wrote, and they
  must both be inside the worktree.
* **A conflict is a protocol violation.** A worker edits only its unit's source; I own the shared files. So a
  conflict means someone touched what they must not: resolve by hand, **re-measure the unit**, record it.
* **A crashed worker is visible in git** — a stale worktree, a branch never merged, an uncommitted tree. That is
  a better failure signal than a timestamp in a claim file: I can inspect exactly what it had.
* **Batch shape is unchanged**: all four units land on `main` before `land.py` runs, so there is still one split
  and one link per land batch, and the merge step is where my verification gate sits.
* **Absolute paths:** the compiler and source paths in `build.ninja` are relative, but the file does name the
  Python interpreter absolutely (`C:\…\Python312\python.exe`) — which is why `MAIN` resolution matters and why
  `configure.py` is re-run only in `MAIN`.

**Spawning a worker (herdr).** Workers live in the session's **Worker tab** - never in the orchestrator's own
pane: a split of the orchestrator's pane drops four agents into the middle of its own work, and the round's
teardown then has to close the orchestrator's pane to release a worktree. Find the tab once, then one pane per
worker:

```sh
herdr tab list                                     # the tab labelled Worker, e.g. w1:t2
herdr pane split <a pane inside that tab> --cwd <the worker's worktree> --direction down
herdr agent start <name> --kind pi --pane <the new pane id>
herdr agent prompt <name> "<read your brief; ack first; you may fan out; then deliver>"
herdr agent wait <name> --until idle --until blocked
```

`--cwd` is what makes the rest work: the pane starts *inside* the worktree, so `recompile.py` resolves that
worktree from its cwd and MAIN's toolchain and target object from git - no junction, no environment variable.

**Teardown is part of the round.** After the handoff and after the integration: close the worker's pane
(`herdr pane close <pane id>`), **then** `claims.py release <unit>`. A live pane holds its worktree as its cwd
and Windows refuses to delete a directory a process is sitting in ("Device or resource busy", or
`git worktree remove` failing with "Permission denied"), so releasing first fails and leaves a directory that
nothing can remove until that pane goes away. If a worktree cannot be removed, ask who is sitting in it - it may
be the owner's own pane.

### 5.2 The worker's input — one generated file, nothing else

`tools/units/brief.py <unit>` writes `tools/units/briefs/<unit>.md`, containing:

1. the **unit**: path, lib, `mw_version`, the real cflags, object and target paths, the `.text` range;
2. the **inventory**: every symbol it owns, with size and current measured %;
3. the **residuals**: the unit header's existing table (what differs and why) — so a re-brief never re-derives
   settled work;
4. the **decided items**: flags landed for this lib, seams already settled, data ranges deliberately not claimed
   (§10) — so a worker does not re-litigate them;
5. the **task**: the functions to write, in address order;
6. the **rules**: the hard rules, the measurement loop, the evidence order, the flag policy (§8), and what it may
   not touch.

The brief's rule text (part 6) is **copied verbatim from §8 of this file** plus the skill that owns the step,
so there is one canonical source and no drift.

**If the brief does not say it, it is not a rule.** Spawning a worker is then one line: *read this brief, do it,
write your report.*

### 5.3 The worker's output — data, not prose

| # | artefact | consumed by |
| --- | --- | --- |
| 1 | the source it owns, compiled and measured, committed on its branch (one commit) | the build, the merge |
| 2 | `MAIN/.pi/outbox/<unit>.json` — per-symbol %, unit %, residual, **config requests** (range/rename/flag with evidence), **flag probes** (numbers + verdict), blockers, and the command it measured with | me and `land.py`, which can refuse a batch from it alone |
| 3 | `MAIN/.pi/notes/<unit>.md` — the full evidence trail | a later session, or a re-brief of the same unit |
| 4 | a ≤ 15-line digest in the reply | human review |
| 5 | the claim released (worktree removed, branch deleted after the merge) | other workers |

Why JSON and not prose: four workers writing four prose reports is exactly how the orchestrator ends up
reconciling numbers by hand — the failure mode this plan exists to remove.

### 5.4 Who runs what, and the failure modes

**Only the orchestrator runs bare `ninja`, the split, the link and `ok`.** Workers compile their own object
through `recompile.py` (direct compiler invocation, mtime asserted, section sizes printed) and measure with
`mt.py`. Otherwise four processes fight over one build tree, and a green `ok` can come from a stale link.

| failure | detected by | handling |
| --- | --- | --- |
| two workers, one unit | `git worktree add -b worker/<slug>` refuses | the loser takes the next unclaimed unit |
| a worker dies mid-unit | stale worktree / branch never merged / no outbox entry | inspect its tree, re-issue the brief; brief step 1 is "compile and measure the unit first", so a half-written source is caught |
| a worker edits a shared file | `land.py verify` diffs the tree against the expected file set | reject the merge, restore the file, re-brief; its other work survives |
| a worker measures a stale object | `recompile.py` asserts the mtime advanced | rerun; discard the numbers (this bit two workers before the helper existed) |
| a worker's number disagrees with the report | I re-measure every symbol I claim | my measurement wins; the difference is investigated, never averaged |
| a worker wants a flag nobody else agrees with | the flag rule (§8.2) | it stays a *probe* in the outbox until a second unit or a source pragma backs it |
| a merge conflict | `git cherry-pick` stops | protocol violation: resolve, re-measure, record |
| a worktree cannot resolve the toolchain/target | `recompile.py` fails with the missing path | fail loudly — never let a worker silently compile nothing |
| the unit is only partially matched | the outbox says so and its score is below `main`'s | **measure before merging**: worse than `main` → drop the branch and re-brief; better → merge, record the residual in the header, mark the unit `partial` in the ledger |
| `main` moved while the worker ran | the cherry-pick conflicts, or the worker's base is old | the worker rebases on `main` before handoff (`git rebase main`); the orchestrator re-measures after the cherry-pick regardless |
| a claim cannot be released | `git worktree remove` fails with `Permission denied`, or the directory gives "Device or resource busy" | a live pane is **sitting in** the worktree (its cwd *is* the worktree) and Windows refuses to delete a directory a process is in. Teardown is part of the round: `herdr pane close <pane>`, then `claims.py release <unit>` - and if that pane is the owner's, ask first, because a stale claim blocks the unit rather than losing anything |
| a claimed seam is wrong | the unit's functions will not match | revisit the seam while the unit is small — matching settles the boundary |

### 5.5 A worker may fan out subagents - under the same rules

A unit is often several independent functions, so a worker is expected to spawn its own subagents for parallel
work where that helps. Three rules make it safe, and they are part of the brief:

* **the subagents work in the worker's worktree, on the worker's branch** - one branch, **one commit**, made by
  the worker. A subagent never commits;
* **the worker assigns disjoint files or functions** (the one-writer-per-file rule applies inside a worker too)
  and is **accountable for everything its subagents produce**: it re-measures every claim they make, exactly as
  the orchestrator re-measures the worker's;
* **every subagent is handed §6.5 and §8 verbatim** (the brief's part 6). A subagent that has not read them
  will name a field `unk4`, reach it with a pointer cast and use a `goto` - and those are repair work for the
  next pass, charged to the worker that spawned it.

The worker's handoff covers its subagents' work as its own: `measured_with` says how the numbers were obtained,
and `residual` covers whatever they left unfinished.

### 5.6 Acknowledgement, heartbeats and timeouts

A terminal multiplexer cannot tell "finished" from "never started": both read as *idle*. In the first round with
four external workers that ambiguity cost forty minutes - one agent sat idle while the other ground through its
unit, and the orchestrator had no cheap way to see which was which. Three mechanisms remove it:

* **Acknowledge first.** A worker's first action, before it reads the target disassembly, is
  `python tools/units/claims.py ack <unit> --agent <name> --pane <pane>`, which writes
  `MAIN/.pi/ack/<slug>.json`: the branch, the agent, the pane and a timestamp. Spawning a worker and seeing no
  ack within two minutes is an unambiguous failure, and the claim can be reclaimed immediately.
* **Heartbeat per iteration.** The same command with `--progress <symbol>` after every function the worker
  measures. The `progress` list is the difference between "quiet because it is thinking" and "quiet because it
  stopped", and it costs one second per function.
* **Status and timeout.** `claims.py status` reports `unacked` / `stalled` / `working` / `done` per claim,
  combining the ack file with what cannot lie - the commits on the branch (`rev-list --count <base>..<branch>`)
  and the outbox's existence - and exits non-zero when anything is unhealthy. `claims.py timeout [<unit>]
  [--apply]` reclaims the unhealthy ones: **it copies the branch to `refs/rescue/<slug>` first**, then removes
  the worktree, deletes the branch and drops the claim, so a timed-out worker loses the lock but never its work.
  The unit is then free to be re-briefed, and the rescue ref is printed for whoever picks it up.

The orchestrator's rule of thumb: **an ack or an artefact, never a hunch.** A worker is progressing if its ack
says so or its branch has commits; if neither is true and the grace period has passed, it is timed out - not
asked again.

## 6. The loop - four steps, one gate each

> **A worker never runs `ninja`, the split, the link or `ok`** (§5.4). In this section the `ninja` commands are
> the orchestrator's; a worker's equivalent is `tools/units/recompile.py` (7.1).

| # | step | tool | gate before moving on | left behind |
| --- | --- | --- | --- | --- |
| 1 | **attribute** | `symbolpreflight.py`, `tudiscover.py`, `attribute.py` | the verdict is `proceed`, or `approve` with a written proposal the owner accepted (§10) | `splits.txt` ranges + `configure.py` entry + the unit's source (mandatory) |
| 2 | **decompile** | the Ghidra dump (`docs/memory-dump.md`), `DumpSymbols.map`, `m2cinput.py` → `tools/m2c` | it compiles, and the disassembly agrees with every instruction-level decision | `src/<Dir>/<file>.c` (+ header beside it) |
| 3 | **match** | `recompile.py`, `mt.py diff/info`, `ninja changes`, `report.json` | the symbol's own score ≥ 80, nothing else regressed, `ok` green | the residual in the unit's file header |
| 4 | **commit** | `land.py verify` + `prepcommit.py` | the batch's knowledge delta is written and `sync_reference.py --check` is clean | one commit per unit, the ledger moves |

### 6.1 Attribute

```sh
python tools/units/symbolpreflight.py <address|name>   # owner, collision verdict, registration drafts
python tools/splits/tudiscover.py at <address|name>    # TU boundary proposal for an unowned address
python tools/units/attribute.py plan <start> <end>     # bulk: one proposal per maximal unclaimed run
python tools/units/attribute.py apply <start> <end>    # ranges + configure entry + stub source
#   `apply` writes `.text` only; the data runs it saw go into splits.txt as comments for the
#   measured second pass. `plan` prints them; `apply` does not.
```

* **A source file is created with its functions, not just a header.** A registered unit whose source does not
  exist is a bug (the build warns `Missing source file`); a file that carries only a header is a note, and its
  header must name where the inventory lives (`ledger.py unit <path>`), never copy a function list.
* **Evidence-first partition.** `attribute.py` cuts only at seams a narrow (≤ 4 cuts) *strong* observation pins;
  a piece under `--min-bytes` joins its neighbour, one over `--max-bytes` is split and flagged as a guess. A
  region with no evidence stays **one** unit whose header says the seam is unproven — one function per file is
  certainly wrong, one file per region is only unproven.
* **Confidence is recorded, not hidden**, and a seam may only be claimed from `tudiscover`'s evidence kinds; a
  shared static or a call pattern is a *hint* for the header, not a seam.
* **Data is a second pass, measured - and it needs a queue, not comments.** `apply` currently writes the data runs
  it saw into `splits.txt` as comments, which nothing reads. Roadmap 7.17 turns that into
  `tools/units/data-queue.json` (`{unit, section, start, end, labels, leak, density, verdict}`), which
  `dataclaim.py` consumes and `land.py` reports. A range our object does not emit must **not** be claimed
  (playbook 23): two of this campaign's three data decisions were "do not claim", and both would have cost
  score.
* **`apply` must be transactional and never overlap.** Today it writes in place and only skips a unit that is already
  in `splits.txt`, so a crash between the two files leaves a half-registration — and `plan` over a whole `.text`
  produces 23 overlapping data spans today. Roadmap **7.20** fixes it: validate every proposal (no range
  overlaps a claimed one, no unit repeated), write `configure.py` and `splits.txt` through temp files and rename
  them, and restore both on any failure. Until then the recovery is
  `git checkout config/RMHE08/splits.txt configure.py` and re-running `plan`.
* **Run `python tools/splits/dump_asm.py --check` before `plan`/`apply`.** `tudiscover` reads the asm dump, and a
  stale dump is silent — it zeroes a codegen fingerprint and turns a seam proposal into a guess. The dump is
  invalidated by *any* edit to `symbols.txt`/`splits.txt`, including the batch's own.
* **Never claim linker-generated data** (`_rom_copy_info`, `_bss_init_info` — MW ld emits them; the DOL
  reproduces them byte-identically), and check whose `extab`/`extabindex` fragment it is before claiming one.

### 6.2 Decompile

```sh
python tools/units/m2cinput.py build/RMHE08/obj/<unit>.o -f <symbol> -o build/tmp/<symbol>.s
python tools/m2c/m2c.py -t ppc-mwcc-c --no-cache -f <symbol> build/tmp/<symbol>.s
```

The disassembly is the arbiter; both decompilers are shapes, neither is codegen evidence. A rename is
`symedit.py` **plus the source in the same edit**. The unit's file header is written on the commit that creates
the file: what it is, its range and function order, where its flags/evidence live, the residual — never a
per-function inventory and never percentages.

### 6.3 Match

```sh
python tools/units/recompile.py <unit>              # compile alone, no ninja, mtime asserted
python .agents/skills/mwcc-unit-matching/scripts/mt.py diff -u <unit> <symbol>
python tools/units/land.py verify                   # the batch gate: split, link, ok, regressions, ledger
```

The bar, all of it: (1) the symbol's own `fuzzy_match_percent` ≥ 80 — a function entry **without** the key is
0 %, and `complete_code_percent` is not a score (it has read 100 % next to 1.77 %); (2) no other symbol
regressed; (3) `ok` green; (4) the object measured came from the **real command line**, not a hand-written
compile; (5) the residual is in the unit header.

**"Re-measure the unit" means exactly this:** run `recompile.py <unit>` (or `ninja build/RMHE08/src/<unit>.o`
when I am the one at the keyboard), then `mt.py info -u <unit>` and `mt.py diff -u <unit> <symbol>` for every
symbol the batch claims, and compare against the numbers in the worker's outbox — a difference is investigated,
never averaged.

Below 80 %: land it only if it measurably improves the unit and regresses nothing, mark it `partial`, and put
the residual in the header — the `AGENTS.md` playbook rows are that second pass's todo list.

### 6.4 Commit

One commit per unit (or per batch registered together), area-prefixed and imperative, carrying the sources, the
registration, the flags it proved and the knowledge delta. `prepcommit.py` stages explicit paths only, refuses
build/original/scratch output, verifies the DOL SHA-1, and handles the `AGENTS.md` LOCAL-ONLY block
(non-negotiable 8). The local-only `## Current task / plan` block is updated **in the same commit**. History is
never rewritten; nothing is ever pushed.

### 6.5 Type and naming discipline — mandatory in phases B and C

Seven rules, all of them checkable. They are **not style preferences**: a wrong or unnamed type is what makes
the *next* function in the same unit cost twice as much, and pointer arithmetic hides exactly the layout that
rules 3 and 4 exist to record.

| # | rule | what it means concretely |
| --- | --- | --- |
| 1 | **A shared type lives in one header** | a type more than one unit uses is defined **once** (under `include/`, or beside its owner and included) and *included* where needed — never copied. The existing convention applies: a declaration moves to `include/` the *second* time a unit needs it, never the first |
| 2 | **An extern lives with the TU that owns the symbol** | a function or variable declared `extern` belongs in the source or header of the translation unit that **defines** it, and consumers include that. Re-declaring someone else's symbol in your own file "to save an include" is forbidden |
| 3 | **A reconstructed class/struct states its size** | every reconstructed type carries `/* size: 0xNN */`, traced from the evidence (allocations, `memset`/`memcpy` lengths, the object's `.data`/`.rel` records, the runtime dump). An approximation is allowed **only** if it is marked as one |
| 4 | **Every field carries its offset** | `/* +0x1C */` on the field, in ascending order, so the layout is readable at a glance and a reviewer can check it against the disassembly |
| 5 | **Every field has a name from its context** | what is stored, compared against, passed on. The **only** exception is a padding or unused field — present in the original object but untouched by the functions we match — which gets `pad_0xNN` / `unused_0xNN` **and keeps its offset** |
| 6 | **Pointer arithmetic to reach a field is forbidden** | `*(u32*)((u8*)self + 0x1C) = v;` is not acceptable; declare the type and write `self->field = v;`. A raw byte offset is allowed only where no field is being named (`memset`, a byte-wise copy, a `sizeof`/offset computation) — and even there prefer `offsetof(Type, field)` |
| 7 | **Symbols have proper names** | a function that arrives as `fn_XXXXXXXX` gets a name for **what it does** plus the naming scheme of its neighbours; a variable or field that arrives as `unkNN` gets a name for **what it holds** and where it is used. Neither `fn_XXXXXXXX` nor `unkNN` may survive in `src/` |
| 8 | **`goto` is forbidden** | No `goto`, and no label used as a control-flow device. Where a shared tail or a dispatch layout looks like it needs one, the conformant shapes are a **helper function**, a `switch` whose cases share a `break`, or a `for (;;)` with `break`/`continue` - and if none of them reproduces the target's codegen, that is a **residual to record with both measurements**, not a licence to use `goto`. The rule exists because the shape is unreadable in isolation (the target of a jump can be a hundred lines away) and it defeats the point of a reconstruction that someone has to read |

**These rules are part of phase C, not a separate chore.** The residual sweep already revisits every unit that
is not byte-identical; the conformance work (rules 1-8) rides the same pass, unit by unit, in address order.

**The `goto` backlog from the first protocol round (2026-09-23).** Four functions reached 100 % with a `goto`
shape before rule 8 existed: `Pl/pl_act`'s `fn_80278144` and `fn_80278310` (`goto ret1; ret0: return 0;`) and
`fn_8027BC48` (`switch` + `goto`), and `Pl/pl_skill`'s `fn_80271BD4`/`fn_80271E0C` (label dispatch). They are the
lint's first reported entries: each needs a conformant shape that keeps the score, or a recorded residual with
the measurement that shows what the conformant shapes score. Their unit headers carry the shapes that were
tried.

**Enforcement is a tool, not a promise.** `tools/units/stylelint.py` (roadmap 7.21) reports each rule with
`file:line`, per unit and as a backlog, and **`land.py verify` refuses a batch that adds a violation** — a rule
enforced by remembering is not a rule. The rules apply to new work immediately; existing units are brought into
conformance as they are touched. The audit of 2026-09-22 says how much there is to bring:

| measure | count | rule |
| --- | --- | --- |
| auto-generated symbol names used in `src/` | **376** | 7 |
| pointer-arithmetic field accesses | **237** | 6 |
| fields still named `unk*` | **320** | 5 |
| struct fields that already carry an offset annotation | 52 | 4 (partial) |
| struct/class size annotations | **0** | 3 |
| types defined in more than one unit's source | `Vec3` (and the `_PLW` family) | 1 |

A batch that touches a unit closes its rows in that table for that unit. The lint's backlog number is the
campaign's second burn-down (§7.11 is the first, bytes).

## 7. The tooling roadmap — build order, why, and the acceptance test

The review's accepted items, in the order that makes each one safe. Everything is `NonMatching`-safe by
construction: a tool can waste time, it cannot break the link.

| # | tool | why it exists (incident) | acceptance test | size |
| --- | --- | --- | --- | --- |
| 7.1 | `tools/units/recompile.py` (+ 7.15) | two workers measured objects the compiler never rewrote (1-second mtime granularity); a worker must not run `ninja` | delete the object, compile **without ninja**, assert the mtime moved, print section sizes and both object paths; resolve `MAIN` for the toolchain and the target object | ~60 |
| 7.2 | `tools/units/claims.py` | two independent processes must never take one unit; the branch is the lock, and a silent worker must be reclaimable (see the ack/heartbeat/timeout layer, §5.6) | `claim` creates the worktree+branch or refuses; `ack`/`status`/`timeout` cover liveness and reclaim (a timed-out branch is rescued to `refs/rescue/<slug>` first); `list` shows owner/age; `remove` cleans up | ~140 |
| 7.3 | `tools/units/brief.py` | every fan-out cost a hand-written 40-line brief and each drifted | one file per unit with §5.2's six parts; idempotent | ~120 |
| 7.4 | `tools/units/handoff.py` | worker replies were inconsistently shaped; detail was lost to truncation | prints the digest skeleton; validates an outbox entry against the schema | ~60 |
| 7.5 | `tools/units/land.py` | the batch checklist was six manual commands and the regression scan was rewritten four times; a green `ok` can come from a stale link | `verify` **deletes `build/RMHE08/ok` (and `main.elf` when the batch flips an object) before the run and requires both to be recreated**, then runs configure → split → report → regressions → `ok` → ledger delta → knowledge-delta check; refuses on a shared-file edit or an outbox violation; owns the baseline (7.16). The `.ninja_log` ordering idea does not work: a `NonMatching` batch never relinks, so `main.elf` never runs | ~240 |
| 7.6 | **first `Matching` flip** | `0 / 5 files linked` today; every linking question is untested and gets more expensive with every unit | one byte-identical **object** (§1) flipped alone in its commit, `ok` green, then the next | — |
| 7.7 | `tools/symbols/dumpmap.py` | 48 367 real names/signatures sit in `DumpSymbols.zip` and using them means remembering the member, format and flags | `lookup <addr\|name>` and `join` (rename candidates, `zz_` confirmations, conflicts) against `symbols.txt` | ~80 |
| 7.8 | `tools/units/dataclaim.py` | the riskiest edit class was reasoned out by hand three times | for each proposed run: target section size/bytes vs ours, verdict, expected effect | ~80 |
| 7.9 | `tools/symbols/phantom.py` | five 4-byte `fn_*` were not functions (dead epilogues) and cost four functions their last 11 points | for every small unnamed `fn_*`, test whether the previous function's bytes include it → merge candidates | ~60 |
| 7.10 | `prepcommit.py` knowledge-delta check | the rule was enforced by me remembering | warns when a unit's score rose and no `src/` header, `docs/` file or `configure.py` comment changed in the same commit | ~40 |
| 7.11 | ledger byte burn-down | `295 / 20 519` and `65 128 / 5 437 392` are two burn-downs and the second predicts the DOL | print both, plus a coarse per-0x10000-block view | ~40 |
| 7.12 | one shared-file writer module | CRLF vs LF silently voided two edits; an undeclared progress category failed the *next* `ninja` | one module owns the writes, asserts every anchor, is idempotent, runs `symbolpreflight`'s overlap check before a range lands | ~120 |
| 7.13 | `attribute.py` size defaults | the two size defaults are a guess | derive them from the units we have measured (288 B … 27 KB) and say so in the docstring | ~20 |
| 7.14 | `attribute.py --max-total-bytes` | §3's 0.5 MB cap is not expressible today (only `--min-bytes`, `--max-bytes`, `--limit`) | a registration batch never exceeds the cap, and the tool says how many bytes it is about to claim | ~20 |
| 7.15 | worktree-safe measurement | a fresh worktree has no `build/` and cannot measure at all; this blocks the first 4-worker round | a worker in a worktree measures its own object against `MAIN`'s target and prints both paths | inside 7.1 |
| 7.16 | `land.py` owns the baseline | `ninja changes` compares against a `baseline.json` nobody refreshes, so a per-batch regression can hide | `verify` refreshes the baseline after a green batch and reports the batch's own delta | inside 7.5 |
| 7.17 | `tools/units/data-queue.json` + `dataclaim.py` (7.8) consuming it | data is 18.5 % of the DOL (1 233 640 B, 45 176 symbols, 720 B matched) and its queue is comments nothing reads | every data run `attribute.py` sees is in the queue with a verdict, and `land.py` reports the queue's size | ~40 |
| 7.18 | **ground-truth guard**: `prepcommit.py` refuses `config/RMHE08/build.sha1` and `config/RMHE08/config.yml`, and a tracked **`tools/git/hooks/pre-commit`** (enabled with `git config core.hooksPath tools/git/hooks` — local config, so 7.18 also states the checks that do *not* depend on a hook: `prepcommit.py`'s and `land.py`'s path refusals) refuses `orig/**`, `build/` and the LOCAL-ONLY block on **any** commit path | `prepcommit.classify('config/RMHE08/build.sha1')` returns `stage` today: a worker or I could rewrite the DOL's expected hash and commit it, after which green `ok` means nothing | a staged `build.sha1` is refused, and the hash is checked against `orig/RMHE08/sys/main.dol`'s own sha1 | ~40 |
| 7.19 | link-order audit for flips | 13 584 objects link in 66-131 s now; with hundreds of `Matching` units the order, pool placement and symbol collisions become real | after a batch of flips, compare `main.MAP`'s section/symbol order against the original and diff the DOL | ~60 |
| 7.20 | transactional `attribute.py apply` | `apply` writes `splits.txt` first and can leave a half-registration; `plan` can propose overlapping data spans | no proposal overlaps a claimed range, both shared files are written via temp+rename, and a failure restores them | ~40 |
| 7.21 | `tools/units/stylelint.py` - the eight rules of §6.5 | 19 units carry 376 auto-generated names, 237 pointer-arithmetic field accesses, 320 `unk*` fields, **0** struct-size annotations, and now a `goto` backlog in `Pl/pl_act`/`Pl/pl_skill`; a rule enforced by remembering is not a rule | flags each rule as `file:line` (including `\bgoto\b`), reports a per-unit backlog (`--budget`), and `land.py` refuses a batch that **adds** a violation | ~140 |

Rules for building them: **a tool that writes shared files goes through 7.12**; **every tool that mutates state
has a selftest** (`ledger_selftest.py`, `attribute_selftest.py`, `m2cinput_selftest.py` are the pattern); and **a
tool is not done until `land.py` calls it or the plan says who runs it**.

**Status (2026-09-23): the first five items are built, tested and committed.**

| item | state | evidence |
| --- | --- | --- |
| 7.18 ground-truth guard | **done** | `prepcommit.py` refuses `build.sha1`/`config.yml` and cross-checks the DOL hash; tracked `tools/git/hooks/pre-commit`; `tools/git/guard_selftest.py` (33 checks); both refusal paths exercised by hand |
| 7.1 + 7.15 `recompile.py` | **done** | compiles without ninja from a worktree that has **no `build/`**, asserts the mtime moved, prints section sizes, measures with `objdiff-cli -1/-2`; verified in MAIN (94.78261 %) and in a scratch worktree (identical, MAIN's object untouched) |
| 7.2 `claims.py` | **done** | branch-as-lock (`worker/<slug>`), registry in `MAIN/.pi/claims.json`, `list`/`release`/`expire`; refuses a second claim; 12 checks |
| 7.3 `brief.py` | **done** | six parts, rules extracted verbatim from §6.5/§8, inventory parsed in-process; for `Pl/pl_act`: 115 symbols, 5 below the bar, every score real |
| 7.4 `handoff.py` | **done** | outbox schema + validation (13 checks): unowned symbols, out-of-range percentages, a rename without evidence, a missing `measured_with` are all refused |
| 7.5 + 7.16 `land.py` | **done (build path dry-run only)** | 13 checks; the gate refuses on a moved base, a shared-file edit, a missing/invalid outbox, a regression, and requires `ok` to be recreated by *this* run; baseline refresh |
| 7.17 data queue / 7.20 transactional `apply` / 7.21 `stylelint.py` | **not yet** | `brief.py` already reads the queue and `land.py` reports the missing lint as skipped |

**Flip campaign (7.6), round 1 2026-09-23.** Six objects are flipped and committed, each alone in its commit -
`memset` (3e5a072), `NetworkWiiMediator` (6e0ed8c), `OSAlarm` (0806eae), `memcpy` (eadd4bd), `lobby_scene`
(a247195), `global_destructor_chain` (1e6f93c) - with `main.dol` still
`bf4850739478caaedfe675949eb7c28595a7fde9`. Two further candidates failed, and each failure wrote a rule the
proof has to contain (detail: `.pi/notes/flip-round-1.md`):

1. **Alignment is part of the proof.** `NetworkWiiMediator` and `OSAlarm` matched the target in every section
   size and every byte, and still broke the DOL: their `.text` was `align 2**4` (from `cflags_base`'s `-O4,p`
   implying `-func_align 16`) where retail's is `2**2`, so the linker rounded the object's start to the next
   16-byte boundary - `fn_80413F3C` moved to `0x80413F40`, every later symbol shifted by 4, 1 223 723 DOL bytes
   differed. Fixed per lib (`cflags_network`, `cflags_os`, commit b523f3d). Every retail object in the project
   is `.text`/`.init align 2**2`, so 4-byte alignment is the default expectation - and the two units that
   genuinely want 16-byte padding (`__start`, `__ppc_eabi_init`) want it from their own section pragmas.
2. **Our object must provide every section the unit's `splits.txt` entry claims.** `sys_mem.cpp` claims
   `extab` (0xA0) + `extabindex` (0x30) + `.text` (0x120) but our object emits only `.text`; flipping it
   removes those 208 bytes from the link and the whole DOL shifts (`_eti_init_info` 0x8003F1C8 -> 0x8003F17C,
   5.5 MB of differing bytes). A flip substitutes our object for the original *region*, so anything the region
   had and our object does not emit is lost.

`tools/units/flipcheck.py` checks all three conditions per unit (claim vs emitted sections, sizes/alignment,
and the bytes against the target object) and reports **7 of 19 ready**. Its one false positive is
`Runtime.PPCEABI.H/__init_cpp_exceptions`, which passes all three and still fails: `dtk dol diff` reports
`__init_cpp_exceptions_reference` expected at `0x8056F2C0` - the *first* `.ctors` entry - and ours holds a
different value there. The unit's `.ctors`/`.dtors`/`.sdata` fragments are claimed and emitted, so this is
not the sys_mem mechanism: a flipped object's `.ctors$10` entry does not land where the original's did. That is
the link-order question roadmap **7.19** has to answer, and it is the flip campaign's own remaining unknown.

**End-to-end dry round (2026-09-23).** claim → brief → measure inside the worktree → outbox → gate: `claims.py
claim` → `brief.py` → (in the worktree) `recompile.py --measure` → `handoff.py --check` → `land.py verify
--dry-run` exited **0** with all five cheap checks passing, then the claim was released. The heavy path
(configure → split → report → regressions → `ok` → baseline) has only been exercised as a *plan*; the first real
land is what runs it.

**Ordering constraint.** **7.18 comes first** - the ground-truth guard exists before any worker does, because it is the
only hole that can invalidate the whole campaign's evidence. Then 7.1 + 7.15 and 7.2 (a worker that cannot
measure, or two workers on one unit, is not a round), then 7.3 + 7.4, then 7.5 - the gate every later batch runs
through. 7.6 (the first flip) may happen before any of them: it needs no worker. — it needs no worker.

**The asm dump is a read-only input for attribution, not a build step.** `config.yml` sets `write_asm: false`,
so `build/RMHE08/asm/` is produced on demand (`python tools/splits/dump_asm.py`, 200–400 s) and stamped with the
sha1 of `symbols.txt`/`splits.txt`/the DOL. `tudiscover` reads it, so before an attribution batch the
orchestrator runs `python tools/splits/dump_asm.py --check` and regenerates when stale — a stale dump is
*silent* and zeroes a codegen fingerprint, which is why the stamp exists.

## 8. Invariants — the rules that do not change

**8.1 Repository rules (non-negotiable).** Never modify `orig/RMHE08/**`. **Never edit `config/RMHE08/build.sha1` or
`config/RMHE08/config.yml`** - they are the ground truth the campaign is measured against, and a rewritten hash
would make every later `ok` meaningless (7.18 enforces it). Never commit build output, original files or
scratch. Never change compiler flags/`mw_version`/tool tags to make something build - a flag change needs
instruction-level evidence and is called out explicitly. `Matching` only when byte-identical. Do not rename or
delete a map symbol unless nothing else depends on it. Never rewrite history, never push. Never print
`symbols.txt`. Never commit the LOCAL-ONLY block.

**8.2 Flag policy.** A scoped `#pragma` in the source is for **one unit's** deviation; a **lib** flag is for when
**two or more units of that lib agree** on the real command line (that is how `Pl`'s four flags were settled:
three units, independently measured). Evidence lives in a comment next to the flag in `configure.py`. Every flag
change is re-measured across the whole lib (function alignment moves every symbol after the first).

**8.3 Seam policy.** A boundary may be claimed only from `tudiscover`'s evidence kinds. Anything else — a shared
static, a call pattern — is a *hint* for the unit header. An unproven seam is allowed (the extent settles as its
functions match) and its header says so.

**8.4 Data policy.** Measure before *and* after every data claim; never claim a range our object does not emit;
never claim linker-generated data.

**8.5 Measurement policy.** The object measured must come from the real command line; a stale `report.json` lies
(regenerate, 1.5–3.5 s); frame size is not progress; a per-symbol score is the only score.

**8.6 Blast radius.** Everything lands `NonMatching`, so a bad batch cannot break the link — but it can break the
*next* session's ability to measure. A half-registered unit, a range with no `configure.py` entry, or a report
that was not regenerated is what actually stops the loop.

## 9. The knowledge delta — the compounding asset, with a machine check

Every batch owes a delta, and it has exactly four homes:

| what was learned | where it goes | who reads it next |
| --- | --- | --- |
| a matching idea (code shape, flag, allocator rule) | a section in `docs/matching.md` **and** its row in the `AGENTS.md` playbook table | every future session and the `mwcc-unit-matching` skill |
| a workflow/tool fact (a gate, an order, a trap) | the skill that owns that step | every agent running that step, workers included |
| a unit's residual | that unit's file header | whoever touches that unit next |
| a campaign mechanic (registration, batching, orchestration) | this file | the next batch's orchestrator |

Three rules make it real: **a batch is not closed until its delta is written** (write it before reading the
staged diff — that is when the numbers are in hand); **the generated skill references must be in sync**
(`sync_reference.py --check` in the same commit, or the knowledge exists and no agent will load it — it once sat
55 lines behind); and **a batch that found nothing new says so** ("nothing new" is a legitimate delta; silence is
indistinguishable from not having looked).

From 7.10 this is a machine check, not a good intention: `prepcommit.py` warns when a unit improved and no
document changed, and `land.py verify` runs the same check before the commit message is written.

## 10. Escalation, grants and the decided queue

**Standing grants (owner, 2026-09-21).** The orchestrator runs the campaign without gates: it decides the batch,
the registrations, the flags, the queue and the commits, and does not ask per item. It interrupts the owner only
for a **pressing** issue: (1) the linked DOL is at risk outside the `NonMatching`-safe path; (2) loss or
destruction — deleting work, rewriting history, pushing, an unrevertible bulk edit; (3) information only the
owner has; (4) a premise change (the bar, the scope, the vendor files, an `AGENTS.md` rule); (5) the
environment. Everything else is the campaign's own business, recorded rather than asked about.

**Decided queue — do not re-litigate:**

1. `memcpy.c` and `memset.c` stay separate (one file would let MWCC inline `__fill_mem` into `memset` and cost a
   100 % symbol).
2. The TRK interrupt-vector table (0x80004380–0x800062B4) stays unowned (zero relocations, not expressible in C
   without hand-written assembly) — and §1's completion test counts functions, so this is not a blocker.
3. `-func_align 4` for `Runtime.PPCEABI.H` — **landed** (`cflags_ppceabi`), three independent witnesses.
4. `pl_act`'s `.sdata2` run (0x8079A080–0x8079A114) and `Gecko_ExceptionPPC.cp`'s `.bss fragmentinfo`
   (0x806F4B48) stay **unclaimed**: our objects do not emit those sections, so the claim pairs a section against
   nothing and loses score. The win came from `extern` declarations used as load operands (playbook 29).
5. `-sdata 0` for `Pl/pl_skill.cpp` is **ruled out** (purely harmful once the real fix landed; the absolute
   access is a property of that symbol's section, not of the unit's addressing mode).
6. `pl_master`'s `fn_8026F908` (9 bytes) is an allocator colouring tie-break — recorded, not chased.
7. Artifacts for the owner are **light-theme** (owner preference, 2026-09-22).

`AGENTS.md`'s local-only block is a **pointer to this file**, not a second queue: where the two disagree, this
file wins and the block is corrected in the same commit.

**Escalation queue (open):** a re-attribution the `preflight` verdict calls `approve`; a `never touch` owner; a
policy question; a batch that would exceed the standing commit approval. Keep working on the rest and hand the
queue over as a list with numbers.

## 11. Verification, handover, and the stop conditions

**`land.py verify` is the land-batch gate** (once built; until then, its commands by hand): `python configure.py`
→ `ninja` → `ninja build/RMHE08/report.json` → the regression scan (`ninja changes`, no measure down) →
`ninja build/RMHE08/ok` → `ledger.py` → the knowledge-delta check.

It must prove the `ok` it reads is *this* run's: `land.py` deletes `build/RMHE08/ok` (and `main.elf` when the
batch flips an object) **before** it starts and requires both to be recreated; it also checks every command's
exit code, `configure.py`'s included — a failed `configure.py` otherwise leaves a stale `build.ninja` and every
later number is meaningless. The `.ninja_log` ordering idea is **not** a substitute: a `NonMatching` batch never
relinks, so `main.elf` never runs and `ok` is the only edge that re-validates. Ordering still matters for
freshness (`ninja -t query build/RMHE08/main.elf` shows the `order_only` dependency on `config.json`).

**And the style lint is part of the same gate** (`python tools/units/stylelint.py --diff <base>`): a batch may not
*add* a violation of §6.5, and it must not leave a violation in a unit it touched.

**Handover.** Before a compaction or the end of a session: the local-only block says which batch is open, the
ledger is the state, and anything worth keeping is in `docs/`, a skill, `AGENTS.md` or a unit header. A finding
that lives in a reply is lost — that has already happened once here.

**Stop conditions** — the orchestrator does not stop because a batch ended. It stops when (1) the campaign target
it was given is closed (report the delta and the new totals); (2) a gate blocks it — an escalation item, a
regression it cannot fix, or a check that keeps failing (three attempts on one symptom is the limit: report the
attempts, not a fourth); (3) its working budget runs out — and then only at a verified boundary: committed batch,
`splits.txt` through a re-split and a report, `ok` green, block updated.

## 12. The next batches, concretely

**Today there are no workers**, and that is the normal case: the orchestrator does everything itself. The §5.2
brief is then a *file it writes for itself* too, so a later worker can pick the unit up, and the compile is
`ninja build/RMHE08/src/<unit>.o` instead of `recompile.py`. Nothing in the protocol waits for workers to exist.

1. **The ground-truth guard** (7.18) - `prepcommit.py` refuses `build.sha1`/`config.yml`, the tracked
   `tools/git/hooks/pre-commit` is added and `core.hooksPath` pointed at it. First, because it is the only hole
   that can invalidate the campaign's evidence, and it is ~40 lines.
2. **The protocol tools** (7.1 + 7.15, 7.2 -> 7.5, 7.17, 7.20) - **7.1/7.15, 7.2-7.5 done 2026-09-23**; 7.17, 7.20 and 7.21 remain - `recompile.py` (worktree-safe), `claims.py`,
   `brief.py`, `handoff.py`, `land.py`, the data queue, transactional `apply`. The first *worker* round does not
   start before 7.1/7.15 and 7.2 exist; the first *land batch* does not start before 7.5 does.
3. **The first `Matching` flip** (7.6) - one byte-identical runtime *object*, alone in its commit. It may run
   before step 2 finishes, because it needs no worker. Cheapest possible moment to discover a linking problem.
4. **`dumpmap.py` + one batched rename pass** (7.7) - thousands of real names, one re-split, every rename
   verified after it.
5. **The attribution pass, scaled** - `attribute.py apply` over the next regions in ascending address order,
   registration batches capped at 0.5 MB, each new unit's stub source in the same commit, its seam re-checked the
   moment its functions match.
6. **Phase C, the residual sweep** - the 80-99.9 % band is on the critical path for the DOL (a unit with any
   residual cannot flip): `pl_act`'s near-misses, `pl_skill`'s two allocator-shaped residuals, `pl_master`'s
   9 bytes, then `Camellia` 99.97 %, `RSO` 99.71 % and `Gecko` 93.68 %. **The type and naming conformance of §6.5
   rides this pass** - the same units, the same order - so a unit leaves phase C byte-identical *and* conformant.
7. **The data second pass** (7.8 + 7.17) - measured, one batch at a time, driven by the data queue rather than
   by comments in `splits.txt`.
8. **Phase D, the flip campaign** (7.19) - as units reach byte-identity, flip them in batches with a link-order
   audit, until `ok` passes from a clean tree (S1).


## 13. Risks, and where each is handled

| risk | handled by |
| --- | --- |
| reaching for flags to fix what is really source shape or liveness | playbook 13, `mwcc-unit-matching` |
| a `Matching` flag on a wrong object | non-negotiable 4, `objdiff-verify`, one flip per commit, §1's per-object rule |
| claiming a data range that lowers a function | playbook 23, §8.4, `dataclaim.py` |
| a rename that splits the map and the source | `symbol-map-editing` (two edits, one change), `symedit.py rename-batch` |
| a stale report or a hand-written compile | `recompile.py` (7.1), §8.5 |
| four agents editing one file | worktrees + branches (§5.1), one writer per unit, `land.py`'s file-set check |
| a worker in a worktree cannot measure (no `build/`, no `build.ninja`) | 7.15 — `MAIN` resolution, explicit `--target`/`--flags`, the submodule init; the first worktree round does not start without it |
| a worker's commits silently dropped by a tip-only cherry-pick | §5.1's one-commit rule, or a range cherry-pick |
| committing the LOCAL-ONLY block | non-negotiable 8, `prepcommit.py`/`localonly.py` |
| rewriting the DOL's ground truth (`build.sha1`) | 7.18's refusal + the sha1 cross-check against `orig/RMHE08/sys/main.dol` |
| a worker's diff landing on `main` before verification | `git cherry-pick --no-commit` → verify → commit (§5.1) |
| a worktree that cannot be removed (submodule, untracked evidence) | no submodule in worktrees; evidence in `MAIN/.pi/`; `remove --force` + `branch -D` + `prune` (7.2) |
| overlapping or half-written registrations | `apply` transactional + the overlap check (§6.1) |
| a residual unit silently blocking the DOL | §1.1: phases C and D are on the critical path, and the residual class is scheduled, not hoped for |
| a green `ok` from stale split objects | §1's clean-tree completion test |
| a green `ok` from a stale link | §11's delete-and-recreate check in `land.py` (plus the exit-code checks) |
| the attribution pass claiming a wrong seam at scale | the 0.5 MB cap, evidence-first partition, unproven seams marked, and matching as the arbiter |
| the compiler-build residual (`0x0e` vs `0x0f` `.comment`) | recorded as a project-wide known difference; a unit that resists with everything else equal is a candidate for the version matrix, once |

## Appendix A — the incident → rule map

The fourteen incidents in `docs/process-review.md`, and the rule that now prevents each:

| incident | rule |
| --- | --- |
| 1 overlapping registration | §6.1 `symbolpreflight` verdict is a gate; 7.12 runs the overlap check |
| 2 CRLF/LF silent no-op | 7.12 (one writer, asserts every anchor) |
| 3 undeclared progress category | 7.12 + `land.py` runs `configure.py` before the split |
| 4 stale-object measurement | 7.1 `recompile.py` |
| 5 stale `report.json` | §8.5, `ledger.py`'s stale warning |
| 6 phantom `fn_*` symbols | 7.9 `phantom.py` |
| 7 the first partitioner's 17 one-function units | §6.1 evidence-first partition + selftest |
| 8 a size cap cutting inside a must-link anchor | `attribute_selftest.py` (23 checks) |
| 9 the wrong zip member | 7.7 `dumpmap.py` + `docs/memory-dump.md` |
| 10 an unsupported seam claim | §8.3 seam policy |
| 11 concurrent writers | §5.1 worktrees + branches |
| 12 manual verification | 7.5 `land.py` |
| 13 the knowledge delta by memory | 7.10 the machine check |
| 14 an out-of-band flag | §8.2 flag policy |

## Appendix B — the numbers, and how to re-measure them

```sh
python tools/units/ledger.py                 # totals, per-module, closed >= 80 %, unclaimed
python tools/units/ledger.py unit <unit>     # one unit's coverage and per-symbol score
python tools/units/ledger.py --json          # machine-readable
ninja build/RMHE08/report.json               # 1.5-3.5 s; the report is what the bar reads
ninja changes                                # regression scan against the baseline
ninja build/RMHE08/ok                        # the DOL hash - the only test that matters
python - <<'EOF'                             # per-edge build costs, straight from .ninja_log
for l in open(".ninja_log"):
    p = l.split("\t")
    if len(p) >= 5 and p[3].endswith(("config.json", "main.elf")):
        print(int(p[1]) - int(p[0]), "ms", p[3])
EOF
```

Baseline for this plan: **295 attributed / 284 closed ≥ 80 % / 217 matched / 65 128 bytes**, commit `9d6b351`.
