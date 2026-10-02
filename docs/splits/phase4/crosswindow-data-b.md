# Window b: data the land gate's data-closure row still refuses (1 address, 2 pairs)

Window b's units reference these words; no claim in the window-b tree covers them, and none can be given to a unit of window b without guessing an owner
(each is read by several units of several windows and lies in a gap between runs the candidate leaves unowned).  The row passes with
`--allow-orphan 0x806A20F0`; both clear when the window that owns the neighbouring run claims them (the last window must end with none).

| address | symbol (section, size) | window b units that read it | other readers | candidate owner |
| --- | --- | --- | --- | --- |
| 0x806A20F0 | eft_control (.bss, 0xC44) | ef/effect (3 sites), ef/eft004_fx (2 sites); also ef/eft_res (20 sites, pre-existing pair) | ef/fn_800FD864_fx, ef/eft019, ef/eft_slot, ef/eft050, menu/fn_8031A6C0 | none: it starts where ef/effect's `.bss` (0x806A1400..0x806A20F0) ends and lies in the unowned `.bss` run 0x806A20F0..0x806A4538 that ends at ef/fn_80114E34's claim; link order puts it after ef/effect's text (0x800F95A4), so ef/eft_res (text before effect) cannot own it |

Where the two sound pairs of the same row are decided: `lbl_806A1110` (.bss) and `lbl_80794A20` (.sbss) lie exactly between the claims of the units before and after
`sound/quest_snd`'s text and are claimed for it (`phase2-overrides.json` rows 58-59, medium); `quest_snd.cpp` defines both.

0x80794868 (`lbl_80794868`, `.sbss`, 8 B) is no longer refused for window b once `sound/snd_stream_reloc` is judged as a recut of the surviving `sound/fn_800E46E8` (the gate's survivor rule): its base pair follows the bytes; window c's own listing of it is unaffected.
