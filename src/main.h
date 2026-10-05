/*
 * `main.cpp`'s shared declarations (docs/plan.md 6.5 rule 2: a declaration lives with the TU that owns
 * the symbol).  `main.cpp` defines these; the consumers - `g3d/g3d_camera.cpp`, `sys_mem.cpp`, later the
 * rest of the SDK units - include this header instead of re-declaring them.
 *
 * Keep it minimal: only the declarations a consumer needs.
 */
#ifndef MHTRI_MAIN_H
#define MHTRI_MAIN_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The screen scissor geometry `fn_8004028C` points at (only the two u16 dims are read). `main.cpp`
 * defines the accessor with a `u16*` return; the type stays opaque to the consumers. */
u16* fn_8004028C(void);
f32 fn_8004029C(void);

/* The game allocator pair `sys_mem.cpp` wraps around the exp heap. */
void* fn_80040420(u32 size);
void fn_80040460(void* block);

/* Writes the scissor rectangle `main.cpp` keeps (`_MH_VEC2` there; a two-float pair). */
struct _MH_VEC2;
void fn_8004030C(struct _MH_VEC2* v);

#ifdef __cplusplus
}  /* extern "C" */

/* The two `main.cpp` symbols whose map names are C++ manglings (`get_ScreenSize__FP8_MH_VEC2`,
 * `ck_WideMode__Fv`); they keep C++ linkage so the linker sees those names. */
struct _MH_VEC2;
void get_ScreenSize(_MH_VEC2* v);
int ck_WideMode(void);
/* 0x8003FBFC `check_change_widemode_flag__Fv` - whether a video-mode change was requested. */
u8 check_change_widemode_flag(void);
#endif

#endif /* MHTRI_MAIN_H */
