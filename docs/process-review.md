# Process review: the matching campaign, after seven batches

A point-in-time review of how the work is actually running - what it costs, what went wrong, and what I would
change. Written 2026-09-22 from the session that took the project from 60 to **217 matched functions**
(65,128 of 5,437,392 `.text` bytes, 295 symbols attributed of 20,519, `main.dol` still `OK`).

It is deliberately blunt about my own mistakes: the incident table is the most useful part, because almost
every one of them is a *process* failure that a tool or a rule can prevent, and every one of them cost
between ten minutes and a wasted batch.

Not committed yet - say the word and it goes in, or I move it to `.pi/notes/`.

---

## 1. What the loop costs today

**Corrected 2026-09-22** against `docs/build-performance.md` (the owner's measured profile) and `.ninja_log`
itself. The first version of this table said "the split is 5-20 minutes and it is the tax on every batch" -
that was **wrong**, and the reason is worth recording: those wall clocks were taken while a *second* stream
was splitting and linking the same tree, and the earliest ones predate the `write_asm: false` change (the
13.5 k `.s` dump was 200-400 s on its own). The per-edge numbers from `.ninja_log` are unambiguous:

| edge | measured | notes |
| --- | --- | --- |
| `build/RMHE08/config.json` (one `dtk dol split`) | **16.8 / 16.8 / 27.1 / 46.5 s** (last four); ~18 s typical | writes 13.5 k objects, skips identical ones; **200-400 s** with `write_asm: true` |
| `build/RMHE08/main.elf` (the link) | **66.8 - 130.9 s** over 13 584 objects | the largest single edge in a batch |
| `report.json` / `baseline.json` | 1.5 - 3.5 s | objdiff over every unit, 32 threads |
| one source unit (`mwcceppc`) | 0.2 - 24 s | worst so far `Pl/pl_skill.o` |
| `main.dol`, `ok` | < 0.5 s | `dtk elf2dol` + the sha1 check |

So a batch boundary costs **~1.5-2.5 min**: split (~20 s) + whatever units changed + link (70-130 s) +
report. A rename/merge-only batch that changes no object is ~20 s (nothing relinks). That is a *healthy*
loop, and it means the earlier framing was aimed at the wrong bottleneck.

What the corrected numbers change:

* The remaining-work arithmetic is much less alarming. 20 224 unclaimed symbols at one 12-function unit per
  batch is 1 685 batches x ~2 min = **~55 h** of *machine* time - and 42 h of that is the link, 9 h the
  split. At 250 functions per batch it is 81 batches ≈ **3 h**. Batching still wins, but for the link's sake
  (and for fewer human review boundaries), not because the split is expensive.
* Workers are *cheap*: they measure with `mt.py` against objects, never link, never split. Only the
  orchestrator's batch boundary pays the 1.5-2.5 min. So worker parallelism is worth pushing harder than
  batching is.
* The split stays the thing to be careful with, for a different reason: it is an **input of itself**
  (`ninja -t deps build/RMHE08/config.json` lists `symbols.txt`/`splits.txt`/the DOL), so any edit to the map
  re-runs it - which is why renames and phantom merges belong in the same batch as range changes (the owner
  documented this in `docs/build-performance.md`, "Knob 2").
* The link is the edge that a *batch* pays whether one object changed or fifty, and it is also the edge that
  can lie (`order_only` on `config.json` - without it ninja links while the split is still writing, and `ok`
  goes green from a stale link). `land.py` (suggestion 8) should therefore assert that the `ok` it reads came
  *after* the split in the same run.

## 2. What went wrong (and what still has no guard)

| # | incident | cost | guard now | still unguarded |
| --- | --- | --- | --- | --- |
| 1 | A registered range (`Gecko_ExceptionPPC.cp`, start 0x80456958) **overlapped** `__init_cpp_exceptions.cpp`; `dtk dol split` failed outright | one worker blocked, one wasted batch | none - a worker caught it, I fixed the start address | `symbolpreflight.py` knows how to detect this (kind 12) but nothing *runs it* before a `splits.txt` edit |
| 2 | `configure.py` is **CRLF**, `splits.txt` is **LF**; two replacement anchors silently matched nothing (a registration that existed in `splits.txt` and nowhere else) | two debug cycles, one re-split | `attribute.py` asserts instead of silently skipping | every other tool/hand-edit still does raw string surgery on these files |
| 3 | `progress_category: "auto"` not declared in `config.progress_categories`; `configure.py` exited 1 and ninja failed *after* the split had started | one 5-minute cycle | `attribute.py` declares it | nothing validates `configure.py` before a batch starts |
| 4 | Filesystem **1-second mtime granularity**: two workers measured an object the compiler never rewrote and reported "no change" for every variant | two workers lost ~20 min each, one wrote three scratch harnesses to work around it | playbook idea 6 mentions it | no shared helper: each worker reinvents "delete the object, then rebuild" |
| 5 | **Stale `report.json`**: one worker's unit read 36 % while its own per-symbol table showed 64 exact functions; another worker's regeneration raced a concurrent object rebuild and failed | one confusing moment | I re-generated before believing anything | `mt.py` does not warn when `report.json` is older than the objects |
| 6 | Five 4-byte `fn_*` symbols were **not functions** (dead epilogues after `mtctr`/`bctr` dispatchers); four dispatchers were stuck at 88.9 % for a map reason | only visible because a worker checked the dump | nothing | the class is mechanical and almost certainly has more members among 20k symbols |
| 7 | My first `attribute.py` design walked seed-by-seed and produced **17 one-function units** for a 784-byte region | one rewrite | evidence-first partitioner + `--min-bytes`/`--max-bytes` | the two size defaults are my guess, not derived from the units we know |
| 8 | The size cap could cut **inside a must-link anchor** (the one cut the evidence forbids) | none - the selftest caught it | selftest (`attribute_selftest.py`, 23 checks) | selftests exist for `ledger`, `m2cinput`, `attribute`; not for `tudiscover`'s new paths or the shared-file writers |
| 9 | I checked the wrong zip member and nearly **rejected a valid rename** (`CntSdRsoTerminate`) | one extra cycle | `docs/memory-dump.md` now documents the `.map` member and its format | no helper, so the next session re-derives the member name, format and flags |
| 10 | A worker claimed the unit's right edge was "probably not the TU boundary" from a shared-static reference; `tudiscover at` supports no such anchor | none - I left the seam and recorded the hint | the plan's "confidence is recorded, not hidden" | nothing states that a *seam* claim must be reproducible by `tudiscover`, not merely plausible |
| 11 | Concurrent writers: the owner's tooling commit landed mid-batch; workers reported each other's files as "dirty from a concurrent stream"; one `ninja` raced another | one failed report generation | workers get disjoint files, shared files are mine | workers run in the main tree; `decompile-symbol` already says "run inside a worktree" and we do not |
| 12 | Verification is manual and repeated: the same ~20-line regression-scan heredoc written **four times this session** (and in every batch before it), plus five commands and a ledger read per batch | ~5 min per batch, and it is where a mistake would hide | `prepcommit.py` verifies the DOL hash and flags per-unit regressions | no single command runs the checklist, and the scan is not a tool |
| 13 | The knowledge-delta rule ("record the win in the same session") is enforced by me remembering | one row nearly missed | `sync_reference.py --check` catches a stale skill reference | nothing links "a unit improved" to "docs or a header changed" |
| 14 | A lib-wide flag (`-opt nopeephole` for `Pl`) was landed while one unit had solved the same problem with a scoped pragma | one measured risk (came out fine) | I measured after landing | the rule "pragma for one unit, lib flag when two agree" lives only in my head |

## 3. Suggestions

Ordered by payoff per hour of work. "mine" = I can do it without a decision; "yours" = needs a call.

### A. Cut the split tax (the only big lever)

**1. Drain a config queue into one split per batch (mine, ~1 h).**
Keep a `.pi/pending-config.json` holding every *range / name / flag* change the workers discover. Workers
never touch shared files; the orchestrator applies the whole queue in one pass and re-splits **once**. This
session already worked this way informally (three worker reports each proposed ranges, I applied them in one
edit) - making it explicit is what stops an accidental split per worker.

**2. Measure whether the split can be partial (mine, ~2 h, could be the 10x).**
Three experiments, in order: (a) does `dtk dol split --no-update` + a hand-maintained `obj/` tree actually skip
the expensive analysis, or only the file writes? (b) how much of the 5-22 min is the *split* versus the
*analysis* (time `dtk dol split` alone on the current tree); (c) can the per-object outputs be cached and only
the changed range re-split into the same directory? If (c) works it is a wrapper script, not a dtk change.
If none works, file an upstream `--only <range>` request and say so in `docs/plan.md` so nobody re-tries it.

**3. Batch by size, not by "what the workers returned" (mine, policy).**
Target **≥ 250 functions or ≥ 15 KB per batch** (one split per ~5 batches of worker time rather than one per
worker). The cost is verification effort, which suggestion 8 automates away.

### B. Make workers cheaper and their output uniform

**4. A brief generator instead of hand-written briefs (mine, ~2 h).**
Every fan-out this session cost me a 40-line brief with the same rules, the same measurement loop, the same
evidence order - and each one drifted slightly (one worker probed a flag, another asked about a queue item
already decided). Proposal: `tools/units/brief.py <unit> [--task "..."]` prints the brief with the unit's own
facts filled in (lib, `cflags`, obj/target paths, `.text` range, the header's residual table, the decided
queue items) plus the invariant rules. The orchestrator then writes 5 lines of task-specific text per worker.
Payoff: smaller prompts, no drift, and the flag policy is stated once.

**5. A handoff formatter so replies are machine-checkable (mine, ~1 h).**
`tools/units/handoff.py <unit>` prints the table skeleton (every symbol, its measured %, a blank note) and
the invariants to check (unit total, no regression, header updated, nothing outside the file touched).
Workers fill it in; the orchestrator can diff it against `report.json` in one step. This session's worker
tables were good but inconsistent in shape (30 rows, 12 rows, prose), which is exactly what makes review slow.

**6. Notes file for detail, 10-line digest in the reply (policy, already half-adopted).**
Subagent output gets truncated (two fan-outs this session lost thousands of characters). The workers who wrote
`.pi/notes/<unit>.md` and replied with a table were strictly better to work with. Make it the rule: full
evidence in the notes file, ≤ 15-line table in the reply.

### C. Make the shared-file edits safe

**7. One module owns `splits.txt` / `configure.py` / `symbols.txt` writes (mine, ~2 h).**
Line endings (CRLF vs LF), idempotency, the `progress_categories` declaration, a `symbolpreflight` overlap
check, and an assert on every anchor. Every writer goes through it, so incidents 1-3 and half of 12 cannot
recur. `attribute.py` already does all of this for its own path - the point is that a *hand* registration and
the next tool get it too.

**8. `tools/units/land.py` - the batch checklist as one command (mine, ~2 h).**
`land.py verify` runs: `configure.py` → re-split → report → regression scan → `ok` → ledger delta → the
knowledge-delta check, and prints one table (per-unit before/after, regressions, DOL status, ledger delta,
"does a header or doc change accompany the improvement?"). This is incident 12: I have written the same
regression scan in every batch, and the one time it matters is the time I forget it.

**9. Extend `prepcommit.py` with the knowledge-delta check (mine, ~1 h).**
It already computes per-unit measures. Add: "unit X improved by N points and no `src/` header, `docs/` file or
`configure.py` comment changed" → warning. That turns the repo's own rule (CLAUDE.md, "record it in the same
session") into a machine check instead of a discipline I have to remember.

### D. Turn repeated reasoning into tools

**10. `tools/units/dataclaim.py` - decide a data claim from the objects (mine, ~2 h).**
The riskiest edit class. Three times this session the answer was "the target emits this section and our object
does not, so do not claim it" (pl_skill's `.sdata2`, pl_act's `.sdata2`, Gecko's `.bss fragmentinfo`), and
each time it was reasoned out by hand from a worker's measurement. The rule is mechanical: compare the target
object's section size/content with ours; if we emit nothing for it, the claim pairs a section against nothing
and loses score. Have the tool print target size / our size / verdict / expected effect for each run
`attribute.py` proposes.

**11. `tools/symbols/dumpmap.py` - the symbol map as a first-class oracle (mine, ~2 h).**
`DumpSymbols.zip`'s `.map` has 48 367 lines of real names, signatures and `zz_` placeholders, and it needs no
Ghidra session. Right now using it means remembering the zip member, the format and the flags. Proposal:
`dumpmap.py lookup <addr|name>` and `dumpmap.py join` (join `symbols.txt` against the dump: rename candidates,
`zz_` confirmations, conflicts). That is a large, cheap naming win across the 20k `fn_*` symbols - and renames
must be batched into one split (suggestion 1).

**12. `tools/symbols/phantom.py` - the phantom-function class (mine, ~1 h).**
Incident 6 cost four functions their last 11 points for a *map* reason. The check is mechanical: for every
small `fn_*` (say ≤ 8 bytes) that the dump map does not name, test whether the preceding function's bytes
include it (union byte-identical) → merge candidates. Bulk win, zero risk when the union check passes.

**13. `tools/units/recompile.py` - the mtime trap in one place (mine, 30 min).**
Delete the object, compile, assert the mtime advanced, print the section sizes. Three workers hit this
independently (incident 4) and one wrote a harness to work around it. It belongs in `tools/`.

**14. Derive `attribute.py`'s size defaults from the units we know (mine, 30 min).**
`--min-bytes`/`--max-bytes` are my guess today. The units we have actually matched say: `sys_mem` 288 B,
`main` 4.7 KB, `pl_act` 27 KB, `pl_skill` 15 KB, `pl_master` 17 KB, the runtime objects 40-850 B. A default
(min 0x100, max 0x8000) plus that table in the docstring is honest; today's 0x200/0x4000 is not.

**15. State the flag rule (policy, one paragraph).**
A scoped `#pragma` is for one unit's deviation; a lib flag is for when **two or more units of that lib agree
on the real command line** (that is what happened with `-O3`/`-inline noauto`/`-opt nopeephole` for `Pl`:
three units, independently measured). And a rule for seam claims: a boundary may be claimed from
`tudiscover`'s evidence kinds only; anything else (a shared static, a call pattern) is a *hint* for the unit's
header, not a seam (incident 10).

### E. Close the loop on the final artifact

**16. Start flipping complete units to `Matching` now (mine, one unit per commit).**
The ninja summary still reads `0.00% linked (0 / 5 files)`. `Runtime.PPCEABI.H` is 20/20 functions at 100 %
and `pl_master` is 9 bytes from byte-identical. Every linking question (`.ctors$10` placement, pool sharing
across units, section ordering, `extab` pairing in the link) is currently untested, and the first flip is the
only way to find out - preferably while the units are few and the diffs small, not at the end with 1 000
objects. This was already the decided policy ("one unit per commit, first flip alone"); the change is to do
it *early*, on a unit that is done, instead of after the attribution pass.

**17. Track attributed *bytes*, not just attributed symbols (mine, 30 min).**
`covered 295 / 20 519` and `65 128 / 5 437 392 bytes` are two different burn-downs, and the second is the one
that predicts the DOL. The ledger should print both, plus a coarse spatial burn-down (per 0x10000 block) so
"where the remaining work is" is a glance instead of a query.

## 4. The loop I would run instead

1. **Queue.** Workers (one file each, disjoint) write their findings, including every range/name/flag change
   they need, into `.pi/pending-config.json`. They never touch shared files.
2. **One split per batch.** I apply the queue, run `land.py verify` (which owns configure → split → report →
   regression scan → `ok` → ledger → knowledge-delta check), fix what it flags, commit.
3. **Batch size by evidence, not by convenience:** ≥ 250 functions or ≥ 15 KB per batch; source-only changes
   (no range, no name, no flag) are *never* batched with a split - they ride the next one for free.
4. **Attribution runs ahead in bigger strides** (`attribute.py apply --limit N`), each new unit's stub written
   in the same commit, and its seam re-checked the moment its functions match (matching settles the boundary).
5. **The dump map names things as we go** (batched renames), and phantom symbols are swept mechanically.

## 5. What I would not change

* **`NonMatching` until byte-identical + green `ok`.** It is what kept every one of these seven batches safe.
* **Per-unit measurement against the real command line**, and "measure before *and* after" for every data
  claim. Two of this session's three data decisions were *not* to claim, and both would have cost score.
* **Recording confidence instead of hiding it** (unproven seams, residuals in the unit header, ruled-out rows
  in the playbook). It is why a fresh session can pick any unit up.
* **The knowledge-delta discipline itself** - only its *enforcement* should move into the tools.
* **Worker-per-file parallelism.** It worked: seven units in one round, then three, with no collisions.

## 6. Ranked next actions

| # | action | who | effort | payoff |
| --- | --- | --- | --- | --- |
| 1 | `land.py` (batch checklist + regression scan as a tool) | mine | 2 h | removes the repeated 5-20 min ritual and the place a mistake hides |
| 2 | config queue + "≥ 250 functions per split" policy | mine | 1 h | 3-5x fewer splits immediately |
| 3 | Partial-split experiments (a/b/c above) | mine | 2 h | potentially 10x on the dominant cost |
| 4 | `dumpmap.py` + a batched rename pass | mine | 2 h | thousands of real names, better attribution quality |
| 5 | `dataclaim.py` | mine | 2 h | makes the riskiest edit class mechanical |
| 6 | `brief.py` + `handoff.py` | mine | 3 h | smaller prompts, uniform replies, no rule drift |
| 7 | One shared-file writer module | mine | 2 h | incidents 1-3 cannot recur |
| 8 | First `Matching` flip (a complete unit) | mine, one commit | 1 h | derisks the link path while it is cheap |
| 9 | `phantom.py` + `recompile.py` + ledger byte burn-down | mine | 2 h | bulk map fixes, no more mtime traps |
| 10 | Knowledge-delta check in `prepcommit.py` | mine | 1 h | the rule enforces itself |
| 11 | `--only <range>` upstream request, if 3 fails | yours | - | the only real fix for the split tax |

---

## 7. Addendum: relocating the tree (2026-09-22, D: -> C:)

The repo was moved from `D:\WiiExperiment\mhtri-dtk` to
`C:\Users\InusualZ\Documents\Development\mhtri-dtk` while this campaign was running, which turned out to be a
useful stress test of the build - and produced three facts worth keeping.

**The tree is relocatable, and that is by design.** `build.ninja`, `objdiff.json` and `compile_commands.json`
contain **no absolute paths** (verified: zero matches for `D:\`/`D:/` in all three; the compiler command lines
are `build\tools\sjiswrap.exe build\compilers\Wii\1.3\mwcceppc.exe` and the sources are `src\...`). So the
already-built `build/` tree is valid at the new path: after copying it, `ninja` did **21 steps in 2.2 s**, the
forced link (`rm build/RMHE08/ok build/RMHE08/main.dol && ninja build/RMHE08/ok`) reported
`main.dol: OK`, and `sha1sum` matched `config/RMHE08/build.sha1` (`bf485073...`) exactly. **No re-download, no
re-split, no recompile.** The report, the ledger and per-unit objdiff all work from the new path unchanged
(295 covered / 284 closed / 217 matched / 65 128 bytes).

**A re-split is ~18 s; the batch cycle is ~1.5-2.5 min.** Touching `config/RMHE08/symbols.txt` (mtime only,
no content change) and running `ninja` re-split the DOL and rewrote the map/splits *identically* (clean
`git status`) in **48 s** - and that 48 s was split (~20 s)+ report + progress + baseline, not the split
alone. `.ninja_log` puts the split at 16.8-46.5 s and the link at 66.8-130.9 s (§1, and
`docs/build-performance.md`). The §1 numbers in the first version of this review (4 m 42 s - 22 m 01 s) were
wall clocks taken under contention from a second stream and, for the earliest ones, with the 13.5 k `.s` asm
dump still enabled (200-400 s on its own). Correction recorded rather than quietly deleted: a review whose
headline number is wrong sends the next session at the wrong bottleneck, which is exactly the failure mode
this document is about.

**Copy recipe (for the next move, or a backup/CI copy).** `build/` and `orig/` are gitignored but required:
`orig/` is the ground truth (4 759 files, the DOL among them) and `build/` holds the downloaded toolchain
(`build/binutils`, `build/tools`, `build/compilers`) plus the split tree - copy both, or re-fetch with
`ninja tools` and pay a re-split. Two traps: in Git Bash, MSYS mangles robocopy's `/E`/`/XO` switches into
paths, so run it as `MSYS2_ARG_CONV_EXCL='*' robocopy ...`; and `robocopy /E /XO` (never `/MIR`) is the safe
form, because `/MIR` would delete destination-only content such as a moved `orig/`. A `.git` copy can leave a
**stale commit-graph** (two commits unreadable, `git fsck` reporting `failed to parse commit ... from
commit-graph` while `log`/`status` work): `git commit-graph write --reachable` fixes it, and the same two
commits did not exist in the source either, so it is a pre-existing wart, not a copy defect.

**Verification checklist for a moved tree** (the exact sequence, ~1 minute of commands):

```sh
git -C <new> log --oneline -1 && git -C <new> status --short && git -C <new> submodule status
git -C <new> fsck --connectivity-only --no-progress      # no output = clean
sha1sum <new>/orig/RMHE08/sys/main.dol                   # bf4850739478caaedfe675949eb7c28595a7fde9
cd <new> && python configure.py && ninja                  # 21 steps if build/ came along
rm -f build/RMHE08/ok build/RMHE08/main.dol && ninja build/RMHE08/ok
python tools/units/ledger.py | head -4                    # numbers must match the old tree
python tools/units/attribute_selftest.py                  # tool self-tests
```
