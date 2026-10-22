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
 *   flipcheck: `.rodata` claimed, not emitted.
 *   flipcheck: `.sbss` claimed, not emitted.
 *   flipcheck: `.sdata` claimed, not emitted.
 *   flipcheck: `.sdata2` claimed, not emitted.
 * NAMES. GUESS: `mhchar_joint_mtx_get`, `mhchar_node_count`
 *   GUESS (from each body and its callers): `copied_mat_access_valid_ck`, `MHcharWorldCallback` and its
 *   `ApplyTiming` hook (the world callback whose table is 0x80597CA0; the hook enables the model's callback timing).
 *   The nw4r inline setters `ScnMdlSimple::SetCalcWorldCallback`/`SetCalcWorldNodeID` and the ICalcWorldCallback
 *   constructor keep their copies here.
 */

#include "types.h"
#include "mh3_pad.h" /* the owner header (rule 2) */
#include "nw4r/math.h"
#include "sound/mhchar.h"
#include "sound/se.h"
#include "g3d/g3d_resmat.h" /* nw4r::g3d::ResMdl (rule 2) */
#include "g3d/g3d_resnode.h" /* nw4r::g3d::ResNode (rule 2) */
#include "g3d/g3d_scnmdlsmpl.h" /* ScnMdlSimple, ICalcWorldCallback (rule 2) */
#include "sys_mem.h"

#pragma peephole off

/* This unit's own functions used before their definition (the file is in address order). */
extern "C" {
s32 mhchar_wld_mtx_array_get(MHchar* self);
void fn_800E2640(u32* dst, u32* src);
void fn_800E2EB0(u32* dst, u32* src);
void fn_800E301C(u32* dst, u32* src);
}

/* The world-matrix callback the model installs (its table is `.data` 0x80597CA0): overrides ExecCallbackA
 * (0x800E3270, unwritten) and adds a timing hook at +0x18.  The class name and `ApplyTiming` are GUESSes (the
 * hook has the same body as `lobby/lb_equip_page.cpp`'s).  size: 0x4 (the table pointer only; an approximation) */
class MHcharWorldCallback : public nw4r::g3d::ICalcWorldCallback {
public:
    MHcharWorldCallback();
    virtual ~MHcharWorldCallback();
    virtual void ExecCallbackA(nw4r::g3d::ChrAnmResult* pResult, nw4r::g3d::ResMdl mdl,
                               nw4r::g3d::FuncObjCalcWorld* pFuncObj);
    virtual void ApplyTiming(nw4r::g3d::ScnMdlSimple* scn, s32 kind, u32 value);
};

/* 0x800E0504 (0x5C): Destroys the callback through the world-callback base (called out of line). */
#pragma dont_inline on
MHcharWorldCallback::~MHcharWorldCallback()
{
}
#pragma dont_inline reset

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
extern "C" void mhchar_joint_mtx_get(MHchar* self, void* arg, void* dst)
{
    if (self->field_0x118 == 0) {
        return;
    }
    void* entry = reinterpret_cast<const nw4r::g3d::ResMdl*>(&self->field_0x114)->GetResNode((u32)arg).mpData;
    int offset = res_node_mtx_id_get(&entry) * 48;
    MHchar* handle = (MHchar*)self->field_0x118;
    mtx34_copy(dst, (void*)(mhchar_wld_mtx_array_get(handle) + offset));
}

/* Read the model's +0xEC word. */
extern "C" s32 mhchar_wld_mtx_array_get(MHchar* self)
{
    return self->field_0xEC;
}

/* Copy the +0x114 joint entry selected by `arg` into `dst`. */
extern "C" void fn_800E0B9C(MHchar* self, void* dst, void* arg)
{
    void* entry = reinterpret_cast<const nw4r::g3d::ResMdl*>(&self->field_0x114)->GetResNode((u32)arg).mpData;
    res_node_assign(dst, &entry);
}

/* The model's joint count, via the +0x114 sub-object. */
int MHchar::get_joint_num(void)
{
    return reinterpret_cast<const nw4r::g3d::ResMdl*>(&field_0x114)->GetResNodeNumEntries();
}

/* Store the model's +0x38 word. */
extern "C" void mhchar_draw_mode_set(MHchar* self, s32 value)
{
    self->field_0x38 = value;
}

/* Read the model's +0x2C word. */
extern "C" s32 fn_800E0DD8(MHchar* self)
{
    return self->field_0x2C;
}

/* Whether the model's +0x4C flag set intersects `mask`. */
extern "C" u32 mhchar_motion_flag_ck(MHchar* self, u32 mask)
{
    return (self->field_0x4C & mask) != 0;
}

/* 0x800E25B0 (0x8): Stores the world-matrix callback (nw4r's inline setter, kept here). */
void nw4r::g3d::ScnMdlSimple::SetCalcWorldCallback(ICalcWorldCallback* pCallback)
{
    mpCalcWorldCallback = pCallback;
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
extern "C" void mhchar_mat_blend_set(MHchar* self, u32 index, f32 value)
{
    self->field_0xD4[index] = value;
}

/* The model's +0x114 joint-list handle. */
extern "C" u32 mhchar_mat_num_get(MHchar* self)
{
    return reinterpret_cast<const nw4r::g3d::ResMdl*>(&self->field_0x114)->GetResMatNumEntries();
}

/* Whether the model's +0x00 word is non-zero. */
extern "C" u32 copied_mat_access_valid_ck(MHchar* self)
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
        return scn_mdl_clean_mat_buffer(handle);
    }
}

/* Whether the model's +0x00 word is non-zero. */
extern "C" u32 fn_800E3150(MHchar* self)
{
    return self->field_0x00 != 0;
}

/* `get_joint_num` for the bare-stem caller. */
extern "C" int mhchar_node_count(MHchar* self)
{
    return reinterpret_cast<const nw4r::g3d::ResMdl*>(&self->field_0x114)->GetResNodeNumEntries();
}

/* 0x800E31F0 (0x74): Enables the model's world callback at `kind`; kinds 1 and 2 also set the node it runs at. */
void MHcharWorldCallback::ApplyTiming(nw4r::g3d::ScnMdlSimple* self, s32 kind, u32 value)
{
    switch (kind) {
    case 1:
    case 2:
        self->EnableScnMdlCallbackTiming((nw4r::g3d::ScnObj::Timing)kind);
        self->SetCalcWorldNodeID(value);
        break;
    case 4:
        self->EnableScnMdlCallbackTiming((nw4r::g3d::ScnObj::Timing)kind);
        break;
    }
}

/* 0x800E3264 (0xC): Stores the node the world callback runs at (nw4r's inline setter, kept here). */
void nw4r::g3d::ScnMdlSimple::SetCalcWorldNodeID(u32 nodeID)
{
    mCalcWorldNodeID = (u16)nodeID;
}

/* Reset the model through its +0x?? state. */
extern "C" void fn_800E3AE8(void* self)
{
    res_file_ctor((u32*)self, 0);
}

/* 0x800E3AF0 (0x3C): Constructs the callback: the world-callback base, then this class's table. */
MHcharWorldCallback::MHcharWorldCallback()
{
}

/* 0x800E3B2C (0x10): Constructs the world-callback base: stores its table (nw4r's inline constructor, kept here). */
nw4r::g3d::ICalcWorldCallback::ICalcWorldCallback()
{
}
