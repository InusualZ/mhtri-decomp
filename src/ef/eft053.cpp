/* ef/eft053.cpp - the eft053 effect family: one `_EFT` instance driven through its `work_0x38` model block, the
 *   shell-actor lookups it reads from a `_PLW`, its per-state machine and its two slot step machines.
 * RANGE. .text 0x80366618-0x8036A828 (18 functions); extab 0x800177D4-0x80017844, extabindex 0x80037224-0x800372CC,
 *   .rodata 0x80570A10-0x80570A20 (`fn_80369D50`'s arm table), .data 0x805EDAE0-0x805EE078, .sdata2
 *   0x8079B740-0x8079B820 (the range's own pool).  The right edge 0x8036A828, where `menu/menu_item_sub.cpp` starts, is
 *   the registered cut, not a proven seam.
 * FLAGS. `cflags_main`; `#pragma peephole off` around `eft053_shell_pos_project` and `eft053_model_list_get` (retail
 *   keeps `extsb` + `cmpwi`).
 * NAMES. `eft053_get_shell_data` and `eft053_get_model_ang` are the runtime dump's own names; the other eight
 *   definitions are GUESSes named `eft053_<what the body does>` in `ef/eft052.cpp`'s scheme (`eft053_set` seeds
 *   `_EFT::field_0x03 = 53`): `eft053_shell_pos_project` (projects the shell frame into a `VEC3`;
 *   `hud/cockpit_quest.cpp`'s `fn_802EF0A0` uses the result as a position), `eft053_model_list_get` (map and area to
 *   the `lbl_805EDE88` model list and count), `eft053_work_act_ck` (`em_act_ck(work, 13, 0|2|3|4)`), `eft053_set`
 *   (the creation entry, `eft050_set`'s shape without arguments), `eft053_release`/`eft053_dispatch` (its `+0x40`
 *   and `+0x34` hooks), `eft053_slot_rot_step`/`eft053_slot_move_step` (one slot's rotation and translation step),
 *   each a GUESS from its own body.
 * RESIDUALS. 8 rows unwritten: 0x80367124-0x80368A30 (`fn_80367124`, `fn_80367760`, `fn_80368374`) and
 *   0x8036928C-0x8036A828 (`fn_8036928C`, `fn_80369D50`, `fn_8036A690`, `fn_8036A814`, `fn_8036A824`).
 *   8 partial rows; `eft053_release` and `eft053_dispatch` match:
 *  - `eft053_shell_pos_project`: frame 0x70 against retail's 0x80 (same instruction count, stack slots only);
 *  - `eft053_model_list_get`: ours fuses `clrlwi.` where retail keeps `clrlwi` + `cmpwi`;
 *  - `eft053_get_shell_data`: ours compares the map cases unsigned (`cmplwi`), drops a `clrlwi`, and emits its
 *    switch table locally (`@1613`) where retail reads `jumptable_805EDF7C`;
 *  - `eft053_get_model_ang`: ours drops the `clrlwi` narrowing of both `u8` arguments;
 *  - `eft053_work_act_ck`: ours keeps an extra saved register where retail branches to its two constant tails;
 *  - `eft053_set`: retail reloads the count (`lwz r0,4(r31)`) before the loop test, ours reuses `r3`;
 *  - `eft053_slot_rot_step`, `eft053_slot_move_step`: the slot-address arithmetic (`slwi`/`add`/`lwz`) sits at
 *    each use in ours and the registers are coloured differently; `eft053_slot_rot_step` calls `map_se_req` by its
 *    plain name where retail calls `map_se_req__FUcPQ34nw4r4math4VEC3`.
 *   flipcheck: `.rodata` claimed, not emitted; `.text` (0x12A4 of 0x4210), extab (0x40 of 0x70), extabindex (0x60 of
 *   0xA8), `.data` (0x78 of 0x598) and `.sdata2` (0x10 of 0xE0) short of the claim and differing; `map_se_req` has
 *   no map row.
 * SHAPES. `eft053_shell_pos_project` keeps both arms of the angle test: with one shared arm MWCC if-converts them.
 */
#include "types.h"
#include "fn_8004CAD8/mtx.h" /* the owner header (rule 2) */
#include "mh3_pad.h" /* the owner header (rule 2) */
#include "nw4r/math.h"
#include "ef.h"
#include "pl.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/fn_8012BDF4.h" /* em_act_ck, em_work_die_ck */
#include "fn_8004CAD8.h"       /* setVector3, mulVecMatAddTrans, rotVecY, VEC3_ctor, MTX34_ctor */
#include "ef/eft053.h"         /* this unit's header (the helpers whose owner header cannot carry them) */
#include "unsplit/unknown.h"
#include "enemy/em_pop.h"   /* the band's unnamed callees */
#include "ef/eft_res.h"        /* eft_res_slot_get/eft_res_model_get/fn_800F8A44 (the effect pool) */
#include "ef/effect.h"         /* eft_state_flags_set (the effect's two state bytes) */
#include "ef/eft019.h"         /* eft019_set_core (the family's shared placement entry) */
#include "draw_shape.h"        /* fn_80056A84 (the shape request the state machine fires) */
#include "sound/fn_800DD1F0.h" /* map_se_req (the map SE the same state fires) */
#include "unsplit/Pl.h"        /* PlSlotGate lbl_806BB7A0 (the family's timing block) */
#include "ef/fn_800CDB2C.h"    /* ran_suu (the family's frame randomiser) */
#include "ef/eft001.h"         /* eft_rot_vec_copy (the rotation copy) */
#include "g3d/g3d_calcworld.h"  /* addVec3To (the vector add) */

/* ---------------------------------------------------------------------------------------------------
 * this range's own data (declared, never defined - playbook 29)
 * ------------------------------------------------------------------------------------------------- */

/* the range's private `.sdata2` pool, 0x8079B740..0x8079B820.  Values read out of the DOL:
 *   0x8079B740 0.0f       0x8079B744 0.5f       0x8079B748 65536.0f   0x8079B74C 0.01f
 *   0x8079B750 45.0f      0x8079B754 360.0f     0x8079B758 35.0f      0x8079B75C 50.0f
 *   0x8079B760 75.0f      0x8079B764 3000.0f    0x8079B768 130.0f     0x8079B76C 2000.0f
 *   0x8079B770 176.0f     0x8079B774 -0.0f      0x8079B778 -2600.0f   0x8079B77C -4800.0f
 *   0x8079B780 -1600.0f   0x8079B784 1000.0f    0x8079B788 300.0f     0x8079B78C 800.0f
 *   0x8079B790 1.0f       0x8079B794 -1400.0f   0x8079B798 -2675.0f   0x8079B79C -2275.0f
 *   0x8079B7A0 -1300.0f   0x8079B7A4 -1200.0f   0x8079B7A8 -200.0f    0x8079B7AC 0.25f
 *   0x8079B7B0 5.0f       0x8079B7B4 -3200.0f   0x8079B7B8 10.0f      0x8079B7BC -980.0f
 *   0x8079B7C0 -1240.0f   0x8079B7C4 -50.0f     0x8079B7C8 150.0f     0x8079B7CC -500.0f
 *   0x8079B7D0 60.0f      0x8079B7D4 0.001f     0x8079B7D8 40000.0f   0x8079B7DC 850.0f
 *   0x8079B7E0 1250.0f    0x8079B7E4 9973.0f    0x8079B7E8 1320.0f    0x8079B7EC 0.9f
 *   0x8079B7F0 500.0f     0x8079B7F4 20.0f      0x8079B7F8 176.0f     0x8079B7FC 0.0f
 *   0x8079B800 -0.1f      0x8079B804 0.1f       0x8079B808 270400.0f  0x8079B80C -30.0f
 *   0x8079B810 80.0f      0x8079B814 -60.0f     0x8079B818 -1500.0f   0x8079B81C -400.0f */
extern "C" f32 lbl_8079B740; /* 0.0f */
extern "C" f32 lbl_8079B744; /* 0.5f */
extern "C" f32 lbl_8079B748; /* 65536.0f */
extern "C" f32 lbl_8079B74C; /* 0.01f */
extern "C" f32 lbl_8079B750; /* 45.0f */
extern "C" f32 lbl_8079B754; /* 360.0f */
extern "C" f32 lbl_8079B758; /* 35.0f */
extern "C" f32 lbl_8079B75C; /* 50.0f */
extern "C" f32 lbl_8079B760; /* 75.0f */
extern "C" f32 lbl_8079B764; /* 3000.0f */
extern "C" f32 lbl_8079B768; /* 130.0f */
extern "C" f32 lbl_8079B76C; /* 2000.0f */
extern "C" f32 lbl_8079B770; /* 176.0f */
extern "C" f32 lbl_8079B774; /* -0.0f */
extern "C" f32 lbl_8079B778; /* -2600.0f */
extern "C" f32 lbl_8079B77C; /* -4800.0f */
extern "C" f32 lbl_8079B780; /* -1600.0f */
extern "C" f32 lbl_8079B784; /* 1000.0f */
extern "C" f32 lbl_8079B788; /* 300.0f */
extern "C" f32 lbl_8079B78C; /* 800.0f */
extern "C" f32 lbl_8079B790; /* 1.0f */
extern "C" f32 lbl_8079B794; /* -1400.0f */
extern "C" f32 lbl_8079B798; /* -2675.0f */
extern "C" f32 lbl_8079B79C; /* -2275.0f */
extern "C" f32 lbl_8079B7A0; /* -1300.0f */
extern "C" f32 lbl_8079B7A4; /* -1200.0f */
extern "C" f32 lbl_8079B7A8; /* -200.0f */
extern "C" f32 lbl_8079B7AC; /* 0.25f */
extern "C" f32 lbl_8079B7B0; /* 5.0f */
extern "C" f32 lbl_8079B7B4; /* -3200.0f */
extern "C" f32 lbl_8079B7B8; /* 10.0f */
extern "C" f32 lbl_8079B7BC; /* -980.0f */
extern "C" f32 lbl_8079B7C0; /* -1240.0f */
extern "C" f32 lbl_8079B7C4; /* -50.0f */
extern "C" f32 lbl_8079B7C8; /* 150.0f */
extern "C" f32 lbl_8079B7CC; /* -500.0f */
extern "C" f32 lbl_8079B7D0; /* 60.0f */
extern "C" f32 lbl_8079B7D4; /* 0.001f */
extern "C" f32 lbl_8079B7D8; /* 40000.0f */
extern "C" f32 lbl_8079B7DC; /* 850.0f */
extern "C" f32 lbl_8079B7E0; /* 1250.0f */
extern "C" f32 lbl_8079B7E4; /* 9973.0f */
extern "C" f32 lbl_8079B7E8; /* 1320.0f */
extern "C" f32 lbl_8079B7EC; /* 0.9f */
extern "C" f32 lbl_8079B7F0; /* 500.0f */
extern "C" f32 lbl_8079B7F4; /* 20.0f */
extern "C" f32 lbl_8079B7F8; /* 176.0f */
extern "C" f32 lbl_8079B7FC; /* 0.0f */
extern "C" f32 lbl_8079B800; /* -0.1f */
extern "C" f32 lbl_8079B804; /* 0.1f */
extern "C" f32 lbl_8079B808; /* 270400.0f */
extern "C" f32 lbl_8079B80C; /* -30.0f */
extern "C" f32 lbl_8079B810; /* 80.0f */
extern "C" f32 lbl_8079B814; /* -60.0f */
extern "C" f32 lbl_8079B818; /* -1500.0f */
extern "C" f32 lbl_8079B81C; /* -400.0f */

/* The `eft053` family's per-slot work record, the object `_EFT::work_0x38` points at.  It is three
 * parallel arrays behind one header: the pooled model handles at +0x04 (stride 4), the per-slot frame
 * counters at +0x40 (stride 4) and the per-slot state bytes at +0x60 (stride 1).  `eft053_set` splits
 * one allocation of this size into the record plus its handle, so size: 0x7C (the record's own `.bss`
 * holds one per effect slot). */
typedef struct Eft053Work {
    /* +0x000 */ s32 count;      /* the slot count `eft053_model_list_get` writes */
    /* +0x004 */ MHchar* models[8]; /* one pooled model handle per slot; `models[0]` is what the
                                     * release path (`fn_800F8A44`) is handed as its list base */
    /* +0x024 */ u8 kind_0x24;   /* the stage model-list kind `eft053_model_list_get` writes */
    /* +0x025 */ u8 pad_0x25[0x10];
    /* +0x035 */ u8 field_0x35[8]; /* per-slot placement index into the family's `Vec` table
                                    * (`lbl_805EDEB0`, 0xC bytes an entry) */
    /* +0x03D */ u8 pad_0x3D[0x3];
    /* +0x040 */ s32 counters[8]; /* per-slot frame counter */
    /* +0x060 */ u8 states[8];   /* per-slot state byte */
    /* +0x068 */ u8 field_0x68[6]; /* per-slot flag byte `fn_80369D50` sets to 0xA */
    /* +0x06E */ u16 field_0x6E[8]; /* per-slot frame window the step is divided into */
} Eft053Work; /* size: 0x7C (approximate past +0x68: no body of this unit reads further) */

/* the range's `.data` tables and its `.rodata` arm table */
extern "C" u32 jumptable_805EDF40[]; /* eft053_model_list_get */
extern "C" u32 jumptable_805EDF7C[]; /* eft053_get_shell_data */
extern "C" u32 jumptable_805EDFB8[]; /* fn_80367124 */
extern "C" u32 jumptable_805EDFF4[]; /* fn_80367760 */
extern "C" u32 jumptable_805EE014[]; /* fn_80367760 */
extern "C" u32 jumptable_805EE03C[]; /* fn_80369D50 */
extern "C" u8 lbl_805EDE88[];        /* .data 0x805EDE88: the model record table */
extern "C" Vec lbl_805EDEA4;       /* .data 0x805EDEA4: the stage's base placement */
extern "C" Vec lbl_805EDEB0[];       /* .data 0x805EDEB0: the family's placement table,
                                      * 0xC bytes (one `Vec`) an entry */
extern "C" u8 lbl_80570A10[];        /* .rodata 0x80570A10, 16 bytes (fn_80369D50) */

/* ---------------------------------------------------------------------------------------------------
 * this range's own functions (the map's stems are `fn_XXXXXXXX`, so they are `extern "C"`; the two
 * runtime-dump names keep C++ linkage, reproduced exactly by their real signature)
 * ------------------------------------------------------------------------------------------------- */
extern "C" void eft053_shell_pos_project(_PLW* plw, VEC3* out);
extern "C" s32 eft053_model_list_get(u8* out_kind, s32* out_count);
s32 eft053_get_shell_data(_PLW* plw, u8 index, VEC3* a, VEC3* b, VEC3* c);
u16 eft053_get_model_ang(u8 a, u8 b);
extern "C" s32 eft053_work_act_ck(_ENEMY_WORK* work);
extern "C" void eft053_set(void);
extern "C" void eft053_release(_EFT* self);
extern "C" void eft053_dispatch(_EFT* self);
extern "C" void fn_80367124(_EFT* self);
extern "C" void fn_80367760(_EFT* self, s32 index);
extern "C" void fn_80368374(_EFT* self, s32 index);
extern "C" void eft053_slot_rot_step(_EFT* self, s32 index);
extern "C" s32 eft053_slot_move_step(_EFT* self, s32 index, u8 table_off, s32 randomize);
extern "C" void fn_8036928C(_EFT* self);
extern "C" void fn_80369D50(_EFT* self, s32 index);

/* One entry of the current stage's model list, reached through `lbl_805EDE88`'s pointer table.
 * size: 0x18 (the loop stride the target multiplies the index by) */
typedef struct EftModelRec {
    /* +0x00 */ u8 kind;      /* the family kind `fn_80368374` compares with its slot's +0x2D byte */
    /* +0x01 */ u8 index;     /* the slot index the same unit compares with its +0x35 byte */
    /* +0x02 */ u8 field_0x02; /* the sub-kind the shell projection switches on */
    /* +0x03 */ u8 pad_0x03[0x01];
    /* +0x04 */ Vec pos_0x04; /* the record's placement vector */
    /* +0x10 */ u16 rot_x;
    /* +0x12 */ u16 rot_y;    /* the rotation `eft053_get_model_ang` returns */
    /* +0x14 */ u16 rot_z;
    /* +0x16 */ u8 pad_0x16[0x02];
} EftModelRec; /* size: 0x18 */

/* The range's own unwritten bodies, declared before use. */
extern "C" void fn_8036A690(_EFT* self);
extern "C" void fn_8036A814(_EFT* self);
extern "C" void fn_8036A824(_EFT* self);

/* ---------------------------------------------------------------------------------------------------
 * bodies, in address order
 * ------------------------------------------------------------------------------------------------- */

#pragma peephole off

/* Projects the player's shell frame into `out` through the Z-X-Y rotation of the signed frame angle and the
 * per-kind scale triple. */
extern "C" void eft053_shell_pos_project(_PLW* plw, VEC3* out)
{
    MTX34 mtx;
    _CP_VECTOR rot;
    f32 scale;
    f32 t;
    s32 a;
    s16 v;

    MTX34_ctor(&mtx);
    setVector3(out, lbl_8079B740, lbl_8079B740, lbl_8079B740);
    eft_rot_vec_copy(&rot, (_CP_VECTOR*)&plw->param_0x54); /* +0x54 is the actor's 3-word rotation */
    a = (s8)plw->shell_ang_0x583;
    if (a >= 0) {
        t = lbl_8079B750 * (f32)a;
        t = t * lbl_8079B74C;
        t = t * lbl_8079B748;
        t = t / lbl_8079B754;
        v = (s16) -(u16)(s32)(lbl_8079B744 + t);
    } else {
        t = lbl_8079B758 * (f32)a;
        t = t * lbl_8079B74C;
        t = t * lbl_8079B748;
        t = t / lbl_8079B754;
        v = (s16) -(u16)(s32)(lbl_8079B744 + t);
    }
    rot.x = v;
    cpSetRotMatrixZXY(&rot, &mtx);
    if (plw->kind_0x09 == 3) {
        out->y = lbl_8079B75C;
        out->z = lbl_8079B760;
        scale = lbl_8079B764;
    } else {
        out->y = lbl_8079B768;
        out->z = lbl_8079B760;
        scale = lbl_8079B76C;
    }
    rotVecY(out, plw->field_0x058);
    addVec3To(out, &plw->vec_0x03C);
    mtx.m[0][3] = out->x;
    mtx.m[1][3] = out->y;
    mtx.m[2][3] = out->z;
    setVector3(out, lbl_8079B740, lbl_8079B740, scale);
    mulVecMatAddTrans(out, &mtx);
}

#pragma peephole on

/* Pick the family's model record for the current map: the map number selects one of six per-stage
 * bodies, each of which tests the current area and writes the record kind and count. */
extern "C" s32 eft053_model_list_get(u8* out_kind, s32* out_count)
{
    *out_kind = 0;
    *out_count = 0;
    switch (get_now_mapno()) {
    case 6:
    case 17:
        switch (get_now_areano()) {
        case 1:
            if (quest_flag_200000_ck(0) != 0) {
                *out_count = 6;
            } else {
                *out_count = 8;
            }
            *out_kind = 0;
            return 1;
        case 2:
            if (quest_flag_200000_ck(0) != 0) {
                *out_count = 6;
            } else {
                *out_count = 8;
            }
            *out_kind = 1;
            return 1;
        }
        break;
    case 7:
    case 18:
        switch (get_now_areano()) {
        case 3:
            *out_count = 7;
            *out_kind = 2;
            return 1;
        }
        break;
    case 8:
    case 19:
        switch (get_now_areano()) {
        case 1:
            *out_count = 4;
            *out_kind = 3;
            return 1;
        }
        break;
    case 9:
        switch (get_now_areano()) {
        case 0:
            *out_count = 4;
            *out_kind = 4;
            return 1;
        case 1:
            *out_count = 4;
            *out_kind = 5;
            return 1;
        }
        break;
    case 11:
    case 20:
        switch (get_now_areano()) {
        case 1:
            *out_count = 4;
            *out_kind = 6;
            return 1;
        }
        break;
    }
    return 0;
}

/* True while the enemy work record is in one of the four act states this family reacts to. */
extern "C" s32 eft053_work_act_ck(_ENEMY_WORK* work)
{
    return em_act_ck(work, 13, 0) || em_act_ck(work, 13, 2) || em_act_ck(work, 13, 3)
        || em_act_ck(work, 13, 4);
}

/* Look the (kind, index) pair up in the stage's model record list and return its 16-bit id, 0 when
 * the current map has no record list or the pair is absent. */
u16 eft053_get_model_ang(u8 a, u8 b)
{
    u8 kind;
    s32 count;
    EftModelRec* records;
    s32 i;

    kind = 0;
    count = 0;
    if (eft053_model_list_get(&kind, &count) == 0) {
        return 0;
    }
    records = ((EftModelRec**)lbl_805EDE88)[kind];
    for (i = 0; i < count; i++) {
        if (records[i].kind == a && records[i].index == b) {
            return records[i].rot_y;
        }
    }
    return 0;
}

/* Allocate a pooled effect record for the family, fill its model slots and arm its state machine. */
extern "C" void eft053_set(void)
{
    _EFT* eft;
    Eft053Work* work;
    s32 i;

    eft = (_EFT*)eft_res_slot_get(0x7C);
    if (eft == 0) {
        return;
    }
    eft->release_0x40 = eft053_release;
    work = (Eft053Work*)eft->work_0x38;
    if (eft053_model_list_get(&work->kind_0x24, &work->count) == 0) {
        fn_8036A824(eft);
        return;
    }
    for (i = 0; i < work->count; i++) {
        work->models[i] = (MHchar*)eft_res_model_get();
        if (work->models[i] == 0) {
            eft_res_slot_release(eft);
            return;
        }
    }
    eft->source_0x30 = 0;
    eft->dispatch_0x34 = eft053_dispatch;
    eft->area_0x44 = get_now_areano();
    eft->field_0x03 = 53;
    eft_state_flags_set(eft, 8, 0);
}

/* Steps one slot of the rotation machine: `states[index]` runs one of seven stages and `counters[index]` feeds the
 * model's rotation. */
extern "C" void eft053_slot_rot_step(_EFT* self, s32 index)
{
    VEC3 pos;
    PlSlotGate* timer;
    Eft053Work* work;
    MHchar* model;
    s32 c;

    work = (Eft053Work*)self->work_0x38;
    timer = lbl_806BB7A0;
    VEC3_ctor(&pos);
    work->counters[index] += 1;
    switch (self->area_0x44) {
    case 1:
        setVector3(&pos, lbl_8079B740, lbl_8079B7DC, lbl_8079B7E0);
        break;
    case 2:
        setVector3(&pos, lbl_8079B7E4, lbl_8079B78C, lbl_8079B7E8);
        break;
    default:
        work->states[index] = 6;
        return;
    }
    model = work->models[index];
    switch (work->states[index]) {
    case 0:
        if (timer[1].flag_0x00 != 0) {
            work->states[index] += 1;
            work->counters[index] = 0;
        }
        break;
    case 1:
        if (work->counters[index] >= 45) {
            work->states[index] += 1;
            work->counters[index] = 0;
        }
        break;
    case 2:
        model->field_0x30 = -work->counters[index] * 3034;
        if (work->counters[index] >= 3) {
            work->states[index] += 1;
            work->counters[index] = 0;
            eft019_set_core(&pos, self->area_0x44, 111, lbl_8079B7EC, lbl_8079B7EC);
            fn_80056A84(&pos, 10, self->area_0x44);
            if (get_now_areano() == self->area_0x44) {
                map_se_req(9, &pos);
            }
        }
        break;
    case 3:
        c = work->counters[index];
        if (c <= 2) {
            model->field_0x30 = c * 1274 - 9102;
        } else if (c <= 4) {
            model->field_0x30 = -((c - 2) * 1274 + 6554);
        } else if (c <= 5) {
            model->field_0x30 = (c - 4) * 849 - 9102;
        } else if (c <= 6) {
            model->field_0x30 = -((c - 5) * 849 + 8253);
        }
        if (work->counters[index] >= 6) {
            work->states[index] += 1;
            work->counters[index] = 0;
        }
        if (work->counters[index] == 4) {
            eft019_set_core(&pos, self->area_0x44, 111, lbl_8079B7EC, lbl_8079B7EC);
        }
        break;
    case 4:
        if (work->counters[index] >= 30) {
            work->states[index] += 1;
            work->counters[index] = 0;
        }
        break;
    case 5:
        model->field_0x30 = -work->counters[index] * 303;
        if (work->counters[index] >= 30) {
            work->states[index] = 6;
            work->counters[index] = timer[1].length_0x04;
        }
        break;
    case 6:
        if (work->counters[index] < timer[1].length_0x04) {
            work->counters[index] = 0;
            work->states[index] = 1;
        } else {
            if (timer[1].flag_0x00 == 0) {
                work->states[index] = 0;
            }
            work->counters[index] = timer[1].length_0x04;
        }
        break;
    }
}

/* Steps one slot of the translation machine over its frame window and re-places the model from the `Vec` table
 * when a stage completes; returns 1 on the last stage. */
extern "C" s32 eft053_slot_move_step(_EFT* self, s32 index, u8 table_off, s32 randomize)
{
    VEC3 pos;
    Eft053Work* work;
    MHchar* model;
    f32 step;
    s32 seed;
    s32 start;
    s32 ret;

    work = (Eft053Work*)self->work_0x38;
    ret = 0;
    step = (f32)work->field_0x6E[work->field_0x35[index]];
    VEC3_ctor(&pos);
    if (randomize != 0) {
        seed = ran_suu(1) % 5;
        start = ran_suu(1) % (s32)(lbl_8079B7F0 / step);
    } else {
        seed = 0;
        start = (s32)(lbl_8079B7F0 / step);
    }
    model = work->models[index];
    switch (work->states[index]) {
    case 0:
        work->states[index] += 1;
        work->field_0x6E[work->field_0x35[index]] = (s16)(lbl_8079B7F4 + (f32)seed);
        break;
    case 1:
        work->counters[index] += 1;
        model->pos_0x04.z -= lbl_8079B744 * (f32)work->counters[index];
        if (lbl_8079B744 * (f32)work->counters[index] >= step) {
            work->states[index] += 1;
            work->counters[index] = start;
            vec_to_mh_vec3(&pos, &lbl_805EDEB0[table_off + work->field_0x35[index]]);
            work->counters[index] =
                (s32)((f32)work->counters[index] + calcVecDistXZ(&model->pos_0x04, &pos) / step);
        }
        break;
    case 2:
        model->pos_0x04.z -= step;
        work->counters[index] -= 1;
        if (work->counters[index] <= 0) {
            work->states[index] += 1;
            work->counters[index] = 0;
        }
        break;
    case 3:
        work->counters[index] += 1;
        model->pos_0x04.z -= step - lbl_8079B744 * (f32)work->counters[index];
        if (step - lbl_8079B744 * (f32)work->counters[index] <= lbl_8079B740) {
            work->states[index] += 1;
            work->counters[index] = 0;
            work->field_0x6E[work->field_0x35[index]] = (s16)(lbl_8079B7F4 + (f32)seed);
        }
        break;
    case 4:
        work->counters[index] += 1;
        model->pos_0x04.z += lbl_8079B744 * (f32)work->counters[index];
        if (lbl_8079B744 * (f32)work->counters[index] >= step) {
            work->states[index] += 1;
            work->counters[index] = start;
            vec_to_mh_vec3(&pos, &lbl_805EDEB0[table_off + work->field_0x35[index]]);
            work->counters[index] =
                (s32)((f32)work->counters[index] + calcVecDistXZ(&model->pos_0x04, &pos) / step);
        }
        break;
    case 5:
        model->pos_0x04.z += step;
        work->counters[index] -= 1;
        if (work->counters[index] <= 0) {
            work->states[index] += 1;
            work->counters[index] = 0;
        }
        break;
    case 6:
        work->counters[index] += 1;
        model->pos_0x04.z += step - lbl_8079B744 * (f32)work->counters[index];
        if (step - lbl_8079B744 * (f32)work->counters[index] <= lbl_8079B740) {
            work->states[index] = 0;
            work->counters[index] = 0;
            ret = 1;
        }
        break;
    }
    return ret;
}

/* Fills the three vectors with the shell's transformed record placement for one kind; returns 0 when the current
 * map has no record for it. */
s32 eft053_get_shell_data(_PLW* plw, u8 index, VEC3* a, VEC3* b, VEC3* c)
{
    VEC3 tmp_a;
    VEC3 tmp_b;
    VEC3 tmp_c;
    EftModelRec* list;
    u8 kind;
    s32 count;
    f32 ang;
    f32 scale_off;
    f32 off_a;
    f32 c28;
    f32 c27;
    f32 c26;
    f32 c25;
    f32 c24;
    f32 c23;
    s32 i;

    kind = 0;
    count = 0;
    ang = lbl_8079B740;
    scale_off = ang;
    off_a = ang;
    c28 = lbl_8079B778;
    c27 = lbl_8079B77C;
    c26 = lbl_8079B780;
    c25 = lbl_8079B784;
    c24 = lbl_8079B788;
    c23 = lbl_8079B78C;
    setVector3(a, ang, ang, ang);
    setVector3(b, lbl_8079B740, lbl_8079B740, lbl_8079B740);
    setVector3(c, lbl_8079B740, lbl_8079B740, lbl_8079B790);
    if (eft053_model_list_get(&kind, &count) == 0) {
        return 0;
    }
    list = ((EftModelRec**)lbl_805EDE88)[kind];
    for (i = 0; i < count; i++) {
        u32 found = 0;
        switch (plw->kind_0x015) {
        case 6:
        case 17:
            off_a = c28;
            scale_off = c25;
            switch (plw->area_0x16) {
            case 1:
                switch (list[i].field_0x02) {
                case 1:
                    found = 1;
                    ang = lbl_8079B794;
                    break;
                case 6:
                    vec_to_mh_vec3(a, &list[i].pos_0x04);
                    return 1;
                }
                break;
            case 2:
                switch (list[i].field_0x02) {
                case 1:
                    found = 1;
                    ang = lbl_8079B794;
                    break;
                case 6:
                    vec_to_mh_vec3(a, &list[i].pos_0x04);
                    return 1;
                }
                break;
            }
            break;
        case 7:
        case 18:
            off_a = c27;
            scale_off = c24;
            if (plw->area_0x16 == 3) {
                switch (list[i].field_0x02) {
                case 1:
                    found = 1;
                    ang = lbl_8079B798;
                    break;
                case 2:
                    found = 1;
                    ang = lbl_8079B79C;
                    break;
                case 3:
                    found = 1;
                    ang = lbl_8079B798;
                    break;
                }
            }
            break;
        case 8:
        case 19:
            off_a = c26;
            scale_off = c23;
            if ((u32)(plw->area_0x16 - 1) <= 1) {
                switch (list[i].field_0x02) {
                case 1:
                    found = 1;
                    ang = lbl_8079B7A0;
                    break;
                case 2:
                    found = 1;
                    ang = lbl_8079B7A4;
                    break;
                case 3:
                    found = 1;
                    ang = lbl_8079B794;
                    break;
                case 4:
                    found = 1;
                    ang = lbl_8079B7A4;
                    break;
                }
            }
            break;
        case 9:
            off_a = c26;
            scale_off = c23;
            if (plw->area_0x16 <= 1) {
                switch (list[i].field_0x02) {
                case 1:
                    found = 1;
                    ang = lbl_8079B7A0;
                    break;
                case 2:
                    found = 1;
                    ang = lbl_8079B7A4;
                    break;
                case 3:
                    found = 1;
                    ang = lbl_8079B794;
                    break;
                case 4:
                    found = 1;
                    ang = lbl_8079B7A4;
                    break;
                }
            }
            break;
        case 11:
        case 20:
            off_a = c26;
            scale_off = c23;
            if (plw->area_0x16 == 1) {
                switch (list[i].field_0x02) {
                case 1:
                    found = 1;
                    ang = lbl_8079B7A0;
                    break;
                case 2:
                    found = 1;
                    ang = lbl_8079B7A4;
                    break;
                case 3:
                    found = 1;
                    ang = lbl_8079B794;
                    break;
                case 4:
                    found = 1;
                    ang = lbl_8079B7A4;
                    break;
                }
            }
            break;
        }
        if (found != 0 && index == list[i].field_0x02) {
            f32 e = ang + lbl_8079B7A8;
            rotVecZ(c, list[i].rot_z);
            rotVecX(c, list[i].rot_x);
            rotVecY(c, list[i].rot_y);
            fn_80050850(c, c);
            vec_to_mh_vec3(b, &list[i].pos_0x04);
            fn_80051EE0(&tmp_a, c, scale_off);
            addVec3To(b, &tmp_a);
            fn_80051EE0(&tmp_b, c, off_a);
            addVec3(&tmp_c, b, &tmp_b);
            copyVec3(a, &tmp_c);
            fn_800513F0(c, e * lbl_8079B7AC);
            return 1;
        }
    }
    return 0;
}

/* Hand the effect's work block to the release path. */
extern "C" void eft053_release(_EFT* self)
{
    Eft053Work* work = (Eft053Work*)self->work_0x38;
    fn_800F8A44(&work->models[0], work->count);
}

/* Dispatch on the effect's state byte into the family's per-state entry points. */
extern "C" void eft053_dispatch(_EFT* self)
{
    switch (self->state_0x05) {
    case 0:
        return fn_80367124(self);
    case 1:
        return fn_8036A690(self);
    case 2:
        return fn_8036A814(self);
    case 3:
        return fn_8036A824(self);
    }
}
