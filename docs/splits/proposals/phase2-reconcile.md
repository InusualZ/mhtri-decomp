# Phase 2 reconciliation: one set of data attachments

Files: `phase2-reconcile.json` (the whole-DOL `attach` + `unowned_data` set, written by `tools/splits/dataattach.py`), `phase2-folds.json` (the text folds,
the data-only unit and the move phase 2 accepted). They replace the six lane files (`phase2-p2a.json` .. `phase2-p2fg.json`, `-p2a-sdata`, `-p2b-outside`,
`-p2b-merge-c`, `-p2d-folds`), which are in git history (`49be9c958`, `b9ec27b02`, `3579c953f`); the six `phase2-p2*.md` lane reports stay as the lanes'
accounts of their own runs (their commands name the lane engines, which are gone).

```
python tools/splits/dataattach.py --lane p2 --linker --out docs/splits/proposals/phase2-reconcile.json     # deterministic, ~30 s
python tools/splits/splitcheck.py --proposal docs/splits/proposals/phase1-{a..g}.json --proposal docs/splits/proposals/phase1-reconcile.json \
    --proposal docs/splits/proposals/phase2-reconcile.json --proposal docs/splits/proposals/phase2-folds.json --emit-splits OUT
python tools/splits/dataattach.py --explain <unit regex | address> [--section S]    # the `reproduce` of every row
python tools/splits/dataattach.py --holdout                                         # the solver's precision on the registered data
```

## 1. Result

Candidate = phase 1 files + `phase1-reconcile.json` + `phase2-reconcile.json` + `phase2-folds.json`: lint 0, warnings 0, 355 units (360 in phase 1: the folds F1-F4 remove 6 units, the data-only unit F5 adds 1, section 4), 0 new failures outside the proposal units.

| invariant | FAIL phase 1 candidate | FAIL + phase 2 |
| --- | ---: | ---: |
| order, coverage, text-cut, extab, ctors, dtors, vtable, bss, local-static | 0 | 0 |
| pool | 63 | 63 |
| data-order | 0 | 0 |
| jumptable | 1 | 1 |

(Phase 1 candidate under the current checker: `data-order` is 0 because of the V->S rule of `docs/splits-program.md`; it was 3 under the old one.) One FAIL
pair moved: `enemy/em_pl_frame.cpp` `pool` became `hud/pl_frame_sync.cpp` `pool` (the fold keeps the unit's open pooldup question).

Rows: 859 ranges attached (strong 732, medium 127), 17 `guess` ranges kept next to their deferral and never applied, 153 `unowned_data` rows, 3 linker-generated.

| section | ranges | symbols | bytes | strong | medium |
| --- | ---: | ---: | ---: | ---: | ---: |
| `.rodata` | 53 | 239 | 39,568 | 45 | 8 |
| `.data` | 220 | 5,501 | 558,361 | 161 | 59 |
| `.sdata` | 170 | 1,910 | 12,831 | 126 | 44 |
| `.sdata2` | 239 | 6,716 | 29,696 | 237 | 2 |
| `.bss` | 91 | 452 | 1,084,546 | 84 | 7 |
| `.sbss` | 83 | 893 | 4,232 | 76 | 7 |
| `.sbss2` | 1 | 1 | 8 | 1 | 0 |
| `extab`, `extabindex` | 1 + 1 | 5 + 5 | 100 + 60 | 2 | 0 |

Symbols in no unit, whole map (`splitcheck` coverage gaps), before phase 2 -> after: `.data` 8,454 -> 2,310, `.sdata2` 6,802 -> 86, `.sdata` 2,052 -> 213,
`.bss` 439 -> 29, `.sbss` 904 -> 37, `.rodata` 277 -> 38, `.sbss2` 3 -> 2; `.init` (2) and `extabindex` `_eti_init_info` (1) are linker tables nobody owns
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
| F5 `stage/stage_res_data` (p2a) | data-only unit for `.data` 0x80582EF8..0x8058AC50 (965 resource-name symbols read by `stage/stg_w` 826, `ef/effect` 113, ..), `after: fn_80047398.cpp` | 360 | 0 | 15,478 | **applied** (the 965 symbols move from the engine's rows to the unit) |
| F6 move `Network/network_shared_data` (p2e) | after `lobby/lb_server_sel_trans`, where its `.sdata`/`.sdata2` sit between that unit's data and `NetworkSessionManagerPat`'s | 359 | 0 | 16,436 | **applied** (`moves`; file order is link order, phase 4 moves the registered unit) |
| F7 `fn_80047398` + `fn_8004C9A0` + `fn_8004CAD8` (p2a) | one TU over 0x80047398..0x80054C64 | 357 | **6**: the double 0x4330000000000000 is held at 0x80795BA0 (`fn_80047398`) and again at 0x80795BC8 (`fn_8004CAD8`), plus the repeats of 0x00000000 / 0x3F800000 between the two units' pools | 15,460 | **held**: one value at two pool addresses is two TUs; the alternative is a registered range 0x80582C60..0x80582EF8 that belongs to a later unit |
| F8 `quest_snd` + `fn_800EF7D8` (p2b) | 0x800EE014..0x800F2A94 | 358 | **2**: 0x4330000000000000 at 0x80796580 and 0x80796598 | 16,812 | **held** (the data stays an `interleave` deferral) |
| F9 `eft001` + `eft002` (p2b) | 0x800FAE08..0x800FD520 | 358 | **2**: 0x4330000080000000 at 0x80796638 and 0x80796650 | 16,467 | **held** |

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
| `multi-tu` | 17 | 1,327 | 68,493 | `NetworkCommunityPat` 366, `fn_80047398` 199, `homebutton/fn_80533474` 199, `network_layer_io` 188, `lb_server_sel_trans` 73, `g3d/fn_80075DCC` 57, `homebutton/fn_8056083C` 53, `ef/eft019` 48, `NetworkSessionManagerPat` 43, `fn_8055B710` 27, `fn_80542D8C` 24, `ef_drawstripestrategy` 19, `network_opening` 14, `tiHKBManager` 7 | a text cut the seam/jump table/vtable puts inside the unit (phase 3, bands b/e/f) |
| `interleave` | 7 | 815 | 46,632 | `quest_snd` + `fn_800EF7D8` 375, `pl_act` + `fn_802840DC` 167, g3d 131 + 76 + 35, `eft001` + `eft002` 31 | merge (F7-F9 class: pool refuses) or an extern-data reading; phase 3 |
| `unread` | 64 | 332 | 125,374 | `ef/eft050` 87, `ef/eft_slot` 86, `homebutton/fn_8055F728` 76, `fn_8055FB70` 76, `lobby/fn_80220038` 47, `Pl/fn_80229ECC` 44, `.bss` runs nothing reads (0x80673C0A..0x80682E80, 62,070 B in a g3d chain; 0x806A2D34..0x806A4538; 0x80708076..0x8070CDE0, 19,818 B in the ARC/AX chain) | nothing decodes a read: only a link-order neighbour or a pointer table in unowned data |
| `pool-order` | 24 | 151 | 772 | `Pl/pl_act_step`, `player_control`, `pl_act` 29 each, `EXI/ProbeBarnacle`, `RVLGX/GXTexture_tail`, `OS/FindContainHeap_` 20, `enemy/em_model`, `menu/get_pop_dat_ptr`, `hud/layout`, `hud/cockpit_quest`, `ef/eft035` 17 | inline-function strings at a pool's end vs a TU boundary (needs one compiled fixture) |
| `ambiguous` | 38 | 93 | 40,625 | `ef/eft050` + `eft_slot` 31 each, g3d 7 + 6 + 5, `fn_80040598` / `mh3_pad` 6, `lobby/lb_menu_scratch` + `lb_menu_pos_tbl` 5 | one symbol between two units' anchors that nothing reads |
| `linker-generated` | 3 | 3 | 196 | `.init` `_rom_copy_info`, `_bss_init_info`; `extabindex` `_eti_init_info` | nobody: mwldeppc emits them |

By section: `.data` 2,310 (multi-tu 1,327, interleave 805, unread 140, ambiguous 39), `.sdata` 213, `.sdata2` 86 (pool-order 78), `.rodata` 38, `.sbss` 37, `.bss` 29, `.sbss2` 2.

## 6. For the tools

* `dataattach.py` decides a symbol from readers and link order only: a global defined in a unit nothing in the unit reads is attached to a reader (hold-out
  3,045 decided, 3,018 right, 27 wrong, 61 undecided; the 27 are boundaries the readers contradict: `em_pop`|`em_model` 17, `em010`|`em011`/`em008` 6, `DWCi_NatNeg` 4).
* A symbol decided on derived (pointer) evidence only is `medium`; the 989-symbol `draw_shape` block shows the weakness: a threshold of strong anchors per block is the next rule.
* The fold test needs the checker's findings, not its counts: F7 passes the count (63 -> 62) and fails on six new findings.
* The regeneration commit of the six lane files (`3579c953f`) says 218 homebutton symbols where the count is 181 (+ 41 of the fold unit); this file has the numbers.
