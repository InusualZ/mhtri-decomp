/*
 * MSL_C/scanf.c - the formatted input engine: the format-string scanner, the string reader `__StringRead`, the
 *    formatter `__sformatter` and `sscanf`.
 *
 * RANGE. .text 0x8045DFC8..0x8045F4AC (4 functions in the map, 0x14E4 B); .rodata 0x80572B50..0x80572B78; .data
 *    0x8060F028..0x8060F298.
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi`.
 * NAMES. file name GUESS (MSL `scanf`); `parse_scan_format` (0x8045DFC8), `__sformatter` (0x8045E65C), `__StringRead`
 *    (0x8045F2F4) and `__strtold` (0x8045F9E8, owned by `MSL_C/strtold.c`) are GUESSes taken from the MSL naming
 *    scheme; the `scan_format` field names and the argument-option enum are GUESSes.
 * EVIDENCE. jump tables `.data` 0x8060F028 / 0x8060F108 / 0x8060F128 / 0x8060F148 and the default record `.rodata`
 *    0x80572B50 are read only here; the string reader is also called by `strtol` / `atoi` through a function
 *    pointer.
 * RESIDUALS. `parse_scan_format` 88: the struct copy-out keeps all ten words in registers where the target copies
 *    pairwise, and the float option test is laid out conversion-first in the target; `__sformatter` 89.0: the
 *    callee-saved assignment differs (the target holds `read` in r26 and the format character in r22, ours r29 and
 *    r15), the `d`/`i` and `o`/`u`/`x` base selection is a ternary where the target jumps into shared code after a
 *    `li` per case, and the unget argument is `(char)(u8)c` in the target; `__StringRead` 99.1: cursor and
 *    character swap r4/r5.
 * SHAPES. the format record is built in a local and copied out; the character class tests are the inline table
 *    reads of `MSL_C/ctype.h`.
 */
#include "MSL_C/ctype.h"
#include "MSL_C/float.h"
#include "MSL_C/mbstring.h"
#include "MSL_C/scanf.h"
#include "MSL_C/strtold.h"
#include "MSL_C/strtoul.h"
#include "Runtime.PPCEABI.H/__va_arg.h"

enum ArgumentOption {
    ARG_NORMAL = 0,
    ARG_CHAR = 1,
    ARG_SHORT = 2,
    ARG_LONG = 3,
    ARG_LONG_LONG = 4,
    ARG_SIZE_T = 5,
    ARG_PTRDIFF_T = 6,
    ARG_INTMAX = 7,
    ARG_DOUBLE = 8,
    ARG_LONG_DOUBLE = 9,
    ARG_WIDE = 10
};

typedef struct scan_format {
    /* +0x00 */ u8 suppress_assignment;     /* `*` was given */
    /* +0x01 */ u8 field_width_specified;   /* a width was given */
    /* +0x02 */ u8 argument_options;        /* ArgumentOption of the length modifier */
    /* +0x03 */ u8 conversion_char;         /* the conversion, 0xFF when invalid */
    /* +0x04 */ int field_width;            /* the maximum field width */
    /* +0x08 */ u8 scan_set[32];            /* one bit per character for `%s` / `%[` */
} scan_format; /* size: 0x28 */

static const scan_format default_format = { 0, 0, 0, 0, 0x7FFFFFFF, { 0 } };

/* 0x8045DFC8 (0x694): parses one conversion specification after a `%` into `format`; returns the next format character. */
static const char* parse_scan_format(const char* format_string, scan_format* format)
{
    const char* s = format_string;
    scan_format f = default_format;
    int c;
    int flag;
    int invert;

    c = *++s;
    if (c == '%') {
        f.conversion_char = c;
        *format = f;
        return s + 1;
    }
    if (c == '*') {
        f.suppress_assignment = 1;
        c = *++s;
    }
    if (ctype_class(c, CTYPE_DIGIT)) {
        f.field_width = 0;
        do {
            f.field_width = f.field_width * 10 + c - '0';
            c = *++s;
        } while (ctype_class(c, CTYPE_DIGIT));
        if (f.field_width == 0) {
            f.conversion_char = 0xFF;
            *format = f;
            return s + 1;
        }
        f.field_width_specified = 1;
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
            f.argument_options = ARG_INTMAX;
            c = *++s;
        }
        break;
    case 'L':
        f.argument_options = ARG_LONG_DOUBLE;
        break;
    case 'j':
        f.argument_options = ARG_LONG_LONG;
        break;
    case 'z':
        f.argument_options = ARG_SIZE_T;
        break;
    case 't':
        f.argument_options = ARG_PTRDIFF_T;
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
        }
        break;
    case 'a':
    case 'A':
    case 'e':
    case 'E':
    case 'f':
    case 'F':
    case 'g':
    case 'G':
        if (!(f.argument_options >= 4 && f.argument_options <= 7) && !(f.argument_options >= 1 && f.argument_options <= 2)) {
            if (f.argument_options != ARG_LONG) {
            } else {
                f.argument_options = ARG_DOUBLE;
            }
        } else {
            f.conversion_char = 0xFF;
        }
        break;
    case 'p':
        f.argument_options = ARG_LONG;
        f.conversion_char = 'x';
        break;
    case 'c':
        if (f.argument_options == ARG_LONG) {
            f.argument_options = ARG_WIDE;
        } else if (f.argument_options != ARG_NORMAL) {
            f.conversion_char = 0xFF;
        }
        break;
    case 's':
        if (f.argument_options == ARG_LONG) {
            f.argument_options = ARG_WIDE;
        } else if (f.argument_options != ARG_NORMAL) {
            f.conversion_char = 0xFF;
        }
        {
            int i;
            for (i = 0; i < 32; i++) {
                f.scan_set[i] = 0xFF;
            }
            f.scan_set[1] = 0xC1;
            f.scan_set[4] = 0xFE;
        }
        break;
    case '[':
        if (f.argument_options == ARG_LONG) {
            f.argument_options = ARG_WIDE;
        } else if (f.argument_options != ARG_NORMAL) {
            f.conversion_char = 0xFF;
        }
        c = *++s;
        invert = 0;
        if (c == '^') {
            c = *++s;
            invert = 1;
        }
        if (c == ']') {
            c = *++s;
            f.scan_set[']' >> 3] |= 1 << (']' & 7);
        }
        while (c != 0 && c != ']') {
            f.scan_set[(c >> 3) & 0x1F] |= 1 << (c & 7);
            if (s[1] == '-' && s[2] != 0 && s[2] != ']') {
                int last = s[2];
                while (++c <= last) {
                    f.scan_set[(c >> 3) & 0x1F] |= 1 << (c & 7);
                }
                c = s[3];
                s += 3;
            } else {
                c = *++s;
            }
        }
        if (c == 0) {
            f.conversion_char = 0xFF;
        } else if (invert) {
            int i;
            u8* set = f.scan_set;
            for (i = 0; i < 32; i++) {
                set[i] = ~set[i];
            }
        }
        break;
    default:
        f.conversion_char = 0xFF;
        break;
    }

    *format = f;
    return s + 1;
}

/* 0x8045E65C (0xC98): scans `format_str` against the characters supplied by `read`, storing through the variadic list; returns the item count. */
static int __sformatter(ReadProc read, ReadArg* read_arg, const char* format_str, va_list arg, int is_secure)
{
    scan_format fmt;
    const char* format = format_str;
    int conversions = 0;
    int terminate = 0;
    int items_assigned = 0;
    int chars_read = 0;
    int scanned;
    int negative;
    int overflow;
    int ch;
    int c_buf_ok = 0;
    u32 buf_len = 0;
    char c;
    long value;
    unsigned long u_value;
    long long ll_value = 0;
    unsigned long long u_ll_value;
    char fc;
    char* arg_ptr;
    char* start;
    int base;

    while ((fc = *format) != 0) {
        if (ctype_class(fc, CTYPE_SPACE)) {
            do {
                format++;
            } while (ctype_class(*format, CTYPE_SPACE));
            if (!terminate) {
                for (;;) {
                    c = read(read_arg, 0, 0);
                    if (!ctype_class(c, CTYPE_SPACE)) {
                        break;
                    }
                    chars_read++;
                }
                read(read_arg, c, 1);
            }
            continue;
        }
        if (fc != '%' && !terminate) {
            c = read(read_arg, 0, 0);
            if ((u8)fc != c) {
                read(read_arg, c, 1);
                if (!is_secure) {
                    break;
                }
                terminate = 1;
                format++;
                continue;
            }
            chars_read++;
            format++;
            continue;
        }

        format = parse_scan_format(format, &fmt);
        if (!fmt.suppress_assignment && fmt.conversion_char != '%') {
            arg_ptr = *(char**)__va_arg(arg, 1);
        } else {
            arg_ptr = NULL;
        }
        if (fmt.conversion_char != 'n' && !terminate && read(read_arg, 0, 2)) {
            if (!is_secure) {
                break;
            }
            terminate = 1;
        }

        switch (fmt.conversion_char) {
        case 'd':
        case 'i':
            base = (fmt.conversion_char == 'd') ? 10 : 0;
            if (terminate) {
                ll_value = 0;
                value = 0;
            } else {
                if (fmt.argument_options == ARG_INTMAX || fmt.argument_options == ARG_LONG_LONG) {
                    u_ll_value = __strtoull(base, fmt.field_width, read, read_arg, &scanned, &negative, &overflow);
                } else {
                    u_value = __strtoul(base, fmt.field_width, read, read_arg, &scanned, &negative, &overflow);
                }
                if (scanned == 0) {
                    if (!is_secure) {
                        break;
                    }
                    ll_value = 0;
                    terminate = 1;
                    value = 0;
                } else {
                    chars_read += scanned;
                    if (fmt.argument_options == ARG_INTMAX || fmt.argument_options == ARG_LONG_LONG) {
                        ll_value = negative ? -u_ll_value : u_ll_value;
                    } else {
                        value = negative ? -u_value : u_value;
                    }
                }
            }
            if (arg_ptr) {
                switch (fmt.argument_options) {
                case ARG_NORMAL:
                    *(int*)arg_ptr = value;
                    break;
                case ARG_CHAR:
                    *(char*)arg_ptr = value;
                    break;
                case ARG_SHORT:
                    *(short*)arg_ptr = value;
                    break;
                case ARG_LONG:
                    *(long*)arg_ptr = value;
                    break;
                case ARG_LONG_LONG:
                    *(long long*)arg_ptr = ll_value;
                    break;
                case ARG_SIZE_T:
                    *(int*)arg_ptr = value;
                    break;
                case ARG_PTRDIFF_T:
                    *(int*)arg_ptr = value;
                    break;
                case ARG_INTMAX:
                    *(long long*)arg_ptr = ll_value;
                    break;
                }
                if (!terminate) {
                    items_assigned++;
                }
            }
            conversions++;
            continue;
        case 'o':
        case 'u':
        case 'x':
        case 'X':
            base = (fmt.conversion_char == 'o') ? 8 : (fmt.conversion_char == 'u') ? 10 : 16;
            if (terminate) {
                u_ll_value = 0;
                u_value = 0;
            } else {
                if (fmt.argument_options == ARG_INTMAX || fmt.argument_options == ARG_LONG_LONG) {
                    u_ll_value = __strtoull(base, fmt.field_width, read, read_arg, &scanned, &negative, &overflow);
                } else {
                    u_value = __strtoul(base, fmt.field_width, read, read_arg, &scanned, &negative, &overflow);
                }
                if (scanned == 0) {
                    if (!is_secure) {
                        break;
                    }
                    u_ll_value = 0;
                    terminate = 1;
                    u_value = 0;
                } else {
                    chars_read += scanned;
                    if (negative) {
                        if (fmt.argument_options == ARG_INTMAX) {
                            u_ll_value = -u_ll_value;
                        }
                        if (fmt.argument_options != ARG_INTMAX) {
                            u_value = -u_value;
                        }
                    }
                }
            }
            if (arg_ptr) {
                switch (fmt.argument_options) {
                case ARG_NORMAL:
                    *(int*)arg_ptr = u_value;
                    break;
                case ARG_CHAR:
                    *(char*)arg_ptr = u_value;
                    break;
                case ARG_SHORT:
                    *(short*)arg_ptr = u_value;
                    break;
                case ARG_LONG:
                    *(long*)arg_ptr = u_value;
                    break;
                case ARG_LONG_LONG:
                    *(long long*)arg_ptr = u_ll_value;
                    break;
                case ARG_SIZE_T:
                    *(int*)arg_ptr = u_value;
                    break;
                case ARG_PTRDIFF_T:
                    *(int*)arg_ptr = u_value;
                    break;
                case ARG_INTMAX:
                    *(long long*)arg_ptr = u_ll_value;
                    break;
                }
                if (!terminate) {
                    items_assigned++;
                }
            }
            conversions++;
            continue;
        case 'a':
        case 'A':
        case 'e':
        case 'E':
        case 'f':
        case 'F':
        case 'g':
        case 'G': {
            f64 dvalue;
            if (terminate) {
                dvalue = *(f32*)__float_nan;
            } else {
                dvalue = __strtold(fmt.field_width, read, read_arg, &scanned, &overflow);
                if (scanned == 0) {
                    if (!is_secure) {
                        break;
                    }
                    terminate = 1;
                    dvalue = *(f32*)__float_nan;
                } else {
                    chars_read += scanned;
                }
            }
            if (arg_ptr) {
                switch (fmt.argument_options) {
                case ARG_NORMAL:
                    *(f32*)arg_ptr = dvalue;
                    break;
                case ARG_DOUBLE:
                    *(f64*)arg_ptr = dvalue;
                    break;
                case ARG_LONG_DOUBLE:
                    *(f64*)arg_ptr = dvalue;
                    break;
                }
                if (!terminate) {
                    items_assigned++;
                }
            }
            conversions++;
            continue;
        }
        case 'c':
            if (!fmt.field_width_specified) {
                fmt.field_width = 1;
            }
            if (arg_ptr) {
                if (is_secure) {
                    c_buf_ok = 1;
                    buf_len = *__va_arg(arg, 1);
                }
                scanned = 0;
                if (terminate) {
                    if (buf_len) {
                        *arg_ptr = 0;
                    }
                    continue;
                }
                start = arg_ptr;
                while (fmt.field_width-- != 0 && (!is_secure || (c_buf_ok = buf_len > (u32)scanned))
                       && (ch = read(read_arg, 0, 0)) != -1) {
                    c = ch;
                    if (fmt.argument_options == ARG_WIDE) {
                        mbtowc((u16*)arg_ptr, &c, 1);
                        arg_ptr += 2;
                    } else {
                        *arg_ptr++ = ch;
                    }
                    scanned++;
                }
                c = ch;
                if (scanned == 0 || (is_secure && !c_buf_ok)) {
                    if (!is_secure) {
                        break;
                    }
                    terminate = 1;
                    if (buf_len) {
                        *start = 0;
                    }
                    continue;
                }
                chars_read += scanned;
                items_assigned++;
            } else {
                scanned = 0;
                while (fmt.field_width-- != 0 && (ch = read(read_arg, 0, 0)) != -1) {
                    c = ch;
                    scanned++;
                }
                c = ch;
                if (scanned == 0) {
                    break;
                }
            }
            conversions++;
            continue;
        case '%':
            if (terminate) {
                continue;
            }
            for (;;) {
                c = read(read_arg, 0, 0);
                if (!ctype_class(c, CTYPE_SPACE)) {
                    break;
                }
                chars_read++;
            }
            if ((u8)c != '%') {
                read(read_arg, c, 1);
                if (!is_secure) {
                    break;
                }
                terminate = 1;
                continue;
            }
            chars_read++;
            continue;
        case 's':
            if (!terminate) {
                c = read(read_arg, 0, 0);
                while (ctype_class(c, CTYPE_SPACE)) {
                    chars_read++;
                    c = read(read_arg, 0, 0);
                }
                read(read_arg, c, 1);
            }
        case '[':
            if (arg_ptr) {
                if (is_secure) {
                    c_buf_ok = 1;
                    buf_len = *__va_arg(arg, 1) - 1;
                }
                scanned = 0;
                if (terminate) {
                    if (buf_len) {
                        *arg_ptr = 0;
                    }
                    continue;
                }
                start = arg_ptr;
                while (fmt.field_width-- != 0 && (!is_secure || (c_buf_ok = (int)buf_len > scanned))
                       && (ch = read(read_arg, 0, 0)) != -1 && (c = ch, (1 << (c & 7)) & fmt.scan_set[(c >> 3) & 0x1F])) {
                    if (fmt.argument_options == ARG_WIDE) {
                        mbtowc((u16*)arg_ptr, &c, 1);
                        arg_ptr += 2;
                    } else {
                        *arg_ptr++ = ch;
                    }
                    scanned++;
                }
                c = ch;
                if (scanned == 0 || (is_secure && !c_buf_ok)) {
                    read(read_arg, ch, 1);
                    if (!is_secure) {
                        break;
                    }
                    terminate = 1;
                    if (buf_len) {
                        *start = 0;
                    }
                    continue;
                }
                chars_read += scanned;
                if (fmt.argument_options == ARG_WIDE) {
                    *(u16*)arg_ptr = 0;
                } else {
                    *arg_ptr = 0;
                }
                items_assigned++;
            } else {
                scanned = 0;
                while (fmt.field_width-- != 0 && (ch = read(read_arg, 0, 0)) != -1
                       && (c = ch, (1 << (c & 7)) & fmt.scan_set[(c >> 3) & 0x1F])) {
                    scanned++;
                }
                c = ch;
                if (scanned == 0) {
                    read(read_arg, ch, 1);
                    break;
                }
                chars_read += scanned;
            }
            if (fmt.field_width >= 0) {
                read(read_arg, c, 1);
            }
            conversions++;
            continue;
        case 'n':
            if (arg_ptr) {
                switch (fmt.argument_options) {
                case ARG_NORMAL:
                    *(int*)arg_ptr = chars_read;
                    break;
                case ARG_SHORT:
                    *(short*)arg_ptr = chars_read;
                    break;
                case ARG_LONG:
                    *(long*)arg_ptr = chars_read;
                    break;
                case ARG_CHAR:
                    *(char*)arg_ptr = chars_read;
                    break;
                case ARG_INTMAX:
                    *(long long*)arg_ptr = chars_read;
                    break;
                }
            }
            continue;
        }
        break;
    }

    if (read(read_arg, 0, 2) && conversions == 0) {
        return -1;
    }
    return items_assigned;
}

/* 0x8045F2F4 (0x88): serves the string source of `sscanf` (read, unget, end test). */
int __StringRead(StringReadState* state, int ch, int action)
{
    switch (action) {
    case 0: {
        const char* cursor = state->cursor;
        u8 next = *cursor;
        if ((char)next == 0) {
            state->hit_nul = 1;
            return -1;
        }
        state->cursor = cursor + 1;
        return next;
    }
    case 1:
        if (state->hit_nul == 0) {
            state->cursor--;
        } else {
            state->hit_nul = 0;
        }
        return ch;
    case 2:
        return state->hit_nul;
    }
    return 0;
}

static inline int only_whitespace(const char* p)
{
    while (*p != 0) {
        if (!ctype_class(*p++, CTYPE_SPACE)) {
            return 0;
        }
    }
    return 1;
}

/* 0x8045F37C (0x130): reads formatted input from the string `buffer`. */
int sscanf(const char* buffer, const char* format, ...)
{
    va_list args;
    StringReadState reader;

    va_start(args, format);
    reader.cursor = buffer;
    if (buffer == NULL || *buffer == 0 || only_whitespace(buffer)) {
        return -1;
    }
    reader.hit_nul = 0;
    return __sformatter((ReadProc)__StringRead, (ReadArg*)&reader, format, args, 0);
}
