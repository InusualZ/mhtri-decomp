/* ef/em_effect_ctrl.cpp - the enemy-effect (`em`) controller: `EmEffectWork`/`EmEffectUnit` and `fn_80101FA4`'s
 *   per-frame screen-box test.
 * RANGE. .text 0x80101FA4-0x80102994 (6 functions); extab 0x8000BEFC-0x8000BF1C, extabindex 0x80025D64-0x80025D94,
 *   .sdata 0x80791790-0x80791798, .sdata2 0x807966F0-0x80796740.  The player-weapon controller from 0x80102994 is
 *   `ef/eft007.cpp`.
 * FLAGS. `cflags_main`; `#pragma peephole off` for `fn_80101FA4`, `fn_801025FC` and `fn_801027D0` (retail keeps the
 *   unfused `subi`/`clrlwi`/`extsb`/`rlwinm` + `cmpwi` forms; playbook 39) and `#pragma fp_contract off` for
 *   `fn_80101FA4` (retail keeps `fmuls` + `fsubs`; playbook 40).
 * NAMES. The unit name is a GUESS (the range holds the `em` controller); the map has only `fn_` stems, so the
 *   definitions are `extern "C"` in a C++ unit.
 * RESIDUALS. 2 partial rows:
 *  - `fn_80101FA4`: one `lfd` of the int-to-f64 magic names our pool entry where retail names `lbl_80796728`, and
 *    the registers are coloured differently;
 *  - `fn_801027D0`: retail's frame places `eye` at +0x08, the probe copy at +0x18, `seg` at +0x28, `cam` at +0x34 and
 *    the quad at +0x40; MWCC allocates the five locals in reverse order whatever the declaration order.
 *   flipcheck: `.sdata` claimed, not emitted; `.text` (0x9E8 of 0x9F0) and `.sdata2` (0x10 of 0x50) short of the
 *   claim and differing; extabindex differs in 1 byte.
 *   Relocation names that differ from retail (pool constants, save helpers, statics): `lbl_80796708`.
 * SHAPES. `EmEffectUnit::handles` is an array member and `fn_80101FA4` writes `unit->handles[i]->field` inline:
 *   retail reloads the handle and `unit->entries[i]` at every access.
 *   The screen-box test and its off-screen accumulator use the negated comparisons (`!(x <= lo)`, `!(x >= hi)`,
 *   `x >= 640.0f`, `x <= 0.0f`): only the `<=`/`>=` spellings give retail's `fcmpo` + `cror`.
 *   `_EmHandle` here and `MHchar` in `ef/eft007.cpp` are two views of one map name whose fields disagree.
 */

#include "ef/eft_res_models_spawn.h" /* eft_res_models_spawn (rule 2: the owner's header) */
#include "ef/eft_res_slot_release.h" /* eft_res_slot_release (rule 2: the owner's header) */
#include "types.h"
#include "nw4r/math.h"
#include "Pl/plw.h"
#include "Pl/pl_hit_sphere.h"
#include "gx.h"
#include "ef/eft004.h"
#include "ef/eft009.h"
#include "unsplit/ef.h"
#include "g3d/g3d_scnroot.h" /* fn_80082BCC (rule 2) */
#include "unsplit/sound.h"
#include "unsplit/unknown.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the matrix helpers */
#include "ef/pRoot.h"
#include "ef/eft007_types.h"

/* --- callees and pool constants the whole unit shares ------------------------------------------- */


/* ===================================================================================================
 * BLOCK A - the `em` (monster/effect) controller family, 0x80101FA4..0x80102994
 * =================================================================================================== */

/* --- types -------------------------------------------------------------------------------------- */

/* The GX colour record (`_GXColor`) comes from `gx.h` - one definition, in the owner's header
 * (rule 1). */

/* One emitter handle: the character record the effect drives.  Only the fields this block reads are
 * named; the rest is padding.
 * size: 0x24 - a lower bound; the record continues past what this block reads */
struct _EmHandle {
    /* +0x00 */ u32 pad_0x00;
    /* +0x04 */ nw4r::math::VEC3 pos;
    /* +0x10 */ u8 pad_0x10[0xC];
    /* +0x1C */ f32 offset_x;
    /* +0x20 */ f32 offset_y;
};

/* One entry of the per-emitter table `EmEffectUnit::entries` (stride 0x14).
 * size: 0x14 */
struct EmEffectEntry {
    /* +0x00 */ u16 unused_0x00;
    /* +0x02 */ u8 intensity; /* 0..255, the emitter radius in pixels */
    /* +0x03 */ s8 follow;    /* 0: own position, 1: copy the first handle, 2: spawn no emitter */
    /* +0x04 */ f32 offset_x;
    /* +0x08 */ f32 offset_y;
    /* +0x0C */ f32 scale;
    /* +0x10 */ u8 color_r;
    /* +0x11 */ u8 color_g;
    /* +0x12 */ u8 color_b;
    /* +0x13 */ u8 alpha_base;
};

/* One entry of the per-emitter probe table `EmEffectUnit::probes` (stride 0x10): a candidate position
 * and the particle scale; a non-positive scale ends the table.
 * size: 0x10 */
struct EmEffectProbe {
    /* +0x00 */ nw4r::math::VEC3 pos;
    /* +0x0C */ f32 scale;
};

/* A line segment in nw4r space; the effect probes it for line of sight.
 * size: 0x18 */
struct EmEffectSegment {
    /* +0x00 */ nw4r::math::VEC3 from;
    /* +0x0C */ nw4r::math::VEC3 to;
};

/* Four positions a caller zeroes before the emitter is built; the effect uses them as a local frame.
 * size: 0x30 */
struct EmEffectQuad {
    /* +0x00 */ nw4r::math::VEC3 v[4];
};

/* The per-emitter record the controller's `+0x38` points at.  It is shared by every `em` shape, so the
 * fields are the union of the views those shapes take of it.
 * size: 0x60 - a lower bound */
struct EmEffectUnit {
    /* +0x00 */ u32 unused_0x00;
    /* +0x04 */ u32 emitter_count;
    /* +0x08 */ f32 scale;      /* emitter radius multiplier */
    /* +0x0C */ f32 scale_rate; /* eased towards 1 while the effect is on the ground */
    /* +0x10 */ _EmHandle* handles[1]; /* `emitter_count` entries, then the tables below */
    /* +0x14 */ u8 pad_0x14[0x2C];
    /* +0x40 */ _EmHandle* self;
    /* +0x44 */ _GXColor color;
    /* +0x48 */ nw4r::math::VEC3 world_pos;
    /* +0x54 */ u8 part_flag;
    /* +0x55 */ u8 on_ground;
    /* +0x56 */ u8 pad_0x56[2];
    /* +0x58 */ EmEffectEntry* entries;
    /* +0x5C */ EmEffectProbe* probes;
};

/* The effect controller the unit's functions are handed.  It is allocated elsewhere with
 * `eft_res_slot_get(0x2C)` and holds the effect's state machine, its owner and two hooks.
 * size: 0x48 */
struct EmEffectWork {
    /* +0x00 */ u8 unused_0x00;
    /* +0x01 */ u8 active;
    /* +0x02 */ u8 type;
    /* +0x03 */ u8 kind;
    /* +0x04 */ u8 unused_0x04;
    /* +0x05 */ u8 frame;
    /* +0x06 */ u8 state;
    /* +0x07 */ u8 last_emitting;
    /* +0x08 */ u8 emitting;
    /* +0x09 */ u8 pad_0x09[3];
    /* +0x0C */ s32 timer;
    /* +0x10 */ u8 pad_0x10[0x20];
    /* +0x30 */ void* owner;
    /* +0x34 */ void (*advance)(EmEffectWork*);
    /* +0x38 */ EmEffectUnit* unit;
    /* +0x3C */ u8 pad_0x3C[4];
    /* +0x40 */ void (*retire)(EmEffectWork*);
    /* +0x44 */ u8 area;
};

/* --- callees ------------------------------------------------------------------------------------ */
extern "C" f32 vec3_length_sq(const nw4r::math::VEC3* work);
extern "C" void subVec3(nw4r::math::VEC3* out, const nw4r::math::VEC3* a, const nw4r::math::VEC3* b);
extern "C" f32 vec3_len(const nw4r::math::VEC3* in);
extern "C" f32 vec3_dot(const nw4r::math::VEC3* a, const nw4r::math::VEC3* b);
extern "C" void vec3_normalize_into(nw4r::math::VEC3* v, const nw4r::math::VEC3* in);
extern "C" void vec3_scale(nw4r::math::VEC3* out, const nw4r::math::VEC3* in, f32 scale);
extern "C" void addVec3(nw4r::math::VEC3* out, const nw4r::math::VEC3* a, const nw4r::math::VEC3* b);
extern "C" void fn_800513F0(nw4r::math::VEC3* v, f32 angle);
extern "C" void addVec3To(nw4r::math::VEC3* out, const nw4r::math::VEC3* in);
extern "C" void fn_80075258(s32* model, nw4r::math::VEC3* out, const nw4r::math::VEC3* pos);
extern "C" void fn_800FA3E8(EmEffectSegment* seg); /* seg = ((0,0,0), (0,0,0)) */
extern "C" void fn_800FA420(nw4r::math::VEC3* out); /* out = (0, 0, 0) */
extern "C" u8 fn_80290598(const EmEffectQuad* quad, const nw4r::math::VEC3* pos, s32 a, s32 b);
extern "C" s32 stage_water_enabled_ck(void);
extern "C" u32 fn_802B45D4(void);
extern "C" u32 fn_802BE39C(void);
extern "C" u8 get_option_21(void);
/* get_now_areano / get_now_mapno / eftGetKeyAlpha / vec_to_mh_vec3 come from `unsplit/unknown.h` as
 * their real declarations (rule 9).  get_camera_pos / get_camera_direction are NOT converted: their map
 * names are no-argument manglings (`__Fv`) but every call site passes an out pointer, so the only
 * declaration that reproduces the target's codegen is the `extern "C"` map spelling - the real
 * `nw4r::math::VEC3 get_camera_pos()` (struct return, sret) grows the frame 0x1C0 -> 0x1F0 and drops the
 * unit's score, so it is left and reported (rule 9, no real name expressible without a score change).
 * move/setTevKColor are `MHchar` members whose owner (the canonical `pl.h` struct) does not carry them
 * yet - left as the map spelling and reported. */
#include "camera/camera.h" /* get_camera_pos (rule 2: the owner's header) */
/* signatures the calls below use, when they differ from the owner header's: a cast call is the same direct call. */
#define eft_res_models_spawn_c1 ((void (*)(void*, void*, u32, s32, u32))eft_res_models_spawn)
/* The call keeps the out-pointer view (`void (VEC3*)`) the target's frame needs: a cast call to the owner's by-value `get_camera_pos` is the same direct call to the map symbol. */
#define get_camera_pos_c1 ((void (*)(nw4r::math::VEC3*))get_camera_pos)
extern "C" void get_camera_direction__Fv(nw4r::math::VEC3* out);
extern "C" void move__6MHcharFUs(_EmHandle* self, u32 motion);
extern "C" void setTevKColor__6MHcharFUl14_GXTevKColorIDP8_GXColor(_EmHandle* self, u32 index, u32 id,
                                                                  _GXColor* color);

/* --- the unit's own pool, declared, never defined (playbook 29) ---------------------------------- */
extern f32 lbl_807966F0; /* 0.0001f  */
extern f32 lbl_807966F4; /* 0.0f     */
extern f32 lbl_807966F8; /* 50.0f    */
extern f32 lbl_807966FC; /* -100.0f  */
extern f32 lbl_80796700; /* 740.0f   */
extern f32 lbl_80796704; /* 580.0f   */
extern f32 lbl_80796708; /* 2.0f     */
extern f32 lbl_8079670C; /* 640.0f   */
extern f32 lbl_80796710; /* 480.0f   */
extern f32 lbl_80796714; /* 1.0f     */
extern f32 lbl_80796718; /* 0.01f    */
extern f32 lbl_8079671C; /* 128.0f   */
extern f32 lbl_80796720; /* 0.03125f */

extern f32 lbl_80796738; /* 0.05f    */

extern u8 lbl_80791790;  /* the key-alpha table `eftGetKeyAlpha` walks */

/* --- the functions, in address order ------------------------------------------------------------ */
extern "C" void fn_801025FC(EmEffectWork* work, u8 part, nw4r::math::VEC3* pos);
extern "C" void fn_801027D0(EmEffectWork* work);
extern "C" EmEffectQuad* fn_8010294C(EmEffectQuad* quad);

void vec_to_mh_vec3(nw4r::math::VEC3* dst, Vec* src);

#pragma fp_contract off
#pragma peephole off

extern "C" void fn_80101FA4(EmEffectWork* work) {
    nw4r::math::VEC3 v130; /* +0x130, the model's screen position */
    nw4r::math::VEC3 v124; /* +0x124, the camera-relative emitter position */
    nw4r::math::VEC3 v118; /* +0x118 */
    nw4r::math::VEC3 v10C; /* +0x10C */
    nw4r::math::VEC3 v100; /* +0x100 */
    nw4r::math::VEC3 vF4;  /* +0x0F4 */
    nw4r::math::VEC3 vE8;  /* +0x0E8 */
    nw4r::math::VEC3 vDC;  /* +0x0DC */
    nw4r::math::VEC3 vD0;  /* +0x0D0 */
    nw4r::math::VEC3 cam;  /* +0x0C4 */
    nw4r::math::VEC3 vB8;  /* +0x0B8 */
    nw4r::math::VEC3 vAC;  /* +0x0AC */
    nw4r::math::VEC3 vA0;  /* +0x0A0 */
    nw4r::math::VEC3 v94;  /* +0x094 */
    nw4r::math::VEC3 v88;  /* +0x088 */
    nw4r::math::VEC3 v7C;  /* +0x07C */
    nw4r::math::VEC3 v70;  /* +0x070 */
    nw4r::math::VEC3 v64;  /* +0x064 */
    nw4r::math::VEC3 v58;  /* +0x058 */
    nw4r::math::VEC3 v4C;  /* +0x04C */
    nw4r::math::VEC3 v40;  /* +0x040 */
    nw4r::math::VEC3 v34;  /* +0x034 */
    nw4r::math::VEC3 v28;  /* +0x028 */
    nw4r::math::VEC3 v1C;  /* +0x01C */
    nw4r::math::VEC3 v10;  /* +0x010 */
    _GXColor color;        /* +0x00C */
    s32 model;             /* +0x008 */
    EmEffectUnit* unit;
    f32 f31;
    f32 f29;
    s32 alpha;
    s32 intensity;
    u8 part;
    u32 i;
    f32 spread;
    f32 off_screen;
    f32 ease;
    f32 scale;

    VEC3_ctor(&v130);
    VEC3_ctor(&v124);
    VEC3_ctor(&v118);
    VEC3_ctor(&v10C);
    VEC3_ctor(&v100);
    VEC3_ctor(&vF4);
    VEC3_ctor(&vE8);
    VEC3_ctor(&vDC);
    VEC3_ctor(&vD0);
    unit = work->unit;
    if (work->area != get_now_areano()) {
        work->active = 0;
        work->frame++;
        return;
    }
    if (vec3_length_sq(&unit->world_pos) < lbl_807966F0) {
        return;
    }
    if (fn_802B45D4() == 1) {
        return;
    }
    if (stage_water_enabled_ck() != 0) {
        get_camera_pos_c1(&cam);
        copyVec3(&vD0, &cam);
        if (vD0.y < lbl_807966F4) {
            return;
        }
    }
    get_camera_direction__Fv(&vB8);
    copyVec3(&v100, &vB8);
    get_camera_pos_c1(&vA0);
    subVec3(&vAC, &unit->world_pos, &vA0);
    copyVec3(&vF4, &vAC);
    model = scn_root_get_current_camera(pRoot);
    fn_80075258(&model, &v130, &unit->world_pos);
    f29 = vec3_dot(&v100, &vF4);
    vec3_normalize_into(&v100, &v100);
    vec3_normalize_into(&vF4, &vF4);
    get_camera_pos_c1(&v94);
    copyVec3(&v124, &v94);
    vec3_scale(&v88, &vF4, lbl_807966F8);
    copyVec3(&vE8, &v88);
    addVec3To(&v124, &vE8);
    if (!(v130.x <= lbl_807966FC) && !(v130.x >= lbl_80796700) && !(v130.y <= lbl_807966FC)
        && !(v130.y >= lbl_80796704) && !(f29 < lbl_807966F4)) {
        vec3_dot(&vE8, &v100);
        vec3_scale(&v64, &v100, lbl_80796708);
        get_camera_pos_c1(&v70);
        addVec3(&v7C, &v70, &v64);
        copyVec3(&v10C, &v7C);
        subVec3(&v58, &v10C, &v124);
        copyVec3(&vE8, &v58);
        fn_800513F0(&vE8, lbl_80796708);
        addVec3(&v4C, &v124, &vE8);
        copyVec3(&v118, &v4C);
        f31 = (f32)(s32)vec3_len(&vE8);
        off_screen = lbl_807966F4;
        if (v130.x >= lbl_8079670C) {
            off_screen = v130.x - lbl_8079670C;
        } else if (v130.x <= lbl_807966F4) {
            off_screen = -v130.x;
        }
        if (v130.y >= lbl_80796710) {
            off_screen += v130.y - lbl_80796710;
        } else if (v130.y <= lbl_807966F4) {
            off_screen += -v130.y;
        }
        ease = lbl_80796714 - (lbl_80796718 * off_screen);
        if (ease < lbl_807966F4) {
            ease = lbl_807966F4;
        }
        intensity = (s32)(lbl_8079671C * ease);
        spread = lbl_80796720;
        vec3_normalize_into(&vDC, &vE8);
        fn_801027D0(work);
        if (unit->scale_rate > lbl_80796718) {
            for (i = 0; i < unit->emitter_count; i++) {
                unit->handles[i]->offset_x = unit->entries[i].offset_x;
                unit->handles[i]->offset_y = unit->entries[i].offset_y;
                vec3_scale(&v28, &vE8, (f32)(u8)intensity);
                vec3_scale(&v34, &v28, spread);
                addVec3(&v40, &v124, &v34);
                copyVec3(&unit->handles[i]->pos, &v40);
                if (unit->entries[i].follow != 0) {
                    copyVec3(&unit->handles[i]->pos, &unit->handles[0]->pos);
                }
                vec3_scale(&v10, &vDC, unit->entries[i].scale);
                vec3_scale(&v1C, &v10, f31 / unit->scale);
                addVec3To(&unit->handles[i]->pos, &v1C);
                color.r = 0xFF - unit->entries[i].color_r;
                color.g = 0xFF - unit->entries[i].color_g;
                color.b = 0xFF - unit->entries[i].color_b;
                alpha = (u8)intensity - unit->entries[i].alpha_base;
                if (fn_802BE39C() == 1) {
                    alpha /= 2;
                }
                if (alpha <= 0) {
                    color.a = 0;
                } else {
                    color.a = (u8)((f32)alpha * unit->scale_rate);
                }
                setTevKColor__6MHcharFUl14_GXTevKColorIDP8_GXColor(unit->handles[i], 0, 3, &color);
                move__6MHcharFUs(unit->handles[i], 0);
                if (unit->entries[i].follow != 2) {
                    eft_res_models_spawn_c1(work, &unit->handles[i], 2, 1, 0);
                }
            }
        }
        if (unit->on_ground == 1) {
            part = 0;
        } else {
            part = 1;
        }
    } else {
        part = 0;
    }
    if (fn_802BE39C() == 0) {
        work->emitting = 1;
    } else {
        work->emitting = 0;
    }
    fn_801025FC(work, part, &v124);
    work->last_emitting = work->emitting;
}

#pragma fp_contract on
#pragma peephole on

/* Counts the controller's own frames up; the effect family's per-frame tick. */
extern "C" void fn_801025E8(EmEffectWork* work) {
    work->frame++;
}

/* The family's "the effect is over" hook; forwards to the shared emitter teardown. */
extern "C" void fn_801025F8(void* self) {
    eft_res_slot_release(self);
}

#pragma peephole off

extern "C" void fn_801025FC(EmEffectWork* work, u8 part, nw4r::math::VEC3* pos) {
    nw4r::math::VEC3 local;
    EmEffectUnit* unit;

    VEC3_ctor(&local);
    unit = work->unit;
    unit->part_flag = part;
    if (work->emitting != work->last_emitting) {
        work->state = 3;
    }
    if (unit->part_flag == 1 && (work->state == 0 || work->state == 4)) {
        work->state = 1;
    }
    switch (work->state) {
    case 1:
        unit->color.a = 0;
        work->timer = 0;
        work->state++;
        /* fallthrough */
    case 2:
        unit->color.a = eftGetKeyAlpha(&lbl_80791790, work->timer);
        work->timer++;
        if (work->timer > 52) {
            work->state++;
        }
        break;
    case 3:
        unit->color.a = 0;
        if (unit->part_flag == 0) {
            work->state = 4;
            work->timer = 7;
        }
        break;
    case 4:
        work->timer--;
        if (work->timer <= 0) {
            work->state = 0;
        }
        break;
    }
    if ((s32)work->type == 12) {
        unit->color.a = (u8)((u32)unit->color.a >> 1);
    }
    if (get_option_21() != 0) {
        unit->color.a = (u8)((u32)unit->color.a >> 1);
    }
    copyVec3(&unit->self->pos, pos);
    setTevKColor__6MHcharFUl14_GXTevKColorIDP8_GXColor(unit->self, 0, 3, &unit->color);
    move__6MHcharFUs(unit->self, 0);
    eft_res_models_spawn_c1(work, &unit->self, 2, 1, 0);
}

extern "C" void fn_801027D0(EmEffectWork* work) {
    nw4r::math::VEC3 eye;  /* +0x008 */
    EmEffectProbe point;   /* +0x018, the probe entry copied locally before it is tested */
    nw4r::math::VEC3 seg;  /* +0x028, the segment's two ends */
    nw4r::math::VEC3 cam;  /* +0x034 */
    EmEffectQuad quad;     /* +0x040 */
    EmEffectUnit* unit;
    u8 hit;
    u32 i;
    f32 zero;

    unit = work->unit;
    fn_800FA3E8((EmEffectSegment*)&seg);
    fn_8010294C(&quad);
    fn_800FA420(&point.pos);
    if (unit->probes == 0) {
        unit->scale_rate = lbl_80796714;
        return;
    }
    copyVec3(&seg, &unit->world_pos);
    get_camera_pos_c1(&eye);
    copyVec3(&cam, &eye);
    hit = fn_8028F4B4((EmEffectSegment*)&seg, &quad);
    zero = lbl_807966F4;
    for (i = 0; unit->probes[i].scale > zero; i++) {
        vec_to_mh_vec3(&point.pos, (Vec*)&unit->probes[i].pos);
        point.scale = unit->probes[i].scale;
        hit = fn_80290598(&quad, &point.pos, 0, 0);
        if (hit != 0) {
            break;
        }
    }
    if (hit != 0) {
        unit->on_ground = 1;
        if (unit->scale_rate > lbl_807966F4) {
            unit->scale_rate -= lbl_80796738;
        }
        if (unit->scale_rate < lbl_807966F4) {
            unit->scale_rate = lbl_807966F4;
        }
    } else {
        unit->on_ground = 0;
        if (unit->scale_rate < lbl_80796714) {
            unit->scale_rate += lbl_80796738;
        }
        if (unit->scale_rate > lbl_80796714) {
            unit->scale_rate = lbl_80796714;
        }
    }
}

#pragma peephole on

/* Zeroes the four positions of a caller's frame; the effect's own vector initialiser. */
extern "C" EmEffectQuad* fn_8010294C(EmEffectQuad* quad) {
    VEC3_ctor(&quad->v[0]);
    VEC3_ctor(&quad->v[1]);
    VEC3_ctor(&quad->v[2]);
    VEC3_ctor(&quad->v[3]);
    return quad;
}
