# Window e: data the land gate's data-closure row still refuses (19 addresses, 28 pairs)

Window e's units reference these words; no claim in the window-e tree covers them. Every one lies in the zone of **another window**: the reconciled candidate
(`phase2-reconcile.json`) either attaches it to a unit of window a/b/c/d/fg or leaves it deferred between units of those windows. A window e lane cannot
claim it without touching those windows' units or creating a data-only unit that belongs to their component (`README.md`, "Components that span windows").
They clear when the owning window lands (the gate then finds the claim), or by an owner ruling that lets window e carry them.

Window of an owner = the window holding its `.text` start (a < 0x800E0000, b < 0x801C0000, c < 0x802A0000, d < 0x80380000, fg >= 0x80460000).

| address | symbol (section, size) | window e units that read it | candidate owner (evidence of the engine) | window |
| --- | --- | --- | --- | --- |
| 0x80580058 | lbl_80580058 (.data, 0x394) | menu_sysmsg | mh3_pad (attach .data 0x8057C880..0x80581088, medium); also read by g3d_anmchr | a |
| 0x8058AFE8 | lbl_8058AFE8 (.data, 0xE3) | em_pop, menu_plsearch (and quest_entry, lobby/fn_801F9CD4) | unowned `unread` run 0x8058AF44..0x8058B178: draw_shape / draw_shape_arm / fn_80056F24 | a |
| 0x805BA540 | lbl_805BA540 (.data, 0x110) | menu_placeinfo (and lobby/fn_80219260) | unowned run 0x805BA540..0x805BA650: lb_menu_scratch / lb_menu_pos_tbl | c |
| 0x805EE078 | lbl_805EE078 (.data, 0x20) | movie (and menu/fn_8031EA8C, unsplit readers) | menu/fn_8036A828 (attach, strong) | d |
| 0x80657A20 | lbl_80657A20 (.data, 0x188) | menu_sysmsg (and homebutton/keyboard_ui) | homebutton/fn_8056083C (multi-tu run 0x80657698..0x80657BA8) | fg |
| 0x806AC8C8 | lbl_806AC8C8 (.bss, 0x660) | get_pop_dat_ptr, menu_plsearch, movie (10 units) | menu/menu_item (attach .bss 0x806AC8A8..0x806AD698, strong) | c |
| 0x806B87C0 | stage_w (.bss, 0x2FE0) | movie (and arenatask, quest_entry, stage/*) | stage/stg_w (attach .bss 0x806B87C0..0x806BB7E0, strong; `get_stg_w`, a store in fn_802AD9CC) | d |
| 0x806BC1D0 | lbl_806BC1D0 (.bss, 0xC8) | em_pop, menu_plsearch (and 4 more) | camera/camera_main (attach .bss 0x806BB7E0..0x806BC300, medium) | d |
| 0x806BF310 | lbl_806BF310 (.bss, 0x58) | get_pop_dat_ptr (and 7 more) | ef/eft052 (attach .bss 0x806BF310..0x806BF3C0, strong) | d |
| 0x806BF368 | lbl_806BF368 (.bss, 0x58) | get_pop_dat_ptr (and eft052, lobby/fn_801E0ADC) | ef/eft052 (same attach) | d |
| 0x806BF530 | lbl_806BF530 (.bss, 0x2EB8) | net_session_close, get_pop_dat_ptr, menu_placeinfo, menu_plsearch, movie (12 units, 6 unsplit readers) | unowned `ambiguous` run 0x806BF530..0x806C23E8: menu/fn_8036A828 / enemy/em020_prog (both d); em020_ai stores to it (fn_80375B9C) | d |
| 0x80790D64 | lbl_80790D64 (.bss, 0x94) | menu_sysmsg (and homebutton/keyboard*, tiHKBManager, fn_805482CC) | homebutton/tiHKBManager (attach .bss 0x80790D58..0x80790DF8, strong) | fg |
| 0x80793F80 | lbl_80793F80 (.sdata, 0x8) | MSL_C/alloc (`__sys_free`) | NAND/nand (attach .sdata 0x80793F08..0x80794000, strong); unsplit readers fn_80477090.., glx_SetLoadCallback writes | fg |
| 0x80794768 | Rmode (.sbss, 0x8) | movie (and draw_shape, fn_80056F24, g3d_anmchr) | main.cpp (attach .sbss 0x80794760..0x80794770, strong; fn_8003F730 writes) | a |
| 0x80794AB0 | lbl_80794AB0 (.sbss, 0x8) | menu_plsearch, movie (and em020_ai) | lobby/lb_npc (attach .sbss 0x80794AB0..0x80794B08, medium; lb_npc writes) | c |
| 0x80794AF0 | lb_item_get_data (.sbss, 0x4) | get_pop_dat_ptr (and 6 lobby units) | lobby/lb_npc (same attach) | c |
| 0x80794B10 | jump_err_msg_str (.sbss, 0x8) | menu_placeinfo (and em020_ai) | unowned `ambiguous` run 0x80794B08..0x80794B20: lb_npc / lb_menu_scratch / lb_menu_pos_tbl | c |
| 0x80794B28 | lbl_80794B28 (.sbss, 0x4) | movie (and Pl/*, lobby_scene) | Pl/player_control (attach .sbss 0x80794B28..0x80794B30, strong; written in fn_80267548) | c |
| 0x80795A20 | lbl_80795A20 (.sbss, 0x8) | menu_sysmsg (and tiHKBManager, unsplit readers) | unowned `ambiguous` run 0x80795A20..0x80795A28: homebutton/fn_8052A040 / fn_8052B004 | fg |

Window-label note: "a" for 0x8058AFE8 and 0x806BC1D0 is by the owner's text start (draw_shape 0x80047398.., camera_main), not by the data address.

Where a data-only unit would be needed (no owner in the candidate): 0x8058AFE8, 0x805BA540, 0x80657A20, 0x806BF530, 0x80794B10, 0x80795A20. Each would be a stub
`NonMatching` unit `after:` the unit that owns the previous claim of that section; it belongs to the component of that unit's window, so it cannot be a window e unit.
