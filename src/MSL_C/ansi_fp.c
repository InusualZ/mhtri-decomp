/*
 * MSL_C/ansi_fp.c - decimal/binary floating-point conversion for the formatted I/O: `__ull2dec`, `__timesdec`,
 *    `__str2dec`, `__two_exp`, the decimal compares and subtraction, `__num2dec_internal`, the `__num2dec` /
 *    `__dec2num` entries.
 *
 * RANGE. .text 0x804592B8..0x8045AB38 (10 functions in the map, 0x1880 B); .rodata 0x80572540..0x80572620; .data
 *    0x8060EA98..0x8060EC00; .sdata2 0x8079C9A8..0x8079C9E0.
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi` (the group of the unit it was cut from).
 * NAMES. file name GUESS (MSL `ansi_fp`). __equals_dec / __less_dec / __minus_dec (map rows renamed from the fn_ stems 80459A24, 80459B08 and
 *    80459C08) are GUESS (equality, ordering and subtraction of two decimals). decimal_powers_of_ten is a GUESS
 *    (the 10^1..10^8 table at 0x8060EBC0).
 * EVIDENCE. `.sdata2` 0x8079C9A8..0x8079C9E0 holds `0.0 / 1.0 / -1.0 / 5.0 / DBL_MAX / 2^52 magic`, and the value
 *    `0.0` is pooled again at 0x8079C9F8 (read by the printf unit) so the next pool is another TU; the jump table
 *    `.data` 0x8060EA98 is the `__two_exp` switch, the power table `.rodata` 0x80572540 the powers-of-two digit strings.
 * RESIDUALS. all ten rows have bodies; 4 at 100 (__ull2dec, __two_exp, __equals_dec, __less_dec). __str2dec 89 and __timesdec 98.7 and
 *    __minus_dec 93: the target scans the rounding tail with a counted loop that falls into the parity test (a shared round-up
 *    tail reached by goto in the original); every goto-free spelling duplicates the inlined round-up or adds a compare.
 *    __num2dec_internal 96.9: r30/r31 hold sign and result swapped. __num2dec 92 and __dec2num 94.3: the stack slot of the NaN
 *    temporary and the saved-register count (r20 against r21) differ. .text is +20 B over the claim (the __minus_dec and
 *    __str2dec epilogues), so the unit cannot flip.
 * SHAPES. the digit loops are compiler-unrolled by eight; `decimal` copies are struct assignments.
 */
#include "MSL_C/ansi_fp.h"
#include "MSL_C/misc_io.h"
#include "MSL_C/__fpclassifyd.h"
#include "MSL_C/float.h"
#include "MSL/s_copysign.h"
#include "MSL/s_frexp.h"
#include "MSL/s_ldexp.h"
#include "MSL/w_math.h"

#define MAX_SIG_DIGITS 36

#pragma dont_inline on
/* Writes the decimal digits of an unsigned 64-bit integer. */
void __ull2dec(decimal* result, u64 val)
{
    u8* first;
    u8* last;

    result->sign = 0;
    result->sig.length = 0;
    while (val != 0) {
        result->sig.text[result->sig.length++] = (u8)(val % 10);
        val /= 10;
    }
    first = result->sig.text;
    last = result->sig.text + result->sig.length;
    while (--last > first) {
        u8 tmp = *first;
        *first++ = *last;
        *last = tmp;
    }
    result->exp = result->sig.length - 1;
}
#pragma dont_inline reset

/* Rounds the digit string of `d` up by one unit in its last place. */
static inline void __round_up_dec(decimal* d)
{
    u8* base = d->sig.text;
    u8* p = base + d->sig.length - 1;

    for (;;) {
        if (*p < 9) {
            (*p)++;
            return;
        }
        if (p == base) {
            *p = 1;
            d->exp++;
            return;
        }
        *p = 0;
        p--;
    }
}

/* Multiplies two decimals, rounding the product to 36 digits. */
void __timesdec(decimal* result, const decimal* x, const decimal* y)
{
    u8 mantissa[MAX_SIG_DIGITS * 2];
    u32 accumulator = 0;
    int n = x->sig.length + y->sig.length - 1;
    u8* ptr = mantissa + n + 1;
    u8* end = ptr;
    int i;

    result->sign = 0;
    for (; n > 0; n--) {
        int k = y->sig.length - 1;
        int j = n - k - 1;
        int count;
        const u8* px;
        const u8* py;

        if (j < 0) {
            j = 0;
            k = n - 1;
        }
        count = k + 1;
        px = x->sig.text + j;
        py = y->sig.text + k;
        if (count > x->sig.length - j) {
            count = x->sig.length - j;
        }
        for (; count > 0; count--) {
            accumulator += *px++ * *py--;
        }
        *--ptr = accumulator % 10;
        accumulator /= 10;
    }
    result->exp = x->exp + y->exp;
    if (accumulator != 0) {
        *--ptr = accumulator;
        result->exp++;
    }
    for (i = 0; i < MAX_SIG_DIGITS && ptr < end; i++) {
        result->sig.text[i] = *ptr++;
    }
    result->sig.length = i;
    if (ptr < end && *ptr >= 5) {
        if (*ptr == 5) {
            const u8* q = ptr + 1;

            while (q < end && *q == 0) {
                q++;
            }
            if (q == end && !(ptr[-1] & 1)) {
                return;
            }
        }
        __round_up_dec(result);
    }
}

/* Builds a decimal from a digit string and an exponent, rounding past 36 digits. */
void __str2dec(decimal* d, const char* s, s16 exp)
{
    int i;

    d->exp = exp;
    d->sign = 0;
    i = 0;
    while (i < MAX_SIG_DIGITS && *s != 0) {
        d->sig.text[i++] = *s++ - '0';
    }
    d->sig.length = i;
    if (*s == 0) {
        return;
    }
    if (*s < 5) {
        return;
    }
    if (*s <= 5) {
        const char* t = s + 1;

        for (; *t != 0; t++) {
            if (*t != '0') {
                break;
            }
        }
        if (*t == 0 && !(d->sig.text[i - 1] & 1)) {
            return;
        }
    }
    __round_up_dec(d);
}

/* Builds 2^exp as a decimal. */
void __two_exp(decimal* result, s32 exp)
{
    decimal root;
    decimal copy;

    switch (exp) {
    case -64:
        __str2dec(result, "5421010862427522170037264004349708557128906" "25", -20);
        break;
    case -53:
        __str2dec(result, "11102230246251565404236316680908203125", -16);
        break;
    case -32:
        __str2dec(result, "23283064365386962890625", -10);
        break;
    case -16:
        __str2dec(result, "152587890625", -5);
        break;
    case -8:
        __str2dec(result, "390625", -3);
        break;
    case -7:
        __str2dec(result, "78125", -3);
        break;
    case -6:
        __str2dec(result, "15625", -2);
        break;
    case -5:
        __str2dec(result, "3125", -2);
        break;
    case -4:
        __str2dec(result, "625", -2);
        break;
    case -3:
        __str2dec(result, "125", -1);
        break;
    case -2:
        __str2dec(result, "25", -1);
        break;
    case -1:
        __str2dec(result, "5", -1);
        break;
    case 0:
        __str2dec(result, "1", 0);
        break;
    case 1:
        __str2dec(result, "2", 0);
        break;
    case 2:
        __str2dec(result, "4", 0);
        break;
    case 3:
        __str2dec(result, "8", 0);
        break;
    case 4:
        __str2dec(result, "16", 1);
        break;
    case 5:
        __str2dec(result, "32", 1);
        break;
    case 6:
        __str2dec(result, "64", 1);
        break;
    case 7:
        __str2dec(result, "128", 2);
        break;
    case 8:
        __str2dec(result, "256", 2);
        break;
    default:
        __two_exp(&root, exp / 2);
        __timesdec(result, &root, &root);
        if (exp & 1) {
            copy = *result;
            if (exp > 0) {
                __str2dec(&root, "2", 0);
            } else {
                __str2dec(&root, "5", -1);
            }
            __timesdec(result, &copy, &root);
        }
        break;
    }
}

/* Returns nonzero when two decimals are equal. */
int __equals_dec(const decimal* x, const decimal* y)
{
    int i;
    int n;
    const decimal* longer = x;

    if (x->sig.text[0] == 0) {
        return y->sig.text[0] == 0;
    }
    if (y->sig.text[0] == 0) {
        return x->sig.text[0] == 0;
    }
    if (x->exp == y->exp) {
        n = x->sig.length;
        if (n > y->sig.length) {
            n = y->sig.length;
        }
        for (i = 0; i < n; i++) {
            if (x->sig.text[i] != y->sig.text[i]) {
                return 0;
            }
        }
        if (n == x->sig.length) {
            longer = y;
        }
        for (; i < longer->sig.length; i++) {
            if (longer->sig.text[i] != 0) {
                return 0;
            }
        }
        return 1;
    }
    return 0;
}

/* Returns nonzero when `x` is less than `y`. */
int __less_dec(const decimal* x, const decimal* y)
{
    int i;
    int n;

    if (x->sig.text[0] == 0) {
        return y->sig.text[0] != 0;
    }
    if (y->sig.text[0] == 0) {
        return 0;
    }
    if (x->exp == y->exp) {
        n = x->sig.length;
        if (n > y->sig.length) {
            n = y->sig.length;
        }
        for (i = 0; i < n; i++) {
            if (x->sig.text[i] < y->sig.text[i]) {
                return 1;
            }
            if (y->sig.text[i] < x->sig.text[i]) {
                return 0;
            }
        }
        if (n == x->sig.length) {
            for (; i < y->sig.length; i++) {
                if (y->sig.text[i] != 0) {
                    return 1;
                }
            }
        }
        return 0;
    }
    return x->exp < y->exp;
}

/* Subtracts `y` from `x` into `result` (x >= y), rounding to even past 36 digits. */
void __minus_dec(decimal* result, const decimal* x, const decimal* y)
{
    *result = *x;
    if (y->sig.text[0] != 0) {
        int shift;
        int n;
        u8* rb;
        u8* rp;
        const u8* yb;
        const u8* yp;
        const u8* ystart;
        u8* q;
        int ylen;

        n = result->sig.length;
        if (n < y->sig.length) {
            n = y->sig.length;
        }
        shift = result->exp - y->exp;
        n += shift;
        if (n > MAX_SIG_DIGITS) {
            n = MAX_SIG_DIGITS;
        }
        while (result->sig.length < n) {
            u8 len = result->sig.length;

            result->sig.text[len] = 0;
            result->sig.length = len + 1;
        }
        rb = result->sig.text;
        rp = rb + n;
        yb = y->sig.text;
        if (y->sig.length + shift < n) {
            rp = rb + y->sig.length + shift;
        }
        yp = yb + ((rp - rb) - shift);
        ystart = yp;
        while (rp > rb && yp > yb) {
            rp--;
            yp--;
            if (*rp < *yp) {
                q = rp - 1;
                while (*q == 0) {
                    q--;
                }
                for (; q != rp;) {
                    (*q)--;
                    q++;
                    *q += 10;
                }
            }
            *rp -= *yp;
        }
        ylen = ystart - yb;
        if (ylen < y->sig.length) {
            int round_down = 0;

            if (*ystart < 5) {
                round_down = 1;
            } else if (*ystart == 5) {
                const u8* t = ystart + 1;
                const u8* tend = yb + y->sig.length;

                for (; t < tend; t++) {
                    if (*t != 0) {
                        break;
                    }
                }
                if (t >= tend) {
                    rp = rb + ylen + shift - 1;
                    if (*rp & 1) {
                        round_down = 1;
                    }
                }
            }
            if (round_down) {
                if (*rp < 1) {
                    q = rp - 1;
                    while (*q == 0) {
                        q--;
                    }
                    for (; q != rp;) {
                        (*q)--;
                        q++;
                        *q += 10;
                    }
                }
                (*rp)--;
            }
        }
        /* strip leading zeros */
        {
            u8* lead = rb;
            u8* tail;

            while (*lead == 0) {
                lead++;
            }
            if (lead > rb) {
                int skip = lead - rb;

                result->exp -= skip;
                for (tail = rb + result->sig.length; lead < tail;) {
                    *rb++ = *lead++;
                }
                result->sig.length -= skip;
            }
        }
        /* strip trailing zeros */
        {
            u8* last = result->sig.text + result->sig.length;

            while (last > result->sig.text && last[-1] == 0) {
                last--;
            }
            result->sig.length = (last - result->sig.text) + 1;
        }
    }
}

/* Counts the trailing zero bits of a nonzero word. */
static inline int count_trailing_zeros(u32 v)
{
    return 32 - __cntlzw((v - 1) & ~v);
}

/* Converts a finite or special double to its exact decimal digits. */
void __num2dec_internal(decimal* result, f64 x)
{
    char sign;

    sign = __signbitd(x) != 0;

    if (x == 0.0) {
        result->sign = sign;
        result->exp = 0;
        result->sig.length = 1;
        result->sig.text[0] = 0;
        return;
    }
    if (__fpclassifyd(x) <= 2) {
        result->sign = sign;
        result->exp = 0;
        result->sig.length = 1;
        result->sig.text[0] = (__fpclassifyd(x) == 1) ? 'N' : 'I';
        return;
    }
    if (sign) {
        x = -x;
    }
    {
        decimal int_part;
        decimal pow2;
        s32 exp;
        f64 mant = frexp(x, &exp);
        u32 lo = ((u32*)&mant)[1];
        u32 hi = ((u32*)&mant)[0] | 0x100000;
        int tz;
        int bits;

        tz = count_trailing_zeros(lo);
        if (lo == 0) {
            tz = count_trailing_zeros(hi) + 32;
        }
        bits = 53 - tz;
        __two_exp(&pow2, exp - bits);
        __ull2dec(&int_part, (u64)ldexp(mant, bits));
        __timesdec(result, &int_part, &pow2);
        result->sign = sign;
    }
}

/* Converts `x` to its decimal digits under `form`. */
void __num2dec(const decform* form, f64 x, decimal* d)
{
    s16 digits = form->digits;
    int i;

    __num2dec_internal(d, x);
    if (d->sig.text[0] > 9) {
        return;
    }
    if (digits > MAX_SIG_DIGITS) {
        digits = MAX_SIG_DIGITS;
    }
    if (digits > 0 && digits < d->sig.length) {
        int cmp;
        u8* base = d->sig.text;
        u8 c = base[digits];

        if (c > 5) {
            cmp = 1;
        } else if (c < 5) {
            cmp = -1;
        } else {
            const u8* p = base + digits + 1;
            const u8* e = base + d->sig.length;

            for (; p < e; p++) {
                if (*p != 0) {
                    cmp = 1;
                    break;
                }
            }
            if (p >= e) {
                cmp = (d->sig.text[digits - 1] & 1) ? 1 : -1;
            }
        }
        d->sig.length = digits;
        if (cmp >= 0) {
            u8* q = d->sig.text + digits - 1;

            for (;;) {
                if (*q < 9) {
                    (*q)++;
                    break;
                }
                if (q == d->sig.text) {
                    *q = 1;
                    d->exp++;
                    break;
                }
                *q = 0;
                q--;
            }
        }
    }
    while (d->sig.length < digits) {
        u8 len = d->sig.length;

        d->sig.text[len] = 0;
        d->sig.length = len + 1;
    }
    d->exp -= d->sig.length - 1;
    for (i = 0; i < d->sig.length; i++) {
        d->sig.text[i] += '0';
    }
}

static f64 decimal_powers_of_ten[8] = { 10.0, 100.0, 1000.0, 10000.0, 100000.0, 1000000.0, 10000000.0, 100000000.0 };

/* Converts decimal digits back to the nearest f64. */
f64 __dec2num(const decimal* d)
{
    decimal dec;
    decimal max_dec;
    decimal nearest;
    decimal next;
    decimal bracket;
    decimal diff_high;
    f64 value;
    f64 x;
    int exp;
    int below;
    int step_down;
    u8* p;
    u8* end;
    f64 one;

    if (d->sig.length == 0) {
        if (d->sign == 0) {
            one = 1.0;
        } else {
            one = -1.0;
        }
        return copysign(0.0, one);
    }
    switch (d->sig.text[0]) {
    case '0':
        if (d->sign == 0) {
            one = 1.0;
        } else {
            one = -1.0;
        }
        return copysign(0.0, one);
    case 'I':
        if (d->sign == 0) {
            one = 1.0;
        } else {
            one = -1.0;
        }
        return copysign(*(f32*)__float_huge, one);
    case 'N': {
        f64 nan_value;

        ((u32*)&nan_value)[1] = 0;
        ((u32*)&nan_value)[0] = 0x7FF00000;
        if (d->sign != 0) {
            ((u32*)&nan_value)[1] = 0;
            ((u32*)&nan_value)[0] = 0x80000000 | 0x7FF00000;
        }
        ((u32*)&nan_value)[0] |= 0x80000;
        return nan_value;
    }
    default:
        break;
    }
    dec = *d;
    p = dec.sig.text;
    end = p + dec.sig.length;
    if (d->sig.text[0] < 'N') {
        for (; p < end; p++) {
            *p -= '0';
        }
    }
    dec.exp += dec.sig.length - 1;
    exp = dec.exp;
    __str2dec(&max_dec, "179769313486231580793728714053034151", 308);
    if (__less_dec(&max_dec, &dec)) {
        if (d->sign == 0) {
            one = 1.0;
        } else {
            one = -1.0;
        }
        return copysign(*(f32*)__float_huge, one);
    }
    p = &dec.sig.text[1];
    value = dec.sig.text[0];
    while (p < end) {
        int cnt = (end - p) % 8;
        u32 acc = 0;
        int i;
        f64 scaled;
        f64 sum;

        if (cnt == 0) {
            cnt = 8;
        }
        for (i = 0; i < cnt; i++) {
            acc = acc * 10 + *p++;
        }
        scaled = value * decimal_powers_of_ten[cnt - 1];
        sum = scaled + (f64)acc;
        if (acc != 0 && scaled == sum) {
            break;
        }
        value = sum;
        exp -= cnt;
    }
    if (exp < 0) {
        value = value / pow(5.0, (f64)-exp);
    } else {
        value = value * pow(5.0, (f64)exp);
    }
    value = ldexp(value, exp);
    if (__fpclassifyd(value) == 2) {
        value = 1.7976931348623157e308;
    }
    below = 0;
    __num2dec_internal(&nearest, value);
    if (!__equals_dec(&nearest, &dec)) {
        int overflow = 0;

        if (__less_dec(&nearest, &dec)) {
            below = 1;
        }
        step_down = (below == 0);
        x = value;
        for (;;) {
            if (step_down == 0) {
                *(s64*)&x += 1;
                if (__fpclassifyd(x) == 2) {
                    overflow = 1;
                    break;
                }
            } else {
                *(s64*)&x -= 1;
            }
            __num2dec_internal(&next, x);
            if (below != 0) {
                if (!__less_dec(&next, &dec)) {
                    break;
                }
            } else if (!__less_dec(&dec, &next)) {
                f64 old = value;

                value = x;
                bracket = nearest;
                nearest = next;
                next = bracket;
                x = old;
                break;
            }
            value = x;
            nearest = next;
        }
        if (!overflow) {
            __minus_dec(&bracket, &dec, &nearest);
            __minus_dec(&diff_high, &next, &dec);
            if (__equals_dec(&bracket, &diff_high)) {
                if (((u32*)&value)[1] & 1) {
                    value = x;
                }
            } else if (!__less_dec(&bracket, &diff_high)) {
                value = x;
            }
        }
    }
    if (dec.sign != 0) {
        value = -value;
    }
    return value;
}
