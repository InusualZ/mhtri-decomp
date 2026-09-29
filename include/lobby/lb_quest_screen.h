/* The declarations `src/lobby/lb_quest_screen.cpp` owns (docs/plan.md 6.5 rule 2: a consumer includes
 * the owner's header, it never declares the symbol itself).
 *
 * The unit is the quest/multiplayer screen band 0x803A3A50..0x803AA4A4 whose head reconstructs the
 * note pane the `enemy` band at 0x80385A54..0x80385CA0 drives (see the unit's file header for the
 * seam and naming evidence).  `note_pane_get_motion` is the one entry point another registered unit
 * calls today: `src/enemy/fn_80382310.cpp`'s `fn_80385A54` tail-calls it for the pane's state 1.
 *
 * The parameter is spelled `NoteWork*` - the consumer's own record, which IS the 0x1F8 bytes
 * `qn_get_motion_no` reads as `_QNPC_W` (the owner's header names both views of the same record).
 */
#ifndef MHTRI_LOBBY_LB_QUEST_SCREEN_H
#define MHTRI_LOBBY_LB_QUEST_SCREEN_H

#include "types.h"

/* The record `note_pane_get_motion` takes, forward-declared so that this header stays a leaf: it is
 * included from `include/unsplit/menu.h`, which the whole menu band takes. */
struct NoteWork;
struct QuestWork;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x803A4E58 - reads the note NPC record's motion number (the +0x54 halfword `qn_get_motion_no`
 * returns) with the retail `VEC3 v; VEC3_ctor(&v);` call-site idiom in front of it. */
void note_pane_get_motion(struct NoteWork* self);

/* The range's quest-work accessors, moved here from `include/unsplit/menu.h` when this unit
 * registered over their addresses (docs/plan.md 6.5 rule 2).  Their bodies are still unwritten, so
 * they keep the map's stems; renaming them is the batch that writes them (rule 7's unblock is the
 * name, and nine consumer sites across `enemy/fn_8012BDF4.cpp`, `enemy/fn_80176C58.cpp`,
 * `lobby/lb_companion_ui.cpp` and the two `menu` units have to be swept with it). */
s32 fn_803A87E0(void);
s32 fn_803A881C(void);
s32 fn_803A8858(void);
s32 fn_803A9690(void);
s32 fn_803AA41C(s32, f32);

/* 0x803A8D4C - the current quest id: the option block's selected id while the local slot has a quest
 * in progress, else the item work's own +0x10 key.  Read by `enemy/fn_80176C58.cpp` (its 0x3EC
 * test) and by the quest selector band. */
u32 quest_id_get(void);

/* 0x803A9DEC - writes one 0x60-byte quest-work element's value.  The caller owns the payload,
 * so the record stays a forward declaration here and the value is untyped. */
void quest_element_set(struct QuestWork* work, u8 index, void* value); /* untyped: caller-owned payload */

/* 0x803AA060 - whether the caller-owned work record may take a roll for element `index`: the
 * element's flags need bit 3 set and bit 6 clear, and the record's own word at +0x2D8 + index*4 needs
 * bit 6 set; `index` 4 is the whole-array form and counts the three elements instead.  The record is
 * the item work (`get_move_work_adrs(0) + 0xDC`), whose three views disagree (this header's
 * `QuestWork`, `lobby/lb_companion_ui.h`'s `LbCompanionWork`, `quest/quest_entry.h`'s `Q_ItemWork`),
 * so the parameter is the forward-declared struct here and each caller passes its own view. */
u32 quest_element_pick_ck(struct QuestWork* work, u8 index, s32 use_alt);


#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_QUEST_SCREEN_H */
