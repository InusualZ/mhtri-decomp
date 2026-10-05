/* sound/mhchar.cpp - the `MHchar` model class: the model block's accessors, the joint/motion helpers and the formation-position helpers,
 * .text 0x800E0504..0x800E3B3C (81 symbols), `.bss` 0x1580 (`.ctors` word, `.data` 0xA0 holding the model's initial joint tables).
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for the unsplit functions of the range; the map's real names (the `MHchar::` methods)
 * are used below.  The language is C++: the map carries C++ manglings (`move__6MHcharFUs`, `setScaleAll__6MHcharFf`).
 *
 * Phase 4: the old `sound/fn_800DD1F0.cpp` (0x800DD1F0..0x800E3CBC) held two TUs; the reconciled candidate cuts it into the SE request
 * tail (`sound/se_req.cpp`, plus the head folded into `sound/fn_800D7F54.cpp`), this `MHchar` model class and the job request
 * (`sound/sound_job.cpp`).  The functions here are the old unit's bodies, moved unchanged (measured there, flags unchanged).  The range's
 * codegen carries the non-record forms the peephole pass would fold, so the original TU was peephole-off.
 *
 * Name: `mhchar` is the class the range is about (`MHchar::` methods, the model block `sound/mhchar.h` views); no `__FILE__`
 * string names the TU, so the file name is a GUESS from the class.
 */

#include "types.h"
#include "mh3_pad.h" /* the owner header (rule 2) */
#include "nw4r/math.h"
#include "sound/se.h"
#include "sound/mhchar.h"
#include "sys_mem.h"

#pragma peephole off

/* This unit's own functions used before their definition (the file is in address order). */
extern "C" {
s32 fn_800E0A8C(MHchar* self);
void fn_800E2640(u32* dst, u32* src);
void fn_800E2EB0(u32* dst, u32* src);
void fn_800E301C(u32* dst, u32* src);
void fn_800E3264(MHchar* self, u32 value);
void fn_800E3B2C(MHchar* self);
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

/* Read the model's +0xEC word. */
extern "C" s32 fn_800E0A8C(MHchar* self)
{
    return self->field_0xEC;
}

/* Copy the +0x114 joint entry selected by `arg` into `dst`. */
extern "C" void fn_800E0B9C(MHchar* self, void* dst, void* arg)
{
    void* entry = fn_80097F18(&self->field_0x114, arg);
    fn_8005D0CC(dst, &entry);
}

/* The model's joint count, via the +0x114 sub-object. */
int MHchar::get_joint_num(void)
{
    return fn_80097F80(&field_0x114);
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

/* Copy one word into `dst` and return `dst` (the assignment-operator shape). */
extern "C" u32* fn_800E2610(u32* dst, u32* src)
{
    fn_800E2640(dst, src);
    return dst;
}

/* Copy `src` into `dst` as one word. */
extern "C" void fn_800E2640(u32* dst, u32* src)
{
    *dst = *src;
}

/* Hand the model's +0x110 sub-object to `fn_800E2680`. */
extern "C" void fn_800E2678(MHchar* self)
{
    return fn_800E2680(&self->field_0x110);
}

/* Tear down the +0x110 motion sub-object. */
extern "C" void fn_800E2680(void* sub)
{
    CleanUpTracks(sub);
    return fn_80093AA0(sub);
}

/* Write the model's +0xD4 float block at `index`. */
extern "C" void fn_800E26B4(MHchar* self, u32 index, f32 value)
{
    self->field_0xD4[index] = value;
}

/* The model's +0x114 joint-list handle. */
extern "C" u32 fn_800E28E4(MHchar* self)
{
    return fn_80098868(&self->field_0x114);
}

/* Whether the model's +0x00 word is non-zero. */
extern "C" u32 fn_800E2994(MHchar* self)
{
    return self->field_0x00 != 0;
}

/* Copy one word into `dst` and return `dst`. */
extern "C" u32* fn_800E2E80(u32* dst, u32* src)
{
    fn_800E2EB0(dst, src);
    return dst;
}

/* Copy `src` into `dst` as one word. */
extern "C" void fn_800E2EB0(u32* dst, u32* src)
{
    *dst = *src;
}

/* Copy one word into `dst` and return `dst`. */
extern "C" u32* fn_800E2FEC(u32* dst, u32* src)
{
    fn_800E301C(dst, src);
    return dst;
}

/* Copy `src` into `dst` as one word. */
extern "C" void fn_800E301C(u32* dst, u32* src)
{
    *dst = *src;
}

/* Release the model's +0x118 handle. */
extern "C" void fn_800E30DC(MHchar* self)
{
    void* handle = (void*)self->field_0x118;
    if (handle) {
        return fn_8007E498(handle);
    }
}

/* Whether the model's +0x00 word is non-zero. */
extern "C" u32 fn_800E3150(MHchar* self)
{
    return self->field_0x00 != 0;
}

/* `get_joint_num` for the bare-stem caller. */
extern "C" int fn_800E31E8(MHchar* self)
{
    return fn_80097F80(&self->field_0x114);
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

/* Store the model's +0x122 halfword (the low 16 bits of `value`). */
extern "C" void fn_800E3264(MHchar* self, u32 value)
{
    self->field_0x122 = (u16)value;
}

/* Reset the model through its +0x?? state. */
extern "C" void fn_800E3AE8(void* self)
{
    res_file_ctor((u32*)self, 0);
}

/* Point the model's +0x00 word at the +0xCA0 joint table, after the base reset. */
extern "C" MHchar* fn_800E3AF0(MHchar* self)
{
    fn_800E3B2C(self);
    self->field_0x00 = (u32)&lbl_80597CA0;
    return self;
}

/* Point the model's +0x00 word at the initial joint table. */
extern "C" void fn_800E3B2C(MHchar* self)
{
    self->field_0x00 = (u32)&lbl_80597CC0;
}
