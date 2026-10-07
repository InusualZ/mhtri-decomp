/*
 * MSL_C/wprintf.c - the wide-character formatted output engine: the wide format scanner, the wide number renderers,
 *    `__wpformatter`, the wide string writer, `swprintf` and `vswprintf`.
 *
 * RANGE. .text 0x80461808..0x80463B20 (10 functions in the map, 0x2318 B); .rodata 0x80573188..0x80573190; .data
 *    0x8060F298..0x8060F538; .sdata2 0x8079CA28..0x8079CA30.
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi`.
 * NAMES. file name GUESS (MSL `wprintf`); wparse_format, wlong2str, wlonglong2str, wdouble2hex, wround_decimal,
 *    wfloat2str, __wpformatter, __wstring_write and swprintf are GUESSes (the narrow printf scheme with a `w` prefix;
 *    map rows renamed from their fn_ stems 80461808, 80461D98, 80461FDC, 80462298, 804626EC, 80462814, 80462FA4,
 *    80463930 and 8046399C). wprint_format is the narrow record with a 16-bit conversion character.
 * EVIDENCE. jump tables `.data` 0x8060F298 and 0x8060F378; the pooled wide strings `-0X0 ... nan` and the empty wide
 *    string are the `.data` record at 0x8060F4C8 (0x70 B); `.rodata` 0x80573188 is the empty narrow string; the
 *    pooled `0.0` at `.sdata2` 0x8079CA28 repeats at 0x8079CA10, so this is a TU of its own. `wfloat2str` builds the
 *    number as narrow digits in a local buffer and widens it with `mbstowcs`.
 * RESIDUALS. all ten rows have bodies; 4 at 100 (wparse_format, __wstring_write, swprintf, vswprintf). __wpformatter 87: the
 *    compiler merges the identical SIZE_T / PTRDIFF_T / default integer fetches where the target keeps one body per option,
 *    -172 B on .text; wfloat2str 89: the target lays the INF / NAN blocks before the digit code and keeps the ctype test as
 *    clrlslwi, ours does not; wdouble2hex 92: the exponent byte merge (rlwimi order) and the callee-saved assignment;
 *    wround_decimal 89.6 and wlong2str 99: as the narrow printf rows. `.data` is 670 B against the map's 672 B and `.rodata` 1 B
 *    against 8 B (padding); the unit cannot flip while .text is short.
 * SHAPES. wide characters are `u16`; the format record is built in a local and copied out; by-value
 *    `wprint_format` arguments are copied by the caller.
 */

#include "MSL_C/ansi_fp.h"
#include "MSL_C/ctype.h"
#include "MSL_C/abort_exit.h"
#include "MSL_C/mbstring.h"
#include "MSL_C/mem.h"
#include "MSL_C/printf_format.h"
#include "MSL_C/wmem.h"
#include "MSL_C/wstring.h"
#include "MSL/strlen.h"
#include "Runtime.PPCEABI.H/__va_arg.h"
#include "stdarg.h"

/* The opaque argument the writers receive. */
typedef struct WWriteArg WWriteArg;
typedef WWriteArg* (*WWriteProc)(WWriteArg*, const u16*, u32);

/* The state of a wide string sink. */
typedef struct OutWStrCtrl {
    /* +0x00 */ u16* wchar_str;         /* destination buffer */
    /* +0x04 */ u32 max_char_count;     /* capacity, including the terminator */
    /* +0x08 */ u32 chars_written;      /* characters stored so far */
} OutWStrCtrl; /* size: 0xC */

/* One parsed conversion specification. */
typedef struct wprint_format {
    /* +0x00 */ u8 justification_options;   /* Justification */
    /* +0x01 */ u8 sign_options;            /* SignOption */
    /* +0x02 */ u8 precision_specified;     /* a `.` precision was given */
    /* +0x03 */ u8 alternate_form;          /* `#` was given */
    /* +0x04 */ u8 argument_options;        /* ArgumentOption of the length modifier */
    /* +0x05 */ u8 pad_0x05;
    /* +0x06 */ u16 conversion_char;        /* the conversion, 0xFFFF when invalid */
    /* +0x08 */ int field_width;
    /* +0x0C */ int precision;
} wprint_format; /* size: 0x10 */

/* 0x80461808 (0x590): parses one conversion specification after a `%` into `format`; returns the next format character. */
static const u16* wparse_format(const u16* format_string, va_list arg, wprint_format* format)
{
    const u16* s = format_string;
    wprint_format f;
    u16 c;
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
        while (ctype_wclass(c, CTYPE_DIGIT)) {
            f.field_width = f.field_width * 10 + c - '0';
            c = *++s;
        }
    }

    if (f.field_width > 509) {
        f.conversion_char = 0xFFFF;
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
            while (ctype_wclass(c, CTYPE_DIGIT)) {
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
            f.argument_options = ARG_LONG_LONG;
        }
        if (!f.precision_specified) {
            f.precision = 1;
        } else if (f.justification_options == ZERO_FILL) {
            f.justification_options = RIGHT_JUSTIFY;
        }
        break;
    case 'f':
    case 'F':
        if ((u8)(f.argument_options + 0xFA) <= 2 || f.argument_options == ARG_SHORT || f.argument_options == ARG_LONG_LONG) {
            f.conversion_char = 0xFFFF;
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
            f.conversion_char = 0xFFFF;
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
            f.conversion_char = 0xFFFF;
        } else if (!f.precision_specified) {
            f.precision = 6;
        }
        break;
    case 'p':
        f.argument_options = ARG_LONG;
        f.alternate_form = 1;
        f.conversion_char = 'x';
        f.precision = 8;
        break;
    case 'c':
        if (f.argument_options == ARG_LONG) {
            f.argument_options = ARG_WIDE;
        } else if (f.precision_specified || f.argument_options != ARG_NORMAL) {
            f.conversion_char = 0xFFFF;
        }
        break;
    case 's':
        if (f.argument_options == ARG_LONG) {
            f.argument_options = ARG_WIDE;
        } else if (f.argument_options != ARG_NORMAL) {
            f.conversion_char = 0xFFFF;
        }
        break;
    case 'n':
        if (f.argument_options == ARG_LONG_DOUBLE) {
            f.argument_options = ARG_LONG_LONG;
        }
        break;
    default:
        f.conversion_char = 0xFFFF;
        break;
    }

    *format = f;
    return s + 1;
}

/* 0x80461D98 (0x244): renders a signed or unsigned long as wide digits, filling backwards from the end of `buff`. */
static u16* wlong2str(long num, u16* buff, wprint_format format)
{
    u32 unsigned_num;
    u32 base;
    int digit;
    int ch;
    u16* p;
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
            if (num != (long)0x80000000) {
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

/* 0x80461FDC (0x2BC): renders a signed or unsigned long long as wide digits, filling backwards from the end of `buff`. */
static u16* wlonglong2str(long long num, u16* buff, wprint_format format)
{
    u64 unsigned_num;
    u64 base;
    int digit;
    int ch;
    u16* p;
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

/* 0x80462298 (0x454): renders a double as a wide hexadecimal floating-point literal (or as INF / NAN), filling backwards from the end of `buff`. */
static u16* wdouble2hex(f64 num, u16* buff, wprint_format format)
{
    decimal dec;
    decform form;
    wprint_format exp_format;
    u16* p;
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
    case '0':
        dec.exp = 0;
        if (dec.sign) {
            p = buff - 5;
            if (format.conversion_char == 'A') {
                wcscpy(p, L"-0X0");
            } else {
                wcscpy(p, L"-0x0");
            }
        } else {
            p = buff - 4;
            if (format.conversion_char == 'A') {
                wcscpy(p, L"0X0");
            } else {
                wcscpy(p, L"0x0");
            }
        }
        return p;
    case 'I':
        if (dec.sign) {
            p = buff - 5;
            if (format.conversion_char == 'A') {
                wcscpy(p, L"-INF");
            } else {
                wcscpy(p, L"-inf");
            }
        } else {
            p = buff - 4;
            if (format.conversion_char == 'A') {
                wcscpy(p, L"INF");
            } else {
                wcscpy(p, L"inf");
            }
        }
        return p;
    case 'N':
        if (dec.sign) {
            p = buff - 5;
            if (format.conversion_char == 'A') {
                wcscpy(p, L"-NAN");
            } else {
                wcscpy(p, L"-nan");
            }
        } else {
            p = buff - 4;
            if (format.conversion_char == 'A') {
                wcscpy(p, L"NAN");
            } else {
                wcscpy(p, L"nan");
            }
        }
        return p;
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
        exp_field = (u32)(((bytes[0] << 25) & 0xFE000000) | (bytes[1] << 17)) >> 21;
        p = wlong2str((exp_field - 0x3FF) & ((-exp_field | exp_field) >> 31), buff, exp_format);
        if (format.conversion_char == 'a') {
            *--p = 'p';
        } else {
            *--p = 'P';
        }

        bit = format.precision * 4 + 11;
        for (i = format.precision; i >= 1; i--) {
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

/* 0x804626EC (0x128): rounds the digits of `dec` to `new_length`, propagating a carry into the exponent. */
static void wround_decimal(decimal* dec, int new_length)
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

/* 0x80462814 (0x790): renders a double in `e`, `f` or `g` style (or as INF / NAN) as narrow digits widened into `buff`. */
static u16* wfloat2str(f64 num, u16* buff, wprint_format format)
{
    decimal dec;
    decform form;
    char digits[512];
    char* p;
    char* q;
    int n;
    int exp;
    int sign_char;
    int int_digits;
    int frac_digits;
    int style;
    u16* wp;
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
            wp = buff - 5;
            if (ctype_wclass(format.conversion_char, CTYPE_UPPER)) {
                wcscpy(wp, L"-INF");
            } else {
                wcscpy(wp, L"-inf");
            }
        } else {
            wp = buff - 4;
            if (ctype_wclass(format.conversion_char, CTYPE_UPPER)) {
                wcscpy(wp, L"INF");
            } else {
                wcscpy(wp, L"inf");
            }
        }
        return wp;
    case 'N':
        if (dec.sign) {
            wp = buff - 5;
            if (ctype_wclass(format.conversion_char, CTYPE_UPPER)) {
                wcscpy(wp, L"-NAN");
            } else {
                wcscpy(wp, L"-nan");
            }
        } else {
            wp = buff - 4;
            if (ctype_wclass(format.conversion_char, CTYPE_UPPER)) {
                wcscpy(wp, L"NAN");
            } else {
                wcscpy(wp, L"nan");
            }
        }
        return wp;
    case '0':
        dec.exp = 0;
        /* fall through */
    default:
        p = digits + 512;
        dec.exp += dec.sig.length - 1;
        *--p = 0;

        switch (format.conversion_char) {
        case 'g':
        case 'G':
            if (dec.sig.length > format.precision) {
                wround_decimal(&dec, format.precision);
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
            style = -1;
            break;
        }

        if (style == 0) {
            if (dec.sig.length > format.precision + 1) {
                wround_decimal(&dec, format.precision + 1);
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

            if (format.precision + ((digits + 512) - p) > 509) {
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
        } else if (style == 1) {
            frac_digits = (dec.sig.length - dec.exp) - 1;
            if (frac_digits < 0) {
                frac_digits = 0;
            }
            if (frac_digits > format.precision) {
                wround_decimal(&dec, dec.sig.length - (frac_digits - format.precision));
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

        if (style >= 0) {
            if (dec.sign) {
                *--p = '-';
            } else if (format.sign_options == SIGN_ALWAYS) {
                *--p = '+';
            } else if (format.sign_options == SIGN_SPACE) {
                *--p = ' ';
            }
        }
        wp = buff - (strlen(p) + 1);
        mbstowcs(wp, p, strlen(p));
        return wp;
    }
}

/* 0x80462FA4 (0x98C): formats `format_str` through `write`, fetching the arguments from `args`; returns the character count or -1. */
static int __wpformatter(WWriteProc write, WWriteArg* write_arg, const u16* format_str, va_list args, int is_secure)
{
    int num_chars;
    int chars_written;
    int field_width;
    const u16* format_ptr;
    const u16* curr_format;
    wprint_format format;
    long long_num;
    long long long_long_num;
    f64 double_num;
    u16 buff[512];
    u16* buff_ptr;
    u16 fill_char = ' ';
    u16 pad_char;
    const char* str_ptr;
    char c_buff;
    u8 narrow_len;

    format_ptr = format_str;
    chars_written = 0;

    while (*format_ptr != 0) {
        curr_format = wcschr(format_ptr, '%');
        if (curr_format == NULL) {
            num_chars = wcslen(format_ptr);
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

        format_ptr = wparse_format(curr_format, args, &format);

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
            if (format.argument_options == ARG_LONG_LONG || format.argument_options == ARG_INTMAX) {
                buff_ptr = wlonglong2str(long_long_num, buff + 512, format);
            } else {
                buff_ptr = wlong2str(long_num, buff + 512, format);
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
            if (format.argument_options == ARG_LONG_LONG || format.argument_options == ARG_INTMAX) {
                buff_ptr = wlonglong2str(long_long_num, buff + 512, format);
            } else {
                buff_ptr = wlong2str(long_num, buff + 512, format);
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
            buff_ptr = wfloat2str(double_num, buff + 512, format);
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
            buff_ptr = wdouble2hex(double_num, buff + 512, format);
            if (buff_ptr != NULL) {
                num_chars = (buff + 511) - buff_ptr;
            }
            break;
        case 's':
            if (format.argument_options == ARG_WIDE) {
                buff_ptr = *(u16**)__va_arg(args, 1);
                if (is_secure && buff_ptr == NULL) {
                    __msl_runtime_constraint_violation_s(NULL, NULL, -1);
                    return -1;
                }
                if (buff_ptr == NULL) {
                    buff_ptr = L"";
                }
                if (format.alternate_form) {
                    num_chars = (u8)*buff_ptr++;
                    if (format.precision_specified && num_chars > format.precision) {
                        num_chars = format.precision;
                    }
                } else if (format.precision_specified) {
                    num_chars = format.precision;
                    {
                        u16* found = wmemchr(buff_ptr, 0, num_chars);
                        if (found != NULL) {
                            num_chars = found - buff_ptr;
                        }
                    }
                } else {
                    num_chars = wcslen(buff_ptr);
                }
                break;
            }
            str_ptr = *(char**)__va_arg(args, 1);
            if (is_secure && str_ptr == NULL) {
                __msl_runtime_constraint_violation_s(NULL, NULL, -1);
                return -1;
            }
            if (str_ptr == NULL) {
                str_ptr = "";
            }
            if (format.alternate_form) {
                narrow_len = *str_ptr++;
                if (format.precision_specified && narrow_len > format.precision) {
                    narrow_len = format.precision;
                }
            } else if (format.precision_specified) {
                narrow_len = format.precision;
                {
                    const char* found = (const char*)memchr(str_ptr, 0, narrow_len);
                    if (found != NULL) {
                        narrow_len = found - str_ptr;
                    }
                }
            } else {
                narrow_len = strlen(str_ptr);
            }
            num_chars = mbstowcs(buff, str_ptr, narrow_len);
            if (num_chars < 0) {
                buff_ptr = NULL;
                break;
            }
            buff_ptr = buff;
            break;
        case 'n': {
            int* n_ptr;

            if (is_secure) {
                __msl_runtime_constraint_violation_s(NULL, NULL, -1);
                return -1;
            }
            n_ptr = *(int**)__va_arg(args, 1);
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
            if (format.argument_options == ARG_WIDE) {
                num_chars = 1;
                *buff = *(u16*)__va_arg(args, 1);
            } else {
                c_buff = *(char*)__va_arg(args, 1);
                num_chars = mbtowc(buff, &c_buff, 1);
            }
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
            num_chars = wcslen(curr_format);
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

/* 0x80463930 (0x6C): copies a run of wide characters into the string sink, truncating at its capacity. */
static void __wstring_write(WWriteArg* arg, const u16* buffer, u32 count)
{
    OutWStrCtrl* osc = (OutWStrCtrl*)arg;
    u32 room = osc->max_char_count - osc->chars_written;
    u32 n;

    if (osc->chars_written + count <= osc->max_char_count) {
        n = count;
    } else {
        n = room;
    }
    wmemcpy(osc->wchar_str + osc->chars_written, buffer, n);
    osc->chars_written += n;
}

/* 0x8046399C (0xF8): formats into a wide buffer of at most `n` characters. */
int swprintf(u16* s, u32 n, const u16* format, ...)
{
    va_list args;
    int ret;
    OutWStrCtrl osc;

    va_start(args, format);
    osc.wchar_str = s;
    osc.max_char_count = n;
    osc.chars_written = 0;
    ret = __wpformatter((WWriteProc)__wstring_write, (WWriteArg*)&osc, format, args, 0);
    if (ret >= 0) {
        if ((u32)ret < n) {
            s[ret] = 0;
        } else {
            ret = -1;
            s[n - 1] = 0;
        }
    }
    return ret;
}

/* 0x80463A94 (0x8C): formats into a wide buffer of at most `n` characters from a variable argument list. */
int vswprintf(u16* s, u32 n, const u16* format, va_list args)
{
    int ret;
    OutWStrCtrl osc;

    osc.wchar_str = s;
    osc.max_char_count = n;
    osc.chars_written = 0;
    ret = __wpformatter((WWriteProc)__wstring_write, (WWriteArg*)&osc, format, args, 0);
    if (ret >= 0) {
        if ((u32)ret < n) {
            s[ret] = 0;
        } else {
            ret = -1;
            s[n - 1] = 0;
        }
    }
    return ret;
}
