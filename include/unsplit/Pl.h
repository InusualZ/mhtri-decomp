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

/* The 0x80229xxx motion/SE helper family `Pl/fn_80229ECC.cpp` dispatches into (all unregistered and
 * unmangled; the actor itself is the `_PLW` at their r3). */
void fn_80229CB4(struct _PLW* self);
void fn_80229E10(struct _se_w* work, s32 code, s32 val);
void fn_80229EA8(struct _se_w* work, s32 a, s32 b, s32 c);

/* 0x80244E88 - the per-motion effect dispatcher `Pl/fn_80229ECC.cpp` hands `&self->field_0xAF4`. */
void fn_80244E88(void* p, u32 a, u32 b, u32 c);

void fn_80267270(_PLW* plw, u32 a, u32 b, u32 c);
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
#endif

#endif /* MHTRI_UNSPLIT_PL_H */
