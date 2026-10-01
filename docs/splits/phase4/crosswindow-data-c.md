# Window c: data the land gate's data-closure row still refuses (2 addresses, 2 pairs, against main 3f7532ded)

`python tools/units/datagap.py --row <window c units> --base-snapshot <MAIN land-base.json>` refuses these 2 (unit, address) pairs; the other 19 addresses of the earlier list are claimed by the landed windows a/d/e/fg: no claim in the window-c tree covers the address.
None is **sole-referenced** (`tools/units/callers.py <addr> --json`: every address has readers in two or more units), so none is a claim of one window-c unit; each is one of

* **shared with another window** - the owner is a unit of window a/b/d/e/fg (or unregistered code) and claiming it from c would take bytes from that window's component;
* **ambiguous inside the window** - the run lies between two of window c's own recut units and the evidence does not pick one (`dataattach` decided it `ambiguous`); a guess would
  move target bytes between NonMatching units on no evidence.

Window of an owner = the window holding its `.text` start (a < 0x800E0000, b < 0x801C0000, c < 0x802A0000, d < 0x80380000, e/fg above).  Readers marked **c** are window c units.

| address | symbol (section, size) | readers | unit that would own it | owner window | why not claimed here |
| --- | --- | --- | --- | --- | --- |
| 0x80794868 | lbl_80794868 (.sbss, 8) | lobby/lb_menu_pos_tbl **c**, main.cpp, sound/fn_800E46E8, Network/network_pat_control | none registered (readers are four units of three windows) | - | no single owner |
| 0x80794B18 | lbl_80794B18 (.sbss, 8) | lobby/lb_npc **c**, lobby/lb_menu_scratch **c**, lobby/lb_menu_pos_tbl **c**, menu/multi_result | lobby/lb_npc, lobby/lb_menu_scratch or lobby/lb_menu_pos_tbl (run 0x80794B08..0x80794B20) | c / e | ambiguous among three c units; multi_result (e, landed) reads it 23 times |

`--allow-orphan` list (the 2 addresses): see `docs/splits/phase4/land-c.txt`.
