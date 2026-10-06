/* Declarations `enemy/em_prog_support.cpp` owns that other units call: `fn_803839EC` and `note_box_draw`, which
 * `hud/cockpit_quest.cpp` calls from its frame update (signatures from its call sites; neither body is written), and
 * the note-pane animation pair.
 */
#ifndef MHTRI_ENEMY_FN_80382310_H
#define MHTRI_ENEMY_FN_80382310_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

void fn_803839EC(void);                  /* 0x803839EC */
void note_box_draw(void);                  /* 0x80383AE4 */

/* 0x80385AE8 - true when the pane's animation pair is exactly `(a, b)` (`NoteWork::field_0x19D`
 * and `field_0x19F`, both compared as zero-extended bytes).  Called from outside this unit by
 * `lobby/lb_quest_screen.cpp`, whose `note_pane_pos_step` gates a 4004/1820 step on it (rule 2:
 * this unit owns the address). */
s32 note_pane_anim_pair_ck(struct NoteWork* self, u32 a, u32 b);
/* 0x80385AD4 - switches the pane onto animation pair `(a, b)`.  Same consumer (rule 2). */
void note_pane_set_anim_pair(struct NoteWork* self, u32 a, u32 b);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
/* 0x80385C90 - the note NPC record's motion number.  The map name
 * `qn_get_motion_no__FP7_QNPC_W` is a mangling, so the declaration sits at C++ scope and the
 * front-end reproduces it (docs/plan.md 6.5 rule 9); the consumer is
 * `lobby/lb_quest_screen.cpp`'s `note_pane_get_motion`. */
struct _QNPC_W;
u16 qn_get_motion_no(struct _QNPC_W* self);
#endif

#endif /* MHTRI_ENEMY_FN_80382310_H */
