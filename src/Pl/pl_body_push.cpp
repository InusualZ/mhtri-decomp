/*
 * Pl/pl_body_push.cpp - the body push-out pass: every frame it pushes the players, the enemies and the AI NPC apart
 *   (capsule against capsule or sphere, through the enemies' hit parts), resolves the camera's push-out against the
 *   same bodies (`coll_push_out`) and keeps the pushed bodies above their floors.  C++; the map's plain names are
 *   `extern "C"`.
 * RANGE. .text 0x80297E34-0x80299ED8 (29 functions); extab 0x80013454-0x80013524, extabindex 0x80030CFC-0x80030E34,
 *   .sdata2 0x8079A380-0x8079A398.  Seams: `Pl/pl_coll.cpp` ends with its static initialiser (0x80297C30) and the two
 *   out-of-line constructors behind it, and its pool (0x8079A318-0x8079A380) holds 0.0f/40.0f/0.001f again before this
 *   unit's 0x8079A380; the right edge is `Pl/pl_hit.cpp`'s (its pool repeats 0.0f/0.5f/40.0f/0.001f, and the calls into
 *   0x802999D8/0x80299CE4 from `coll_push_out`/`body_push_pl_body` keep the push code together up to 0x80299ED8).
 * FLAGS. `cflags_pl` (configure.py's Pl lib).
 * NAMES. GUESS: the file name (no `__FILE__` string in the range; the bodies push bodies apart) and every row's name,
 *   from its body and callers: `body_push_reset`, `body_push_{pl,em,npc}_pass` (the three pair walks),
 *   `body_push_pl_em`, `body_push_pl_body`, `body_push_em_em`, `body_push_npc_em` (one pair each),
 *   `body_push_{pl_em,npc_em,em_em}_apply`, `{pl,npc,em}_push_accum`, `{pl,npc,em}_push_drift_set`, `pl_push_drift_add`,
 *   `pl_push_capsule_get`, `npc_push_capsule_get`, `em_push_sphere_get`, `push_vec_finish`, `push_accum`,
 *   `coll_push_out_em`, `body_shape_get`, `pl_push_vertical_ck`, `npc_push_vertical_ck`.  `hit_lists_init` (the pass's
 *   entry, called by `Pl/pl_motion.cpp`) and `coll_push_out` (the camera's) are earlier GUESSes kept as they are.
 * RESIDUALS. Every row is written.  `body_push_reset`, `body_push_em_pass`, `body_push_pl_pass`: register permutations
 *   of the loop pointers and counters (permdecl tried every declaration order).  The pool is declared by literal, not
 *   claimed by name (`.sdata2` 0x8079A380-0x8079A398 is emitted by the literals); the hit parts' callers keep the
 *   other units' views (`ai/ai_npc.h`'s `_AINPC_W`, a local view of `_PLW::physics_0x13C`).
 * SHAPES. The move-type test is `(u8)(type + 0xFF) <= 1` (retail `addi 0xff` + `clrlwi`; `- 1` gives `subi`, a
 *   `switch` a jump); a part-list skip advances the pointer before `continue`, and the body-list skips in
 *   `body_push_pl_pass` also leave the loop on a null link (retail repeats `lwz`/`cmpwi`/`beq` per skip); the shape
 *   test in `body_shape_get` is the range test `(u32)(shape - 1) > 1` with the sphere first; `coll_push_out` and
 *   `coll_push_out_em` take the radius second (its `fmr` comes before the integer moves).
 */

#include "types.h"
#include "mh3_pad.h"
#include "nw4r/math.h"
#include "pl.h"
#include "fn_8004CAD8.h"
#include "ef/effect.h"            /* `eft_vec_ctor`, `eft_shape_init` (rule 2) */
#include "ef/eft_capsule_ctor.h"
#include "g3d/g3d_calcworld.h"    /* `addVec3To` (rule 2) */
#include "Pl/fn_8025F088.h"       /* `get_move_work_adrs` / `get_move_work_max` (rule 2) */
#include "Pl/pl_hit_sphere.h"     /* `PlCapsule`, `PlCapsuleBox`, `pl_capsule_box_set` (rule 2) */
#include "Pl/pl_coll.h"           /* `HitSphere` and the push tests (rule 2) */
#include "Pl/pl_joint_body.h"
#include "Pl/pl_hit.h"            /* the body list, the part predicates and `em_part_sphere_get` (rule 2) */
#include "ai/ai_npc.h"            /* `_AINPC_W` */
#include "ai/ainpc_work_get.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/EnemyData.h"
#include "enemy/em_common.h"      /* `em_data_kind_get` (rule 2) */
#include "enemy/em_flags836_ck.h"
#include "enemy/fn_801251D0.h"    /* `get_enemy_data` (rule 2) */
#include "enemy/fn_8012EC74.h"    /* `get_em_chg_scale` (rule 2) */
#include "stage/stg_w.h"          /* `get_now_areano` (rule 2) */
#include "menu/coll_push_out.h"

/* The player's push-out drift (`_PLW::drift_vel_0x574`, byte-declared in `Pl/plw.h`). */
static inline nw4r::math::VEC3* pl_drift(_PLW* plw)
{
    return (nw4r::math::VEC3*)plw->drift_vel_0x574;
}

extern "C" {
u32 pl_push_vertical_ck(_PLW* plw);
u32 npc_push_vertical_ck(_AINPC_W* npc);
void body_push_reset(void);
void body_push_pl_pass(void);
void body_push_em_pass(void);
void body_push_npc_pass(void);
void body_push_pl_em(_PLW* plw, _ENEMY_WORK* em);
void body_push_pl_body(_PLW* plw, _BODY_W* body);
void body_push_em_em(_ENEMY_WORK* a, _ENEMY_WORK* b);
void body_push_npc_em(_AINPC_W* npc, _ENEMY_WORK* em);
void pl_push_capsule_get(PlCapsule* out, _PLW* plw);
void npc_push_capsule_get(PlCapsule* out, _AINPC_W* npc);
void pl_push_accum(_PLW* plw, nw4r::math::VEC3* sum, f32* max_sq, nw4r::math::VEC3* push);
void npc_push_accum(_AINPC_W* npc, nw4r::math::VEC3* sum, f32* max_sq, nw4r::math::VEC3* push);
void em_push_accum(_ENEMY_WORK* em, nw4r::math::VEC3* sum, f32* max_sq, nw4r::math::VEC3* push);
void push_vec_finish(nw4r::math::VEC3* sum, f32* max_sq);
void body_push_pl_em_apply(_PLW* plw, nw4r::math::VEC3* pl_push, _ENEMY_WORK* em, nw4r::math::VEC3* em_push);
void body_push_npc_em_apply(_AINPC_W* npc, nw4r::math::VEC3* npc_push, _ENEMY_WORK* em, nw4r::math::VEC3* em_push);
void body_push_em_em_apply(_ENEMY_WORK* a, nw4r::math::VEC3* a_push, _ENEMY_WORK* b, nw4r::math::VEC3* b_push);
void pl_push_drift_add(_PLW* plw, nw4r::math::VEC3* push);
void pl_push_drift_set(_PLW* plw, nw4r::math::VEC3* drift);
void npc_push_drift_set(_AINPC_W* npc, nw4r::math::VEC3* drift);
void em_push_drift_set(_ENEMY_WORK* em, nw4r::math::VEC3* drift);
void em_push_sphere_get(HitSphere* out, _ENEMY_WORK* em);
void push_accum(nw4r::math::VEC3* sum, f32* max_sq, nw4r::math::VEC3* push);
void coll_push_out_em(nw4r::math::VEC3* pos, f32 radius, _ENEMY_WORK* em, u8 list);
s32 body_shape_get(_BODY_W* body, EmWeakPoint* shape, PlCapsule* capsule, HitSphere* sphere);
}

/* 0x80297E34 (0x2C): the frame's push pass: clears every drift, then pushes players, enemies and the NPC apart. */
extern "C" void hit_lists_init(void)
{
    body_push_reset();
    body_push_pl_pass();
    body_push_em_pass();
    body_push_npc_pass();
}

/* 0x80297E60 (0x14): whether the player's push moves it vertically too (its +0x009 state is 2). */
extern "C" u32 pl_push_vertical_ck(_PLW* plw)
{
    return plw->kind_0x09 == 2;
}

/* 0x80297E74 (0x14): whether the NPC's push moves it vertically too (its variant is 1). */
extern "C" u32 npc_push_vertical_ck(_AINPC_W* npc)
{
    return npc->variant == 1;
}

/* 0x80297E88 (0x11C): clears the drift of every live player, enemy and the NPC. */
extern "C" void body_push_reset(void)
{
    u16 count;
    _PLW* plw;
    u32 i;
    _ENEMY_WORK* em;
    _AINPC_W* npc;

    count = get_move_work_max(2);
    plw = (_PLW*)get_move_work_adrs(2);
    for (i = 0; i < count; i++, plw++) {
        if (plw->slot_active != 0) {
            setVector3(pl_drift(plw), 0.0f, 0.0f, 0.0f);
            plw->drift_on_0x582 = 0;
        }
    }
    count = get_move_work_max(3);
    em = (_ENEMY_WORK*)get_move_work_adrs(3);
    for (i = 0; i < count; i++, em++) {
        if (em->active != 0) {
            setVector3(&em->drift_vel_0x8B4, 0.0f, 0.0f, 0.0f);
            em->drift_on_0x8C0 = 0;
        }
    }
    npc = (_AINPC_W*)ainpc_work_get();
    if (npc->active != 0) {
        setVector3(&npc->drift_vel, 0.0f, 0.0f, 0.0f);
        npc->drift_active = 0;
    }
}

/* 0x80297FA4 (0x164): pushes every live player against the enemies of its area and the body slots of its area. */
extern "C" void body_push_pl_pass(void)
{
    _PLW* plw;
    u32 i;
    u16 pl_count;
    u16 em_count;

    pl_count = get_move_work_max(2);
    em_count = get_move_work_max(3);
    plw = (_PLW*)get_move_work_adrs(2);
    for (i = 0; i < pl_count; i++, plw++) {
        if (plw->slot_active != 0 && plw->field_0x580 <= 0) {
            _ENEMY_WORK* em = (_ENEMY_WORK*)get_move_work_adrs(3);
            u32 j;
            _BODY_W* body;

            for (j = 0; j < em_count; j++, em++) {
                if (em->active != 0 && em->fx_timer_0x8C2 <= 0 && plw->area_0x16 == em->area_no) {
                    body_push_pl_em(plw, em);
                }
            }
            body = hit_body_list_get();
            while (body != NULL) {
                if (body->field_0x005 == 0) {
                    body = body->next_0x00;
                    if (body == NULL) {
                        break;
                    }
                    continue;
                }
                if ((body->field_0x03F & 1) == 0) {
                    body = body->next_0x00;
                    if (body == NULL) {
                        break;
                    }
                    continue;
                }
                if (plw->area_0x16 != body->source) {
                    body = body->next_0x00;
                    if (body == NULL) {
                        break;
                    }
                    continue;
                }
                switch (body->kind) {
                case 2:
                    body_push_pl_body(plw, body);
                    break;
                }
                body = body->next_0x00;
            }
        }
    }
}

/* 0x80298108 (0x168): pushes every pair of live enemies in one area apart (two flying kind-9 enemies never). */
extern "C" void body_push_em_pass(void)
{
    _ENEMY_WORK* a;
    _ENEMY_WORK* base;
    u16 count;
    s32 i;

    count = get_move_work_max(3);
    base = (_ENEMY_WORK*)get_move_work_adrs(3);
    for (i = 0, a = base; i < count - 1; a++, i++) {
        if (a->active != 0 && a->fx_timer_0x8C2 <= 0 && a->fx_timer_0x8C4 <= 0) {
            s32 j;
            _ENEMY_WORK* b;

            for (j = i + 1, b = base + j; j < count; b++, j++) {
                if (b->active != 0 && b->fx_timer_0x8C2 <= 0 && b->fx_timer_0x8C4 <= 0 && a->area_no == b->area_no
                    && (em_data_kind_get(a) != 9 || em_data_kind_get(b) != 9)
                    && (a->fx_timer_0x8C6 <= 0 || em_data_kind_get(a) > em_data_kind_get(b))
                    && (b->fx_timer_0x8C6 <= 0 || em_data_kind_get(b) > em_data_kind_get(a))) {
                    body_push_em_em(a, b);
                }
            }
        }
    }
}

/* 0x80298270 (0xC0): pushes the NPC against every live enemy of its area. */
extern "C" void body_push_npc_pass(void)
{
    _AINPC_W* npc;
    _ENEMY_WORK* em;
    u16 count;
    u32 i;

    npc = (_AINPC_W*)ainpc_work_get();
    count = get_move_work_max(3);
    if (npc->active != 0 && npc->field_0x47C <= 0) {
        em = (_ENEMY_WORK*)get_move_work_adrs(3);
        for (i = 0; i < count; i++, em++) {
            if (em->active != 0 && em->fx_timer_0x8C2 <= 0 && npc->area == em->area_no) {
                body_push_npc_em(npc, em);
            }
        }
    }
}

/* 0x80298330 (0x2BC): pushes a player's body capsule against the enemy's hit parts (at most three contacts) and
 * splits the summed push between the two by the enemy's size; the player is kept between its floor and 480 above it. */
extern "C" void body_push_pl_em(_PLW* plw, _ENEMY_WORK* em)
{
    PlCapsuleBox pl_box;
    PlCapsuleBox part_box;
    PlCapsule pl_capsule;
    PlCapsule part_capsule;
    HitSphere sphere;
    nw4r::math::VEC3 pl_sum;
    nw4r::math::VEC3 em_sum;
    nw4r::math::VEC3 pl_push;
    nw4r::math::VEC3 em_push;
    f32 pl_max;
    f32 em_max;
    EmWeakPoint* part;
    u8 hits;
    f32 start_y;

    eft_shape_init((struct EftShape*)&pl_box);
    eft_shape_init((struct EftShape*)&part_box);
    eft_capsule_ctor((nw4r::math::CAPSULE*)&pl_capsule);
    eft_capsule_ctor((nw4r::math::CAPSULE*)&part_capsule);
    eft_vec_ctor(&sphere.pos);
    VEC3_ctor(&pl_sum);
    VEC3_ctor(&em_sum);
    VEC3_ctor(&pl_push);
    VEC3_ctor(&em_push);
    hits = 0;
    pl_max = 0.0f;
    em_max = 0.0f;
    setVector3(&pl_sum, 0.0f, 0.0f, 0.0f);
    setVector3(&em_sum, 0.0f, 0.0f, 0.0f);
    start_y = plw->motion_pos_0x40;
    pl_push_capsule_get(&pl_capsule, plw);
    pl_capsule_box_set(&pl_capsule, &pl_box);
    part = em->parts_0x8C8;
    if (part != NULL) {
        while (part->joint != -1) {
            if (hit_push_enabled_ck(part) == 0 || em_flags836_ck(em, part->part) == 1) {
                part++;
                continue;
            }
            switch (em_part_sphere_get(em, part, &part_capsule, &sphere)) {
            case 0:
                if (pl_capsule_sphere_push(&pl_box, &sphere, &pl_push, &em_push) == 1) {
                    pl_push_accum(plw, &pl_sum, &pl_max, &pl_push);
                    em_push_accum(em, &em_sum, &em_max, &em_push);
                    hits++;
                }
                break;
            case 1:
                pl_capsule_box_set(&part_capsule, &part_box);
                if (pl_capsule_capsule_push(&pl_box, &part_box, &pl_push, &em_push) == 1) {
                    pl_push_accum(plw, &pl_sum, &pl_max, &pl_push);
                    em_push_accum(em, &em_sum, &em_max, &em_push);
                    hits++;
                }
                break;
            }
            if (hits >= 3) {
                break;
            }
            part++;
        }
    }
    if (hits != 0) {
        f32 floor_y;

        push_vec_finish(&pl_sum, &pl_max);
        push_vec_finish(&em_sum, &em_max);
        if ((u32)(plw->field_0x00A - 6) <= 2 && plw->motion_vel_0x7C > 0.0f && pl_sum.y > 0.0f) {
            pl_sum.y = 0.0f;
        }
        body_push_pl_em_apply(plw, &pl_sum, em, &em_sum);
        if (plw->motion_pos_0x40 + pl_drift(plw)->y < plw->ground_y_0x060) {
            pl_drift(plw)->y = plw->ground_y_0x060 - plw->motion_pos_0x40;
        } else {
            floor_y = 480.0f + plw->ground_y_0x060;
            if (start_y < floor_y && plw->motion_pos_0x40 + pl_drift(plw)->y > floor_y) {
                pl_drift(plw)->y = floor_y - plw->motion_pos_0x40;
            }
        }
    }
}

/* 0x802985EC (0x204): pushes a player's body capsule out of one body slot's shape (only the player moves). */
extern "C" void body_push_pl_body(_PLW* plw, _BODY_W* body)
{
    PlCapsuleBox pl_box;
    PlCapsuleBox body_box;
    PlCapsule pl_capsule;
    PlCapsule body_capsule;
    HitSphere sphere;
    nw4r::math::VEC3 pl_sum;
    nw4r::math::VEC3 body_sum;
    nw4r::math::VEC3 pl_push;
    nw4r::math::VEC3 body_push;
    f32 pl_max;
    f32 start_y;
    f32 floor_y;

    eft_shape_init((struct EftShape*)&pl_box);
    eft_shape_init((struct EftShape*)&body_box);
    eft_capsule_ctor((nw4r::math::CAPSULE*)&pl_capsule);
    eft_capsule_ctor((nw4r::math::CAPSULE*)&body_capsule);
    eft_vec_ctor(&sphere.pos);
    VEC3_ctor(&pl_sum);
    VEC3_ctor(&body_sum);
    VEC3_ctor(&pl_push);
    VEC3_ctor(&body_push);
    pl_max = 0.0f;
    setVector3(&pl_sum, 0.0f, 0.0f, 0.0f);
    setVector3(&body_sum, 0.0f, 0.0f, 0.0f);
    start_y = plw->motion_pos_0x40;
    pl_push_capsule_get(&pl_capsule, plw);
    pl_capsule_box_set(&pl_capsule, &pl_box);
    if (body->shape_0x0C != NULL) {
        switch (body_shape_get(body, body->shape_0x0C, &body_capsule, &sphere)) {
        case 0:
            if (pl_capsule_sphere_push(&pl_box, &sphere, &pl_push, &body_push) == 1) {
                pl_push_accum(plw, &pl_sum, &pl_max, &pl_push);
            }
            break;
        case 1:
            pl_capsule_box_set(&body_capsule, &body_box);
            if (pl_capsule_capsule_push(&pl_box, &body_box, &pl_push, &body_push) == 1) {
                pl_push_accum(plw, &pl_sum, &pl_max, &pl_push);
            }
            break;
        }
    }
    push_vec_finish(&pl_sum, &pl_max);
    if ((u32)(plw->field_0x00A - 6) <= 2 && plw->motion_vel_0x7C > 0.0f && pl_sum.y > 0.0f) {
        pl_sum.y = 0.0f;
    }
    pl_push_drift_add(plw, &pl_sum);
    if (plw->motion_pos_0x40 + pl_drift(plw)->y < plw->ground_y_0x060) {
        pl_drift(plw)->y = plw->ground_y_0x060 - plw->motion_pos_0x40;
    } else {
        floor_y = 480.0f + plw->ground_y_0x060;
        if (start_y < floor_y && plw->motion_pos_0x40 + pl_drift(plw)->y > floor_y) {
            pl_drift(plw)->y = floor_y - plw->motion_pos_0x40;
        }
    }
}

/* 0x802987F0 (0x190): pushes two enemies' body spheres apart; a walker's push stays horizontal. */
extern "C" void body_push_em_em(_ENEMY_WORK* a, _ENEMY_WORK* b)
{
    HitSphere a_sphere;
    HitSphere b_sphere;
    nw4r::math::VEC3 a_push;
    nw4r::math::VEC3 b_push;
    nw4r::math::VEC3 a_sum;
    nw4r::math::VEC3 b_sum;
    f32 a_max;
    f32 b_max;
    s32 hit;

    eft_vec_ctor(&a_sphere.pos);
    eft_vec_ctor(&b_sphere.pos);
    VEC3_ctor(&a_push);
    VEC3_ctor(&b_push);
    VEC3_ctor(&a_sum);
    VEC3_ctor(&b_sum);
    hit = 0;
    a_max = 0.0f;
    b_max = 0.0f;
    setVector3(&a_sum, 0.0f, 0.0f, 0.0f);
    setVector3(&b_sum, 0.0f, 0.0f, 0.0f);
    em_push_sphere_get(&a_sphere, a);
    em_push_sphere_get(&b_sphere, b);
    if (pl_sphere_push(&a_sphere, &b_sphere, &a_push) == 1) {
        pl_sphere_push(&b_sphere, &a_sphere, &b_push);
        em_push_accum(a, &a_sum, &a_max, &a_push);
        em_push_accum(b, &b_sum, &b_max, &b_push);
        hit = 1;
    }
    if (hit != 0) {
        if (a->field_0x1E2 != 1 && a->field_0x1E2 != 2 && a_sum.y > 0.0f) {
            a_sum.y = 0.0f;
        }
        if (b->field_0x1E2 != 1 && b->field_0x1E2 != 2 && b_sum.y > 0.0f) {
            b_sum.y = 0.0f;
        }
        push_vec_finish(&a_sum, &a_max);
        push_vec_finish(&b_sum, &b_max);
        body_push_em_em_apply(a, &a_sum, b, &b_sum);
    }
}

/* 0x80298980 (0x2BC): the NPC's form of `body_push_pl_em`. */
extern "C" void body_push_npc_em(_AINPC_W* npc, _ENEMY_WORK* em)
{
    PlCapsuleBox npc_box;
    PlCapsuleBox part_box;
    PlCapsule npc_capsule;
    PlCapsule part_capsule;
    HitSphere sphere;
    nw4r::math::VEC3 npc_sum;
    nw4r::math::VEC3 em_sum;
    nw4r::math::VEC3 npc_push;
    nw4r::math::VEC3 em_push;
    f32 npc_max;
    f32 em_max;
    EmWeakPoint* part;
    u8 hits;
    f32 start_y;

    eft_shape_init((struct EftShape*)&npc_box);
    eft_shape_init((struct EftShape*)&part_box);
    eft_capsule_ctor((nw4r::math::CAPSULE*)&npc_capsule);
    eft_capsule_ctor((nw4r::math::CAPSULE*)&part_capsule);
    eft_vec_ctor(&sphere.pos);
    VEC3_ctor(&npc_sum);
    VEC3_ctor(&em_sum);
    VEC3_ctor(&npc_push);
    VEC3_ctor(&em_push);
    hits = 0;
    npc_max = 0.0f;
    em_max = 0.0f;
    setVector3(&npc_sum, 0.0f, 0.0f, 0.0f);
    setVector3(&em_sum, 0.0f, 0.0f, 0.0f);
    start_y = npc->pos_0x178.y;
    npc_push_capsule_get(&npc_capsule, npc);
    pl_capsule_box_set(&npc_capsule, &npc_box);
    part = em->parts_0x8C8;
    if (part != NULL) {
        while (part->joint != -1) {
            if (hit_push_enabled_ck(part) == 0 || em_flags836_ck(em, part->part) == 1) {
                part++;
                continue;
            }
            switch (em_part_sphere_get(em, part, &part_capsule, &sphere)) {
            case 0:
                if (pl_capsule_sphere_push(&npc_box, &sphere, &npc_push, &em_push) == 1) {
                    npc_push_accum(npc, &npc_sum, &npc_max, &npc_push);
                    em_push_accum(em, &em_sum, &em_max, &em_push);
                    hits++;
                }
                break;
            case 1:
                pl_capsule_box_set(&part_capsule, &part_box);
                if (pl_capsule_capsule_push(&npc_box, &part_box, &npc_push, &em_push) == 1) {
                    npc_push_accum(npc, &npc_sum, &npc_max, &npc_push);
                    em_push_accum(em, &em_sum, &em_max, &em_push);
                    hits++;
                }
                break;
            }
            if (hits >= 3) {
                break;
            }
            part++;
        }
    }
    if (hits != 0) {
        f32 floor_y;

        push_vec_finish(&npc_sum, &npc_max);
        push_vec_finish(&em_sum, &em_max);
        if ((u32)(npc->motion - 4) <= 1 && npc->pos_0x1D4.y > 0.0f && npc_sum.y > 0.0f) {
            npc_sum.y = 0.0f;
        }
        body_push_npc_em_apply(npc, &npc_sum, em, &em_sum);
        if (npc->pos_0x178.y + npc->drift_vel.y < npc->field_0x19C) {
            npc->drift_vel.y = npc->field_0x19C - npc->pos_0x178.y;
        } else {
            floor_y = 480.0f + npc->field_0x19C;
            if (start_y < floor_y && npc->pos_0x178.y + npc->drift_vel.y > floor_y) {
                npc->drift_vel.y = floor_y - npc->pos_0x178.y;
            }
        }
    }
}

/* 0x80298C3C (0x194): the player's body capsule: from the midpoint of joints 0x11/0x14 towards joint 0xD, shortened
 * by the 40.0 radius at both ends, all moved by the frame's drift. */
extern "C" void pl_push_capsule_get(PlCapsule* out, _PLW* plw)
{
    nw4r::math::VEC3 hip_a;
    nw4r::math::VEC3 hip_b;
    nw4r::math::VEC3 axis;
    nw4r::math::VEC3 mid;
    nw4r::math::VEC3 head;
    nw4r::math::VEC3 dir;
    nw4r::math::VEC3 tmp0;
    nw4r::math::VEC3 tmp1;
    nw4r::math::VEC3 tmp2;
    nw4r::math::VEC3 tmp3;
    nw4r::math::VEC3 tmp4;

    VEC3_ctor(&hip_a);
    VEC3_ctor(&hip_b);
    VEC3_ctor(&axis);
    VEC3_ctor(&mid);
    VEC3_ctor(&head);
    VEC3_ctor(&dir);
    ((PlJointBody*)plw->physics_0x13C)->chr_0x04.get_joint_wpos(0x14, &hip_b);
    ((PlJointBody*)plw->physics_0x13C)->chr_0x04.get_joint_wpos(0x11, &hip_a);
    addVec3To(&hip_a, pl_drift(plw));
    addVec3To(&hip_b, pl_drift(plw));
    addVec3(&tmp0, &hip_a, &hip_b);
    copyVec3(&mid, &tmp0);
    vec3_scale_in_place(&mid, 0.5f);
    ((PlJointBody*)plw->physics_0x13C)->chr_0x04.get_joint_wpos(0xD, &head);
    addVec3(&tmp1, &head, pl_drift(plw));
    copyVec3(&head, &tmp1);
    subVec3(&tmp2, &head, &mid);
    copyVec3(&axis, &tmp2);
    if (vec3_length_sq(&axis.x) > 0.001f) {
        vec3_normalize_into(&dir, &axis);
    } else {
        copyVec3(&dir, &axis);
    }
    vec3_scale_in_place(&dir, 40.0f);
    addVec3(&tmp3, &mid, &dir);
    copyVec3(&out->start, &tmp3);
    subVec3(&tmp4, &head, &dir);
    copyVec3(&out->end, &tmp4);
    out->radius = 40.0f;
}

/* 0x80298DD0 (0x188): the NPC's body capsule, the same construction on joints 0x12/0x14 and 0xE with a 30.0 radius. */
extern "C" void npc_push_capsule_get(PlCapsule* out, _AINPC_W* npc)
{
    nw4r::math::VEC3 hip_a;
    nw4r::math::VEC3 hip_b;
    nw4r::math::VEC3 axis;
    nw4r::math::VEC3 mid;
    nw4r::math::VEC3 head;
    nw4r::math::VEC3 dir;
    nw4r::math::VEC3 tmp0;
    nw4r::math::VEC3 tmp1;
    nw4r::math::VEC3 tmp2;
    nw4r::math::VEC3 tmp3;
    nw4r::math::VEC3 tmp4;

    VEC3_ctor(&hip_a);
    VEC3_ctor(&hip_b);
    VEC3_ctor(&axis);
    VEC3_ctor(&mid);
    VEC3_ctor(&head);
    VEC3_ctor(&dir);
    npc->model.get_joint_wpos(0x14, &hip_b);
    npc->model.get_joint_wpos(0x12, &hip_a);
    addVec3To(&hip_a, &npc->drift_vel);
    addVec3To(&hip_b, &npc->drift_vel);
    addVec3(&tmp0, &hip_a, &hip_b);
    copyVec3(&mid, &tmp0);
    vec3_scale_in_place(&mid, 0.5f);
    npc->model.get_joint_wpos(0xE, &head);
    addVec3(&tmp1, &head, &npc->drift_vel);
    copyVec3(&head, &tmp1);
    subVec3(&tmp2, &head, &mid);
    copyVec3(&axis, &tmp2);
    if (vec3_length_sq(&axis.x) > 0.001f) {
        vec3_normalize_into(&dir, &axis);
    } else {
        copyVec3(&dir, &axis);
    }
    vec3_scale_in_place(&dir, 40.0f);
    addVec3(&tmp3, &mid, &dir);
    copyVec3(&out->start, &tmp3);
    subVec3(&tmp4, &head, &dir);
    copyVec3(&out->end, &tmp4);
    out->radius = 30.0f;
}

/* 0x80298F58 (0xC4): adds one contact's push to the player's sum (the height only while it moves vertically) and keeps
 * the largest squared push. */
extern "C" void pl_push_accum(_PLW* plw, nw4r::math::VEC3* sum, f32* max_sq, nw4r::math::VEC3* push)
{
    nw4r::math::VEC3 unused;
    f32 len_sq;

    VEC3_ctor(&unused);
    len_sq = vec3_length_sq(&push->x);
    sum->x += push->x;
    if (pl_push_vertical_ck(plw) == 1) {
        sum->y += push->y;
    }
    sum->z += push->z;
    if (len_sq > *max_sq) {
        *max_sq = len_sq;
    }
}

/* 0x8029901C (0xC4): the NPC's form of `pl_push_accum`. */
extern "C" void npc_push_accum(_AINPC_W* npc, nw4r::math::VEC3* sum, f32* max_sq, nw4r::math::VEC3* push)
{
    nw4r::math::VEC3 unused;
    f32 len_sq;

    VEC3_ctor(&unused);
    len_sq = vec3_length_sq(&push->x);
    sum->x += push->x;
    if (npc_push_vertical_ck(npc) == 1) {
        sum->y += push->y;
    }
    sum->z += push->z;
    if (len_sq > *max_sq) {
        *max_sq = len_sq;
    }
}

/* 0x802990E0 (0xB0): the enemy's form: flying and swimming enemies (move type 1/2) take the height too. */
extern "C" void em_push_accum(_ENEMY_WORK* em, nw4r::math::VEC3* sum, f32* max_sq, nw4r::math::VEC3* push)
{
    nw4r::math::VEC3 unused;
    f32 len_sq;

    VEC3_ctor(&unused);
    len_sq = vec3_length_sq(&push->x);
    sum->x += push->x;
    if ((u8)(em->field_0x1E2 + 0xFF) <= 1) {
        sum->y += push->y;
    }
    sum->z += push->z;
    if (len_sq > *max_sq) {
        *max_sq = len_sq;
    }
}

/* 0x80299190 (0x60): turns a summed push into its direction scaled to the largest single push. */
extern "C" void push_vec_finish(nw4r::math::VEC3* sum, f32* max_sq)
{
    if (vec3_length_sq(&sum->x) > 0.001f) {
        vec3_normalize_into(sum, sum);
    }
    vec3_scale_in_place(sum, sqrt_f32(*max_sq));
}

/* 0x802991F0 (0x148): applies a player/enemy push: a small enemy (kind < 4) takes it all, a kind-4 one half each, a
 * bigger one leaves it all to the player. */
extern "C" void body_push_pl_em_apply(_PLW* plw, nw4r::math::VEC3* pl_push, _ENEMY_WORK* em, nw4r::math::VEC3* em_push)
{
    nw4r::math::VEC3 pl_vec;
    nw4r::math::VEC3 em_vec;
    nw4r::math::VEC3 pl_half;
    nw4r::math::VEC3 em_half;
    u8 kind;

    VEC3_ctor(&pl_vec);
    VEC3_ctor(&em_vec);
    kind = em_data_kind_get(em);
    if (kind < 4) {
        copyVec3(&em_vec, &em->drift_vel_0x8B4);
        addVec3To(&em_vec, em_push);
        em->drift_on_0x8C0 = 1;
        em_push_drift_set(em, &em_vec);
    } else if (kind == 4) {
        copyVec3(&pl_vec, pl_drift(plw));
        vec3_scale(&pl_half, pl_push, 0.5f);
        addVec3To(&pl_vec, &pl_half);
        plw->drift_on_0x582 = 1;
        pl_push_drift_set(plw, &pl_vec);
        copyVec3(&em_vec, &em->drift_vel_0x8B4);
        vec3_scale(&em_half, em_push, 0.5f);
        addVec3To(&em_vec, &em_half);
        em->drift_on_0x8C0 = 1;
        em_push_drift_set(em, &em_vec);
    } else {
        copyVec3(&pl_vec, pl_drift(plw));
        addVec3To(&pl_vec, pl_push);
        plw->drift_on_0x582 = 1;
        pl_push_drift_set(plw, &pl_vec);
    }
}

/* 0x80299338 (0x148): the NPC's form of `body_push_pl_em_apply` (the split kind is 3). */
extern "C" void body_push_npc_em_apply(_AINPC_W* npc, nw4r::math::VEC3* npc_push, _ENEMY_WORK* em,
                                       nw4r::math::VEC3* em_push)
{
    nw4r::math::VEC3 npc_vec;
    nw4r::math::VEC3 em_vec;
    nw4r::math::VEC3 npc_half;
    nw4r::math::VEC3 em_half;
    u8 kind;

    VEC3_ctor(&npc_vec);
    VEC3_ctor(&em_vec);
    kind = em_data_kind_get(em);
    if (kind < 3) {
        copyVec3(&em_vec, &em->drift_vel_0x8B4);
        addVec3To(&em_vec, em_push);
        em->drift_on_0x8C0 = 1;
        em_push_drift_set(em, &em_vec);
    } else if (kind == 3) {
        copyVec3(&npc_vec, &npc->drift_vel);
        vec3_scale(&npc_half, npc_push, 0.5f);
        addVec3To(&npc_vec, &npc_half);
        npc->drift_active = 1;
        npc_push_drift_set(npc, &npc_vec);
        copyVec3(&em_vec, &em->drift_vel_0x8B4);
        vec3_scale(&em_half, em_push, 0.5f);
        addVec3To(&em_vec, &em_half);
        em->drift_on_0x8C0 = 1;
        em_push_drift_set(em, &em_vec);
    } else {
        copyVec3(&npc_vec, &npc->drift_vel);
        addVec3To(&npc_vec, npc_push);
        npc->drift_active = 1;
        npc_push_drift_set(npc, &npc_vec);
    }
}

/* 0x80299480 (0x234): applies an enemy/enemy push: the smaller kind takes it all, equal kinds half each. */
extern "C" void body_push_em_em_apply(_ENEMY_WORK* a, nw4r::math::VEC3* a_push, _ENEMY_WORK* b,
                                      nw4r::math::VEC3* b_push)
{
    nw4r::math::VEC3 a_vec;
    nw4r::math::VEC3 b_vec;
    u8 a_kind;
    u8 b_kind;

    VEC3_ctor(&a_vec);
    VEC3_ctor(&b_vec);
    a_kind = em_data_kind_get(a);
    b_kind = em_data_kind_get(b);
    if (a_kind > b_kind) {
        copyVec3(&b_vec, &b->drift_vel_0x8B4);
        b_vec.x += b_push->x;
        if ((u8)(b->field_0x1E2 + 0xFF) <= 1) {
            b_vec.y += b_push->y;
        }
        b_vec.z += b_push->z;
        b->drift_on_0x8C0 = 1;
        em_push_drift_set(b, &b_vec);
    } else if (a_kind == b_kind) {
        copyVec3(&a_vec, &a->drift_vel_0x8B4);
        a_vec.x += 0.5f * a_push->x;
        if ((u8)(a->field_0x1E2 + 0xFF) <= 1) {
            a_vec.y += 0.5f * a_push->y;
        }
        a_vec.z += 0.5f * a_push->z;
        a->drift_on_0x8C0 = 1;
        em_push_drift_set(a, &a_vec);
        copyVec3(&b_vec, &b->drift_vel_0x8B4);
        b_vec.x += 0.5f * b_push->x;
        if ((u8)(b->field_0x1E2 + 0xFF) <= 1) {
            b_vec.y += 0.5f * b_push->y;
        }
        b_vec.z += 0.5f * b_push->z;
        b->drift_on_0x8C0 = 1;
        em_push_drift_set(b, &b_vec);
    } else {
        copyVec3(&a_vec, &a->drift_vel_0x8B4);
        a_vec.x += a_push->x;
        if ((u8)(a->field_0x1E2 + 0xFF) <= 1) {
            a_vec.y += a_push->y;
        }
        a_vec.z += a_push->z;
        a->drift_on_0x8C0 = 1;
        em_push_drift_set(a, &a_vec);
    }
}

/* 0x802996B4 (0x68): adds a push to the player's drift and switches the drift on. */
extern "C" void pl_push_drift_add(_PLW* plw, nw4r::math::VEC3* push)
{
    nw4r::math::VEC3 drift;

    VEC3_ctor(&drift);
    copyVec3(&drift, pl_drift(plw));
    addVec3To(&drift, push);
    plw->drift_on_0x582 = 1;
    pl_push_drift_set(plw, &drift);
}

/* 0x8029971C (0x64): stores the player's drift, dropping the height unless the player moves vertically. */
extern "C" void pl_push_drift_set(_PLW* plw, nw4r::math::VEC3* drift)
{
    pl_drift(plw)->x = drift->x;
    if (pl_push_vertical_ck(plw) != 0) {
        pl_drift(plw)->y = drift->y;
    } else {
        pl_drift(plw)->y = 0.0f;
    }
    pl_drift(plw)->z = drift->z;
}

/* 0x80299780 (0x64): the NPC's form of `pl_push_drift_set`. */
extern "C" void npc_push_drift_set(_AINPC_W* npc, nw4r::math::VEC3* drift)
{
    npc->drift_vel.x = drift->x;
    if (npc_push_vertical_ck(npc) != 0) {
        npc->drift_vel.y = drift->y;
    } else {
        npc->drift_vel.y = 0.0f;
    }
    npc->drift_vel.z = drift->z;
}

/* 0x802997E4 (0x3C): the enemy's form: flying and swimming enemies keep the height. */
extern "C" void em_push_drift_set(_ENEMY_WORK* em, nw4r::math::VEC3* drift)
{
    em->drift_vel_0x8B4.x = drift->x;
    if ((u8)(em->field_0x1E2 + 0xFF) <= 1) {
        em->drift_vel_0x8B4.y = drift->y;
    } else {
        em->drift_vel_0x8B4.y = 0.0f;
    }
    em->drift_vel_0x8B4.z = drift->z;
}

/* 0x80299820 (0x128): the enemy's body sphere: its drifted position plus the size record's centre offset for its move
 * type, scaled by the enemy's size, with the scaled radius. */
extern "C" void em_push_sphere_get(HitSphere* out, _ENEMY_WORK* em)
{
    nw4r::math::VEC3 center;
    nw4r::math::VEC3 offset;
    EmSizeParams* size;
    nw4r::math::VEC3 tmp0;
    nw4r::math::VEC3 tmp1;
    nw4r::math::VEC3 tmp3;
    nw4r::math::VEC3 tmp2;
    f32 scale;
    nw4r::math::VEC3* src;

    VEC3_ctor(&center);
    VEC3_ctor(&offset);
    size = get_enemy_data(em)->size_0x54;
    addVec3(&tmp0, &em->pos, &em->drift_vel_0x8B4);
    copyVec3(&center, &tmp0);
    scale = get_em_chg_scale(em);
    if (em->field_0x1E2 == 1) {
        if (em->field_0x1E3 == 1) {
            src = &size->center_0x18;
        } else {
            src = &size->center_0x0C;
        }
    } else if (em->field_0x1E2 == 2) {
        src = &size->center_0x0C;
    } else {
        src = &size->center_0x00;
    }
    copyVec3(&offset, setVec3(&tmp1, src->x, src->y, src->z));
    vec3_scale(&tmp2, &offset, scale);
    addVec3(&tmp3, &center, &tmp2);
    copyVec3(&out->pos, &tmp3);
    out->radius = size->radius_0x24 * scale;
}

/* 0x80299948 (0x90): adds one contact's push to a sum and keeps the largest squared push. */
extern "C" void push_accum(nw4r::math::VEC3* sum, f32* max_sq, nw4r::math::VEC3* push)
{
    nw4r::math::VEC3 unused;
    f32 len_sq;

    VEC3_ctor(&unused);
    len_sq = vec3_length_sq(&push->x);
    sum->x += push->x;
    sum->y += push->y;
    sum->z += push->z;
    if (len_sq > *max_sq) {
        *max_sq = len_sq;
    }
}

/* 0x802999D8 (0x23C): pushes the sphere at `pos` (of `radius`) out of one of an enemy's two hit-part lists. */
extern "C" void coll_push_out_em(nw4r::math::VEC3* pos, f32 radius, _ENEMY_WORK* em, u8 list)
{
    PlCapsuleBox part_box;
    HitSphere sphere;
    HitSphere part_sphere;
    PlCapsule part_capsule;
    nw4r::math::VEC3 push;
    nw4r::math::VEC3 part_push;
    nw4r::math::VEC3 sum;
    nw4r::math::VEC3 unused;
    f32 max_sq;
    u8 hits;
    EmWeakPoint* part;

    eft_shape_init((struct EftShape*)&part_box);
    eft_vec_ctor(&sphere.pos);
    eft_vec_ctor(&part_sphere.pos);
    eft_capsule_ctor((nw4r::math::CAPSULE*)&part_capsule);
    VEC3_ctor(&push);
    VEC3_ctor(&part_push);
    VEC3_ctor(&sum);
    VEC3_ctor(&unused);
    hits = 0;
    max_sq = 0.0f;
    setVector3(&sum, 0.0f, 0.0f, 0.0f);
    setVector3(&unused, 0.0f, 0.0f, 0.0f);
    copyVec3(&sphere.pos, pos);
    sphere.radius = radius;
    switch (list) {
    case 0:
        part = em->parts_0x8C8;
        break;
    case 1:
        part = em->parts_0x8CC;
        break;
    default:
        return;
    }
    if (part != NULL) {
        while (part->joint != -1) {
            if (hit_camera_push_enabled_ck(part) == 0 || em_flags836_ck(em, part->part) == 1) {
                part++;
                continue;
            }
            switch (em_part_sphere_get(em, part, &part_capsule, &part_sphere)) {
            case 0:
                if (pl_sphere_push(&sphere, &part_sphere, &push) == 1) {
                    push_accum(&sum, &max_sq, &push);
                    hits++;
                }
                break;
            case 1:
                pl_capsule_box_set(&part_capsule, &part_box);
                if (pl_capsule_sphere_push(&part_box, &sphere, &part_push, &push) == 1) {
                    push_accum(&sum, &max_sq, &push);
                    hits++;
                }
                break;
            }
            if (hits >= 3) {
                break;
            }
            part++;
        }
    }
    if (hits != 0) {
        push_vec_finish(&sum, &max_sq);
        pos->x += sum.x;
        pos->z += sum.z;
        pos->y += sum.y;
    }
}

/* 0x80299C14 (0xD0): pushes the camera's sphere out of every live enemy of the current area of at least `min_kind`. */
extern "C" void coll_push_out(nw4r::math::VEC3* pos, f32 radius, u8 min_kind, u8 list)
{
    u16 count;
    _ENEMY_WORK* em;
    s32 i;

    count = get_move_work_max(3);
    em = (_ENEMY_WORK*)get_move_work_adrs(3);
    for (i = 0; i < count - 1; i++, em++) {
        if (em->active != 0 && em->fx_timer_0x8C2 <= 0 && min_kind <= em_data_kind_get(em)
            && em->area_no == get_now_areano()) {
            coll_push_out_em(pos, radius, em, list);
        }
    }
}

/* 0x80299CE4 (0x1F4): turns a body slot's shape into a sphere (0) or a capsule (1) placed on its owner object. */
extern "C" s32 body_shape_get(_BODY_W* body, EmWeakPoint* shape, PlCapsule* capsule, HitSphere* sphere)
{
    nw4r::math::VEC3 ofs;
    nw4r::math::VEC3 ofs_b;
    nw4r::math::MTX34 mtx;
    BodyOwnerObj* owner;
    s32 result;

    VEC3_ctor(&ofs);
    VEC3_ctor(&ofs_b);
    MTX34_ctor(&mtx);
    result = 0;
    owner = body->owner_0x10;
    ofs.x = shape->offset.x;
    ofs.y = shape->offset.y;
    ofs.z = shape->offset.z;
    if (shape->joint == 1000) {
        sphere->radius = shape->radius;
        copyVec3(&sphere->pos, &owner->pos);
        cpSetRotMatrix(&owner->rot, &mtx);
        mulVecMat(&ofs, &mtx);
        sphere->pos.x += ofs.x;
        sphere->pos.y += ofs.y;
        sphere->pos.z += ofs.z;
        return 0;
    }
    cpSetRotMatrix(&owner->rot, &mtx);
    if ((u32)(shape->shape - 1) > 1) {
        if (shape->shape == 0) {
            sphere->radius = shape->radius;
            mulVecMat(&ofs, &mtx);
            sphere->pos.x += ofs.x;
            sphere->pos.y += ofs.y;
            sphere->pos.z += ofs.z;
            result = 0;
        }
    } else {
        capsule->radius = shape->radius;
        ofs_b.x = shape->offset_b.x;
        ofs_b.y = shape->offset_b.y;
        ofs_b.z = shape->offset_b.z;
        mulVecMat(&ofs, &mtx);
        capsule->start.x = ofs.x + mtx.m[0][3];
        capsule->start.y = ofs.y + mtx.m[1][3];
        capsule->start.z = ofs.z + mtx.m[2][3];
        mulVecMat(&ofs_b, &mtx);
        capsule->end.x = ofs_b.x + mtx.m[0][3];
        capsule->end.y = ofs_b.y + mtx.m[1][3];
        capsule->end.z = ofs_b.z + mtx.m[2][3];
        result = 1;
    }
    return result;
}
