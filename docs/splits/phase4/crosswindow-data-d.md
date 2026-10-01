# Window d: data the land gate's data-closure row still refuses (5 addresses, 6 pairs)

Window d's units reference these words; no claim in the window-d tree covers them. Each is **shared** by units of two windows (two or more units read it)
and its candidate owner lies in **another window**: the reconciled candidate (`phase2-reconcile.json`) attaches it to a unit of window b or c, or leaves
it deferred between units of other windows. A window d lane cannot claim it without touching those units. They clear when the owning window lands, or by
the owner ruling that lets the orchestrator land with a recorded `--allow-orphan` for exactly these addresses (`datagap.py --row ... --allow-orphan 0x806A20F0
--allow-orphan 0x806A54E0 --allow-orphan 0x805B7B60 --allow-orphan 0x805B7B80 --allow-orphan 0x805B7BA0` prints `row: PASS`, every allowance matched, on the
tree after `git merge main` with window a landed).

Window of an owner = the window holding its `.text` start (a < 0x800E0000, b < 0x801C0000, c < 0x802A0000, d < 0x80380000, e < 0x80460000).

| address | symbol (section, size) | window d units that read it | candidate owner (evidence of the engine) | window |
| --- | --- | --- | --- | --- |
| 0x806A20F0 | eft_control (.bss, 0xC44) | ef/eft_slot, menu/fn_8031A6C0 (also ef/eft_res, effect, eft004, eft019) | ambiguous interval ef/effect.cpp..ef/eft019.cpp (readers) | a |
| 0x806A54E0 | lbl_806A54E0 (.bss, 0x2200) | ef/eft_slot (also enemy_control, em_common, fn_801550FC, fn_80171194, lb_quest_screen) | enemy/enemy_control (readers) | b |
| 0x805B7B60 | lbl_805B7B60 (.data, 0x20) | lobby/lb_screen_step (also lobby/fn_801E0ADC) | lobby/fn_801E0ADC (reader) | c |
| 0x805B7B80 | lbl_805B7B80 (.data, 0x20) | lobby/lb_screen_step (also lobby/fn_801E0ADC) | lobby/fn_801E0ADC (reader) | c |
| 0x805B7BA0 | lbl_805B7BA0 (.data, 0x10) | lobby/lb_screen_step (also lobby/fn_801E0ADC) | lobby/fn_801E0ADC (reader) | c |

`lbl_8058AA98` (.data 0x10, read by ai/ai_npc and lobby/fn_801F9CD4, candidate owner main/draw_shape) was on this list before window a landed; the landed
window a claims it, so its allowance is now stale and is not part of the landing command.

Decided inside window d (not in the list): `lbl_806BF530` (.bss 0x2EB8, read by menu_item_sub, em_prog_support and em020_prog, chain menu_item_sub..em020_prog)
is attached to `enemy/em020_prog` (override row 50); `lbl_8079ADE0` (1.0f) goes to `ef/fn_8030681C` (override row 49).

Deferred, not refused by the row (reported): `lbl_8058AA70` (ai_npc, ambiguous owner beside ef/ef_sphere), `lbl_805E7B50` / `lbl_805E7B60` (ef/eft_slot and
ef/eft050, span-blocked), `lbl_80791B60` (lobby/lb_screen_step), `lbl_80791BC8` (lobby/fn_802FA9A0), `lb_tr_flag_data` (lobby/fn_8030121C).
