/*
 * The `g3d/g3d_anmchr.cpp` cluster's cross-unit declarations (docs/plan.md 6.5 rule 2).  A symbol a
 * registered unit owns is declared once, in that owner's header, and every consumer includes it; this
 * is that header for the g3d character-animation cluster registered from proposal `8005ABD8`
 * (`.text` 0x8005ABD8-0x80063888).
 *
 * `g3d/fn_800680CC.cpp` - the right-hand animation-object cluster (0x800680CC-0x8006EAC0) - calls
 * these nine `fn_XXXXXXXX`/`dtor_XXXXXXXX` stems and used to declare them locally, because the range
 * was unclaimed and its address band named no single module.  Registering `g3d_anmchr.cpp` makes them
 * owned, so the declarations move here and the consumer includes this header instead.
 *
 * All nine keep C linkage (the map carries plain `fn_XXXXXXXX`/`dtor_XXXXXXXX` stems).  The return
 * types are the ones the target bodies imply and the consumers use: 0x8005DC60/0x8005DCD0 store
 * through their first argument and return it; 0x800628A4 and 0x8005DC24 load through their argument.
 * The owner's own forward prototypes in `g3d_anmchr.cpp` predate this header and are ABI-identical.
 */
#ifndef MHTRI_G3D_G3D_ANMCHR_H
#define MHTRI_G3D_G3D_ANMCHR_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8005D1AC - r3 the output object, r4 a scalar; the clear/sub-init leaf
 * `enemy/fn_801A9540.cpp`'s `fn_801A9C6C` runs.  Added with that unit's registration (rule 2:
 * this range owns the address). */
void fn_8005D1AC(void* out, s32 a);
void **fn_8005DC60(void **out, void *v);   /* 0x8005DC60 - stores `v` through `out`, returns `out` */
void **fn_8005DCD0(void **out, void *v);   /* 0x8005DCD0 - stores `v` through `out`, returns `out` */
void dtor_8005D384(void *self, s32 flag);  /* 0x8005D384 - the teardown destructor */
void fn_8005D3E0(void *self);              /* 0x8005D3E0 - empty advance (`blr`) */
u32 fn_800628B4(void *self);               /* 0x800628B4 - `*(u32*)self != 0` */
void *fn_800628A4(void *self);             /* 0x800628A4 - loads the word at +0x0 of `self` */
void **fn_80062914(void **out, u32 v);     /* 0x80062914 - stores `v` through `out`, returns `out` */
void *fn_8006268C(void *self, u32 value);   /* 0x8006268C - stores `value` at +0x0 of `self`, returns `self`.
                                             * ONE declaration only: this branch's consumer
                                             * g3d/g3d_resanmtexsrt.cpp spells the parameters `(out, v)` and
                                             * main's g3d/g3d_resanmlight.cpp spells them `(self, value)`,
                                             * but the type is identical, so the merge keeps main's landed
                                             * line and drops the duplicate (a second, disagreeing-looking
                                             * declaration is the "illegal function overloading" trap). */

u32 fn_80062750(void *out, void *key);     /* 0x80062750 - the resource-table lookup */
u32 fn_8005DC24(u32 *p);                   /* 0x8005DC24 - loads the word at +0x0 of `p`, +4 */
void *fn_8005B1E4(void *self, u32 value);  /* 0x8005B1E4 - stores `value` at +0x0 of `self`, returns `self` */

/* Added when `g3d/g3d_resanmchr.cpp` registered (rule 2): the `ResAnmChr` channel evaluators call
 * back into this cluster's frame/rate helpers.  Types are the ones the target bodies imply. */
f32 fn_800610AC(f32 value);                /* 0x800610AC - the reciprocal helper */
void *fn_800618BC(void *self);             /* 0x800618BC - the resource-table base */
s32 fn_800628C8(void *self, s32 key);      /* 0x800628C8 - the table entry lookup */

/* 0x800600C0 - `GetParent()`, the `!GetParent()` assert's test; the ScnMdl destructor calls it.
 * Inside the `extern "C"` block: the map name is plain, so a C++ consumer must not mangle it
 * (`g3d_scnmdl.cpp` referenced `fn_800600C0__FPUl`).  The owner defines it `extern "C" u32
 * fn_800600C0(u32*)` in `g3d_anmchr.cpp`; `fn_80075DCC.cpp`'s zero-argument declaration is a
 * separate TU and its object already references the plain name. */
u32 fn_800600C0(u32* p);

#ifdef __cplusplus
}
#endif

/* The font API this range's address owns (0x8005C804 `font_set_size__Fss`, 0x8005C810
 * `font_print_ex__FsssPSce`, 0x8005C9A4 `font_flush__Fv`).  The owner defines the first and the
 * third at C++ scope (`g3d_anmchr.cpp:619`/`:639`); the second is the varargs engine the font
 * cluster wraps, and it is the `font_print_ex` spelling the map records.  Declared at C++ scope,
 * outside the `extern "C"` block above, so a consumer's call mangles to the map's names.
 * Added with `menu/menu_item.cpp` (rule 2: this range owns the three addresses); `font_set_size` is
 * also declared by `include/unsplit/lobby.h`, with the same signature, so a consumer may include
 * both. */
void font_set_size(s16 x, s16 y);
void font_flush(void);
void font_print_ex(s16 x, s16 y, s16 flag, s8* fmt, ...);

/* The font cluster's own text helpers, added with `menu/arena_result.cpp` (rule 2: this range owns
 * the three addresses).  All three keep the owner's own spelling and are declared at C++ scope so a
 * consumer's call mangles to what the target objects' relocations carry (`__FPc`, `__FPcPc`,
 * `__FPcl`): 0x8005B874 `flfntStrLen` (a `b` tail call into the decoding walker, so its length is
 * the walker's), 0x8005C9A8 `msg_str_gen` and 0x8005B878 `flKnjMsgNumPtr` (the pointer to the
 * `index`-th character of `s`, which the result screen truncates a long name at).  `flfntStrLen` is
 * the one name both lanes declare (`hud/layout.cpp` needs it too) and the two spellings are the same
 * declaration, so the merge keeps the landed line and declares it ONCE, with the owner's own
 * parameter name (`g3d_anmchr.cpp:599` defines `s32 flfntStrLen(char* s)`) - a second, differently
 * spelled copy on the same type is the `illegal function overloading` trap. */
s32 flfntStrLen(char* s);
/* The font cluster's position/size/colour setters and the print-flush-reset sequence the network band's
 * message renderer drives (owner: this range; C++ scope so a consumer's call mangles to `__Fll`, `__FUl`,
 * `__FPce`, `__Fv`). */
void flfntSetPos(s32 x, s32 y);
void flfntSetSize(s32 x, s32 y);
void flfntSetColor(u32 color);
void flfntPrintf(char* fmt, ...);
void flfntFlush(void);
void flfntStackReset(void);
void msg_str_gen(char* src, char* dst);
char* flKnjMsgNumPtr(char* s, s32 index);
/* 0x8005C7E0 - the substring search the font helpers share (a thin `strchr`); the owner defines it
 * at C linkage, as does `include/menu/menu_item_page.h`, which declares the same signature. */
extern "C" char* flfntStrChr(char* s, s32 c);

/* `utf82unicode2` (`0x8005BE40`) is the other string helper this range owns, added with
 * `hud/layout.cpp`: it returns the byte count of the UTF-8 character at the pointer, which is what a
 * scanner advances by.  Declared at C++ scope so the mangling matches the map's (`__FPUc`). */
s32 utf82unicode2(u8* str);

#endif /* MHTRI_G3D_G3D_ANMCHR_H */

