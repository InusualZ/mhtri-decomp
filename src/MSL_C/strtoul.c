/*
 * MSL_C/strtoul.c - the string-to-integer routines: `__strtoul`, `__strtoull`, `strtol`, `atoi`.
 *
 * RANGE. .text 0x80460D0C..0x80461778 (4 functions in the map, 0xA6C B).
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi`.
 * NAMES. file name GUESS (MSL `strtoul`); `__strtoull` is the map name of the 64-bit twin (renamed from its
 *    generated label; GUESS: it differs only in the `__div2u` / carry arithmetic); the state names are GUESSes.
 * EVIDENCE. both `__strtoul` variants take the string reader of the scanner unit by pointer and read the locale
 *    class table; `strtol` / `atoi` call `__strtoul`; `errno` is the only data they write.
 * RESIDUALS. COARSE: no data separates `atoi` from `strtol`; `__strtoul` 96.3 and `__strtoull` 96.7: the target
 *    shares one failure tail between the non-alphanumeric and the out-of-base paths where ours duplicates it, sets
 *    the width counter after the argument registers of the first read, and `__strtoull` keeps a zero in r14 (a
 *    sign-extended 64-bit compare against it) and spills `chars_scanned` to the stack.
 * SHAPES. a six-state scanner (sign, base prefix, digits) driven by a switch on the state word.
 */
#include "MSL_C/ctype.h"
#include "MSL_C/errno.h"
#include "MSL_C/scanf.h"
#include "MSL_C/strtoul.h"

enum ScanState {
    SCAN_START = 0x01,
    SCAN_SIGN_FOUND = 0x02,
    SCAN_LEADING_ZERO = 0x04,
    SCAN_NEED_DIGIT = 0x08,
    SCAN_DIGIT_LOOP = 0x10,
    SCAN_FINISHED = 0x20,
    SCAN_FAILURE = 0x40
};

#define SCAN_FINAL (SCAN_FINISHED | SCAN_FAILURE)
#define SCAN_SUCCESS (SCAN_LEADING_ZERO | SCAN_DIGIT_LOOP | SCAN_FINISHED)

#define IS_CLASS(c, mask) ctype_class(c, mask)
#define TO_UPPER(c) ctype_toupper(c)

/* 0x80460D0C (0x414): scans an unsigned integer through `read` and returns its magnitude. */
u32 __strtoul(int base, int max_width, ReadProc read, ReadArg* read_arg, int* chars_scanned, int* negative, int* overflow)
{
    int state = SCAN_START;
    int count = 0;
    int spaces = 0;
    u32 value = 0;
    u32 limit = 0;
    int c;

    *overflow = 0;
    *negative = 0;

    if (base < 0 || base == 1 || base > 36 || max_width < 1) {
        state = SCAN_FAILURE;
    } else {
        count = 1;
        c = read(read_arg, 0, 0);
    }

    if (base != 0) {
        limit = 0xFFFFFFFFU / (u32)base;
    }

    while (count <= max_width && c != -1 && !(state & SCAN_FINAL)) {
        switch (state) {
        case SCAN_START:
            if (IS_CLASS(c, CTYPE_SPACE)) {
                c = read(read_arg, 0, 0);
                spaces++;
            } else {
                if (c == '+') {
                    count++;
                    c = read(read_arg, 0, 0);
                } else if (c == '-') {
                    count++;
                    c = read(read_arg, 0, 0);
                    *negative = 1;
                }
                state = SCAN_SIGN_FOUND;
            }
            break;
        case SCAN_SIGN_FOUND:
            if ((base == 0 || base == 16) && c == '0') {
                state = SCAN_LEADING_ZERO;
                count++;
                c = read(read_arg, 0, 0);
            } else {
                state = SCAN_NEED_DIGIT;
            }
            break;
        case SCAN_LEADING_ZERO:
            if (c == 'X' || c == 'x') {
                base = 16;
                state = SCAN_NEED_DIGIT;
                count++;
                c = read(read_arg, 0, 0);
            } else {
                if (base == 0) {
                    base = 8;
                }
                state = SCAN_DIGIT_LOOP;
            }
            break;
        case SCAN_NEED_DIGIT:
        case SCAN_DIGIT_LOOP:
            if (base == 0) {
                base = 10;
            }
            if (limit == 0) {
                limit = 0xFFFFFFFFU / (u32)base;
            }
            if (IS_CLASS(c, CTYPE_DIGIT)) {
                c -= '0';
                if (c >= base) {
                    state = (state == SCAN_DIGIT_LOOP) ? SCAN_FINISHED : SCAN_FAILURE;
                    c += '0';
                    break;
                }
            } else if (IS_CLASS(c, CTYPE_ALPHA)) {
                if (TO_UPPER(c) - ('A' - 10) >= base) {
                    state = (state == SCAN_DIGIT_LOOP) ? SCAN_FINISHED : SCAN_FAILURE;
                    break;
                }
                c = TO_UPPER(c) - ('A' - 10);
            } else {
                state = (state == SCAN_DIGIT_LOOP) ? SCAN_FINISHED : SCAN_FAILURE;
                break;
            }
            if (value > limit) {
                *overflow = 1;
            }
            value *= base;
            if ((u32)c > 0xFFFFFFFFU - value) {
                *overflow = 1;
            }
            value += c;
            state = SCAN_DIGIT_LOOP;
            count++;
            c = read(read_arg, 0, 0);
            break;
        }
    }

    if (!(state & SCAN_SUCCESS)) {
        value = 0;
        *chars_scanned = 0;
    } else {
        *chars_scanned = count + spaces - 1;
    }
    read(read_arg, c, 1);
    return value;
}

/* 0x80461120 (0x4A8): scans an unsigned 64-bit integer through `read` and returns its magnitude. */
u64 __strtoull(int base, int max_width, ReadProc read, ReadArg* read_arg, int* chars_scanned, int* negative, int* overflow)
{
    int state = SCAN_START;
    int count = 0;
    int spaces = 0;
    u64 value = 0;
    u64 limit = 0;
    int c;

    *overflow = 0;
    *negative = 0;

    if (base < 0 || base == 1 || base > 36 || max_width < 1) {
        state = SCAN_FAILURE;
    } else {
        count = 1;
        c = read(read_arg, 0, 0);
    }

    if (base != 0) {
        limit = 0xFFFFFFFFFFFFFFFFULL / (u64)base;
    }

    while (count <= max_width && c != -1 && !(state & SCAN_FINAL)) {
        switch (state) {
        case SCAN_START:
            if (IS_CLASS(c, CTYPE_SPACE)) {
                c = read(read_arg, 0, 0);
                spaces++;
            } else {
                if (c == '+') {
                    count++;
                    c = read(read_arg, 0, 0);
                } else if (c == '-') {
                    count++;
                    c = read(read_arg, 0, 0);
                    *negative = 1;
                }
                state = SCAN_SIGN_FOUND;
            }
            break;
        case SCAN_SIGN_FOUND:
            if ((base == 0 || base == 16) && c == '0') {
                state = SCAN_LEADING_ZERO;
                count++;
                c = read(read_arg, 0, 0);
            } else {
                state = SCAN_NEED_DIGIT;
            }
            break;
        case SCAN_LEADING_ZERO:
            if (c == 'X' || c == 'x') {
                base = 16;
                state = SCAN_NEED_DIGIT;
                count++;
                c = read(read_arg, 0, 0);
            } else {
                if (base == 0) {
                    base = 8;
                }
                state = SCAN_DIGIT_LOOP;
            }
            break;
        case SCAN_NEED_DIGIT:
        case SCAN_DIGIT_LOOP:
            if (base == 0) {
                base = 10;
            }
            if (!limit) {
                limit = 0xFFFFFFFFFFFFFFFFULL / (u64)base;
            }
            if (IS_CLASS(c, CTYPE_DIGIT)) {
                c -= '0';
                if (c >= base) {
                    state = (state == SCAN_DIGIT_LOOP) ? SCAN_FINISHED : SCAN_FAILURE;
                    c += '0';
                    break;
                }
            } else if (IS_CLASS(c, CTYPE_ALPHA)) {
                if (TO_UPPER(c) - ('A' - 10) >= base) {
                    state = (state == SCAN_DIGIT_LOOP) ? SCAN_FINISHED : SCAN_FAILURE;
                    break;
                }
                c = TO_UPPER(c) - ('A' - 10);
            } else {
                state = (state == SCAN_DIGIT_LOOP) ? SCAN_FINISHED : SCAN_FAILURE;
                break;
            }
            if (value > limit) {
                *overflow = 1;
            }
            value *= base;
            if ((u64)c > 0xFFFFFFFFFFFFFFFFULL - value) {
                *overflow = 1;
            }
            value += c;
            state = SCAN_DIGIT_LOOP;
            count++;
            c = read(read_arg, 0, 0);
            break;
        }
    }

    if (!(state & SCAN_SUCCESS)) {
        value = 0;
        *chars_scanned = 0;
    } else {
        *chars_scanned = count + spaces - 1;
    }
    read(read_arg, c, 1);
    return value;
}

/* 0x804615C8 (0xEC): converts a decimal, octal or hexadecimal string to a signed long. */
long strtol(const char* str, char** end, int base)
{
    u32 value;
    int scanned;
    int negative;
    int overflow;
    StringReadState reader;

    reader.cursor = str;
    reader.hit_nul = 0;
    value = __strtoul(base, 0x7FFFFFFF, (ReadProc)__StringRead, (ReadArg*)&reader, &scanned, &negative, &overflow);
    if (end != NULL) {
        *end = (char*)str + scanned;
    }
    if (overflow || (!negative && value > 0x7FFFFFFFU) || (negative && value > 0x80000000U)) {
        errno = 0x22;
        return negative ? 0x80000000 : 0x7FFFFFFF;
    }
    if (negative) {
        value = -value;
    }
    return value;
}

/* 0x804616B4 (0xC4): converts a decimal string to an int. */
int atoi(const char* str)
{
    u32 value;
    int overflow;
    int negative;
    int scanned;
    StringReadState reader;

    reader.cursor = str;
    reader.hit_nul = 0;
    value = __strtoul(10, 0x7FFFFFFF, (ReadProc)__StringRead, (ReadArg*)&reader, &scanned, &negative, &overflow);
    if (overflow || (!negative && value > 0x7FFFFFFFU) || (negative && value > 0x80000000U)) {
        errno = 0x22;
        return negative ? 0x80000000 : 0x7FFFFFFF;
    }
    if (negative) {
        value = -value;
    }
    return value;
}
