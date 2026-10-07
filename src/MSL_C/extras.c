/*
 * MSL_C/extras.c - the MSL `extras` string routines: `stricmp`, `strupr` and a third case-insensitive helper.
 *
 * RANGE. .text 0x804681CC..0x80468358 (3 functions in the map, 0x18C B).
 * FLAGS. the `OS` lib's `cflags_os`.
 * NAMES. file name GUESS (MSL `extras`); `wcsnicmp` names the unnamed third row (GUESS: it is a bounded `stricmp` over the wide
 *    lower table).
 * EVIDENCE. family only; all three read the locale class table `.data` 0x8060EDB0; no data of their own; the TRK
 *    access stubs follow.
 * RESIDUALS. COARSE; `stricmp` and `strupr` differ in where the sign extension of the second character and the stored byte
 *    sit (same instruction count).
 * SHAPES. the case maps are read through `_current_locale.ctype`; the inline `tolower`/`toupper` return the
 *    character unchanged outside 0..255.
 */
#include "MSL_C/locale.h"

static inline int out_of_range(int c)
{
    return c < 0 || c > 255;
}

static inline int tolower_c(int c)
{
    return out_of_range(c) ? c : (int)_current_locale.ctype->lower_map[c];
}

static inline int toupper_c(int c)
{
    return out_of_range(c) ? c : (int)_current_locale.ctype->upper_map[c];
}

static inline u16 towlower_c(u16 c)
{
    return (c >= 256) ? c : _current_locale.ctype->wlower_map[c];
}

int stricmp(const char* a, const char* b)
{
    int c1;
    int c2;

    do {
        c1 = tolower_c(*a++);
        c2 = tolower_c(*b++);
        if (c1 < c2) {
            return -1;
        }
        if (c1 > c2) {
            return 1;
        }
    } while (c1 != 0);
    return 0;
}

char* strupr(char* s)
{
    char* p = s;
    char c;

    while ((c = *p) != 0) {
        int ic = c;

        *p++ = toupper_c(ic);
    }
    return s;
}

int wcsnicmp(const u16* a, const u16* b, u32 n)
{
    u32 c1;
    u32 c2;
    u32 i;

    for (i = 0; i < n; i++) {
        c1 = towlower_c(*a++);
        c2 = towlower_c(*b++);
        if (c1 < c2) {
            return -1;
        }
        if (c1 > c2) {
            return 1;
        }
        if (c1 == 0) {
            return 0;
        }
    }
    return 0;
}
