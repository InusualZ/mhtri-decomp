/*
 * MSL_C/locale.h - the C locale records `MSL_C/locale.c` owns: the character-table descriptor and the record that
 *    points at it, as the string routines read them.
 */
#ifndef MSL_C_LOCALE_H
#define MSL_C_LOCALE_H

#include "types.h"

/* The per-locale character tables. */
typedef struct CtypeTables {
    /* +0x00 */ char locale_name[8];        /* "C" */
    /* +0x08 */ const u16* ctype_map;       /* 256 class flags per narrow character */
    /* +0x0C */ const s8* upper_map;        /* narrow toupper table */
    /* +0x10 */ const s8* lower_map;        /* narrow tolower table */
    /* +0x14 */ const u16* wctype_map;      /* wide class flags */
    /* +0x18 */ const u16* wupper_map;      /* wide toupper table */
    /* +0x1C */ const u16* wlower_map;      /* wide tolower table */
    /* +0x20 */ int (*mbtowc_fn)(u16* out, const char* in, u32 n); /* multibyte decoder */
    /* +0x24 */ int (*wctomb_fn)(char* out, u16 wc);               /* multibyte encoder */
} CtypeTables; /* size: 0x28 */

/* The current locale record. */
typedef struct LocaleRecord {
    /* +0x00 */ u8 pad_0x00[0x38];
    /* +0x38 */ const CtypeTables* ctype;   /* the active character tables */
    /* +0x3C */ u8 pad_0x3c[0xC];
} LocaleRecord; /* size: 0x48 */

#ifdef __cplusplus
extern "C" {
#endif

extern LocaleRecord _current_locale;

#ifdef __cplusplus
}
#endif

#endif
