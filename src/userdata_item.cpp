/*
 * `userdata_item.cpp` - the user-data / item-table / sprite-transform band that follows the face render unit: `.text`
 * 0x80048964..0x8004C9A0 (87 functions: `fn_80048964`, the second GX FIFO writer family, `drawSpr2TF`, `subTransSet*`, the user-data
 * and equipment helpers `arena_userdata_apply`, `decodePlayerCard`, `userdata_gunner_ck`, `userdata_equip_item_slots_get`, and the
 * `{id, value}` item-table helpers `item_count_find`, `item_pair_index_find`, `item_take`, `color_rgba_copy`).
 *
 * Seam: the old `fn_80047398.cpp` held 0x80047398..0x8004C9A0 as one unproven run; the reconciled candidate cuts it at
 * 0x80048964 (the extab/extabindex records and the `.bss` 0x806699B8 object the sub-transform code owns start there) and keeps
 * `fn_80047398.cpp` for the face render half.  `.bss` 0x5B0 (`sub_trans_buf`, `sub_trans_state`), `.sbss` 0xC (`bg_tex_disp_func`,
 * `lobby_world_block`), `.sdata2` 0x18.
 *
 * Name: GUESS `userdata_item` - the named functions in the range are user-data (`arena_userdata_apply`, `userdata_gunner_ck`,
 * `userdata_equip_item_slots_get`, `decodePlayerCard`) and item-table (`item_*`) helpers; no `__FILE__` string or runtime-dump name
 * exists for the range (it replaces the placeholder stem `fn_80048964`).  Module: the game-root band (`main` lib, `cflags_main`).
 *
 * Status: the small table helpers and the FIFO writers have bodies; the rest of the range (87 functions) is not decompiled.
 * NAMES. GUESS: `userdata_pouch_size`, `userdata_pouch_get`, `item_slots_free_count`
 *   GUESS (from each body and its callers): userdata_opening_seen_set, userdata_item_count_total
 */

#include "types.h"
#include "id_value.h"
#include "gx.h"

#include "EXI/GXSetTexCoordGen2.h" /* the SDK function the pipe helper tail-calls (rule 2) */

/* --- the GX pipe writers (second family) and the small table helpers ----------------------------------- */

/* Empty stub. */
extern "C" void fn_80048CA0(void)
{
}

/* Two float vertices (second writer family). */
extern "C" void fn_80048CA4(f32 x, f32 y)
{
    GXWGFifo.f32 = x;
    GXWGFifo.f32 = y;
}

/* One raw word. */
extern "C" void fn_80048CB4(u32 value)
{
    GXWGFifo.u32 = value;
}

/* A 2D position pair (second writer family). */
extern "C" void fn_80048CC0(s32 x, s32 y)
{
    GXWGFifo.s16 = x;
    GXWGFifo.s16 = y;
}

/* The unit's fixed texture-coordinate generator (second writer family). */
extern "C" void fn_80048CD8(u32 a, u32 b, u32 c, u32 d)
{
    GXSetTexCoordGen2(a, b, c, d, 0, 0x7D);
}

/* --- the small table helpers ----------------------------------------------------------------------- */

/* The record size for one menu kind. */
extern "C" u32 userdata_pouch_size(u32 kind)
{
    return kind == 1 ? 0x20 : 0x18;
}

/* The record block for one layout kind. */
extern "C" u8* userdata_pouch_get(u8* base, u32 kind)
{
    if (kind == 1) {
        return base + 0x100;
    }
    return base + 0xA0;
}

/* Whether the given id belongs to the face-record family. */
extern "C" u32 fn_8004B034(u16 id)
{
    if ((u32)(id - 0x1B6) <= 1) {
        return 1;
    }
    if (id == 0) {
        return 1;
    }
    if (id == 0xDF) {
        return 1;
    }
    return 0;
}

/* Looks up an id in a 4-byte `{id, value}` table; 0 when it is absent. */
extern "C" s32 item_count_find(u16 id, const IdValue* table, s32 count)
{
    s32 value = 0;

    if (id != 0 && count > 0) {
        do {
            if (table->id == id) {
                value = table->value;
                break;
            }
            table++;
        } while (--count);
    }
    return value;
}

/* The index of an id in a 4-byte `{id, value}` table, or -1. */
extern "C" s32 item_pair_index_find(u16 id, const IdValue* table, s32 count)
{
    s32 index = 0;

    if (count > 0) {
        do {
            if (table->id == id) {
                return index;
            }
            table++;
            index++;
        } while (--count);
    }
    return -1;
}

/* Clears one record and reports whether it had been in use. */
extern "C" u32 fn_8004BD30(IdValue* entry)
{
    entry->value = 0;
    if (entry->id != 0) {
        entry->id = 0;
        return 1;
    }
    return 0;
}

/* Counts the free entries of a 4-byte `{id, value}` table. */
extern "C" u32 item_slots_free_count(const IdValue* table, s32 count)
{
    u32 free = 0;

    if (count > 0) {
        do {
            if (table->id == 0) {
                free++;
            }
            table++;
        } while (--count);
    }
    return free;
}

/* Copies one 4-byte RGBA colour byte by byte. */
extern "C" void color_rgba_copy(u8* dst, const u8* src)
{
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
    dst[3] = src[3];
}

/* The hook `bg_tex_disp_func` holds: `fn_80046F28` (mh3_pad.cpp) stores the function, `fn_800492A4` calls it.  The map
 * gives the object 8 bytes; only the first word is referenced (GUESS: the second is the hook's argument). */
struct BgTexDispHook {
    /* +0x00 */ void (*func)(void);
    /* +0x04 */ u32 unused_0x04;
}; /* size: 0x8 */

/* This unit's own `.bss` (`splits.txt` `.bss 0x806699B8..0x80669F68`) and `.sbss` (`0x80794878..0x80794884`), in
 * address order.  Defined at the foot of the file, after every use.  `lobby_world_block` is the cache
 * `get_userdata()` (0x8004D120) and the user-data init `fn_800497B4`/`fn_800498EC` store the 0x6000-byte
 * user-data block in; the block belongs to the user-data subsystem this band and `fn_8004C9A0`/`fn_8004CAD8` share. */
u8 sub_trans_buf[0x580];               /* +0x806699B8: the sub-transform buffer `subTransSetPrio` fills (GUESS name) */
u8 sub_trans_state[0x30];              /* +0x80669F38: the sub-transform stack state (GUESS name) */
BgTexDispHook bg_tex_disp_func;        /* +0x80794878 */
u8* lobby_world_block;                 /* +0x80794880: the cached user-data pointer */
