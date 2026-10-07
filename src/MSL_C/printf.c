/*
 * MSL_C/printf.c - the formatted output engine: `parse_format`, `long2str`, `longlong2str`, `double2hex`,
 *    `round_decimal`, `float2str`, `__pformatter` and the `printf` / `vprintf` / `sprintf` / `snprintf` family with
 *    the file and string writers.
 *
 * RANGE. .text 0x8045BADC..0x8045DFA0 (15 functions in the map, 0x24C4 B); .rodata 0x80572B28..0x80572B50; .data
 *    0x8060EDF8..0x8060F028; .sdata 0x80793CD8..0x80793CE0; .sdata2 0x8079C9F8..0x8079CA00.
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi`.
 * NAMES. file name GUESS (MSL `printf`); `vsprintf` (0x8045DD54), `round_decimal` (0x8045C950) and the `print_format`
 *    field names are GUESSes from the MSL scheme; `_lconv` (0x8060EC00, owned by `MSL_C/locale.c`) is a GUESS for the
 *    record whose first word is the decimal-point string.
 * EVIDENCE. `.sdata` `@wstringBase0` (0x80793CD8, the empty wide string), the string base `.rodata` 0x80572B28
 *    (`-INF` ... `nan` and an empty string, 0x25 B), the jump tables `.data` 0x8060EDF8 and 0x8060EED8 and the pooled
 *    `0.0` at `.sdata2` 0x8079C9F8 are all read here; the pool value `0.0` repeats at 0x8079C9A8 (floating-point
 *    conversion unit) and 0x8079CA10 (string-to-float unit), which makes this a TU of its own. `vsprintf` and
 *    `sprintf` are `vsnprintf` with an unbounded count (inlined).
 * RESIDUALS. `__pformatter` 87: the bad-conversion tail is shared through a null test after the switch where the
 *    target jumps to one block, the integer fetch is an if chain where the target keeps one body per option, and the
 *    callee-saved assignment differs; `float2str` 92 and `double2hex` 89: the `isupper` test is folded for a `u8`
 *    argument where the target keeps the range compare, block order of the inf/nan cases, register names;
 *    `round_decimal` 89.6: the target shares one zero-length block between the early and the final return;
 *    `longlong2str` 97.7: register names; `__FileWrite` 95: the target keeps an empty then-branch jump; `.sdata` /
 *    `.rodata` 2 B / 3 B short of the map rows (padding).
 * SHAPES. the format record is built in a local and copied out; by-value `print_format` arguments are copied by the
 *    caller; the writers are reached through the `WriteProc` pointer.
 */

#include "MSL_C/ansi_files.h"
#include "MSL_C/ansi_fp.h"
#include "MSL_C/ctype.h"
#include "MSL_C/file_io.h"
#include "MSL_C/printf.h"
#include "MSL_C/wchar_io.h"
#include "MSL_C/abort_exit.h"
#include "MSL_C/mbstring.h"
#include "MSL_C/mem.h"
#include "MSL_C/string.h"
#include "MSL/strlen.h"
#include "Runtime.PPCEABI.H/__va_arg.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "stdarg.h"

/* The opaque argument the writers receive (a FILE or an output-string record). */
typedef struct WriteArg WriteArg;
typedef WriteArg* (*WriteProc)(WriteArg*, const char*, u32);

/* The state of a string sink. */
typedef struct OutStrCtrl {
    /* +0x00 */ char* char_str;         /* destination buffer */
    /* +0x04 */ u32 max_char_count;     /* capacity, including the terminator */
    /* +0x08 */ u32 chars_written;      /* characters stored so far */
} OutStrCtrl; /* size: 0xC */

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

/* One parsed conversion specification. */
typedef struct print_format {
    /* +0x00 */ u8 justification_options;   /* Justification */
    /* +0x01 */ u8 sign_options;            /* SignOption */
    /* +0x02 */ u8 precision_specified;     /* a `.` precision was given */
    /* +0x03 */ u8 alternate_form;          /* `#` was given */
    /* +0x04 */ u8 argument_options;        /* ArgumentOption of the length modifier */
    /* +0x05 */ u8 conversion_char;         /* the conversion, 0xFF when invalid */
    /* +0x06 */ u8 pad_0x06[2];
    /* +0x08 */ int field_width;
    /* +0x0C */ int precision;
} print_format; /* size: 0x10 */

/* 0x8045BADC (0x5BC): parses one conversion specification after a `%` into `format`; returns the next format character. */
static const char* parse_format(const char* format_string, va_list arg, print_format* format)
{
    const char* s = format_string;
    print_format f;
    int c;
    int flag;

    c = *++s;
    f.justification_options = RIGHT_JUSTIFY;
    f.sign_options = SIGN_ONLY_MINUS;
    f.precision_specified = 0;
    f.alternate_form = 0;
    f.argument_options = ARG_NORMAL;
    f.field_width = 0;
    f.precision = 0;

    if (c == '%') {
        f.conversion_char = c;
        *format = f;
        return s + 1;
    }

    for (;;) {
        flag = 1;
        switch (c) {
        case '-':
            f.justification_options = LEFT_JUSTIFY;
            break;
        case '+':
            f.sign_options = SIGN_ALWAYS;
            break;
        case ' ':
            if (f.sign_options != SIGN_ALWAYS) {
                f.sign_options = SIGN_SPACE;
            }
            break;
        case '#':
            f.alternate_form = 1;
            break;
        case '0':
            if (f.justification_options != LEFT_JUSTIFY) {
                f.justification_options = ZERO_FILL;
            }
            break;
        default:
            flag = 0;
            break;
        }
        if (!flag) {
            break;
        }
        c = *++s;
    }

    if (c == '*') {
        f.field_width = *(int*)__va_arg(arg, 1);
        if (f.field_width < 0) {
            f.justification_options = LEFT_JUSTIFY;
            f.field_width = -f.field_width;
        }
        c = *++s;
    } else {
        while (ctype_class(c, CTYPE_DIGIT)) {
            f.field_width = f.field_width * 10 + c - '0';
            c = *++s;
        }
    }

    if (f.field_width > 509) {
        f.conversion_char = 0xFF;
        *format = f;
        return s + 1;
    }

    if (c == '.') {
        c = *++s;
        f.precision_specified = 1;
        if (c == '*') {
            f.precision = *(int*)__va_arg(arg, 1);
            if (f.precision < 0) {
                f.precision_specified = 0;
            }
            c = *++s;
        } else {
            while (ctype_class(c, CTYPE_DIGIT)) {
                f.precision = f.precision * 10 + c - '0';
                c = *++s;
            }
        }
    }

    flag = 1;
    switch (c) {
    case 'h':
        f.argument_options = ARG_SHORT;
        if (s[1] == 'h') {
            f.argument_options = ARG_CHAR;
            c = *++s;
        }
        break;
    case 'l':
        f.argument_options = ARG_LONG;
        if (s[1] == 'l') {
            f.argument_options = ARG_LONG_LONG;
            c = *++s;
        }
        break;
    case 'L':
        f.argument_options = ARG_LONG_DOUBLE;
        break;
    case 'j':
        f.argument_options = ARG_INTMAX;
        break;
    case 't':
        f.argument_options = ARG_PTRDIFF_T;
        break;
    case 'z':
        f.argument_options = ARG_SIZE_T;
        break;
    default:
        flag = 0;
        break;
    }
    if (flag) {
        c = *++s;
    }

    f.conversion_char = c;
    switch (c) {
    case 'd':
    case 'i':
    case 'u':
    case 'o':
    case 'x':
    case 'X':
        if (f.argument_options == ARG_LONG_DOUBLE) {
            f.conversion_char = 0xFF;
        } else if (!f.precision_specified) {
            f.precision = 1;
        } else if (f.justification_options == ZERO_FILL) {
            f.justification_options = RIGHT_JUSTIFY;
        }
        break;
    case 'f':
    case 'F':
        if ((u8)(f.argument_options + 0xFA) <= 2 || f.argument_options == ARG_SHORT || f.argument_options == ARG_LONG_LONG) {
            f.conversion_char = 0xFF;
        } else if (!f.precision_specified) {
            f.precision = 6;
        }
        break;
    case 'a':
    case 'A':
        if (!f.precision_specified) {
            f.precision = 13;
        }
        if ((u8)(f.argument_options + 0xFA) <= 2 || (u8)(f.argument_options + 0xFF) <= 1 || f.argument_options == ARG_LONG_LONG) {
            f.conversion_char = 0xFF;
        }
        break;
    case 'g':
    case 'G':
        if (f.precision == 0) {
            f.precision = 1;
        }
        /* fall through */
    case 'e':
    case 'E':
        if ((u8)(f.argument_options + 0xFA) <= 2 || (u8)(f.argument_options + 0xFF) <= 1 || f.argument_options == ARG_LONG_LONG) {
            f.conversion_char = 0xFF;
        } else if (!f.precision_specified) {
            f.precision = 6;
        }
        break;
    case 'p':
        f.conversion_char = 'x';
        f.alternate_form = 1;
        f.argument_options = ARG_LONG;
        f.precision = 8;
        break;
    case 'c':
        if (f.argument_options == ARG_LONG) {
            f.argument_options = ARG_WIDE;
        } else if (f.precision_specified || f.argument_options != ARG_NORMAL) {
            f.conversion_char = 0xFF;
        }
        break;
    case 's':
        if (f.argument_options == ARG_LONG) {
            f.argument_options = ARG_WIDE;
        } else if (f.argument_options != ARG_NORMAL) {
            f.conversion_char = 0xFF;
        }
        break;
    case 'n':
        if (f.argument_options == ARG_LONG_DOUBLE) {
            f.conversion_char = 0xFF;
        }
        break;
    default:
        f.conversion_char = 0xFF;
        break;
    }

    *format = f;
    return s + 1;
}

/* 0x8045C098 (0x240): renders a signed or unsigned long as digits, filling backwards from the end of `buff`. */
static char* long2str(long num, char* buff, print_format format)
{
    u32 unsigned_num;
    u32 base;
    int digit;
    int ch;
    char* p;
    int n;
    int minus;

    minus = 0;
    n = 0;
    p = buff;
    *--p = 0;
    if (num == 0 && format.precision == 0 && !(format.alternate_form && format.conversion_char == 'o')) {
        return p;
    }

    switch (format.conversion_char) {
    case 'd':
    case 'i':
        base = 10;
        if (num < 0) {
            if (num != (long long)0x8000000000000000LL) {
                num = -num;
            }
            minus = 1;
        }
        break;
    case 'o':
        base = 8;
        format.sign_options = SIGN_ONLY_MINUS;
        break;
    case 'u':
        base = 10;
        format.sign_options = SIGN_ONLY_MINUS;
        break;
    case 'x':
    case 'X':
        base = 16;
        format.sign_options = SIGN_ONLY_MINUS;
        break;
    }

    do {
        unsigned_num = num;
        digit = unsigned_num % base;
        num = unsigned_num / base;
        if (digit < 10) {
            ch = '0' + digit;
        } else {
            ch = 'A' - 10 + digit;
            if (format.conversion_char == 'x') {
                ch = 'a' - 10 + digit;
            }
        }
        *--p = ch;
        n++;
    } while (num != 0);

    if (base == 8 && format.alternate_form && *p != '0') {
        *--p = '0';
        n++;
    }
    if (format.justification_options == ZERO_FILL) {
        format.precision = format.field_width;
        if (minus || format.sign_options) {
            format.precision--;
        }
        if (base == 16 && format.alternate_form) {
            format.precision -= 2;
        }
    }
    if (format.precision + (buff - p) > 509) {
        return NULL;
    }
    while (n < format.precision) {
        *--p = '0';
        n++;
    }
    if (base == 16 && format.alternate_form) {
        *--p = format.conversion_char;
        *--p = '0';
    }
    if (minus) {
        *--p = '-';
    } else if (format.sign_options == SIGN_ALWAYS) {
        *--p = '+';
    } else if (format.sign_options == SIGN_SPACE) {
        *--p = ' ';
    }
    return p;
}

/* 0x8045C2D8 (0x2B0): renders a signed or unsigned long long as digits, filling backwards from the end of `buff`. */
static char* longlong2str(long long num, char* buff, print_format format)
{
    u64 unsigned_num;
    u64 base;
    int digit;
    int ch;
    char* p;
    int n;
    int minus;

    minus = 0;
    n = 0;
    p = buff;
    *--p = 0;
    if (num == 0 && format.precision == 0 && !(format.alternate_form && format.conversion_char == 'o')) {
        return p;
    }

    switch (format.conversion_char) {
    case 'd':
    case 'i':
        base = 10;
        if (num < 0) {
            if (num != (long long)0x8000000000000000LL) {
                num = -num;
            }
            minus = 1;
        }
        break;
    case 'o':
        base = 8;
        format.sign_options = SIGN_ONLY_MINUS;
        break;
    case 'u':
        base = 10;
        format.sign_options = SIGN_ONLY_MINUS;
        break;
    case 'x':
    case 'X':
        base = 16;
        format.sign_options = SIGN_ONLY_MINUS;
        break;
    }

    do {
        unsigned_num = num;
        digit = unsigned_num % base;
        num = unsigned_num / base;
        if (digit < 10) {
            ch = '0' + digit;
        } else {
            ch = 'A' - 10 + digit;
            if (format.conversion_char == 'x') {
                ch = 'a' - 10 + digit;
            }
        }
        *--p = ch;
        n++;
    } while (num != 0);

    if (base == 8 && format.alternate_form && *p != '0') {
        *--p = '0';
        n++;
    }
    if (format.justification_options == ZERO_FILL) {
        format.precision = format.field_width;
        if (minus || format.sign_options) {
            format.precision--;
        }
        if (base == 16 && format.alternate_form) {
            format.precision -= 2;
        }
    }
    if (format.precision + (buff - p) > 509) {
        return NULL;
    }
    while (n < format.precision) {
        *--p = '0';
        n++;
    }
    if (base == 16 && format.alternate_form) {
        *--p = format.conversion_char;
        *--p = '0';
    }
    if (minus) {
        *--p = '-';
    } else if (format.sign_options == SIGN_ALWAYS) {
        *--p = '+';
    } else if (format.sign_options == SIGN_SPACE) {
        *--p = ' ';
    }
    return p;
}

/* 0x8045C588 (0x3C8): renders a double as a hexadecimal floating-point literal (or as INF / NAN), filling backwards from the end of `buff`. */
static char* double2hex(f64 num, char* buff, print_format format)
{
    decimal dec;
    decform form;
    print_format exp_format;
    char* p;
    int i;
    int bit;
    int shift;
    u8 nibble;
    int exp_field;
    int digit;
    u8* bytes;
    u8 radix = *_lconv.decimal_point;

    if (format.precision > 509) {
        return NULL;
    }

    form.style = 0;
    form.digits = 32;
    __num2dec(&form, num, &dec);

    switch (dec.sig.text[0]) {
    case 'I':
        if (dec.sign) {
            p = buff - 5;
            if (format.conversion_char == 'A') {
                strcpy(p, "-INF");
            } else {
                strcpy(p, "-inf");
            }
        } else {
            p = buff - 4;
            if (format.conversion_char == 'A') {
                strcpy(p, "INF");
            } else {
                strcpy(p, "inf");
            }
        }
        return p;
    case 'N':
        if (dec.sign) {
            p = buff - 5;
            if (format.conversion_char == 'A') {
                strcpy(p, "-NAN");
            } else {
                strcpy(p, "-nan");
            }
        } else {
            p = buff - 4;
            if (format.conversion_char == 'A') {
                strcpy(p, "NAN");
            } else {
                strcpy(p, "nan");
            }
        }
        return p;
    case '0':
        dec.exp = 0;
        /* fall through */
    default:
        bytes = (u8*)&num;
        exp_format.justification_options = RIGHT_JUSTIFY;
        exp_format.sign_options = SIGN_ALWAYS;
        exp_format.precision_specified = 0;
        exp_format.alternate_form = 0;
        exp_format.argument_options = ARG_NORMAL;
        exp_format.conversion_char = 'd';
        exp_format.field_width = 0;
        exp_format.precision = 1;
        exp_field = (u32)(((bytes[1] << 17) & ~0xFE000000) | (bytes[0] << 25)) >> 21;
        p = long2str((exp_field - 0x3FF) & ((-exp_field | exp_field) >> 31), buff, exp_format);
        if (format.conversion_char == 'a') {
            *--p = 'p';
        } else {
            *--p = 'P';
        }

        bit = format.precision * 4 + 11;
        for (i = format.precision; i > 0; i--) {
            if (bit < 64) {
                shift = 7 - (bit & 7);
                nibble = bytes[bit >> 3] >> shift;
                if ((bit & ~7) != ((bit - 4) & ~7)) {
                    nibble |= (bytes[(bit >> 3) - 1] << 8) >> shift;
                }
                nibble &= 0xF;
                if (nibble < 10) {
                    digit = '0' + nibble;
                } else if (format.conversion_char == 'a') {
                    digit = 'a' - 10 + nibble;
                } else {
                    digit = 'A' - 10 + nibble;
                }
            } else {
                digit = '0';
            }
            *--p = digit;
            bit -= 4;
        }

        if (format.precision != 0 || format.alternate_form) {
            *--p = radix;
        }
        if (__fabs(num) != 0.0) {
            *--p = '1';
        } else {
            *--p = '0';
        }
        if (format.conversion_char == 'a') {
            *--p = 'x';
        } else {
            *--p = 'X';
        }
        *--p = '0';

        if (dec.sign) {
            *--p = '-';
        } else if (format.sign_options == SIGN_ALWAYS) {
            *--p = '+';
        } else if (format.sign_options == SIGN_SPACE) {
            *--p = ' ';
        }
        return p;
    }
}

/* 0x8045C950 (0x128): rounds the digits of `dec` to `new_length`, propagating a carry into the exponent. */
static void round_decimal(decimal* dec, int new_length)
{
    char c;
    char* p;
    char* q;
    int carry;
    int length;

    if (new_length < 0) {
        dec->exp = 0;
        dec->sig.length = 1;
        dec->sig.text[0] = '0';
        return;
    }

    length = dec->sig.length;
    if (new_length >= length) {
        return;
    }

    p = (char*)dec->sig.text + new_length;
    c = dec->sig.text[new_length] - '0';
    if (c == 5) {
        q = (char*)dec->sig.text + length;
        do {
            --q;
        } while (q > p && *q == '0');
        if (q == p) {
            carry = p[-1] & 1;
        } else {
            carry = 1;
        }
    } else {
        carry = c > 5;
    }

    while (new_length != 0) {
        c = *--p + carry - '0';
        carry = c > 9;
        if (carry || c == 0) {
            --new_length;
        } else {
            *p = c + '0';
            break;
        }
    }

    if (carry) {
        dec->sig.length = 1;
        dec->exp++;
        dec->sig.text[0] = '1';
        return;
    }
    if (new_length != 0) {
        dec->sig.length = new_length;
        return;
    }
    dec->exp = 0;
    dec->sig.length = 1;
    dec->sig.text[0] = '0';
}

/* 0x8045CA78 (0x79C): renders a double in `e`, `f` or `g` style (or as INF / NAN), filling backwards from the end of `buff`. */
static char* float2str(f64 num, char* buff, print_format format)
{
    decimal dec;
    decform form;
    char* p;
    char* q;
    int n;
    int exp;
    int sign_char;
    int int_digits;
    int frac_digits;
    int style;
    u8 radix = *_lconv.decimal_point;

    if (format.precision > 509) {
        return NULL;
    }

    form.style = 0;
    form.digits = 32;
    __num2dec(&form, num, &dec);

    q = (char*)&dec.sig.text[dec.sig.length];
    while (dec.sig.length > 1 && *--q == '0') {
        dec.sig.length--;
        dec.exp++;
    }

    switch (dec.sig.text[0]) {
    case 'I':
        if (num < 0.0) {
            p = buff - 5;
            if (ctype_class(format.conversion_char, CTYPE_UPPER)) {
                strcpy(p, "-INF");
            } else {
                strcpy(p, "-inf");
            }
        } else {
            p = buff - 4;
            if (ctype_class(format.conversion_char, CTYPE_UPPER)) {
                strcpy(p, "INF");
            } else {
                strcpy(p, "inf");
            }
        }
        return p;
    case 'N':
        if (dec.sign) {
            p = buff - 5;
            if (ctype_class(format.conversion_char, CTYPE_UPPER)) {
                strcpy(p, "-NAN");
            } else {
                strcpy(p, "-nan");
            }
        } else {
            p = buff - 4;
            if (ctype_class(format.conversion_char, CTYPE_UPPER)) {
                strcpy(p, "NAN");
            } else {
                strcpy(p, "nan");
            }
        }
        return p;
    case '0':
        dec.exp = 0;
        /* fall through */
    default:
        p = buff;
        dec.exp += dec.sig.length - 1;
        *--p = 0;

        switch (format.conversion_char) {
        case 'g':
        case 'G':
            if (dec.sig.length > format.precision) {
                round_decimal(&dec, format.precision);
            }
            if (dec.exp < -4 || dec.exp >= format.precision) {
                if (format.alternate_form) {
                    format.precision--;
                } else {
                    format.precision = dec.sig.length - 1;
                }
                if (format.conversion_char == 'g') {
                    format.conversion_char = 'e';
                } else {
                    format.conversion_char = 'E';
                }
                style = 0;
            } else {
                if (format.alternate_form) {
                    format.precision -= dec.exp + 1;
                } else {
                    format.precision = dec.sig.length - (dec.exp + 1);
                    if (format.precision < 0) {
                        format.precision = 0;
                    }
                }
                style = 1;
            }
            break;
        case 'e':
        case 'E':
            style = 0;
            break;
        case 'f':
        case 'F':
            style = 1;
            break;
        default:
            return p;
        }

        if (style == 0) {
            if (dec.sig.length > format.precision + 1) {
                round_decimal(&dec, format.precision + 1);
            }
            exp = dec.exp;
            sign_char = '+';
            if (exp < 0) {
                exp = -exp;
                sign_char = '-';
            }
            n = 0;
            while (exp != 0 || n < 2) {
                n++;
                *--p = '0' + exp % 10;
                exp /= 10;
            }
            *--p = sign_char;
            *--p = format.conversion_char;

            if (format.precision + (buff - p) > 509) {
                return NULL;
            }
            if (dec.sig.length < format.precision + 1) {
                n = format.precision + 2 - dec.sig.length;
                while (--n != 0) {
                    *--p = '0';
                }
            }
            q = (char*)&dec.sig.text[dec.sig.length];
            n = dec.sig.length;
            while (--n != 0) {
                *--p = *--q;
            }
            if (format.precision != 0 || format.alternate_form) {
                *--p = radix;
            }
            *--p = dec.sig.text[0];
        } else {
            frac_digits = (dec.sig.length - dec.exp) - 1;
            if (frac_digits < 0) {
                frac_digits = 0;
            }
            if (frac_digits > format.precision) {
                round_decimal(&dec, dec.sig.length - (frac_digits - format.precision));
                frac_digits = (dec.sig.length - dec.exp) - 1;
                if (frac_digits < 0) {
                    frac_digits = 0;
                }
            }
            int_digits = dec.exp + 1;
            if (int_digits < 0) {
                int_digits = 0;
            }
            if (int_digits + frac_digits > 509) {
                return NULL;
            }

            n = 0;
            q = (char*)&dec.sig.text[dec.sig.length];
            while (n < format.precision - frac_digits) {
                *--p = '0';
                n++;
            }
            n = 0;
            while (n < frac_digits && n < dec.sig.length) {
                *--p = *--q;
                n++;
            }
            for (; n < frac_digits; n++) {
                *--p = '0';
            }
            if (format.precision != 0 || format.alternate_form) {
                *--p = radix;
            }
            if (int_digits != 0) {
                n = 0;
                while (n < int_digits - dec.sig.length) {
                    *--p = '0';
                    n++;
                }
                for (; n < int_digits; n++) {
                    *--p = *--q;
                }
            } else {
                *--p = '0';
            }
        }

        if (dec.sign) {
            *--p = '-';
        } else if (format.sign_options == SIGN_ALWAYS) {
            *--p = '+';
        } else if (format.sign_options == SIGN_SPACE) {
            *--p = ' ';
        }
        return p;
    }
}

/* 0x8045D214 (0x8AC): formats `format_str` through `write`, fetching the arguments from `args`; returns the character count or -1. */
static int __pformatter(WriteProc write, WriteArg* write_arg, const char* format_str, va_list args, int is_secure)
{
    int num_chars;
    int chars_written;
    int field_width;
    const char* format_ptr;
    const char* curr_format;
    print_format format;
    long long_num;
    long long long_long_num;
    f64 double_num;
    char buff[512];
    char* buff_ptr;
    char fill_char = ' ';
    char pad_char;
    const u16* wcs_ptr;

    format_ptr = format_str;
    chars_written = 0;

    while (*format_ptr != 0) {
        curr_format = strchr(format_ptr, '%');
        if (curr_format == NULL) {
            num_chars = strlen(format_ptr);
            chars_written += num_chars;
            if (num_chars != 0 && write(write_arg, format_ptr, num_chars) == NULL) {
                return -1;
            }
            break;
        }

        num_chars = curr_format - format_ptr;
        chars_written += num_chars;
        if (num_chars != 0 && write(write_arg, format_ptr, num_chars) == NULL) {
            return -1;
        }

        format_ptr = parse_format(curr_format, args, &format);

        switch (format.conversion_char) {
        case 'd':
        case 'i':
            if (format.argument_options == ARG_LONG) {
                long_num = *(long*)__va_arg(args, 1);
            } else if (format.argument_options == ARG_LONG_LONG) {
                long_long_num = *(long long*)__va_arg(args, 2);
            } else if (format.argument_options == ARG_INTMAX) {
                long_long_num = *(long long*)__va_arg(args, 2);
            } else if (format.argument_options == ARG_SIZE_T) {
                long_num = *(long*)__va_arg(args, 1);
            } else if (format.argument_options == ARG_PTRDIFF_T) {
                long_num = *(long*)__va_arg(args, 1);
            } else {
                long_num = *(long*)__va_arg(args, 1);
            }
            if (format.argument_options == ARG_SHORT) {
                long_num = (short)long_num;
            }
            if (format.argument_options == ARG_CHAR) {
                long_num = (char)long_num;
            }
            if (format.argument_options == ARG_LONG_LONG || format.argument_options == ARG_INTMAX) {
                buff_ptr = longlong2str(long_long_num, buff + 512, format);
            } else {
                buff_ptr = long2str(long_num, buff + 512, format);
            }
            if (buff_ptr != NULL) {
                num_chars = (buff + 511) - buff_ptr;
            }
            break;
        case 'o':
        case 'u':
        case 'x':
        case 'X':
            if (format.argument_options == ARG_LONG) {
                long_num = *(long*)__va_arg(args, 1);
            } else if (format.argument_options == ARG_LONG_LONG) {
                long_long_num = *(long long*)__va_arg(args, 2);
            } else if (format.argument_options == ARG_INTMAX) {
                long_long_num = *(long long*)__va_arg(args, 2);
            } else if (format.argument_options == ARG_SIZE_T) {
                long_num = *(long*)__va_arg(args, 1);
            } else if (format.argument_options == ARG_PTRDIFF_T) {
                long_num = *(long*)__va_arg(args, 1);
            } else {
                long_num = *(long*)__va_arg(args, 1);
            }
            if (format.argument_options == ARG_SHORT) {
                long_num = (u16)long_num;
            }
            if (format.argument_options == ARG_CHAR) {
                long_num = (u8)long_num;
            }
            if (format.argument_options == ARG_LONG_LONG || format.argument_options == ARG_INTMAX) {
                buff_ptr = longlong2str(long_long_num, buff + 512, format);
            } else {
                buff_ptr = long2str(long_num, buff + 512, format);
            }
            if (buff_ptr != NULL) {
                num_chars = (buff + 511) - buff_ptr;
            }
            break;
        case 'E':
        case 'F':
        case 'G':
        case 'e':
        case 'f':
        case 'g':
            if (format.argument_options == ARG_LONG_DOUBLE) {
                double_num = *(f64*)__va_arg(args, 3);
            } else {
                double_num = *(f64*)__va_arg(args, 3);
            }
            buff_ptr = float2str(double_num, buff + 512, format);
            if (buff_ptr != NULL) {
                num_chars = (buff + 511) - buff_ptr;
            }
            break;
        case 'A':
        case 'a':
            if (format.argument_options == ARG_LONG_DOUBLE) {
                double_num = *(f64*)__va_arg(args, 3);
            } else {
                double_num = *(f64*)__va_arg(args, 3);
            }
            buff_ptr = double2hex(double_num, buff + 512, format);
            if (buff_ptr != NULL) {
                num_chars = (buff + 511) - buff_ptr;
            }
            break;
        case 's':
            if (format.argument_options == ARG_WIDE) {
                wcs_ptr = *(const u16**)__va_arg(args, 1);
                if (is_secure && wcs_ptr == NULL) {
                    __msl_runtime_constraint_violation_s(NULL, NULL, -1);
                    return -1;
                }
                if (wcs_ptr == NULL) {
                    wcs_ptr = (const u16*)L"";
                }
                if ((int)wcstombs(buff, wcs_ptr, 512) < 0) {
                    buff_ptr = NULL;
                    break;
                }
                buff_ptr = buff;
            } else {
                buff_ptr = *(char**)__va_arg(args, 1);
            }
            if (is_secure && buff_ptr == NULL) {
                __msl_runtime_constraint_violation_s(NULL, NULL, -1);
                return -1;
            }
            if (buff_ptr == NULL) {
                buff_ptr = "";
            }
            if (format.alternate_form) {
                num_chars = (u8)*buff_ptr++;
                if (format.precision_specified && num_chars > format.precision) {
                    num_chars = (u8)format.precision;
                }
            } else if (format.precision_specified) {
                num_chars = (u8)format.precision;
                {
                    char* found = (char*)memchr(buff_ptr, 0, num_chars);
                    if (found != NULL) {
                        num_chars = found - buff_ptr;
                    }
                }
            } else {
                num_chars = strlen(buff_ptr);
            }
            break;
        case 'n': {
            int* n_ptr = *(int**)__va_arg(args, 1);
            if (is_secure) {
                __msl_runtime_constraint_violation_s(NULL, NULL, -1);
                return -1;
            }
            switch (format.argument_options) {
            case ARG_NORMAL:
                *n_ptr = chars_written;
                break;
            case ARG_SHORT:
                *(short*)n_ptr = chars_written;
                break;
            case ARG_LONG:
                *(long*)n_ptr = chars_written;
                break;
            case ARG_INTMAX:
                *(long long*)n_ptr = chars_written;
                break;
            case ARG_SIZE_T:
                *(int*)n_ptr = chars_written;
                break;
            case ARG_PTRDIFF_T:
                *(int*)n_ptr = chars_written;
                break;
            case ARG_LONG_LONG:
                *(long long*)n_ptr = chars_written;
                break;
            }
            continue;
        }
        case 'c':
            buff_ptr = buff;
            num_chars = 1;
            *buff = *(char*)__va_arg(args, 1);
            break;
        case '%':
            *buff = '%';
            buff_ptr = buff;
            num_chars = 1;
            break;
        default:
            buff_ptr = NULL;
            break;
        }

        if (buff_ptr == NULL) {
            num_chars = strlen(curr_format);
            if (num_chars != 0 && write(write_arg, curr_format, num_chars) == NULL) {
                return -1;
            }
            return chars_written + num_chars;
        }

        field_width = num_chars;
        if (format.justification_options != LEFT_JUSTIFY) {
            fill_char = (format.justification_options == ZERO_FILL) ? '0' : ' ';
            if ((*buff_ptr == '+' || *buff_ptr == '-' || *buff_ptr == ' ') && fill_char == '0') {
                if (write(write_arg, buff_ptr, 1) == NULL) {
                    return -1;
                }
                buff_ptr++;
                num_chars--;
            }
            if (format.justification_options == ZERO_FILL && (format.conversion_char == 'a' || format.conversion_char == 'A')) {
                if (num_chars < 2) {
                    return -1;
                }
                if (write(write_arg, buff_ptr, 2) == NULL) {
                    return -1;
                }
                num_chars -= 2;
                buff_ptr += 2;
            }
            while (field_width < format.field_width) {
                if (write(write_arg, &fill_char, 1) == NULL) {
                    return -1;
                }
                field_width++;
            }
        }
        if (num_chars != 0 && write(write_arg, buff_ptr, num_chars) == NULL) {
            return -1;
        }
        if (format.justification_options == LEFT_JUSTIFY) {
            while (field_width < format.field_width) {
                pad_char = ' ';
                if (write(write_arg, &pad_char, 1) == NULL) {
                    return -1;
                }
                field_width++;
            }
        }
        chars_written += field_width;
    }

    return chars_written;
}

/* 0x8045DAC0 (0x58): writes a run of characters to a stream; returns the stream or NULL on a short write. */
static WriteArg* __FileWrite(WriteArg* file, const char* buffer, u32 count)
{
    if (count == __fwrite(buffer, 1, count, (FILE*)file)) {
    } else {
        file = NULL;
    }
    return file;
}

/* 0x8045DB18 (0x6C): copies a run of characters into the string sink, truncating at its capacity. */
static WriteArg* __StringWrite(WriteArg* arg, const char* buffer, u32 count)
{
    OutStrCtrl* osc = (OutStrCtrl*)arg;
    u32 room = osc->max_char_count - osc->chars_written;
    u32 n;

    if (osc->chars_written + count <= osc->max_char_count) {
        n = count;
    } else {
        n = room;
    }
    memcpy(osc->char_str + osc->chars_written, buffer, n);
    osc->chars_written += n;
    return (WriteArg*)1;
}

/* 0x8045DB84 (0xCC): prints formatted output on the console. */
int printf(const char* format, ...)
{
    va_list args;

    if (fwide(&__files[1], -1) >= 0) {
        return -1;
    }
    va_start(args, format);
    return __pformatter((WriteProc)__FileWrite, (WriteArg*)&__files[1], format, args, 0);
}

/* 0x8045DC50 (0x7C): prints formatted output on the console from a variable argument list. */
int vprintf(const char* format, va_list args)
{
    if (fwide(&__files[1], -1) >= 0) {
        return -1;
    }
    return __pformatter((WriteProc)__FileWrite, (WriteArg*)&__files[1], format, args, 0);
}

/* 0x8045DCCC (0x88): formats into a buffer of at most `n` characters from a variable argument list. */
int vsnprintf(char* s, u32 n, const char* format, va_list args)
{
    int ret;
    OutStrCtrl osc;

    osc.char_str = s;
    osc.max_char_count = n;
    osc.chars_written = 0;
    ret = __pformatter(__StringWrite, (WriteArg*)&osc, format, args, 0);
    if (s != NULL) {
        if ((u32)ret < n) {
            s[ret] = 0;
        } else if (n != 0) {
            s[n - 1] = 0;
        }
    }
    return ret;
}

/* 0x8045DD54 (0x84): formats into an unbounded buffer from a variable argument list. */
int vsprintf(char* s, const char* format, va_list args)
{
    return vsnprintf(s, (u32)-1, format, args);
}

/* 0x8045DDD8 (0xF4): formats into a buffer of at most `n` characters. */
int snprintf(char* s, u32 n, const char* format, ...)
{
    va_list args;
    int ret;
    OutStrCtrl osc;

    va_start(args, format);
    osc.char_str = s;
    osc.max_char_count = n;
    osc.chars_written = 0;
    ret = __pformatter(__StringWrite, (WriteArg*)&osc, format, args, 0);
    if (s != NULL) {
        if ((u32)ret < n) {
            s[ret] = 0;
        } else if (n != 0) {
            s[n - 1] = 0;
        }
    }
    return ret;
}

/* 0x8045DECC (0xD4): formats into an unbounded buffer. */
int sprintf(char* s, const char* format, ...)
{
    va_list args;

    va_start(args, format);
    return vsnprintf(s, (u32)-1, format, args);
}
