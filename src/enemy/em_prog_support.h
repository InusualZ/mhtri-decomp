/* The enemy note-pane band's shared support block `enemy/em_prog_support.cpp` (`.text` 0x80383148..0x80385EE0):
 * the pane helpers the program tail `enemy/em_prog_tail.cpp` calls (docs/plan.md 6.5 rule 2).
 */
#ifndef MHTRI_ENEMY_EM_PROG_SUPPORT_H
#define MHTRI_ENEMY_EM_PROG_SUPPORT_H

#include "types.h"
#include "enemy/note_work.h"

extern "C" {

/* The NPC sound callback `qnpc_snd_func_get` hands out (the shape `se_entry_request` takes). */
typedef void (*QnpcSndFunc)(struct _ENEMY_WORK* work, s32 event);
/* 0x80385414 - the NPC sound callback the quest NPC sets up (GUESS name). */
QnpcSndFunc qnpc_snd_func_get(void);
/* 0x80385B5C - starts motion `motion` on the pane's model whatever it is playing (GUESS name). */
void note_pane_motion_start(NoteWork* self, u16 motion, u32 blend, s32 frame);

/* 0x80384F80 - whether the quest NPC of (`map`, `area`) needs its models loaded (GUESS name). */
u32 qnpc_load_ck(u8 map, u8 area);
/* 0x80385118 - the `load_file_req` completion of one quest NPC model file: registers the read file in the resource
 * slot `*ctx` names (GUESS name). */
void qnpc_res_load_done(u32 data, s32 size, s32 flag, u32* ctx);

/* 0x80385AD4 - arms the pane's animation pair and runs the motion reset. */
void note_pane_set_anim_pair(NoteWork* self, u32 a, u32 b);

/* 0x80385AA0 - stores the pane's mode byte (GUESS name: the program states set it to 0 first). */
void note_pane_mode_set(NoteWork* self, u8 mode);

/* 0x80385BF4 - starts motion `motion` on the pane's model unless the quest NPC is already in it (GUESS name). */
void note_pane_motion_set(NoteWork* self, u16 motion, u32 b, u32 c);

/* 0x80385B5C - starts motion `motion` (up to 16) on the pane's model for `frames` frames.  GUESS name. */
void note_pane_motion_start(NoteWork* self, u16 motion, u32 b, s32 frames);

/* 0x80385C64 - whether the pane model's motion has ended (GUESS name: a tail call into the model's end test). */
u32 note_pane_motion_end_ck(NoteWork* self);

/* 0x80385B18 - the world matrix of joint `joint` of the pane's model (GUESS name). */
void note_pane_joint_mtx_get(NoteWork* self, u32 joint, nw4r::math::MTX34* out);

/* 0x80385C70 - whether the pane model's motion `a` is at frame `frame` (within `range`); the frame pair passes
 * through to the model's test in f1/f2 (GUESS name). */
u32 note_pane_motion_frame_ck(NoteWork* self, u32 a, f32 frame, f32 range);

/* 0x80385C98 - stores the quest NPC model's flag byte (GUESS name; the tail sets it to 90). */
void qn_chr_flag_set(_QNPC_W* self, u8 flag);

}

#endif /* MHTRI_ENEMY_EM_PROG_SUPPORT_H */
