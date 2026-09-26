#ifndef MHTRI_UNSPLIT_PL_H
#define MHTRI_UNSPLIT_PL_H

#include "types.h"

struct _se_w;

/* Declarations for symbols with no registered owner whose address band names the `Pl` module
 * (docs/plan.md 6.5, rule 2).  Plain C-linkage names, so C-visible.
 */
struct _PLW;
struct _se_w;

#ifdef __cplusplus
extern "C" {
#endif

s8 fn_802748C8(void* a);

/* Pl-band helpers with no registered owner, called by `Pl/fn_80262940.cpp` (proposal /80262940,
 * `.text` 0x80262940-0x802693C4): they sit in the unclaimed runs 0x8024????-0x80262940 and
 * 0x80273B14-0x80276B58 / 0x8027D684-... , so this band header is their rule-2 home. */
u32 fn_80257E70(struct _PLW* self);
u32 fn_8025FA00(void* a, void* b);
u16 fn_80260A18(struct _PLW* self);
s32 fn_80261770(struct _PLW* self, u8* a, u16* b, u16* c, s32* d, u16* e, u16* f);
u32 fn_802621B0(struct _PLW* self, void* b);
s32 fn_80262688(struct _PLW* self);
u32 fn_802745DC(struct _PLW* self, u32 v);
s32 fn_80274AB8(s32 a);
s32 fn_80276254(struct _PLW* self, s32 v);
u32 fn_802764B0(struct _PLW* self, s32 v);
u32 fn_80276514(struct _PLW* self, s32 v);
void fn_80275AC4(struct _PLW* self, s32 a, u16 b, u16 c);
s32 fn_8027D7EC(struct _PLW* self, u8 flag);
s32 fn_8027E1E4(struct _PLW* self);
u32 fn_8027E220(struct _PLW* self, s32 v);

/* The 0x80229xxx motion/SE helper family `Pl/fn_80229ECC.cpp` dispatches into (all unregistered and
 * unmangled; the actor itself is the `_PLW` at their r3). */
void fn_80229CB4(struct _PLW* self);
void fn_80229E10(struct _se_w* work, s32 code, s32 val);
void fn_80229EA8(struct _se_w* work, s32 a, s32 b, s32 c);

/* 0x80244E88 - the per-motion effect dispatcher `Pl/fn_80229ECC.cpp` hands `&self->field_0xAF4`. */
void fn_80244E88(void* p, u32 a, u32 b, u32 c);

#ifdef __cplusplus
}

/* 0x803C4814 - `event_demo_ck__Fv`, a C++ free function the Pl/ef band reads.  Unregistered, and the
 * files that spell it `int` locally would clash with a `u32` in `unsplit/unknown.h`, so it lives
 * here. */
u32 event_demo_ck(void);

struct _PLW;

/* `Get_motion_no` is defined at 0x8026A308; the map spells it `Get_motion_no__FP4_PLW`, so the real
 * C++ declaration is the callable spelling and the front-end mangles it back (rule 9). */
u16 Get_motion_no(struct _PLW* plw);

/* 0x8026A248 / 0x8026A314 - the two character setters `src/lobby/fn_802076D4.cpp` drives.  The map
 * spells them `Pl_chr_setX__FP4_PLWUsll` and `Pl_frame_check__FP4_PLWUlff`, so the real declarations are
 * the callable spellings and the front-end mangles them back (rule 9).  Their addresses sit in the Pl
 * band's unclaimed gap (0x802693C4..0x8026BA1C), so this band header is their rule-2 home. */
void Pl_chr_setX(struct _PLW* plw, u16 motion, s32 a, s32 b);
u32 Pl_frame_check(struct _PLW* plw, u32 mask, f32 a, f32 b);
#endif

#endif /* MHTRI_UNSPLIT_PL_H */
