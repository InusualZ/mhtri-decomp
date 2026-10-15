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
 * RESIDUALS. `.data` is 670 B against the map's 672 B and `.rodata` 1 B against 8 B (padding).
 * SHAPES. wide characters are `u16`; the format record is built in a local and copied out; by-value
 *    `wprint_format` arguments are copied by the caller. The bodies are the narrow printf bodies with MSL's wide
 *    spellings, retail's copy-paste slips included: `wfloat2str` checks `buff - p + precision` against the start of its
 *    narrow buffer, and the narrow `%#s` path reads its length byte through the stale wide `buff_ptr`. MSL's own gotos
 *    shape `wround_decimal` (`goto return_zero`), `wfloat2str` (`goto e_format` / `f_format`) and `__wpformatter`
 *    (`goto conversion_error` into the switch's `default`); `__wpformatter` stores the `%n` count through `buff_ptr`,
 *    and its local declaration order decides its colouring and the `fill_char` / `pad_char` stack slots.
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

/* Returns the absolute value of `x` (MSL's inline `fabs`). */
static inline f64 fabs(f64 x)
{
    return __fabs(x);
}

/* Tests a wide character for the upper-case class of the active locale (0 outside 0..255). */
static inline int wide_isupper(u16 c)
{
    return c >= 0x100 ? 0 : (_current_locale.ctype->wctype_map[c] & CTYPE_UPPER);
}

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
            digit += '0';
        } else {
            digit = (format.conversion_char == 'x') ? digit + 'a' - 10 : digit + 'A' - 10;
        }
        *--p = digit;
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
    int digit;
    u64 unsigned_num;
    u64 base;
    u16* p;
    int n;
    int minus;

    minus = 0;
    n = 0;
    p = buff;
    *--p = 0;
    unsigned_num = num;
    if (num == 0 && format.precision == 0 && !(format.alternate_form && format.conversion_char == 'o')) {
        return p;
    }

    switch (format.conversion_char) {
    case 'd':
    case 'i':
        base = 10;
        if (num < 0) {
            if (num != (long long)0x8000000000000000LL) {
                unsigned_num = -num;
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
        digit = unsigned_num % base;
        unsigned_num = unsigned_num / base;
        if (digit < 10) {
            digit += '0';
        } else {
            digit = (format.conversion_char == 'x') ? digit + 'a' - 10 : digit + 'A' - 10;
        }
        *--p = digit;
        n++;
    } while (unsigned_num != 0);

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
    u16* p;
    u8* q;
    u8 working_byte;
    f64 ld;
    u32 snum;
    long exp;
    wprint_format exp_format;
    int hex_precision;
    int mantissa_bit;
    decform form;
    decimal dec;
    int radix_marker;

    radix_marker = *(u8*)_lconv.decimal_point;
    p = buff;
    ld = num;

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
    }

    exp_format.justification_options = RIGHT_JUSTIFY;
    exp_format.sign_options = SIGN_ALWAYS;
    exp_format.precision_specified = 0;
    exp_format.alternate_form = 0;
    exp_format.argument_options = ARG_NORMAL;
    exp_format.field_width = 0;
    exp_format.precision = 1;
    exp_format.conversion_char = 'd';

    snum = ((u8*)&num)[0] << 25;
    snum |= ((u8*)&num)[1] << 17;
    snum = (snum >> 21) & 0x7FF;
    exp = snum ? snum - 0x3FF : 0;

    p = wlong2str(exp, buff, exp_format);
    if (format.conversion_char == 'a') {
        *--p = 'p';
    } else {
        *--p = 'P';
    }
    q = (u8*)&num;

    mantissa_bit = 11 + format.precision * 4;
    for (hex_precision = format.precision; hex_precision >= 1; hex_precision--) {
        if (mantissa_bit < 64) {
            int mantissa_byte;

            mantissa_byte = mantissa_bit >> 3;
            working_byte = *(q + mantissa_byte) >> (7 - (mantissa_bit & 7));
            if ((mantissa_bit & ~7) != ((mantissa_bit - 4) & ~7)) {
                working_byte |= (*(q + (mantissa_byte - 1)) << 8) >> (7 - (mantissa_bit & 7));
            }
            if ((working_byte &= 0x0F) < 10) {
                working_byte += '0';
            } else if (format.conversion_char == 'a') {
                working_byte += 'a' - 10;
            } else {
                working_byte += 'A' - 10;
            }
        } else {
            working_byte = '0';
        }
        *--p = working_byte;
        mantissa_bit -= 4;
    }

    if (format.precision != 0 || format.alternate_form) {
        *--p = radix_marker;
    }
    if (fabs(ld) != 0.0) {
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

/* 0x804626EC (0x128): rounds the digits of `dec` to `new_length`, propagating a carry into the exponent. */
static void wround_decimal(decimal* dec, int new_length)
{
    char c;
    char* p;
    int carry;

    if (new_length < 0) {
    return_zero:
        dec->exp = 0;
        dec->sig.length = 1;
        dec->sig.text[0] = '0';
        return;
    }

    if (new_length >= dec->sig.length) {
        return;
    }

    p = (char*)dec->sig.text + new_length + 1;
    c = *--p - '0';

    if (c == 5) {
        char* q = (char*)dec->sig.text + dec->sig.length;

        while (--q > p && *q == '0') {
        }
        carry = (q == p) ? p[-1] & 1 : 1;
    } else {
        carry = c > 5;
    }

    while (new_length != 0) {
        c = *--p - '0' + carry;
        if ((carry = c > 9) != 0 || c == 0) {
            --new_length;
        } else {
            *p = c + '0';
            break;
        }
    }

    if (carry != 0) {
        dec->exp += 1;
        dec->sig.length = 1;
        dec->sig.text[0] = '1';
        return;
    } else if (new_length == 0) {
        goto return_zero;
    }

    dec->sig.length = new_length;
}

/* 0x80462814 (0x790): renders a double in `e`, `f` or `g` style (or as INF / NAN) as narrow digits widened into `buff`. */
static u16* wfloat2str(f64 num, u16* wbuff, wprint_format format)
{
    decimal dec;
    decform form;
    char buff[512];
    u16* pw;
    char* p;
    char* q;
    int n;
    int digits;
    int sign;
    int int_digits;
    int frac_digits;
    int radix_marker;

    radix_marker = *(u8*)_lconv.decimal_point;

    if (format.precision > 509) {
        return NULL;
    }

    form.style = 0;
    form.digits = 32;
    __num2dec(&form, num, &dec);

    p = (char*)dec.sig.text + dec.sig.length;
    while (dec.sig.length > 1 && *--p == '0') {
        --dec.sig.length;
        ++dec.exp;
    }

    switch (dec.sig.text[0]) {
    case '0':
        dec.exp = 0;
        break;
    case 'I':
        if (num < 0.0) {
            pw = wbuff - 5;
            if (wide_isupper(format.conversion_char)) {
                wcscpy(pw, L"-INF");
            } else {
                wcscpy(pw, L"-inf");
            }
        } else {
            pw = wbuff - 4;
            if (wide_isupper(format.conversion_char)) {
                wcscpy(pw, L"INF");
            } else {
                wcscpy(pw, L"inf");
            }
        }
        return pw;
    case 'N':
        if (dec.sign) {
            pw = wbuff - 5;
            if (wide_isupper(format.conversion_char)) {
                wcscpy(pw, L"-NAN");
            } else {
                wcscpy(pw, L"-nan");
            }
        } else {
            pw = wbuff - 4;
            if (wide_isupper(format.conversion_char)) {
                wcscpy(pw, L"NAN");
            } else {
                wcscpy(pw, L"nan");
            }
        }
        return pw;
    }

    dec.exp += dec.sig.length - 1;
    p = buff + 512;
    *--p = 0;

    switch (format.conversion_char) {
    case 'g':
    case 'G':
        if (dec.sig.length > format.precision) {
            wround_decimal(&dec, format.precision);
        }
        if (dec.exp < -4 || dec.exp >= format.precision) {
            if (format.alternate_form) {
                --format.precision;
            } else {
                format.precision = dec.sig.length - 1;
            }
            if (format.conversion_char == 'g') {
                format.conversion_char = 'e';
            } else {
                format.conversion_char = 'E';
            }
            goto e_format;
        }
        if (format.alternate_form) {
            format.precision -= dec.exp + 1;
        } else {
            if ((format.precision = dec.sig.length - (dec.exp + 1)) < 0) {
                format.precision = 0;
            }
        }
        goto f_format;
    case 'e':
    case 'E':
    e_format:
        if (dec.sig.length > format.precision + 1) {
            wround_decimal(&dec, format.precision + 1);
        }
        n = dec.exp;
        sign = '+';
        if (n < 0) {
            n = -n;
            sign = '-';
        }
        for (digits = 0; n || digits < 2; ++digits) {
            *--p = n % 10 + '0';
            n /= 10;
        }
        *--p = sign;
        *--p = format.conversion_char;
        if (buff - p + format.precision > 509) {
            return NULL;
        }
        if (dec.sig.length < format.precision + 1) {
            n = format.precision + 2 - dec.sig.length;
            while (--n != 0) {
                *--p = '0';
            }
        }
        q = (char*)dec.sig.text + dec.sig.length;
        n = dec.sig.length;
        while (--n != 0) {
            *--p = *--q;
        }
        if (format.precision || format.alternate_form) {
            *--p = radix_marker;
        }
        *--p = dec.sig.text[0];
        if (dec.sign) {
            *--p = '-';
        } else if (format.sign_options == SIGN_ALWAYS) {
            *--p = '+';
        } else if (format.sign_options == SIGN_SPACE) {
            *--p = ' ';
        }
        break;
    case 'f':
    case 'F':
    f_format:
        if ((frac_digits = -dec.exp + dec.sig.length - 1) < 0) {
            frac_digits = 0;
        }
        if (frac_digits > format.precision) {
            wround_decimal(&dec, dec.sig.length - (frac_digits - format.precision));
            if ((frac_digits = -dec.exp + dec.sig.length - 1) < 0) {
                frac_digits = 0;
            }
        }
        if ((int_digits = dec.exp + 1) < 0) {
            int_digits = 0;
        }
        if (int_digits + frac_digits > 509) {
            return NULL;
        }
        q = (char*)dec.sig.text + dec.sig.length;
        for (digits = 0; digits < (format.precision - frac_digits); ++digits) {
            *--p = '0';
        }
        for (digits = 0; digits < frac_digits && digits < dec.sig.length; ++digits) {
            *--p = *--q;
        }
        for (; digits < frac_digits; ++digits) {
            *--p = '0';
        }
        if (format.precision || format.alternate_form) {
            *--p = radix_marker;
        }
        if (int_digits) {
            for (digits = 0; digits < int_digits - dec.sig.length; ++digits) {
                *--p = '0';
            }
            for (; digits < int_digits; ++digits) {
                *--p = *--q;
            }
        } else {
            *--p = '0';
        }
        if (dec.sign) {
            *--p = '-';
        } else if (format.sign_options == SIGN_ALWAYS) {
            *--p = '+';
        } else if (format.sign_options == SIGN_SPACE) {
            *--p = ' ';
        }
        break;
    }

    pw = wbuff - strlen(p) - 1;
    mbstowcs(pw, p, strlen(p));
    return pw;
}

/* 0x80462FA4 (0x98C): formats `format_str` through `write`, fetching the arguments from `args`; returns the character count or -1. */
static int __wpformatter(WWriteProc write, WWriteArg* write_arg, const u16* format_str, va_list args, int is_secure)
{
    int num_chars;
    u16 fill_char = ' ';
    int chars_written;
    wprint_format format;
    const u16* format_ptr;
    const u16* curr_format;
    f64 double_num;
    long long_num;
    long long long_long_num;
    char c_buff;
    int field_width;
    u16 buff[512];
    u16 pad_char;
    int narrow_len;
    u16* buff_ptr;
    const char* str_ptr;

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
                long_num = *(u32*)__va_arg(args, 1);
            } else if (format.argument_options == ARG_PTRDIFF_T) {
                long_num = *(long*)__va_arg(args, 1);
            } else {
                long_num = *(int*)__va_arg(args, 1);
            }
            if (format.argument_options == ARG_SHORT) {
                long_num = (short)long_num;
            }
            if (format.argument_options == ARG_LONG_LONG || format.argument_options == ARG_INTMAX) {
                if ((buff_ptr = wlonglong2str(long_long_num, buff + 512, format)) == NULL) {
                    goto conversion_error;
                }
            } else {
                if ((buff_ptr = wlong2str(long_num, buff + 512, format)) == NULL) {
                    goto conversion_error;
                }
            }
            num_chars = (buff + 511) - buff_ptr;
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
                long_num = *(u32*)__va_arg(args, 1);
            } else if (format.argument_options == ARG_PTRDIFF_T) {
                long_num = *(long*)__va_arg(args, 1);
            } else {
                long_num = *(int*)__va_arg(args, 1);
            }
            if (format.argument_options == ARG_SHORT) {
                long_num = (u16)long_num;
            }
            if (format.argument_options == ARG_LONG_LONG || format.argument_options == ARG_INTMAX) {
                if ((buff_ptr = wlonglong2str(long_long_num, buff + 512, format)) == NULL) {
                    goto conversion_error;
                }
            } else {
                if ((buff_ptr = wlong2str(long_num, buff + 512, format)) == NULL) {
                    goto conversion_error;
                }
            }
            num_chars = (buff + 511) - buff_ptr;
            break;
        case 'E':
        case 'F':
        case 'G':
        case 'e':
        case 'f':
        case 'g':
            if (format.argument_options == ARG_LONG_DOUBLE) {
                double_num = *(long double*)__va_arg(args, 3);
            } else {
                double_num = *(f64*)__va_arg(args, 3);
            }
            buff_ptr = wfloat2str(double_num, buff + 512, format);
            if (buff_ptr == NULL) {
                goto conversion_error;
            }
            num_chars = (buff + 511) - buff_ptr;
            break;
        case 'A':
        case 'a':
            if (format.argument_options == ARG_LONG_DOUBLE) {
                double_num = *(long double*)__va_arg(args, 3);
            } else {
                double_num = *(f64*)__va_arg(args, 3);
            }
            buff_ptr = wdouble2hex(double_num, buff + 512, format);
            if (buff_ptr == NULL) {
                goto conversion_error;
            }
            num_chars = (buff + 511) - buff_ptr;
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
                narrow_len = (u8)*buff_ptr++;
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
            if ((num_chars = mbstowcs(buff, str_ptr, narrow_len)) < 0) {
                goto conversion_error;
            }
            buff_ptr = buff;
            break;
        case 'n':
            if (is_secure) {
                __msl_runtime_constraint_violation_s(NULL, NULL, -1);
                return -1;
            }
            buff_ptr = *(u16**)__va_arg(args, 1);
            switch (format.argument_options) {
            case ARG_NORMAL:
                *(int*)buff_ptr = chars_written;
                break;
            case ARG_SHORT:
                *(short*)buff_ptr = chars_written;
                break;
            case ARG_LONG:
                *(long*)buff_ptr = chars_written;
                break;
            case ARG_INTMAX:
                *(long long*)buff_ptr = chars_written;
                break;
            case ARG_SIZE_T:
                *(int*)buff_ptr = chars_written;
                break;
            case ARG_PTRDIFF_T:
                *(int*)buff_ptr = chars_written;
                break;
            case ARG_LONG_LONG:
                *(long long*)buff_ptr = chars_written;
                break;
            }
            continue;
        case 'c':
            buff_ptr = buff;
            if (format.argument_options == ARG_WIDE) {
                *buff = *(int*)__va_arg(args, 1);
                num_chars = 1;
            } else {
                c_buff = *(int*)__va_arg(args, 1);
                num_chars = mbtowc(buff, &c_buff, 1);
            }
            break;
        case '%':
            *buff = '%';
            buff_ptr = buff;
            num_chars = 1;
            break;
        default:
        conversion_error:
            num_chars = wcslen(curr_format);
            chars_written += num_chars;
            if (num_chars != 0 && write(write_arg, curr_format, num_chars) == NULL) {
                return -1;
            }
            return chars_written;
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
