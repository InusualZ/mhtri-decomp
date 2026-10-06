/* Declarations `enemy/em025_prog.cpp` owns from its effect-slot helpers (0x8019E840-0x8019EC38); the signatures are
 * the call sites' registers, and `fn_8019E9AC` is `s32` because its caller tests the result signed.
 */
#ifndef MHTRI_ENEMY_FN_8019ED34_H
#define MHTRI_ENEMY_FN_8019ED34_H

#include "types.h"

struct _ENEMY_WORK;

/* The pooled `.sdata2` floats `enemy/em025_prog.cpp` reads (bare marker symbols in the map), declared and never
 * defined. */
extern const f32 lbl_807985C8;
extern const f32 lbl_807985CC;
extern const f32 lbl_80798664;
extern const f32 lbl_807986A8;
extern const f32 lbl_807988F4;
extern const f32 lbl_807988F8;
extern const f32 lbl_807988FC;
extern const f32 lbl_80798900;
extern const f32 lbl_80798904;
extern const f32 lbl_8079893C;
extern const f32 lbl_80798948;
extern const f32 lbl_8079894C;
extern const f32 lbl_80798950;
extern const f32 lbl_80798954;
extern const f32 lbl_80798958;
extern const f32 lbl_8079895C;
extern const f32 lbl_80798960;
extern const f32 lbl_8079896C;
extern const f32 lbl_80798974;
extern const f32 lbl_80798978;
extern const f32 lbl_8079897C;
extern const f32 lbl_80798980;
extern const f32 lbl_80798984;
extern const f32 lbl_80798988;
extern const f32 lbl_8079898C;
extern const f32 lbl_80798990;
extern const f32 lbl_80798994;
extern const f32 lbl_80798998;
extern const f32 lbl_8079899C;
extern const f32 lbl_807989A0;
extern const f32 lbl_807989A4;
extern const f32 lbl_807989A8;
extern const f32 lbl_807989AC;
extern const f32 lbl_807989B0;
extern const f32 lbl_807989B4;
extern const f32 lbl_807989B8;
extern const f32 lbl_807989BC;
extern const f32 lbl_807989C0;
extern const f32 lbl_807989C4;
extern const f32 lbl_807989C8;
extern const f32 lbl_807989CC;
extern const f32 lbl_807989D0;
extern const f32 lbl_80798A48;
extern const f32 lbl_80798A4C;
extern const f32 lbl_80798A70;
extern const f32 lbl_80798A74;
extern const f32 lbl_80798A78;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80339E04 (`lobby/lb_companion_ui.cpp`): two arguments at the call site, of which the callee reads only r3. */
void lb_area_change_send(u8 a, s32 b);
/* r3 the work record, r4 the slot id; arms one action slot. */
void fn_8019E960(struct _ENEMY_WORK* self, s32 slot);
/* r3 the work record; returns the slot word the caller compares against 0. */
s32 fn_8019E9AC(struct _ENEMY_WORK* self, s32 slot);
/* r3 the work record: the callees at 0x801A9748 and 0x801A98F8 the band's state machines call. */
void fn_801A9748(struct _ENEMY_WORK* self);
void fn_801A98F8(struct _ENEMY_WORK* self);
/* r3 the work record; the per-joint slot release `fn_801A94C0` tail-calls. */
void fn_8019EA04(struct _ENEMY_WORK* self);
/* 0x8019E840 - r3 the work record; returns the 1/0 flag `fn_8019F07C` tests. */
u32 fn_8019E840(struct _ENEMY_WORK* self);
/* r3 the work record and three scalars. */
void fn_8019EC38(struct _ENEMY_WORK* self, s32 a, s32 b, s32 c);

#ifdef __cplusplus
}
#endif

#endif
