# Phase 3, lane B: proposal-level corrections (2026-10-01)

Base `0f638f4fa` (engine and checker fixed by lane A). Candidate = phase 1 a..g + `phase1-reconcile.json` + `phase2-reconcile.json` (regenerated with
`phase2-overrides.json`, byte-reproducible) + `phase2-folds.json`. Every change was rendered before it was kept: lint 0, warnings 0, "new failures outside the proposal units" 0.

```
python tools/splits/dataattach.py --lane p2 --linker --overrides docs/splits/proposals/phase2-overrides.json --out docs/splits/proposals/phase2-reconcile.json
python tools/splits/splitcheck.py --proposal <phase1-a..g> --proposal phase1-reconcile.json --proposal phase2-reconcile.json --proposal phase2-folds.json     # one --proposal per file
python tools/splits/matchinggain.py        # section H
```

## Result

| | before (354 units) | now (354 units) |
| --- | ---: | ---: |
| FAIL: order, coverage, text-cut, extab, ctors, dtors, data-order, vtable, jumptable, bss, local-static | 0 | 0 |
| FAIL: pool (units) | 81 | **75** (23 findings gone, **1 new**: `main/draw_shape.cpp` .sdata `lbl_80790F0C`, below) |
| symbols in no unit: `.data` / `.sdata` / `.bss` / `.sbss` / `.rodata` / `.sdata2` | 1,843 / 55 / 34 / 35 / 12 / 8 | **1,020** / 65 / 23 / 34 / 11 / 8 |
| attach rows (strong / medium) | 881 (751 / 130) | 928 (774 / 154) |
| `unowned_data` rows | 139 | 123 (`unread` 42, `ambiguous` 30, `multi-tu` 26, `reader-vs-pointer` 14, `interleave` 4, `orchestrator` 2, `invariant` 2, `linker-generated` 3) |
| overrides | 0 | 31 (27 applied, 2 deferred by decision, 2 blocked by `order`) |

The one added finding is **not clean by the letter of the rule**: after F5 is held and the cluster recut (E1 + E2), `main/draw_shape.cpp` owns 989 `.data` symbols and the
`.sdata` run 0x80790EF8..0x807910F8, whose first use goes down at `lbl_80790F0C` (named by the initializer at 0x80581770, after the strings read at 0x80052E28..0x80052EC8).
It is the engine's *accepted* class (numeric/string inversion recorded as an open question, `stats.accepted`), the same class as the 18 already accepted; without the recut but with
F5 removed the same class appears at `lbl_80791000` (a worse position: the block is then unowned). The recut removes 14 findings in the cluster. Revert = drop the two units
`fn_80048964` and `draw_shape` of `phase1-a.json` and put F5 back; the orchestrator decides.

Fixes to the engine made on the way (`tools/splits/dataattach.py`, selftest 65 checks, both new checks fail when the code is reverted): an `attach` override with `takes_from` now removes the
`sinit_owner_questions` row it decides, and its outcome in `--stats` reads `applied` (it read `no unowned symbol in the range`). New tool `tools/splits/matchinggain.py` (selftest).

## Items

Status: **already** = decided by lane A's rules (re-run `--explain`), **applied** = override / unit / cut, **held** = recorded with the failing check.

| item | status | what |
| --- | --- | --- |
| A | applied | overrides 0-3: `em008_prog` .bss 0x806A7868..0x806A7898, `em011_prog` .bss 0x806A7970..0x806A79A0, `em011_prog_tbl` .data 0x805A7CE8..0x805A7D54 (11 of 11 slots in em011), `lbl_8056FCD0` .rodata 0x8056FCD0..0x8056FD10 (medium); em010 keeps 0x806A7898..0x806A7970 |
| B | already | `ef/eft019` .data 0x8059F670..0x805A00B8 strong (48: reader 14, pointer-graph 30, jumptable 4), `.sdata` 0x80791840..0x80791930 medium |
| C1 | applied | unit `Pl/pl_act_step_data` (after `lobby/fn_80220038`, `phase2-folds.json`): .data 0x805BDC48..0x805C15A8 (72 symbols) + .bss 0x806AACC0..0x806AB3E0 (10), clean (0 new FAIL; one `bss` UNKNOWN, no read of its own). The lobby row already ended at 0x805BDC48. `lbl_805BFCE8` and `lbl_805BFFE0` are also read by lobby/fn_80212810 and menu/fn_802E4978 as externs |
| C2 | applied | overrides 4-6: pl_act .data 0x805C6100..0x805C80A8 and fn_802840DC 0x805C9568..0x805CBFC0 (medium), 0x805C80A8..0x805C9568 (29 symbols) stays deferred |
| C3 | applied | fold (overrides 7, 8): `Pl/pl_frame_data` + `Pl/pl_act_data` into `Pl/pl_act_step` (all 118 literals read by it only), pool FAIL 81 -> 79. Phase 4 cost: `src/Pl/pl_frame_data.*`, `pl_act_data.*` removed. `Pl/bss_pool` into `Pl/pl_coll`: already (strong, sinit, 12 symbols) |
| D | recorded | `lobby/fn_801E0ADC` text edge 0x801E0ADC vs data edge 0x805B75E8 (interval 0x805B75E8..0x805B76D0); p2c finding 2 (fn_802840DC edge) |
| E1 | applied (F5 held) | F5 `stage/stage_res_data` removed: 951 of 965 symbols read by nobody, 14 `.sdata` objects of draw_shape's run are pointer-linked with it, 321 are tables read by 7 units. Consequence in the E2 line |
| E2 | applied | `phase1-a.json`: cut 0x80048964 (pooldup interval [0x80048764, 0x80048964], 10 starts) and 0x8005270C (interval [0x800525E4, 0x8005270C], 6 starts), both medium, at the first reader of the later pool group; c = `main/draw_shape` [0x8005270C, 0x80055EC4) absorbs the registered draw_shape, replaces fn_8004CAD8's tail, removes the edge 0x80054C64; its data is exactly the brief's (.data 0x80581770.., .sdata 0x80790EF8..0x807910F8, .bss 0x8066A920.., .sbss 0x807948A0..). **Not as briefed:** the pools force a THIRD cut in (0x80048E2C, 0x80050BC0] (0.0 at 0x80795BA8/BDC, 1.0 at BAC/BE0, 0x4330000080000000 at BB8/C10): the registered edges 0x8004C9A0 and 0x8004CAD8 both lie in it, so they stay and fn_8004C9A0 remains a 0x138-byte unit (a TU b = tail + fn_8004C9A0 + head of fn_8004CAD8 would repeat 0.0 and 1.0). 14 pool findings gone |
| E3 | applied | eft001/eft002: cut 0x800FBE64 (interval [0x800FBD68, 0x800FBE64], 2 starts), TU-Y `ef/fn_800FBE64` absorbs eft002; head .data 0x8059B5F8..0x8059B638 (3), TU-Y 28 symbols, 4 findings gone. quest_snd: cut 0x800EEAE0 (interval [0x800EE868, 0x800EEAE0], 8 starts), TU-2 `sound/fn_800EEAE0` absorbs fn_800EF7D8: 375 `.data` + 67 `.sdata` symbols |
| E4 | applied | `fn_801B4348` text fold [0x801B4348, 0x801B7020) (`phase1-b.json`; the edge 0x801B4458 removed); overrides 9, 10: `fn_80382310` residue into `fn_80383148` (.data strong) + .bss 0x806C5488..0x806C5528 (medium) |
| E5 | applied | DWC: cut 0x805073C0 medium on the three-section seam (`.data` llhhh, `.sbss` 12 + 7, `.sdata` llh; the last low read 0x8050719C, the first high read 0x805073C8), the three roster guesses stay merged as `DWCi/dwc_error` [0x805073C0, 0x80507C40) and the registered edge 0x80507C40 stays (merging forward into DWCi_Np_CPUCopyFast has no signal at 0x80507C40 either way, not rendered). soi: cut 0x8052A040 medium (vtable 0x8064A638 has 20 of 28 slots in 0x8052A040..0x8052B004; the soi pooldup interval holds it): soi's own pool FAIL gone (4 findings); the vtables 0x8064A5F8/0x8064A638 stay deferred (override 11) |
| E6 | applied | `menu_plsearch`: cut 0x804513FC medium (V->S seam 0x80607E50, last group-1 read 0x80451364, first group-2 read 0x80451600; 5 starts), override 12 gives the 381 unread symbols between the seam and the first read symbol to TU2. Cost: 19 `.sdata` symbols 0x80793C10..0x80793C7F are now `unread` between the halves. Anchors: quest_entry .data 132 symbols with 6 read by the unit (4.5 %), menu_placeinfo .data 28 with 4 read (14 %): strong by `max(3, N/50)`, medium by a share rule |
| E7 | recorded | network_opening Z/W cuts (6 and 2 positions), lb_server_sel_trans seam 0x805F91F0, SessionManagerPat seams 0x805FB2B8/0x805FB718 with the intervals (`phase2-folds.json` open_questions) |
| E8 | relabelled | F6 `Network/network_shared_data`: **dissolve at registration (phase 4)**, read by lb_server_sel_trans, NetworkPeerMcs, NetworkSessionBase, NetworkSessionStable, NetworkSessionManager and NetworkSessionManagerPat; `.sdata` 0x80793900..0x80793930 and `.sdata2` 0x8079C690..0x8079C758 split by reader (monotone in text order); the pool FAILs are the `extern const` scalar question |
| F | mixed | **applied** (override 13): em_pop | em_model .data edge at 0x805F8220 (17 labels read only by em_model, strong); **recorded**: DWCi_NatNeg .bss four symbols (a `range` register item), ef_sphere / fn_800CDB2C names, AXFXReverbHiExpShutdown, F4 verified (`lobby/fn_801EC9F8_merged` renders, 55 symbols), the 0x80054C64 edge (applied with E2), g3d_resanmtexsrt, ef_effect (V->S 0x805925B8), g3d_scnobj (zigzag 0x8058F4C8, fn_80075DCC cut in [0x8007B340, 0x8007B878], 28 starts: not forced) |
| G | mixed | **applied** (overrides 14-22): nine vtables to the first candidate (medium, slot owner, ranges start at the unit's own .data end, `takes_from` itself to include the alignment bytes); fg: 0x8064E6C0, 0x80650080/0x806500A0, 0x806504C8 applied (overrides 24, 26, 27), **held**: 0x8064DDE0 -> fn_80533474 and 0x8064F338 -> fn_80542D8C (blocked by `order`: each unit's own .data ends before a deferred run); `.sdata` 0x80793D20 -> AX/fn_80475E30 (override 29, read 5 + 5 by both AX units: one TU spans the cut), `.rodata` sin/cos table -> nw4r/math_triangular (override 30, medium); **already**: `.sbss2` 0x8079D7E0 (gki_buffer, medium), eft_slot .data / .sdata (interleave row 0x805E7B50..0x805E7B70 kept), `.sdata2` 0x8079A448 / 0x8079A8E0, `.rodata` 0x80573188 -> MSL_C/alloc; 0x80790C08 array deferred; **applied** em035_prog_tbl (override 28). One more vtable of the same class, 0x80594840 (slot owner = the second candidate), recorded |
| H | tool | `matchinggain.py`: 37 Matching units, 11 gain data (list below), 7 absent under their name |
| I | done | F1 and F3 `reproduce`/evidence commands replaced by `callers.py` commands that print the cited lines; every command of `phase2-folds.json` (28) and `phase2-overrides.json` (32) runs and prints (0 failing, none empty; a row whose range is owned reports `is in no unowned run`, a pre-override state) |

## H. Matching units the candidate gives data to (`matchinggain.py`) - phase 4: define the data in the source and re-measure (playbook 23/29; `flipcheck` refuses), or demote

| unit | section (registered -> candidate) |
| --- | --- |
| `menu/menu_note.cpp` | .data 0x28 -> 0x50 (+0x28: `__FILE__` + assert strings 0x805E91F8..0x805E9220) |
| `hud/fn_80324F7C.c` | .data +0x18, .sdata +0x40 |
| `ef/fn_803066F0.c` | .sdata +8, .sdata2 +4 |
| `Runtime.PPCEABI.H/global_destructor_chain.c` | .sbss +8 |
| `ef/fn_800FD520.c` | .sdata +8, .sdata2 +0x30 |
| `ef/fn_800FD718.c` | .sdata2 +8 |
| `ef/fn_80101DF4.cpp` | .data +0x11B8 |
| `ef/fn_80104BD0.c` | .data +0x258, .sdata2 +0xC |
| `ef/fn_8011722C.c` | .data +0x28, .sdata2 +8 |
| `g3d/g3d_gpu.cpp` | .data +0x48, .sdata2 +4 |
| `gx/fn_8009ACE4.c` | .sdata2 +0x10 |

Seven more Matching units no longer exist under their name (swallowed by a fold or a roster guess; they match standalone but the evidence says they are not TUs): `ai/fn_802D0DCC.c` (in `ai/ai_npc`),
`enemy/fn_80149D6C.c` (`em001_prog`), `enemy/fn_80177608.cpp` (`em015_prog`), `enemy/em020_handlers.cpp` (`em020_prog`), `Pl/pl_master.cpp` (`Pl/pl_act`),
`Network/initNetworkSessionStable.cpp` (`NetworkSessionManagerPat`), `OS/OSAlarm.c` (`NAND/nand`: a roster **guess** merged forward swallowed a registered Matching unit; the 0x10-byte unit lies in the OSAlarm roster file, fold or keep it).

## Open questions that remain for phase 4 (`phase2-folds.json` `open_questions`, 16, plus the units' own and `phase2-reconcile.json`'s 32)

network_opening Z/W; lb_server_sel_trans and SessionManagerPat seams; pl_act | fn_802840DC (29 symbols); lobby/fn_801E0ADC edge conflict; the cluster's third cut and the accepted pool inversion; ef_effect two TUs;
g3d_resanmtexsrt data edge; g3d_scnobj / fn_80075DCC cut [0x8007B340, 0x8007B878]; the HBM vtables 0x8064A5F8/0x8064A638 and the two blocked vtables; DWCi_NatNeg .bss; the placeholder names
(`fn_800CDB2C` after the wrong address, `AXFXReverbHiExpShutdown`); menu_plsearch's remaining TUs; `ef_drawsmoothstripestrategy` 0x80594840; the 19 `.sdata` symbols between the plsearch halves.

## Phase 4 must do before `splits.txt` is touched

* **Registered-edge corrections**: remove 0x80054C64 (draw_shape interior), 0x800FCED4 (eft002), 0x800EF7D8 (fn_800EF7D8), 0x801B4458, 0x80413C64 and 0x804155D4 (not TU edges; the recut is open), 0x805F84C0 -> 0x805F8220 (.data), em010 | em011 `.data` 0x805A7D54 -> 0x805A7CE8, `.rodata` 0x8056FD10 side -> 0x8056FCD0, `.bss` 0x806A7868 / 0x806A7970 (em008 | em010 | em011), pl_act | fn_802840DC `.data`, DWCi_NatNeg `.bss` (4 symbols, a `range` register item).
* **Unit moves / removals**: `Network/network_shared_data` dissolves (data by reader); `Pl/pl_frame_data`, `Pl/pl_act_data`, `Pl/bss_pool`, `hud/cockpit_icon_data`, `enemy/fn_80382310` fold away (src files removed); `Pl/pl_act_step_data` is a new data-only unit.
* **Renames**: `ef/fn_800CDB2C.cpp` -> `fn_800CE5A8`; the placeholder names of the new units (`main/fn_80048964`, `main/draw_shape` as a root unit, `ef/fn_800FBE64`, `sound/fn_800EEAE0`, `menu/fn_804513FC`, `Pl/pl_act_step_data`, `DWCi/dwc_error`, `homebutton/fn_8052A040`); `AXFXReverbHiExpShutdown` (rule 7).
* **Matching demotions or data definitions**: the 11 units of section H and the 7 swallowed ones.

## J. The seven dtk boundaries (phase 4 tool, 2026-10-01)

Overrides 31-38 of `phase2-overrides.json` give the six unaligned symbols and the 4-byte `.sdata2` gap an owner (item 4 decided as the two arrays, item 6 as two rows); the decisions, evidence and the dtk result per window are in `docs/splits/phase4/README.md`. `phase2-reconcile.json` regenerated: 935 attach rows (928), `unowned_data` 116 (123), pool 75 (75), lint 0; overrides 39 rows (35 applied, 2 deferred, 2 blocked by `order`).
