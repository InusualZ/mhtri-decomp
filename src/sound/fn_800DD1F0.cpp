/* sound/fn_800DD1F0.cpp - the SE (`se_w`) request cluster's tail and the `MHchar` model class,
 * .text 0x800DD1F0..0x800E3CBC (125 symbols, 0x6ACC bytes).
 *
 * rule 7 deferred: the symbol map has only fn_XXXXXXXX for this range (checked with symedit: the
 * unsplit functions of the range are bare `.text` entries in config/RMHE08/symbols.txt; the map's real
 * names - the `* _se_req` helpers and the `MHchar::` methods - are used verbatim below)
 *
 * Registered once at its final home from proposal/800DD1F0_fn_800DD1F0.cpp.  The module is `sound`:
 * the two bracketing registered units (sound/fn_800D7F54.cpp, sound/fn_800DCFEC.c) are both `sound`
 * and include/unsplit/sound.h is the band that declares this range's symbols for its consumers.
 * The language is C++: the map carries C++ manglings (`shell_se_req__FP5_se_wPQ34nw4r4math4VEC3UcUl`,
 * `move__6MHcharFUs`) and the range manipulates `nw4r::math` types by value.
 *
 * The name is the map's `fn_` stem (evidence class 4): no `__FILE__` string names this range, and the
 * range is plainly two original TUs merged by the discovery's `--max-bytes` cap - the SE request
 * helpers/cluster (0x800DD1F0..0x800E0560) and the `MHchar` model class (0x800E0560..0x800E3CBC,
 * tudiscover's 71-function match set).  Neighbours (sound/fn_800D7F54.cpp, sound/fn_800DCFEC.c) use
 * the same stem scheme, so the stem fits the siblings.
 *
 * What is reconstructed here: filled in per function as it is measured.
 */

#include "types.h"
#include "mh3_pad.h" /* the owner header (rule 2) */
#include "nw4r/math.h"
#include "sound/se.h"

/* The range's codegen carries the non-record forms the peephole pass would fold (`extsb` before a
 * compare and the `clrlwi` narrowing of the byte arguments), so the original TU was peephole-off - the
 * same per-unit pragma the SE cluster next door (sound/fn_800D7F54.cpp) carries. */
#pragma peephole off

/* --- the SE request layer's tail; `_PLW::field_0xAFC` is the `_se_w` the request lands in --- */

/* Non-positional SE: bank 46, id 15. */
extern "C" s32 fn_800DD38C(void)
{
    return fn_800DBB78(46, 15);
}

SeSlot* st_ice_se_req(nw4r::math::VEC3* pos)
{
    return fn_800DA72C(46, 18, pos);
}

SeSlot* st_ice_break_se_req(nw4r::math::VEC3* pos)
{
    return fn_800DA72C(46, 19, pos);
}

extern "C" SeSlot* fn_800DD504(nw4r::math::VEC3* pos)
{
    return fn_800DA72C(46, 64, pos);
}

extern "C" SeSlot* fn_800DD514(nw4r::math::VEC3* pos)
{
    return fn_800DA72C(46, 13, pos);
}

extern "C" void fn_800DD524(_PLW* self)
{
    _se_w* work = self->field_0xAFC;
    if (work) {
        return fn_800D9CC8(work, 0, 3, 3);
    }
}

extern "C" void fn_800DD544(_PLW* self, u32 flag)
{
    _se_w* work = self->field_0xAFC;
    if (!work) {
        return;
    }
    s32 id = ((u8)flag == 0) ? 1 : 2;
    return fn_800D9CC8(work, id, 3, 3);
}

extern "C" void fn_800DD574(_PLW* self, u32 arg)
{
    _se_w* work = self->field_0xAFC;
    if (!work) {
        return;
    }
    s32 id = 0;
    switch ((u8)arg) {
    case 0:
        id = 7;
        break;
    case 1:
        id = 8;
        break;
    case 2:
        id = 9;
        break;
    }
    return fn_800D9CC8(work, id, 3, 3);
}

extern "C" void fn_800DD5CC(_PLW* self)
{
    _se_w* work = self->field_0xAFC;
    if (work) {
        return fn_800D9CC8(work, 7, 3, 3);
    }
}

extern "C" void fn_800DD5EC(_PLW* self)
{
    _se_w* work = self->field_0xAFC;
    if (work) {
        return fn_800D9CC8(work, 0, 3, 3);
    }
}

extern "C" void fn_800DD60C(_PLW* self)
{
    _se_w* work = self->field_0xAFC;
    if (work) {
        return fn_800D9CC8(work, 2, 3, 3);
    }
}

extern "C" void fn_800DD62C(_PLW* self, nw4r::math::VEC3* pos)
{
    _se_w* work = self->field_0xAFC;
    if (work) {
        s32 id = (fn_800D8D8C(pos) == 1) ? 18 : 8;
        se_req_pos_ps(work, id, ((work->field_0x0C + 1) << 24) | 2, pos);
    }
}

extern "C" void fn_800DD69C(_PLW* self, nw4r::math::VEC3* pos)
{
    _se_w* work = self->field_0xAFC;
    if (work) {
        s32 id = (fn_800D8D8C(pos) == 1) ? 14 : 13;
        se_req_pos_ps(work, id, 2, pos);
    }
}

extern "C" void fn_800DD700(_PLW* self, nw4r::math::VEC3* pos)
{
    _se_w* work = self->field_0xAFC;
    if (work) {
        s32 id = (fn_800D8D8C(pos) == 1) ? 17 : 7;
        se_req_pos_ps(work, id, ((work->field_0x0C + 1) << 24) | 2, pos);
    }
}

extern "C" void fn_800DD770(_PLW* self, nw4r::math::VEC3* pos)
{
    _se_w* work = self->field_0xAFC;
    if (work) {
        s32 id = (fn_800D8D8C(pos) == 1) ? 17 : 7;
        se_req_pos_ps(work, id, ((work->field_0x0C + 1) << 24) | 2, pos);
    }
}

extern "C" void fn_800DD7E0(_PLW* self, nw4r::math::VEC3* pos, s8 flag)
{
    _se_w* work = self->field_0xAFC;
    if (!work) {
        return;
    }
    s32 id;
    switch (flag) {
    case 0:
        if (fn_800D8D8C(pos) == 1) {
            id = 12;
        } else {
            id = 10;
        }
        break;
    case 1:
        if (fn_800D8D8C(pos) == 1) {
            id = 13;
        } else {
            id = 11;
        }
        break;
    case -1:
        id = 0;
        break;
    default:
        return;
    }
    se_req_pos_ps(work, id, 2, pos);
}

/* --- the model class's simple accessors and the formation-position helpers (all `MHchar`) --- */

#include "sound/mhchar.h"

/* Copy `src` into the model's +0xDC vector (the second element of the +0xD4 float block). */
extern "C" void fn_800E0808(MHchar* self, nw4r::math::VEC3* src)
{
    if (src) {
        copyVec3((nw4r::math::VEC3*)&self->field_0xD4[2], src);
    }
}

/* Store the model's +0x3C word. */
extern "C" void fn_800E090C(MHchar* self, u32 value)
{
    self->field_0x3C = value;
}

/* Copy `src` into the model's +0x10 vector. */
extern "C" void fn_800E09D0(MHchar* self, nw4r::math::VEC3* src)
{
    copyVec3(&self->field_0x10, src);
}

/* Broadcast the pool constant over the model's +0x10 vector. */
extern "C" void fn_800E09D8(MHchar* self)
{
    f32 c = lbl_80796438;
    self->field_0x10.x = c;
    self->field_0x10.y = c;
    self->field_0x10.z = c;
}

/* Read the model's +0xEC word. */
extern "C" s32 fn_800E0A8C(MHchar* self)
{
    return self->field_0xEC;
}

/* Store the model's +0x38 word. */
extern "C" void fn_800E0BE8(MHchar* self, s32 value)
{
    self->field_0x38 = value;
}

/* Read the model's +0x2C word. */
extern "C" s32 fn_800E0DD8(MHchar* self)
{
    return self->field_0x2C;
}

/* Whether the model's +0x4C flag set intersects `mask`. */
extern "C" u32 fn_800E1958(MHchar* self, u32 mask)
{
    return (self->field_0x4C & mask) != 0;
}

/* Store the model's +0x11C word. */
extern "C" void fn_800E25B0(MHchar* self, s32 value)
{
    self->field_0x11C = value;
}

/* The model's +0x00 base plus `offset`, or 0 when `offset` is 0. */
extern "C" u32 fn_800E25F4(MHchar* self, u32 offset)
{
    u32 base = self->field_0x00;
    if (offset != 0) {
        return base + offset;
    }
    return 0;
}

/* Copy `src` into `dst` as one word. */
extern "C" void fn_800E2640(u32* dst, u32* src)
{
    *dst = *src;
}

/* Write the model's +0xD4 float block at `index`. */
extern "C" void fn_800E26B4(MHchar* self, u32 index, f32 value)
{
    self->field_0xD4[index] = value;
}

/* Whether the model's +0x00 word is non-zero. */
extern "C" u32 fn_800E2994(MHchar* self)
{
    return self->field_0x00 != 0;
}

/* Copy `src` into `dst` as one word. */
extern "C" void fn_800E2EB0(u32* dst, u32* src)
{
    *dst = *src;
}

/* Copy `src` into `dst` as one word. */
extern "C" void fn_800E301C(u32* dst, u32* src)
{
    *dst = *src;
}

/* Whether the model's +0x00 word is non-zero. */
extern "C" u32 fn_800E3150(MHchar* self)
{
    return self->field_0x00 != 0;
}

/* Store the model's +0x122 halfword (the low 16 bits of `value`). */
extern "C" void fn_800E3264(MHchar* self, u32 value)
{
    self->field_0x122 = (u16)value;
}

/* Release the model's +0x118 handle. */
extern "C" void fn_800E30DC(MHchar* self)
{
    void* handle = (void*)self->field_0x118;
    if (handle) {
        return fn_8007E498(handle);
    }
}

/* Reset the model through its +0x?? state. */
extern "C" void fn_800E3AE8(void* self)
{
    return fn_80054FE8(self, 0);
}

/* Point the model's +0x00 word at the initial joint table. */
extern "C" void fn_800E3B2C(MHchar* self)
{
    self->field_0x00 = (u32)&lbl_80597CC0;
}

/* Hand the model's +0x110 sub-object to `fn_800E2680`. */
extern "C" void fn_800E2678(MHchar* self)
{
    return fn_800E2680(&self->field_0x110);
}

/* The model's joint count, via the +0x114 sub-object. */
int MHchar::get_joint_num(void)
{
    return fn_80097F80(&field_0x114);
}

/* `get_joint_num` for the bare-stem caller. */
extern "C" int fn_800E31E8(MHchar* self)
{
    return fn_80097F80(&self->field_0x114);
}

/* The model's +0x114 joint-list handle. */
extern "C" u32 fn_800E28E4(MHchar* self)
{
    return fn_80098868(&self->field_0x114);
}

/* Broadcast `scale` over the model's +0x1C scale vector and hand it to the +0x118 render handle. */
void MHchar::setScaleAll(f32 scale)
{
    this->scale_0x1C.x = scale;
    this->scale_0x1C.y = scale;
    this->scale_0x1C.z = scale;
    MHchar* handle = (MHchar*)this->field_0x118;
    if (handle) {
        return fn_800E0808(handle, &this->scale_0x1C);
    }
}

/* Tear down the +0x110 motion sub-object. */
extern "C" void fn_800E2680(void* sub)
{
    CleanUpTracks(sub);
    return fn_80093AA0(sub);
}

/* Point the model's +0x00 word at the +0xCA0 joint table, after the base reset. */
extern "C" MHchar* fn_800E3AF0(MHchar* self)
{
    fn_800E3B2C(self);
    self->field_0x00 = (u32)&lbl_80597CA0;
    return self;
}

/* Copy the +0x114 joint entry selected by `arg` into `dst`. */
extern "C" void fn_800E0B9C(MHchar* self, void* dst, void* arg)
{
    void* entry = fn_80097F18(&self->field_0x114, arg);
    fn_8005D0CC(dst, &entry);
}

/* Place `dst` at the joint whose index the +0x114 list resolves, scaled by 48 bytes per entry. */
extern "C" void fn_800E0A14(MHchar* self, void* arg, void* dst)
{
    if (self->field_0x118 == 0) {
        return;
    }
    void* entry = fn_80097F18(&self->field_0x114, arg);
    int offset = fn_8006FDCC(&entry) * 48;
    MHchar* handle = (MHchar*)self->field_0x118;
    fn_800532DC(dst, (void*)(fn_800E0A8C(handle) + offset));
}

/* Copy one word into `dst` and return `dst` (the assignment-operator shape). */
extern "C" u32* fn_800E2610(u32* dst, u32* src)
{
    fn_800E2640(dst, src);
    return dst;
}

/* Copy one word into `dst` and return `dst`. */
extern "C" u32* fn_800E2E80(u32* dst, u32* src)
{
    fn_800E2EB0(dst, src);
    return dst;
}

/* Copy one word into `dst` and return `dst`. */
extern "C" u32* fn_800E2FEC(u32* dst, u32* src)
{
    fn_800E301C(dst, src);
    return dst;
}

/* Positional SE: bank 46, id `30 + (arg & 3)`, at the caller's position broadcast over all three axes. */
extern "C" SeSlot* fn_800DD3B8(u32 arg)
{
    nw4r::math::VEC3 v;
    f32 c = lbl_807963E0;
    setVec3(&v, c, c, c);
    return fn_800DA72C(46, ((u8)arg & 3) + 30, &v);
}

/* One 0x34-byte motion record of the model's motion table. size: 0x34 */
struct MhMotionEntry {
    /* +0x00 */ u8 pad_0x00[8];
    /* +0x08 */ u8 field_0x08;
    /* +0x09 */ u8 field_0x09;
    /* +0x0A */ u8 pad_0x0A[0x0E];
    /* +0x18 */ u32 field_0x18;
    /* +0x1C */ u8 pad_0x1C[2];
    /* +0x1E */ u16 field_0x1E;
    /* +0x20 */ u32 field_0x20;
    /* +0x24 */ f32 field_0x24;
    /* +0x28 */ f32 field_0x28;
    /* +0x2C */ u8 pad_0x2C[8];
};

/* Clear a motion record and seed its two rate constants. */
extern "C" void fn_800E3C90(MhMotionEntry* entry)
{
    entry->field_0x18 = 0;
    entry->field_0x20 = 0;
    entry->field_0x1E = 0;
    entry->field_0x08 = 0;
    entry->field_0x09 = 0;
    entry->field_0x24 = lbl_80796450;
    entry->field_0x28 = lbl_80796454;
}

/* Apply `value` to the +0x122 halfword for the motion kinds that carry it. */
extern "C" void fn_800E31F0(void* unused, MHchar* self, s32 kind, u32 value)
{
    switch (kind) {
    case 1:
    case 2:
        fn_80080B10(self, kind);
        fn_800E3264(self, value);
        break;
    case 4:
        fn_80080B10(self, kind);
        break;
    }
}

#include "sys_mem.h"

/* The ryugeki (shell-shot) SE: bank 46 id selected by the caller's kind flag. */
SeSlot* ryugeki_shot_se_req(_PLW* self, nw4r::math::VEC3* pos, u8 kind)
{
    _se_w* work = self->field_0xAFC;
    if (!work) {
        return 0;
    }
    s32 id = kind ? 31 : 11;
    return se_req_pos_ps(work, id, 2, pos);
}

/* Clear the in-use flag of all 32 SE slots. */
extern "C" void fn_800DF948(_se_w* work)
{
    for (int i = 0; i < 32; i++) {
        work->slots[i].in_use = 0;
    }
}

/* Release `self` through its +0x?? teardown when `flag` is positive. */
extern "C" void* fn_800E0504(void* self, s16 flag)
{
    if (self) {
        fn_800810DC(self, 0);
        if (flag > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* Claim a free slot for the ryugeki positional SE (bank 46) and fill it in. */
extern "C" void fn_800DD9FC(_PLW* self, nw4r::math::VEC3* pos, u8 kind)
{
    _se_w* work = self->field_0xAFC;
    if (!work) {
        return;
    }
    s32 id = kind ? 30 : 10;
    SeSlot* slot = fn_800D8E58(work, id);
    if (!slot) {
        return;
    }
    slot->state = 3;
    slot->kind = 2;
    slot->owner = work->field_0x0C;
    slot->id = id;
    slot->field_0x30 = -1;
    slot->field_0x34 = -1;
    slot->param = 0;
    copyVec3(&slot->pos, pos);
    slot->field_0x10 = slot->pos;
}

/* Find the first live slot owned by `owner`/`id` whose hold is still positive. */
extern "C" SeSlot* fn_800DDF44(_se_w* work, u32 a4, s32 a5)
{
    SeSlot* slot = &work->slots[0];
    for (int i = 0; i < 32; i++, slot++) {
        if (slot->in_use == 0) {
            continue;
        }
        if (slot->field_0x03 == 0) {
            continue;
        }
        if (slot->field_0x44 != a4) {
            continue;
        }
        if (slot->owner != (u32)work->field_0x0C) {
            continue;
        }
        if (slot->id != (u32)a5) {
            continue;
        }
        if (slot->field_0x48 <= -1) {
            continue;
        }
        return slot;
    }
    return 0;
}
