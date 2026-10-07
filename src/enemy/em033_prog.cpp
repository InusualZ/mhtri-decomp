/* enemy/em033_prog.cpp - the em033 enemy's program handlers and the head of em035's, after the tail of the eft052
 *   cockpit item layer.
 * RANGE. .text 0x8035BAB4-0x8035F2B4 (55 functions); .rodata 0x805709D0-0x80570A10, .data 0x805ED370-0x805ED838,
 *   .sdata 0x80793340-0x80793348, .sdata2 0x8079B668-0x8079B71C, extab, extabindex.  0x8035BAB4-0x8035E034 is the
 *   tail of the band `ef/eft052.cpp` heads; 0x8035E034-0x8035F060 is em033 (`em033_prog_tbl`, 0x805ED370, lists
 *   `fn_8035E034`/`fn_8035E578`/`fn_8035E1E0`) and 0x8035F060-0x8035F2B4 the head of em035 (`em035_prog_tbl`,
 *   0x805ED838, lists `fn_8035F060`/`fn_8035F174`/`fn_8035F178`).
 * SEAM. Unproven at both edges: `tudiscover.py at 0x8035E034` puts a strong seam at 0x8035F060 (the `.sdata2` run
 *   jumps `lbl_8079B710` -> `lbl_8079B718`) where `em035_prog_tbl` starts, and `lbl_805ED808` (this unit's `.data`)
 *   mixes `fn_8035F004`/`fn_8035EF50`/`fn_8035EF58` with `enemy/fn_80138074.c`'s `fn_801394D4`..`fn_8013A650`.
 * NAMES. `em033_prog` is a GUESS from `em033_prog_tbl`, the first object of the unit's `.data`.
 * RESIDUALS. 39 rows unwritten: 0x8035BAB4-0x8035E034 (`fn_8035BAB4` .. `fn_8035DFC0`).
 *  - `fn_8035E580` saves from r20 where retail calls `_restgpr_22`; `fn_8035EF58` saves no registers through
 *    the helpers where retail calls `_savegpr_27`/`_restgpr_27` (register pressure, not a callee choice).
 *  - `fn_8035E1E0`: retail repeats the `em_move_target_set` call (with `lbl_8079B70C`) and the `fn_8012B380` tail in every
 *    case body (920 B), ours shares them (756 B);
 *  - `fn_8035E580`: retail reads the `prev` byte before any store (uninitialised), ours starts it at 0, and the
 *    loop colours differently (`_savegpr_22` against our `_savegpr_20`);
 *  - `fn_8035ECD8`: retail keeps `act_id & 0xF` (`clrlwi r0,r4,28`) in every case body, ours folds the mask under
 *    the `(u8)(act_id - 1) <= 3` guard (360 B against 320 B);
 *  - `fn_8035EF58`: the prologue's callee-saved set and the `p = *(a + 4)` local colour differently, and ours calls
 *    `fn_8008E8D0` at a later point;
 *  - `fn_8035E81C`, `fn_8035E984`: ours calls `get_now_areano` a second time where retail reuses the narrowed
 *    result, and compares unsigned where retail uses `cmpwi`;
 *  - `fn_8035ECD8`, `fn_8035EE40`, `fn_8035F178`: retail keeps `clrlwi` + `slwi`, ours fuses them into `clrlslwi`;
 *    `fn_8035F004`, `fn_8035F060`: retail keeps `extsh`/`clrlwi` + `cmpwi`, ours emits the record forms;
 *  - `fn_8035E034`: retail's frame is 0x50 against our 0x40 and it extracts the id with `extrwi`;
 *    `fn_8035EB80`: retail spills f31 with `psq_st` and saves one register fewer; `fn_8035EAA0`: retail narrows the
 *    return with `clrlwi`/`extsh`.
 *   flipcheck: `.data`/`.rodata`/`.sdata`/`.sdata2` claimed, not emitted; `.text`/extab/extabindex short of the claim.
 */

#include "enemy/fn_8012B380.h" /* fn_8012B380 (rule 2: the owner's header) */
#include "enemy/em_hit_by_set.h" /* fn_8012CEB4 (rule 2: the owner's header) */
#include "types.h"
#include "nw4r/math.h"
#include "enemy.h"
#include "unsplit/unknown.h"
#include "unsplit/enemy.h"
#include "enemy/enemy_control.h" /* em_spawn_request (the owner's header, rule 2) */
#include "fn_8004CAD8.h"
#include "enemy/fn_801251D0.h"
#include "enemy/fn_8012EC74.h"
#include "enemy/fn_80138074.h"
#include "enemy/em_pop.h" /* the roster records and kind searches (the owner's header, rule 1/2) */
#include "enemy/em_model.h"
#include "ef/fn_800CDB2C.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "stage/stg_w.h"
/* signatures the calls below use, when they differ from the owner header's: a cast call is the same direct call. */
#define fn_8012CEB4_c1 ((void (*)(_ENEMY_WORK*, s16, u32))em_hit_by_set)

/* Callees outside this unit. */

/* The vector helpers `g3d/g3d_resanmchr.cpp` (the first two) and `fn_8004CAD8.cpp` define. */
extern "C" void fn_8008E8D0(void* a, void* b);
extern "C" void fn_8008DA10(void* a, void* b);
void vec3_scale_in_place(VEC3* v, f32 s);

/* The object the aim writer's first argument points at, and the source block its +0x4 member names
 * (`fn_8035EF58`).  Only the two fields the body touches are named. */
struct EmAimOwner {
    /* +0x000 */ u8 pad_0x000[0x4];
    /* +0x004 */ u8* field_0x004;
}; /* size: 0x8 */
struct EmAimSource {
    /* +0x000 */ u8 pad_0x000[0xA];
    /* +0x00A */ u8 field_0x00A;
    /* +0x00B */ u8 pad_0x00B[0x348 - 0xB];
    /* +0x348 */ nw4r::math::VEC3 field_0x348;
}; /* size: 0x354 */

extern "C" u32 fn_802B0998(u8 index);

/* 0x80295924 (`Pl/pl_coll.cpp`): the segment/line intersection test, declared at C++ scope (its map name is
 * this signature's mangling). */
int findInterSection(VEC3* a, VEC3* b, VEC3* c, u8 d, u32 e, u8 f, u16 g, u8* h);

/* The runtime's scalar deleter; `__dl__FPv` is its mangling. */
void operator delete(void* ptr) throw();

/* This unit's own forward declarations (definitions follow in address order). */
extern "C" void fn_8035EE40(_ENEMY_WORK* self, u8 a, VEC3* out);
extern "C" u8 fn_8035E580(_ENEMY_WORK* self, u8 a);
extern "C" u8 fn_8035E984(u8 a, u8 b);

/* Pool literals (declared, never defined - playbook 29). */
extern "C" f32 lbl_8079B704;
extern "C" f32 lbl_8079B708;
extern "C" f32 lbl_8079B70C;
extern "C" f32 lbl_8079B710;
extern "C" f32 lbl_8079B718;

/* The shared-state accessors. */
u8 get_now_areano();
u8 get_now_mapno();

/* 0x8035E034 (0x1AC): em033's distance/id query. */
extern "C" u8 fn_8035E034(_ENEMY_WORK* self, u8 mode)
{
    VEC3 a;
    VEC3 b;
    VEC3 c;

    VEC3_ctor(&a);
    VEC3_ctor(&b);
    switch (mode) {
    case 0:
        return self->field_0x1E4;
    case 1:
        return self->field_0x33E;
    case 2:
        fn_8035EE40(self, 0, &a);
        subVec3(&c, &a, &self->pos);
        copyVec3(&b, &c);
        return vec3_len((const f32*)&b) <= lbl_8079B704;
    case 3:
        fn_8035EE40(self, 1, &a);
        subVec3(&c, &a, &self->pos);
        copyVec3(&b, &c);
        return vec3_len((const f32*)&b) <= lbl_8079B708;
    case 4:
        return self->field_0x33F;
    case 5:
        fn_8035EE40(self, 2, &a);
        subVec3(&c, &a, &self->pos);
        copyVec3(&b, &c);
        return vec3_len((const f32*)&b) <= lbl_8079B70C;
    case 6:
        return em_roster_kind_aim_pos_get(self->field_0x33B, self->act_id) != 0;
    default:
        return 0;
    }
}

/* 0x8035E1E0 (0x398): em033's action/out-pair selector. */
extern "C" void fn_8035E1E0(_ENEMY_WORK* self, u8* out_a, u8* out_b)
{
    switch ((u8)stage_map_kind_get(self->field_0x1E0)) {
    case 1:
        switch (self->act_id) {
        case 1:
        case 2:
        case 3:
        case 4:
            *out_a = 0xC;
            *out_b = 0;
            em_move_target_set(self, 0, 2, lbl_8079B70C);
            break;
        default:
            *out_a = 0xC;
            *out_b = 0;
            fn_8012B380(self, 5, 2, 10);
            break;
        }
        break;
    case 2:
        switch (self->act_id) {
        case 4:
            *out_a = 0xC;
            *out_b = 0;
            if (fn_802B0998(3) == 1) {
                em_move_target_set(self, 1, 3, lbl_8079B70C);
            } else {
                em_move_target_set(self, 0, 2, lbl_8079B70C);
            }
            break;
        case 6:
            *out_a = 0xC;
            *out_b = 2;
            fn_80126278(self, (u16)((self->act_id & 0xF) << 8), &self->pos);
            self->pos_0x1BC.y = 0x6C00;
            break;
        case 9:
            *out_a = 0xC;
            *out_b = 0;
            em_move_target_set(self, 0, 2, lbl_8079B70C);
            break;
        default:
            *out_a = 0xC;
            *out_b = 0;
            fn_8012B380(self, 5, 2, 10);
            break;
        }
        break;
    case 3:
        switch (self->act_id) {
        case 5:
            *out_a = 0xC;
            *out_b = 0;
            em_move_target_set(self, 0, 2, lbl_8079B70C);
            break;
        case 6:
            *out_a = 0xC;
            *out_b = 0;
            em_move_target_set(self, 0, 2, lbl_8079B70C);
            break;
        case 9:
            *out_a = 0xC;
            *out_b = 0;
            em_move_target_set(self, 0, 2, lbl_8079B70C);
            break;
        default:
            *out_a = 0xC;
            *out_b = 0;
            fn_8012B380(self, 5, 2, 10);
            break;
        }
        break;
    case 5:
        *out_a = 0xC;
        *out_b = 0;
        if (self->act_id == 2) {
            em_move_target_set(self, 0, 2, lbl_8079B70C);
        } else {
            fn_8012B380(self, 5, 2, 10);
        }
        break;
    default:
        *out_a = 0xC;
        *out_b = 0;
        fn_8012B380(self, 5, 2, 10);
        break;
    }
}

/* 0x8035E578 (0x8): a constant predicate. */
extern "C" u8 fn_8035E578(void)
{
    return 1;
}

/* 0x8035E580 (0x29C): the em033 roster scan (select the nearest/valid monster record). */
extern "C" u8 fn_8035E580(_ENEMY_WORK* self, u8 a)
{
    VEC3 v1;
    VEC3 v2;
    u8 buf[0x10];
    u8 best = 0xFF;
    u8 prev = 0;
    u8 n;
    int i;

    VEC3_ctor(&v1);
    VEC3_ctor(&v2);
    n = (u8)em_roster_kind_collect(self->act_id, buf, 0x10);
    if (n > 0x10) {
        return 0xFF;
    }
    for (i = 0; i < n; i++) {
        u8 id = buf[i];
        s32 type = (s8)em_roster_kind_field5_get(id, self->act_id);
        if (a == 0) {
            if (type == 5 || type == 1 || type == 0xF) {
                VEC3* p = em_roster_kind_aim_pos_get(id, self->act_id);
                if (p != 0) {
                    u16 ang = em_hit_mask_get(self);
                    int ok = 1;
                    if (findInterSection(&self->pos, p, &v1, 1, 0xFFFF, self->act_id, ang, 0) > 0) {
                        VEC3 v3;
                        subVec3(&v3, &v1, p);
                        copyVec3(&v2, &v3);
                        if (vec3_len((const f32*)&v2) > em_roster_record_get(id)->radius_0x1F0) {
                            ok = 0;
                        }
                    }
                    if (ok) {
                        if (best == 0xFF) {
                            best = id;
                        } else if (type == 5) {
                            best = id;
                            prev = (u8)type;
                        } else if (type == 1 && prev != 5 && prev != 1) {
                            best = id;
                            prev = (u8)type;
                        }
                        if (type == 5) {
                            return best;
                        }
                    }
                }
            }
        } else {
            switch ((u8)stage_map_kind_get(get_now_mapno())) {
            case 1:
                if ((u8)(self->act_id - 1) > 3) {
                    return 0xFF;
                }
                break;
            case 2:
                if (self->act_id != 6 && self->act_id != 9) {
                    return 0xFF;
                }
                break;
            case 3:
                if ((u8)(self->act_id - 5) > 1 && self->act_id != 9) {
                    return 0xFF;
                }
                break;
            case 5:
                if (self->act_id != 2) {
                    return 0xFF;
                }
                break;
            default:
                return 0xFF;
            }
            if (type == 4 || type == 0 || type == 0xE) {
                if (em_roster_kind_aim_pos_get(id, self->act_id) != 0) {
                    return id;
                }
            }
        }
    }
    return best;
}

/* 0x8035E81C (0x168): em033's map/area gate over the roster. */
extern "C" u8 fn_8035E81C(u8 a)
{
    u8 buf[0x10];
    u8 n;
    int i;

    switch ((u8)stage_map_kind_get(get_now_mapno())) {
    case 1:
        if ((u8)(get_now_areano() - 1) > 3) {
            return 0;
        }
        break;
    case 2:
        if (get_now_areano() != 6 && get_now_areano() != 9) {
            return 0;
        }
        break;
    case 3:
        if ((u8)(get_now_areano() - 5) > 1 && get_now_areano() != 9) {
            return 0;
        }
        break;
    case 5:
        if (get_now_areano() != 2) {
            return 0;
        }
        break;
    default:
        return 0;
    }
    n = (u8)em_roster_kind_collect(a, buf, 0x10);
    if (n > 0x10) {
        return 0;
    }
    for (i = 0; i < n; i++) {
        s32 type = (s8)em_roster_kind_field5_get(buf[i], a);
        if (type == 5 || type == 1 || type == 0xF) {
            return 1;
        }
    }
    return 0;
}

/* 0x8035E984 (0x11C): the same gate for one record id. */
extern "C" u8 fn_8035E984(u8 a, u8 b)
{
    s32 type;

    switch ((u8)stage_map_kind_get(get_now_mapno())) {
    case 1:
        if ((u8)(get_now_areano() - 1) > 3) {
            return 0;
        }
        break;
    case 2:
        if (get_now_areano() != 6 && get_now_areano() != 9) {
            return 0;
        }
        break;
    case 3:
        if ((u8)(get_now_areano() - 5) > 1 && get_now_areano() != 9) {
            return 0;
        }
        break;
    case 5:
        if (get_now_areano() != 2) {
            return 0;
        }
        break;
    default:
        return 0;
    }
    type = (s8)em_roster_kind_field5_get(a, b);
    if (type == 4 || type == 0 || type == 0xE) {
        return 1;
    }
    return 0;
}

/* 0x8035EAA0 (0xE0): locks the current target in, or picks a random action. */
extern "C" void fn_8035EAA0(_ENEMY_WORK* self, u8 a)
{
    u8 id = fn_8035E580(self, 0);

    if (id != 0xFF) {
        self->field_0x33B = id;
        copyVec3(&self->target, em_roster_kind_aim_pos_get(id, self->act_id));
        fn_8012B380(self, 6, 0xFF, 0);
        fn_8013072C(self, 5, 1);
    } else {
        self->field_0x33B = 0xFF;
        if (a == 1) {
            fn_8012CEB4_c1(self, (s16)(ran_suu(0) & 0xF), 0);
            fn_8013072C(self, 0, 0);
        } else {
            fn_8013072C(self, 5, 0);
        }
    }
}

/* 0x8035EB80 (0x158): keeps tracking the cached record, or drops it. */
extern "C" void fn_8035EB80(_ENEMY_WORK* self)
{
    VEC3 v1;
    VEC3 v2;
    VEC3 v3;

    VEC3_ctor(&v1);
    VEC3_ctor(&v2);
    if (fn_8035E984(self->field_0x33B, self->act_id) == 1) {
        VEC3* p = em_roster_kind_aim_pos_get(self->field_0x33B, self->act_id);
        u16 ang = em_hit_mask_get(self);
        int keep = 1;
        if (findInterSection(&self->pos, p, &v1, 1, 0xFFFF, self->act_id, ang, 0) > 0) {
            subVec3(&v3, &v1, p);
            copyVec3(&v2, &v3);
            if (vec3_len((const f32*)&v2) > em_roster_record_get(self->field_0x33B)->radius_0x1F0) {
                keep = 0;
            }
        }
        if (keep) {
            copyVec3(&self->target, p);
            fn_8012B380(self, 6, 0xFF, 0);
            self->field_0x33F = 2;
        }
    } else {
        self->field_0x33B = 0xFF;
        fn_8012CEB4_c1(self, (s16)(ran_suu(0) & 0xF), 0);
        self->field_0x33F = 0;
        fn_8013072C(self, 0, 0);
    }
}

/* 0x8035ECD8 (0x168): syncs the aim vector for the current action. */
extern "C" void fn_8035ECD8(_ENEMY_WORK* self)
{
    switch ((u8)stage_map_kind_get(self->field_0x1E0)) {
    case 1:
        if ((u8)(self->act_id - 1) <= 3) {
            fn_80126278(self, (u16)(((self->act_id & 0xF) << 8) | 2), &self->aim);
        }
        break;
    case 2:
        switch (self->act_id) {
        case 4:
            fn_80126278(self, (u16)(((self->act_id & 0xF) << 8) | 2), &self->aim);
            break;
        case 6:
            fn_80126278(self, (u16)(((self->act_id & 0xF) << 8) | 2), &self->aim);
            break;
        case 9:
            fn_80126278(self, (u16)(((self->act_id & 0xF) << 8) | 2), &self->aim);
            break;
        default:
            break;
        }
        break;
    case 3:
        if ((u8)(self->act_id - 5) <= 1 || self->act_id == 9) {
            fn_80126278(self, (u16)(((self->act_id & 0xF) << 8) | 2), &self->aim);
        }
        break;
    case 5:
        if (self->act_id == 2) {
            fn_80126278(self, (u16)(((self->act_id & 0xF) << 8) | 2), &self->aim);
        }
        break;
    default:
        break;
    }
}

/* 0x8035EE40 (0x110): the small per-field aim dispatcher. */
extern "C" void fn_8035EE40(_ENEMY_WORK* self, u8 a, VEC3* out)
{
    switch (a) {
    case 0:
        if ((u8)stage_map_kind_get(self->field_0x1E0) == 3 && self->act_id == 6) {
            fn_80126278(self, (u16)(((self->act_id & 0xF) << 8) | 7), out);
        }
        break;
    case 1:
        if ((u8)stage_map_kind_get(self->field_0x1E0) == 3 && self->act_id == 6) {
            fn_80126278(self, (u16)(((self->act_id & 0xF) << 8) | 8), out);
        }
        break;
    case 2:
        if ((u8)stage_map_kind_get(self->field_0x1E0) == 3 && self->act_id == 6) {
            fn_80126278(self, (u16)(((self->act_id & 0xF) << 8) | 5), out);
        }
        break;
    default:
        break;
    }
}

/* 0x8035EF50 (0x8): a one-line forwarder. */
extern "C" void fn_8035EF50(_ENEMY_WORK* self)
{
    fn_8013A654(self, 1);
}

/* 0x8035EF58 (0xAC): the effect/target aim writer. */
extern "C" void fn_8035EF58(void* a, void* b, void* c, void* d, u32 e, u8* f)
{
    VEC3 v;
    struct EmAimSource* p = (struct EmAimSource*)((struct EmAimOwner*)a)->field_0x004;

    VEC3_ctor(&v);
    if (f[4] == 0xFF) {
        if (e - 5 <= 1) {
            if (p->field_0x00A == 1) {
                fn_8008DA10(b, &v);
                vec3_scale_in_place(&v, lbl_8079B710);
                fn_8008E8D0(b, &v);
            }
        } else if (e == 0x17) {
            fn_8008E8D0(b, &p->field_0x348);
        }
    }
}

/* 0x8035F004 (0x5C): the em033 record's release hook. */
extern "C" _ENEMY_WORK* fn_8035F004(_ENEMY_WORK* p, u16 a)
{
    if (p != 0) {
        fn_8013918C(p, 0);
        if ((s16)a > 0) {
            operator delete(p);
        }
    }
    return p;
}

/* 0x8035F060 (0x114): the em035 run-state constructor. */
extern "C" void fn_8035F060(_ENEMY_WORK* self, u8 a)
{
    self->init_0x320.field_0x328 = 0x708;
    self->init_0x320.field_0x32A = 0xB4;
    self->bytes_0x32C.field_0x32C = 0;
    self->bytes_0x32C.field_0x32D = 0;
    switch (a) {
    case 2:
        if (self->field_0x00A == 2) {
            em_fall_height_get(self);
            em_fall_start(self);
            em_act_arm_unless_down(self, 3, 0);
        } else {
            em_act_arm_unless_down(self, 1, 0);
        }
        self->field_0x1CC = lbl_8079B718;
        if (self->run_flags_0xB12 & 1) {
            copyVec3(&self->pos, (VEC3*)&self->run_0xB04[0]);
            self->pos_0x1BC.y = self->run_angle_0xB10;
        } else {
            self->run_angle_0xB10 = (u16)self->pos_0x1BC.y;
            copyVec3((VEC3*)&self->run_0xB04[0], &self->pos);
        }
        em_state_refresh(self);
        break;
    case 0:
        if (self->field_0x00A != 0) {
            copyVec3(&self->pos, (VEC3*)&self->run_0xB04[0]);
            self->pos_0x1BC.y = self->run_angle_0xB10;
        }
        break;
    default:
        break;
    }
}

/* 0x8035F174 (0x4): an empty virtual slot. */
extern "C" void fn_8035F174(void)
{
}

/* 0x8035F178 (0x13C): em035's effect-summon hook. */
extern "C" void fn_8035F178(_ENEMY_WORK* self, u8 a, u8 b)
{
    u32 s[3];

    switch (a) {
    case 1:
        if (b == 1) {
            s[0] = 0;
            s[1] = (u32)((ran_suu(0) & 0xFF) << 8);
            s[2] = 0;
            em_spawn_request(self->field_0x01A, 0x1A, 4, self->act_id, 0xFF, 1, 0x20, 1,
                        0xFF, &self->pos, (s32)s);
            self->init_0x320.field_0x32A = 0xF0;
        }
        break;
    case 3:
        if (b == 1) {
            s[0] = 0;
            s[1] = (u32)((ran_suu(0) & 0xFF) << 8);
            s[2] = 0;
            em_spawn_request(self->field_0x01A, 0x1A, 5, self->act_id, 0xFF, 1, 0x20, 1,
                        0xFF, &self->pos, (s32)s);
            self->init_0x320.field_0x32A = 0xF0;
        }
        break;
    default:
        break;
    }
}

