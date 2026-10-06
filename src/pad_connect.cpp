/*
 * `pad_connect.cpp` - the pad-connect / system-flag tail of the game-root pad file: `.text` 0x80046C80..0x80047398 (20 functions:
 * the mutex/pad-connect init `fn_80046C80`, `setSoftresetFlag`, the `system_w.field_0x30` flag setters, `screen_split_mode_ck`,
 * the 32-bit word copy helpers), `.bss` 0x806694D0..0x806694E8 (`game_mutex`), `.data` 0xD0, `.sdata` 0x8, `.sdata2` 0x18.
 *
 * Seam: the old `mh3_pad.cpp` ran 0x800408A8..0x80047398 as one proposal-capped unit; the reconciled candidate cuts it at
 * 0x80046C80 (the unit's own extab group and the `.bss` `game_mutex` object start there) and keeps `mh3_pad.cpp` for the pad / mode
 * half.  The `"mh3_pad.cpp"` `__FILE__` string stays with `mh3_pad.cpp`.
 *
 * Name: GUESS `pad_connect` - the unit opens with the pad-connect init (it constructs `game_mutex` and registers the pad tasks) and
 * holds the soft-reset / split-screen flag accessors; no `__FILE__` string or runtime-dump name covers the range.
 *
 * Status: 11 of the 20 functions have bodies (the flag accessors and the word copies); `fn_80046C80`, the pad-connect callbacks
 * and the rest (9 functions) are not decompiled.  The bodies were measured with the old unit's flags (`cflags_main`).
 */

#include "types.h"
#include "unsplit/unknown.h" /* SystemWork / system_w (rule 1/2) */
#include "mh3_pad/Screen_w.h"   /* ScreenWork / Screen_w (rule 1/2) */
#include "g3d/fn_80075DCC.h" /* fn_8007A510 (rule 2) */

/* ------------------------------------------------------------------ *
 * 0x80046EE4 - 0x800470B4  (system flags)
 * ------------------------------------------------------------------ */

extern "C" void fn_80046EE4(void)
{
    system_w.field_0x30 = 1;
}

extern "C" void fn_80046EF8(void)
{
    system_w.field_0x30 = 0;
}

extern "C" s32 fn_80046F0C(void)
{
    return system_w.field_0x30 != 0;
}

extern "C" s32 screen_split_mode_ck(void)
{
    return Screen_w.split_mode != 0;
}

extern "C" s8 fn_800470B4(void)
{
    if (Screen_w.split_mode != 0) {
        return (s8)Screen_w.split_view_no;
    }
    return -1;
}

extern "C" u8 fn_800472DC(s32 arg0)
{
    return (u8)(fn_800470B4() * 2 + arg0);
}

extern "C" void fn_80046E98(void)
{
    if (system_w.field_0x21 != 0) {
        fn_8007A510();
    }
}

/* `setSoftresetFlag__Fb` */
void setSoftresetFlag(bool flag)
{
    if (flag) {
        system_w.field_0x20 = 0;
    } else {
        system_w.field_0x20 = 1;
    }
}

/* ------------------------------------------------------------------ *
 * 0x80047234 - 0x8004726C  (32-bit word copy)
 * ------------------------------------------------------------------ */

extern "C" s32 fn_80047234(s32* src)
{
    return *src;
}

extern "C" void word_copy(s32* dst, s32* src)
{
    *dst = *src;
}

extern "C" void* word_copy_return_dst(void* dst, const void* src)
{
    word_copy((s32*)dst, (s32*)src);
    return dst;
}


/* This unit's own `.bss` (`splits.txt` `.bss 0x806694D0..0x806694E8`).  Defined at the foot of the file, after every use. */
u8 game_mutex[0x18];                   /* +0x806694D0: an OSMutex, initialised by `fn_80046C80` (GUESS name) */
