---
id: 241
title: A case that jumps forward into the middle of the next case: put the next case's label inside an if (0) block
status: works
problem: Retail's switch case ends with a `b` into the middle of the following case (past that case's own test), so the shared tail exists once; the goto-free source repeats the tail in each case and the function grows by its size.
tags: [source-shape]
applies: [Wii/1.3]
demo: 
related: [183, 213, 219]
---

# 241. A case that jumps forward into the middle of the next case: put the next case's label inside an if (0) block

**Problem.** Retail lays cases 0, 1, 2 out in order, and case 0 finishes with a `b` that lands *after* case 1's
prefix (its "result arrived?" test), case 1 with a `b` past case 2's prefix: a chain of forward jumps into the
middle of the next case. The goto-free spelling repeats the shared tail in every case (MWCC has no tail-merge,
idea 213) and the function is the tails' size longer, with every later instruction shifted.

**Why it happens.** A `case` label may sit anywhere inside the switch body, including inside a nested block.
`if (0) { case 1: <prefix> }` is a block the front end must keep (its label is reachable), and the constant-false
test becomes the one unconditional `b` over it - exactly the jump retail has from the end of case 0 to the code
after case 1's prefix. A `break` inside the block still leaves the switch.

**How to work it.** Write the cases as one fall-through run: case 0's body, then `if (0) { case 1: <case 1's
prefix> }`, then the shared code, then `if (0) { case 2: <prefix> }`, then the next shared code and its `break`.
Keep a one-line comment above each `if (0)` saying which path skips the prefix. It only covers *forward* jumps into
a *later* case; a backward jump into an earlier case is still idea 213's wall.

**Result.** `menu/menu_plsearch` (Wii/1.3, -O4 by pragma, nopeephole): `plsearch_page19_step` 92.86 -> 99.64 %
(2260 -> 2140 B, the target's size; step 40's friend-sync / profile-resend / command-18 chain). The remainder is
register colouring.

More cases (Wii/1.3, each unit's real flags):

| unit / function | jump | before -> after |
| --- | --- | --- |
| `enemy/em018_prog` `em018_motion_events` | motion 0x55 into motion 0x56's water tail | 98.70 -> 100 |
| `Pl/pl_act_step` `pl_gather_item_take` | results 0-2 over result 3's SE into the bonus tail | 73.70 -> 100 |
| `Pl/pl_act_step` `pl_item_use_act_pick` | items 380/384 (`want = 1`) into item 583's search | 94.08 -> 100 |
| `Pl/pl_act_step` `pl_act_fishing` | grade 1 into the 2/3 grade's tail (the after-switch code moved into the switch) | 76.83 -> 98.99 |
| `Pl/pl_act_step` `pl_scan_act_select` | rows 3/24 over rows 37..15 (all wrapped in one `if (0)`) into row 16; key 248 into 247's tail | 98.53 -> 99.35 |
| `Pl/pl_act_step` `pl_act_net_hook_main` | items 0x168/0xD into 0x2C's last call; 0x1B/0xA7/0xA6 into 0x1A (nested `if (0)`s) | 84.42 -> 87.93 |
| `menu/menu_friendlist` `friend_pick_step` | the request chain above, verbatim | 92.28 -> 99.66 |
| `enemy/em020_prog` `lobby_net_update` | case 0 into case 1's roster fetch | 93.20 -> 96.38 |
| `lobby/lb_pane_ui` `lobby_enter_seq_step` / `lobby_scene_setup_step` | case 0 into case 1's create test and into case 2's done block | 89.54 / 89.31 -> 89.64 / 89.41 |

Limits, measured:

* **A target inside a nested switch's case** (em020's roster fetch, lb_pane_ui's create test): an outer `case` label
  inside the inner `switch` binds to the inner one, so the inner switch has to become an if-chain to host the label;
  its error arm then sits inline where retail's switch puts it after the shared code (a `beq`/`bne` pair differs).
* **Crossing forward jumps** (`DWCi/dwc_nonport` `DWCi_Np_CPUCopyFast`: the align-2 code jumps over the align-4 code
  to the 2-byte loop while the align-4 code jumps past that loop to the 4-byte one): `if (0)` blocks only nest, so
  overlapping intervals stay walls.
* **Backward jumps** stay idea 213's walls: `menu/menu_placeinfo` `lb_community_select_step` (all seven page-count
  jumps come from later cases), `lb_city_select_screen_step`, `fn_8004CAD8` `write_wpad_memory` (step 4 into step 3),
  `Network/network_pat_control` `updateMessagePool`, the after-switch tails `pl_act_net_hook_main` keeps inside the
  switch, and the refresh hold tail of `lobby_net_update` (case 2 inside an `if (0)` measured -28 B there).
* **A jump to after the switch from a loop** (`lobby/lb_npc` `lb_arm_wrestle_step`, idea 217) and **a jump out of a
  plain loop** (`ef/eft_res`, `enemy/enemy_control`, `camera/camera_main`) have no `switch` to put a label in.

**Example.**

```
case 0:
    sub = 1;
    if (need_sync) { sync(); break; }
    /* no sync needed: on to the resend */
    if (0) {
case 1:
        if (result == 0) break;
    }
    sub = 2;
    if (resend_needed) { resend(); break; }
    /* nothing to resend: on to the request */
    if (0) {
case 2:
        if (result == 0) break;
    }
    request();
    break;
```
