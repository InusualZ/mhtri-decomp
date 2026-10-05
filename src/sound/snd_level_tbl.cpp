/* sound/snd_level_tbl.cpp - the level-table lookup
 *
 * `.text` 0x800E8E48..0x800E8E60, 1 function written (the rest of the range is not decompiled yet).
 * Name is a GUESS: one function indexing the 128-entry level table `lbl_80597E20` (`.data` 0x200).
 * Each function keeps the `#pragma` state it had in its retired source.
 */

#include "types.h"

extern u32 lbl_80597E20[128];       /* the level table fn_800E8E48 indexes */

extern "C" {
u32 fn_800E8E48(s16 idx);
}

#pragma peephole off

/* One entry of the level table. */
extern "C" u32 fn_800E8E48(s16 idx)
{
    return lbl_80597E20[idx];
}
