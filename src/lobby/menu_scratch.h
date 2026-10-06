/* The record types of `lb_menu_scratch` (.bss 0x806AA8C8, 0x1C0 B; `lobby/lb_menu_scratch.cpp`, declared in
 * `lobby/lb_menu_scratch.h`), with the selection-flag bytes `lobby/fn_80219260.cpp` writes at +0x156. */
#ifndef MHTRI_LOBBY_MENU_SCRATCH_H
#define MHTRI_LOBBY_MENU_SCRATCH_H

#include "types.h"
#include "nw4r/math.h"

/* The screen work block at `lb_menu_scratch` (0x1C0 bytes in the map).  The range's table is the
 * 0x10-byte run at +0x0C (`index_0x14C` selects one of its rows), and the tail from +0x160 is the
 * 8 x 0xC vector run `fn_8021EFC8` clears. */
typedef struct LbMenuRow {
    /* +0x00 */ u32 table_0x00;    /* the row's table pointer `fn_8021EC20` hands out */
    /* +0x04 */ u32 model_0x04;    /* the row's model pointer `fn_8021EBF0` hands out */
    /* +0x08 */ u8 unused_0x08[8];
} LbMenuRow; /* size: 0x10 */

typedef struct LbMenuScratch {
    /* +0x000 */ u32 unused_0x000;
    /* +0x004 */ s16* fixed_a_0x004;  /* the -2 mode's string row */
    /* +0x008 */ u8* fixed_b_0x008;   /* the -2 mode's model row */
    /* +0x00C */ LbMenuRow rows_0x00C[20];
    /* +0x14C */ s16 index_0x14C;   /* -1 = none, -2 = the two special pointers, >= 0 = a row */
    /* +0x14E */ u8 unused_0x14E[0x02];
    /* +0x150 */ u32 timer_0x150;
    /* +0x154 */ u8 count_0x154;
    /* +0x155 */ u8 unused_0x155[0x01];
    /* +0x156 */ u8 flags_0x156[8];  /* one byte per selected slot (`lobby/fn_80219260.cpp`) */
    /* +0x15E */ u8 unused_0x15E[0x02];
    /* +0x160 */ VEC3 slots_0x160[8];
} LbMenuScratch; /* size: 0x1C0 (the map's own record) */

#endif /* MHTRI_LOBBY_MENU_SCRATCH_H */
