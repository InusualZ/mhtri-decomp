/*
 * Pl/fn_802693C4.cpp - the player part/motion cluster (proposal 802693C4).
 *
 * `.text` 0x802693C4-0x8026BA1C (63 functions, 9816 B) and the exception runs the link order places
 * this TU between the two neighbours' - extab 0x80012554-0x8001265C and extabindex
 * 0x8002F6E8-0x8002F808 (the gaps between `Pl/fn_80262940.cpp`'s and `Pl/pl_master.cpp`'s).
 * The range's first record in the unclaimed extabindex run is `fn_802695A4`, so the four functions
 * before it may belong to `Pl/fn_80262940.cpp` instead: the seam is recorded as open on both sides.
 *
 * Home (evidence order): 1. no `__FILE__` string covers the band - the `.data` source-name pool
 * carries none between `enemy_control.cpp` @0x805A1BB8 and `menu_item.cpp` @0x805CDFC8, and this
 * range's own data references are the `.sdata2` float pool, the `.data` tables at
 * 0x805BAB58/0x805BAB74/0x805C0C98/0x805C1584 and 0x805C5F90, and the `.sbss` global at 0x80794B28;
 * 2. `python tools/symbols/dumpmap.py lookup <addr>` answers only `zz_XXXXXXXX_` placeholders for 58
 * of the 63 addresses; 3. the code is the Pl module - every actor parameter is the `_PLW` the
 * sibling units take, the table it walks is `lbl_80794B28` (`_PLGLOBAL`) and the callees are
 * `Pl_chr_set_attr`/`Pl_chr_setX`/`Pl_frame_check`/`Get_motion_no`/`pl_get_joint_wpos`; 4. the stem
 * is the map's `fn_802693C4`.
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/symedit.py range 0x802693C4 0x8026BA1C`; 58 of the 63 entries are bare `fn_`
 * placeholders, the other 5 are the manglings this unit defines).
 *
 * Language: C++ - the unit's own symbols carry manglings (`set_com_motion_type__FUc`,
 * `Get_motion_no__FP4_PLW`, `Pl_frame_check__FP4_PLWUlff`, `Pl_chr_setX__FP4_PLWUsll`,
 * `Pl_chr_set_attr__FP4_PLWUsllUl`) and its functions reach class members (`MHchar::get_joint_wpos`).
 *
 * Flags: `cflags_pl` (`-O3 -inline noauto -opt nopeephole -Cpp_exceptions on`, mw_version Wii/1.0) -
 * the set every sibling Pl unit measures with.
 *
 * Residual (the range is worked in address order; the parts of it without a body yet):
 *   * 0x802695A4 (1012 B) - the equipment-to-part dispatcher: reads `_PLOBJ->group[3][3..5]`, packs
 *     `(type << 24) | value` into the same words and arms `fn_802693C4` per part.
 *   * 0x80269AC4 (788 B) - the player move start-up (`memset(self + 0xB8, 0, 0x84)`, the new rig at
 *     `__nw__FUl(0x1560)`, then the per-part table setup).
 *   * 0x80269DD8 / 0x80269E8C / 0x80269EC8 / 0x80269F04 / 0x80269F88 - the rig's constructor, the two
 *     element constructors (`__construct_array`, vtables 0x805BAB58/0x805BAB74) and its destructor;
 *     they need the two 0x90/0x110-byte element types modelled first.
 *   * 0x8026A0D4 `Pl_chr_set_attr` (164 B) and 0x8026A178 (172 B) / 0x8026A248 `Pl_chr_setX` (116 B):
 *     both funnel into `fn_800E12CC`, whose argument order is still open.
 *   * 0x8026A3A8 / 0x8026A3EC / 0x8026A5A4 / 0x8026A738 / 0x8026A864 / 0x8026A954 / 0x8026AA9C /
 *     0x8026ABB0 / 0x8026AC2C / 0x8026AE04 / 0x8026AE5C / 0x8026AF08 (2604 B) / 0x8026B934 /
 *     0x8026B99C - the motion/act bodies of the range; unwritten.
 */

#include "types.h"
#include "pl.h"
#include "Pl/fn_802693C4.h"
#include "Pl/fn_80224AC4.h"
#include "Pl/pl_skill.h"
#include "sound/fn_800DD1F0.h"
#include "unsplit/Pl.h"

/* The six symbols the map spells mangled are defined at C++ scope so the front-end reproduces the
 * map's names (rules 9 and 50); the rest are the map's unmangled `fn_` stems and are `extern "C"`. */

void set_com_motion_type(u8 type)
{
    if (lbl_80794B28 != NULL) {
        lbl_80794B28->com_motion_type = type;
    }
}

u16 Get_motion_no(_PLW* self)
{
    return ((PlSeRig*)self->physics_0x13C)->models_0x004[0].model.motion_no_0x50;
}

/* The two flags are the callers' (`frame` is masked to 16 bits); the body passes a constant 0/1 to
 * the model layer in their place, which is why the two floats are unused here. */
u32 Pl_frame_check(_PLW* self, u32 frame, f32 a, f32 b)
{
    return fn_800E16DC(&((PlSeRig*)self->physics_0x13C)->models_0x004[0].model, (u16)frame, 0);
}

void pl_get_joint_wpos(_PLW* self, u32 joint, nw4r::math::VEC3* out)
{
    ((PlSeRig*)self->physics_0x13C)->models_0x004[0].model.get_joint_wpos(joint, out);
}

extern "C" {

/* Arms one part of one player's object record: 0 when the part is already armed (its state is 1) or
 * was consumed (> 1), 1 after the state moves to 2 and the part's gate is cleared. */
u32 fn_802693C4(s32 player, u8 part, s32 value)
{
    if (lbl_80794B28 == NULL) {
        return 0;
    }
    _PLOBJ* obj;
    _PLWORK* work;

    obj = &lbl_80794B28->table[player];
    if (obj->state[part] > 1) {
        return 0;
    }
    if (obj->group[3][part] == value) {
        obj->state[part] = 1;
        return 0;
    }
    obj->part_flag_0x0B[part] = 0;
    obj->group[0][part] = value;
    obj->state[part] = 2;
    work = &lbl_80794B28->objects[player];
    if (part < 7) {
        ((PlSeRig*)work->model_0x13C)->field_0x9C0[part] = 0;
    } else {
        ((PlSeRig*)work->model_0x13C)->field_0x9C7[part - 7] = 0;
    }
    return 1;
}

/* Consumes one part: 0 when the part already carries this value, 1 after the flag is set, the value
 * stored and the state moved to 2; a consumed part (> 1) is reported as 0. */
u32 fn_80269474(s32 player, u8 part, s32 value)
{
    _PLGLOBAL* g = lbl_80794B28;
    _PLOBJ* obj;

    if (g == NULL) {
        return 0;
    }
    obj = &g->table[player];
    if (obj->group[3][part] == value) {
        return 0;
    }
    if (obj->state[part] > 1) {
        return 0;
    }
    obj->part_flag_0x0B[part] = 1;
    obj->group[0][part] = value;
    obj->state[part] = 2;
    ((PlSeRig*)lbl_80794B28->objects[player].model_0x13C)->field_0x9C0[part] = 0;
    return 1;
}

/* The value stored for one part of the player the work record names, or -1 with no global or no
 * work record. */
s32 fn_80269508(_PLW* self, s32 part)
{
    if (self == NULL) {
        return -1;
    }
    if (lbl_80794B28 == NULL) {
        return -1;
    }
    return lbl_80794B28->table[self->chunk_ofs].group[3][part];
}

/* 0 when the part already carries this value, 1 while it is still free, -1 once it is consumed. */
s32 fn_8026954C(s32 player, u8 part, s32 value)
{
    if (lbl_80794B28 == NULL) {
        return -1;
    }
    _PLOBJ* obj = &lbl_80794B28->table[player];
    if (obj->group[3][part] == value) {
        return 0;
    }
    if (obj->state[part] <= 1) {
        return 1;
    }
    return -1;
}

/* Returns 0 with no global, otherwise the global's common motion type. */
u8 fn_802699AC(void)
{
    if (lbl_80794B28 == NULL) {
        return 0;
    }
    return lbl_80794B28->com_motion_type;
}

/* Clears one player's object record: for every part, a part that is mid-load (state 4 or 5) drops
 * its load request, then the state, the six value words and finally the player's slot byte go to 0
 * (the state and value words to -1). */
void fn_802699C8(u8 player)
{
    _PLGLOBAL* g = lbl_80794B28;
    _PLOBJ* obj;
    s32 i;

    if (g == NULL) {
        return;
    }
    obj = &g->table[player];
    for (i = 0; i < 7; i++) {
        if ((u32)(obj->state[i] - 4) <= 1) {
            g->loaded_0x200[i] = 0;
        }
        obj->state[i] = 0;
        obj->group[0][i] = -1;
        obj->group[1][i] = -1;
        obj->group[2][i] = -1;
        obj->group[3][i] = -1;
        obj->group[4][i] = -1;
        obj->group[5][i] = -1;
    }
    for (; i < 11; i++) {
        if ((u32)(obj->state[i] - 4) <= 1) {
            g->loaded_0x200[i] = 0;
        }
        obj->state[i] = 0;
        obj->group[0][i] = -1;
        obj->group[1][i] = -1;
        obj->group[2][i] = -1;
        obj->group[3][i] = -1;
        obj->group[4][i] = -1;
        obj->group[5][i] = -1;
    }
    g->slot_state[player] = 0;
}

/* The motion-number table lookup for the 0-999 range: row `id / 100`, entry `id % 100`. */
s16 fn_8026A00C(u16 id)
{
    if (id >= 1000) {
        return 0;
    }
    return lbl_805C0C98[id / 100][id % 100];
}

/* The motion-number table lookup for the 1000+ range: two levels, the first keyed by the work
 * record's own byte, the second by `(id - 1000) / 100`. */
s16 fn_8026A068(_PLW* self, u16 id)
{
    u16 index = (u16)(id - 1000);
    u8 kind = self->field_0x002;
    s16* row = lbl_805C1584[kind][index / 100];

    if (row == NULL) {
        return 0;
    }
    return row[index % 100];
}

/* The character attribute setter's argument shuffle: the motion word is narrowed and the fifth
 * argument is left 0. */
void fn_8026A224(_PLW* self, u16 motion, s32 a, s32 b)
{
    Pl_chr_set_attr(self, motion, a, b, 0);
}

/* The character motion setter's argument shuffle (`fn_8026A178` takes a sixth, always-0 argument). */
void fn_8026A230(_PLW* self, s32 a, u16 motion, s32 b, s32 c)
{
    fn_8026A178(self, a, motion, b, c, 0);
}

/* The model block's per-index value setter. */
void fn_8026A23C(_PLW* self, s32 index, f32 value)
{
    fn_800E26B4(&((PlSeRig*)self->physics_0x13C)->models_0x004[0].model, index, value);
}

/* Hands the model layer the scale the work record carries. */
void fn_8026A2BC(_PLW* self)
{
    fn_800E1640(&((PlSeRig*)self->physics_0x13C)->models_0x004[0].model, self->field_0x354);
}

void fn_8026A2D0(_PLW* self, u8 value)
{
    ((PlSeRig*)self->physics_0x13C)->models_0x004[0].model.field_0xF1 = value;
}

void fn_8026A2DC(_PLW* self)
{
    ((PlSeRig*)self->physics_0x13C)->models_0x004[0].model.field_0xF1 = 0;
}

void fn_8026A2EC(_PLW* self, u8 value)
{
    ((PlSeRig*)self->physics_0x13C)->models_0x004[0].model.field_0xF2 = value;
}

void fn_8026A2F8(_PLW* self)
{
    ((PlSeRig*)self->physics_0x13C)->models_0x004[0].model.field_0xF2 = 0;
}

/* The same frame check as `Pl_frame_check`, with the model layer's second flag set. */
u32 fn_8026A328(_PLW* self, u16 frame, f32 a, f32 b)
{
    return fn_800E16DC(&((PlSeRig*)self->physics_0x13C)->models_0x004[0].model, frame, 1);
}

/* The model layer's frame reset. */
u32 fn_8026A33C(_PLW* self)
{
    return fn_800E2198(&((PlSeRig*)self->physics_0x13C)->models_0x004[0].model, 0);
}

f32 fn_8026A34C(_PLW* self)
{
    return ((PlSeRig*)self->physics_0x13C)->models_0x004[0].model.field_0x74;
}

f32 fn_8026A358(_PLW* self)
{
    return ((PlSeRig*)self->physics_0x13C)->models_0x004[0].model.field_0xA4;
}

/* Whether the model's blend value is above zero. */
u32 fn_8026A364(_PLW* self)
{
    return !(((PlSeRig*)self->physics_0x13C)->models_0x004[0].model.field_0xBC < lbl_8079A000);
}

/* The named joint's world position, through the model. */
void fn_8026A394(_PLW* self, s32 joint, nw4r::math::MTX34* out)
{
    fn_800E0A14(&((PlSeRig*)self->physics_0x13C)->models_0x004[0].model, joint, out);
}

/* The work record's actor-mode byte. */
u8 fn_8026A3A0(_PLW* self)
{
    return self->field_0x18;
}

/* The three motion integrators: position += velocity, velocity += acceleration, and both zeroed. */
void fn_8026A4E4(_PLW* self)
{
    self->motion_pos_0x3C += self->motion_vel_0x78;
    self->motion_pos_0x40 += self->motion_vel_0x7C;
    self->motion_pos_0x44 += self->motion_vel_0x80;
}

void fn_8026A518(_PLW* self)
{
    self->motion_vel_0x78 = self->motion_vel_0x78 + self->motion_acc_0x84;
    self->motion_vel_0x7C = self->motion_vel_0x7C + self->motion_acc_0x88;
    self->motion_vel_0x80 = self->motion_vel_0x80 + self->motion_acc_0x8C;
    self->motion_pos_0x3C += self->motion_vel_0x78;
    self->motion_pos_0x40 += self->motion_vel_0x7C;
    self->motion_pos_0x44 += self->motion_vel_0x80;
}

void fn_8026A570(_PLW* self)
{
    self->motion_acc_0x8C = lbl_8079A000;
    self->motion_acc_0x88 = lbl_8079A000;
    self->motion_acc_0x84 = lbl_8079A000;
    self->motion_vel_0x80 = lbl_8079A000;
    self->motion_vel_0x7C = lbl_8079A000;
    self->motion_vel_0x78 = lbl_8079A000;
}

void fn_8026A590(_PLW* self)
{
    self->motion_acc_0x8C = lbl_8079A000;
    self->motion_acc_0x88 = lbl_8079A000;
    self->motion_acc_0x84 = lbl_8079A000;
}

/* The three per-id flag words (128 ids each at +0xE0 and +0xF0, one word at +0xDC): set, test and
 * clear. */
void fn_8026A618(_PLW* self, s32 id)
{
    self->id_flags_0xE0[id / 32] |= 1 << (id & 31);
}

u32 fn_8026A644(_PLW* self, s32 id)
{
    return (self->id_flags_0xE0[id / 32] & (1 << (id & 31))) != 0;
}

void fn_8026A678(_PLW* self, s32 id)
{
    self->id_flags_0xF0[id / 32] |= 1 << (id & 31);
}

s32 fn_8026A6A4(_PLW* self, s32 id)
{
    return (self->id_flags_0xF0[id / 32] & (1 << (id & 31))) != 0;
}

void fn_8026A6D8(_PLW* self, s32 id)
{
    self->id_flags_0xDC |= 1 << (id & 31);
}

s32 fn_8026A6F4(_PLW* self, s32 id)
{
    return (self->id_flags_0xDC & (1 << (id & 31))) != 0;
}

void fn_8026A718(_PLW* self, s32 id)
{
    self->id_flags_0xDC &= ~(1 << (id & 31));
}

/* Whether the work record's second flag bit is set. */
u32 fn_8026BA04(_PLW* self)
{
    return (self->field_0x134 & 2) != 0;
}

} /* extern "C" */
