/*
 * Declarations for the symbols `src/fn_80047398.cpp` owns (docs/plan.md 6.5, rule 2).  `drawSpr2TF`
 * and `subTransSet` are C++ manglings (`drawSpr2TF__FUcP9fltSpr2TFUc`, `subTransSet__FUllPUl`), so
 * they are declared at global scope with the owner's parameter types - never inside `extern "C"`,
 * which would ask the linker for an unmangled name.
 */
#ifndef MHTRI_FN_80047398_H
#define MHTRI_FN_80047398_H

#include "types.h"

/* The 2D sprite record `drawSpr2TF` consumes. size: 0x18 */
typedef struct fltSpr2TF {
    /* +0x00 */ s16 x;
    /* +0x02 */ s16 y;
    /* +0x04 */ s16 w;
    /* +0x06 */ s16 h;
    /* +0x08 */ s16 angle; /* degrees; drawSpr2TF scales it to radians */
    /* +0x0A */ s16 pad_0x0A;
    /* +0x0C */ u32 color;
    /* +0x10 */ s16 r;
    /* +0x12 */ s16 g;
    /* +0x14 */ s16 b;
    /* +0x16 */ s16 a;
} fltSpr2TF; /* size: 0x18 */

#ifdef __cplusplus
void drawSpr2TF(u8 id, fltSpr2TF* spr, u8 flag);
void subTransSet(u32 a, s32 b, u32* c);
#endif

#ifdef __cplusplus
/* 0x800497AC - the node-buffer allocator the ScnMdl replacement passes call (rule 2).  The target
 * object references the plain name, so C linkage - it belongs inside the `extern "C"` block. */
extern "C" {
#endif
s32 fn_800497AC(void* pSelf);

/* Added when `camera/fn_802B5C58.cpp` registered (rule 2).  0x80047398 returns the current
 * `nw4r::g3d::Camera` (through `ScnRoot`); every camera accessor starts from it. */
void* fn_80047398(void);
/* 0x8004C4F0 - the record copy the light unit's copy constructor calls; added with the
 * `light/light.cpp` registration (rule 2: this range owns the address). */
void fn_8004C4F0(u8* dst, const u8* src);

/* Added with the `ef/eft052.cpp` registration (rule 2: this range owns every one of these
 * addresses - the cabinet/item-page helpers the cockpit hold band calls). */
u16 fn_8004AE70(void* userdata);
s32 fn_8004AEC0(void* userdata);
s32 fn_8004AF0C(u8 idx);
s16 fn_8004AF20(void* userdata);
void* fn_8004AF60(void* userdata, u8 idx);
void* fn_8004AF78(void* userdata);
u32 fn_8004B064(u16 id, void* a, s32 b);
u32 fn_8004B70C(u16 id, void* a, u16 b);
void fn_8004B200(void* userdata, u16 id, s16 delta);
s16 fn_8004B624(void* userdata, u16 id);
s16 fn_8004B7B0(u16 id, void* a, u16 b);
void fn_8004BCBC(void* userdata, u16 id, s16 count, s32 flag);
void fn_8004BEA4(u16 id, s16 count, void* out);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_FN_80047398_H */

