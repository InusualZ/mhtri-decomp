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
 * RESIDUALS. `__str2dec`, `__timesdec`, `__minus_dec`: retail scans the rounding tail with a counted loop whose
 *    nonzero exit jumps past the parity test (MSL's `goto round` / `goto done`); every goto-free spelling either
 *    duplicates the inlined round-up or adds the `q < end` compare and loses the `mtctr`/`bdnz` - the goto spellings
 *    measure 100 against 89.2, 98.7 and 99.3.
 * SHAPES. the digit loops are compiler-unrolled by eight; `decimal` copies are struct assignments. MSL's helper
 *    shapes decide the colouring: `count_trailing_zero(f64)` declares its locals before assigning them, `__num2dec`
 *    rounds through the inline `__rounddec` / `__must_round` / `__dorounddecup`, and `__dec2num` keeps the NaN word
 *    as a `u64` and runs its feedback loop with the bracket tail inside the loop.
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
void __minus_dec(decimal* z, const decimal* x, const decimal* y)
{
    int zlen;
    int dexp;
    u8* ib;
    u8* i;
    u8* ie;
    const u8* jb;
    const u8* j;
    const u8* jn;

    *z = *x;
    if (y->sig.text[0] == 0) {
        return;
    }
    zlen = z->sig.length;
    if (zlen < y->sig.length) {
        zlen = y->sig.length;
    }
    dexp = z->exp - y->exp;
    zlen += dexp;
    if (zlen > MAX_SIG_DIGITS) {
        zlen = MAX_SIG_DIGITS;
    }
    while (z->sig.length < zlen) {
        z->sig.text[z->sig.length++] = 0;
    }
    ib = z->sig.text;
    i = ib + zlen;
    if (y->sig.length + dexp < zlen) {
        i = ib + (y->sig.length + dexp);
    }
    jb = y->sig.text;
    j = jb + (i - ib - dexp);
    jn = j;
    while (i > ib && j > jb) {
        --i;
        --j;
        if (*i < *j) {
            u8* k = i - 1;

            while (*k == 0) {
                --k;
            }
            while (k != i) {
                --*k;
                *++k += 10;
            }
        }
        *i -= *j;
    }
    if (jn - jb < y->sig.length) {
        int round_down = 0;

        if (*jn < 5) {
            round_down = 1;
        } else if (*jn == 5) {
            const u8* je = y->sig.text + y->sig.length;

            j = jn + 1;
            while (j < je && *j == 0) {
                ++j;
            }
            if (j >= je) {
                i = ib + (jn - jb) + dexp - 1;
                if (*i & 1) {
                    round_down = 1;
                }
            }
        }
        if (round_down) {
            if (*i < 1) {
                u8* k = i - 1;

                while (*k == 0) {
                    --k;
                }
                while (k != i) {
                    --*k;
                    *++k += 10;
                }
            }
            *i -= 1;
        }
    }
    for (i = ib; *i == 0; ++i) {
    }
    if (i > ib) {
        u8 dl = (u8)(i - ib);

        z->exp -= dl;
        ie = ib + z->sig.length;
        for (; i < ie; ++i, ++ib) {
            *ib = *i;
        }
        z->sig.length -= dl;
    }
    ib = z->sig.text;
    for (i = ib + z->sig.length; i > ib;) {
        --i;
        if (*i != 0) {
            break;
        }
    }
    z->sig.length = (u8)(i - ib + 1);
}

/* Counts the trailing zero bits of a nonzero word. */
static inline int count_trailing_zeros(u32 v)
{
    return 32 - __cntlzw((v - 1) & ~v);
}

/* Counts the trailing zero bits of the 53-bit significand of a normalised double. */
static inline int count_trailing_zero(f64 x)
{
    int tz;
    u32 hi;
    u32 lo;

    lo = ((u32*)&x)[1];
    hi = ((u32*)&x)[0] | 0x100000;
    tz = count_trailing_zeros(lo);
    if (lo == 0) {
        tz = count_trailing_zeros(hi) + 32;
    }
    return tz;
}

/* Returns 1 when cutting `d` to `digits` digits rounds up, -1 when it truncates (ties go to even). */
static inline int __must_round(const decimal* d, int digits)
{
    const u8* e;
    const u8* i = d->sig.text + digits;

    if (*i > 5) {
        return 1;
    }
    if (*i < 5) {
        return -1;
    }
    {
        e = d->sig.text + d->sig.length;

        for (i++; i < e; i++) {
            if (*i != 0) {
                return 1;
            }
        }
    }
    if (d->sig.text[digits - 1] & 1) {
        return 1;
    }
    return -1;
}

/* Adds one unit in the last of the first `digits` digits of `d`, carrying into the exponent. */
static inline void __dorounddecup(decimal* d, int digits)
{
    u8* i;
    u8* b = d->sig.text;

    i = b + digits - 1;

    for (;;) {
        if (*i < 9) {
            (*i)++;
            break;
        }
        if (i == b) {
            *i = 1;
            d->exp++;
            break;
        }
        *i-- = 0;
    }
}

/* Rounds `d` to `digits` significant digits. */
static inline void __rounddec(decimal* d, int digits)
{
    if (digits > 0 && digits < d->sig.length) {
        int round = __must_round(d, digits);

        d->sig.length = digits;
        if (round >= 0) {
            __dorounddecup(d, digits);
        }
    }
}

/* Converts a finite or special double to its exact decimal digits. */
void __num2dec_internal(decimal* result, f64 x)
{
    char sign = (char)(__signbitd(x) != 0);

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
    if (sign != 0) {
        x = -x;
    }
    {
        s32 exp;
        f64 frac = frexp(x, &exp);
        long num_bits_extract = 53 - count_trailing_zero(frac);
        decimal int_part;
        decimal pow2;

        __two_exp(&pow2, exp - num_bits_extract);
        __ull2dec(&int_part, (u64)ldexp(frac, num_bits_extract));
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
    __rounddec(d, digits);
    while (d->sig.length < digits) {
        d->sig.text[d->sig.length++] = 0;
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
    if (d->sig.length == 0) {
        return copysign(0.0, d->sign == 0 ? 1.0 : -1.0);
    }

    switch (d->sig.text[0]) {
    case '0':
        return copysign(0.0, d->sign == 0 ? 1.0 : -1.0);
    case 'I':
        return copysign(*(f32*)__float_huge, d->sign == 0 ? 1.0 : -1.0);
    case 'N': {
        f64 result;
        u64* ll = (u64*)&result;

        *ll = 0x7FF0000000000000ULL;
        if (d->sign) {
            *ll |= 0x8000000000000000ULL;
        }
        *ll |= 0x8000000000000ULL;
        return result;
    }
    }
    {
        decimal dec = *d;
        u8* i = dec.sig.text;
        u8* e = i + dec.sig.length;
        f64 first_guess;
        int exponent;

        for (; i < e; ++i) {
            *i -= '0';
        }
        dec.exp += dec.sig.length - 1;
        exponent = dec.exp;

        {
            decimal max;

            __str2dec(&max, "179769313486231580793728714053034151", 308);
            if (__less_dec(&max, &dec)) {
                return copysign(*(f32*)__float_huge, d->sign == 0 ? 1.0 : -1.0);
            }
        }

        i = dec.sig.text;
        first_guess = *i++;

        while (i < e) {
            u32 ival = 0;
            int j;
            f64 temp1;
            f64 temp2;
            int ndig = (int)(e - i) % 8;

            if (ndig == 0) {
                ndig = 8;
            }
            for (j = 0; j < ndig; ++j, ++i) {
                ival = ival * 10 + *i;
            }
            temp1 = first_guess * decimal_powers_of_ten[ndig - 1];
            temp2 = temp1 + ival;
            if (ival != 0 && temp1 == temp2) {
                break;
            }
            first_guess = temp2;
            exponent -= ndig;
        }

        if (exponent < 0) {
            first_guess /= pow(5.0, -exponent);
        } else {
            first_guess *= pow(5.0, exponent);
        }
        first_guess = ldexp(first_guess, exponent);
        if (__fpclassifyd(first_guess) == 2) {
            first_guess = 1.7976931348623157e308;
        }

        {
            decimal feedback1;
            decimal feedback2;
            decimal difflow;
            decimal diffhigh;
            f64 next_guess;
            u64* ull = (u64*)&next_guess;
            int guessed_low = 0;

            __num2dec_internal(&feedback1, first_guess);
            if (!__equals_dec(&feedback1, &dec)) {
                if (__less_dec(&feedback1, &dec)) {
                    guessed_low = 1;
                }
                next_guess = first_guess;
                for (;;) {
                    if (guessed_low) {
                        ++*ull;
                        if (__fpclassifyd(next_guess) == 2) {
                            break;
                        }
                    } else {
                        --*ull;
                    }
                    __num2dec_internal(&feedback2, next_guess);
                    if (guessed_low && !__less_dec(&feedback2, &dec)) {
                    } else if (!guessed_low && !__less_dec(&dec, &feedback2)) {
                        difflow = feedback1;
                        feedback1 = feedback2;
                        feedback2 = difflow;
                        {
                            f64 temp = first_guess;

                            first_guess = next_guess;
                            next_guess = temp;
                        }
                    } else {
                        feedback1 = feedback2;
                        first_guess = next_guess;
                        continue;
                    }
                    __minus_dec(&difflow, &dec, &feedback1);
                    __minus_dec(&diffhigh, &feedback2, &dec);
                    if (__equals_dec(&difflow, &diffhigh)) {
                        if (*(u64*)&first_guess & 1) {
                            first_guess = next_guess;
                        }
                    } else if (!__less_dec(&difflow, &diffhigh)) {
                        first_guess = next_guess;
                    }
                    break;
                }
            }
        }
        if (dec.sign) {
            first_guess = -first_guess;
        }
        return first_guess;
    }
}
