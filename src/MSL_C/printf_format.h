/*
 * MSL_C/printf_format.h - the conversion-specification enums shared by `MSL_C/printf.c` and `MSL_C/wprintf.c`.
 */
#ifndef MSL_C_PRINTF_FORMAT_H
#define MSL_C_PRINTF_FORMAT_H

enum Justification { LEFT_JUSTIFY = 0, RIGHT_JUSTIFY = 1, ZERO_FILL = 2 };
enum SignOption { SIGN_ONLY_MINUS = 0, SIGN_ALWAYS = 1, SIGN_SPACE = 2 };
enum ArgumentOption {
    ARG_NORMAL = 0,
    ARG_CHAR = 1,
    ARG_SHORT = 2,
    ARG_LONG = 3,
    ARG_LONG_LONG = 4,
    ARG_WIDE = 5,
    ARG_INTMAX = 6,
    ARG_SIZE_T = 7,
    ARG_PTRDIFF_T = 8,
    ARG_LONG_DOUBLE = 9
};

#endif
