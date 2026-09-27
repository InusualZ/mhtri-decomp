/* The `sound` SE (`se_w`) band's shared records and the unsplit entry points of the SE request layer.
 *
 * docs/plan.md 6.5 rule 1: a type more than one unit uses is defined once and included.  The SE request
 * cluster `src/sound/fn_800D7F54.cpp` first needed these records; this unit is the second, so they are
 * stated here once.  The layouts and sizes are that unit's, traced from its disassembly (the 0x2966C
 * memset, the 32-slot walk at +0x3C, the 16-step ramp at +0x29238, the pointer array at `_PLW`+0xAFC).
 * The migration request in this unit's outbox asks `fn_800D7F54.cpp` to include this header and drop its
 * local copies; until then the two are textually identical.
 */
#ifndef MHTRI_SOUND_SE_H
#define MHTRI_SOUND_SE_H

#include "types.h"
#include "nw4r/math.h"

/* One of the 32 sound-slot records the SE work object carries at +0x3C.
 * size: 0x50 */
struct SeSlot {
    /* +0x00 */ u8 in_use;
    /* +0x01 */ u8 state;
    /* +0x02 */ u8 kind;
    /* +0x03 */ u8 field_0x03;
    /* +0x04 */ nw4r::math::VEC3 pos;
    /* +0x10 */ nw4r::math::VEC3 field_0x10;
    /* +0x1C */ f32 field_0x1C;
    /* +0x20 */ f32 field_0x20;
    /* +0x24 */ u8 field_0x24;
    /* +0x25 */ u8 field_0x25;
    /* +0x26 */ u8 pad_0x26[2];
    /* +0x28 */ u32 owner;
    /* +0x2C */ u32 id;
    /* +0x30 */ s32 field_0x30;
    /* +0x34 */ s32 field_0x34;
    /* +0x38 */ u32 param;
    /* +0x3C */ u32 field_0x3C;
    /* +0x40 */ u32 field_0x40;
    /* +0x44 */ u32 field_0x44;
    /* +0x48 */ s16 field_0x48;
    /* +0x4A */ u8 field_0x4A;
    /* +0x4B */ u8 field_0x4B;
    /* +0x4C */ s16 field_0x4C;
    /* +0x4E */ u8 pad_0x4E;
    /* +0x4F */ u8 field_0x4F;
};

/* The SE work object: one per sound source.  +0x08 and +0x0C are read by `se_req_pos_ps`.
 * size: 0x295F4 (at least; `fn_800D7F54` clears 0x2966C bytes) */
struct _se_w {
    /* +0x000 */ u8 pad_0x000[8];
    /* +0x008 */ s32 field_0x08;
    /* +0x00C */ s32 field_0x0C;
    /* +0x010 */ u8 pad_0x010[0x2C];
    /* +0x03C */ SeSlot slots[32];
    /* +0x0A3C */ u8 pad_0x0A3C[0x287FC];
    /* +0x29238 */ f32 ramp[16];
    /* +0x29278 */ f32 field_0x29278;
    /* +0x2927C */ u8 pad_0x2927C[0x368];
    /* +0x295E4 */ s32 voices[2];
    /* +0x295EC */ u8 pad_0x295EC[2];
    /* +0x295EE */ u8 voice_slot;
    /* +0x295EF */ u8 field_0x295EF;
    /* +0x295F0 */ s32 field_0x295F0;
};

/* The actor/motion object the SE work hangs off (`+0xAFC`) - the union of the views the band reads.
 * size: 0xB00 (at least)
 *
 * `include/pl.h` owns the full union-of-views definition of the same record (docs/plan.md 6.5 rule 1),
 * and a unit that needs both this header and that one used to fail with a redefinition.  This
 * narrower view is therefore only defined when pl.h has not been included. */
#ifndef PL_H
struct _PLW {
    /* +0x000 */ u8 pad_0x000[2];
    /* +0x002 */ u8 field_0x002;   /* the motion-kind byte the kind dispatch switches on */
    /* +0x003 */ u8 field_0x003;
    /* +0x004 */ u8 pad_0x004[4];
    /* +0x008 */ u8 field_0x008;
    /* +0x009 */ u8 pad_0x009[1];
    /* +0x00A */ u8 field_0x00A;
    /* +0x00B */ u8 pad_0x00B[0x0F];
    /* +0x01A */ u16 field_0x01A;
    /* +0x01C */ u8 pad_0x01C[0x1AC];
    /* +0x1C8 */ u32 field_0x1C8;
    /* +0x1CC */ u8 pad_0x1CC[0x2D8];
    /* +0x4A4 */ u32 field_0x4A4;
    /* +0x4A8 */ u8 pad_0x4A8[0xFF];
    /* +0x5A7 */ u8 field_0x5A7;   /* the SE part/voice index handed to the motion banks */
    /* +0x5A8 */ u8 pad_0x5A8[0x54C];
    /* +0xAF4 */ _se_w* field_0xAF4;
    /* +0xAF8 */ _se_w* field_0xAF8;
    /* +0xAFC */ _se_w* field_0xAFC;
};
#endif /* !PL_H */

/* The per-kind container `get_move_work_adrs` returns.
 * size: 0x238 (approximate) */
struct SeWork {
    /* +0x000 */ u8 pad_0x000[0x112];
    /* +0x112 */ u8 field_0x112;
    /* +0x113 */ u8 pad_0x113[0x25];
    /* +0x138 */ _se_w* se[64];
};

#ifdef __cplusplus
extern "C" {
#endif

/* C-linkage helpers of the SE request layer (`src/sound/fn_800D7F54.cpp`). */
void fn_800D9CC8(_se_w* work, s32 a, s32 b, s32 c);
u32 fn_800D8D8C(nw4r::math::VEC3* pos);
SeSlot* fn_800DA72C(s32 kind, s32 id, nw4r::math::VEC3* pos);
s32 fn_800DBB78(s32 bank, s32 id);

/* C-linkage helpers the model methods call (their owners are elsewhere in the game).
 * 0x80041E40 is owned by `src/mh3_pad.cpp`; this copy is normalised to that owner's body
 * (`returns dst`) - its header cannot be included from the `ef` band (VEC3_ctor/
 * setVec3 conflict, filed 2026-09-27). */
void fn_800D3ACC(void* sub);
void fn_8007E498(void* obj);
void fn_80054FE8(void* obj, s32 flag);
void fn_800E2680(void* sub);
int fn_80097F80(void* sub);
u32 fn_80098868(void* sub);

void fn_80093AA0(void* obj);
void CleanUpTracks(void* obj);
void* fn_80097F18(void* sub, void* arg);
void fn_8005D0CC(void* obj, void* sub);
int fn_8006FDCC(void* sub);
void fn_800532DC(void* dst, void* src);
void fn_80080B10(void* obj, s32 kind);
void fn_800810DC(void* obj, s32 flag);
SeSlot* fn_800D8E58(_se_w* work, s32 id);
/* 0x80047058 is `mh3_pad.cpp`'s: the retail body takes NO argument (it loads `Screen_w` itself), so
 * the `void* obj` this header carried was the call site's guess and collided with the owner. */
s32 fn_80047058(void);

/* this unit's read-only pool constants (referenced, not defined here - playbook 29) */
extern const f32 lbl_807963E0;
extern const f32 lbl_80796438;
/* the model's initial joint/motion table this unit points `MHchar::field_0x00` at */
extern u32 lbl_80597CC0[];
extern u32 lbl_80597CA0[];
extern const f32 lbl_80796450;
extern const f32 lbl_80796454;

#ifdef __cplusplus
}
#endif

/* The C++ workhorse; the compiler mangles it to `se_req_pos_ps__FP5_se_wllPQ34nw4r4math4VEC3`. */
SeSlot* se_req_pos_ps(_se_w* work, long id, long param, nw4r::math::VEC3* pos);

#endif /* MHTRI_SOUND_SE_H */
