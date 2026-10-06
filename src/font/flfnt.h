/* font/flfnt.h - the cross-unit declarations of `font/flfnt.cpp`, the game's font/debug-print console.
 *   The plain map stems are C linkage; the `flfnt*`/`font_*` entry points are at C++ scope so a call mangles
 *   to the map's names. */
#ifndef MHTRI_FONT_FLFNT_H
#define MHTRI_FONT_FLFNT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

void *fn_8005B1E4(void *self, u32 value);  /* 0x8005B1E4 - stores `value` at +0x0 of `self`, returns `self` */

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
/* 0x8005B740 `flfntGetPosX__Fv` (C++ scope) - the font pen's x position. */
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

#endif /* MHTRI_FONT_FLFNT_H */
