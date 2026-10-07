/*
 * MSL_C/strtold.c - the string-to-floating-point routine (`__strtold`: digit collection, the `INFINITY` / `NAN(`
 *    forms, the hexadecimal form, the scale and round steps).
 *
 * RANGE. .text 0x8045F9E8..0x80460D0C (1 functions in the map, 0x1324 B); .rodata 0x80572B78..0x80572B88; .sdata2
 *    0x8079CA08..0x8079CA28.
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi`.
 * NAMES. file name GUESS (MSL `strtold`); the scan-state names are GUESSes from the state each one handles; `nan` names
 *    the bare-return row 0x804681C4 (owned by `MSL/w_math.cpp`).
 * EVIDENCE. `.sdata2` 0x8079CA08..0x8079CA28 pools `NAN(`, `0.0`, `DBL_MIN`, `DBL_MAX` and the string `INFINITY` is
 *    `.rodata` 0x80572B78; the value `0.0` repeats at 0x8079C9F8 and 0x8079CA28, so it is a TU of its own.
 * RESIDUALS. the one row has a body (.text 4924 B against 4900 B): the callee-saved assignment differs (the target keeps
 *    the sign flag in r14 and the locale table in r15, ours spills the sign flag), so most rows differ in register names
 *    only; the INFINITY / NAN( locals are initialised from the pool in the prologue and copied again in their cases in the
 *    target, ours initialises them once at the case; `.rodata` is 9 B against the map's 16 B (padding).
 * SHAPES. a sixteen-state decimal scanner driven by a switch on a bit-flag state word, with the hexadecimal digits in a
 *    second switch on a second state word; the reader callback is `read(arg, 0, 0)` / `read(arg, c, 1)`.
 */
#include "MSL_C/ansi_fp.h"
#include "MSL_C/ctype.h"
#include "MSL_C/float.h"
#include "MSL_C/strtold.h"
#include "MSL/w_math.h"
#include "Runtime.PPCEABI.H/memset.h"

enum ScanState {
    SCAN_START = 0x0001,
    SCAN_SIG_START = 0x0002,
    SCAN_LEADING_SIG_ZEROS = 0x0004,
    SCAN_INT_DIGIT_LOOP = 0x0008,
    SCAN_FRAC_DIGIT_START = 0x0010,
    SCAN_FRAC_DIGIT_LOOP = 0x0020,
    SCAN_EXP_START = 0x0040,
    SCAN_EXP_SIGN = 0x0080,
    SCAN_EXP_DIGIT_START = 0x0100,
    SCAN_LEADING_EXP_ZEROS = 0x0200,
    SCAN_EXP_DIGIT_LOOP = 0x0400,
    SCAN_FINISHED = 0x0800,
    SCAN_FAILURE = 0x1000,
    SCAN_NAN = 0x2000,
    SCAN_INFINITY = 0x4000,
    SCAN_HEX = 0x8000
};

enum HexState {
    HEX_START = 0x01,
    HEX_LEADING_ZEROS = 0x02,
    HEX_INT_DIGITS = 0x04,
    HEX_FRAC_DIGITS = 0x08,
    HEX_EXP_START = 0x10,
    HEX_EXP_SIGN = 0x20,
    HEX_EXP_DIGIT_START = 0x40,
    HEX_LEADING_EXP_ZEROS = 0x80,
    HEX_EXP_DIGITS = 0x100
};

#define SCAN_FINAL (SCAN_FINISHED | SCAN_FAILURE)
#define SCAN_SUCCESS (SCAN_LEADING_SIG_ZEROS | SCAN_INT_DIGIT_LOOP | SCAN_FRAC_DIGIT_LOOP | SCAN_LEADING_EXP_ZEROS | SCAN_EXP_DIGIT_LOOP | SCAN_FINISHED)
#define HEX_SUCCESS (HEX_LEADING_ZEROS | HEX_INT_DIGITS | HEX_FRAC_DIGITS | HEX_LEADING_EXP_ZEROS | HEX_EXP_DIGITS)

#define CTYPE_XDIGIT 0x0400
#define MAX_DIGITS 20

#define TO_UPPER(c) ctype_toupper(c)

/* 0x8045F9E8 (0x1324): scans a floating-point number through `read` and returns its value. */
f64 __strtold(int max_width, ReadProc read, ReadArg* read_arg, int* chars_scanned, int* overflow)
{
    int state = SCAN_START;
    int count = 1;
    int spaces = 0;
    int hex_state = 0;
    int negative = 0;
    int exp = 0;
    int exp_adj = 0;
    int bin_exp = 0;
    int exp_negative = 0;
    int bin_exp_negative = 0;
    int sign_detected = 0;
    int n_mant = 0;
    int nibble = 0;
    decimal d = { 0 };
    u8 mantissa[8];
    u8 result[8];
    u8* mant_ptr;
    u8 radix = *_lconv.decimal_point;
    int c;

    *overflow = 0;
    c = read(read_arg, 0, 0);

    while (count <= max_width && c != -1 && !(state & SCAN_FINAL)) {
        switch (state) {
        case SCAN_START:
            if (ctype_class(c, CTYPE_SPACE)) {
                c = read(read_arg, 0, 0);
                spaces++;
            } else {
                switch (TO_UPPER(c)) {
                case '-':
                    negative = 1;
                    /* fall through */
                case '+':
                    count++;
                    c = read(read_arg, 0, 0);
                    sign_detected = 1;
                    break;
                case 'I':
                    count++;
                    c = read(read_arg, 0, 0);
                    state = SCAN_INFINITY;
                    break;
                case 'N':
                    count++;
                    c = read(read_arg, 0, 0);
                    state = SCAN_NAN;
                    break;
                default:
                    state = SCAN_SIG_START;
                    break;
                }
            }
            break;
        case SCAN_INFINITY: {
            char infinity_string[9] = "INFINITY";
            int matched = 1;
            const char* p = &infinity_string[1];

            while (matched < 8 && *p == TO_UPPER(c)) {
                p++;
                matched++;
                count++;
                c = read(read_arg, 0, 0);
            }
            if (matched == 3 || matched == 8) {
                f32 inf;

                if (negative) {
                    inf = -*(f32*)__float_huge;
                } else {
                    inf = *(f32*)__float_huge;
                }
                *chars_scanned = matched + (sign_detected + spaces);
                return inf;
            }
            state = SCAN_FAILURE;
            break;
        }
        case SCAN_NAN: {
            char nan_string[5] = "NAN(";
            char n_buffer[32] = { 0 };
            int tag_len = 0;
            int matched = 1;
            const char* p = &nan_string[1];
            char* tag = n_buffer;
            f64 nan_value;

            while (matched < 4 && *p == TO_UPPER(c)) {
                p++;
                matched++;
                count++;
                c = read(read_arg, 0, 0);
            }
            if (matched == 3 || matched == 4) {
                if (matched == 4) {
                    while (tag_len < 32) {
                        if (!ctype_class(c, CTYPE_DIGIT) && !ctype_class(c, CTYPE_ALPHA) && c != radix) {
                            break;
                        }
                        *tag++ = c;
                        tag_len++;
                        count++;
                        c = read(read_arg, 0, 0);
                    }
                    if (c != ')') {
                        state = SCAN_FAILURE;
                        break;
                    }
                    tag_len++;
                }
                n_buffer[tag_len] = 0;
                if (negative) {
                    nan_value = -nan(n_buffer);
                } else {
                    nan_value = nan(n_buffer);
                }
                *chars_scanned = sign_detected + matched + (tag_len + spaces);
                return nan_value;
            }
            state = SCAN_FAILURE;
            break;
        }
        case SCAN_SIG_START:
            if (c == radix) {
                state = SCAN_FRAC_DIGIT_START;
                count++;
                c = read(read_arg, 0, 0);
            } else if (!ctype_class(c, CTYPE_DIGIT)) {
                state = SCAN_FAILURE;
            } else if (c == '0') {
                count++;
                c = read(read_arg, 0, 0);
                if (TO_UPPER(c) == 'X') {
                    hex_state = HEX_START;
                    state = SCAN_HEX;
                } else {
                    state = SCAN_LEADING_SIG_ZEROS;
                }
            } else {
                state = SCAN_INT_DIGIT_LOOP;
            }
            break;
        case SCAN_LEADING_SIG_ZEROS:
            if (c == '0') {
                count++;
                c = read(read_arg, 0, 0);
            } else {
                state = SCAN_INT_DIGIT_LOOP;
            }
            break;
        case SCAN_INT_DIGIT_LOOP:
            if (!ctype_class(c, CTYPE_DIGIT)) {
                if (c == radix) {
                    state = SCAN_FRAC_DIGIT_LOOP;
                    count++;
                    c = read(read_arg, 0, 0);
                } else {
                    state = SCAN_EXP_START;
                }
            } else {
                if (d.sig.length < MAX_DIGITS) {
                    d.sig.text[d.sig.length] = c;
                    d.sig.length++;
                } else {
                    exp_adj++;
                }
                count++;
                c = read(read_arg, 0, 0);
            }
            break;
        case SCAN_FRAC_DIGIT_START:
            if (!ctype_class(c, CTYPE_DIGIT)) {
                state = SCAN_FAILURE;
            } else {
                state = SCAN_FRAC_DIGIT_LOOP;
            }
            break;
        case SCAN_FRAC_DIGIT_LOOP:
            if (!ctype_class(c, CTYPE_DIGIT)) {
                state = SCAN_EXP_START;
            } else {
                if (d.sig.length < MAX_DIGITS) {
                    if (c != '0' || d.sig.length != 0) {
                        d.sig.text[d.sig.length] = c;
                        d.sig.length++;
                    }
                    exp_adj--;
                }
                count++;
                c = read(read_arg, 0, 0);
            }
            break;
        case SCAN_EXP_START:
            if (TO_UPPER(c) == 'E') {
                state = SCAN_EXP_SIGN;
                count++;
                c = read(read_arg, 0, 0);
            } else {
                state = SCAN_FINISHED;
            }
            break;
        case SCAN_EXP_SIGN:
            if (c == '+') {
                count++;
                c = read(read_arg, 0, 0);
            } else if (c == '-') {
                count++;
                c = read(read_arg, 0, 0);
                exp_negative = 1;
            }
            state = SCAN_EXP_DIGIT_START;
            break;
        case SCAN_EXP_DIGIT_START:
            if (!ctype_class(c, CTYPE_DIGIT)) {
                state = SCAN_FAILURE;
            } else if (c == '0') {
                state = SCAN_LEADING_EXP_ZEROS;
                count++;
                c = read(read_arg, 0, 0);
            } else {
                state = SCAN_EXP_DIGIT_LOOP;
            }
            break;
        case SCAN_LEADING_EXP_ZEROS:
            if (c == '0') {
                count++;
                c = read(read_arg, 0, 0);
            } else {
                state = SCAN_EXP_DIGIT_LOOP;
            }
            break;
        case SCAN_EXP_DIGIT_LOOP:
            if (!ctype_class(c, CTYPE_DIGIT)) {
                state = SCAN_FINISHED;
            } else {
                exp = c + exp * 10 - '0';
                if (exp > 0x134) {
                    *overflow = 1;
                }
                count++;
                c = read(read_arg, 0, 0);
            }
            break;
        case SCAN_HEX:
            switch (hex_state) {
            case HEX_START:
                memset(mantissa, 0, 8);
                mant_ptr = mantissa;
                n_mant = 0;
                nibble = 0;
                hex_state = HEX_LEADING_ZEROS;
                count++;
                c = read(read_arg, 0, 0);
                break;
            case HEX_LEADING_ZEROS:
                if (c == '0') {
                    count++;
                    c = read(read_arg, 0, 0);
                } else {
                    hex_state = HEX_INT_DIGITS;
                }
                break;
            case HEX_INT_DIGITS:
                if (!ctype_class(c, CTYPE_XDIGIT)) {
                    if (c == radix) {
                        hex_state = HEX_FRAC_DIGITS;
                        count++;
                        c = read(read_arg, 0, 0);
                    } else {
                        hex_state = HEX_EXP_START;
                    }
                } else if (n_mant < 14) {
                    u8 old = mant_ptr[nibble / 2];
                    u8 digit;

                    n_mant++;
                    c = TO_UPPER(c);
                    digit = c - '0';
                    if (c >= 'A') {
                        digit = c - ('A' - 10);
                    }
                    if (nibble & 1) {
                        mant_ptr[nibble / 2] = old | digit;
                    } else {
                        mant_ptr[nibble / 2] = old | ((digit * 16) & 0xF0);
                    }
                    nibble++;
                    count++;
                    c = read(read_arg, 0, 0);
                } else {
                    count++;
                    c = read(read_arg, 0, 0);
                }
                break;
            case HEX_FRAC_DIGITS:
                if (!ctype_class(c, CTYPE_XDIGIT)) {
                    hex_state = HEX_EXP_START;
                } else if (n_mant < 14) {
                    u8 old = mant_ptr[nibble / 2];
                    u8 digit;

                    c = TO_UPPER(c);
                    digit = c - '0';
                    if (c >= 'A') {
                        digit = c - ('A' - 10);
                    }
                    if (nibble & 1) {
                        mant_ptr[nibble / 2] = old | digit;
                    } else {
                        mant_ptr[nibble / 2] = old | ((digit * 16) & 0xF0);
                    }
                    nibble++;
                    count++;
                    c = read(read_arg, 0, 0);
                } else {
                    count++;
                    c = read(read_arg, 0, 0);
                }
                break;
            case HEX_EXP_START:
                if (TO_UPPER(c) == 'P') {
                    hex_state = HEX_EXP_SIGN;
                    count++;
                    c = read(read_arg, 0, 0);
                } else {
                    state = SCAN_FINISHED;
                }
                break;
            case HEX_EXP_SIGN:
                if (c == '-') {
                    bin_exp_negative = 1;
                } else if (c != '+') {
                    read(read_arg, c, 1);
                    count--;
                }
                hex_state = HEX_EXP_DIGIT_START;
                count++;
                c = read(read_arg, 0, 0);
                break;
            case HEX_EXP_DIGIT_START:
                if (!ctype_class(c, CTYPE_DIGIT)) {
                    state = SCAN_FAILURE;
                } else if (c == '0') {
                    hex_state = HEX_LEADING_EXP_ZEROS;
                    count++;
                    c = read(read_arg, 0, 0);
                } else {
                    hex_state = HEX_EXP_DIGITS;
                }
                break;
            case HEX_LEADING_EXP_ZEROS:
                if (c == '0') {
                    count++;
                    c = read(read_arg, 0, 0);
                } else {
                    hex_state = HEX_EXP_DIGITS;
                }
                break;
            case HEX_EXP_DIGITS:
                if (!ctype_class(c, CTYPE_DIGIT)) {
                    state = SCAN_FINISHED;
                } else {
                    bin_exp = c + bin_exp * 10 - '0';
                    if (exp > 0x7FFF) {
                        *overflow = 1;
                    }
                    count++;
                    c = read(read_arg, 0, 0);
                }
                break;
            }
            break;
        }
    }

    if (state != SCAN_HEX) {
        if ((state & SCAN_SUCCESS) == 0) {
            *chars_scanned = 0;
        } else {
            *chars_scanned = count + spaces - 1;
        }
    } else if (count - 1 <= 2 || !(hex_state & (HEX_LEADING_ZEROS | HEX_INT_DIGITS | HEX_FRAC_DIGITS | HEX_LEADING_EXP_ZEROS | HEX_EXP_DIGITS))) {
        *chars_scanned = 0;
    } else {
        *chars_scanned = count + spaces - 1;
    }
    read(read_arg, c, 1);

    if (hex_state == 0) {
        f64 value;
        u8 len;
        u8* p;

        if (exp_negative) {
            exp = -exp;
        }
        len = d.sig.length;
        p = d.sig.text + len;
        while (len-- != 0 && *--p == '0') {
            exp_adj++;
        }
        d.sig.length = len + 1;
        if (d.sig.length == 0) {
            d.sig.text[d.sig.length] = '0';
            d.sig.length++;
        }
        if ((u32)(exp + 0x134) > 0x268) {
            if (*overflow == 0 && d.sig.text[0] == '0' && d.sig.text[1] == 0) {
                return 0.0;
            }
            *overflow = 1;
        }
        if (*overflow != 0) {
            if (exp_negative) {
                return 0.0;
            }
            if (negative) {
                return -*(f64*)__double_huge;
            }
            return *(f64*)__double_huge;
        }
        d.exp = exp + exp_adj;
        value = __dec2num(&d);
        if (value != 0.0 && value < 2.2250738585072014e-308) {
            *overflow = 1;
        } else if (value > 1.7976931348623157e308) {
            *overflow = 1;
            value = *(f64*)__double_huge;
        }
        if (negative && (state & SCAN_SUCCESS)) {
            return -value;
        }
        return value;
    } else {
        int shift;
        int i;
        int bit;
        u8* src;
        u8* dst;
        u8* q;
        u8 carry;
        int biased;
        u32 shifted;

        if (bin_exp_negative) {
            bin_exp = -bin_exp;
        }
        bin_exp += n_mant * 4;
        i = 0;
        while (i < 4 && !(mantissa[0] & (0x80 >> i))) {
            i++;
            bin_exp--;
        }
        shift = i + 1;
        if (shift != 0) {
            carry = 0;
            for (q = &mantissa[7]; q >= mantissa; q--) {
                u8 t = *q;

                *q = carry | (t << shift);
                carry = t >> (8 - shift);
            }
        }
        memset(result, 0, 8);
        src = mantissa;
        dst = &result[1];
        bit = 0;
        for (i = 0; i < 7; i++) {
            u8 v = *src;
            int sh;

            if ((u32)(bit + 8) > 0x34) {
                v &= 0xFF << (0x34 - bit);
            }
            sh = (bit + 12) & 7;
            src++;
            *dst |= v >> sh;
            bit += 8;
            {
                u8 next = dst[1];

                dst++;
                *dst = next | (u8)(v << (8 - sh));
            }
        }
        biased = exp + (bin_exp - 1) + 0x3FF;
        if (biased & 0xFFFFF800) {
            *overflow = 1;
            return 0.0;
        }
        shifted = biased << 21;
        result[0] |= shifted >> 25;
        result[1] |= shifted >> 17;
        if (negative) {
            result[0] |= 0x80;
        }
        return *(f64*)result;
    }
}
