/*
 * MSL_C/ansi_fp.h - the floating-point to decimal conversions, owned by `MSL_C/ansi_fp.c`.
 */
#ifndef MSL_C_ANSI_FP_H
#define MSL_C_ANSI_FP_H

#include "types.h"

/* The significant digits of a decimal. */
typedef struct DecimalDigits {
    /* +0x00 */ u8 length;                  /* digit count */
    /* +0x01 */ u8 text[36];                /* ASCII digits, or `I` / `N` for infinity / NaN */
} DecimalDigits; /* size: 0x25 */

/* The decimal digits of a floating-point value. */
typedef struct decimal {
    /* +0x00 */ char sign;                  /* 0 positive, 1 negative */
    /* +0x01 */ char unused;
    /* +0x02 */ s16 exp;                    /* decimal exponent of the first digit */
    /* +0x04 */ DecimalDigits sig;
} decimal; /* size: 0x2A */

/* How `__num2dec` renders a value. */
typedef struct decform {
    /* +0x00 */ char style;                 /* 0 floating, 1 fixed */
    /* +0x02 */ s16 digits;                 /* significant digits requested */
} decform; /* size: 0x4 */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8045A248 (0x1A4): converts `x` to its decimal digits under `form`. */
void __num2dec(const decform* form, f64 x, decimal* d);

/* 0x8045A3EC (0x74C): converts decimal digits back to the nearest f64. */
f64 __dec2num(const decimal* d);

#ifdef __cplusplus
}
#endif

#endif
