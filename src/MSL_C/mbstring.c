/*
 * MSL_C/mbstring.c - the multibyte string conversions: `mbtowc` dispatch, the C-locale one-byte converters, `mbstowcs`
 *    and `wcstombs`.
 *
 * RANGE. .text 0x8045B3A0..0x8045B598 (5 functions in the map, 0x1F8 B).
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi`; `#pragma use_lmw_stmw on` (the loops save r26..r31 with stmw).
 * NAMES. file name GUESS (MSL `mbstring`); `mbtowc` (map row fn_8045B3A0) is a GUESS, `__mbtowc_noconv` (fn_8045B3B8) is a
 *    GUESS, `mbstowcs` (fn_8045B420) is a GUESS.
 * EVIDENCE. `wcstombs` and `mbstowcs` read the locale record's +0x38 table; `mbtowc` jumps through its +0x20 slot.
 * RESIDUALS. none known.
 * SHAPES. the converters dispatch through the active locale's function table.
 */
#include "MSL_C/mbstring.h"
#include "MSL_C/locale.h"
#include "MSL/strlen.h"
#include "MSL_C/string.h"

#pragma use_lmw_stmw on

s32 mbtowc(u16* wide, const char* bytes, u32 count)
{
    return _current_locale.ctype->mbtowc_fn(wide, bytes, count, _current_locale.ctype);
}

s32 __mbtowc_noconv(u16* wide, const char* bytes, u32 count)
{
    if (bytes == NULL) {
        return 0;
    }
    if (count == 0) {
        return -1;
    }
    if (wide != NULL) {
        *wide = *(u8*)bytes;
    }
    if (*bytes == 0) {
        return 0;
    }
    return 1;
}

s32 __wctomb_noconv(char* bytes, char wide)
{
    if (bytes == NULL) {
        return 0;
    }
    *bytes = wide;
    return 1;
}

u32 mbstowcs(u16* wide, const char* bytes, u32 count)
{
    u32 written;
    s32 remaining = strlen(bytes);
    s32 consumed;

    if (wide != NULL) {
        for (written = 0; written < count; written++) {
            if (*bytes != 0) {
                consumed = _current_locale.ctype->mbtowc_fn(wide++, bytes, remaining, _current_locale.ctype);
                if (consumed > 0) {
                    bytes += consumed;
                    remaining -= consumed;
                } else {
                    return -1;
                }
            } else {
                *wide = 0;
                break;
            }
        }
    } else {
        written = 0;
    }
    return written;
}

u32 wcstombs(char* bytes, const u16* wide, u32 count)
{
    s32 length;
    u32 written = 0;
    char buffer[4];
    const u16* source;

    if (bytes == NULL || wide == NULL) {
        return 0;
    }
    source = wide;
    while (written <= count) {
        if (*source == 0) {
            bytes[written] = 0;
            break;
        }
        length = _current_locale.ctype->wctomb_fn(buffer, *source++, _current_locale.ctype);
        if (written + length > count) {
            break;
        }
        strncpy(bytes + written, buffer, length);
        written += length;
    }
    return written;
}
