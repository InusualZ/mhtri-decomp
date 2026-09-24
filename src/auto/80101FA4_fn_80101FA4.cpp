/* auto/80101FA4_fn_80101FA4.cpp - the `ef` effect-setup batch: three controller families whose
 * per-frame hooks share one object, 22 function(s), 0x80101FA4..0x80103D28.
 *
 * The seam is pinned by the `.data` pool run jumptable_8059E160 -> jumptable_8059E268
 * (`tu-boundary-discovery`).  The unit is C++ (`tools/units/langcheck.py`): two definitions and 21
 * callees arrive mangled, so every definition the map spells plainly is `extern "C"` - a C++ free
 * function would mangle and objdiff pairs by name (docs/matching.md row 42).  A mangled *callee* is a
 * C++ declaration whose signature reproduces the map's argument list; the ones the map spells plainly
 * are `extern "C"`.
 *
 * Three families in address order, each with its own work record: the `em` controller
 * (0x80101FA4..0x80102994, `EmEffectWork`/`EmEffectUnit`), the player-weapon `eft007` controller
 * (0x80102994..0x80103960, `_EFT007`/`_EFT007_WORK`/`_PLW`) and the enemy emitter
 * (0x80103960..0x80103D28, `_EFT_EMITTER`/`_ENEMY_WORK`).  `fn_800F8788` hands each family one of the
 * 0x48-byte effect pool slots, so the shared prototype returns `void*` and each call site names its view.
 *
 * Load-bearing source shapes (each measured, not stylistic):
 *   - `#pragma peephole off` for `fn_80101FA4`, `fn_801025FC`, `fn_801027D0`, the whole `eft007` block
 *     and `fn_801039B0`/`fn_80103B60`: retail keeps the unfused `subi`+`cmpwi`, `clrlwi`+`cmpwi`,
 *     `extsb`+`cmpwi` and `rlwinm`+`cmpwi` forms our `-O3` peephole fuses into their record forms
 *     (row 39).  The whole target object has zero fused record forms.
 *   - `#pragma fp_contract off` for `fn_80101FA4`: retail keeps `fmuls` + `fsubs` where the default
 *     contracts them into `fnmsubs` (row 40).
 *   - `EmEffectUnit::handles` is an array member, not a pointer, and `fn_80101FA4` writes
 *     `unit->handles[i]->field` inline rather than through a local: retail reloads both the handle and
 *     `unit->entries[i]` at every access, and hoisting either into a local costs ~2.5 points.
 *   - `fn_80101FA4`'s screen-box test and its off-screen accumulator use the negated comparisons
 *     (`!(x <= lo)`, `!(x >= hi)`, `x >= 640.0f`, `x <= 0.0f`): retail branches on the precise
 *     `fcmpo`+`cror` pair only the `<=`/`>=` spellings produce.
 *   - The `eft007` setters take the part index as `u8` while the enemy wrappers take it as `u32`; the
 *     `u32`->`u8` narrowing at the call *is* retail's `clrlwi`.
 *
 * Residuals (none of them source-reachable):
 *   - `fn_80101FA4` 98.12 %: one `lfd` of the int->f64 conversion magic names an anonymous pool entry
 *     where the split names `lbl_80796728` (dtk's reloc naming), plus register colouring.
 *   - `fn_801027D0` 97.79 %: retail's frame puts `eye` at +0x08, the probe copy at +0x18, `seg` at
 *     +0x28, `cam` at +0x34 and the quad at +0x40 with a 4-byte hole at +0x14; MWCC allocates the same
 *     five locals in the reverse order and the order is not source-reachable (reordering the
 *     declarations changes nothing).
 *   - `fn_80103518` 98.38 %: instruction-identical (105/105 rows ignoring register numbers); only the
 *     callee-saved colours differ - the allocator's web priority (row 22).
 *   - The three switch functions emit their own `.data` jump tables, so their `bctr` relocs name local
 *     labels where the split names `jumptable_8059DB00/DB50/DBA0`.  Those tables are not claimed.
 *   - `_EmHandle` here and `MHchar` in the `eft007` block are two views of one map name whose fields
 *     disagree (+0x04 is a position here, a byte and a `u16` there); they are kept apart rather than
 *     forced into one layout.
 */
#include "types.h"

#include "nw4r/math.h"

/* --- callees and pool constants the whole unit shares ------------------------------------------- */

extern "C" void fn_80043EA8(nw4r::math::VEC3* out); /* out = (0, 0, 0) */
extern "C" void fn_800F93D8(void* self, void* list, u32 mode, s32 count, u32 arg);
extern "C" void fn_800F886C(void* self);

/* `fn_800F8788` hands out one of the 0x48-byte effect pool slots; the three families each view it as
 * their own record, so the shared prototype returns `void*` and each call site names its view. */
extern "C" void* fn_800F8788(u32 pool_id);
extern "C" void fn_800F9DF4(void* self, u8 a, u8 b);
struct _CP_VECTOR;
extern "C" void fn_800FC0D4(_CP_VECTOR* dst, const _CP_VECTOR* src);

/* ===================================================================================================
 * BLOCK A - the `em` (monster/effect) controller family, 0x80101FA4..0x80102994
 * =================================================================================================== */

/* --- types -------------------------------------------------------------------------------------- */

/* The GX colour record (`_GXColor`), four bytes in r/g/b/a order.
 * size: 0x4 */
struct _GXColor {
    /* +0x0 */ u8 r;
    /* +0x1 */ u8 g;
    /* +0x2 */ u8 b;
    /* +0x3 */ u8 a;
};

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
 * `fn_800F8788(0x2C)` and holds the effect's state machine, its owner and two hooks.
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

extern "C" void fn_80043EA8(nw4r::math::VEC3* out); /* out = (0, 0, 0) */
extern "C" void* fn_80041E40(nw4r::math::VEC3* out, const nw4r::math::VEC3* in);
extern "C" f32 fn_80050EDC(const nw4r::math::VEC3* work);
extern "C" void fn_80050CA0(nw4r::math::VEC3* out, const nw4r::math::VEC3* a, const nw4r::math::VEC3* b);
extern "C" f32 fn_80050F24(const nw4r::math::VEC3* in);
extern "C" f32 fn_80052214(const nw4r::math::VEC3* a, const nw4r::math::VEC3* b);
extern "C" void fn_80050850(nw4r::math::VEC3* v, const nw4r::math::VEC3* in);
extern "C" void fn_80051EE0(nw4r::math::VEC3* out, const nw4r::math::VEC3* in, f32 scale);
extern "C" void fn_80051378(nw4r::math::VEC3* out, const nw4r::math::VEC3* a, const nw4r::math::VEC3* b);
extern "C" void fn_800513F0(nw4r::math::VEC3* v, f32 angle);
extern "C" void fn_80073F68(nw4r::math::VEC3* out, const nw4r::math::VEC3* in);
extern "C" void fn_80075258(s32* model, nw4r::math::VEC3* out, const nw4r::math::VEC3* pos);
extern "C" s32 fn_80082BCC(s32 model);
extern "C" void fn_800FA3E8(EmEffectSegment* seg); /* seg = ((0,0,0), (0,0,0)) */
extern "C" void fn_800FA420(nw4r::math::VEC3* out); /* out = (0, 0, 0) */
extern "C" u8 fn_8028F4B4(const EmEffectSegment* seg, const EmEffectQuad* quad);
extern "C" u8 fn_80290598(const EmEffectQuad* quad, const nw4r::math::VEC3* pos, s32 a, s32 b);
extern "C" s32 fn_802AFF38(void);
extern "C" u32 fn_802B45D4(void);
extern "C" u32 fn_802BE39C(void);
extern "C" u8 fn_803BEE04(void);
extern "C" u8 get_now_areano__Fv(void);
extern "C" void get_camera_pos__Fv(nw4r::math::VEC3* out);
extern "C" void get_camera_direction__Fv(nw4r::math::VEC3* out);
extern "C" void move__6MHcharFUs(_EmHandle* self, u32 motion);
extern "C" void setTevKColor__6MHcharFUl14_GXTevKColorIDP8_GXColor(_EmHandle* self, u32 index, u32 id,
                                                                  _GXColor* color);
extern "C" u8 eftGetKeyAlpha__FPUcl(u8* key, s32 frame);
extern "C" void vec_to_mh_vec3__FPQ34nw4r4math4VEC3P3Vec(nw4r::math::VEC3* out,
                                                          const nw4r::math::VEC3* in);

extern s32 pRoot;

/* --- pooled constants (declared, never defined: the pool belongs to the data pass) --------------- */

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
extern f64 lbl_80796728; /* 4.5036e15, the signed int -> f64 magic */
extern f64 lbl_80796730; /* 4.5036e15, the unsigned int -> f64 magic */
extern f32 lbl_80796738; /* 0.05f    */
extern f32 lbl_80796740; /* 0.0f     */
extern u8 lbl_80791790;  /* the key-alpha table `eftGetKeyAlpha` walks */

/* --- the functions, in address order ------------------------------------------------------------ */

extern "C" void fn_801025FC(EmEffectWork* work, u8 part, nw4r::math::VEC3* pos);
extern "C" void fn_801027D0(EmEffectWork* work);
extern "C" EmEffectQuad* fn_8010294C(EmEffectQuad* quad);

/* Steps the effect's state machine once and drives every emitter handle from the entry table. */
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

    fn_80043EA8(&v130);
    fn_80043EA8(&v124);
    fn_80043EA8(&v118);
    fn_80043EA8(&v10C);
    fn_80043EA8(&v100);
    fn_80043EA8(&vF4);
    fn_80043EA8(&vE8);
    fn_80043EA8(&vDC);
    fn_80043EA8(&vD0);
    unit = work->unit;
    if (work->area != get_now_areano__Fv()) {
        work->active = 0;
        work->frame++;
        return;
    }
    if (fn_80050EDC(&unit->world_pos) < lbl_807966F0) {
        return;
    }
    if (fn_802B45D4() == 1) {
        return;
    }
    if (fn_802AFF38() != 0) {
        get_camera_pos__Fv(&cam);
        fn_80041E40(&vD0, &cam);
        if (vD0.y < lbl_807966F4) {
            return;
        }
    }
    get_camera_direction__Fv(&vB8);
    fn_80041E40(&v100, &vB8);
    get_camera_pos__Fv(&vA0);
    fn_80050CA0(&vAC, &unit->world_pos, &vA0);
    fn_80041E40(&vF4, &vAC);
    model = fn_80082BCC(pRoot);
    fn_80075258(&model, &v130, &unit->world_pos);
    f29 = fn_80052214(&v100, &vF4);
    fn_80050850(&v100, &v100);
    fn_80050850(&vF4, &vF4);
    get_camera_pos__Fv(&v94);
    fn_80041E40(&v124, &v94);
    fn_80051EE0(&v88, &vF4, lbl_807966F8);
    fn_80041E40(&vE8, &v88);
    fn_80073F68(&v124, &vE8);
    if (!(v130.x <= lbl_807966FC) && !(v130.x >= lbl_80796700) && !(v130.y <= lbl_807966FC)
        && !(v130.y >= lbl_80796704) && !(f29 < lbl_807966F4)) {
        fn_80052214(&vE8, &v100);
        fn_80051EE0(&v64, &v100, lbl_80796708);
        get_camera_pos__Fv(&v70);
        fn_80051378(&v7C, &v70, &v64);
        fn_80041E40(&v10C, &v7C);
        fn_80050CA0(&v58, &v10C, &v124);
        fn_80041E40(&vE8, &v58);
        fn_800513F0(&vE8, lbl_80796708);
        fn_80051378(&v4C, &v124, &vE8);
        fn_80041E40(&v118, &v4C);
        f31 = (f32)(s32)fn_80050F24(&vE8);
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
        fn_80050850(&vDC, &vE8);
        fn_801027D0(work);
        if (unit->scale_rate > lbl_80796718) {
            for (i = 0; i < unit->emitter_count; i++) {
                unit->handles[i]->offset_x = unit->entries[i].offset_x;
                unit->handles[i]->offset_y = unit->entries[i].offset_y;
                fn_80051EE0(&v28, &vE8, (f32)(u8)intensity);
                fn_80051EE0(&v34, &v28, spread);
                fn_80051378(&v40, &v124, &v34);
                fn_80041E40(&unit->handles[i]->pos, &v40);
                if (unit->entries[i].follow != 0) {
                    fn_80041E40(&unit->handles[i]->pos, &unit->handles[0]->pos);
                }
                fn_80051EE0(&v10, &vDC, unit->entries[i].scale);
                fn_80051EE0(&v1C, &v10, f31 / unit->scale);
                fn_80073F68(&unit->handles[i]->pos, &v1C);
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
                    fn_800F93D8(work, &unit->handles[i], 2, 1, 0);
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
#pragma peephole on
#pragma fp_contract on

/* Counts the controller's own frames up; the effect family's per-frame tick. */
extern "C" void fn_801025E8(EmEffectWork* work) {
    work->frame++;
}

/* The family's "the effect is over" hook; forwards to the shared emitter teardown. */
extern "C" void fn_801025F8(void* self) {
    fn_800F886C(self);
}

/* Advances the controller's part-fade state machine and pushes the handle's colour and motion. */
#pragma peephole off
extern "C" void fn_801025FC(EmEffectWork* work, u8 part, nw4r::math::VEC3* pos) {
    nw4r::math::VEC3 local;
    EmEffectUnit* unit;

    fn_80043EA8(&local);
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
        unit->color.a = eftGetKeyAlpha__FPUcl(&lbl_80791790, work->timer);
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
    if (fn_803BEE04() != 0) {
        unit->color.a = (u8)((u32)unit->color.a >> 1);
    }
    fn_80041E40(&unit->self->pos, pos);
    setTevKColor__6MHcharFUl14_GXTevKColorIDP8_GXColor(unit->self, 0, 3, &unit->color);
    move__6MHcharFUs(unit->self, 0);
    fn_800F93D8(work, &unit->self, 2, 1, 0);
}
#pragma peephole on

/* Keeps the emitter's radius rate eased towards 1 while the probe table finds ground under it. */
#pragma peephole off
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
    fn_80041E40(&seg, &unit->world_pos);
    get_camera_pos__Fv(&eye);
    fn_80041E40(&cam, &eye);
    hit = fn_8028F4B4((EmEffectSegment*)&seg, &quad);
    zero = lbl_807966F4;
    for (i = 0; unit->probes[i].scale > zero; i++) {
        vec_to_mh_vec3__FPQ34nw4r4math4VEC3P3Vec(&point.pos, &unit->probes[i].pos);
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
    fn_80043EA8(&quad->v[0]);
    fn_80043EA8(&quad->v[1]);
    fn_80043EA8(&quad->v[2]);
    fn_80043EA8(&quad->v[3]);
    return quad;
}

/* ===================================================================================================
 * BLOCK B - the `eft007` (player weapon) controller family, 0x80102994..0x80103960
 * =================================================================================================== */

/* ---------------------------------------------------------------------------------------------------
 * The `eft007` family's types. The two setters take the player work (`_PLW`); every other function of the
 * block takes the effect object `fn_800F8788(44)` hands back, whose +0x38 pool block the family fills.
 * --------------------------------------------------------------------------------------------------- */

/* `nw4r::math::VEC3` and `MTX34` come from `nw4r/math.h` (the shared header - the matrix landed there
 * while this block was being written, so it is not re-declared here). */
namespace nw4r {
namespace ef {
/* The emitter object the pool block holds. Only ever used through pointers here, so the size is the
 * smallest an empty class can be - a lower bound, an approximation. */
struct Effect { /* size: 0x04 - lower bound, an approximation (opaque here) */
    void SetRootMtx(const nw4r::math::MTX34& mtx);
    void RetireEmitterAll();
};
}  // namespace ef
}  // namespace nw4r

/* The engine's three-word rotation vector the setters copy in; `fn_800FC0D4` fills it and
 * `rotLocalMatX/Y` take its x/y as the joint ids. size: 0x0C - Pl/pl_act.cpp's extent. */
struct _CP_VECTOR {
    /* +0x00 */ u32 x;
    /* +0x04 */ u32 y;
    /* +0x08 */ u32 z;
};

struct _MHcharJoints;

/* The character/model an effect hangs off (`MHchar` in the map's mangling). Only the offsets this block
 * reads are named. */
struct MHchar { /* size: 0x140 - lower bound, an approximation (Pl/pl_act.cpp's extent) */
    /* +0x000 */ u8 unused_0x000[0x0A];
    /* +0x00A */ u8 kind_0x0A;     /* == 4 gates the joint frame checks */
    /* +0x00B */ u8 unused_0x00B;
    /* +0x00C */ u16 motion_0x0C;  /* the animation id `fn_801036C0` switches on */
    /* +0x00E */ u8 unused_0x00E[0x16 - 0x0E];
    /* +0x016 */ u8 area_0x16;     /* the stage area the effect belongs to */
    /* +0x017 */ u8 unused_0x017[0x54 - 0x17];
    /* +0x054 */ _CP_VECTOR rot_0x54;
    /* +0x060 */ u8 unused_0x060[0x64 - 0x60];
    /* +0x064 */ f32 field_0x64;
    /* +0x068 */ u8 unused_0x068[0x13C - 0x68];
    /* +0x13C */ _MHcharJoints* joints_0x13C;

    void get_joint_wpos(unsigned long joint, nw4r::math::VEC3* out);
};

/* What `MHchar::joints_0x13C` points at: a 4-byte word and then the joint-bearing character the joint
 * calls are made on. Only ever used through the pointer. size: 0x144 - lower bound, an approximation. */
struct _MHcharJoints {
    /* +0x000 */ u32 unused_0x000;
    /* +0x004 */ MHchar joint_0x004;
};

/* The player work the two setters take (`_PLW` in the map's mangling) - the character plus the
 * player-only tail; only the inherited `area_0x16` is read here. size: 0x668 */
struct _PLW : MHchar {
    /* +0x140 */ u8 unused_0x140[0x668 - 0x140];
};

/* The effect object `fn_800F8788(44)` returns and the two setters fill: the pool block at +0x38, the two
 * handlers at +0x34/+0x40 and the source character at +0x30. */
struct _EFT007;

/* The effect's pool block: a count, the two pooled emitters it covers, the parameter scale/id and the two
 * vectors the per-frame bodies place it by. size: 0x2C */
struct _EFT007_WORK {
    /* +0x00 */ s32 count;
    /* +0x04 */ nw4r::ef::Effect* effects[2];
    /* +0x0C */ f32 paramscale;
    /* +0x10 */ u32 param_id;
    /* +0x14 */ nw4r::math::VEC3 pos_0x14;
    /* +0x20 */ nw4r::math::VEC3 joint_pos_0x20;
};

struct _EFT007 { /* size: 0x48 */
    /* +0x00 */ u8 unused_0x00;
    /* +0x01 */ u8 flag_0x01;
    /* +0x02 */ u8 type_0x02;
    /* +0x03 */ u8 field_0x03;
    /* +0x04 */ u8 unused_0x04;
    /* +0x05 */ u8 state_0x05;
    /* +0x06 */ u8 step_0x06;
    /* +0x07 */ u8 rot_flag_0x07;
    /* +0x08 */ u8 colour_index_0x08;
    /* +0x09 */ u8 unused_0x09[0x0C - 0x09];
    /* +0x0C */ s32 timer_0x0C;
    /* +0x10 */ s32 delay_0x10;
    /* +0x14 */ u8 unused_0x14[0x18 - 0x14];
    /* +0x18 */ nw4r::math::VEC3 pos_0x18;
    /* +0x24 */ _CP_VECTOR rot_0x24;
    /* +0x30 */ _PLW* model_0x30;
    /* +0x34 */ void (*dispatch_0x34)(_EFT007* self);
    /* +0x38 */ _EFT007_WORK* work_0x38;
    /* +0x3C */ u8 unused_0x3C[0x40 - 0x3C];
    /* +0x40 */ void (*release_0x40)(_EFT007* self);
    /* +0x44 */ u8 area_0x44;
    /* +0x45 */ u8 unused_0x45[0x48 - 0x45];
};

/* ---------------------------------------------------------------------------------------------------
 * the callees this block calls. A plain name is what `symbols.txt` spells, so it is `extern "C"`; a
 * mangled one is a C++ declaration whose signature reproduces the map's argument list.
 * ------------------------------------------------------------------------------------------------- */

extern "C" s32 fn_800F92F4(void* self, u32 arg);
extern "C" void fn_800F996C(nw4r::ef::Effect* effect, u32 arg);
extern "C" void fn_800DD7E0(MHchar* model, nw4r::math::VEC3* pos, s32 flag);
extern "C" void fn_800E0A14(void* joints, u32 param_id, nw4r::math::MTX34* mtx);
extern "C" void fn_800FBB90(nw4r::math::MTX34* mtx, nw4r::math::VEC3* vec);
extern "C" void fn_8010140C(nw4r::math::MTX34* mtx, nw4r::math::VEC3* vec);
extern "C" void fn_80101428(nw4r::math::MTX34* mtx, nw4r::math::VEC3* vec);
extern "C" u8 fn_803311A0(MHchar* model);
extern "C" void fn_8005050C(nw4r::math::MTX34* mtx);
extern "C" u8 fn_80331210(_PLW* self);

void push_eft_effect_heap_num(nw4r::ef::Effect** effects, long count);
void setVector3(nw4r::math::VEC3* out, f32 x, f32 y, f32 z);
nw4r::ef::Effect* res_eft_create(u16 id, u16 kind, unsigned long arg);
void change_paramscale_eff(nw4r::ef::Effect* effect, f32 scale);
u32 effect_move(nw4r::ef::Effect* effect);
void change_color_eff(nw4r::ef::Effect* effect, nw4r::math::VEC3* pos, _GXColor color);
void mulVecMat(nw4r::math::VEC3* vec, nw4r::math::MTX34* mtx);
void rotLocalMatX(unsigned long joint, nw4r::math::MTX34* mtx);
void rotLocalMatY(unsigned long joint, nw4r::math::MTX34* mtx);
void rotLocalMatZ(unsigned long joint, nw4r::math::MTX34* mtx);
void cpSetRotMatrix(_CP_VECTOR* rot, nw4r::math::MTX34* mtx);
u32 get_stg_eft_col(u8 area, u8 kind);
u32 Pl_frame_check(_PLW* plw, unsigned long frame, f32 a, f32 b);

/* --- this block's own functions, so the setters can name their two handlers before their bodies --- */
extern "C" void fn_80102CD0(_EFT007* self);
extern "C" void fn_80102D0C(_EFT007* self);
extern "C" void fn_80102D48(_EFT007* self);
extern "C" void fn_80103130(_EFT007* self);
extern "C" void fn_80103518(_EFT007* self);
extern "C" void fn_801036BC(_EFT007* self);
extern "C" s32 fn_801036C0(_EFT007* self);
extern "C" void fn_8010383C(_EFT007* self, nw4r::math::MTX34* mtx);

/* --- the unit's own `.data`/`.sdata2` pool, referenced but never emitted here (playbook 29) --- */
extern "C" u16 lbl_8059DAA0[];
extern "C" u16 lbl_8059DAC8[];
extern "C" u8 lbl_8059DAF0[];
extern f32 lbl_80796744;
extern f32 lbl_80796748;
extern f32 lbl_8079674C;

/* Retail was built with the peephole pass off for this block: with it on MWCC folds the paired-single
 * epilogue's `li r0,<slot>; psq_lx fN,r1,r0` into `psq_l fN,<slot>(r1)`, turns a materialised bool's
 * `cmpwi` into the record form `srwi.`, fuses a switch operand's `clrlwi` away and drops the `clrlwi`
 * that follows a narrowing compound assignment. Measured with the pair commented out, the pass is what
 * costs eft007_set (93.47 -> 100), eft007_set_vec (94.34 -> 100), fn_80102D48 (98.32 -> 100),
 * fn_80103130 (97.08 -> 100) and fn_80103518 (96.86 -> 98.38); `fn_80102CD0`/`fn_80102D0C`/
 * `fn_801036BC`/`fn_801036C0`/`fn_8010383C` measure the same either way; BLOCK C keeps the pass on.
 *
 * Residual: `fn_80103518` (98.38 %) is instruction-identical to the target - 105 of 105 rows, ignoring
 * register numbers - and differs only in which callee-saved registers the allocator hands to `self`
 * (retail r30, ours r31), `work` (retail r31, ours r29) and the two loop pointers that walk `work`.
 * Declaring the locals in the other orders and loading `work` on either side of the two setup calls
 * were both measured and change nothing, so it is the allocator's web priority, not a source shape
 * (playbook 22).
 *
 * Load-bearing source shapes (each measured): `type += 1`, not `type++`, keeps the charge in the `u8`
 * parameter's own register where retail has `addi r0,r29,1`; `fn_801036C0`'s motion switch needs the
 * explicit `case 53` or MWCC's decision tree splits one value lower and every later row shifts;
 * `fn_80102D48`'s case 9 and case 10 each carry their own copy of the placement body (retail has both;
 * one shared copy is 10 instructions short); `fn_80103130`'s colour block is an `if`/`else` whose else
 * holds the switch - written the other way round MWCC lays the two blocks out mirrored. */
#pragma peephole off

/* Spawns the `eft007` effect for the player's weapon: builds the object, seeds its pool block with the
 * id, the scale and the placement vector, maps the weapon type onto the effect's type/colour pair and
 * installs the two handlers. */
void eft007_set(_PLW* self, u8 type, u8 colour, unsigned long id, nw4r::math::VEC3* vec, f32 scale)
{
    if (self->area_0x16 != (u8)get_now_areano__Fv()) {
        return;
    }
    _EFT007* effect = (_EFT007*)fn_800F8788(44);
    if (effect == 0) {
        return;
    }
    _EFT007_WORK* work = effect->work_0x38;
    work->count = 1;
    work->param_id = id;
    work->paramscale = scale;
    if (vec != 0) {
        fn_80041E40(&work->pos_0x14, vec);
    } else {
        setVector3(&work->pos_0x14, lbl_80796740, lbl_80796740, lbl_80796740);
    }
    u8 motion = fn_80331210(self);
    switch (type) {
    case 3:
    case 4:
    case 6:
    case 9:
    case 16:
        switch (motion) {
        case 0:
        case 1:
        case 2:
            colour = motion;
            break;
        case 3:
            if (type != 3) {
                type += 1;
            } else {
                colour = motion;
            }
            break;
        }
        break;
    }
    effect->model_0x30 = self;
    effect->type_0x02 = type;
    effect->field_0x03 = 2;
    effect->timer_0x0C = 0;
    effect->flag_0x01 = 1;
    effect->area_0x44 = self->area_0x16;
    effect->colour_index_0x08 = colour;
    fn_800F9DF4(effect, 0, 0);
    effect->release_0x40 = fn_80102CD0;
    effect->dispatch_0x34 = fn_80102D0C;
}

/* The same spawn for a caller-supplied rotation vector: the vector is copied into the effect's rotation
 * slot and the rotation flag is raised, so the per-frame body applies the two local rotations. */
void eft007_set_vec(_PLW* self, u8 type, u8 colour, unsigned long id, nw4r::math::VEC3* vec, f32 scale,
                    _CP_VECTOR* rot)
{
    if (self->area_0x16 != (u8)get_now_areano__Fv()) {
        return;
    }
    _EFT007* effect = (_EFT007*)fn_800F8788(44);
    if (effect == 0) {
        return;
    }
    _EFT007_WORK* work = effect->work_0x38;
    work->count = 1;
    work->param_id = id;
    work->paramscale = scale;
    if (vec != 0) {
        fn_80041E40(&work->pos_0x14, vec);
    } else {
        setVector3(&work->pos_0x14, lbl_80796740, lbl_80796740, lbl_80796740);
    }
    u8 motion = fn_80331210(self);
    switch (type) {
    case 3:
    case 4:
    case 6:
    case 9:
    case 16:
        switch (motion) {
        case 0:
        case 1:
        case 2:
            colour = motion;
            break;
        case 3:
            if (type != 3) {
                type += 1;
            } else {
                colour = motion;
            }
            break;
        }
        break;
    }
    effect->model_0x30 = self;
    effect->type_0x02 = type;
    effect->field_0x03 = 2;
    effect->timer_0x0C = 0;
    effect->flag_0x01 = 1;
    fn_800FC0D4(&effect->rot_0x24, rot);
    effect->area_0x44 = self->area_0x16;
    effect->colour_index_0x08 = colour;
    effect->rot_flag_0x07 = 1;
    fn_800F9DF4(effect, 0, 0);
    effect->release_0x40 = fn_80102CD0;
    effect->dispatch_0x34 = fn_80102D0C;
}

/* Releases the effect's pool block: hands every pooled emitter back and clears the count. */
extern "C" void fn_80102CD0(_EFT007* self)
{
    _EFT007_WORK* work = self->work_0x38;
    push_eft_effect_heap_num(work->effects, work->count);
    work->count = 0;
}

/* Runs the effect's `state_0x05` handler - the per-frame step of the animation. */
extern "C" void fn_80102D0C(_EFT007* self)
{
    switch (self->state_0x05) {
    case 0:
        fn_80102D48(self);
        return;
    case 1:
        fn_80103130(self);
        return;
    case 2:
        fn_80103518(self);
        return;
    case 3:
        fn_801036BC(self);
        return;
    }
}

/* Places and advances the first step of the animation family: builds the two pooled emitters from the
 * weapon type's resource pair and runs the per-type placement. Every arm shares one tail (the model's
 * joint position then the second step), so the arms only `break`. */
extern "C" void fn_80102D48(_EFT007* self)
{
    nw4r::math::MTX34 mtx;
    nw4r::math::VEC3 pos;
    _PLW* model = self->model_0x30;
    _EFT007_WORK* work = self->work_0x38;

    fn_8005050C(&mtx);
    fn_80043EA8(&pos);
    self->state_0x05++;
    work->effects[0] =
        res_eft_create(lbl_8059DAA0[self->type_0x02], lbl_8059DAC8[self->type_0x02], 0);
    if (work->effects[0] == 0) {
        fn_801036BC(self);
        return;
    }
    switch (self->type_0x02) {
    case 0:
    case 3:
    case 7:
    case 8:
    case 11:
        break;
    case 1:
    case 2:
    case 12:
    case 13:
    case 14:
    case 15: {
        fn_80041E40(&pos, &work->pos_0x14);
        fn_8010383C(self, &mtx);
        mulVecMat(&pos, &mtx);
        fn_80101428(&mtx, &pos);
        fn_8010140C(&mtx, &self->pos_0x18);
        if (self->rot_flag_0x07 == 1) {
            rotLocalMatY(self->rot_0x24.y, &mtx);
            rotLocalMatX(self->rot_0x24.x, &mtx);
        }
        for (s32 i = 0; i < work->count; i++) {
            work->effects[i]->SetRootMtx(mtx);
            change_paramscale_eff(work->effects[i], work->paramscale);
        }
        break;
    }
    case 4:
        self->timer_0x0C = 7;
        work->effects[1] = res_eft_create(0x632, 0x15, 0);
        if (work->effects[1] == 0) {
            fn_801036BC(self);
            return;
        }
        work->count++;
        break;
    case 5:
        self->timer_0x0C = 7;
        break;
    case 6:
        work->effects[1] = res_eft_create(0x632, 0x15, 0);
        if (work->effects[1] == 0) {
            fn_801036BC(self);
            return;
        }
        work->count++;
        break;
    case 9:
        work->effects[1] = res_eft_create(0x6C5, 0x15, 0);
        if (work->effects[1] == 0) {
            fn_801036BC(self);
            return;
        }
        work->count++;
        if (fn_803311A0(model) >= 2) {
            self->timer_0x0C = 3;
            work->paramscale *= lbl_80796744;
        } else {
            self->timer_0x0C = 12;
        }
        fn_80041E40(&pos, &work->pos_0x14);
        fn_8010383C(self, &mtx);
        mulVecMat(&pos, &mtx);
        fn_80101428(&mtx, &pos);
        fn_8010140C(&mtx, &self->pos_0x18);
        fn_800DD7E0(model, &self->pos_0x18, 0);
        break;
    case 10:
        if (fn_803311A0(model) >= 2) {
            self->timer_0x0C = 3;
            work->paramscale *= lbl_80796744;
        } else {
            self->timer_0x0C = 12;
        }
        fn_80041E40(&pos, &work->pos_0x14);
        fn_8010383C(self, &mtx);
        mulVecMat(&pos, &mtx);
        fn_80101428(&mtx, &pos);
        fn_8010140C(&mtx, &self->pos_0x18);
        fn_800DD7E0(model, &self->pos_0x18, 0);
        break;
    case 16:
    case 17:
        self->timer_0x0C = 0x14;
        break;
    case 18:
        fn_8010383C(self, &mtx);
        fn_80041E40(&pos, &work->pos_0x14);
        mulVecMat(&pos, &mtx);
        fn_80101428(&mtx, &pos);
        fn_8010140C(&mtx, &self->pos_0x18);
        rotLocalMatX(self->rot_0x24.x, &mtx);
        rotLocalMatY(self->rot_0x24.y, &mtx);
        work->effects[0]->SetRootMtx(mtx);
        change_paramscale_eff(work->effects[0], work->paramscale);
        break;
    case 19:
        fn_8010383C(self, &mtx);
        work->effects[0]->SetRootMtx(mtx);
        change_paramscale_eff(work->effects[0], work->paramscale);
        break;
    }
    model->joints_0x13C->joint_0x004.get_joint_wpos(work->param_id, &work->joint_pos_0x20);
    fn_80103130(self);
}

/* Per-frame body of the second step of the animation family: re-places and drives every pooled emitter,
 * recolours them from the weapon type's key table (or the stage's colour), then either hands them back to
 * the pool or, for three of the types, waits for the camera to move past the model. */
extern "C" void fn_80103130(_EFT007* self)
{
    nw4r::math::MTX34 mtx;
    nw4r::math::VEC3 pos;
    /* Retail initialises four vectors here; only `pos` is read back, but the other three
     * `fn_80043EA8` calls print in the object, so the locals have to stay. */
    nw4r::math::VEC3 spare_a;
    nw4r::math::VEC3 spare_b;
    nw4r::math::VEC3 spare_c;
    nw4r::math::VEC3 cam;
    _GXColor color;
    _EFT007_WORK* work = self->work_0x38;
    _PLW* model = self->model_0x30;

    fn_8005050C(&mtx);
    fn_80043EA8(&pos);
    fn_80043EA8(&spare_a);
    fn_80043EA8(&spare_b);
    fn_80043EA8(&spare_c);
    if (fn_800F92F4(self, 0) == 0) {
        self->flag_0x01 = 0;
        self->state_0x05++;
        return;
    }
    if (((u32)(self->type_0x02 - 3) <= 2 || (u32)(self->type_0x02 - 9) <= 1 ||
         (u32)(self->type_0x02 - 16) <= 1) &&
        model->kind_0x0A != 4) {
        self->flag_0x01 = 0;
        self->state_0x05++;
        self->step_0x06 = 2;
        return;
    }
    s32 timer = --self->timer_0x0C;
    u32 do_place;
    switch (self->type_0x02) {
    case 1:
    case 2:
    case 12:
    case 13:
    case 14:
    case 15:
    case 18:
    case 19:
        do_place = 0;
        break;
    case 4:
    case 5:
    case 9:
    case 10:
    case 16:
    case 17:
        if (timer < 0) {
            self->flag_0x01 = 0;
            self->state_0x05++;
            return;
        }
        do_place = 1;
        break;
    case 6:
    case 7:
        if (fn_801036C0(self) == 0) {
            self->flag_0x01 = 0;
            self->state_0x05++;
            self->step_0x06 = 0;
            return;
        }
        do_place = 1;
        break;
    case 0:
    case 3:
    case 8:
    case 11:
    default:
        do_place = 1;
        break;
    }
    if (do_place == 1) {
        fn_80041E40(&pos, &work->pos_0x14);
        fn_8010383C(self, &mtx);
        mulVecMat(&pos, &mtx);
        fn_80101428(&mtx, &pos);
        fn_8010140C(&mtx, &self->pos_0x18);
        if (self->rot_flag_0x07 == 1) {
            rotLocalMatY(self->rot_0x24.y, &mtx);
            rotLocalMatX(self->rot_0x24.x, &mtx);
        }
        for (s32 i = 0; i < work->count; i++) {
            work->effects[i]->SetRootMtx(mtx);
            change_paramscale_eff(work->effects[i], work->paramscale);
        }
    }
    for (s32 i = 0; i < work->count; i++) {
        if (effect_move(work->effects[i]) == 0) {
            self->flag_0x01 = 0;
            self->state_0x05++;
            return;
        }
    }
    if ((u32)(self->type_0x02 - 13) > 2) {
        switch (self->type_0x02) {
        case 3:
            color.r = lbl_8059DAF0[self->colour_index_0x08 * 4];
            color.g = lbl_8059DAF0[self->colour_index_0x08 * 4 + 1];
            color.b = lbl_8059DAF0[self->colour_index_0x08 * 4 + 2];
            color.a = lbl_8059DAF0[self->colour_index_0x08 * 4 + 3];
            change_color_eff(work->effects[0], &self->pos_0x18, color);
            break;
        case 4:
        case 6:
        case 9:
            color.r = lbl_8059DAF0[self->colour_index_0x08 * 4];
            color.g = lbl_8059DAF0[self->colour_index_0x08 * 4 + 1];
            color.b = lbl_8059DAF0[self->colour_index_0x08 * 4 + 2];
            color.a = lbl_8059DAF0[self->colour_index_0x08 * 4 + 3];
            change_color_eff(work->effects[1], &self->pos_0x18, color);
            break;
        }
    } else {
        u32 col = get_stg_eft_col(self->area_0x44, 1);
        color.r = (col & 0xFF000000) >> 24;
        color.g = (col & 0x00FF0000) >> 16;
        color.b = (col & 0x0000FF00) >> 8;
        color.a = col & 0xFF;
        change_color_eff(work->effects[0], &self->pos_0x18, color);
    }
    if ((u32)(self->type_0x02 - 13) <= 2) {
        get_camera_pos__Fv(&cam);
        if (cam.y < model->field_0x64) {
            fn_800F93D8(self, work->effects, 1, work->count, 0);
        }
    } else {
        fn_800F93D8(self, work->effects, 1, work->count, 0);
    }
}

/* Per-frame body of the third step: a two-stage retire/park state machine over the pooled emitters. */
extern "C" void fn_80103518(_EFT007* self)
{
    nw4r::math::VEC3 vec;
    nw4r::math::MTX34 mtx;
    _EFT007_WORK* work = self->work_0x38;

    fn_80043EA8(&vec);
    fn_8005050C(&mtx);

    if ((u32)(self->type_0x02 - 6) <= 1 || (u32)(self->type_0x02 - 9) <= 1 ||
        (u32)(self->type_0x02 - 16) <= 1) {
        switch (self->step_0x06) {
        case 0: {
            self->flag_0x01 = 1;
            self->step_0x06++;
            self->delay_0x10 = 10;
            for (s32 i = 0; i < work->count; i++) {
                work->effects[i]->RetireEmitterAll();
            }
            /* fallthrough */
        }
        case 1: {
            fn_80041E40(&vec, &work->pos_0x14);
            fn_8010383C(self, &mtx);
            mulVecMat(&vec, &mtx);
            fn_80101428(&mtx, &vec);
            fn_8010140C(&mtx, &self->pos_0x18);
            for (s32 i = 0; i < work->count; i++) {
                work->effects[i]->SetRootMtx(mtx);
                fn_800F996C(work->effects[i], 0);
            }
            if (--self->delay_0x10 < 0) {
                self->state_0x05++;
                return;
            }
            fn_800F93D8(self, work->effects, 1, work->count, 0);
            return;
        }
        default:
            self->state_0x05++;
            break;
        }
    } else {
        self->state_0x05++;
    }
}

/* Tail-destroys an effect object (the emitter pools are released by its `release_0x40` handler first). */
extern "C" void fn_801036BC(_EFT007* self)
{
    fn_800F886C(self);
}

/* Answers whether the model is in one of the motion states that park or retire the effect: 1 when the
 * motion is one of the `Pl_frame_check` families, otherwise the step machine is walked to its end state
 * (which retires the pooled emitters once) and 0 comes back. */
extern "C" s32 fn_801036C0(_EFT007* self)
{
    _PLW* model = self->model_0x30;

    if (model->kind_0x0A == 4) {
        switch (model->motion_0x0C) {
        case 9:
        case 22:
        case 53:
        case 54:
        case 72:
        case 86:
        case 87:
        case 88:
            return Pl_frame_check(model, 2, lbl_80796748, lbl_80796740) == 1;
        case 24:
        case 55:
            if (self->step_0x06 == 0) {
                self->step_0x06++;
                fn_800DD7E0(model, &self->pos_0x18, 1);
            }
            return 1;
        case 10:
        case 56:
            if (Pl_frame_check(model, 2, lbl_8079674C, lbl_80796740) == 1) {
                return 1;
            }
            if (self->step_0x06 == 1) {
                self->step_0x06 = 2;
                fn_800DD7E0(model, &self->pos_0x18, -1);
            }
            return 0;
        }
    }
    if (self->step_0x06 == 1) {
        self->step_0x06 = 2;
        fn_800DD7E0(model, &self->pos_0x18, -1);
    }
    return 0;
}

/* Builds the model's placement matrix for the effect: reads the model's joint matrix, then applies the
 * per-effect-type local rotation set (which is what gives each weapon type its own swing axis). */
extern "C" void fn_8010383C(_EFT007* self, nw4r::math::MTX34* mtx)
{
    nw4r::math::VEC3 pos;
    nw4r::math::VEC3 origin;
    _PLW* model = self->model_0x30;
    _EFT007_WORK* work = self->work_0x38;

    fn_80043EA8(&pos);
    fn_80043EA8(&origin);
    fn_800E0A14(&model->joints_0x13C->joint_0x004, work->param_id, mtx);
    switch (self->type_0x02) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 8:
    case 13:
        rotLocalMatY(0xF82F, mtx);
        rotLocalMatX(0xF99B, mtx);
        rotLocalMatZ(0xFE95, mtx);
        return;
    case 6:
    case 7:
    case 15:
        rotLocalMatY(0xE4FB, mtx);
        rotLocalMatX(0xFFA6, mtx);
        rotLocalMatZ(0xECCE, mtx);
        return;
    case 9:
    case 10:
    case 11:
    case 12:
    case 14:
        rotLocalMatZ(0x8000, mtx);
        return;
    case 16:
    case 17:
    case 18:
        fn_8010140C(mtx, &pos);
        cpSetRotMatrix(&model->rot_0x54, mtx);
        fn_800FBB90(mtx, &pos);
        return;
    }
}

#pragma peephole reset

/* ===================================================================================================
 * BLOCK C - the enemy-emitter family, 0x80103960..0x80103D28
 *
 * The emitter is the 0x48-byte pool slot `fn_800F8788(0x48)` hands out (`fn_800F8788`'s stride is 72
 * and it pre-clears `+0x04..+0x09` and `+0x14..+0x18`).  It is the same record size as
 * `auto/800FCED4`'s `_EFT`, but a different pool id, so its `+0x38` work block is this family's joint
 * record instead of `_EFT_WORK`.  `+0x0C` is the countdown `fn_801041BC` decrements, `+0x18` the joint
 * rotation `fn_8010140C` reads, `+0x24` the joint position copied out of the enemy, `+0x34`/`+0x40`
 * the advance/retire hooks `fn_800F886C` calls, and `+0x05` the step `fn_80103CEC` dispatches on.
 * `_EFT_JOINT`'s `+0x0C` matrix and `+0x3C` offset are what `get_joint_wmat_em` / `vec_to_mh_vec3`
 * write, which is what fixes that layout.
 *
 * All six functions match their target bytes (100 %), and the block is compiled with
 * `#pragma peephole off` around the two setters: retail keeps the unfused `rlwinm` + `cmpwi` form of
 * the `joint_flags & 6` test, which `-O3`'s peephole fuses into `rlwinm.` (playbook 39).  With the
 * peephole off, three load-bearing source shapes remain:
 *   - the two wrappers (`fn_80103960`/`fn_80103968`) take `part` as `u32` while the setter
 *     `fn_80103B60` takes it as `u8`: the `u32 -> u8` narrowing at the call is the `clrlwi r4,r4,24`
 *     retail has before the `b`/`bl`;
 *   - inside the setters `part` is `u8`, so `emitter->type = part` stays a raw `stb` (a wider `part`
 *     makes MWCC narrow the store with an extra `clrlwi`);
 *   - the tables are indexed `part & 0xFF` and the guards compare `(u32)part` / `(s32)emitter->type`:
 *     those spellings are what make MWCC emit the `clrlwi` on the index and pick `cmplwi` for the
 *     `== 7` test and `cmpwi` for the `== 0x24` test, exactly as retail does.
 *
 * Types and callees shared with BLOCK A/B (`Vec`, `_CP_VECTOR`, `_ENEMY_WORK`, `nw4r::ef::Effect`,
 * `fn_800F8788`, `fn_800F9DF4`, `fn_800FC0D4`, `vec_to_mh_vec3`, `push_eft_effect_heap_num`) are
 * declared here once - dedupe them when this block is merged with BLOCK A/B.  `nw4r::math::MTX34` is
 * not declared here: it comes from `include/nw4r/math.h`.
 * =================================================================================================== */

namespace nw4r {
namespace ef {
struct Effect;
}  // namespace ef
}  // namespace nw4r

/* The engine's own 3-float vector, which `vec_to_mh_vec3` converts to an `nw4r::math::VEC3`.
 * `include/ef.h` typedefs the same type; it cannot be included from this file because it declares
 * `fn_80043EA8` with C++ linkage while BLOCK A declares it `extern "C"`. */
typedef struct Vec {
    /* +0x00 */ f32 x;
    /* +0x04 */ f32 y;
    /* +0x08 */ f32 z;
} Vec; /* size: 0x0C */

/* The engine's 3-word position/rotation triple that `fn_800FC0D4` copies (`Pl/pl_act.cpp` spells it
 * the same way). */
/* The enemy the emitter hangs off.  Only the bytes this block reads are named: the joint position the
 * emitter copies out of (its y component is added to the caller's height), the area number
 * `get_now_areano()` is compared against, and the joint-variant flags that pick the effect subtype. */
struct _ENEMY_WORK {
    /* +0x000 */ u8 unused_0x000[0x1BC];
    /* +0x1BC */ _CP_VECTOR pos_0x1BC;
    /* +0x1C8 */ u8 unused_0x1C8[0x1E1 - 0x1C8];
    /* +0x1E1 */ u8 area_no;
    /* +0x1E2 */ u8 unused_0x1E2[0x228 - 0x1E2];
    /* +0x228 */ u16 joint_flags; /* bit 1 selects the `0x25` subtype over `0x27` */
    /* +0x22A */ u8 unused_0x22A[0x22C - 0x22A];
};
/* size: 0x22C - lower bound (the enemy record continues past what this block reads). */

struct _EFT_EMITTER;
struct _EFT_JOINT;

typedef void (*EftEmitterHook)(_EFT_EMITTER* self);

/* The work block at `_EFT_EMITTER::joint`: the joint's effect list plus the world matrix and the
 * offset the state machines transform through.  `get_joint_wmat_em` fills the matrix at `+0x0C` and
 * `vec_to_mh_vec3` the offset at `+0x3C`, which is what fixes the stride between them; the count is
 * `lbl_8059DE00[part]`, i.e. 1 or 2. */
struct _EFT_JOINT {
    /* +0x00 */ s32 count;
    /* +0x04 */ nw4r::ef::Effect* effects[2];
    /* +0x0C */ nw4r::math::MTX34 mtx;
    /* +0x3C */ nw4r::math::VEC3 offset;
};
/* size: 0x48 - lower bound (the block `fn_800F8B44` hands back is larger). */

/* The 0x48-byte emitter `fn_800F8788(0x48)` hands out. */
struct _EFT_EMITTER {
    /* +0x00 */ u8 in_use; /* set to 1 by the pool allocator */
    /* +0x01 */ u8 unused_0x01;
    /* +0x02 */ u8 type; /* the joint/motion id, indexes lbl_8059DCF8 */
    /* +0x03 */ u8 eft_id; /* 8 for this family */
    /* +0x04 */ u8 unused_0x04;
    /* +0x05 */ u8 frame; /* the step fn_80103CEC dispatches on */
    /* +0x06 */ u8 unused_0x06;
    /* +0x07 */ u8 unused_0x07;
    /* +0x08 */ u8 unused_0x08[0x0C - 0x08];
    /* +0x0C */ s32 timer_0x0C; /* the countdown fn_801041BC decrements */
    /* +0x10 */ s32 unused_0x10; /* zeroed by fn_80103B60, never read by this family */
    /* +0x14 */ u8 unused_0x14[0x18 - 0x14];
    /* +0x18 */ _CP_VECTOR rot; /* the joint rotation fn_8010140C reads */
    /* +0x24 */ _CP_VECTOR pos; /* the joint position, copied out of the enemy */
    /* +0x30 */ _ENEMY_WORK* owner;
    /* +0x34 */ EftEmitterHook advance; /* fn_80103CEC */
    /* +0x38 */ _EFT_JOINT* joint;
    /* +0x3C */ u8 unused_0x3C[0x40 - 0x3C];
    /* +0x40 */ EftEmitterHook retire; /* fn_80103CB0 */
    /* +0x44 */ u8 area_no;
    /* +0x45 */ u8 unused_0x45[0x48 - 0x45];
};
/* size: 0x48 */

/* Callees: a plain name is what `symbols.txt` spells, so it is declared `extern "C"`; the three
 * mangled ones are declared as C++ functions so the mangling reproduces the map's symbol. */
extern "C" u8 get_now_mapno__Fv(void);
extern "C" void fn_80103CB0(_EFT_EMITTER* self);
extern "C" void fn_80103CEC(_EFT_EMITTER* self);
extern "C" void fn_80103D28(_EFT_EMITTER* self);
extern "C" void fn_801041BC(_EFT_EMITTER* self);
extern "C" void fn_801048A0(_EFT_EMITTER* self);
extern "C" void fn_801048B0(_EFT_EMITTER* self);
extern "C" _EFT_EMITTER* fn_80103B60(_ENEMY_WORK* enemy, u8 part);

void vec_to_mh_vec3(nw4r::math::VEC3* dst, Vec* src);
void get_joint_wmat_em(_ENEMY_WORK* enemy, u32 joint, nw4r::math::MTX34* mtx);
void push_eft_effect_heap_num(nw4r::ef::Effect** effects, long count);

/* The block's pooled data - declared, never defined here (the pools belong to the data pass). */
extern u32 lbl_8059DCF8[]; /* joint/motion id per emitter type */
extern u8 lbl_8059DE00[];  /* effect count per part */
extern Vec lbl_8059DE48[]; /* joint offset per part */

/* Builds the emitter for the enemy's part and hands it back. */
extern "C" void* fn_80103960(_ENEMY_WORK* enemy, u32 part)
{
    return fn_80103B60(enemy, part);
}

/* Builds the part's emitter and, when it exists, copies the emitter's joint offset into the caller's
 * vector. */
extern "C" void fn_80103968(_ENEMY_WORK* enemy, u32 part, nw4r::math::VEC3* offset)
{
    _EFT_EMITTER* emitter = (_EFT_EMITTER*)fn_80103B60(enemy, part);

    if (emitter != NULL) {
        fn_80041E40(&emitter->joint->offset, offset);
    }
}

/* Builds the part's emitter for the enemy's own area, positions it from the caller's coordinates and
 * the enemy's joint, and installs the advance/retire hooks. */
#pragma peephole off
extern "C" void fn_801039B0(_ENEMY_WORK* enemy, u8 part, s32 pos_x, s32 pos_y)
{
    if (enemy->area_no == get_now_areano__Fv()) {
        _EFT_EMITTER* emitter = (_EFT_EMITTER*)fn_800F8788(0x48);

        if (emitter != NULL) {
            _EFT_JOINT* joint = emitter->joint;

            joint->count = lbl_8059DE00[part & 0xFF];
            vec_to_mh_vec3(&joint->offset, &lbl_8059DE48[part & 0xFF]);
            emitter->eft_id = 8;
            emitter->type = part;

            if ((s32)emitter->type == 0x24) {
                switch (get_now_mapno__Fv()) {
                case 4:
                case 15:
                    emitter->type = 0x26;
                    break;
                case 5:
                case 16:
                    if (enemy->joint_flags & 6) {
                        emitter->type = 0x25;
                    } else {
                        emitter->type = 0x27;
                    }
                    break;
                default:
                    if (enemy->joint_flags & 6) {
                        emitter->type = 0x25;
                    }
                    break;
                }
            }

            emitter->owner = enemy;
            emitter->area_no = enemy->area_no;

            if (emitter->type == 2) {
                emitter->timer_0x0C = 4;
            } else {
                emitter->timer_0x0C = 0;
            }

            emitter->pos.x = pos_x;
            emitter->pos.y = pos_y + enemy->pos_0x1BC.y;
            emitter->pos.z = 0;

            fn_800F9DF4(emitter, 0, 0);
            get_joint_wmat_em(enemy, lbl_8059DCF8[emitter->type], &joint->mtx);

            emitter->retire = fn_80103CB0;
            emitter->advance = fn_80103CEC;
        }
    }
}
#pragma peephole on

/* Builds the part's emitter for the enemy's own area, positions it from the enemy's joint, and
 * installs the advance/retire hooks.  Returns NULL when the part is out of area or retired. */
#pragma peephole off
extern "C" _EFT_EMITTER* fn_80103B60(_ENEMY_WORK* enemy, u8 part)
{
    _EFT_EMITTER* emitter;
    _EFT_JOINT* joint;

    if (enemy->area_no != get_now_areano__Fv() || (u32)part == 7) {
        return NULL;
    }

    emitter = (_EFT_EMITTER*)fn_800F8788(0x48);
    if (emitter == NULL) {
        return NULL;
    }

    joint = emitter->joint;
    joint->count = lbl_8059DE00[part & 0xFF];
    vec_to_mh_vec3(&joint->offset, &lbl_8059DE48[part & 0xFF]);
    emitter->eft_id = 8;
    emitter->type = part;
    emitter->owner = enemy;
    emitter->area_no = enemy->area_no;

    if (emitter->type == 2) {
        emitter->timer_0x0C = 4;
    } else {
        emitter->timer_0x0C = 0;
    }

    emitter->unused_0x10 = 0;
    fn_800FC0D4(&emitter->pos, &enemy->pos_0x1BC);

    fn_800F9DF4(emitter, 0, 0);
    get_joint_wmat_em(enemy, lbl_8059DCF8[emitter->type], &joint->mtx);

    emitter->retire = fn_80103CB0;
    emitter->advance = fn_80103CEC;

    return emitter;
}
#pragma peephole on

/* Retires the emitter's joint effects and resets its joint count. */
extern "C" void fn_80103CB0(_EFT_EMITTER* emitter)
{
    _EFT_JOINT* joint = emitter->joint;

    push_eft_effect_heap_num(joint->effects, joint->count);
    joint->count = 0;
}

/* Advances the emitter to the next frame's handler. */
extern "C" void fn_80103CEC(_EFT_EMITTER* emitter)
{
    switch (emitter->frame) {
    case 0:
        fn_80103D28(emitter);
        return;
    case 1:
        fn_801041BC(emitter);
        return;
    case 2:
        fn_801048A0(emitter);
        return;
    case 3:
        fn_801048B0(emitter);
        return;
    }
}
