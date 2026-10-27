/*
 * NHTTP_util.c - the Revolution SDK NHTTP library's string-helper TU: the C-library shims, the
 *   case-insensitive compare, URL encoding, number parsing and printing, token and substring search, and
 *   Base64 encoding.
 * RANGE. .text 0x80516CA0-0x80517614 (15 functions), in address order; .rodata 0x80574D00-0x80574D28 (the
 *   powers-of-ten table of `NHTTPi_uintToStr`), .data 0x80630B38-0x80630B80 (`NHTTPi_base64Alphabet`).  Both
 *   `.data` edges are 8-byte section starts behind fills (0x80630B31..B38, 0x80630B79..B80) that a 0x41-byte
 *   object inside one TU would not get.  The `.text` edges have no data pin: the left one is where the
 *   socket layer ends, the right one is where the comm thread's helpers begin.
 * SOURCE FILE. GUESS `NHTTP_util.c`: no `__FILE__` string; named for what the TU holds.
 * FLAGS. GC/3.0a5.2 with `cflags_nhttp`, plus `#pragma inline_max_size(8)` (as `d_nhttp.c`).
 * NAMES. GUESS, read off the bodies: `NHTTPi_strToDec`, `NHTTPi_uintToStr`.
 * SHAPES.
 *   - the four 4-12 byte shims (`NHTTPi_memcpy`/`strlen`/`strcmp`/`memclr`) are `asm` (MWCC would inline a C
 *     tail-call shim); a range test keeps the constant on the left (`'0' <= c`) to keep retail's two compares.
 *   - `NHTTPi_encodeUrlChar` computes the nibbles as `s32` from `(u8)c`.
 * RESIDUALS.
 *   - `NHTTPi_Base64Encode`: retail divides by 3 with `li r0,3; divwu` and multiplies with `mulli` (no
 *     reciprocal, no shift-subtract); no spelling, local, inline divisor or `optimization_level` reproduces it.
 *   - `NHTTPi_containsString`, `NHTTPi_uintToStr`, `NHTTPi_compareToken`, `NHTTPi_strtonum`: register colouring
 *     or scheduling.
 */

#include "NHTTP/d_nhttp.h"
#include "NHTTP/NHTTP_request.h"
#include "NHTTP/NHTTP_util.h"
#include "NHTTP/NHTTP_thread.h"
#include "NHTTP/NHTTP_list.h"
#include "NHTTP/NHTTP_bgnend.h"
#include "NHTTP/NHTTP_os_RVL.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "unsplit/Runtime.PPCEABI.H.h"
#include "unsplit/SO.h"
#include "SO/soi.h"
#include "unsplit/SSL.h"

/* Retail inlines only the one-line list wrappers into this file's bodies and keeps a `bl` to every
 * bigger function (`NHTTPi_SocSend`, `NHTTPGetBodyBuffer`, `NHTTPi_GetResponseSize`, ...); an inline
 * limit of 8-10 reproduces that, where the default limit folds them all in. */
#pragma inline_max_size(8)

/* 0x80516CA0 (4): tail shim to memcpy. */
asm void* NHTTPi_memcpy(void* dst, const void* src, u32 n) {
    nofralloc
    b memcpy
}

/* 0x80516CA4 (4): tail shim to strlen. */
asm u32 NHTTPi_strlen(const char* s) {
    nofralloc
    b strlen
}

/* 0x80516CA8 (4): tail shim to strcmp. */
asm int NHTTPi_strcmp(const char* a, const char* b) {
    nofralloc
    b strcmp
}

/* 0x80516CAC (0xC): zero-fill shim (memset(dst, 0, n)). */
asm void* NHTTPi_memclr(void* dst, u32 n) {
    nofralloc
    mr r5, r4
    li r4, 0
    b memset
}

/* The case fold `NHTTPi_strnicmp` and `NHTTPi_strToHex` both need.  MWCC inlines it at every call
 * site, so it has no symbol of its own. */
static inline s32 NHTTPi_toLower(char c) {
    return ((c >= 'A') & (c <= 'Z')) ? c + 32 : c;
}

/* 0x80516CB8 (0xCC): the bounded case-insensitive compare.  It answers 0 when both strings run
 * out inside `n` characters, and the number of characters still uncompared otherwise - not the
 * character difference a `strnicmp` would report. */
s32 NHTTPi_strnicmp(const char* s1, const char* s2, s32 n) {
    for (; n > 0; n--) {
        char c1 = *s1++;
        char c2 = *s2++;

        if (c1 == 0 || c2 == 0) {
            if (c1 == 0 && c2 == 0) {
                n = 0;
                break;
            }
        }
        if (NHTTPi_toLower(c1) != NHTTPi_toLower(c2)) {
            break;
        }
    }
    return n;
}

/* 0x80516D84 (0x70): the URL-encoded length of a NUL-terminated string. */
s32 NHTTPi_urlEncodedLength(const char* s) {
    const char* p = s;
    char c = *p++;
    s32 length = 0;

    while (c != 0) {
        if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
            c == ' ') {
            length += 1;
        } else {
            length += 3;
        }
        c = *p++;
    }
    return length;
}

/* 0x80516DF4 (0x74): the same for at most `n` characters. */
s32 NHTTPi_urlEncodedLengthN(const char* s, s32 n) {
    const char* p = s;
    char c = *p++;
    s32 length = 0;

    for (; n > 0; n--) {
        if (('0' <= c && c <= '9') || ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z') ||
            c == ' ') {
            length += 1;
        } else {
            length += 3;
        }
        c = *p++;
    }
    return length;
}

/* 0x80516E68 (0xA0): write one URL-encoded character at `dst`.  A space becomes `+`, an
 * alphanumeric is copied through, and anything else becomes `%XX` in upper-case hex.  Answers the
 * number of bytes written. */
s32 NHTTPi_encodeUrlChar(u8* dst, char c) {
    s32 high;
    s32 low;

    if (c == ' ') {
        dst[0] = '+';
        return 1;
    }
    if (('0' <= c && c <= '9') || ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z')) {
        dst[0] = c;
        return 1;
    }
    high = (u8)c >> 4;
    low = (u8)c & 0xF;
    dst[0] = '%';
    dst[1] = high < 10 ? high + '0' : high + '7';
    dst[2] = low < 10 ? low + '0' : low + '7';
    return 3;
}

/* 0x80516F08 (0x124): read at most `n` characters as a hexadecimal number.  Leading spaces are
 * skipped and a space or NUL after the first digit ends the number; answers -1 when the field holds
 * no hex digit, when a character is neither a hex digit, a space nor the end, and when eight
 * characters would carry the value into the sign bit. */
s32 NHTTPi_strToHex(const char* s, s32 n) {
    s32 value;
    BOOL seen;

    if (n > 8) {
        return -1;
    }
    if ((n == 8) & (*s > '7')) {
        return -1;
    }
    value = 0;
    seen = FALSE;
    for (; n > 0; n--) {
        char c = NHTTPi_toLower(*s);

        if ('0' <= c && c <= '9') {
            value = value * 16 + (c - '0');
            seen = TRUE;
        } else if ('a' <= c && c <= 'f') {
            value = value * 16 + (c - 'a' + 10);
            seen = TRUE;
        } else if (seen && (c == ' ' || c == 0)) {
            break;
        } else if (!seen && c == ' ') {
            /* a leading space is skipped */
        } else {
            return -1;
        }
        s++;
    }
    return value;
}

/* Read at most `n` characters as a decimal number: leading spaces are skipped, a space or NUL after
 * the first digit ends it.  Answers -1 for a character that is no digit, for more than ten
 * characters and for an overflow. */
s32 NHTTPi_strToDec(const char* s, s32 n) {
    s32 value;
    BOOL seen;
    s32 previous;

    if (n > 10) {
        return -1;
    }
    value = 0;
    seen = FALSE;
    for (; n > 0; n--) {
        char c = *s;

        if (seen && (c == ' ' || c == 0)) {
            break;
        }
        if (!(!seen && c == ' ')) {
            if ('0' > c || c > '9') {
                return -1;
            }
            previous = value;
            seen = TRUE;
            value = value * 10 + c - '0';
            if (previous > value) {
                return -1;
            }
        }
        s++;
    }
    return value;
}

/* Write the decimal digits of `value` at `buf` (no terminator; nine or ten digits at most) and answer
 * how many were written. */
s32 NHTTPi_uintToStr(char* buf, u32 value) {
    u32 powers[9] = {1000000000, 100000000, 10000000, 1000000, 100000, 10000, 1000, 100, 10};
    char* out = buf;
    s32 i;
    BOOL started = FALSE;
    s32 length = 0;

    for (i = 0; i < 9; i++) {
        u32 power = powers[i];

        if (value >= power) {
            u32 digit = value / power;

            started = TRUE;
            length++;
            *out++ = digit + '0';
            value -= digit * power;
        } else if (started) {
            *out++ = '0';
            length++;
        }
    }
    buf[length] = value + '0';
    return length + 1;
}

/* 0x80517248 (0xB4): compare two tokens case-insensitively.  Answers 0 once both have run out at a
 * NUL or a space, -1 as soon as a character differs. */
s32 NHTTPi_compareToken(const char* a, const char* b) {
    while (NHTTPi_toLower(*a) == NHTTPi_toLower(*b)) {
        if (*a == 0 || *a == ' ') {
            return 0;
        }
        a++;
        b++;
    }
    return -1;
}

/* 0x805172FC (0x90): read at most `n` characters as a decimal number, skipping spaces.  Answers -1
 * when the field held no digit at all, or when the number needs more than nine digits. */
s32 NHTTPi_strtonum(const char* s, s32 n) {
    char c;
    s32 value = 0;
    s32 digits = 0;

    for (; n != 0; n--) {
        c = *s;

        if (c != ' ') {
            if ((c >= '0') & (c <= '9')) {
                value = value * 10 + (c - '0');
                digits++;
                if (digits > 9) {
                    return -1;
                }
            }
        }
        s++;
    }
    if (digits == 0) {
        return -1;
    }
    return value;
}

/* 0x8051738C (0xAC): find `needle` inside the first `haystackLen` characters of `haystack`.
 * Answers 0 on a hit and -1 on a miss, or when the needle is the longer of the two. */
s32 NHTTPi_containsString(const char* haystack, s32 haystackLen, const char* needle, s32 needleLen) {
    const char* p;
    s32 i;
    s32 k;

    if (haystackLen < needleLen) {
        return -1;
    }
    p = haystack;
    for (i = 0; i < haystackLen - needleLen + 1; i++) {
        if (needle[0] == *p) {
            for (k = 1; k < needleLen; k++) {
                if (haystack[i + k] != needle[k]) {
                    break;
                }
            }
            if (k == needleLen) {
                return 0;
            }
        }
        p++;
    }
    return -1;
}

/* Base64-encode the NUL-terminated `src` into `dst` (with '=' padding and a terminator).  Answers the
 * encoded length. */
s32 NHTTPi_Base64Encode(char* dst, const char* src) {
    const char* alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    char* out = dst;
    s32 length = strlen(src);
    u32 groups = (u32)(length + 2) / 3;
    s32 covered = 0;
    u32 i;

    if (length > 0) {
        covered = groups * 3;
        for (i = 0; i < groups; i++) {
            out[0] = alphabet[src[0] >> 2];
            out[1] = alphabet[((src[0] << 4) & 0x30) + (src[1] >> 4)];
            out[2] = alphabet[((src[1] << 2) & 0x3C) + (src[2] >> 6)];
            out[3] = alphabet[src[2] & 0x3F];
            src += 3;
            out += 4;
        }
    }
    if (covered == length + 1) {
        out[-1] = '=';
    } else if (covered == length + 2) {
        out[-2] = '=';
        out[-1] = '=';
    }
    out[0] = 0;
    return strlen(dst);
}
