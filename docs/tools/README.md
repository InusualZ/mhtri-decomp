# Tools: the map

`tools/` is one package of shared concepts (`tools/lib/`) and the thin tools on top of it. This page is the index: which
concept lives in which lib module, which tools stand on it, where each tool's spec is, how to add a tool, and the header every
tool file carries. The design is `design.md`; the analysis that produced it is `inventory.md`, `duplication.md`, `prose-audit.md`;
the plan to get there is `migration.md`; what is dropped is `retired.md`; what the owner still has to decide is `questions.md`.

## Concept -> lib module -> tools

| concept | lib module | spec | tools standing on it |
| --- | --- | --- | --- |
| trees, paths, scratch, state, ground truth | `lib/repo.py` | `spec/lib-repo.md` | every tool |
| subprocess with the codec rule | `lib/proc.py` | `spec/lib-proc.md` | every tool that shells out |
| git calls | `lib/git.py` | `spec/lib-git.md` | land, claims, slots, lane, rescue, mergebranch, stylelint, guard, prepcommit, commitlint, edit |
| bytes, line endings, atomic writes, anchors | `lib/text.py` | `spec/lib-text.md` | edit, escape, symedit, dataqueue, backlog, sync_profiles, mergebranch |
| stamped caches | `lib/cache.py` | `spec/lib-cache.md` | callers, tudiscover, undefrefs, verifyunit |
| generated names and manglings | `lib/names.py` | `spec/lib-names.md` | callees, dumpmap, undefrefs, langcheck, mangle, methodize, stylelint, typeregistry |
| `symbols.txt`, `splits.txt`, `configure.py`, ownership | `lib/project/` | `spec/lib-project.md` | symedit, stylelint, symbolpreflight, ledger, datagap, dataclaim, dataqueue, flipcheck, vtableaudit, unwindcut, land, brief, queue, ... |
| ELF, DOL, objdump text, DWARF, fixture builders | `lib/binary/` | `spec/lib-binary.md` | elfsect, objalign, objextab, dwarfmap, dossier, linkorder, langcheck, flipcheck, verifyunit, datagap, sectiongap, pairgap, relocdiff, undefrefs, vtableaudit, vtslot, unwindcut, m2cinput, callees, infer, mwlink |
| instruction decode, reference scan | `lib/ppc.py` | `spec/lib-ppc.md` | splitcheck, phantom, infer, dossier, accessextent, callers, vtableaudit |
| who references what (index, census, the query, the dump's stamp) | `lib/refs.py` | `spec/lib-refs.md` | callers, accessextent, datagap, dataclosure, dataclaim, poolseams, tudiscover, splitcheck |
| the TU-seam evidence (data order, pool, source names) | `tools/splits/seams/evidence.py` | `spec/seams.md` | tudiscover, dataorder, dataseams, poolseams, langcheck, splitcheck |
| scores, the metric, freshness, regression | `lib/report.py` | `spec/lib-report.md` | ledger, verifyunit, unitscore, symdiff, measure, recompile, pairgap, datagap, brief, land, flags/* |
| the unit and its compile command | `lib/units.py` | `spec/lib-units.md` | recompile, measure, unitscore, symdiff, flipcheck, datagap, verifyunit, brief, flags/* |
| target vs ours comparisons | `lib/objcompare.py` | `spec/lib-objcompare.md` | datagap, dataclosure, sectiongap, pairgap, relocdiff, flipcheck, undefrefs, verifyunit |
| fact tokens and where they survive | `lib/facts.py` | `spec/lib-facts.md` | factscheck, sweepcomments, lanecheck |
| the runtime dump's symbol map, by address | `lib/dumpsyms.py` | `spec/lib-dumpsyms.md` | dumpmap, lanecheck |
| comment vocabulary: stale-path and narrative markers, the stale judgement | `lib/comments.py` | `spec/lib-comments.md` | stylelint (rule 15), sweepcomments |
| C/C++ text scanning | `lib/cscan.py` | `spec/lib-cscan.md` | stylelint, typeregistry, declclash, recordmerge, methodize, vtableaudit, shapes |
| findings, rows, verdicts, add-only diff | `lib/findings.py` | `spec/lib-findings.md` | land, stylelint, vtableaudit, undefrefs, datagap, splitcheck, flipcheck, verifyunit, dataclaim, symbolpreflight, handoff |
| the CLI entry point | `lib/cli.py` | `spec/lib-cli.md` | every tool |
| the test harness | `lib/testing.py` | `spec/lib-testing.md` | every test under `tools/tests/` |
| the outbox and notes | `lib/outbox.py` | `spec/lib-outbox.md` | handoff, backlog, tooling, playbook, brief, land, slots |
| integrator requests: schema, legacy loader, classification, owners, STOPGAP | `lib/requests.py` | `spec/lib-requests.md` | integrate, stylelint, brief, handoff |
| lanes: naming, registry, sessions, slot pool, seeding, rescue, teardown, launch, landing log | `lib/lanes/` | `spec/lib-lanes.md` | claims, slots, lane, rescue, wtsafe, queue, lanecmd, worktreehook, landlog, brief, backlog, recompile |

## Spec index (the kept and new tools)

Core gate: `land` (+ `lane-manifest`), `verifyunit`, `stylelint`, `vtableaudit`, `undefrefs`, `flipcheck`, `datagap` (+ `dataclosure`), `langcheck`,
`lanecheck` (advisory, the review's mechanical half),
`ledger`, `recompile`, `handoff`, `playbook`, `commitlint`, `guard`, `prepcommit`, `selftest`, `objalign`, `objextab`,
`ideas` (+ `ideas_demo`), `sync_playbook_index`, `sync_profiles`.

Core evidence: `callers`, `callees`, `accessextent`, `dossier`, `symedit`, `dumpmap`, `phantom`, `mangle`, `methodize`,
`symbolpreflight`, `tudiscover`, `dataorder`, `dataseams`, `poolseams` (+ `seams`), `splitcheck` (+ `invariants`), `dump_asm`,
`dataclaim`, `dataqueue`,
`sectiongap`, `pairgap`, `relocdiff`, `unitscore`, `symdiff`, `fnasm`, `immreloc`, `measure`, `unwindcut`, `vtslot`, `linkorder`, `m2cinput`,
`typeregistry`, `declclash`, `elfsect`, `dwarfmap`, `unitinfo`.

Agent plumbing: `claims`, `slots`, `lane`, `rescue`, `wtsafe`, `queue`, `lanecmd`, `worktreehook`, `landlog`, `brief` (+ `briefing`), `backlog`, `integrate`,
`tooling`, `mergebranch` (+ the package `merge`: `unionprose`, `unionresolve`, `unionguard`), `recordmerge`, `edit`, `escape`, `profileprobe`, `install`, `doclinks`.

Matching experiments: `infer`, `frame`, `optsweep`, `mwcc_matrix`, `tryvar` (+ `variants`), `shapesearch` (+ `shapes`), `slotmap`,
`mwcc-debugger`, `mwlink` (+ its CLI shim `mwlink_debugger`).

Dormant: `rso`. Template (dtk-template, no spec of ours): `project`, `ninja_syntax`, `download_tool`, `transform_dep`,
`decompctx`, `changes_fmt`. Retired: `retired.md`.

Each spec is `docs/tools/spec/<name>.md` with the fixed sections: Purpose, Users, CLI (flags, exit codes, JSON), Inputs and
outputs, Invariants and rules, Lib dependencies, Test contract, Known gaps.

## Use it when: the roster

One line per tool: what it is, and **use it when ...** (moved from `docs/pipeline.md` section 9 in WP6). Paths are
relative to `MAIN`; the contract of each tool is its spec.

### The lane, the claim and the environment

| tool | use it when … |
| --- | --- |
| `tools/units/claims.py` | **any lane starts**: claim a unit (`--pipeline` opts into the three-phase loop, §1) — the claim cuts a fresh branch off `main`'s tip and holds a slot for the whole loop; `release` returns the slot and **fails closed** (E2, §2.3). |
| `tools/units/slots.py` | **you need a lane directory**: `init` builds the six-slot pool, `status`/`verify` read the pool and prove a kept tree current — *a stale build tree is a lie* (§7), so reach for `verify` the moment a measurement is suspect; the reset is fail-closed. |
| `tools/units/queue.py` | **you are choosing the next item**: `next` picks a unit, takes a free slot and prints the paste-ready spawn line with the claim's own worktree as `cwd`; it refuses while all six slots are taken, and refuses a `cwd` that resolves to MAIN. |
| `tools/units/brief.py` | **you are launching a lane**: it generates `tools/units/briefs/<unit>.md`, the one file the worker reads — unit, inventory, residuals, decided items, the task, and the rules copied verbatim from the plan. *If the brief does not say it, it is not a rule.* |

### The gate, the merge and the landing

| tool | use it when … |
| --- | --- |
| `tools/units/land.py` | **you are about to put a batch on `main`**: its rows are the law (§4). `--units` takes units *or* batch paths; `--no-outbox` is for a bookkeeping row, never for a code refusal; its `--diff` grandfathers existing rule findings and refuses *added* ones. |
| `tools/units/integrate.py` (`land.py integrate`) | **lanes filed integrator requests**: classifies them, applies the mechanical ones (renames with their sweep, owner-header declarations, STOPGAP removal) on an `integrate/<date>` branch, builds once (narrowing a failed build per declaration), pre-runs the gate's drift/lint/undefrefs/vtable rows and prints the landing line (§10.10). `--dry-run` writes nothing. |
| `tools/units/mergebranch.py` | **a branch was cut before `main` moved**: `resolve` brings `main` in by **row replacement** (never a union, never a `src/**` union), with the rule-2 **address** sweep — it supersedes the manual merge procedure (§5). |
| `tools/units/flipcheck.py` | **you are deciding whether a unit may be flipped**: READY is **necessary, not sufficient**, and its relocation half does not run for an already-`Matching` unit (§8). |
| `tools/objdiff/relocdiff.py <unit> --by-owner` | **a unit scores ~100 % and you want to know its relocations are the target's**: aligns relocations per owning symbol and compares type, symbol **name** and addend against the target object, printing only differences (`N/N relocations match`, exit 1 on any; a moved function is not a difference). Without `--by-owner` it prints both sides' tables paired by offset. objdiff scores a `bl` to the wrong symbol as equal, so this sees a wrong callee, vtable slot or pool entry that no score does. It runs on any built unit, `Matching` or not - the relocation half `flipcheck` skips for a `Matching` unit (§8). |
| `tools/elf/objalign.py` | **a unit's claimed start is not 8-aligned**: clamp the compiled section's `sh_addralign` so mwld stops rounding the section up and shifting every later section (a build-step ELF fix). |
| `tools/elf/objextab.py` | **a `Matching` unit's exception tables need the map's names**: give the MWCC object the `@etb_…`/`@eti_…` names `dtk dol split` synthesises, so the link resolves once the unit is the only definition (a build-step ELF fix). |

### The measurement

| tool | use it when … |
| --- | --- |
| `tools/units/measure.py` | **you are searching**: score a whole unit in one compile and one `objdiff report`, printed as a per-symbol table. |
| `tools/units/recompile.py --measure` | **you are proving one symbol**: one symbol per run, usable from any worktree; resolution is invocation-first and prints the path it used with its kind (`[worktree-split]` / `[registered]` / `[auto-fallback]`). Together with `measure.py` it is the **one** implementation of the official metric (§7). |
| `tools/units/sectiongap.py` | **a flip is doubtful**: it prints each differing section **with its relocations**, not just a size — the `extab`/`extabindex` gaps `datagap.py` skips, and the right-size/wrong-offset record no size check can see (F39/F41; being built). |
| `tools/units/datagap.py` | **a unit reads 100 % but may not be the target**: it compares the *data* sections' sizes, so you see the extra `.sdata2`/string pool objdiff does not count. |

### The record, the map, the language

| tool | use it when … |
| --- | --- |
| `tools/units/symbolpreflight.py` | **before any source is written**: answer who owns an address and what a registration would touch — offline and bounded. |
| `tools/symbols/symedit.py` | **you need a map row**: query and surgically edit `symbols.txt` (~65 700 lines) without loading it into context; the map row and the source change together. |
| `tools/symbols/dumpmap.py` | **you want a rename candidate**: resolve `symbols.txt` names against the runtime dump's Dolphin map. |
| `tools/splits/tudiscover.py` | **you need a TU boundary**: propose the translation-unit seam around an address, offline, from the data-section referrer runs. |
| `tools/units/callees.py` | **you are about to write bodies**: name a unit's callees — every generated symbol its bodies reference, its owner, the call shape — before rule 7 bites. |
| `tools/units/callers.py` | **you need "who calls / who reads this"**: the whole-DOL caller index, **address-keyed** because the `asm/` dump is stale (rule 12's evidence tool; the session called it the single most useful recon tool). |
| `tools/units/dataclaim.py --unit U` | **rule 12 bit you (an `extern` of unowned data)**: every data symbol U references but does not own, its census sharers, and the exact `splits.txt` claim / named data-only unit to paste. Read-only; it never writes `splits.txt`. |
| `tools/units/unwindcut.py <unit> <cut>` | **you are re-cutting a seam** (moving a unit's right edge to a function boundary): the `extabindex`/`extab` partition at the cut with the sum check, the paste-ready lines for both halves, and the `.ctors`/`.dtors` words the cut obliges you to drop. Read-only; it refuses a cut that is not a function boundary. |
| `tools/units/langcheck.py` | **a unit's language is in question**: decide C vs C++ from evidence (a mangled definition, a `.cpp` `__FILE__` string), never from convenience. |
| `tools/units/movehdr.py` | **the header layout moves**: `include/P` -> `src/P` (plus an exception table), every quoted `#include` simulated in both layouts and both search orders first, refused on any target change; `--dry-run` plans, a moved tree reports nothing to do (`spec/movehdr.md`). |
| `tools/units/sweepcomments.py` | **comments carry stale paths or narrative history**: one comment-only pass per run (`--paths`, `--history`, `--fixes`, `--if0`), never code, strings or `#include` lines; `--list-stale` / `--markers` count what is left (`spec/sweepcomments.md`). |
| `tools/units/factscheck.py` | **a change deletes prose**: every fact token (address, name, size) a diff removes must survive in the file, `configure.py`, `splits.txt`, `symbols.txt` or `docs/`; `--explain` says where (`spec/factscheck.md`). |
| `tools/units/recordmerge.py` | **two lanes each hold a view of the same record header**: fold them into one definition with the checks the hand passes lacked. |

### The rules and the audits

| tool | use it when … |
| --- | --- |
| `tools/units/stylelint.py` | **you changed `src/`**: rules 1-9 and 11-15 with `file:line` (10 is vtableaudit's); `--diff <ref>` is the gate's add-only comparison (a recut's *moved* findings are credited one per removal from another file of the batch and printed as `moved`, a copy is not), `--budget` a debt read. |
| `tools/units/vtableaudit.py` | **a unit owns a code-pointer run**: find a vtable it owns but does not emit and a hand-written `+0x00` table store — rule 10 made mechanical, with `--diff` at the gate. |
| `tools/units/datagap.py --census --unit <unit>` | **before you report a unit**: the data its target object references that no claim covers (orphans, with neighbours, section and readers) plus the strict view - the unit's sole-owned pairs as `REFUSE` or `deferred <class>`; `dataclaim.py --unit <unit>` prints the `splits.txt` edit that claims the refusable ones. |
| `tools/units/lanecheck.py` | **before a review round**: the checks a reviewer did by hand over a branch's touched units - an owner cited by address, a gone path, "unowned" data that splits.txt owns, wrong callees and flipcheck blockers the header's RESIDUALS do not name, empty stubs, unmarked GUESS names; one `file:line \| class \| what \| hint` line each (`spec/lanecheck.md`). |
| `tools/units/declclash.py` | **a cross-unit lane hits `(10197) illegal function overloading`**: list the function names declared more than once with *different text* in one include closure, before any source is edited. |

### The registers and the suite

| tool | use it when … |
| --- | --- |
| `tools/units/backlog.py` | **you file or work the backlog**: one ranked register built from `.pi/outbox/*.json` and `.pi/notes/*.md`, with the credit ledger (one resolved item per new proposal claim). |
| `tools/units/tooling.py` | **you file a tooling gap**: turn the reports' "Tooling and environment" entries into the ranked `docs/tooling-requests.md`, clustered by demand. |
| `tools/selftest.py` | **before claiming green**: `--changed` maps `tools/**` diffs to selftests — a **docs** diff maps to **none**, so run `sync_profiles.py --check` and `sync_reference.py --check` by hand until the mapping lands (R7). |

### The compiler and the linker

| tool | use it when … |
| --- | --- |
| `tools/mwcc-debugger/` + `locate/verify_pcode.py` | **a body differs and the source shapes are exhausted**: drive `mwcceppc.exe` under gdb, dump the per-pass PCode, and classify which optimizer pass is responsible. |
| `tools/mwlink_debugger.py` | **the DOL hash moved after a flip**: interrogate `mwldeppc.exe` — trace / diagnose / align / phases. **In production** per its six-point gate: health-checked against `main.MAP`/`main.elf`, `main.elf` never written, 260/260 sampled inputs, with its own gaps note. |

**Debugger detail worth knowing.** `mwcc-debugger`: run `verify_pcode.py` on the last dump first (`MATCH` or a
real `FAIL`), then on the dump before the divergence for a `PASS-DELTA` that **names the pass**
(`after-peephole` for the `lbzu` fusion), and write that pass into the unit header's residual; a missing gdb is
one `fetch_gdb.py` line. `mwlink_debugger.py`: `trace` (kept/dropped, section addresses with bytes read back from
the ELF, symbol resolution, decoded relocations, ctor/dtor rank), `trace --link` (the build's own link line into
scratch, byte-identical to `main.elf`), `phases [--prove]`, `messages` (the linker's message catalogue lives in PE
RT_STRING resources - why `strings` finds nothing), `verify`. **No `mwldeppc.exe` carries a CodeView blob** (31
checked), so the compiler's lever is absent for the linker. Gaps channels: `.pi/notes/mwcc-debugger-gaps.md`,
`.pi/notes/mwlink-debugger-gaps.md` (§6).

### Audit and recovery

| tool | use it when … |
| --- | --- |
| `tools/units/rescue.py audit` | **you want to know if any `refs/rescue/*` holds unlanded work**: classifies `redundant` / `landed-with-drift` / `unlanded` / `unknown` (~40 s); `--prune` deletes only `redundant`. |
| `tools/units/verifyunit.py` | **a `Matching` claim needs a byte-level backstop**: address-aware comparison, since `report.json` carries no evidence for `Matching` units (§7). |
| `tools/units/backlog.py triage` | **the register may hold ghosts**: classifies open items resolved / stale / open from evidence and parks only what it can prove (§11). |
| `tools/units/ledger.py` | **you need a headline number**: covered / closed / matched, re-measured, never quoted from a doc. |

## How to add a tool

1. Write the spec first (`spec/_template.md`), including the test contract; name the lib modules it stands on. If a concept it
   needs is not in `lib/`, add it to the lib (with its contract test) rather than to the tool.
2. Create `tools/<area>/<name>.py` with the header below, the prologue line, a `lib.cli.Tool`, and functions that return
   `Finding`/`Row` values (never print inside the logic).
3. Create `tools/tests/<area>/test_<name>.py` with `TIER = "fixture"` on `lib.testing`; add a smoke test only for a check that
   genuinely needs the live tree, and make it skip (not fail) when its input is absent and never pin a count.
4. Add the tool to the spec index above and, if a profile or skill should call it, to that profile's prose.
5. `python tools/selftest.py --changed` green; `python tools/units/stylelint.py --diff main` is not for tools (it lints `src/`).

## The in-code header template

The only module-level prose a tool carries:

```python
"""<one-line purpose>. Spec: docs/tools/spec/<name>.md. CLI: <shape>."""
```

Example:

```python
"""Can our object fill every section the unit claims? Spec: docs/tools/spec/flipcheck.md. CLI: flipcheck.py [unit..]."""
```

A rule, an incident, a design note or a usage block does not go here; the spec holds them, and `tests/lib/test_headers.py`
refuses a module docstring longer than three lines or without the spec path.
