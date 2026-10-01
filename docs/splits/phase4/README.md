# Phase 4: the reconciled candidate goes into `splits.txt`, window by window

Owner decision (2026-10-01): phase 4 writes the candidate into `config/RMHE08/splits.txt` **and merges the source files of every folded or recut
registered unit into one file**, re-registered `NonMatching` (a `Matching` unit folded into a merged unit is demoted), so no registered unit is left without
a source and no unit in `splits.txt` is left unconstructible. Six window lanes, landed one window at a time. The tool is `tools/splits/applysplits.py`.

## What is here

| file | what |
| --- | --- |
| `manifest-<w>.md` / `.json` | the work list of window `w` (a, b, c, d, e, fg), generated against main at `0a83bf692` |
| `baseline-splits.txt` | the `splits.txt` the proposals were written against; the candidate is **always rendered over this file** (rendering over a half-applied tree names a unit twice) |
| `names.json` | the names a lane gave placeholder units; `plan`, `apply`, `manifest` and `verify` all read it |

A manifest is **state-dependent**: a component already applied is no longer in it. Regenerate it in the lane's tree
(`python tools/splits/applysplits.py manifest --window W --md build/tmp/phase4/W.md`) instead of trusting a committed copy after anything landed.

## The tool

```
python tools/splits/applysplits.py plan     --window W [--json OUT] [--check] [--dtk [--probe]]
python tools/splits/applysplits.py apply    --window W
python tools/splits/applysplits.py manifest --window W [--json OUT] [--md OUT]
python tools/splits/applysplits.py verify   --window W [--base REF] [--before-report F] [--no-build] [--allow-unregistered]
python tools/splits/applysplits.py all      [--dtk]            # every window, read-only: what committed here was produced by `all --dtk`
python tools/splits/applysplits.py --selftest
```

`W` is `a`, `b`, `c`, `d`, `e`, `fg` (text starts below 0x800E0000, 0x801C0000, 0x802A0000, 0x80380000, 0x80460000, the rest plus the `.init`-only runtime units) or
`0xLO..0xHI`. A **component** is a connected set of changed old blocks and changed candidate blocks (same name, or sharing bytes in any section); it goes to the window holding
its lowest `.text` start, a data-only unit takes the window of the unit before it in file order. `apply` writes `splits.txt` and nothing else, and applying the windows in any order
gives exactly the candidate (checked by `all`, and by the selftest on a fixture).

## Lane brief (one window = one lane = one landing batch)

Profile: `decompiler` for the source work (it is unit work: registration + bodies), launched by `slots.py spawn --kind unit` into a slot. Land windows **in order a, b, c, d, e, fg**:
a window checked alone can fail `order` where a neighbour window has not landed (windows c and e do), the cumulative state does not.

1. **Start**: tree cut from main with the earlier windows landed. `ninja build/RMHE08/report.json`, copy `build/RMHE08/report.json` to `build/tmp/phase4/W-before-report.json`
   (`verify` compares function scores against it). `python tools/splits/applysplits.py manifest --window W --md build/tmp/phase4/W.md` and read it whole.
2. **Names first.** Decide the final name of every unit under "Names and map rows to review" (a `fn_<addr>` stem for a unit with bodies is refused by the registration row; a stem at the
   wrong address, e.g. `ef/fn_800CDB2C` for the unit at 0x800CE5A8, is a rename; a `.cpp` name whose evidence says `.c` is a language decision, below) and put each in
   `docs/splits/phase4/names.json` (`{"names": {"<candidate name>": "<final name>"}}`). A name of a unit that is registered today is not an entry here; it is a `--unit-rename` at landing.
3. `python tools/splits/applysplits.py plan --window W --check --dtk` then `apply --window W`; `git diff --stat` must show `config/RMHE08/splits.txt` only.
4. **Per component, in manifest order**: for a *fold* or a *recut*, `git mv` the source named by "start from" (the survivor of the same name when there is one) to the final path, then bring in
   every other absorbed source's functions **in text order** (file order = text order; the manifest lists each source's functions with address and size, and "defined in the source" is a heuristic:
   a function it cannot find is simply not decompiled yet). Merge includes, move the headers the manifest names (`include/<module>/<stem>.h` and the files that include them) or keep them and add the
   include, delete the absorbed sources (`git rm`). A candidate with no registered source (a *new unit*, a *data-only* unit) gets a stub: a source file whose header comment says what it is, its `.text`
   range, why it sits there and what is unknown (CLAUDE.md step 3), never an unregistered source and never a registered unit without one.
5. **configure.py** (not the flags): remove the `Object(...)` rows of the absorbed units, add one `Object(NonMatching, "<name>")` where the first absorbed unit was, with the cflags of the absorbed
   unit it starts from (when absorbed units carry different `cflags=`, that is a finding to record in the unit's header, not a choice to make silently). A Matching unit in "Matching demotions" is
   `NonMatching` in the merged row. A Matching unit in "Matching units that gain data" either defines the data in its source and re-measures (playbook 23/29, `flipcheck` refuses a section that
   differs) or is demoted: say which in the unit header.
6. **Language**: a merged unit takes `.cpp` when any function in its range has a C++ mangled name or an absorbed source is `.cpp` (the manifest prints the count and an example); only `.c`
   sources and no mangled name means `.c`. The manifest lists every unit whose proposal name disagrees: rename it through `names.json`.
7. **Map rows** (rule 7): rename what the manifest lists through `python tools/symbols/symedit.py rename` and sweep the references in the same change; re-measure the owner and its consumers.
8. `python configure.py`, then `python tools/splits/applysplits.py verify --window W --before-report build/tmp/phase4/W-before-report.json`. Its rows: splits.txt equals the plan; every unit of the
   window is registered iff it has a source; every folded unit's source is gone and unregistered; every function appears once in its merged file; no name collides; no demoted unit is still
   `Matching`; every unit of the window compiles; `ninja build/RMHE08/ok` is green (read the `FAILED` count); no function scores lower than before. Fix, rerun.
9. `python tools/units/stylelint.py --diff main` adds no section 6.5 violation; `python tools/selftest.py --changed main` is green.
10. Commit on the lane branch (`game/<module>: ...` for the source, `config/splits: apply phase 4 window W` for the splits and names, one commit per concern, no rationale in the message). Do not land.

### Landing (the orchestrator)

One window is one batch: `land.py record-base`, then the batch from the manifest's last section:

```
python tools/units/land.py land --units <the manifest's --units list> --unit-rename OLD=NEW ... [--allow-regression UNIT] [--allow-orphan ADDR]
```

* **`--unit-rename`**: the pairs are printed by the manifest and follow `land.set_unit_renames`: several OLDs to one NEW (a fold), one OLD to several NEWs (a split source), `OLD=` when the
  unit's base entries go with it. A unit that keeps its name and also feeds another is a survivor (named in `--units`), not a rename.
* The **registration row** (`verifyunit`): every unit of `--units` needs an `Object(...)` row, a `splits.txt` block and a build target. The **data-closure row** is where a recut that leaves a claimed
  byte unclaimed, or a fold whose absorbed unit is not mapped by `--unit-rename`, is refused; `--allow-orphan` is a recorded escape, not a fix. **`--allow-regression`** is for a Matching unit
  that legitimately leaves with its functions. The row "a neighbour's split target object moved" fires for a unit this batch re-ranges: name it in `--units`.
* `docs/pipeline.md` section 4 is the full list of rows and their cures.

## The six windows, against main at `0a83bf692` (the numbers are in the manifests)

| window | components | old blocks -> candidate blocks | kinds (candidate units) | registered sources touched | Matching demotions / data gains | `--unit-rename` pairs | lane size (score) |
| --- | ---: | --- | --- | ---: | --- | ---: | --- |
| a | 62 | 68 -> 74 | 55 data gain, 15 recut, 4 fold | 68 | 0 / 2 | 12 | large |
| b | 39 | 76 -> 63 | 22 data gain, 27 recut, 14 fold | 76 | 2 / 5 | 66 | very large |
| c | 15 | 29 -> 19 | 8 data gain, 4 recut, 6 fold, 1 data-only | 29 | 1 / 0 | 22 | very large |
| d | 27 | 46 -> 39 | 14 data gain, 15 recut, 8 fold, 2 new | 46 | 2 / 3 | 32 | very large |
| e | 33 | 20 -> 35 | 11 data gain, 5 recut, 3 fold, 16 new | 20 | 1 / 1 | 5 | large |
| fg | 59 | 19 -> 76 | 9 data gain, 17 recut, 5 fold, 45 new | 19 | 1 / 0 | 24 | large |

Totals: **11** Matching units gain data and **7** are demoted (`matchinggain.py`'s 11 + 7: the two lists are reproduced exactly). "Lane size" is a heuristic printed in each manifest (functions in the
sources a unit takes over + 6 per unit that is not a data gain + registered sources touched).

### What dtk says (`dtk dol split --no-update` on the edited file, the real acceptance test)

| window | alone | with the blockers given an owner (`probe`) | cumulative a..W with the blockers given an owner |
| --- | --- | --- | --- |
| a | **refused**: `Invalid alignment for split: auto_08_80673C0A_bss` | accepted | accepted |
| b | accepted | accepted | accepted |
| c | accepted | accepted | accepted |
| d | **refused**: `Unsplit data in .sdata from lobby/lb_companion_ui.cpp 0x8079308A to next split 0x8079308C` | accepted | accepted |
| e | **refused**: `Invalid alignment for split: menu/fn_804513FC.cpp .sbss 0x80794D65` | accepted (one `.sdata2` gap given away) | accepted |
| fg | **refused**: `Invalid alignment for split: auto_08_80708076_bss` | accepted | accepted |

The candidate itself is not dtk-clean, and the reasons are decisions, not rules:

* **Alignment padding (fixed mechanically, listed in each manifest as "alignment fixes")**: 18 data ranges end where the last symbol ends, off the 4-byte alignment, in front of a gap; dtk refuses the
  `auto_*` unit that would start there. The registered file has 180 gaps and none unaligned. `align_candidate` extends the end over the padding when no map symbol starts in it (plan, apply and verify
  all use the aligned candidate).
* **Six boundaries the tool cannot move** (an unowned symbol that is not 4-aligned begins exactly there): `g3d/g3d_anmchr.cpp .bss` 0x80673C0A (`lbl_80673C0A`, 0xF276 bytes), `ARC/arc.cpp .bss` 0x80708076,
  `NHTTP/d_nhttp.c .bss` 0x8076317A, `homebutton/fn_80556FC4.cpp .bss` 0x8079082A (the second element of an array of 0x5A-byte objects), `lobby/lb_companion_ui.cpp .sdata` 0x8079308A (a 2-byte object),
  `menu/fn_804513FC.cpp .sbss` 0x80794D65 (a 5-byte object). Each needs an owner for the symbol: an `attach` override in `phase2-overrides.json` and a regenerated `phase2-reconcile.json`
  (phase 3 work, not a lane's). The probe gives each to the unit before it and dtk accepts the whole candidate.
* **One gap dtk will not turn into an `auto_*` unit**: `.sdata2` 0x8079C78C..0x8079C790 between `Network/NetworkSessionManagerPat.cpp` and `Network/NetworkCommunityPat.cpp` (4 bytes of a literal nobody owns:
  `Unsplit data`). Found only by running dtk; the rule that makes this one gap special and the other small `.sdata2`/`.sbss` gaps fine is not known.

Until those seven are decided, `ninja build/RMHE08/ok` (which splits with the real config) fails for windows a, d, e and fg after `apply`. A window lane that meets one reports it and stops; it does not guess an owner.

## Problems the plan cannot express (all windows; the per-window lists are in the manifests)

* Components that span windows (applied with the lowest): `sound/fn_800D7F54` (a+b; `sound/fn_800DD1F0.cpp` crosses 0x800E0000), `Pl/pl_act_step` (b+c; `enemy/fn_801BD6C0.cpp` crosses 0x801C0000),
  `enemy/em019_ai` (d+e; `enemy/em019_ai.cpp` and `enemy/em_act_mot.cpp` cross 0x80380000), `menu/menu_item.cpp` crosses 0x802A0000, `MSL_C/alloc.cpp` crosses 0x80460000.
* Language: units that merge `.c` and `.cpp` sources (`enemy/em_common`, `em_kind`, `em001_prog`, `stage/stg_w`, `ai/ai_npc`; they take `.cpp`), and units the proposals named `.cpp` whose evidence says
  `.c` (`AX/AXFXReverbHiExp`, `MTX/mtxvec`, `MTX/mtx44`, `MTX/vec`, `NAND/nand`: cut from `.c` sources, no mangled function). A rename in `names.json` settles each.
* The seams `phase3-notes.md` lists as open stay open (network_opening Z/W, `pl_act | fn_802840DC`, `lobby/fn_801E0ADC`, `ef_effect` two TUs, ...): the candidate does not cut them, so the manifest does not either.
* Rule 7 review items (each manifest lists its own): three map rows named after a unit they do not lie in (`AXFXReverbHiExpShutdown` 0x804760C0, `quest_entry_notify` 0x803AB0FC,
  `quest_item_slots_prune` 0x803B22E8); 14 units whose `fn_<addr>` stem is not their `.text` start (`ef/fn_800CDB2C`, which is the unit at 0x800CE5A8 and the `ef_sphere` swap, `sound/fn_800DD1F0`,
  `enemy/fn_801550FC`, `fn_8019ED34`, `fn_801A9540`, `fn_801BD6C0`, `fn_801CA004`, `fn_801CCBC4`, `fn_801D80EC`, `fn_80387844`, `THP/fn_804DF200`, `nw4r/fn_80501CE8`, `homebutton/fn_80542D8C`,
  `fn_8055B710`); six `ef/eftNNN_fx` name-run remnants (`eft004`, `eft007`, `eft013`, `eft022`, `eft026`, `eft029`); `ef/ef_drawstripestrategy_base` (named `_base` so as not to collide with the
  registered `ef_drawstripestrategy`); 60 more `fn_<addr>` stems of new units.
* A window cannot keep the build green with units whose bodies do not exist unless each gets the stub of step 4; there are 63 new units (d 2, e 16, fg 45) plus the tails cut from registered
  units, and the stubs are most of the fg lane.
