# The lobby units

The `src/lobby/` units: the evidence behind the lib's flags and the per-unit pragmas, and the seam measurements
too long for a unit header.  Each unit's own facts (range, names, residuals, load-bearing shapes) are in its file
header; `configure.py` carries one pointer line per row.

## Flag evidence

**The lib group `cflags_lobby`** is `cflags_base` with `-O3` for `-O4,p`, `-inline noauto` and
`-Cpp_exceptions on`.  `cflags_main` derives from it.

* **`-O3`** (`lobby/lobby_scene.c`): the retail `fn_801EC9E0` (0x18 B, 6 instructions) reads the small-data scene
  pointer, then the +0x10 table base, before the argument's byte, and keeps the table base in r4; `-O4,p` instead
  hoists the byte load, splits the base across r3 and reorders the two loads.  `-O3` is byte-identical.  The level
  is per unit: the OS unit beside this group wants `-O4,p`, so probe both.
* **`-inline noauto`** (lib-wide before/after): 5 units improved (`fn_801E7530` 27.64778 -> 31.76088, the colour
  screen band of `lb_pane_ui` 20.33288 -> 22.07398, the range 0x8021E1EC..0x80224AC4 - now `lb_menu_scratch`,
  `lb_menu_pos_tbl` and the page half of `lb_equip_page` - 9.21654 -> 9.66140, `lb_npc` 13.36667 -> 13.74540,
  `fn_80219260` 11.00314 -> 11.08171), 8 were unchanged (every unit already at its ceiling among them) and none
  regressed, checked per function; `lobby_scene.c` stayed byte-identical at 100.0.
  The same knob closed four `main.cpp` functions, and `cflags_lobby` keeps `-O3` for the same reason `main` does.
* **`-Cpp_exceptions on`** (the flags audit): every lobby target but `lobby_scene.c` (a C file, whose object the flag
  leaves byte-identical) carries extab/extabindex, and 8 units spelled it out as a file-wide `#pragma exceptions on`.
  With the lib flag on and those 8 pragmas deleted, all 20 objects measured had byte-identical allocatable sections,
  every unit score was identical and main.dol stayed BF4850739478CAAEDFE675949EB7C28595A7FDE9; the 11 targets
  without a pragma emit their extab (0 -> 56..216 B of their 480..752 B targets).  `lobby/lb_menu_page.cpp` without
  the flag emits no unwind records at all (`datagap.py --mode both`: `extab 16B (ours 0B)`, `extabindex 24B (ours 0B)`).

**`#pragma peephole off`** is the lobby units' common per-file deviation (playbook 39): retail keeps the unfused
`clrlwi`/`rlwinm` + `cmpwi` pairs and `clrlwi` + `slwi` index scales the pass folds into record forms or one
`rlwinm`.  Measured A/Bs (whole file, pass on -> off):

* `lobby/lb_menu_page.cpp`: unit 96.484505 -> 99.55954.
* `lobby/lb_companion_ui.cpp`: 24 -> 47 functions byte-identical, 60 -> 73 of 133 at or above 80 %.
* `lobby/lb_pane_ui.cpp` (the digit-entry band): `fn_801EC9F8` 78.90 -> 100.00 (256 -> 308 B); the hair/inner
  colour band: `get_change_hair_color` 84.23 -> 100.00, `get_change_inner_color` the same.
* `lobby/lb_npc.cpp`: off around `fn_801FC268` 96.9 -> 100.0, and from `fn_80203114` to the end of the NPC work band
  (0x802076D4), `fn_80203114` 99.38 -> 100.0.
* `lobby/lb_menu_scratch.cpp` / `lb_menu_pos_tbl.cpp` / `lb_equip_page.cpp` (measured over the range
  0x8021E1EC..0x80224AC4; `tools/flags/infer.py` reads "3 kept `clrlwi` before a narrowing store"): `fn_8021E1EC`
  94.78 -> 97.10, `fn_8021E484` 95.33 -> 100.00, `fn_80220B50` 92.31 -> 100.00, while `fn_8021E304` 100.00 -> 93.33 and
  `fn_802216B4` 100.00 -> 87.69 (those two keep the pass on).

**`#pragma dont_inline on` / `__declspec(noinline)`** (measured under `-inline auto`): `lobby/lb_pane_ui.cpp`
`fn_801ED688` 7.00 -> 95.86 (860 B against 1556 B inlined); `lobby/lb_npc.cpp` `fn_8021213C` 276 B against a 92 B
target with `fn_80212060` inlined, `fn_80203114` 54.81 -> 99.09, `fn_801FDEE4`/`fn_801FE13C`/`fn_801FCA80`
91.1 / 84.4 / 69.7 -> 100.0 / 99.6 / 98.9.

**The `Pl` half of `lobby/lb_equip_page.cpp`** (0x80224AC4..0x80229ECC) has `cflags_pl`'s fingerprint (`Wii/1.0`,
`-opt nopeephole`): recompiled with the `Wii/1.0` compiler the object's `.text` (4160 B), extab and extabindex are
byte-identical and its 44 functions score the same, so the lobby lib's `Wii/1.3` plus `#pragma peephole off`
moves no code.

## Seams

**`lobby/lb_companion_ui.cpp` (0x80338808..0x8033F270): undecidable.**  The range is a
`--max-bytes` cut of the unclaimed 0x8030121C..0x8035E034 stretch.

* No `__FILE__` string exists for the band: the region 0x8030121C..0x80349DD8 holds two (`menu_infomation.cpp`,
  `menu_note.cpp`, both outside), and no target object of 0x80334568..0x803432B4 cites a `.c`/`.cpp` string.
* `.sdata2` pins: 4805 single-referrer labels, 4 inversions DOL-wide, none near either edge; the pool run is an
  ordered disjoint partition, 0x8079B2B0 (below) | 0x8079B2B8/BC (here) | 0x8079B2C0+ (above).
* Nothing private crosses an edge (only `lobby_world_block`, `lobby_w` and `_savegpr_*` are cited from both sides).
* The `scope:local` anchors (654 `.data` labels, 0/653 owner-order inversions) order the three switch tables
  (0x805E27D4 <- `lb_act_dispatch`, 0x805E27F8 <- `lb_act_dispatch_ex`, 0x805E71FC <- `fn_8033C9B0`) and the
  `.data`/`.sdata` runs continue across both edges with ascending owners - the candidate-only class.
* What agrees with the extent: the unwind pair (extab from 0x8001698C + 8 = 0x80016994 to 0x80016CA4, extabindex
  0x80035CB8 + 0xC = 0x80035CC4 .. 0x8003615C) and the bracketing `hud/pl_frame_sync.cpp` / `ef/eft050.cpp`.

**`lobby/lb_quest_board.cpp` (0x80394038..0x803967F0): consistent with one TU.**  The `.sdata2` label runs are
ordered by referrer across both edges (0x8079C2A8/0x8079C2B0 -> 0x80393B28/0x80393D4C below,
0x8079C2B4..0x8079C2E8 here, 0x8079C2EC.. -> 0x803967F0 above), and so are `.data` (0x805F13B8 below,
0x805F13D0..0x805F14EC here, 0x805F1500 -> 0x80396CAC above) and `.sdata` (0x8079346C -> 0x803933E8 below,
0x80793470..0x80793480 here, 0x80793488 -> 0x8039AFBC above).  The shared words 0x8079C2C8/0x8079C2E0 (also read by
0x804BF530 / 0x8027D968) defeat the single-referrer pair test; the other owners rise monotonically.

**`lobby/lb_quest_screen.cpp` (0x803A3A50..0x803AA4A4): five bands, not one TU.**

* 0x803A3A50..0x803A52A4: the note pane (`NoteWork`, the record `enemy/em_prog_support.cpp` drives).
* 0x803A52A4..0x803A75D8: the lobby item/quest screen (`lobby_w`, `lb_param_w`, `lb_item_get_data`, `subTransSet`,
  `LbStr`, `menu_cursor_step`): a paged list with item icons and page arrows.
* 0x803A7718..0x803A86EC: resource loading (`nwAddResource`, `load_file_req`, `work_mem_*`).
* 0x803A86F0..0x803A96C4: enemy/player model and quest bookkeeping (`Screen_w`, `em_area_ck`, `get_em_chg_scale`,
  the `lbl_8079C478..C4BC` float run).
* 0x803A96C4..0x803AA4A4: enemy area/quest logic plus `lb_sub16_send`.

The DOL's six menu/lobby `.cpp` literals (`menu_item.cpp`, `cockpit.cpp`, `layout.cpp`, `cockpit_quest.cpp`,
`menu_infomation.cpp`, `menu_note.cpp`) are none of them referenced from the range, and the dump answers `zz_` for
92 of its 95 functions (the three names are duplicates: `DBClose` at 295 addresses, `JKRArchive::getResSize(void)`
at two).
