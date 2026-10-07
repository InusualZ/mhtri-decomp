/*
 * MSL_C/string.c - the C string routines: `strcpy`, `strncpy`, `strcat`, `strcmp`, `strncmp`, `strchr`, `strrchr`,
 *    `strtok`, `strstr`.
 *
 * RANGE. .text 0x8045F554..0x8045F9E8 (9 functions in the map, 0x494 B); .sdata 0x80793CE8..0x80793CF0; .sdata2
 *    0x8079CA00..0x8079CA08.
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi`; `#pragma ipa file` is what moves the pooled "" literal from
 *    `.rodata` to `.sdata2` (the target has no `.rodata`); code scores are the same with and without it.
 * NAMES. file name GUESS (MSL `string`); `strtok` is the map's name and a GUESS (the dump has a placeholder there); `strlen` is registered separately (`MSL/strlen.cpp`); the statics
 *    `strtok_empty_string` and `strtok_cursor` are GUESS (the first is the never-written empty pointer the second
 *    is reset to).
 * EVIDENCE. `strtok`'s two `.sdata` statics (0x80793CE8 / 0x80793CEC) point at the pooled empty string `.sdata2`
 *    0x8079CA00; no function of the run calls outside it.
 * RESIDUALS. register allocation only: `strcpy` (the two `& 3` loads and the constant register), `strcmp` (same),
 *    `strstr` (the loop byte lives in r3 where the target uses r0), `strtok` (`li r0,0` scheduled after the cursor
 *    store, 3 instructions).
 * SHAPES. `strcpy` and `strcmp` copy/compare word-wise once both pointers share an alignment, using the
 *    zero-byte test `(w - 0x01010101) & ~w & 0x80808080`; the rest walk with pre-incremented byte pointers.
 */
#pragma ipa file
#include "types.h"

#define HAS_ZERO_BYTE(w) ((((w) + 0xFEFEFEFF) & ~(w)) & 0x80808080)

char* strcpy(char* dst, const char* src)
{
    u8* d = (u8*)dst;
    const u8* s = (const u8*)src;
    u32 c;
    u32 n;

    if (((u32)d & 3) == ((u32)s & 3)) {
        u32 off = (u32)s & 3;

        if (off != 0) {
            c = *s;
            *d = c;
            if (c == 0) {
                return dst;
            }
            for (n = 3 - off; n > 0; n--) {
                c = *++s;
                *++d = c;
                if (c == 0) {
                    return dst;
                }
            }
            d++;
            s++;
        }
        {
            u32* ws = (u32*)s;
            u32* wd = (u32*)d;
            u32 w = *ws;

            if (!HAS_ZERO_BYTE(w)) {
                wd--;
                do {
                    *++wd = w;
                    w = *++ws;
                } while (!HAS_ZERO_BYTE(w));
                wd++;
            }
            s = (const u8*)ws;
            d = (u8*)wd;
        }
    }
    c = *s;
    *d = c;
    if (c != 0) {
        do {
            c = *++s;
            *++d = c;
        } while (c != 0);
    }
    return dst;
}

char* strncpy(char* dst, const char* src, u32 n)
{
    const u8* s = (const u8*)src;
    u8* d = (u8*)dst;

    s--;
    d--;
    n++;
    while (--n != 0) {
        u32 c = *++s;
        *++d = c;
        if (c == 0) {
            while (--n != 0) {
                *++d = 0;
            }
            return dst;
        }
    }
    return dst;
}

char* strcat(char* dst, const char* src)
{
    const u8* s = (const u8*)src;
    u8* d = (u8*)dst;
    u32 c;

    s--;
    d--;
    do {
        c = *++d;
    } while (c != 0);
    d--;
    do {
        c = *++s;
        *++d = c;
    } while (c != 0);
    return dst;
}

int strcmp(const char* a, const char* b)
{
    const u8* p = (const u8*)a;
    const u8* q = (const u8*)b;
    u32 c;
    s32 diff;

    c = *p;
    diff = c - *q;
    if (diff != 0) {
        return diff;
    }
    {
        if (((u32)q & 3) == ((u32)p & 3)) {
            u32 off = (u32)p & 3;

            if (off != 0) {
                u32 n;

                if (c == 0) {
                    return 0;
                }
                for (n = 3 - off; n > 0; n--) {
                    c = *++p;
                    diff = c - *++q;
                    if (diff != 0) {
                        return diff;
                    }
                    if (c == 0) {
                        return 0;
                    }
                }
                p++;
                q++;
            }
            {
                const u32* wp = (const u32*)p;
                const u32* wq = (const u32*)q;
                u32 wa = *wp;
                u32 wb = *wq;

                if (!HAS_ZERO_BYTE(wa)) {
                    while (wa == wb) {
                        wa = *++wp;
                        wb = *++wq;
                        if (((wa + 0xFEFEFEFF) & 0x80808080) != 0) {
                            break;
                        }
                    }
                }
                p = (const u8*)wp;
                q = (const u8*)wq;
            }
            c = *p;
            diff = c - *q;
            if (diff != 0) {
                return diff;
            }
        }
    }
    if (c == 0) {
        return 0;
    }
    for (;;) {
        c = *++p;
        diff = c - *++q;
        if (diff != 0) {
            return diff;
        }
        if (c == 0) {
            return 0;
        }
    }
}

int strncmp(const char* a, const char* b, u32 n)
{
    const u8* p = (const u8*)a;
    const u8* q = (const u8*)b;
    u32 c;
    u32 d;

    p--;
    q--;
    n++;
    while (--n != 0) {
        c = *++p;
        d = *++q;
        if (c != d) {
            return c - d;
        }
        if (c == 0) {
            break;
        }
    }
    return 0;
}

char* strchr(const char* s, int ch)
{
    const u8* p = &((const u8*)s)[-1];
    u32 want = (u8)ch;
    u32 c;

    while ((c = *++p) != 0) {
        if (c == want) {
            return (char*)p;
        }
    }
    if (want != 0) {
        return 0;
    }
    return (char*)p;
}

char* strrchr(const char* s, int ch)
{
    const u8* last = 0;
    const u8* p = &((const u8*)s)[-1];
    u32 want = (u8)ch;
    u32 c;

    while ((c = *++p) != 0) {
        if (c == want) {
            last = p;
        }
    }
    if (last != 0) {
        return (char*)last;
    }
    if (want != 0) {
        return 0;
    }
    return (char*)p;
}

static char* strtok_empty_string = "";
static char* strtok_cursor = "";

char* strtok(char* str, const char* set)
{
    u8 table[32] = {0};
    const u8* d;
    const u8* start;
    u32 c;

    if (str != 0) {
        strtok_cursor = str;
    }
    d = (const u8*)set;
    d--;
    while ((c = *++d) != 0) {
        table[(c >> 3) & 0x1F] |= (u8)(1 << (c & 7));
    }
    d = (const u8*)strtok_cursor;
    d--;
    while ((c = *++d) != 0) {
        if (!(table[(c >> 3) & 0x1F] & (u8)(1 << (c & 7)))) {
            break;
        }
    }
    if (c == 0) {
        strtok_cursor = strtok_empty_string;
        return 0;
    }
    start = d;
    while ((c = *++d) != 0) {
        if (table[(c >> 3) & 0x1F] & (u8)(1 << (c & 7))) {
            break;
        }
    }
    if (c == 0) {
        strtok_cursor = strtok_empty_string;
    } else {
        char* t = (char*)d;

        strtok_cursor = t + 1;
        *t = 0;
    }
    return (char*)start;
}

char* strstr(const char* haystack, const char* needle)
{
    const u8* h = (const u8*)haystack;
    u32 c;
    u32 first;

    h--;
    if (needle != 0) {
        first = *(const u8*)needle;
        if (first != 0) {
            while ((c = *++h) != 0) {
                if (c == first) {
                    const u8* p = h - 1;
                    const u8* q = (const u8*)needle;
                    u32 a;
                    u32 b;

                    q--;
                    do {
                        a = *++p;
                        b = *++q;
                    } while (a == b && a != 0);
                    if (b == 0) {
                        return (char*)h;
                    }
                }
            }
            return 0;
        }
    }
    return (char*)haystack;
}
