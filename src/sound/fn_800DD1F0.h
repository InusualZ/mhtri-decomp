/*
 * Declarations owned by `sound/fn_800DD1F0.cpp` (docs/plan.md 6.5 rule 2).  The effect manager's stage
 * size class comes from the sound unit's quest/challenge data.
 */
#ifndef MHTRI_SOUND_FN_800DD1F0_H
#define MHTRI_SOUND_FN_800DD1F0_H

#include "types.h"
#include "nw4r/math.h"
#include "gx.h"

#ifdef __cplusplus
class MHchar;
#endif

#ifdef __cplusplus
extern "C" {
#endif
/* 0x800E2F40 - sets material `index`'s alpha compare on the character model `chr` (nothing when the
 * model has no resource); the signature is the owner `sound/mhchar.cpp`'s definition. */
#ifdef __cplusplus
void mhchar_mat_tev_set(MHchar* chr, u32 index, _GXCompare comp0, u8 ref0, _GXAlphaOp op, _GXCompare comp1,
                        u8 ref1);
#endif



/* 0x800E1640 / 0x800E16DC / 0x800E2198 - the model-block setters the Pl part accessors tail-call.
 * The addresses sit inside this unit's range (0x800DD1F0-0x800E3CBC), so the declarations belong to
 * this header (rule 2); `Pl/fn_802693c4.cpp` and `Pl/pl_act.cpp` are the call sites.  The value
 * setters' return is `u32` because `Pl_frame_check`/`Pl_motion_end_ck` pass it straight back to their own
 * callers, which compare it against 1. */
void mhchar_motion_speed_set(MHchar* chr, f32 value);
u32 mhchar_frame_check(MHchar* chr, u16 motion, s32 flag);
u32 mhchar_motion_end_ck(MHchar* chr, s32 flag);
void mhchar_mat_blend_set(MHchar* chr, u32 index, f32 value);
/* 0x800E0A14 - the joint's world position through the model; the same signature `unsplit/sound.h`
 * carries, so a TU that includes both headers sees one declaration, not an overload. */
void mhchar_joint_mtx_get(void* chr, u32 joint, Mtx34* out);
/* 0x800DD514 - the fixed SE request the effect step makes when its joint window is hit (the body
 * loads 46/13 and tail-calls the SE request path).  Added with `ef/fn_8030681C.cpp`. */
void se_bank46_id13_req(nw4r::math::VEC3* pos);
/* 0x800E26C4 - releases the model/SE handle the caller's record holds.  Added with
 * `lobby/lb_quest_ui.cpp`: its per-row effect step hands the record's pointer here and then flags
 * the record through `ef/eft_res.cpp`'s `eft_res_models_release`, so the declaration belongs with this unit
 * (rule 2) rather than in the lobby source. */
void mhchar_model_release(void* handle);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_FN_800DD1F0_H */
