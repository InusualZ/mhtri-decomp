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
6. **Language**: the ten units the first version flagged are decided below (`names.json` `languages`, already applied to the extension in `splits.txt`); for any other merged unit the extension is
   `.cpp` when a function in its range has a C++ mangled name or an absorbed source is `.cpp` (the manifest prints the count and an example), `.c` for only `.c` sources and no mangled name. A unit
   the manifest flags with a language problem is decided through `names.json` `languages` (final stem -> extension), never by renaming the file alone.
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

Every window is **accepted** by plain dtk, alone and cumulatively (`all --dtk` runs the real splitter on each window applied to the current file and on windows a..W applied in order; no probe, no
blocker given to a neighbour: the `probe` diagnostic of the first version runs only when plain dtk refuses, and is absent from every manifest now).

| window | alone | cumulative a..W |
| --- | --- | --- |
| a | accepted | accepted |
| b | accepted | accepted |
| c | accepted | accepted |
| d | accepted | accepted |
| e | accepted | accepted |
| fg | accepted | accepted |

Before the seven decisions below, a, d, e and fg were refused alone (`Invalid alignment for split: auto_08_80673C0A_bss`, `Unsplit data in .sdata from lobby/lb_companion_ui.cpp 0x8079308A ...`,
`menu/fn_804513FC.cpp .sbss 0x80794D65`, `auto_08_80708076_bss`) and the cumulative state was accepted only with the seven given to a neighbour. `plan --window W --dtk` prints the same verdict per window.

* **Alignment padding (fixed mechanically, listed in each manifest as "alignment fixes")**: data ranges that end where the last symbol ends, off the 4-byte alignment, in front of a gap; dtk refuses the
  `auto_*` unit that would start there. `align_candidate` extends the end over the padding when no map symbol starts in it (plan, apply and verify all use the aligned candidate).
* **What dtk refuses (measured)**: a split off a 4-byte boundary, and the 2-byte `.sdata` and 4-byte `.sdata2` gaps between two units (the other small gaps of the registered file are accepted: the rule that
  separates them is still not known, so a gap that dtk refuses is given to a unit, never left). A TU's section starts aligned to its largest object, so a symbol that starts off the word is packed behind
  the previous TU's last object (the previous unit takes it and its padding), except where the unit that *follows* holds word-sized objects (then it starts on the word before the byte run, item 6).

### The seven boundaries, decided (`docs/splits/proposals/phase2-overrides.json` rows 31-38, orchestrator `attach` rows; `phase2-reconcile.json` regenerated, byte-reproducible)

| # | range | owner | grade | evidence |
| --- | --- | --- | --- | --- |
| 1 | `.bss` 0x80673C0A..0x80682E80 (`lbl_80673C0A`, 0xF276 B, no reader) | `g3d/g3d_anmchr.cpp` | medium | starts where g3d_anmchr's last object `lbl_8066AE80` (0x8D8A B) ends, next unit `g3d_state` starts aligned at 0x80682E80 |
| 2 | `.bss` 0x80708076..0x8070C414 (`lbl_80708076`, 0x439E B, no reader) | `ARC/arc.cpp` | medium | packed behind `lbl_807046E0` (read by arc); ends on a word; `lbl_8070C414` (0x9CC B) stays deferred (aligned both sides) |
| 3 | `.bss` 0x8076317A..0x80763900 (`lbl_8076317A`, 0x786 B, no reader) | `NHTTP/d_nhttp.c` | medium | packed behind `NHTTPi_systemInfo` (0x80762C60 + 0x51A); `ssl` starts aligned at 0x80763900 |
| 4 | `.bss` 0x807907D0..0x80790D10, **the two arrays** (12 x 0x5A B and 12 x 0x16 B) | `homebutton/fn_80556FC4.cpp` | medium | one object each, so decided as arrays, not at the edge: their only code readers (`fn_80557744` takes both bases, `fn_80558670`, `fn_805586D4`) are in `fn_80556FC4`'s text; the only other evidence is the pointer table `lbl_80655A98` of `fn_8055FB70` (weight 1) that lists the elements. Residual: `lbl_80790250` (0x580 B, read by the `fn_8055B710` candidate) lies before the first array in link order and stays deferred, as before |
| 5 | `.sdata` 0x8079308A..0x8079308C (`lbl_8079308A`, 2 B, no reader) | `lobby/lb_companion_ui.cpp` | medium | packed behind the 2-byte `lbl_80793088` (read by lb_companion_ui); `eft050` starts aligned at 0x8079308C |
| 6 | `.sbss` 0x80794D60..0x80794D64 (`lbl_80794D60`, read only as an extern by homebutton code) and 0x80794D64..0x80794D65 (`lbl_80794D64`) | `menu/menu_plsearch.cpp` (D60, with its padding) and `menu/fn_804513FC.cpp` (D64) | medium / strong | the two 1-byte objects leave 3 bytes between them: one TU packs bytes without padding, so they are two TUs' and the padding is the previous unit's tail; `fn_804513FC` holds 4-byte objects (from 0x80794D6C), so its `.sbss` starts on a word, 0x80794D64, never 0x80794D65 |
| 7 | `.sdata2` 0x8079C78C..0x8079C790 (`lbl_8079C78C`, 4 B, a shared scalar) | `Network/NetworkSessionManagerPat.cpp` | medium | read by `fn_803E1B2C` (0x803E1B74, Session's text) and `fn_803EAF14` (0x803EAFF0, Community's text); the first use in text order is Session's and its neighbour `lbl_8079C780` is read the same way and already sits in the Session block; a 4-byte gap is what dtk refuses, so a gap cannot stay unowned |

Effect, measured on the reconciled candidate (`splitcheck`, phase 1 + `phase2-reconcile.json` + `phase2-folds.json`): lint 0, **pool 75 -> 75** (all other invariants 0), the candidate is 354 units as before, 935
attachments (928 before), `unowned_data` 116 rows (123 before); the overrides are 39 rows, 35 applied, 2 deferred, 2 blocked by `order` (the same four as before). Moving item 7 to `NetworkCommunityPat` was not tried:
the pool count already equals the baseline and Session is the first reader.

### Language decisions (`names.json` `languages`: final unit stem -> extension, read by `applysplits.py`; the symbols decide)

The rule (docs/plan.md section 12): a C++ mangled map name proves the TU is C++, and one mangled function in a range is enough because the unit is one compile (its unmangled neighbours are
placeholder `fn_<addr>` names or `extern "C"`); unmangled names with C linkage are `.c`; a fold of `.c` and `.cpp` members stays `.cpp` only when the range holds a mangled function.

| unit | decision | evidence |
| --- | --- | --- |
| `enemy/em_common` | `.cpp` | 19 mangled functions in 514 (`get_enemy_data__FP11_ENEMY_WORK`, `em_work_die_ck__FP11_ENEMY_WORK`, ...) |
| `stage/stg_w` | `.cpp` | 6 mangled (`get_stg_w__Fv`, `get_stg_weapon_work__FP4_PLWPl`, ...) |
| `ai/ai_npc` | `.cpp` | 8 mangled (`ai_area_ck__FP8_AINPC_W`, ...); the Matching `.c` member `ai/fn_802D0DCC` is demoted in the fold |
| `enemy/em_kind` | `.c` | no mangled function in 123; the real name in it (`em_kind_release`) is unmangled; members are `.c` (78 functions) and a `.cpp` registered by default (45, none mangled). Weakest decision: only the absence of a mangled name backs it, a lane that finds a C++ construct in the bodies flips it in `names.json` |
| `enemy/em001_prog` | `.c` | no mangled function in 124 (real names `em_res_user_data_ctor`, `em_spawn_rec_init` unmangled); the absorbed Matching unit `enemy/fn_80149D6C.c` is a `.c` that matches as C |
| `AX/AXFXReverbHiExp` | `.c` | 8 functions, none mangled, an SDK C library (`AXFXReverbHiExpInit`, absorbed source `AX/AXFXReverbHi.c`) |
| `MTX/mtxvec`, `MTX/mtx44`, `MTX/vec` | `.c` | SDK C library, unmangled `PSMTXMultVec`, `C_MTXOrtho`, `PSVECNormalize`, `PSVECMag`, `PSVECDotProduct`; the absorbed source is a `.c` |
| `NAND/nand` | `.c` | 341 functions, none mangled; absorbed source `OS/OSAlarm.c` is a `.c` |

The manifests no longer list a "language" problem for these ten; the unit's extension in `splits.txt` (and so its source file, registered `Object(...)` row and `--units` entry) is the decided one.

### Components that span windows, and units that cross a window edge (decided)

A component belongs wholly to **one** window: the one holding its lowest `.text` start (a data-only unit takes the window of the unit before it in file order). That window's lane lands **every** unit of
the component - the recut and fold results, their sources, the data-only units' removal - **including the candidate units whose text lies in a later window's address zone**. A later lane **must not touch
those units, their sources, their `configure.py` rows or their `splits.txt` blocks**, and starts at the first unit after the component's last range. The manifests already reflect this (a later manifest does
not list the component; `applysplits.py all` checks that the windows applied in turn equal the candidate).

| component (window that lands it) | units landed there | what a later window must not touch |
| --- | --- | --- |
| `sound/fn_800D7F54` (**a**; spans a+b: old `fn_800DD1F0` is 0x800DD1F0..0x800E3CBC and crosses 0x800E0000) | `sound/fn_800D7F54` (fold of 3, 0x800D7F54..0x800DD40C), `sound/fn_800DD1F0` (0x800DD40C..0x800E0504), `sound/mhchar` (0x800E0504..0x800E3B3C), `sound/sound_job` (0x800E3B3C..0x800E3CBC); sources `src/sound/fn_800D7F54.cpp`, `fn_800DD1F0.cpp` (survivor of the three recuts), `mhchar.cpp`, `sound_job.cpp` | window b: all four units and sources; b's first sound unit is `sound/fn_800E3CBC` (0x800E3CBC, data gain) |
| `Pl/pl_act_step` (**b**; spans b+c because the two data-only units `Pl/pl_frame_data`, `Pl/pl_act_data` sit in b and the code in c's zone) | `Pl/pl_act_step` (fold of 6 units, 0x802430E8..0x802673A4, with the data-only units' literals), `Pl/player_control` (recut, 0x802673A4..0x802693C4); sources `src/Pl/pl_act_step.cpp`, `src/Pl/fn_80262940.cpp`; `src/Pl/pl_frame_data.*` and `pl_act_data.*` removed | window c: both units and sources; c starts at 0x802693C4 |
| `enemy/em019_ai` (**d**; spans d+e, `em019_ai` and `em_act_mot` cross 0x80380000) | `enemy/em019_ai` (fold of 4, 0x80378F9C..0x80383148), `enemy/fn_80383148` (recut, 0x80383148..0x80385EE0), `enemy/fn_80385EE0` (recut, 0x80385EE0..0x803868DC); sources `src/enemy/em019_ai.cpp` and `src/enemy/fn_80382310.cpp` (split in two: both recuts come from it) | window e: both recut units, the source `fn_80382310.cpp`, and the two `.bss`/`.data` rows overrides 9 and 10 give them; e starts at 0x803868DC |
| `enemy/fn_801BD6C0` (**b**; crosses 0x801C0000, one unit, data gain) | the whole unit 0x801BB758..0x801C29F8 | window c: this unit; c starts after 0x801C29F8 |
| `Pl/pl_coll` and `menu/menu_item` (**c**; `menu_item` 0x80297E34..0x802A6624 crosses 0x802A0000) | `Pl/pl_coll` (fold of 2, 0x8028F66C..0x80297E34), `menu/menu_item` (fold of 2, to 0x802A6624); sources `src/Pl/fn_8028F66C.cpp`, `src/menu/menu_item.cpp` | window d: `menu/menu_item`; d starts after 0x802A6624 |
| `MSL_C/alloc` (**e**; new unit 0x804578FC..0x804642C8 crosses 0x80460000) | the whole new unit and its stub | window fg: `MSL_C/alloc`; fg's first unit follows 0x804642C8 |

## Problems the plan cannot express (all windows; the per-window lists are in the manifests)

* Components that span windows and units that cross a window edge: decided above ("Components that span windows"); the lowest window lands the whole component.
* Language: decided above (`names.json` `languages`) for the ten units the first version flagged.
* The seams `phase3-notes.md` lists as open stay open (network_opening Z/W, `pl_act | fn_802840DC`, `lobby/fn_801E0ADC`, `ef_effect` two TUs, ...): the candidate does not cut them, so the manifest does not either.
* Rule 7 review items (each manifest lists its own): three map rows named after a unit they do not lie in (`AXFXReverbHiExpShutdown` 0x804760C0, `quest_entry_notify` 0x803AB0FC,
  `quest_item_slots_prune` 0x803B22E8); 14 units whose `fn_<addr>` stem is not their `.text` start (`ef/fn_800CDB2C`, which is the unit at 0x800CE5A8 and the `ef_sphere` swap, `sound/fn_800DD1F0`,
  `enemy/fn_801550FC`, `fn_8019ED34`, `fn_801A9540`, `fn_801BD6C0`, `fn_801CA004`, `fn_801CCBC4`, `fn_801D80EC`, `fn_80387844`, `THP/fn_804DF200`, `nw4r/fn_80501CE8`, `homebutton/fn_80542D8C`,
  `fn_8055B710`); six `ef/eftNNN_fx` name-run remnants (`eft004`, `eft007`, `eft013`, `eft022`, `eft026`, `eft029`); `ef/ef_drawstripestrategy_base` (named `_base` so as not to collide with the
  registered `ef_drawstripestrategy`); 60 more `fn_<addr>` stems of new units.
* A window cannot keep the build green with units whose bodies do not exist unless each gets the stub of step 4; there are 63 new units (d 2, e 16, fg 45) plus the tails cut from registered
  units, and the stubs are most of the fg lane.
