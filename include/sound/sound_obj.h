/* sound/sound_obj.h - the declarations `sound/sound_obj.cpp` owns that other units call (docs/plan.md 6.5 rule 2).  The signatures are the ones the callers use;
 * a function the unit has not written yet is declared from its callers. */
#ifndef MHTRI_SOUND_SOUND_OBJ_H
#define MHTRI_SOUND_SOUND_OBJ_H

#include "types.h"

struct ReverbMgr;
struct Sound;
struct Sound2;

#ifdef __cplusplus
extern "C" {
#endif
Sound* fn_800E5714(void);
void fn_800E5F40(Sound2* self);
ReverbMgr* fn_800E66D0(void);
/* untyped: an opaque handle passed through (the callee reads no field of it) */
void fn_800E66DC(void* unused, u32 owner, u32 kind, u8 a, u8 b, u8 c, u8 d, s16 e, s16 f);
void* fn_800E6764(Sound* snd, u32 a, u32 b, u32 c);
void* fn_800E6818(Sound* snd, u32 a, u32 b, u32 c, u32 d);
/* untyped: an opaque handle passed through (the callee reads no field of it) */
void* fn_800E6A18(void* p, u32 idx);
void* fn_800E6A58(Sound* snd, u32 a, u32 b, u32 c, u32 d);
/* untyped: an opaque handle passed through (the callee reads no field of it) */
u32 fn_800E7700(void* unused, u32 key);
void fn_800E778C(Sound* snd, u32 idx, u32 entry_idx);
/* untyped: an opaque handle passed through (the callee reads no field of it) */
s16 fn_800E78F8(void* unused, u32 key);
/* untyped: an opaque handle passed through (the callee reads no field of it) */
void fn_800E7990(void* unused, u32 key, u16 pitch);
#ifdef __cplusplus
}
#endif

#endif
