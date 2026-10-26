/* sound/mhchar.cpp - the `MHchar` model class: the model block's reset and placement, the motion layers (start,
 * cross-fade, frame tests, root motion), the model/resource construction and release, the material accessors,
 * the world-matrix callback and the motion banks the models draw their animations from.
 *
 * RANGE. .text 0x800E0504..0x800E3B3C (81 symbols, address order), extab/extabindex, .ctors 0x8056F2F0, .rodata
 *   0x8056F868 (the indirect-matrix id list), .data 0x80597C80 (the frame-test jump table, the two callback
 *   tables, the bounds-assert strings), .bss 0x80695610 (the motion bank tables), .sdata 0x80791450 ("_",
 *   "NULL"), .sbss 0x80794980 (the callback object and the two single-file banks), .sdata2 0x80796438 (0, 1, 2,
 *   -1 and the int-to-float bias).
 * FLAGS. The unit's library flags; `#pragma peephole off` (the range keeps the unfused compare forms).
 * NAMES. The class members are the map's manglings.  The `MHchar` record is pl.h's 0x140-byte prefix; the
 *   methods read the whole 0x164-byte record through `MHcharModel` below.
 *   GUESS (from each body and its callers): `MHcharWorldCallback` and `ApplyTiming`; every `mhchar_*` C name;
 *   `scn_leaf_scale_set`, `anm_obj_chr_res_get_res`, `anm_obj_chr_res_set_play_policy`,
 *   `anm_obj_chr_node_get_child`, `res_anm_chr_get_name`, `res_anm_chr_ofs_to_ptr`, `res_anm_chr_assign`,
 *   `res_anm_chr_common_assign`, `res_file_bind_self`, `res_file_release`, `res_gen_mode_copy_ctor`,
 *   `res_gen_mode_common_copy_ctor`, `res_mat_pix_copy_ctor`, `res_mat_pix_common_copy_ctor`,
 *   `scn_mdl_vis_access_valid_ck` (the nw4r inline accessors this TU keeps out of line), `MHcharBankFile`;
 *   the bank tables `mhchar_bank*`, `mhchar_world_callback`.
 * RESIDUALS.
 *   - `MHcharModel` repeats pl.h's `MHchar` prefix: pl.h's record stops at +0x140 and its embedders pad the
 *     rest, so the full layout cannot move into pl.h without re-cutting every embedder.  Modelling `MHchar` as
 *     a class with a `ResMdl` member (lb_npc's `mhchar_construct`, request lobby-npc#4) waits on that move.
 *   - `__ct__14MHcharBankFileFv` (0x800E3AE8): retail's array thunk tail-calls `res_file_ctor` (the C-named
 *     `ResFile(void* = NULL)` constructor, a draw_shape row); a C++ constructor returns `this`, so ours saves it.
 *   - .data: retail emits the base `ICalcWorldCallback` table (0x80597CC0) here and the two callback tables
 *     before the assert strings; the g3d header defines the base slots out of line, so this object emits
 *     neither the base table nor that order.
 *   - `mhchar_mat_hide_set`/`mhchar_mdl_prepare`: retail copies the `ResMatMisc` handle through the
 *     out-of-line `ResMatMisc(const ResMatMisc&)`, which g3d_resmat.h does not declare (declaring it changes
 *     `g3d/g3d_draw.cpp`'s copies).
 *   - `mhchar_frame_check`: the hold path jumps into the shared case switch (a `goto` shape); the body uses a
 *     flag, one saved register more.
 *   - `mhchar_layer_weight_update`/`mhchar_model_release`: retail hoists the motion-table walk's trip count into
 *     a saved register; ours reloads it (register colouring only).
 *   - `mhchar_motion_layer_set`: retail keeps `layer * 4` in a saved register across the calls (colouring).
 *   - `mhchar_motion_start`: retail rounds the layer-0 frame (`frsp`) before the previous-frame store.
 *   - The `res_file_*`/`nw_*`/`res_anm_chr_ctor`/`frame_ctrl_set_play_policy` callees are registry names; until
 *     they land in the map the relocations read as a name difference.
 */

#include "types.h"
#include "mh3_pad.h" /* the owner header (rule 2) */
#include "nw4r/math.h"
#include "sound/mhchar.h"
#include "sound/fn_800E3CBC.h"     /* PrimRec */
#include "sound/sound_job_request.h"
#include "g3d/g3d_resmat.h"        /* nw4r::g3d::ResMdl, ResMat, ResTexSrt, ResMatMisc (rule 2) */
#include "g3d/g3d_resnode.h"       /* nw4r::g3d::ResNode (rule 2) */
#include "g3d/g3d_resfile.h"       /* nw4r::g3d::ResFile (rule 2) */
#include "g3d/g3d_anmchr.h"        /* the AnmObjChr classes, ResAnmChr, res_node_copy_ctor/res_node_get_id */
#include "g3d/g3d_scnmdlsmpl.h"    /* ScnMdlSimple, ICalcWorldCallback (rule 2) */
#include "nw4r/g3d/scnmdl.h"       /* ScnMdl, ScnMdl::CopiedMatAccess (rule 2) */
#include "g3d/g3d_scnmdl.h"        /* the CopiedMatAccess block getters and end-edit hooks */
#include "g3d/g3d_calcmaterial.h"  /* res_mat_chan_end_edit, res_mat_tev_color_*, res_tex_srt_copy_ctor */
#include "g3d/g3d_anmclr.h"        /* res_mat_copy_ctor */
#include "g3d/res_mat_chan_copy_ctor.h"
#include "g3d/res_mdl_copy_ctor.h"
#include "g3d/res_node_assign.h"
#include "g3d/res_node_mtx_id_get.h"
#include "g3d/fn_8005AA28.h"       /* res_node_is_valid */
#include "g3d/anm_chr_result_scale_get.h"
#include "g3d/chr_anm_result_init.h"
#include "g3d/res_file_get_res_anm_chr.h"
#include "nw_anm_chr_res_create.h"
#include "hermite_interp.h"
#include "rotVecZXY.h"
#include "g3d/CleanUpTracks.h"
#include "draw_shape.h"            /* res_file_ctor, res_file_assign */
#include "draw_shape/mtx34_copy.h"
#include "fn_8004CAD8.h"           /* cpSetRotMatrixZXY, subVec3, MTX34_ctor */
#include "vec3_scale.h"
#include "g3d/g3d_calcworld.h"     /* addVec3To */
#include "MSL_C/string.h"     /* strstr */
#include "MSL_C/strtoul.h"         /* atoi */
#include "Runtime.PPCEABI.H/memset.h"
#include "nw4r/db_assert.h"
#include "sys_mem.h"

#pragma peephole off

/* The world-matrix callback the model installs (its table is `.data` 0x80597CA0): overrides ExecCallbackA and
 * adds a timing hook at +0x18.  size: 0x4 */
class MHcharWorldCallback : public nw4r::g3d::ICalcWorldCallback {
public:
    MHcharWorldCallback();
    virtual ~MHcharWorldCallback();
    virtual void ExecCallbackA(nw4r::g3d::ChrAnmResult* pResult, nw4r::g3d::ResMdl mdl,
                               nw4r::g3d::FuncObjCalcWorld* pFuncObj);
    virtual void ApplyTiming(nw4r::g3d::ScnMdlSimple* scn, s32 kind, u32 value);
};

/* The scene work `nw_scn_mdl_create` hands back: the model it constructed sits at +0x04.
 * size: 0x8 (approximation: only +0x04 is read) */
struct MHcharScnWork {
    /* +0x00 */ u8 pad_0x00[0x4];
    /* +0x04 */ nw4r::g3d::ScnMdl* scn_mdl;
};

/* The model record the `MHchar` methods address; pl.h's `MHchar` is its first 0x140 bytes.  Six motion layers
 * (0..2 playing, 3..5 the outgoing copies a cross-fade fades out) keep a motion number, a rate, the current and
 * previous frame, the end and loop frames, a blend weight and the animation; the 16 motions of a model file are
 * built once by `mhchar_model_create`.  size: 0x164 (the `mhchar_reset` stores run to +0x163) */
struct MHcharModel {
    /* +0x000 */ u8 state_0x00[4];
    /* +0x004 */ nw4r::math::VEC3 pos;
    /* +0x010 */ nw4r::math::VEC3 ofs;            /* added to `pos` when the model matrix is built */
    /* +0x01C */ nw4r::math::VEC3 scale;
    /* +0x028 */ _CP_VECTOR rot;
    /* +0x034 */ u8 sound_on;                     /* non-zero: the placement queues the sound job */
    /* +0x035 */ u8 sound_layer;                  /* the job's layer */
    /* +0x036 */ u16 field_0x36;
    /* +0x038 */ s32 draw_mode;                   /* bit 0: no sound job, bit 1: no matrix rebuild */
    /* +0x03C */ void (*draw_callback)(struct PrimRec*);
    /* +0x040 */ MHcharWorldCallback* world_callback;
    /* +0x044 */ f32 blend_timer;                 /* frames left of the layer cross-fade */
    /* +0x048 */ f32 blend_time;                  /* the cross-fade's length */
    /* +0x04C */ u32 motion_flags;                /* bit 0: keep the outgoing layers playing, bit 1: no root motion */
    /* +0x050 */ u16 motion_no[6];
    /* +0x05C */ f32 rate[6];
    /* +0x074 */ f32 frame[6];
    /* +0x08C */ f32 prev_frame[6];
    /* +0x0A4 */ f32 end_frame[6];
    /* +0x0BC */ f32 loop_frame[6];
    /* +0x0D4 */ f32 weight[6];
    /* +0x0EC */ s32 root_node_id;                /* the "NULL" node, whose translation is the root motion */
    /* +0x0F0 */ u8 frame_set;                    /* a motion was just started: the next update keeps its frame */
    /* +0x0F1 */ u8 anim_skip;                    /* frames the motion update is skipped */
    /* +0x0F2 */ u8 move_skip;                    /* frames the root motion is not applied */
    /* +0x0F3 */ u8 pad_0xF3;
    /* +0x0F4 */ nw4r::g3d::AnmObjChrRes* layer_anm[6];
    /* +0x10C */ MHcharScnWork* scn_work;
    /* +0x110 */ nw4r::g3d::ResFile file;
    /* +0x114 */ nw4r::g3d::ResMdl mdl;
    /* +0x118 */ nw4r::g3d::ScnMdl* scn_mdl;
    /* +0x11C */ nw4r::g3d::AnmObjChrRes* motion_res[16];
    /* +0x15C */ u32 motion_num;
    /* +0x160 */ nw4r::g3d::AnmObjChrBlend* blend;
};

/* A bank's resource file: a `ResFile` handle built empty by `res_file_ctor` (nw4r's `ResFile(void* = NULL)`;
 * the array constructor runs this class's out-of-line copy, 0x800E3AE8).  size: 0x4 */
struct MHcharBankFile {
    /* +0x0 */ nw4r::g3d::ResFile file;

    MHcharBankFile() { res_file_ctor((u32*)&file, 0); }
};

/* The motion banks: per kind, the resource file(s), the motion-to-animation index table and the start of each
 * hundred-group in it (kind 0 the common bank, 1/4 four weapon banks, 2 six banks, 3 one bank). */
extern u16 mhchar_bank0_anm_tbl[400];
extern u16 mhchar_bank0_start_tbl[10];
extern u16 mhchar_bank1_anm_tbl[4][300];
extern u16 mhchar_bank1_start_tbl[4][5];
extern u16 mhchar_bank2_anm_tbl[6][150];
extern u16 mhchar_bank2_start_tbl[6][5];
extern u16 mhchar_bank3_anm_tbl[160];
extern u16 mhchar_bank3_start_tbl[5];
extern MHcharBankFile mhchar_bank1_files[4];
extern MHcharBankFile mhchar_bank2_files[6];

extern MHcharWorldCallback mhchar_world_callback;
extern MHcharBankFile mhchar_bank0_file;
extern MHcharBankFile mhchar_bank3_file;

/* The address of a handle word returned by value: the word lives in a temporary for the call it is passed to,
 * where retail copies the returned handle out of line. */
inline struct ResHandle* res_handle_temp(const u32& word)
{
    return (struct ResHandle*)&word;
}

/* This unit's own functions used before their definition (the file is in address order). */
extern "C" {
void scn_leaf_scale_set(nw4r::g3d::ScnLeaf* self, nw4r::math::VEC3* src);
void mhchar_ofs_clear(MHchar* self);
void mhchar_motion_update(MHchar* self);
void mhchar_model_mtx_update(MHchar* self);
nw4r::math::MTX34* mhchar_wld_mtx_array_get(nw4r::g3d::ScnMdlSimple* mdl);
void mhchar_draw_mode_set(MHchar* self, s32 value);
u32 anm_obj_chr_res_get_res(nw4r::g3d::AnmObjChrRes* anm);
void anm_obj_chr_res_set_play_policy(nw4r::g3d::AnmObjChrRes* anm, nw4r::g3d::PlayPolicyFunc policy);
void mhchar_layer_frame_range_set(MHchar* self, s32 layer, s32 to_end, s32 loop, u8 looped);
u32 mhchar_motion_flag_ck(MHchar* self, u32 mask);
nw4r::g3d::AnmObjChr* mhchar_layer_anm_get(MHchar* self, s32 layer);
nw4r::g3d::AnmObjChr* anm_obj_chr_node_get_child(nw4r::g3d::AnmObjChrNode* node, int idx);
void mhchar_layer_trans_delta_get(MHchar* self, nw4r::math::VEC3* out, s32 layer, f32 from, f32 to);
void mhchar_layer_weight_update(MHchar* self);
const char* res_anm_chr_get_name(const nw4r::g3d::ResAnmChr* res);
const char* res_anm_chr_ofs_to_ptr(const nw4r::g3d::ResAnmChr* res, s32 ofs);
nw4r::g3d::ResAnmChr* res_anm_chr_assign(nw4r::g3d::ResAnmChr* dst, u32* src);
void res_anm_chr_common_assign(nw4r::g3d::ResAnmChr* dst, u32* src);
void res_file_bind_self(nw4r::g3d::ResFile* file);
void res_file_release(nw4r::g3d::ResFile* file);
u32 mhchar_mat_num_get(MHchar* self);
u32 copied_mat_access_valid_ck(nw4r::g3d::ScnMdl::CopiedMatAccess* access);
struct ResHandle* res_gen_mode_copy_ctor(struct ResHandle* dst, u32* src);
void res_gen_mode_common_copy_ctor(struct ResHandle* dst, u32* src);
struct ResHandle* res_mat_pix_copy_ctor(struct ResHandle* dst, u32* src);
void res_mat_pix_common_copy_ctor(struct ResHandle* dst, u32* src);
void mhchar_mat_buffer_clean(MHchar* self, u32 index, u32 option);
u32 scn_mdl_vis_access_valid_ck(ScnMdlVisAccess* access);
void mhchar_mat_hide_set(MHchar* self, u32 index, s32 light);
void mhchar_bank_start_set(s32 kind, s32 index, u16 group, u16 start);
u32 mhchar_bank_anm_chr_get(nw4r::g3d::ResAnmChr* out, s32 kind, s32 index, u16 motion);
nw4r::g3d::AnmObjChrRes* mhchar_bank_anm_create(u32 kind, s32 index, u16 motion, nw4r::g3d::ResMdl* mdl);
}

/* 0x800E0504 (0x5C): Destroys the callback through the world-callback base (called out of line). */
#pragma dont_inline on
MHcharWorldCallback::~MHcharWorldCallback()
{
}
#pragma dont_inline reset

/* 0x800E0560 (0x1C8): Clears the model record: no model, no motions, unit scale and the shared world callback. */
extern "C" void mhchar_reset(MHchar* chr)
{
    MHcharModel* self = reinterpret_cast<MHcharModel*>(chr);
    int i;

    self->state_0x00[0] = 0;
    self->state_0x00[1] = 0;
    self->state_0x00[2] = 0;
    self->state_0x00[3] = 0;
    self->sound_on = 1;
    self->pos.x = 0.0f;
    self->pos.y = 0.0f;
    self->pos.z = 0.0f;
    mhchar_ofs_clear(chr);
    self->rot.x = 0;
    self->rot.y = 0;
    self->rot.z = 0;
    self->blend = 0;
    self->scn_mdl = 0;
    for (i = 0; i < 16; i++) {
        self->motion_res[i] = 0;
    }
    self->scn_work = 0;
    self->field_0x36 = 0;
    self->blend_timer = 0.0f;
    self->blend_time = 0.0f;
    self->motion_flags = 0;
    self->frame_set = 0;
    self->root_node_id = 0;
    self->anim_skip = 0;
    self->move_skip = 0;
    self->motion_num = 0;
    for (i = 0; i < 6; i++) {
        self->motion_no[i] = 0;
        self->rate[i] = 0.0f;
        self->frame[i] = 0.0f;
        self->prev_frame[i] = 0.0f;
        self->end_frame[i] = 0.0f;
        self->loop_frame[i] = 0.0f;
        self->weight[i] = 0.0f;
        self->layer_anm[i] = 0;
    }
    mhchar_draw_mode_set(chr, 0);
    chr->setScaleAll(1.0f);
    self->world_callback = &mhchar_world_callback;
    self->sound_layer = 0;
    self->draw_callback = 0;
}

/* 0x800E0728 (0xE0): Places the model: runs the world-callback timing, the scale, the motion update, the
 * matrix and the sound job. */
void MHchar::move(u16 flags)
{
    MHcharModel* self = reinterpret_cast<MHcharModel*>(this);

    if (self->scn_mdl) {
        if (self->scn_mdl && self->blend) {
            if (self->world_callback) {
                self->world_callback->ApplyTiming(self->scn_mdl, 1, self->root_node_id);
            }
        }
        scn_leaf_scale_set(self->scn_mdl, &self->scale);
        mhchar_motion_update(this);
        if ((self->draw_mode & 2) == 0) {
            mhchar_model_mtx_update(this);
        }
        if (self->sound_on != 0 && (self->draw_mode & 1) == 0) {
            PrimRec* rec = sound_job_request(0, self->sound_layer, (s32)self->scn_mdl, 0, flags, 0,
                                             self->draw_callback);
            *reinterpret_cast<MHchar**>(&rec->f_0x2C) = this;
        }
    }
}

/* Copy `src` into the leaf's scale (nw4r's inline `ScnLeaf::SetScale`, kept here). */
extern "C" void scn_leaf_scale_set(nw4r::g3d::ScnLeaf* self, nw4r::math::VEC3* src)
{
    if (src) {
        copyVec3(&self->mScale, src);
    }
}

/* 0x800E081C (0xA0): Places the model at the matrix `mtx` and queues the sound job. */
void MHchar::move2(nw4r::math::MTX34* mtx, u16 flags)
{
    MHcharModel* self = reinterpret_cast<MHcharModel*>(this);

    if (self->scn_mdl) {
        scn_leaf_scale_set(self->scn_mdl, &self->scale);
        self->scn_mdl->SetMtx(nw4r::g3d::ScnObj::MTX_LOCAL, mtx);
        if (self->sound_on != 0 && (self->draw_mode & 1) == 0) {
            sound_job_request(0, self->sound_layer, (s32)self->scn_mdl, 0, flags, 0, self->draw_callback);
        }
    }
}

/* 0x800E08BC (0x50): Queues the model's sound job with `flags` when it has a model and its sound is on. */
extern "C" void mhchar_sound_job_request(MHchar* chr, u16 flags)
{
    MHcharModel* self = reinterpret_cast<MHcharModel*>(chr);

    if (self->scn_mdl && self->sound_on != 0 && (self->draw_mode & 1) == 0) {
        sound_job_request(0, self->sound_layer, (s32)self->scn_mdl, 0, flags, 0, self->draw_callback);
    }
}

/* Store the model's +0x3C word. */
extern "C" void mhchar_draw_callback_set(MHchar* self, u32 value)
{
    self->field_0x3C = value;
}

/* 0x800E0914 (0xBC): Builds the model matrix from the rotation and the offset position and recomputes the
 * world matrices when a motion is attached. */
extern "C" void mhchar_model_mtx_update(MHchar* chr)
{
    MHcharModel* self = reinterpret_cast<MHcharModel*>(chr);
    nw4r::math::MTX34 mtx;
    MTX34_ctor(&mtx);

    if (self->scn_mdl) {
        scn_leaf_scale_set(self->scn_mdl, &self->scale);
        cpSetRotMatrixZXY(&self->rot, &mtx);
        mtx.m[0][3] = self->pos.x + self->ofs.x;
        mtx.m[1][3] = self->pos.y + self->ofs.y;
        mtx.m[2][3] = self->pos.z + self->ofs.z;
        self->scn_mdl->SetMtx(nw4r::g3d::ScnObj::MTX_LOCAL, &mtx);
        if (self->blend) {
            self->scn_mdl->G3dProc(1, 0, 0);
        }
    }
}

/* Copy `src` into the model's position offset. */
extern "C" void mhchar_ofs_set(MHchar* self, nw4r::math::VEC3* src)
{
    copyVec3(&self->field_0x10, src);
}

/* Clear the model's position offset. */
extern "C" void mhchar_ofs_clear(MHchar* self)
{
    f32 c = 0.0f;
    self->field_0x10.x = c;
    self->field_0x10.y = c;
    self->field_0x10.z = c;
}

/* Broadcast `scale` over the model's +0x1C scale vector and hand it to the +0x118 render handle. */
void MHchar::setScaleAll(f32 scale)
{
    MHcharModel* self = reinterpret_cast<MHcharModel*>(this);
    this->scale_0x1C.x = scale;
    this->scale_0x1C.y = scale;
    this->scale_0x1C.z = scale;
    if (self->scn_mdl) {
        return scn_leaf_scale_set(self->scn_mdl, &self->scale);
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
    nw4r::g3d::ScnMdlSimple* handle = (nw4r::g3d::ScnMdlSimple*)self->field_0x118;
    mtx34_copy((Mtx34*)dst, (Mtx34*)((u8*)mhchar_wld_mtx_array_get(handle) + offset));
}

/* Read the scene model's world-matrix array (nw4r's inline `ScnMdlSimple::GetWldMtxArray`). */
extern "C" nw4r::math::MTX34* mhchar_wld_mtx_array_get(nw4r::g3d::ScnMdlSimple* mdl)
{
    return mdl->mpWorldMtxArray;
}

/* 0x800E0A94 (0xB4): Writes the world position of joint `joint` to `out` (zero without a model). */
void MHchar::get_joint_wpos(u32 joint, nw4r::math::VEC3* out)
{
    MHcharModel* self = reinterpret_cast<MHcharModel*>(this);
    nw4r::math::MTX34 mtx;
    MTX34_ctor(&mtx);

    if (self->scn_mdl) {
        void* entry = self->mdl.GetResNode(joint).mpData;
        int offset = res_node_mtx_id_get(&entry) * 48;
        mtx34_copy((Mtx34*)&mtx, (Mtx34*)((u8*)mhchar_wld_mtx_array_get(self->scn_mdl) + offset));
        out->x = mtx.m[0][3];
        out->y = mtx.m[1][3];
        out->z = mtx.m[2][3];
    } else {
        out->x = 0.0f;
        out->y = 0.0f;
        out->z = 0.0f;
    }
}

/* 0x800E0B48 (0x54): The id of the model node named `name`, 0 when the model has none. */
extern "C" u32 mhchar_node_id_get(MHchar* chr, const char* name)
{
    MHcharModel* self = reinterpret_cast<MHcharModel*>(chr);
    struct ResHandle node;
    u32 entry = (u32)self->mdl.GetResNode(name).mpData;
    res_node_copy_ctor(&node, &entry);
    if (res_node_is_valid(&node) == 0) {
        return 0;
    }
    return res_node_get_id(&node);
}

/* Copy the +0x114 joint entry selected by `arg` into `dst`. */
extern "C" void mhchar_res_node_get(MHchar* self, void* dst, void* arg)
{
    void* entry = reinterpret_cast<const nw4r::g3d::ResMdl*>(&self->field_0x114)->GetResNode((u32)arg).mpData;
    res_node_assign((struct ResHandle*)dst, (struct ResHandle*)&entry);
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

/* 0x800E0BF0 (0x1E8): Starts motion `motion` on layer `layer` at `start` frames with rate `rate`, as the only
 * playing layer. */
void MHchar::frame_init(long layer, u16 motion, f32 start, u32 flags, f32 rate)
{
    MHcharModel* self = reinterpret_cast<MHcharModel*>(this);

    if (self->scn_mdl && self->motion_res[motion] && self->blend) {
        self->motion_no[layer] = motion;
        self->motion_flags = flags;
        nw4r::g3d::AnmObjChrRes* anm = self->motion_res[motion];
        nw4r::g3d::ResAnmChr res;
        u32 handle = anm_obj_chr_res_get_res(anm);
        res_anm_chr_copy_ctor(&res, (nw4r::g3d::ResAnmChr*)&handle);
        u8 looped = res.GetAnmPolicy() == nw4r::g3d::ANM_POLICY_LOOP;
        anm->SetFrame(start);
        self->rate[layer] = 2.0f * rate;
        anm->SetUpdateRate(self->rate[layer]);
        self->blend->Attach(layer, anm);
        self->layer_anm[layer] = anm;
        self->blend_timer = 0.0f;
        self->motion_flags &= ~3;
        mhchar_layer_frame_range_set(this, layer, 1, 0, looped);
        self->blend_time = 0.0f;
        self->blend_timer = 0.0f;
        self->blend->SetWeight(layer, 1.0f);
        self->weight[layer] = 1.0f;
        self->frame[layer] = self->blend->GetFrame();
        self->prev_frame[layer] = self->frame[layer];
        self->frame_set = 1;
        self->scn_mdl->SetAnmObj(self->blend, nw4r::g3d::ScnMdlSimple::ANMOBJTYPE_CHR);
    }
}

/* The animation resource a resource animation plays (nw4r's inline `AnmObjChrRes::GetResAnm`, kept here). */
extern "C" u32 anm_obj_chr_res_get_res(nw4r::g3d::AnmObjChrRes* anm)
{
    return (u32)anm->mRes.mpData;
}

/* 0x800E0DE0 (0x3D8): Starts `anm` on layer 0; with `blend` frames the playing layers move to the fade-out
 * slots 3..5 and cross-fade, otherwise they are detached (and destroyed when `destroy` is set). */
extern "C" void mhchar_motion_start(MHchar* chr, nw4r::g3d::AnmObjChrRes* anm, s32 blend, f32 start, s32 loop,
                                    u32 flags, f32 rate, u8 destroy)
{
    MHcharModel* self = reinterpret_cast<MHcharModel*>(chr);
    int i;
    f32 fade;
    u8 looped;
    nw4r::g3d::ResAnmChr res;

    if (self->scn_mdl) {
        anm->Bind(self->mdl);
        for (i = 0; i < 3; i++) {
            nw4r::g3d::AnmObjChrRes* old = self->blend->Detach(i + 3);
            self->layer_anm[i + 3] = 0;
            if (old && destroy) {
                old->Destroy();
            }
        }
        self->motion_flags = flags;
        fade = blend;
        if (fade < 0.0f) {
            fade *= -1.0f;
            self->motion_flags = flags | 2;
        }
        res_anm_chr_copy_ctor(&res, (nw4r::g3d::ResAnmChr*)res_handle_temp(anm_obj_chr_res_get_res(anm)));
        if (res.GetAnmPolicy() != nw4r::g3d::ANM_POLICY_LOOP) {
            looped = 0;
        } else {
            anm_obj_chr_res_set_play_policy(anm, nw4r::g3d::GetAnmPlayPolicy(nw4r::g3d::ANM_POLICY_ONETIME));
            looped = 1;
        }
        anm->SetFrame(start);
        self->rate[0] = 2.0f * rate;
        anm->SetUpdateRate(self->rate[0]);
        for (i = 0; i < 3; i++) {
            nw4r::g3d::AnmObjChrRes* old;
            if (i == 0) {
                old = self->blend->Attach(0, anm);
                self->layer_anm[0] = anm;
            } else {
                old = self->blend->Detach(i);
            }
            if (old && fade > 0.0f) {
                self->blend_timer = 2.0f + fade;
                self->blend->Attach(i + 3, old);
                self->blend->SetWeight(i + 3, 1.0f);
                self->loop_frame[i + 3] = self->loop_frame[i];
                self->end_frame[i + 3] = self->end_frame[i];
                self->rate[i + 3] = self->rate[i];
                self->frame[i + 3] = self->frame[i];
                self->prev_frame[i + 3] = self->frame[i];
                self->layer_anm[i + 3] = old;
                self->weight[i + 3] = self->weight[i];
                old->SetUpdateRate(0.0f);
                if (mhchar_motion_flag_ck(chr, 1) == 0) {
                    anm->SetUpdateRate(0.0f);
                }
            } else {
                if (i == 0) {
                    self->blend_timer = 0.0f;
                    self->motion_flags &= ~3;
                }
                if (old && destroy) {
                    old->Destroy();
                }
            }
            if (i == 0) {
                self->weight[0] = 1.0f;
            } else {
                self->weight[i] = 0.0f;
            }
        }
        mhchar_layer_frame_range_set(chr, 0, 0, loop, looped);
        self->blend_time = self->blend_timer;
        self->blend->SetWeight(0, 1.0f);
        self->prev_frame[0] = self->frame[0] = self->blend->GetFrame();
        self->frame_set = 1;
        self->scn_mdl->SetAnmObj(self->blend, nw4r::g3d::ScnMdlSimple::ANMOBJTYPE_CHR);
    }
}

/* Set the resource animation's frame policy (nw4r's inline `AnmObjChrRes::SetPlayPolicy`, kept here). */
extern "C" void anm_obj_chr_res_set_play_policy(nw4r::g3d::AnmObjChrRes* anm, nw4r::g3d::PlayPolicyFunc policy)
{
    frame_ctrl_set_play_policy(&(nw4r::g3d::FrameCtrl&)*anm, policy);
}

/* 0x800E11C0 (0xC0): Starts bank motion `motion` (bank `kind`, set `index`) as the model's layer-0 motion. */
extern "C" void mhchar_motion_set(MHchar* chr, u32 kind, s32 index, u16 motion, s32 blend, f32 start, s32 loop,
                                  u32 flags, f32 rate)
{
    MHcharModel* self = reinterpret_cast<MHcharModel*>(chr);

    if (self->mdl.IsValid()) {
        nw4r::g3d::AnmObjChrRes* anm = mhchar_bank_anm_create(kind, index, motion, &self->mdl);
        if (anm && self->blend) {
            self->motion_no[0] = motion;
            mhchar_motion_start(chr, anm, blend, start, loop, flags, rate, 1);
        }
    }
}

/* 0x800E1280 (0x4C): Starts the model's own motion `motion` (built by `mhchar_model_create`) on layer 0. */
extern "C" void mhchar_motion_start_no(MHchar* chr, s32 layer, u16 motion, s32 blend, f32 start, s32 loop,
                                       u32 flags, f32 rate)
{
    MHcharModel* self = reinterpret_cast<MHcharModel*>(chr);

    if (self->motion_res[motion] && self->blend) {
        self->motion_no[layer] = motion;
        return mhchar_motion_start(chr, self->motion_res[motion], blend, start, loop, flags, rate, 0);
    }
}

/* 0x800E12CC (0x274): Starts bank motion `motion` on layer `layer` (0..2); layer 0 goes through
 * `mhchar_motion_set`. */
extern "C" void mhchar_motion_layer_set(MHchar* chr, s32 layer, u32 kind, s32 index, u16 motion, s32 hold,
                                        f32 start, s32 loop, u32 flags, f32 rate)
{
    MHcharModel* self = reinterpret_cast<MHcharModel*>(chr);
    u8 looped;

    if (self->scn_mdl) {
        if (layer == 0) {
            mhchar_motion_set(chr, kind, index, motion, hold, start, loop, flags, rate);
        } else if (layer < 3) {
            nw4r::g3d::AnmObjChrRes* anm = mhchar_bank_anm_create(kind, index, motion, &self->mdl);
            if (anm && self->blend) {
                anm->Bind(self->mdl);
                self->motion_no[layer] = motion;
                nw4r::g3d::ResAnmChr res;
                res_anm_chr_copy_ctor(&res, (nw4r::g3d::ResAnmChr*)res_handle_temp(anm_obj_chr_res_get_res(anm)));
                if (res.GetAnmPolicy() != nw4r::g3d::ANM_POLICY_LOOP) {
                    looped = 0;
                } else {
                    anm_obj_chr_res_set_play_policy(anm,
                                                    nw4r::g3d::GetAnmPlayPolicy(nw4r::g3d::ANM_POLICY_ONETIME));
                    looped = 1;
                }
                anm->SetFrame(start);
                self->rate[layer] = 2.0f * rate;
                if (mhchar_motion_flag_ck(chr, 1) == 0 && hold) {
                    anm->SetUpdateRate(0.0f);
                } else {
                    anm->SetUpdateRate(self->rate[layer]);
                }
                nw4r::g3d::AnmObjChrRes* old = self->blend->Attach(layer, anm);
                if (old) {
                    old->Destroy();
                }
                self->layer_anm[layer] = anm;
                mhchar_layer_frame_range_set(chr, layer, 0, loop, looped);
                self->blend->SetWeight(layer, 1.0f);
                self->weight[layer] = 0.0f;
                self->frame[layer] = self->blend->GetFrame();
                self->prev_frame[layer] = self->frame[layer];
                self->scn_mdl->SetAnmObj(self->blend, nw4r::g3d::ScnMdlSimple::ANMOBJTYPE_CHR);
            }
        }
    }
}

/* 0x800E1540 (0x100): Sets layer `layer`'s end frame from its animation and its loop frame: none for a
 * one-time animation, the end with `to_end`, otherwise `loop` (0 when negative). */
extern "C" void mhchar_layer_frame_range_set(MHchar* chr, s32 layer, s32 to_end, s32 loop, u8 looped)
{
    MHcharModel* self = reinterpret_cast<MHcharModel*>(chr);
    nw4r::g3d::ResAnmChr res;
    u32 handle = anm_obj_chr_res_get_res(self->layer_anm[layer]);
    res_anm_chr_copy_ctor(&res, (nw4r::g3d::ResAnmChr*)&handle);

    self->end_frame[layer] = res.GetNumFrame();
    if ((int)looped != 1) {
        self->loop_frame[layer] = -1.0f;
    } else if (to_end == 0) {
        if (loop < 0) {
            self->loop_frame[layer] = 0.0f;
        } else {
            self->loop_frame[layer] = loop;
        }
    } else {
        self->loop_frame[layer] = res.GetNumFrame();
    }
}

/* 0x800E1640 (0x9C): Sets the layer-0 rate from `speed` and, outside a cross-fade, hands it to every layer. */
extern "C" void mhchar_motion_speed_set(MHchar* chr, f32 speed)
{
    MHcharModel* self = reinterpret_cast<MHcharModel*>(chr);
    int i;

    self->rate[0] = 2.0f * speed;
    if (!(self->blend_timer > 0.0f)) {
        for (i = 0; i < 6; i++) {
            nw4r::g3d::AnmObjChr* anm = mhchar_layer_anm_get(chr, i);
            if (anm) {
                self->rate[i] = self->rate[0];
                anm->SetUpdateRate(self->rate[i]);
            }
        }
    }
}

/* 0x800E16DC (0x27C): Tests layer 0's frame step against `a`/`b` by `mode` (0: `a` crossed, 1/6: reached,
 * 2: not reached, 3/7: inside [a, b], 4: inside (a, b], 5: inside [a, b)); `reverse` reads the step backwards. */
extern "C" u32 mhchar_frame_check(MHchar* chr, u16 mode, u16 reverse, f32 a, f32 b)
{
    MHcharModel* self = reinterpret_cast<MHcharModel*>(chr);
    int wrapped = 0;
    f32 rate = self->rate[0];
    f32 timer = self->blend_timer;
    int fading = 0;
    int hold = 0;
    f32 prev;
    f32 cur;

    if (reverse == 0) {
        if (timer > 0.0f) {
            fading = 1;
            if (timer - rate <= 0.0f) {
                fading = 1;
            }
        }
    } else if (timer > 0.0f) {
        fading = 1;
    }
    if ((u32)(mode - 6) <= 1 || (s32)mode == 2) {
        if (fading && mhchar_motion_flag_ck(chr, 1) == 0) {
            hold = 1;
        }
    } else if (fading && mhchar_motion_flag_ck(chr, 1) == 0) {
        return 0;
    }
    if (hold) {
        prev = self->frame[0];
        cur = prev;
    } else if (reverse == 0) {
        prev = self->frame[0];
        cur = prev + self->rate[0];
        if (cur > self->end_frame[0]) {
            cur = self->loop_frame[0];
            if (cur < 0.0f) {
                cur = self->end_frame[0];
            } else {
                wrapped = 1;
            }
        }
    } else {
        prev = self->prev_frame[0];
        cur = self->frame[0];
        if (cur < prev) {
            wrapped = 1;
        }
    }
    switch (mode) {
    case 0:
        if (wrapped == 0) {
            if (a > prev && a <= cur) {
                return 1;
            }
        } else if ((a <= cur && a >= self->loop_frame[0]) || a > prev) {
            return 1;
        }
        break;
    case 1:
    case 6:
        if (cur >= a) {
            return 1;
        }
        break;
    case 2:
        if (cur < a) {
            return 1;
        }
        break;
    case 3:
    case 7:
        if (cur >= a && cur <= b) {
            return 1;
        }
        break;
    case 4:
        if (cur > a && cur <= b) {
            return 1;
        }
        break;
    case 5:
        if (cur >= a && cur < b) {
            return 1;
        }
        break;
    }
    return 0;
}

/* Whether the model's motion flags intersect `mask`. */
extern "C" u32 mhchar_motion_flag_ck(MHchar* self, u32 mask)
{
    return (self->field_0x4C & mask) != 0;
}

/* 0x800E1970 (0x2BC): Advances the motion layers one frame: folds each layer's frame at its end, sums the
 * root-node translation the step moved, and moves the model by it (scaled and rotated). */
extern "C" void mhchar_motion_update(MHchar* chr)
{
    MHcharModel* self = reinterpret_cast<MHcharModel*>(chr);
    nw4r::g3d::ChrAnmResult result;
    nw4r::math::VEC3 move;
    nw4r::math::VEC3 delta;
    nw4r::math::VEC3 scaled;
    int i;

    if (self->blend && mhchar_layer_anm_get(chr, 0)) {
    if (self->anim_skip != 0) {
        self->anim_skip--;
        return;
    }
    chr_anm_result_init(&result);
    VEC3_ctor(&move);
    VEC3_ctor(&delta);
    self->prev_frame[0] = self->frame[0];
    self->prev_frame[1] = self->frame[1];
    self->prev_frame[2] = self->frame[2];
    self->prev_frame[3] = self->frame[3];
    self->prev_frame[4] = self->frame[4];
    self->prev_frame[5] = self->frame[5];
    if (self->frame_set == 0) {
        self->blend->UpdateFrame();
    } else {
        self->frame_set = 0;
    }
    mhchar_layer_weight_update(chr);
    move.x = 0.0f;
    move.y = 0.0f;
    move.z = 0.0f;
    for (i = 0; i < 6; i++) {
        nw4r::g3d::AnmObjChr* anm = mhchar_layer_anm_get(chr, i);
        if (anm) {
            self->frame[i] = anm->GetFrame();
            if (self->frame[i] > self->end_frame[i]) {
                if (self->loop_frame[i] < 0.0f) {
                    self->frame[i] = self->end_frame[i];
                } else {
                    anm->SetFrame(self->loop_frame[i]);
                    self->frame[i] = anm->GetFrame();
                }
            }
            f32 step = self->rate[i];
            f32 from = anm->GetFrame();
            f32 to = from + step;
            if (to >= self->end_frame[i]) {
                to = from;
                from = from - step;
            }
            if (from < 0.0f) {
                from = 0.0f;
            }
            if (to < 0.0f) {
                to = 0.0f;
            }
            mhchar_layer_trans_delta_get(chr, &delta, i, from, to);
            vec3_scale(&scaled, &delta, self->blend->GetWeight(i));
            addVec3To(&move, &scaled);
        }
    }
    if (self->move_skip != 0) {
        self->move_skip--;
        return;
    }
    if (mhchar_motion_flag_ck(chr, 2) != 1) {
        move.x *= self->scale.x;
        move.y *= self->scale.y;
        move.z *= self->scale.z;
        rotVecZXY(&move, &self->rot);
        addVec3To(&self->pos, &move);
    }
    }
}

/* Layer `layer`'s animation (the blend's child). */
extern "C" nw4r::g3d::AnmObjChr* mhchar_layer_anm_get(MHchar* chr, s32 layer)
{
    MHcharModel* self = reinterpret_cast<MHcharModel*>(chr);
    return anm_obj_chr_node_get_child(self->blend, layer);
}

/* Child `idx` of the node, asserting the index (nw4r's inline accessor, kept here). */
extern "C" nw4r::g3d::AnmObjChr* anm_obj_chr_node_get_child(nw4r::g3d::AnmObjChrNode* node, int idx)
{
    int valid = 0;
    if (idx >= 0 && idx <= node->mChildrenArraySize - 1) {
        valid = 1;
    }
    if (!valid) {
        nw4r::db::Panic("g3d_anmchr.h", 0x98, "idx is out of bounds(%d)\n%d <= idx <= %d not satisfied.", idx, 0,
                        node->mChildrenArraySize - 1);
    }
    return node->mpChildrenArray[idx];
}

/* 0x800E1CC8 (0x188): The root-node translation layer `layer` moves between frames `from` and `to`. */
extern "C" void mhchar_layer_trans_delta_get(MHchar* chr, nw4r::math::VEC3* out, s32 layer, f32 from, f32 to)
{
    MHcharModel* self = reinterpret_cast<MHcharModel*>(chr);
    nw4r::g3d::ChrAnmResult result;
    nw4r::math::VEC3 at_from;
    nw4r::math::VEC3 at_to;
    nw4r::math::VEC3 diff;

    chr_anm_result_init(&result);
    VEC3_ctor(&at_from);
    VEC3_ctor(&at_to);
    nw4r::g3d::AnmObjChr* anm = mhchar_layer_anm_get(chr, layer);
    if (anm == 0) {
        out->x = 0.0f;
        out->y = 0.0f;
        out->z = 0.0f;
        return;
    }
    f32 frame = anm->GetFrame();
    anm->SetFrame(from);
    anm_chr_result_trans_get((struct ResAnmChrObj*)anm->GetResult(&result, self->root_node_id), &at_from.x);
    anm->SetFrame(to);
    anm_chr_result_trans_get((struct ResAnmChrObj*)anm->GetResult(&result, self->root_node_id), &at_to.x);
    subVec3(&diff, &at_to, &at_from);
    copyVec3(out, &diff);
    anm->SetFrame(frame);
}

/* Destroys a detached fade animation unless it is one of the model's own motions (those stay alive). */
inline void mhchar_fade_anm_destroy(MHcharModel* self, nw4r::g3d::AnmObjChrRes* anm)
{
    for (int i = 0; i < 16; i++) {
        if (anm == self->motion_res[i]) {
            return;
        }
    }
    anm->Destroy();
}

/* 0x800E1E50 (0x348): Steps the cross-fade: fades the outgoing layers 3..5 by a Hermite curve (detaching them
 * when it ends) and spreads the remaining weight over the playing layers 0..2. */
extern "C" void mhchar_layer_weight_update(MHchar* chr)
{
    MHcharModel* self = reinterpret_cast<MHcharModel*>(chr);
    f32 fade_weight;
    f32 total;
    int i;

    if (self->blend == 0) {
        return;
    }
    fade_weight = 0.0f;
    if (mhchar_layer_anm_get(chr, 3)) {
        self->blend_timer -= self->rate[0];
        if (self->blend_timer <= fade_weight) {
            self->blend_timer = fade_weight;
            for (i = 0; i < 3; i++) {
                nw4r::g3d::AnmObjChrRes* old = self->blend->Detach(i + 3);
                self->layer_anm[i + 3] = 0;
                if (old) {
                    mhchar_fade_anm_destroy(self, old);
                }
                nw4r::g3d::AnmObjChr* anm = mhchar_layer_anm_get(chr, i);
                if (anm) {
                    anm->SetUpdateRate(self->rate[i]);
                }
            }
            self->motion_flags &= ~3;
        } else {
            if (self->blend_timer > self->blend_time) {
                fade_weight = 0.0f;
            } else {
                fade_weight = hermite_interp(self->blend_timer / self->blend_time, fade_weight, fade_weight,
                                             fade_weight, 1.0f, 1.0f, fade_weight);
            }
            total = 0.0f;
            for (i = 0; i < 3; i++) {
                if (mhchar_layer_anm_get(chr, i + 3)) {
                    total += self->weight[i + 3];
                }
            }
            for (i = 0; i < 3; i++) {
                if (mhchar_layer_anm_get(chr, i + 3) && total > 0.0f) {
                    self->blend->SetWeight(i + 3, fade_weight * (self->weight[i + 3] / total));
                } else {
                    self->blend->SetWeight(i + 3, 0.0f);
                }
            }
        }
    }
    total = 0.0f;
    fade_weight = 1.0f - fade_weight;
    for (i = 0; i < 3; i++) {
        if (mhchar_layer_anm_get(chr, i)) {
            total += self->weight[i];
        }
    }
    for (i = 0; i < 3; i++) {
        if (mhchar_layer_anm_get(chr, i) && total > 0.0f) {
            self->blend->SetWeight(i, fade_weight * (self->weight[i] / total));
        } else {
            self->blend->SetWeight(i, 0.0f);
        }
    }
}

/* 0x800E2198 (0x90): Whether layer `layer`'s next step passes its end frame. */
extern "C" u32 mhchar_motion_end_ck(MHchar* chr, s32 layer)
{
    MHcharModel* self = reinterpret_cast<MHcharModel*>(chr);
    f32 frame = self->blend->GetFrame();
    return frame + self->blend->GetUpdateRate() > self->end_frame[layer];
}

/* 0x800E2228 (0x388): Builds the model from `file`: the scene model with the options `flags` selects, the
 * motion blend, the file's own motions (by the number after '_' in their names), the materials' light set and
 * the root node. */
extern "C" void mhchar_model_create(MHchar* chr, nw4r::g3d::ResFile* file, u32 flags, s32 light)
{
    MHcharModel* self = reinterpret_cast<MHcharModel*>(chr);
    nw4r::g3d::ResAnmChr anm;
    res_anm_chr_ctor(&anm, 0);

    res_file_assign((u32*)&self->file, (u32*)file);
    res_file_init(&self->file);
    res_file_bind_self(&self->file);
    res_mdl_copy_ctor((struct ResHandle*)&self->mdl, res_handle_temp(res_file_get_res_mdl(&self->file, 0)));

    u32 option = 0;
    u32 opa = flags & 8;
    if (opa) {
        option |= 0x40;
    }
    if (flags & 4) {
        option |= 8;
    }
    if (flags & 0x10) {
        option |= 0x100;
    }
    u32 lit = flags & 0x20;
    if (lit) {
        option |= 0x20;
    }
    if (flags & 0x40) {
        option |= 0x80;
    }
    u32 xlu = flags & 0x100;
    if (xlu) {
        option |= 0x204;
    }
    if (flags & 0x200) {
        option |= 0x10;
    }
    self->scn_work = nw_scn_mdl_create(self->mdl, option, 1);
    self->scn_mdl = self->scn_work->scn_mdl;
    if (opa) {
        self->scn_mdl->SetScnObjOption(0x30001, 0);
    }
    if (xlu) {
        self->scn_mdl->SetScnObjOption(0x204, 0);
    }
    if (flags & 0x80) {
        self->scn_mdl->SetScnObjOption(0x20001, 1);
    }
    do {
        if (flags & 1) {
            self->blend = nw_anm_chr_blend_create(self->mdl, 6);
        } else {
            self->blend = 0;
            self->motion_num = 0;
            break;
        }
        if (res_file_has_anm_chr(&self->file)) {
            if (flags & 2) {
                u32 i;
                int first = -1;
                self->motion_num = res_file_get_res_anm_chr_num(&self->file);
                for (i = 0; i < self->motion_num; i++) {
                    res_anm_chr_assign(&anm, (u32*)res_handle_temp(res_file_get_res_anm_chr(&self->file, i)));
                    nw4r::g3d::AnmObjChrRes* res = nw_anm_chr_res_create(anm, self->mdl, false);
                    res->Bind(self->mdl);
                    const char* num = strstr(res_anm_chr_get_name(&anm), "_");
                    if (num) {
                        int no = atoi(num + 1);
                        self->motion_res[no] = res;
                        if (first == -1) {
                            first = no;
                        }
                    }
                }
            }
        } else {
            self->motion_num = 0;
        }
    } while (0);
    if (lit && light) {
        u32 num = self->mdl.GetResMatNumEntries();
        for (u32 i = 0; i < num; i++) {
            mhchar_mat_hide_set(chr, i, light);
        }
    }
    for (int i = 0; i < 6; i++) {
        self->weight[i] = 0.0f;
        self->rate[i] = 0.0f;
    }
    if (self->scn_mdl && self->blend) {
        self->scn_mdl->SetCalcWorldCallback(self->world_callback);
    }
    nw4r::g3d::ResNode node(0);
    res_node_assign((struct ResHandle*)&node, res_handle_temp((u32)self->mdl.GetResNode("NULL").mpData));
    if (res_node_is_valid((struct ResHandle*)&node) == 1) {
        self->root_node_id = res_node_get_id((struct ResHandle*)&node);
    } else {
        self->root_node_id = 0;
    }
}

/* 0x800E25B0 (0x8): Stores the world-matrix callback (nw4r's inline setter, kept here). */
void nw4r::g3d::ScnMdlSimple::SetCalcWorldCallback(ICalcWorldCallback* pCallback)
{
    mpCalcWorldCallback = pCallback;
}

/* The animation's name (nw4r's inline `ResAnmChr::GetName`, kept here). */
extern "C" const char* res_anm_chr_get_name(const nw4r::g3d::ResAnmChr* res)
{
    return res_anm_chr_ofs_to_ptr(res, res->ref().toName);
}

/* The resource block's base plus `ofs`, or 0 when `ofs` is 0 (nw4r's inline `ofs_to_ptr`, kept here). */
extern "C" const char* res_anm_chr_ofs_to_ptr(const nw4r::g3d::ResAnmChr* res, s32 ofs)
{
    u8* base = (u8*)res->mpData;
    if (ofs != 0) {
        return (const char*)(base + ofs);
    }
    return 0;
}

/* Copies the handle word into `dst` and returns `dst` (the handle's assignment). */
extern "C" nw4r::g3d::ResAnmChr* res_anm_chr_assign(nw4r::g3d::ResAnmChr* dst, u32* src)
{
    res_anm_chr_common_assign(dst, src);
    return dst;
}

/* Copies the handle word into `dst`. */
extern "C" void res_anm_chr_common_assign(nw4r::g3d::ResAnmChr* dst, u32* src)
{
    dst->mpData = (nw4r::g3d::ResAnmChrData*)*src;
}

/* Binds the file against itself (nw4r's inline `ResFile::Bind()`, kept here). */
extern "C" void res_file_bind_self(nw4r::g3d::ResFile* file)
{
    nw4r::g3d::ResFile self = *file;
    res_file_bind(file, &self);
}

/* Releases the model's resource file. */
extern "C" void mhchar_file_release(MHchar* chr)
{
    MHcharModel* self = reinterpret_cast<MHcharModel*>(chr);
    return res_file_release(&self->file);
}

/* Cleans up and terminates a resource file. */
extern "C" void res_file_release(nw4r::g3d::ResFile* file)
{
    CleanUpTracks(file);
    return res_file_terminate(file);
}

/* Write the model's +0xD4 float block at `index`. */
extern "C" void mhchar_mat_blend_set(MHchar* self, u32 index, f32 value)
{
    self->field_0xD4[index] = value;
}

/* 0x800E26C4 (0x220): Releases the motions: detaches and destroys the blend's children and the blend, clears
 * the layers and destroys the model's own motions. */
extern "C" void mhchar_model_release(MHchar* chr)
{
    MHcharModel* self = reinterpret_cast<MHcharModel*>(chr);
    int i;

    if (self->blend) {
        for (i = 0; i < 3; i++) {
            nw4r::g3d::AnmObjChrRes* old = self->blend->Detach(i);
            if (old) {
                int j;
                for (j = 0; j < 16; j++) {
                    if (self->motion_res[j] == old) {
                        self->motion_res[j] = 0;
                    }
                }
                old->Destroy();
            }
        }
        self->blend->Destroy();
        self->blend = 0;
    }
    for (i = 0; i < 6; i++) {
        self->motion_no[i] = 0;
        self->rate[i] = 0.0f;
        self->frame[i] = 0.0f;
        self->prev_frame[i] = 0.0f;
        self->end_frame[i] = 0.0f;
        self->loop_frame[i] = 0.0f;
        self->weight[i] = 0.0f;
        self->layer_anm[i] = 0;
    }
    for (i = 0; i < 16; i++) {
        if (self->motion_res[i]) {
            self->motion_res[i]->Destroy();
            self->motion_res[i] = 0;
        }
    }
}

/* The model's +0x114 joint-list handle. */
extern "C" u32 mhchar_mat_num_get(MHchar* self)
{
    return reinterpret_cast<const nw4r::g3d::ResMdl*>(&self->field_0x114)->GetResMatNumEntries();
}

/* 0x800E28EC (0xA8): Sets material `index`'s channel material colour (the copied material, marked dirty with
 * `keep`). */
void MHchar::setMatColor(u32 index, _GXChannelID channel, _GXColor color, bool keep)
{
    MHcharModel* self = reinterpret_cast<MHcharModel*>(this);

    if (self->scn_mdl) {
        nw4r::g3d::ScnMdl::CopiedMatAccess access(self->scn_mdl, index);
        if (copied_mat_access_valid_ck(&access)) {
            struct ResHandle chan;
            u32 handle = scn_mdl_mat_access_get_chan(&access, keep);
            res_mat_chan_copy_ctor((struct ResHandle*)&chan, (struct ResHandle*)&handle);
            reinterpret_cast<nw4r::g3d::ResMatChan&>(chan).GXSetChanMatColor(channel, color);
            res_mat_chan_end_edit((struct ResHandle*)&chan);
        }
    }
}

/* Whether the copied-material access resolved a material. */
extern "C" u32 copied_mat_access_valid_ck(nw4r::g3d::ScnMdl::CopiedMatAccess* access)
{
    return access->mpScnMdl != 0;
}

/* 0x800E29A8 (0x78): Sets every material's channel material colour. */
extern "C" void mhchar_mat_color_set_all(MHchar* chr, _GXChannelID channel, _GXColor* color, bool keep)
{
    u32 num = mhchar_mat_num_get(chr);
    for (u32 i = 0; i < num; i++) {
        chr->setMatColor(i, channel, *color, keep);
    }
}

/* 0x800E2A20 (0xD4): Sets every material's channel alpha to `alpha`: opaque materials drop their copied blocks,
 * the others blend. */
extern "C" void mhchar_mat_alpha_set(MHchar* chr, _GXChannelID channel, u8 alpha, bool keep)
{
    u32 num = mhchar_mat_num_get(chr);
    for (u32 i = 0; i < num; i++) {
        _GXColor color;
        if (chr->getMatColor(i, channel, &color)) {
            color.a = alpha;
            chr->setMatColor(i, channel, color, keep);
            if (alpha == 0xFF) {
                mhchar_mat_buffer_clean(chr, i, 0x80);
            } else {
                chr->setMatAlphaBlendMode(i, (_GXBlendMode)1, (_GXBlendFactor)4, (_GXBlendFactor)5, (_GXLogicOp)0);
            }
        }
    }
}

/* 0x800E2AF4 (0x9C): Reads material `index`'s channel material colour; 0 without a model or material. */
u32 MHchar::getMatColor(u32 index, _GXChannelID channel, _GXColor* out)
{
    MHcharModel* self = reinterpret_cast<MHcharModel*>(this);

    if (self->scn_mdl == 0) {
        return 0;
    }
    nw4r::g3d::ScnMdl::CopiedMatAccess access(self->scn_mdl, index);
    if (copied_mat_access_valid_ck(&access)) {
        struct ResHandle chan;
        u32 handle = scn_mdl_mat_access_get_chan(&access, false);
        res_mat_chan_copy_ctor((struct ResHandle*)&chan, (struct ResHandle*)&handle);
        return reinterpret_cast<nw4r::g3d::ResMatChan&>(chan).GXGetChanMatColor(channel, out);
    }
    return 0;
}

/* 0x800E2B90 (0xA8): Sets material `index`'s channel ambient colour. */
void MHchar::setAmbColor(u32 index, _GXChannelID channel, _GXColor color, bool keep)
{
    MHcharModel* self = reinterpret_cast<MHcharModel*>(this);

    if (self->scn_mdl) {
        nw4r::g3d::ScnMdl::CopiedMatAccess access(self->scn_mdl, index);
        if (copied_mat_access_valid_ck(&access)) {
            struct ResHandle chan;
            u32 handle = scn_mdl_mat_access_get_chan(&access, keep);
            res_mat_chan_copy_ctor((struct ResHandle*)&chan, (struct ResHandle*)&handle);
            reinterpret_cast<nw4r::g3d::ResMatChan&>(chan).GXSetChanAmbColor(channel, color);
            res_mat_chan_end_edit((struct ResHandle*)&chan);
        }
    }
}

/* 0x800E2C38 (0x9C): Reads material `index`'s channel ambient colour; 0 without a model or material. */
u32 MHchar::getAmbColor(u32 index, _GXChannelID channel, _GXColor* out)
{
    MHcharModel* self = reinterpret_cast<MHcharModel*>(this);

    if (self->scn_mdl == 0) {
        return 0;
    }
    nw4r::g3d::ScnMdl::CopiedMatAccess access(self->scn_mdl, index);
    if (copied_mat_access_valid_ck(&access)) {
        struct ResHandle chan;
        u32 handle = scn_mdl_mat_access_get_chan(&access, false);
        res_mat_chan_copy_ctor((struct ResHandle*)&chan, (struct ResHandle*)&handle);
        return reinterpret_cast<nw4r::g3d::ResMatChan&>(chan).GXGetChanAmbColor(channel, out);
    }
    return 0;
}

/* 0x800E2CD4 (0x8C): Reads material `index`'s TEV constant colour `id`. */
void MHchar::getTevKColor(u32 index, _GXTevKColorID id, _GXColor* out)
{
    MHcharModel* self = reinterpret_cast<MHcharModel*>(this);

    if (self->scn_mdl) {
        nw4r::g3d::ScnMdl::CopiedMatAccess access(self->scn_mdl, index);
        if (copied_mat_access_valid_ck(&access)) {
            struct ResHandle tev;
            u32 handle = scn_mdl_mat_access_get_tev_color(&access, false);
            res_mat_tev_color_copy_ctor((struct ResHandle*)&tev, (struct ResHandle*)&handle);
            reinterpret_cast<nw4r::g3d::ResMatTevColor&>(tev).GXGetTevKColor(id, out);
        }
    }
}

/* 0x800E2D60 (0x9C): Sets material `index`'s TEV constant colour `id`. */
void MHchar::setTevKColor(u32 index, _GXTevKColorID id, _GXColor* color)
{
    MHcharModel* self = reinterpret_cast<MHcharModel*>(this);

    if (self->scn_mdl) {
        nw4r::g3d::ScnMdl::CopiedMatAccess access(self->scn_mdl, index);
        if (copied_mat_access_valid_ck(&access)) {
            struct ResHandle tev;
            u32 handle = scn_mdl_mat_access_get_tev_color(&access, false);
            res_mat_tev_color_copy_ctor((struct ResHandle*)&tev, (struct ResHandle*)&handle);
            reinterpret_cast<nw4r::g3d::ResMatTevColor&>(tev).GXSetTevKColor(id, *color);
            res_mat_tev_color_end_edit((struct ResHandle*)&tev);
        }
    }
}

/* 0x800E2DFC (0x84): Sets material `index`'s TEV stage count. */
void MHchar::setNumTevStages(u32 index, u8 count)
{
    MHcharModel* self = reinterpret_cast<MHcharModel*>(this);

    if (self->scn_mdl) {
        nw4r::g3d::ScnMdl::CopiedMatAccess access(self->scn_mdl, index);
        if (copied_mat_access_valid_ck(&access)) {
            struct ResHandle mode;
            u32 handle = scn_mdl_mat_access_get_gen_mode(&access, false);
            res_gen_mode_copy_ctor((struct ResHandle*)&mode, &handle);
            reinterpret_cast<nw4r::g3d::ResGenMode&>(mode).GXSetNumTevStages(count);
            res_gen_mode_end_edit((struct ResHandle*)&mode);
        }
    }
}

/* Copy-constructs a gen-mode handle and returns it. */
extern "C" struct ResHandle* res_gen_mode_copy_ctor(struct ResHandle* dst, u32* src)
{
    res_gen_mode_common_copy_ctor(dst, src);
    return dst;
}

/* Copies the handle word into `dst`. */
extern "C" void res_gen_mode_common_copy_ctor(struct ResHandle* dst, u32* src)
{
    dst->mpData = (void*)*src;
}

/* 0x800E2EBC (0x84): Sets material `index`'s cull mode. */
extern "C" void mhchar_cull_mode_set(MHchar* chr, u32 index, _GXCullMode cull)
{
    MHcharModel* self = reinterpret_cast<MHcharModel*>(chr);

    if (self->scn_mdl) {
        nw4r::g3d::ScnMdl::CopiedMatAccess access(self->scn_mdl, index);
        if (copied_mat_access_valid_ck(&access)) {
            struct ResHandle mode;
            u32 handle = scn_mdl_mat_access_get_gen_mode(&access, false);
            res_gen_mode_copy_ctor((struct ResHandle*)&mode, &handle);
            reinterpret_cast<nw4r::g3d::ResGenMode&>(mode).GXSetCullMode(cull);
            res_gen_mode_end_edit((struct ResHandle*)&mode);
        }
    }
}

/* 0x800E2F40 (0xAC): Sets material `index`'s alpha compare. */
extern "C" void mhchar_mat_tev_set(MHchar* chr, u32 index, _GXCompare comp0, u8 ref0, _GXAlphaOp op,
                                   _GXCompare comp1, u8 ref1)
{
    MHcharModel* self = reinterpret_cast<MHcharModel*>(chr);

    if (self->scn_mdl) {
        nw4r::g3d::ScnMdl::CopiedMatAccess access(self->scn_mdl, index);
        if (copied_mat_access_valid_ck(&access)) {
            struct ResHandle pix;
            u32 handle = scn_mdl_mat_access_get_pix(&access, false);
            res_mat_pix_copy_ctor((struct ResHandle*)&pix, &handle);
            reinterpret_cast<nw4r::g3d::ResMatPix&>(pix).GXSetAlphaCompare(comp0, ref0, op, comp1, ref1);
            res_mat_pix_end_edit((struct ResHandle*)&pix);
        }
    }
}

/* Copy-constructs a pixel-engine handle and returns it. */
extern "C" struct ResHandle* res_mat_pix_copy_ctor(struct ResHandle* dst, u32* src)
{
    res_mat_pix_common_copy_ctor(dst, src);
    return dst;
}

/* Copies the handle word into `dst`. */
extern "C" void res_mat_pix_common_copy_ctor(struct ResHandle* dst, u32* src)
{
    dst->mpData = (void*)*src;
}

/* 0x800E3028 (0xB4): Sets material `index`'s blend mode. */
void MHchar::setMatAlphaBlendMode(u32 index, _GXBlendMode mode, _GXBlendFactor src, _GXBlendFactor dst,
                                  _GXLogicOp op)
{
    MHcharModel* self = reinterpret_cast<MHcharModel*>(this);

    if (self->scn_mdl) {
        nw4r::g3d::ScnMdl::CopiedMatAccess access(self->scn_mdl, index);
        if (copied_mat_access_valid_ck(&access)) {
            struct ResHandle pix;
            u32 handle = scn_mdl_mat_access_get_pix(&access, false);
            res_mat_pix_copy_ctor((struct ResHandle*)&pix, &handle);
            reinterpret_cast<nw4r::g3d::ResMatPix&>(pix).GXSetBlendMode(mode, src, dst, op);
            res_mat_pix_end_edit((struct ResHandle*)&pix);
        }
    }
}

/* Drops material `index`'s copied blocks selected by `option`. */
extern "C" void mhchar_mat_buffer_clean(MHchar* chr, u32 index, u32 option)
{
    MHcharModel* self = reinterpret_cast<MHcharModel*>(chr);
    if (self->scn_mdl) {
        return scn_mdl_clean_mat_buffer(self->scn_mdl, index, option);
    }
}

/* 0x800E30F0 (0x60): Shows or hides node `index`. */
void MHchar::setVisibility(u32 index, bool visible)
{
    MHcharModel* self = reinterpret_cast<MHcharModel*>(this);

    if (self->scn_mdl) {
        ScnMdlVisAccess access;
        scn_mdl_vis_access_ctor(&access, self->scn_mdl, index);
        if (scn_mdl_vis_access_valid_ck(&access)) {
            scn_mdl_vis_access_set_visibility(&access, visible);
        }
    }
}

/* Whether the visibility access resolved a model. */
extern "C" u32 scn_mdl_vis_access_valid_ck(ScnMdlVisAccess* access)
{
    return access->mpScnMdl != 0;
}

/* 0x800E3164 (0x84): Sets material `index`'s light set. */
extern "C" void mhchar_mat_hide_set(MHchar* chr, u32 index, s32 light)
{
    MHcharModel* self = reinterpret_cast<MHcharModel*>(chr);

    if (self->scn_mdl) {
        nw4r::g3d::ScnMdl::CopiedMatAccess access(self->scn_mdl, index);
        if (copied_mat_access_valid_ck(&access)) {
            u32 handle = scn_mdl_mat_access_get_mat_misc(&access, false);
            nw4r::g3d::ResMatMisc misc(*(nw4r::g3d::ResMatMisc*)&handle);
            misc.SetLightSetIdx(light);
            res_mat_misc_end_edit((struct ResHandle*)&misc);
        }
    }
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

/* 0x800E3270 (0x4C): Zeroes the root node's translation, so the motion's root motion moves the record instead. */
void MHcharWorldCallback::ExecCallbackA(nw4r::g3d::ChrAnmResult* pResult, nw4r::g3d::ResMdl mdl,
                                        nw4r::g3d::FuncObjCalcWorld* pFuncObj)
{
    nw4r::math::VEC3 zero;
    VEC3_ctor(&zero);
    zero.x = 0.0f;
    zero.y = 0.0f;
    zero.z = 0.0f;
    anm_chr_result_trans_set((struct ResAnmChrObj*)pResult, &zero.x);
}

/* 0x800E32BC (0x9C): Records the start of hundred-group `group` in bank `kind`'s set `index`. */
extern "C" void mhchar_bank_start_set(s32 kind, s32 index, u16 group, u16 start)
{
    switch (kind) {
    case 0:
        mhchar_bank0_start_tbl[group] = start;
        break;
    case 1:
    case 4:
        mhchar_bank1_start_tbl[index][group] = start;
        break;
    case 2:
        mhchar_bank2_start_tbl[index][group] = start;
        break;
    case 3:
        mhchar_bank3_start_tbl[group] = start;
        break;
    }
}

/* 0x800E3358 (0x250): Loads bank `kind`'s set `index` from `file`: every animation named `<x>_<motion>` is
 * filed under its motion number (hundreds the group, the rest its slot). */
extern "C" void mhchar_res_bank_set(s32 kind, s32 index, nw4r::g3d::ResFile* file)
{
    nw4r::g3d::ResAnmChr anm;
    u16* tbl;
    s32 size;
    res_anm_chr_ctor(&anm, 0);

    if (res_file_has_anm_chr(file)) {
        u16 start = 0;
        u16 slot = 0;
        u16 group = 0;
        switch (kind) {
        case 0:
            res_file_assign((u32*)&mhchar_bank0_file, (u32*)file);
            tbl = mhchar_bank0_anm_tbl;
            size = 400;
            break;
        case 1:
        case 4:
            res_file_assign((u32*)&mhchar_bank1_files[index], (u32*)file);
            tbl = mhchar_bank1_anm_tbl[index];
            size = 300;
            break;
        case 2:
            res_file_assign((u32*)&mhchar_bank2_files[index], (u32*)file);
            tbl = mhchar_bank2_anm_tbl[index];
            size = 150;
            break;
        case 3:
            res_file_assign((u32*)&mhchar_bank3_file, (u32*)file);
            tbl = mhchar_bank3_anm_tbl;
            size = 160;
            break;
        case 5:
            return;
        }
        memset(tbl, 0, size * 2);
        int num = res_file_get_res_anm_chr_num(file);
        for (int i = 0; i < num; i++) {
            u32 entry = res_file_get_res_anm_chr(file, i);
            res_anm_chr_assign(&anm, &entry);
            const char* p = strstr(res_anm_chr_get_name(&anm), "_");
            if (p) {
                int no = atoi(p + 1);
                int motion = no % 1000;
                int hundred = motion / 100;
                for (;;) {
                    if (group == hundred) {
                        break;
                    }
                    mhchar_bank_start_set(kind, index, group, start);
                    start += (u16)(slot + 1);
                    slot = 0;
                    group++;
                }
                slot = motion % 100;
                tbl[start + slot] = i;
            }
        }
        mhchar_bank_start_set(kind, index, group, start);
    }
}

/* 0x800E35A8 (0x278): Looks motion `motion` of bank `kind`'s set `index` up into `out`; 0 for an unknown kind. */
extern "C" u32 mhchar_bank_anm_chr_get(nw4r::g3d::ResAnmChr* out, s32 kind, s32 index, u16 motion)
{
    switch (kind) {
    case 0:
    case 1:
    case 4: {
        u16 no = motion % 1000;
        if (motion < 1000) {
            u32 entry = res_file_get_res_anm_chr(
                &mhchar_bank0_file.file, mhchar_bank0_anm_tbl[no % 100 + mhchar_bank0_start_tbl[no / 100]]);
            res_anm_chr_assign(out, &entry);
        } else {
            u32 entry = res_file_get_res_anm_chr(
                &mhchar_bank1_files[index].file,
                mhchar_bank1_anm_tbl[index][no % 100 + mhchar_bank1_start_tbl[index][no / 100]]);
            res_anm_chr_assign(out, &entry);
        }
        break;
    }
    case 2: {
        u32 entry = res_file_get_res_anm_chr(
            &mhchar_bank2_files[index].file,
            mhchar_bank2_anm_tbl[index][motion % 100 + mhchar_bank2_start_tbl[index][motion / 100]]);
        res_anm_chr_assign(out, &entry);
        break;
    }
    case 3: {
        u32 entry = res_file_get_res_anm_chr(
            &mhchar_bank3_file.file, mhchar_bank3_anm_tbl[motion % 100 + mhchar_bank3_start_tbl[motion / 100]]);
        res_anm_chr_assign(out, &entry);
        break;
    }
    default:
        return 0;
    }
    return 1;
}

/* 0x800E3820 (0x9C): Builds a resource animation of bank motion `motion` on `mdl`; NULL when there is none. */
extern "C" nw4r::g3d::AnmObjChrRes* mhchar_bank_anm_create(u32 kind, s32 index, u16 motion,
                                                           nw4r::g3d::ResMdl* mdl)
{
    nw4r::g3d::ResAnmChr anm;
    res_anm_chr_ctor(&anm, 0);
    if (mhchar_bank_anm_chr_get(&anm, kind, index, motion) == 0) {
        return 0;
    }
    return nw_anm_chr_res_create(anm, *mdl, true);
}

/* 0x800E38BC (0x1AC): Prepares a loaded model's materials: environment-mapped texture coordinates follow
 * camera -1, light 1, and the normal-map indirect stages use reference 1. */
extern "C" void mhchar_mdl_prepare(nw4r::g3d::ResMdl* mdl)
{
    const _GXIndTexMtxID ids[3] = {GX_ITM_0, GX_ITM_1, GX_ITM_2};
    u32 num = mdl->GetResMatNumEntries();

    for (u32 i = 0; i < num; i++) {
        struct ResHandle mat;
        u32 handle = (u32)mdl->GetResMat(i).mpData;
        res_mat_copy_ctor((struct ResHandle*)&mat, (struct ResHandle*)&handle);
        if (reinterpret_cast<nw4r::g3d::ResMat&>(mat).IsValid()) {
            u32 srt_handle = (u32)reinterpret_cast<nw4r::g3d::ResMat&>(mat).GetResTexSrt().mpData;
            nw4r::g3d::ResTexSrt srt;
            res_tex_srt_copy_ctor(&srt, srt_handle);
            u32 tex_num = reinterpret_cast<nw4r::g3d::ResMat&>(mat).GetNumResTexPlttInfo();
            if (srt.IsValid() == 1) {
                for (u32 j = 0; j < tex_num; j++) {
                    u32 map_mode;
                    srt.GetMapMode(j, &map_mode, 0, 0);
                    switch (map_mode) {
                    case 3:
                        srt.SetMapMode(j, map_mode, -1, 1);
                        break;
                    case 4:
                        srt.SetMapMode(j, map_mode, -1, 1);
                        break;
                    }
                }
            }
            u32 misc_handle = (u32)reinterpret_cast<nw4r::g3d::ResMat&>(mat).GetResMatMisc().mpData;
            nw4r::g3d::ResMatMisc misc(*(nw4r::g3d::ResMatMisc*)&misc_handle);
            if (misc.IsValid() == 1) {
                for (u32 j = 0; j < 3; j++) {
                    nw4r::g3d::ResMatMiscData::IndirectMethod method;
                    misc.GetIndirectMethod(ids[j], &method, 0);
                    if (method == 1) {
                        misc.SetIndirectMethod(ids[j], (nw4r::g3d::ResMatMiscData::IndirectMethod)1, 1);
                    }
                }
            }
        }
    }
}

/* 0x800E3AF0 (0x3C): Constructs the callback: the world-callback base, then this class's table. */
MHcharWorldCallback::MHcharWorldCallback()
{
}

/* 0x800E3B2C (0x10): Constructs the world-callback base: stores its table (nw4r's inline constructor, kept here). */
nw4r::g3d::ICalcWorldCallback::ICalcWorldCallback()
{
}

/* The motion banks and the callback object (defined after the code that addresses them, so each is addressed by
 * its own symbol). */
u16 mhchar_bank0_anm_tbl[400];
u16 mhchar_bank0_start_tbl[10];
u16 mhchar_bank1_anm_tbl[4][300];
u16 mhchar_bank1_start_tbl[4][5];
u16 mhchar_bank2_anm_tbl[6][150];
u16 mhchar_bank2_start_tbl[6][5];
u16 mhchar_bank3_anm_tbl[160];
u16 mhchar_bank3_start_tbl[5];
MHcharWorldCallback mhchar_world_callback;
MHcharBankFile mhchar_bank0_file;
MHcharBankFile mhchar_bank1_files[4];
MHcharBankFile mhchar_bank2_files[6];
MHcharBankFile mhchar_bank3_file;
