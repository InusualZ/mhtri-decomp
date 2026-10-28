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
 * RESIDUALS. .text 4896 B against 4900 B with the same instruction stream except four sites: (1) the callee-saved
 *    colouring - the named locals take retail's registers in retail's order (hex_state r31 .. n_mant r22), but the
 *    parameters `overflow`/`read_arg`/`read` and the `_current_locale` address stall in simplify and are coloured first
 *    (r31..r28) where retail colours them after the locals (r19/r18/r17, locale r15), which renames every register in
 *    the function; the dump (`tools/mwcc-debugger`, pass 1) shows them pushed in the scan after the locals with degree
 *    31-33 against K=29, and `regsim verify` diverges on this function's spill picks, so no source order was found;
 *    (2) both hexadecimal digit cases compare the nibble count as `li r0,14; cmplw` in retail and `cmplwi` here (a
 *    local constant, `14 > n`, `(int)` and `sizeof` spellings fold the same); (3) `SCAN_SIG_START` keeps the upper-cased
 *    character in r3 in retail and in r0 here (one `mr`); (4) the mantissa repack loop takes the update form on the
 *    second byte (`lbzu`) in retail and on the store (`stbu`) here. `.rodata` is 9 B against the map's 16 B (padding).
 *    The hexadecimal exponent's overflow test reads the decimal `exp`, as retail does.
 * SHAPES. a sixteen-state decimal scanner driven by a switch on a bit-flag state word, with the hexadecimal digits in a
 *    second switch on a second state word; the reader callback is `read(arg, 0, 0)` / `read(arg, c, 1)`. The
 *    declaration order is MSL's (`state`, `hex_state`, `count`, `spaces`, `c`, `d`, `negative`, ...), which is what
 *    puts the named locals in retail's registers; the digit stores are `text[length++]`, the NaN tag loop is a `while`
 *    with the class tests in its condition, the success test is one conditional expression, `sh` is initialised with
 *    `bit + 12` before the mask and reduced after it, and the result pointer is set before the normalising shift.
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
    int hex_state = 0;
    int count = 1;
    int spaces = 0;
    int c;
    decimal d = { 0 };
    int negative = 0;
    int exp_negative = 0;
    int exp = 0;
    int exp_adj = 0;
    int sign_detected = 0;
    int radix = *(u8*)_lconv.decimal_point;
    u8* mant_ptr;
    u8 mantissa[8];
    int bin_exp = 0;
    int nibble;
    int bin_exp_negative = 0;
    u32 n_mant = 0;

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
            int matched = 1;
            int tag_len = 0;
            char nan_string[5] = "NAN(";
            char n_buffer[32] = { 0 };
            const char* p = &nan_string[1];
            f64 nan_value;

            while (matched < 4 && *p == TO_UPPER(c)) {
                p++;
                matched++;
                count++;
                c = read(read_arg, 0, 0);
            }
            if (matched == 3 || matched == 4) {
                if (matched == 4) {
                    char* tag = n_buffer;

                    while (tag_len < 32 && (ctype_class(c, CTYPE_DIGIT) || ctype_class(c, CTYPE_ALPHA) || c == radix)) {
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
                    d.sig.text[d.sig.length++] = c;
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
                        d.sig.text[d.sig.length++] = c;
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
                    int digit;
                    u8 uch1;

                    n_mant++;
                    c = TO_UPPER(c);
                    if (c >= 'A') {
                        digit = c - ('A' - 10);
                    } else {
                        digit = c - '0';
                    }
                    uch1 = digit;
                    if (nibble % 2) {
                        old |= uch1;
                    } else {
                        old |= (u8)(uch1 << 4);
                    }
                    mant_ptr[nibble++ / 2] = old;
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
                    int digit;
                    u8 uch1;

                    c = TO_UPPER(c);
                    if (c >= 'A') {
                        digit = c - ('A' - 10);
                    } else {
                        digit = c - '0';
                    }
                    uch1 = digit;
                    if (nibble % 2) {
                        old |= uch1;
                    } else {
                        old |= (u8)(uch1 << 4);
                    }
                    mant_ptr[nibble++ / 2] = old;
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

    if (state != SCAN_HEX ? !(state & SCAN_SUCCESS) : !(count - 1 > 2 && (hex_state & HEX_SUCCESS))) {
        *chars_scanned = 0;
    } else {
        *chars_scanned = count + spaces - 1;
    }
    read(read_arg, c, 1);

    if (hex_state == 0) {
        f64 value;
        int len;
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
            d.sig.text[d.sig.length++] = '0';
        }
        if ((u32)(exp + 0x134) > 0x268) {
            if (*overflow == 0 && d.sig.text[0] == '0' && d.sig.text[1] == 0) {
                return 0.0;
            }
            *overflow = 1;
        }
        exp += exp_adj;
        if (*overflow != 0) {
            if (exp_negative) {
                return 0.0;
            }
            if (negative) {
                return -*(f64*)__double_huge;
            }
            return *(f64*)__double_huge;
        }
        d.exp = exp;
        value = __dec2num(&d);
        if (value != 0.0 && value < 2.2250738585072014e-308) {
            *overflow = 1;
        } else if (value > 1.7976931348623157e308) {
            *overflow = 1;
            value = *(f64*)__double_huge;
        }
        if (negative && (state & SCAN_SUCCESS)) {
            value = -value;
        }
        return value;
    } else {
        u8 result[8];
        int shift;
        u32 i;
        u32 bit;
        u8* src;
        u8* dst;
        u8 carry;

        dst = result;
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
        bin_exp--;
        if (shift != 0) {
            carry = 0;
            for (mant_ptr = &mantissa[7]; mant_ptr >= mantissa; mant_ptr--) {
                u8 t = *mant_ptr;

                *mant_ptr = carry | (t << shift);
                carry = t >> (8 - shift);
            }
        }
        memset(dst, 0, 8);
        src = mantissa;
        dst = &result[1];
        bit = 0;
        for (i = 0; i < 7; i++) {
            u8 v = *src;
            u32 sh = bit + 12;

            if (bit + 8 > 0x34) {
                v &= (u8)(0xFF << (0x34 - bit));
            }
            sh %= 8;
            src++;
            *dst |= v >> sh;
            *++dst |= v << (8 - sh);
            bit += 8;
        }
        bin_exp += exp + 0x3FF;
        if (bin_exp & 0xFFFFF800) {
            *overflow = 1;
            return 0.0;
        }
        bin_exp <<= 21;
        result[0] |= (u32)bin_exp >> 25;
        result[1] |= (u32)bin_exp >> 17;
        if (negative) {
            result[0] |= 0x80;
        }
        return *(f64*)result;
    }
}
