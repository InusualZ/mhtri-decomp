# Phase 2 reconciliation: one set of data attachments

Files: `phase2-reconcile.json` (the whole-DOL `attach` + `unowned_data` set, written by `tools/splits/dataattach.py`), `phase2-folds.json` (the text folds,
the data-only unit and the move phase 2 accepted). They replace the six lane files (`phase2-p2a.json` .. `phase2-p2fg.json`, `-p2a-sdata`, `-p2b-outside`,
`-p2b-merge-c`, `-p2d-folds`), which are in git history (`49be9c958`, `b9ec27b02`, `3579c953f`); the six `phase2-p2*.md` lane reports stay as the lanes'
accounts of their own runs (their commands name the lane engines, which are gone).

```
python tools/splits/dataattach.py --lane p2 --linker --overrides docs/splits/proposals/phase2-overrides.json --out docs/splits/proposals/phase2-reconcile.json     # deterministic, ~30 s
python tools/splits/splitcheck.py --proposal docs/splits/proposals/phase1-{a..g}.json --proposal docs/splits/proposals/phase1-reconcile.json \
    --proposal docs/splits/proposals/phase2-reconcile.json --proposal docs/splits/proposals/phase2-folds.json --emit-splits OUT
python tools/splits/dataattach.py --explain <unit regex | address> [--section S]    # the `reproduce` of every row
python tools/splits/dataattach.py --holdout                                         # the solver's precision on the registered data
```

## 1. Result

Candidate = phase 1 files + `phase1-reconcile.json` + `phase2-reconcile.json` + `phase2-folds.json`: lint 0, warnings 0, 354 units (360 in phase 1: the folds F1-F4 remove 6 units, the data-only unit F5 adds 1, section 4; the sinit fold of `Pl/bss_pool` into `Pl/pl_coll` removes one), 0 new failures outside the proposal units.
Regenerated 2026-10-01 after the phase 3 engine and checker fixes (section 7); the numbers below are the regenerated file's.

| invariant | FAIL phase 1 candidate | FAIL + phase 2, before the fixes (old checker) | same file, new checker | FAIL + regenerated phase 2, new checker |
| --- | ---: | ---: | ---: | ---: |
| coverage, text-cut, extab, ctors, dtors, vtable, bss, local-static | 0 | 0 | 0 | 0 |
| order | 0 | 0 | 2 (`Pl/pl_frame_data`, `Pl/pl_act_data`) | 0 (the engine moves the two units) |
| pool | 63 | 63 | 66 | 81 (18 accepted numeric/string inversions, section 7) |
| data-order | 0 | 0 | 0 | 0 |
| jumptable | 1 | 1 | 0 | 0 |

(Phase 1 candidate under the current checker: `data-order` is 0 because of the V->S rule of `docs/splits-program.md`; it was 3 under the old one.) One FAIL
pair moved: `enemy/em_pl_frame.cpp` `pool` became `hud/pl_frame_sync.cpp` `pool` (the fold keeps the unit's open pooldup question).

Rows: 881 ranges attached (strong 751, medium 130), 28 `guess` ranges kept next to their deferral and never applied, 139 `unowned_data` rows, 3 linker-generated;
2 `moves` (derived), 32 `open_questions`.

| section | ranges | symbols | bytes | strong | medium |
| --- | ---: | ---: | ---: | ---: | ---: |
| `.rodata` | 54 | 265 | 40,676 | 45 | 9 |
| `.data` | 235 | 5,968 | 581,201 | 178 | 57 |
| `.sdata` | 169 | 2,068 | 13,716 | 123 | 46 |
| `.sdata2` | 239 | 6,794 | 30,080 | 237 | 2 |
| `.bss` | 96 | 461 | 1,106,880 | 88 | 8 |
| `.sbss` | 84 | 895 | 4,248 | 77 | 7 |
| `.sbss2` | 2 | 3 | 24 | 1 | 1 |
| `extab`, `extabindex` | 1 + 1 | 5 + 5 | 100 + 60 | 2 | 0 |

Symbols in no unit, whole map (`splitcheck` coverage gaps), before phase 2 -> after: `.data` 8,454 -> 1,843, `.sdata2` 6,802 -> 8, `.sdata` 2,052 -> 55,
`.bss` 439 -> 34, `.sbss` 904 -> 35, `.rodata` 277 -> 12, `.sbss2` 3 -> 0 (previous regeneration: 2,310 / 86 / 213 / 29 / 37 / 38 / 2); `.init` (2) and `extabindex` `_eti_init_info` (1) are linker tables nobody owns
(`unowned_data` kind `linker-generated`). The six lane files together attach 17,675 symbols (`.data` 7,391, `.sdata2` 6,753, `.sdata` 1,974, `.bss` 414, `.sbss` 872,
`.rodata` 259); the reconciled set attaches 15,722 (`.data` 5,501, `.sdata2` 6,716, `.sdata` 1,910, `.bss` 452, `.sbss` 893, `.rodata` 239) and 965 `.data` symbols
more belong to the data-only unit of section 3.

## 2. The lanes against each other

The six lane files never contradict one another: 2,372 symbols were attached by two lane files and every one of them to the same unit.

| overlap (lane files) | symbols | section | what it is |
| --- | ---: | --- | --- |
| `p2b-outside` + `p2c` | 1,113 | `.sdata2` | the 3,422-literal pool run 0x807966F0..0x80799E00, the part after 0x80798FE8 (`enemy/fn_801CA004` ..) |
| `p2a` + `p2b` | 532 | `.data` 311, `.sdata2` 139, `.sbss` 51, `.bss` 26, `.rodata` 5 | the five shared runs: `.data` 0x80595840..0x80597C80, the 616-literal `.sdata2` run (0x80796438 on) |
| `p2e` + `p2fg` | 460 | `.sbss` 194, `.bss` 83, `.rodata` 73, `.sdata` 53, `.data` 47, `extab` 5, `extabindex` 5 | the runs after `MSL_C/alloc`: `.rodata` 0x80572438.., `.data` after `Gecko_ExceptionPPC`, the five `alloc` extab rows |
| `p2a-sdata` + `p2c` | 195 | `.sdata` | the start-of-link-order `.sdata` run, the part before 0x807922A0 |
| `p2a-sdata` + `p2b` | 72 | `.sdata` | the same run, 0x80791450..0x80791AD8 |

The engine decides each run once over its whole chain and writes a row only for a unit once, so none of this can recur. The boundaries the lanes named, as decided
(owner of the last symbol before, first symbol from the boundary; readers decoded from the retail text):

| boundary | before | from the boundary | evidence |
| --- | --- | --- | --- |
| `.sdata2` 0x80798FE8 (p2b / p2c) | `enemy/fn_801BD6C0` | `enemy/fn_801CA004` | `lbl_80798FE8` is read 43 times, all by `fn_801CA004`; the last literal before it is read only by `fn_801BD6C0` |
| `.sdata` 0x807922A0 (p2c / p2d) | `menu/menu_item` | `menu/menu_message` | `menu_fmt_int` read 3 times, all `menu_message`; `lbl_8079229C` only `menu_item` |
| `.sdata2` 0x8079A3F8 (p2c / p2d) | `menu/menu_item` | `menu/menu_message`, then `stage/shell` | `lbl_8079A3F8` read only by `menu_message`, `lbl_8079A400` only by `shell` |
| `.sdata2` 0x8079BFF8 (p2d / p2e) | `enemy/fn_80385EE0` | `enemy/fn_80387844` | `lbl_8079BFF8` read 228 times by `fn_80387844` alone (p2d gave the units of the d/e boundary, p2e deferred them: now one row each) |
| `.sdata2` 0x8079C330 (p2d / p2e) | `menu/menu_result` | `menu/multi_result` | `lbl_8079C330` read only by `multi_result` |
| `.bss` 0x806C23E8..0x806C2418 (p2d / p2e) | `enemy/em019_ai` (2 symbols, medium) | `enemy/fn_80383148` from 0x806C2418 | `lbl_806C23E8` read twice by `em019_ai`, `lbl_806C2418` 19 times by `fn_80383148`; the neighbour `lbl_806BF530` (0x2EB8 B, read by 20 units) stays open |
| `.rodata` 0x80572438 (p2e / p2fg) | `Runtime.PPCEABI.H/ptmf.c` | `Runtime.PPCEABI.H/runtime.cpp`, then `MSL_C/alloc` | `lbl_80572438` read only by `runtime.cpp`; one row per unit in the reconciled file |
| `.data` 0x80595840 (a / b) | `g3d/g3d_xsi` | `g3d/g3d_basic` | `lbl_80595840` read 3 times by `g3d_basic` |
| `.data` 0x80597C80 (a / b) | `sound/fn_800DD1F0` | `sound/mhchar` | `jumptable_80597C80` read by `mhchar` |
| `.sdata2` 0x80796438 (a / b) | `sound/fn_800DD1F0` | `sound/mhchar` | `lbl_80796438` read 29 times by `mhchar` |

`p2a-sdata`, `p2b-outside` and `p2b-merge-c` are dropped as files: the first two are rows of units outside their lane's window and the engine writes them with their unit's
window, the third is fold F3 below.

## 3. What the reconciled set does not carry from the lane files

Per symbol, the lane files' rows against `phase2-reconcile.json`: 14,976 symbols the same, 445 the same data under a fold-renamed unit or the homebutton rows below,
2,254 only in the lane rows, 301 only here (runs a lane left to its neighbour's window). The 445: 181 symbols of homebutton `.data`/`.rodata` that p2fg gave
to one guess block (`fn_80566440`, `fn_8053E808`, medium) and the engine gives to the unit that reads them, 264 symbols of folded units whose lane rows used the
unit before the fold (`hud/pl_frame_sync`, `ef/fn_800FD864`, `lobby/fn_801EC9F8`; the same bytes). No strong lane row flips to another real unit.

The 2,254 lane-only symbols (1,056 of them strong) are held back for these reasons:

| held back as | symbols | what holds them | units (symbols) |
| --- | ---: | --- | --- |
| `multi-tu`: attaching them makes an invariant FAIL that phase 1 does not (settle loop) | 1,128 | `data-order` 14 blocked rows (a V->S or zigzag seam inside a merged guess unit), `jumptable` 2, `vtable` 1 | `NetworkCommunityPat` 366, `homebutton/fn_80533474` 199, `network_layer_io` 188, `homebutton/fn_80566440` 79, `lb_server_sel_trans` 73, `g3d/fn_80075DCC` 57, `ef/eft019` 48 (`jumptable_8059FE90` is read by `eft001`), `NetworkSessionManagerPat` 43 |
| `interleave`: the readers contradict the place link order gives (static symbol, or three symbols of one pair) | 784 | the data of two units alternates | `sound/fn_800EF7D8` 375 (with `quest_snd`), `Pl/fn_802840DC` 81 and `Pl/pl_act` 55, g3d (`fn_80063888` 73, `g3d_resmat` 68, `fn_800680CC` 58, `g3d_resanmlight` 31), `ef/eft001` 30 |
| `pool-order`: a block that goes down in first-use order is cut back to its longest in-order stretch | 141 | idea 94 | `Pl/player_control` 23, `RVLGX/GXTexture_tail` 20, `hud/cockpit_quest` 17, `menu/get_pop_dat_ptr` 17, `mh3_pad` 11, `stage/stg_w` 8, `ef/ef_util` 6 |
| `unread`, `ambiguous` | 201 | no reader decides between the candidates | `ef/eft_slot` 117, `homebutton/fn_80566440` 55, `homebutton/fn_8053E808` 26 |

## 4. Folds

Each scenario is rendered with the engine re-run on it (`dataattach.py --proposal phase1-* --proposal <scenario>`) and compared with the reference (phase 1 + the
unfolded reconcile rows, 359 units): the FAIL count per invariant, the FAIL (unit, invariant) pairs and the FAIL **findings** (a finding the reference does not
have is a new failure even when the count stays the same).

| fold | what | units | new FAIL findings | symbols attached (reference 16,436) | verdict |
| --- | --- | ---: | ---: | ---: | --- |
| F1 `hud/pl_frame_sync` (p2d) | `enemy/em_pl_frame` + `hud/net_char_sync`, 0x8033041C..0x80338808, removes the cut 0x80334568 | 358 | 0 (the old `em_pl_frame` `pool` pair becomes `pl_frame_sync`'s, same findings) | 16,507 | **applied** (`phase2-folds.json`); the unit still holds two TUs by the pool (0x41F00000 at 0x8079B218 and 0x8079B258), second TU in (0x80330CEC, 0x8033129C] |
| F2 `cockpit_icon_data` into `lobby/lb_companion_ui` (p2d) | data-only unit whose data lies between `lb_companion_ui`'s on both sides; its `.data` (62 symbols, strong) and `.sdata` (71, medium) go to `lb_companion_ui`, the unit leaves | 359 | 0 | in the reference | **applied** as `fold-data-only` attach rows with `takes_from` (`phase2-reconcile.json`) |
| F3 `fn_800FD864_fx` (p2b merge-c) | `ef/fn_800FD864` + `fn_800FE978` + `eft004`, 0x800FD864..0x80100448, removes the cuts 0x800FE978 and 0x800FF8D4 | 357 | 0 | 16,515 | **applied** |
| F4 `lobby/fn_801EC9F8_merged` (p2c) | `lobby/fn_801EC9F8` + `fn_801F3294` + `fn_801F9CD4`, 0x801EC9F8..0x801FBF78, removes the registered cuts 0x801F3294 and 0x801F9CD4; the 55 deferred symbols of p2c are attached | 357 | 0 | 16,530 | **applied** |
| F5 `stage/stage_res_data` (p2a) | data-only unit for `.data` 0x80582EF8..0x8058AC50 (965 resource-name symbols read by `stage/stg_w` 826, `ef/effect` 113, ..), `after: fn_80047398.cpp` | 360 | 0 | 15,478 | **held in phase 3** (removed from `phase2-folds.json`): 951 of the 965 symbols are read by nobody, 14 `.sdata` objects of draw_shape's run are pointer-linked with the block, 321 are tables read by 7 unrelated units; the cluster recut of `phase3-notes.md` (E1/E2) gives the block to `main/draw_shape` |
| F6 move `Network/network_shared_data` (p2e) | after `lobby/lb_server_sel_trans`, where its `.sdata`/`.sdata2` sit between that unit's data and `NetworkSessionManagerPat`'s | 359 | 0 | 16,436 | **applied** (`moves`; file order is link order); phase 3: **dissolve at registration (phase 4)**, six text units read it and its `.sdata`/`.sdata2` split by reader (`phase3-notes.md` E8) |
| F7 `fn_80047398` + `fn_8004C9A0` + `fn_8004CAD8` (p2a) | one TU over 0x80047398..0x80054C64 | 357 | **6**: the double 0x4330000000000000 is held at 0x80795BA0 (`fn_80047398`) and again at 0x80795BC8 (`fn_8004CAD8`), plus the repeats of 0x00000000 / 0x3F800000 between the two units' pools | 15,460 | **held** as a fold (one value at two pool addresses is two TUs); phase 3 recuts the cluster instead (cuts 0x80048964 and 0x8005270C, the registered edges 0x8004C9A0 / 0x8004CAD8 kept: `phase3-notes.md` E2) |
| F8 `quest_snd` + `fn_800EF7D8` (p2b) | 0x800EE014..0x800F2A94 | 358 | **2**: 0x4330000000000000 at 0x80796580 and 0x80796598 | 16,812 | **held** as a fold; phase 3 recuts at 0x800EEAE0 (E3), 375 `.data` symbols attached |
| F9 `eft001` + `eft002` (p2b) | 0x800FAE08..0x800FD520 | 358 | **2**: 0x4330000080000000 at 0x80796638 and 0x80796650 | 16,467 | **held** as a fold; phase 3 recuts at 0x800FBE64 (E3), TU-Y absorbs eft002 |

All applied folds together (`phase2-folds.json`: four units, one move): 355 units, FAIL pool 63 / data-order 0 / jumptable 1 / the rest 0, 0 new findings,
15,722 symbols attached. Held with the exact failing check (also in the file's `open_questions`): F7, F8, F9; `Network/network_opening` (p2e: `lbl_80602968`,
slots in `network_layer_io`, sits after `NetworkWiiMediator`'s `.data`; attaching it FAILs `vtable`, so the engine blocks it - either the registered edge 0x80413C64
or that range is wrong); `Pl/fn_802840DC` (p2c: `jumptable_805C80A8` before 30 symbols read by `pl_act`/`pl_act_step`: an edge question, not a fold).

One result needs a second look in phase 3. Without F5 the engine attached the 989 `.data` symbols 0x80582FB0..0x8058AF41 to `draw_shape` (medium: 4 read by
`draw_shape`, 965 placed by pointer-graph evidence and 20 by contiguity) where p2a deferred the block: a unit with 0x22C0 B of text would own 32 KB of data on
derived evidence. F5 is the alternative that passes every check; neither is proven.

## 5. Still undecided, by cause (`unowned_data` of `phase2-reconcile.json`)

| class | rows | symbols | bytes | units (symbols) | what decides it |
| --- | ---: | ---: | ---: | --- | --- |
| `multi-tu` | 28 | 891 | 48,509 | `NetworkCommunityPat` 361, `fn_80047398` 199, `homebutton/fn_80533474` 198, `NetworkSessionManagerPat` 43, `fn_8055B710` 19, `lb_server_sel_trans` 16, `ef_drawstripestrategy` 14, `network_opening` 14 | a text cut the seam/jump table/vtable puts inside the unit (phase 3, bands b/e/f); a seam-cut block keeps the pieces before the seam, the blocked piece takes the later pieces along |
| `interleave` | 7 | 815 | 46,632 | `quest_snd` + `fn_800EF7D8` 375, `pl_act` + `fn_802840DC` 167, g3d 131 + 76 + 35, `eft001` + `eft002` 31 | merge (F7-F9 class: pool refuses) or an extern-data reading; phase 3 |
| `unread` | 60 | 207 | 116,094 | `lobby/fn_80220038` + `Pl/fn_80229ECC` 72, `nw_resource` + `g3d_xsi` + `fn_800D77B0` 13, `draw_shape` + `draw_shape_arm` 10, `.bss` runs nothing reads (0x80673C0A..0x80682E80 in a g3d chain; 0x806A2D34..0x806A4538; 0x80708076..0x8070CDE0 in the ARC/AX chain) | nothing decodes a read: only a link-order neighbour or a pointer table in unowned data |
| `reader-vs-pointer` | 14 | 25 | 6,222 | the 12-element `.bss` array 0x80790C08..0x80790D10 (`homebutton/fn_80556FC4` + `fn_8055FB70`) and its neighbour 0x807907D0, `fn_8055FD58` + `fn_80533474` 8, `fn_8055F728` + `fn_8055EAF0` / `fn_8053E808` | a reader and the pointer tables around it name different units (section 7, rule 5) |
| `ambiguous` | 27 | 50 | 21,057 | `fn_80063888` + `fn_800680CC` 6, `fn_80040598` + `mh3_pad` 6, `eft007_fx` + `eft009` 5, `lb_npc` + `lb_menu_scratch` + `lb_menu_pos_tbl` 4 | one symbol between two units' anchors that nothing reads |
| `linker-generated` | 3 | 3 | 196 | `.init` `_rom_copy_info`, `_bss_init_info`; `extabindex` `_eti_init_info` | nobody: mwldeppc emits them |

The 24 `pool-order` rows of the previous regeneration (151 symbols) are all decided now: the numeric `.sdata2` ones attach to their reader (the inversion is an open
question of the unit), the strings by the initialiser order, the rest narrowed to the units that read them (section 7, rule 4).

By section: `.data` 1,844 (multi-tu 891, interleave 805, unread 127, reader-vs-pointer 11, ambiguous 10), `.sdata` 55, `.sdata2` 8, `.rodata` 12, `.sbss` 35, `.bss` 34, `.init` 2, `extabindex` 1.

## 6. For the tools

* `dataattach.py` decides a symbol from the unit's own `__sinit`, its readers and link order: a global defined in a unit nothing in the unit reads is attached to a reader, unless a
  sinit says otherwise (hold-out 3,040 decided, 3,013 right, 27 wrong, 64 undecided; the 27: `em_pop`|`em_model` 17, `DWCi_NatNeg` 4 and six symbols held by `em010_prog`'s range
  that `em008`'s and `em011`'s sinits construct - the registered range is the error).
* A symbol decided on derived (pointer) evidence only is `medium`, and a block with fewer than `max(3, N/50)` symbols read by its unit is medium (section 7, rule 6).
* The fold test needs the checker's findings, not its counts: F7 passes the count (63 -> 62) and fails on six new findings.
* The regeneration commit of the six lane files (`3579c953f`) says 218 homebutton symbols where the count is 181 (+ 41 of the fold unit); this file has the numbers.

## 7. Phase 3 engine and checker fixes (2026-10-01)

Six reviewers re-derived the attachments; the fixes below are in `tools/splits/dataattach.py` and `tools/splits/splitcheck.py` (selftests: `splitcheck` 137 checks,
`dataattach` 63 checks, each rule fails a named check when its code is reverted), and this file is the engine's regeneration, not an edit. Row census against the previous
`phase2-reconcile.json` (`attach`): 809 rows unchanged, 31 with another range or owner, 9 regraded medium -> strong and 5 strong -> medium, 3 removed, 2 split, 23 new;
(`unowned_data`): 96 unchanged, 38 decided, 13 changed, 6 split, 5 new. Symbols whose fate changed: 1,461 (regraded medium -> strong 465 and strong -> medium 133, multi-tu -> attached 436,
unread -> attached 156, pool-order -> attached 151, ambiguous -> attached 43, attached -> unread 40, -> reader-vs-pointer 25, folded in 12).

| rule | what the engine does now | symbols whose fate changes without it (ablation: the rule reverted, the file regenerated) |
| --- | --- | ---: |
| 1 `__sinit` evidence | a global a unit's own `__sinit` constructs is that unit's (a store, a ctor's `this`, `__register_global_object`, a tail-called ctor); `.bss` 0x80696D90, 0x806A4548, `emc_work` + 0x806A54E0, `cockpit_work` + `cockpit_state`, 0x80790D64, `.sbss` 0x80795A18 / 0x80795A30, `lb_menu_scratch`, `lobby_w`, `lbl_8066A920` decide; `Pl/bss_pool` is a fold into `Pl/pl_coll` (strong, sinit); groups an owner's range holds although another unit's sinit constructs them become questions (`em010` / `em008` / `em011`) | 102 (hold-out unchanged) |
| 2 false pointer edges | unaligned `.4byte` sites and holders of at least 64 non-zero words under 1 % of them pointers (the SJIS table `lbl_80574E10`: 12 in 4,278 B) name nothing | 362 (strong -> medium: false neighbours stop a pointer-graph row being strong) |
| 3 decoder | `lis` live in r14..r31 until written, `mr` copies, any GPR write kills, the `_f_*` operands of a module loader are no readers | 130 (87 strong -> medium, 40 unread -> attached, 3 other) |
| 4 pool order | initialiser strings first, numeric literals TU-local (inversion = open question, `pool` FAIL accepted), blocks nobody else reads kept whole, candidates narrowed to readers | 160 (151 back to `pool-order` deferrals, 9 medium -> strong) |
| 5 reader-vs-pointer | a disagreement at a block edge and an array (>= 4 contiguous equal-size elements one table names) split between units are deferred with both listed; an enclosed one stays, medium, noted | 163 (25 deferred -> attached, 138 medium -> strong: the enclosed disagreements); hold-out without it: 3,045 decided / 3,018 right / 27 wrong / 59 undecided |
| 6 anchors | strong needs `max(3, N/50)` read symbols; V->S seams cut a row; row signals count `pointer-graph` as `explain` does | 546 (388 blocked whole instead of cut at the seam, 157 medium -> strong without the threshold, 1 other) |
| 7 closure | pointer closure to a fixpoint (<= 16 rounds); pure code-pointer tables and `.data`-owned pointer-graph rows are strong | 579 (464 strong -> medium, 115 undecided without the deeper closure) |
| 8 labels | `file-string` only for a real file name, else `string-reader`; medium rows say when a foreign unit reads their first symbol | 0 (labels and notes: `string-reader` 1,494 symbols, `file-string` 213 instead of 1,431) |
| 9 `--explain` | prints a data-only unit; the analysis is cached (`build/tmp/dataattach/`): 14.2 s -> 0.64 s per call after the first | - |
| 10 `order` | file-position check per section (longest non-decreasing subsequence): finds `Pl/pl_frame_data`, `Pl/pl_act_data`; the engine moves them | 0 (the two `moves`; without the check the candidate's `order` is 0 and the units stay out of order) |
| 11 `jumptable` | a reader is the `lwzx`/`mtctr`/`bctr` dispatch: `jumptable_8059FE90` has one reader, the candidate's `jumptable` FAIL goes 1 -> 0 | 0 |
| 13 `--overrides` | `attach` / `defer` / `exclude` rows, blocked when they make an invariant fail | - |

## 8. Phase 3, lane B (2026-10-01)

The numbers above are the regeneration of section 7; `phase2-overrides.json` now carries 31 orchestrator rows, `phase1-a/b/e/f.json` eight text edits (cluster recut, eft001/eft002, quest_snd, `fn_801B4348` fold, DWC, soi, menu_plsearch) and `phase2-folds.json` the data-only unit
`Pl/pl_act_step_data`: 354 units, FAIL pool 81 -> 75, every other invariant 0, symbols in no unit `.data` 1,843 -> 1,020, `.bss` 34 -> 23, `.rodata` 12 -> 11, `.sbss` 35 -> 34, `.sdata` 55 -> 65
(19 of them the plsearch recut). Per item, the held ones with their failing check and the phase 4 list: `phase3-notes.md`.
