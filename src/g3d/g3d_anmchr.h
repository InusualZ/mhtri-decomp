/* g3d/g3d_anmchr.h - the cross-unit declarations of `g3d/g3d_anmchr.cpp` (C linkage, plain map stems).
 *   0x8005DC60/0x8005DCD0 store through their first argument and return it; 0x800628A4 and 0x8005DC24 load through
 *   theirs.  The owner's own forward prototypes are ABI-identical. */
#ifndef MHTRI_G3D_G3D_ANMCHR_H
#define MHTRI_G3D_G3D_ANMCHR_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8005D1AC - the clear/sub-init leaf `enemy/fn_801A9540.cpp`'s fn_801A9C6C runs (r3 the output object, r4 a
 * scalar). */
void fn_8005D1AC(void* out, s32 a);
void **fn_8005DC60(void **out, void *v);   /* 0x8005DC60 - stores `v` through `out`, returns `out` */
void **fn_8005DCD0(void **out, void *v);   /* 0x8005DCD0 - stores `v` through `out`, returns `out` */
void dtor_8005D384(void *self, s32 flag);  /* 0x8005D384 - the teardown destructor */
void fn_8005D3E0(void *self);              /* 0x8005D3E0 - empty advance (`blr`) */
u32 fn_800628B4(void *self);               /* 0x800628B4 - `*(u32*)self != 0` */
void *fn_800628A4(void *self);             /* 0x800628A4 - loads the word at +0x0 of `self` */
void **fn_80062914(void **out, u32 v);     /* 0x80062914 - stores `v` through `out`, returns `out` */
void *fn_8006268C(void *self, u32 value);   /* 0x8006268C - stores `value` at +0x0 of `self`, returns `self`. */

u32 fn_80062750(void *out, void *key);     /* 0x80062750 - the resource-table lookup */
u32 fn_8005DC24(u32 *p);                   /* 0x8005DC24 - loads the word at +0x0 of `p`, +4 */
void *fn_8005B1E4(void *self, u32 value);  /* 0x8005B1E4 - stores `value` at +0x0 of `self`, returns `self` */

/* The frame/rate helpers `g3d/g3d_resanmchr.cpp`'s channel evaluators call (types the target bodies imply). */
f32 fn_800610AC(f32 value);                /* 0x800610AC - the reciprocal helper */
void *fn_800618BC(void *self);             /* 0x800618BC - the resource-table base */
s32 fn_800628C8(void *self, s32 key);      /* 0x800628C8 - the table entry lookup */

/* 0x800600C0 - `GetParent()`, the `!GetParent()` assert's test the ScnMdl destructor calls; C linkage, because the
 * map name is plain. */
u32 fn_800600C0(u32* p);

/* 0x8005BF68 - the display width of the character at `glyph`: 1 for a half-width one, 2 otherwise
 * (GUESS name, from the 1/2 result the network message layout adds to its line width). */
u16 getGlyphWidth(const char* glyph);

#ifdef __cplusplus
}
#endif

/* The font API (0x8005C804 `font_set_size__Fss`, 0x8005C810 `font_print_ex__FsssPSce`, the varargs engine, and
 * 0x8005C9A4 `font_flush__Fv`), at C++ scope so a call mangles to the map's names; `unsplit/lobby.h` declares
 * `font_set_size` with the same signature. */
void font_set_size(s16 x, s16 y);
/* 0x8005B740 `flfntGetPosX__Fv` (`g3d_anmchr.cpp`, C++ scope) - the font pen's x position. */
s16 flfntGetPosX(void);
void font_flush(void);
void font_print_ex(s16 x, s16 y, s16 flag, s8* fmt, ...);

/* The font cluster's text helpers, at C++ scope so a call mangles to the relocations' `__FPc`/`__FPcPc`/`__FPcl`:
 * 0x8005B874 `flfntStrLen` (a tail call into the decoding walker), 0x8005C9A8 `msg_str_gen` and 0x8005B878
 * `flKnjMsgNumPtr` (the pointer to the `index`-th character of `s`). */
s32 flfntStrLen(char* s);
/* The font cluster's position/size/colour setters and print-flush-reset sequence, at C++ scope so a call mangles
 * to `__Fll`, `__FUl`, `__FPce`, `__Fv`. */
void flfntSetPos(s32 x, s32 y);
void flfntSetSize(s32 x, s32 y);
void flfntSetColor(u32 color);
void flfntPrintf(char* fmt, ...);
void flfntFlush(void);
void flfntStackReset(void);
void msg_str_gen(char* src, char* dst);
char* flKnjMsgNumPtr(char* s, s32 index);
/* 0x8005C7E0 - the substring search the font helpers share (a thin `strchr`); the owner defines it
 * at C linkage, as does `menu/menu_item_page.h`, which declares the same signature. */
extern "C" char* flfntStrChr(char* s, s32 c);

/* 0x8005BE40 `utf82unicode2` - the byte count of the UTF-8 character at the pointer; C++ scope (`__FPUc`). */
s32 utf82unicode2(u8* str);

#endif /* MHTRI_G3D_G3D_ANMCHR_H */

