/*
 * MSL_C/locale.h - the C locale records `MSL_C/locale.c` owns: the numeric-formatting record, the character-table
 *    descriptor, the per-category records and the record that points at them, as the string routines read them.
 */
#ifndef MSL_C_LOCALE_H
#define MSL_C_LOCALE_H

#include "types.h"

/* The per-locale character tables. */
typedef struct CtypeTables {
    /* +0x00 */ char locale_name[8];        /* "C" */
    /* +0x08 */ const u16* ctype_map;       /* 256 class flags per narrow character */
    /* +0x0C */ const u8* upper_map;        /* narrow toupper table */
    /* +0x10 */ const u8* lower_map;        /* narrow tolower table */
    /* +0x14 */ const u16* wctype_map;      /* wide class flags */
    /* +0x18 */ const u16* wupper_map;      /* wide toupper table */
    /* +0x1C */ const u16* wlower_map;      /* wide tolower table */
    /* +0x20 */ int (*mbtowc_fn)(u16* out, const char* in, u32 n, const struct CtypeTables* ctype); /* multibyte decoder */
    /* +0x24 */ int (*wctomb_fn)(char* out, u16 wc, const struct CtypeTables* ctype); /* multibyte encoder */
} CtypeTables; /* size: 0x28 */

/* The collation category of a locale. */
typedef struct CollateLocale {
    /* +0x00 */ char locale_name[8];        /* "C" */
    /* +0x08 */ u32 first_char;             /* first character covered by `char_weights` */
    /* +0x0C */ u32 last_weight;            /* highest weight in `char_weights` */
    /* +0x10 */ u32 pad_0x10;               /* zero */
    /* +0x14 */ const u16* char_weights;    /* collation weight per character from `first_char` */
    /* +0x18 */ u32 pad_0x18;               /* zero */
} CollateLocale; /* size: 0x1C */

/* The monetary category of a locale. */
typedef struct MonetaryLocale {
    /* +0x00 */ char locale_name[8];        /* "C" */
    /* +0x08 */ const char* mon_decimal_point;
    /* +0x0C */ const char* mon_thousands_sep;
    /* +0x10 */ const char* mon_grouping;
    /* +0x14 */ const char* positive_sign;
    /* +0x18 */ const char* negative_sign;
    /* +0x1C */ const char* currency_symbol;
    /* +0x20 */ s8 frac_digits;             /* CHAR_MAX when not available */
    /* +0x21 */ s8 p_cs_precedes;
    /* +0x22 */ s8 n_cs_precedes;
    /* +0x23 */ s8 p_sep_by_space;
    /* +0x24 */ s8 n_sep_by_space;
    /* +0x25 */ s8 p_sign_posn;
    /* +0x26 */ s8 n_sign_posn;
    /* +0x27 */ u8 pad_0x27;
    /* +0x28 */ const char* int_curr_symbol;
    /* +0x2C */ s8 int_frac_digits;
    /* +0x2D */ s8 int_p_cs_precedes;
    /* +0x2E */ s8 int_n_cs_precedes;
    /* +0x2F */ s8 int_p_sep_by_space;
    /* +0x30 */ s8 int_n_sep_by_space;
    /* +0x31 */ s8 int_p_sign_posn;
    /* +0x32 */ s8 int_n_sign_posn;
    /* +0x33 */ u8 pad_0x33;
} MonetaryLocale; /* size: 0x34 */

/* The numeric category of a locale. */
typedef struct NumericLocale {
    /* +0x00 */ char locale_name[8];        /* "C" */
    /* +0x08 */ const char* decimal_point;  /* "." */
    /* +0x0C */ const char* thousands_sep;  /* "" */
    /* +0x10 */ const char* grouping;       /* "" */
    /* +0x14 */ u32 pad_0x14;               /* zero */
} NumericLocale; /* size: 0x18 */

/* The time category of a locale. */
typedef struct TimeLocale {
    /* +0x00 */ char locale_name[8];        /* "C" */
    /* +0x08 */ const char* am_pm;          /* "AM|PM" */
    /* +0x0C */ const char* date_time_format; /* "%a %b %e %T %Y" */
    /* +0x10 */ const char* time_12h_format;  /* "%I:%M:%S %p" */
    /* +0x14 */ const char* date_format;      /* "%m/%d/%y" */
    /* +0x18 */ const char* time_format;      /* "%T" */
    /* +0x1C */ const char* weekday_names;    /* abbreviated and full names, `|` separated */
    /* +0x20 */ const char* month_names;      /* abbreviated and full names, `|` separated */
    /* +0x24 */ const char* era_string;       /* "" */
} TimeLocale; /* size: 0x28 */

/* The current locale record. */
typedef struct LocaleRecord {
    /* +0x00 */ const struct LocaleRecord* next;  /* next locale of the chain, 0 for "C" */
    /* +0x04 */ char locale_name[0x30];     /* "C" */
    /* +0x34 */ const CollateLocale* collate;
    /* +0x38 */ const CtypeTables* ctype;   /* the active character tables */
    /* +0x3C */ const MonetaryLocale* monetary;
    /* +0x40 */ const NumericLocale* numeric;
    /* +0x44 */ const TimeLocale* time;
} LocaleRecord; /* size: 0x48 */

/* The numeric-formatting record of the C locale. */
typedef struct LocaleConv {
    /* +0x00 */ const char* decimal_point;  /* "." */
    /* +0x04 */ const char* thousands_sep;
    /* +0x08 */ const char* grouping;
    /* +0x0C */ const char* mon_decimal_point;
    /* +0x10 */ const char* mon_thousands_sep;
    /* +0x14 */ const char* mon_grouping;
    /* +0x18 */ const char* positive_sign;
    /* +0x1C */ const char* negative_sign;
    /* +0x20 */ const char* currency_symbol;
    /* +0x24 */ s8 frac_digits;             /* CHAR_MAX when not available */
    /* +0x25 */ s8 p_cs_precedes;
    /* +0x26 */ s8 n_cs_precedes;
    /* +0x27 */ s8 p_sep_by_space;
    /* +0x28 */ s8 n_sep_by_space;
    /* +0x29 */ s8 p_sign_posn;
    /* +0x2A */ s8 n_sign_posn;
    /* +0x2B */ u8 pad_0x2b;
    /* +0x2C */ const char* int_curr_symbol;
    /* +0x30 */ s8 int_frac_digits;
    /* +0x31 */ s8 int_p_cs_precedes;
    /* +0x32 */ s8 int_n_cs_precedes;
    /* +0x33 */ s8 int_p_sep_by_space;
    /* +0x34 */ s8 int_n_sep_by_space;
    /* +0x35 */ s8 int_p_sign_posn;
    /* +0x36 */ s8 int_n_sign_posn;
    /* +0x37 */ u8 pad_0x37;
} LocaleConv; /* size: 0x38 */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8060EC00 - the C locale's numeric-formatting record. */
extern LocaleConv _lconv;

/* 0x8060EDB0 - the active locale record. */
extern LocaleRecord _current_locale;

#ifdef __cplusplus
}
#endif

#endif
