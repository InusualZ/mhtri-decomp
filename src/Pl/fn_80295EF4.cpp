/*
 * Naming note: the symbol map spells 97 of this range's 99 functions as bare `fn_XXXXXXXX` and
 * names the other two with a mangling (`get_hit_id__Fv`) or a plain name; the map is the only
 * evidence for a name here - no `__FILE__` string covers the band (the DOL's string pool jumps from
 * `enemy_control.cpp` at 0x805A1BB8 straight to `menu_item.cpp` at 0x805CDFC8) and
 * `tools/symbols/dumpmap.py lookup` answers a `zz_XXXXXXXX_` placeholder for every in-range address
 * (checked for 0x80295EF4, 0x8029B918, 0x8029F204 and the range's other endpoints).
 *
 * Pl/fn_80295EF4.cpp - the hit/land query band of the `Pl` module.  `.text` 0x80295EF4-0x8029F3C8
 * (99 functions, 0x94D4 B).  Nothing else is claimed: the band's `.data` tables and both pools
 * (`.sdata2` 0x8079A330-0x8079A3C8, `.sdata` 0x80794B58) stay with the `auto_*` objects, and so does
 * the `.ctors` word for the static initializer `fn_80297C30` (invariant 8.4 - never claim a range the
 * object does not emit).
 *
 * The seam is unproven, and it is a `--max-bytes` cut, not a TU boundary (the brief says so and the
 * evidence agrees): the band's `.sdata2` run 0x8079A330-0x8079A3C8 is referenced from 0x80291664
 * through 0x8029F204 without a single run break, and the static initializer at 0x80297C30 (inside
 * this range) constructs arrays whose element constructors are the *previous* proposal's functions
 * (`fn_80295544` at 0x80295544) - i.e. 0x8028F66C-0x8029F3C8 and their neighbours are very likely one
 * original object.  Both halves are registered separately only because discovery cut the run; the
 * merge is requested in the outbox (`shared-file`).
 *
 * Module `Pl` - class 3 of the brief's evidence order.  Class 1 (a `__FILE__` string) and class 2 (a
 * real runtime-dump name) both fail (above).  Class 3 is conclusive: the band's callees are the Pl
 * family (`get_move_work_adrs`, `_PLW` fields, `Pl_frame_check`) and its record vocabulary is the one
 * the map already names inside the Pl band (`_HIT_W`, `LandData`, `hit_ground_comon`, `get_hit_id`,
 * `body_set`), with the registered sibling `Pl/fn_80288CEC.cpp` ending at 0x8028F66C straight before
 * it.  File name - class 4: nothing names a file, so the stem stays the map's `fn_80295EF4`, the
 * sibling class-4 pattern of `Pl/fn_80229ECC.cpp` / `Pl/fn_8024158.cpp` / `Pl/fn_80288CEC.cpp`.
 *
 * Language: C++ (mangled `get_hit_id__Fv` is defined here, and the range reaches mangled callees).
 * Flags: `cflags_pl` (the lib's, settled by `Pl/pl_act.cpp`/`pl_master.cpp`/`pl_skill.cpp`).
 *
 * RESIDUAL (this pass).  24 of the range's 99 functions are written and measured; the row is at
 * ~4 % of the range's bytes, with 16 functions byte-identical and the rest of this pass's set between
 * 92 and 99 %.  Per-symbol numbers are in `build/RMHE08/report.json` and the outbox; what is
 * deliberately NOT written, and why:
 *   * `fn_8029F204` (452 B, the hit-record initializer) reads the owner's area byte for all four
 *     owner kinds; kinds 1 and 2 land on offsets `pl.h` does not name (+0x1E1 inside an `_EQUIP`
 *     sub-record, +0x08 inside a run another lane owns), and this pass did not invent names for them.
 *   * The owner work records (`_HIT_W`'s owner, written at +0x470/+0x574/+0x8B4 by the
 *     `fn_802996B4`..`fn_802997E4` family) are not pinned: `pl.h`'s `+0x470` is an `s16` while those
 *     functions store a float triple there, so the field's owner is unresolved (the same blocker the
 *     sibling band records from the other side).
 *   * The band's switches (`fn_80299EF8`, `fn_8029A358`, `fn_8029B918`, `fn_8029E35C`, ...) are the
 *     ceiling the sibling band already recorded: every jump table in the range is zero-filled in the
 *     original DOL, so a sparse switch's case-to-body mapping is not recoverable from the image.
 *   * `.text` only is claimed.  The split does hand the unit's target object the `.ctors` word
 *     (4 B), `extab` (0x240 B) and `extabindex` (0x360 B) that bracket the range, and our object does
 *     emit the extab of the functions it defines - but the ranges are not registered here because the
 *     `.text` cut is not a TU boundary (see above) and this source states the static initializer
 *     `fn_80297C30` as an explicit function rather than a file-scope object (invariant 8.4).
 *
 * Measured residuals in the written set (best variant landed for each):
 *   * `fn_802961F8` / `fn_80296228` (95.0 / 95.71): retail's loop guard is `cmplwi r5,0` + `ble`
 *     where MWCC emits `cmpwi` + `beq` from every shape tried (`while`, `for`, `for (;;)`+break,
 *     `u32` and `s32` counts, an explicit pre-loop guard - the last one costs 12 bytes).
 *   * `fn_8029F084` (92.41): retail carries an unused accumulator (`li r5,0` then `+9` per outer
 *     iteration) that MWCC's dead-code pass removes from every source spelling tried; ours is 4 B
 *     short.  The loop shape itself (two unrolled ten-entry rows) is the one that reproduces.
 *   * `fn_802963B0` / `fn_802963FC` / `fn_802969A8` / `fn_802969D0` (98.32 / 98.32 / 99.0 / 99.0):
 *     instruction-for-instruction equal but for the operand order of one `add` in the index
 *     arithmetic, and the branch form of the range checks (`||`-chain vs `&&`-chain inversion).
 *   * `fn_80296368` (98.78), `fn_8029A140` (68.61): the same class - only the branch sense of one
 *     `fcmpo` differs.  `fn_8029A140`'s tail block is laid out in the other order (4 B).
 */

#include "types.h"
#include "nw4r/math.h"
#include "pl.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "ef/fn_800CDB2C.h"   /* PlayMode_ck (rule 2: the owner is `ef/fn_800CDB2C.cpp`) */
#include "Pl/fn_80295EF4.h"
#include "Pl/bss_pool.h"   /* the owner of the `.bss` arrays `pl_land_data` / `pl_hit_id_list` (rule 2) */

/* The pool constants this pass needs: 0.0f (`lbl_8079A330`), 1.0f (`lbl_8079A338`) and the pair
 * `lbl_8079A380` = 0.0f / `lbl_8079A3A4` = 1.0f.  They are declared, never defined (invariant 8.4). */

/* ------------------------------------------------------------------------------------------------ *
 * List scans: the registry's own id lists and the two helpers that walk a caller's array.
 * ------------------------------------------------------------------------------------------------ */

/* Whether `key` occurs in the first `count` words of `list`. */
extern "C" u32 fn_802961F8(u32 key, u32* list, s32 count)
{
    while (count > 0) {
        if (key == *list) {
            return 1;
        }
        list++;
        count--;
    }
    return 0;
}

/* The same scan over the global id array. */
extern "C" u32 fn_80296228(u32 key, s32 count)
{
    u32* list = pl_hit_id_list;

    while (count > 0) {
        if (key == *list) {
            return 1;
        }
        list++;
        count--;
    }
    return 0;
}

/* Whether `id` is *absent* from the two ten-entry halves of a caller's 20-entry id table. */
extern "C" u32 fn_8029F084(u16* ids, u16 id)
{
    s32 checked;   /* the original's running count of tested entries (its increments are +9) */
    s32 row;

    checked = 0;
    for (row = 0; row < 2; row++, ids += 10) {
        s32 j;

        for (j = 0; j < 10; j++) {
            if (id == ids[j]) {
                return 0;
            }
        }
        checked += 9;
    }
    return 1;
}

/* Empties a 20-entry id table and clears the count byte that indexes it. */
extern "C" void fn_8029F170(u16* ids, u8* count)
{
    s32 i;

    *count = 0;
    for (i = 0; i < 20; i++) {
        ids[i] = 0xFFFF;
    }
}

/* Appends `id` to the ring, wrapping the count around at 20. */
extern "C" void fn_8029F1D4(u16* ids, u8* count, u16 id)
{
    ids[*count] = id;
    (*count)++;
    if ((u8)*count >= 20) {
        *count = 0;
    }
}

/* ------------------------------------------------------------------------------------------------ *
 * The hit registry (`.bss` `lbl_806AC8A8`).
 * ------------------------------------------------------------------------------------------------ */

/* Clears the registry: both list heads, both counts, the id counter and the trailing word. */
extern "C" void fn_80299ED8(void)
{
    lbl_806AC8A8.head_0x00 = 0;
    lbl_806AC8A8.head_0x04 = 0;
    lbl_806AC8A8.count_0x08 = 0;
    lbl_806AC8A8.count_0x0A = 0;
}

/* Pushes an active hit onto the registry's first list. */
extern "C" void fn_8029EFDC(_HIT_W* hit)
{
    HitRegistry* reg = &lbl_806AC8A8;
    _HIT_W* prev;

    if (hit->active_0x05 == 0) {
        return;
    }
    prev = reg->head_0x00;
    reg->head_0x00 = hit;
    hit->next_0x00 = prev;
    reg->count_0x08++;
}

/* The same push onto the registry's second list. */
extern "C" void fn_8029F00C(_HIT_W* hit)
{
    HitRegistry* reg = &lbl_806AC8A8;
    _HIT_W* prev;

    if (hit->active_0x05 == 0) {
        return;
    }
    prev = reg->head_0x04;
    reg->head_0x04 = hit;
    hit->next_0x00 = prev;
    reg->count_0x0A++;
}

/* Records one hit's owner and its kind. */
extern "C" void fn_8029F03C(_HIT_W* self, u8 kind, void* owner)
{
    self->owner_0x10 = owner;
    self->owner_kind_0x06 = kind;
}

/* Records the second owner slot of a hit. */
extern "C" void fn_8029F048(_HIT_W* self, u8 kind, void* owner)
{
    self->owner_0x14 = owner;
    self->kind_0x07 = kind;
}

/* Hands out the next hit id, wrapping around at 60000. */
u16 get_hit_id(void)
{
    HitRegistry* reg = &lbl_806AC8A8;

    reg->next_id_0x0C++;
    if (reg->next_id_0x0C >= 0xEA60) {
        reg->next_id_0x0C = 0;
    }
    return reg->next_id_0x0C;
}

/* ------------------------------------------------------------------------------------------------ *
 * The land table (`.bss` `pl_land_data`, 15 x 0x88).
 * ------------------------------------------------------------------------------------------------ */

/* Zeroes the four vectors of one land record. */
extern "C" LandData* fn_80297D9C(LandData* self)
{
    VEC3_ctor(&self->vec_0x0C);
    VEC3_ctor(&self->vec_0x18);
    VEC3_ctor(&self->box_min_0x48);
    VEC3_ctor(&self->box_max_0x54);
    return self;
}

/* Clears the whole table. */
extern "C" void fn_8029708C(void)
{
    memset(pl_land_data, 0, 0x7F8);
}

/* ------------------------------------------------------------------------------------------------ *
 * The 0x14-byte position record.
 * ------------------------------------------------------------------------------------------------ */

/* Clears the header fields and points the record's vector up by one. */
extern "C" void fn_802977E4(PlHitPoint* self)
{
    self->field_0x00 = 0;
    self->field_0x02 = 0;
    self->field_0x03 = 0;
    self->field_0x04 = 0;
    self->field_0x06 = 0;
    self->field_0x07 = 0;
    setVector3(&self->pos_0x08, 0.0f, 1.0f, 0.0f);
}

/* Copies one position record. */
extern "C" void fn_80297BE4(PlHitPoint* dst, PlHitPoint* src)
{
    *dst = *src;
}

/* ------------------------------------------------------------------------------------------------ *
 * The land record's two 3-D grids and its AABB.
 * ------------------------------------------------------------------------------------------------ */

/* Whether `pos` lies strictly inside the record's horizontal box. */
extern "C" u32 fn_80296368(nw4r::math::VEC3* pos, LandData* land)
{
    if (!(pos->x < land->box_max_0x54.x && pos->x > land->box_min_0x48.x &&
          pos->z < land->box_max_0x54.z && pos->z > land->box_min_0x48.z)) {
        return 0;
    }
    return 1;
}

/* Whether a grid-B coordinate triple is inside the record's second grid. */
extern "C" u32 fn_802963B0(s32 x, s32 y, s32 z, LandData* land)
{
    if ((u32)x >= (u32)land->dim_x_0x74 || x < 0 || (u32)y >= (u32)land->dim_y_0x70 || y < 0 ||
        (u32)z >= (u32)land->dim_z_0x6C || z < 0) {
        return 0;
    }
    return 1;
}

/* The same range check for the record's first grid. */
extern "C" u32 fn_802963FC(s32 x, s32 y, s32 z, LandData* land)
{
    if ((u32)x >= (u32)land->dim_x_0x38 || x < 0 || (u32)y >= (u32)land->dim_y_0x34 || y < 0 ||
        (u32)z >= (u32)land->dim_z_0x30 || z < 0) {
        return 0;
    }
    return 1;
}

/* One cell of the record's second grid, indexed [x][y][z]. */
extern "C" u32 fn_802969A8(s32 x, s32 y, s32 z, LandData* land)
{
    return land->cells_0x78[(x * land->dim_y_0x70 + y) * land->dim_z_0x6C + z];
}

/* One cell of the record's first grid, indexed [x][y][z]. */
extern "C" u32 fn_802969D0(s32 x, s32 y, s32 z, LandData* land)
{
    return land->cells_0x3C[(x * land->dim_y_0x34 + y) * land->dim_z_0x30 + z];
}

/* ------------------------------------------------------------------------------------------------ *
 * The 0x3C-byte box record (`.bss` `pl_hit_box`, 10 records).
 * ------------------------------------------------------------------------------------------------ */

/* Zeroes the four vectors of one box record. */
extern "C" void* fn_80297DE8(void* self)
{
    PlHitBox* box = (PlHitBox*)self;

    VEC3_ctor(&box->vec_0x08);
    VEC3_ctor(&box->vec_0x14);
    VEC3_ctor(&box->vec_0x20);
    VEC3_ctor(&box->vec_0x2C);
    return self;
}

/* ------------------------------------------------------------------------------------------------ *
 * The two id/tile lookups at the tail of the band.
 * ------------------------------------------------------------------------------------------------ */

/* The byte `lbl_805CDC88` holds for a tile id, or 0 for an id past the table. */
extern "C" u32 fn_8029B8F4(u32 id)
{
    if ((u8)id > 0x24) {
        return 0;
    }
    return lbl_805CDC88[(u8)id];
}

/* Whether two menu/quest ids agree, or the scene is in play mode 3. */
extern "C" u32 fn_8029D6FC(u8 a, u8 b)
{
    if (a == b || PlayMode_ck() != 3) {
        return 1;
    }
    return 0;
}

/* The four-way kind `fn_8029F204`'s two floats select: 0 when neither value is usable, 1/2 by which
 * slot carries the positive value and 3 when the second slot is the unusable default. */
extern "C" u32 fn_8029A140(_HIT_W* self)
{
    if (self->value_0x50 <= -100.0f) {
        return 0;
    }
    if (self->value_0x50 > 0.0f) {
        return 1;
    }
    if (self->value_0x54 > 0.0f) {
        return 2;
    }
    if (self->value_0x54 <= -100.0f) {
        return 3;
    }
    return 2;
}
