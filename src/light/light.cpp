/*
 * light/light.cpp - the map light module: the scene light setting's light objects, the point lights the effects
 * register, the two distance-attenuated spot channels and the ten flare models they drive, the per-actor light sets,
 * and the lobby's fixed directional lights.
 *
 * RANGE. `.text` 0x802BF278..0x802C2700 (63 functions); `.data`, `.bss`, `.sdata`, `.sbss`, `.sdata2` emitted here
 *   (byte-identical to the target object); the unit's config/RMHE08/splits.txt block.
 * FLAGS. `cflags_main` (`-O3 -inline noauto -Cpp_exceptions on`), `#pragma peephole off` (retail keeps the unfused
 *   `clrlwi`+`cmpwi` pairs); `light_init` under `#pragma dont_inline on` (retail calls ut::Color's constructor and
 *   destructor out of line).
 * NAMES. Real: `light_init`, `light_move`, `set_amblight`, `make_dir_light2` (manglings in the map). GUESS (from each
 *   body and its callers): every other function and data row of the range (`light_point_*` the ten point-light
 *   records the effects register, `light_spot_*` the two attenuated channels, `light_flare_*` the ten flare models,
 *   `light_em_slot_*` the enemy light-slot table, `light_lobby_*` the lobby lights, `light_demo_*` demo 33's light,
 *   `root_light_setting_get`, `light_static_init`), the record types `LightPoint`, `LightSpot`, `LightFlare`,
 *   `LightPointWork`, and the record views `LightSceneRoot`, `LightPlayerView`, `LightNpcView`.
 * RESIDUALS. light_id_advance: retail materialises the counter's address once (`addi r4,r13,...`) and stores the
 *   wrap through it. light_static_init is retail's `__sinit` (its `.ctors` word 0x8056F384 is not emitted here): the
 *   map spells the inline VEC3 constructors it calls as C names (`setVec3`, `VEC3_ctor`), so a compiler-generated
 *   initializer cannot reproduce its calls; the record constructors after it are plain functions in retail's order.
 *   light_update, light_em_slot_alloc, light_em_slot_find_free, light_point_nearest_get, light_set_default_select,
 *   light_flare_setup: register allocation only. light_init: the light-setting/object registers and one `addi`
 *   scheduled late. light_flare_update: the camera temporary's stack slot. light_point_scale_set: `mode`'s copy
 *   scheduled one instruction early. light_point_param_get: one dead `b` of an empty case.
 * SHAPES. `light_set_get`/`light_setting_get_light_set` keep the out-pointer form (a by-value LightSet measured
 *   worse); a pointer to the light set held across the loop (`pset`) is retail's register shape; `copyVec3(&v,
 *   &get_camera_pos())` passes the returned temporary directly as retail does.
 */

#include "types.h"
#include "gx.h"
#include "nw4r/math.h"
#include "nw4r/ut_Color.h"
#include "light/light.h"
#include "light/light_demo_end.h"
#include "light/light_em_slot_alloc.h"
#include "light/light_em_slot_free.h"
#include "pl.h"
#include "g3d/g3d_light.h"
#include "g3d/g3d_anmscn.h"
#include "g3d/light_obj_is_enable.h"
#include "g3d/light_obj_alpha_disable.h"
#include "g3d/light_setting_get_amb_light_obj_array.h"
#include "g3d/light_setting_get_light_set.h"
#include "g3d/obj_delete_flagged.h"
#include "g3d/g3d_anmclr.h"
#include "g3d/g3d_root_model_bind.h"
#include "sound/mhchar.h"
#include "sound/mhchar_model_create.h"
#include "sound/mhchar_mat_alpha_set.h"
#include "sound/mhchar_mat_color_set_all.h"
#include "lobby/mhchar_construct.h"
#include "ef/fn_800CDB2C.h"
#include "ef/get_eft_res_name.h"
#include "ef/get_move_work_adrs.h"
#include "ef/system_lobby_flag_get.h"
#include "ef/pRoot.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/em018_aim_flag2_ck.h"
#include "ai/ainpc_work_get.h"
#include "Pl/Pl_master_ck.h"
#include "stage/stg_w.h"
#include "stage/get_now_mapno.h"
#include "stage/stage_cell_vec_get.h"
#include "camera/camera.h"
#include "camera/camera_main.h"
#include "camera/camera_angle_x_get.h"
#include "lobby/event_demo_running_ck.h"
#include "lobby/lb_server_sel_trans.h"
#include "draw_shape.h"
#include "draw_shape/mtx34_copy.h"
#include "nw_resource.h"
#include "screen_split_ck.h"
#include "color_rgba_copy.h"
#include "fn_8004CAD8.h"
#include "mh3_pad.h"
#include "Runtime.PPCEABI.H/memset.h"

using namespace nw4r;

/* ------------------------------------------------------------------------------------------------ */
/* types                                                                                             */
/* ------------------------------------------------------------------------------------------------ */

/* One registered point light: its id, position, colour and attenuation (attn = (1 - falloff) / (range *
 * falloff * range), the factor light_point_color_at scales the colour by). size: 0x20 */
struct LightPoint {
    /* A member function makes the record a class for MWCC, so `light_point_assign`'s `*dst = *src` copies it member
     * by member (the vector and the colour as words), as retail does. */
    bool is_active() const { return active != 0; }

    /* +0x00 */ u8 active;
    /* +0x01: alignment padding */
    /* +0x02 */ u16 id;
    /* +0x04 */ math::VEC3 pos;
    /* +0x10 */ GXColor color;
    /* +0x14 */ f32 range;
    /* +0x18 */ f32 falloff;
    /* +0x1C */ f32 attn;
};

/* A flare model following one of the far point lights; `phase` fades it in (1), holds it (2) and fades it out
 * (3). size: 0x170 */
struct LightFlare {
    /* +0x000 */ u8 phase;
    /* +0x001 */ u8 step;
    /* +0x002 */ u8 active;
    /* +0x003 */ u8 pad_0x003;
    /* +0x004 */ u16 point_id;
    /* +0x006 */ u8 visible;
    /* +0x007 */ u8 pad_0x007;
    /* +0x008 */ MHchar chr;
    /* +0x148 */ u8 unused_0x148[0x24];
    /* +0x16C */ u8 alpha;
    /* +0x16D */ u8 pad_0x16D[3];
};

/* One of the two near point lights fed to a distance-attenuated light object; `intensity` ramps up to the point's
 * range (phase 1), holds (2) and ramps down (3). size: 0x2C */
struct LightSpot {
    /* +0x00 */ u8 phase;
    /* +0x01 */ u8 active;
    /* +0x02 */ u8 pad_0x02[2];
    /* +0x04 */ LightPoint point;
    /* +0x24 */ f32 timer;
    /* +0x28 */ f32 intensity;
};

/* The module work (`light_point_work`). size: 0x1024 */
struct LightPointWork {
    /* +0x0000 */ u8 state;
    /* +0x0001 */ u8 state_sub;
    /* +0x0002 */ u8 pad_0x0002[2];
    /* +0x0004 */ LightPoint point[10];
    /* +0x0144 */ u16 point_num;
    /* +0x0146 */ u8 pad_0x0146[2];
    /* +0x0148 */ LightFlare flare[10];
    /* +0x0FA8 */ LightSpot spot[2];
    /* +0x1000 */ u8 em_slot[32];
    /* +0x1020 */ u8 lobby_flag;
    /* +0x1021 */ u8 pad_0x1021[3];
};

/* The scene root's light setting sits at +0x2878 (root_light_setting_get). size: 0x2888 (approximation: only the
 * light setting's offset is traced) */
struct LightSceneRoot {
    /* +0x0000 */ u8 unused_0x0000[0x2878];
    /* +0x2878 */ g3d::LightSetting light_setting;
};

/* ------------------------------------------------------------------------------------------------ */
/* data                                                                                              */
/* ------------------------------------------------------------------------------------------------ */

extern LightPointWork light_point_work;
extern math::VEC3 light_dir_center;
extern math::VEC3 light_lobby_dir[4];

/* The flare, spot-fade and demo-light tunables (.sdata). */
f32 light_flare_dist = 350.0f;
u16 light_flare_angle = 0x3000;
u8 light_spot_fade_in = 5;
u8 light_spot_fade_out = 10;
f32 light_flare_pull = 0.9f;
u8 light_flare_alpha_max = 40;
f32 light_flare_scale = 2.0f;
u8 light_flare_fade_in = 5;
u8 light_flare_fade_out = 10;
f32 light_demo_falloff = 0.99f;
f32 light_demo_range = 200.0f;

/* The point-light id counter, the demo light's id and the demo's ambient colour (.sbss). */
u16 light_id_counter;
u16 light_demo_id;
GXColor light_demo_amb;

extern s32 light_player_src_index[4];
extern s32 light_enemy_src_index[4];
extern GXColor light_lobby_color[4];
extern f32 light_demo_point_table[24];

/* ------------------------------------------------------------------------------------------------ */
/* prototypes                                                                                        */
/* ------------------------------------------------------------------------------------------------ */

extern "C" {
void light_id_reset(void);
GXColor* light_color_assign(GXColor* self, const ut::Color& src);
g3d::LightSetting* root_light_setting_get(LightSceneRoot* root);
u32 light_map_lobby_ck(void);
void light_flare_setup(LightPointWork* w);
void light_point_clear(LightPointWork* w);
LightPoint* light_point_free_get(LightPointWork* w);
LightPoint* light_point_find(LightPointWork* w, u16 id);
u16 light_id_advance(void);
void light_point_param_get(GXColor* color, f32* range, f32* falloff, u8 mode);
s32 light_point_add(u16 id, math::VEC3* pos, GXColor* color, f32 range, f32 falloff);
LightPoint* light_point_nearest_get(LightPointWork* w, math::VEC3* pos, s32 rank, u8 mode);
u32 light_point_color_at(LightPoint* p, math::VEC3* pos);
void light_obj_point_apply(LightPointWork* w, g3d::LightObj* obj, math::VEC3* pos);
u32 light_color_fade(GXColor* cur, u32 prev);
void light_set_get(g3d::LightSet* out, s32 index);
void light_set_default_select(s32 index);
u8 light_em_slot_find_free(void);
void light_point_all_clear(void);
u8 light_flare_free_get(LightPointWork* w);
u8 light_flare_find(LightPointWork* w, u16 id);
void light_spot_start(LightPointWork* w, LightPoint* p);
void light_point_assign(LightPoint* dst, const LightPoint* src);
void light_spot_release(LightPointWork* w, LightPoint* p);
void light_spot_update(LightPointWork* w, g3d::LightObj* objs);
void light_flare_start(LightPointWork* w, LightPoint* p);
void light_flare_release(LightPointWork* w, LightPoint* p);
void light_flare_update(LightPointWork* w);
void light_update(LightPointWork* w);
void light_obj_pos_set(g3d::LightObj* obj, math::VEC3* pos);
void light_lobby_dir_init(void);
void light_lobby_select(u8 lobby);
void light_lobby_update(LightPointWork* w);
g3d::LightObj* light_obj_area_select(u8 index);
void light_obj_dir_set(g3d::LightObj* obj, math::VEC3* dir);
void light_obj_init(g3d::LightObj* obj, math::VEC3* pos, GXColor color, s32 mode);
void light_obj_color_enable(g3d::LightObj* obj);
void light_obj_point_init(g3d::LightObj* obj, math::VEC3* pos, GXColor color, f32 ref_dist, f32 ref_bright);
void light_dir_set(math::VEC3* dir);
void light_area_setup(void);
s32 light_point_color_get(math::VEC3* pos, GXColor* out);
void light_area_select(void);
void light_area_release(void);
void light_demo_start(u8 demo);
void light_demo_update(u8 demo);
u32 light_demo_color_override(_ENEMY_WORK* em, u8 index, GXColor* color);
s32 light_clamp(s32 value, s32 low, s32 high);
void light_static_init(void);
LightPointWork* light_point_work_ctor(LightPointWork* self);
LightSpot* light_spot_ctor(LightSpot* self);
LightFlare* light_flare_ctor(LightFlare* self);
LightPoint* light_point_ctor(LightPoint* self);
}

#pragma peephole off

/* ------------------------------------------------------------------------------------------------ */
/* setup                                                                                             */
/* ------------------------------------------------------------------------------------------------ */

/* 0x802BF278 (0xC): resets the point-light id counter. */
extern "C" void light_id_reset(void)
{
    light_id_counter = 0;
}

#pragma dont_inline on
/* 0x802BF284 (0x1AC): clears the module work and the scene's light objects and sets the default ambient and
 * directional light. */
void light_init(void)
{
    GXColor color;
    math::VEC3 pos;
    g3d::LightObj* objs;
    LightPointWork* w;
    u32 num;
    u32 i;

    w = &light_point_work;
    color_ctor_white((u32*)&color);
    VEC3_ctor(&pos);
    root_light_setting_get((LightSceneRoot*)pRoot);
    GXColor amb = {0xFF, 0xFF, 0xFF, 0xFF};
    memset(w, 0, sizeof(LightPointWork));
    g3d::LightSetting* setting = root_light_setting_get((LightSceneRoot*)pRoot);
    objs = light_setting_get_light_obj_array(setting);
    w->state_sub = 0;
    w->state = 0;
    num = light_setting_get_num_light_obj(setting);
    for (i = 0; i < num; i++) {
        objs[i].Clear();
    }
    set_amblight(0, amb);
    for (i = 0; i < 32; i++) {
        w->em_slot[i] = 0xFF;
    }
    light_color_assign(&color, ut::Color(0x60, 0x60, 0x60, 0xFF));
    pos.x = 0.0f;
    pos.y = 0.0f;
    pos.z = 0.0f;
    light_obj_init(objs, &pos, color, 0);
    light_point_clear(w);
    light_id_reset();
}
#pragma dont_inline reset

/* 0x802BF430 (0x30): copies a colour and returns the destination. */
extern "C" GXColor* light_color_assign(GXColor* self, const ut::Color& src)
{
    color_rgba_copy((u8*)self, (const u8*)&src);
    return self;
}

/* 0x802BF460 (0x8): returns the scene root's light setting. */
extern "C" g3d::LightSetting* root_light_setting_get(LightSceneRoot* root)
{
    return &root->light_setting;
}

/* 0x802BF468 (0x48): whether the current map is one of the two lobby maps (21, 22). */
extern "C" u32 light_map_lobby_ck(void)
{
    if (get_now_mapno() == 21 || get_now_mapno() == 22) {
        return 1;
    }
    return 0;
}

/* 0x802BF4B0 (0x10C): selects the default light sets and creates the ten flare models from effect resource 0x38. */
extern "C" void light_flare_setup(LightPointWork* w)
{
    u32 file;
    u32 sub;
    ResEntry* entry;
    LightFlare* flare;
    MHchar* chr;
    u8 i;

    res_file_ctor(&file, 0);
    for (i = 0; i < 40; i++) {
        light_set_default_select(i);
    }
    if (light_map_lobby_ck() != 0) {
        light_lobby_dir_init();
        return;
    }
    if (get_now_mapno() == 0xFF) {
        return;
    }
    entry = ckResourceName(get_eft_res_name(0x38));
    if (entry == NULL) {
        return;
    }
    res_file_assign(&file, res_file_ctor(&sub, (u32)entry->data));
    for (i = 0; i < 10; i++) {
        flare = &w->flare[i];
        flare->active = 0;
        flare->visible = 0;
        flare->alpha = 0;
        chr = &flare->chr;
        mhchar_reset(chr);
        mhchar_model_create(chr, &file, 0x44, 0);
        flare->chr.field_0x34 = 0;
        flare->chr.ready = 1;
        chr->setScaleAll(1.5f);
    }
}

/* 0x802BF5BC (0x78): releases the flare models' g3d works. */
void light_work_reset(void)
{
    u8 i;
    LightPointWork* w = &light_point_work;
    LightFlare* flare;

    for (i = 0; i < 10; i++) {
        flare = &w->flare[i];
        if (flare->chr.g3d_0x10C != NULL) {
            push_g3d_wk(flare->chr.g3d_0x10C);
            flare->chr.g3d_0x10C = NULL;
        }
    }
}

/* 0x802BF634 (0x74): clears the ten point lights. */
extern "C" void light_point_clear(LightPointWork* w)
{
    s32 i;

    for (i = 0; i < 10; i++) {
        memset(&w->point[i], 0, sizeof(LightPoint));
        w->point[i].id = 0xFFFF;
    }
    w->point_num = 0;
}

/* 0x802BF6A8 (0x40): returns the first inactive point light, or NULL. */
extern "C" LightPoint* light_point_free_get(LightPointWork* w)
{
    s32 i;

    for (i = 0; i < 10; i++) {
        if (w->point[i].active == 0) {
            return &w->point[i];
        }
    }
    return NULL;
}

/* 0x802BF6E8 (0x100): returns the active point light with id `id`, or NULL. */
extern "C" LightPoint* light_point_find(LightPointWork* w, u16 id)
{
    s32 i;

    for (i = 0; i < 10; i++) {
        if (w->point[i].active != 0 && id == w->point[i].id) {
            return &w->point[i];
        }
    }
    return NULL;
}

/* 0x802BF7E8 (0x2C): advances the point-light id counter, wrapping it past 60000. */
extern "C" u16 light_id_advance(void)
{
    light_id_counter++;
    if (light_id_counter > 60000) {
        light_id_counter = 0;
    }
    return light_id_counter;
}

/* 0x802BF814 (0x4): hands out the next point-light id. */
u16 light_id_next(void)
{
    return light_id_advance();
}

/* 0x802BF818 (0x1E8): the point-light colour, range and falloff of the current map and area. */
extern "C" void light_point_param_get(GXColor* color, f32* range, f32* falloff, u8 mode)
{
    u8 map = get_now_mapno();
    u8 area = get_now_areano();
    f32 scale = 1.0f;

    color->a = 0xFF;
    *range = 50.0f;
    *falloff = 0.99f;
    switch (map) {
    case 1:
    case 12:
        switch (area) {
        case 0:
        case 1:
        case 3:
        case 5:
        case 6:
            scale = 0.5f;
            break;
        case 2:
            scale = 0.7f;
            break;
        }
        break;
    case 2:
    case 13:
        switch (area) {
        case 10:
            scale = 0.7f;
            break;
        case 11:
            break;
        default:
            scale = 0.5f;
            break;
        }
        break;
    case 3:
    case 14:
        if (area == 0) {
            scale = 0.7f;
        }
        break;
    case 4:
    case 15:
        switch (area) {
        case 4:
            color->r = 0xFF;
            color->g = 0xFF;
            color->b = 0xFF;
            *range = 1500.0f;
            *falloff = 0.3f;
            return;
        case 6:
            if (mode == 1) {
                *range = 3500.0f;
                *falloff = 0.3f;
            }
            break;
        default:
            scale = 0.5f;
            break;
        }
        break;
    case 5:
    case 16:
        switch (area) {
        case 2:
            scale = 0.5f;
            break;
        }
        break;
    case 6:
    case 17:
        switch (area) {
        case 1:
        case 2:
            scale = 0.5f;
            break;
        }
        break;
    case 7:
    case 18:
        if (area == 0) {
            scale = 0.5f;
        }
        break;
    case 8:
    case 9:
    case 11:
    case 19:
    case 20:
        scale = 0.5f;
        break;
    }
    color->r = (u8)(s32)(255.0f * scale);
    color->g = (u8)(s32)(128.0f * scale);
    color->b = 0;
}

/* The source light objects each player / companion light and each enemy light copies, the lobby's light colours
 * (three directional, one ambient) and demo 33's point-light path: {start frame, end frame, x, z} rows ending
 * at a non-positive start (.data). */
s32 light_player_src_index[4] = {0, 1, 2, 3};
s32 light_enemy_src_index[4] = {0, 1, 2, 0};
GXColor light_lobby_color[4] = {
    {0x5A, 0x5A, 0x5A, 0xFF},
    {0xFF, 0xFF, 0xFF, 0xFF},
    {0x40, 0x50, 0x50, 0xFF},
    {0x46, 0x46, 0x46, 0xFF},
};
f32 light_demo_point_table[24] = {
    214.0f, 376.0f, -1340.0f, -1170.0f,
    722.0f, 832.0f, -117.0f, -2284.0f,
    832.0f, 938.0f, -1300.0f, -1330.0f,
    938.0f, 1566.0f, -1022.0f, -2266.0f,
    1808.0f, 2182.0f, -1856.0f, -1847.0f,
    -1.0f, 0.0f, 0.0f, 0.0f,
};

/* 0x802BFA00 (0xE0): registers a point light; returns 0 when all ten are in use. */
extern "C" s32 light_point_add(u16 id, math::VEC3* pos, GXColor* color, f32 range, f32 falloff)
{
    LightPointWork* w = &light_point_work;
    LightPoint* p = light_point_free_get(w);

    if (p == NULL) {
        return 0;
    }
    w->point_num++;
    p->active = 1;
    p->id = id;
    copyVec3(&p->pos, pos);
    color_rgba_copy((u8*)&p->color, (u8*)color);
    p->range = range;
    p->falloff = falloff;
    p->attn = (1.0f - falloff) / (range * (falloff * range));
    return 1;
}

/* 0x802BFAE0 (0x8): registers a point light. */
void light_point_set(u16 id, math::VEC3* pos, GXColor* color, f32 range, f32 falloff)
{
    light_point_add(id, pos, color, range, falloff);
}

/* 0x802BFAE8 (0x134): registers a point light with the map's colour scaled by `scale`; refused during a demo
 * unless `mode` is set. */
extern "C" s32 light_point_scale_set(u16 id, math::VEC3* pos, u8 mode, f32 scale)
{
    GXColor color;
    f32 range;
    f32 falloff;

    if (mode == 0 && (u8)event_demo_running_ck() != 0) {
        return 0;
    }
    light_point_param_get(&color, &range, &falloff, mode);
    color.r = (u8)(s32)(color.r * scale);
    color.g = (u8)(s32)(color.g * scale);
    color.b = (u8)(s32)(color.b * scale);
    return light_point_add(id, pos, &color, range, falloff);
}

/* 0x802BFC1C (0x2A4): returns the `rank`-th nearest active point light to `pos` (lights behind the camera and
 * beyond the flare distance sort last when `mode` is 1), or NULL. */
extern "C" LightPoint* light_point_nearest_get(LightPointWork* w, math::VEC3* pos, s32 rank, u8 mode)
{
    u8 behind[10];
    u8 swap;
    s32 j;
    u8* bp;
    s32* ip;
    LightPoint* p;
    f32* dp;
    s32 index[10];
    s32 i;
    s32 k;
    f32 dist[10];

    if (rank + 1 > w->point_num) {
        return NULL;
    }
    for (i = 0; i < 10; i++) {
        index[i] = 0xFF;
        dist[i] = 0.0f;
        behind[i] = 0;
    }
    p = w->point;
    dp = dist;
    ip = index;
    bp = behind;
    for (i = 0; i < 10; i++) {
        if (p->active != 0) {
            f32 d = vec3_dist_sq(pos, &p->pos);
            *dp = d;
            *ip = i;
            if (mode == 1) {
                s32 rot = get_camera_rot_y();
                u16 ang = rot - (u16)calcVecAng2(pos, &p->pos);
                if (ang > light_flare_angle && ang < (u16)-light_flare_angle &&
                    d > light_flare_dist * light_flare_dist) {
                    *bp = 1;
                }
            }
        }
        dp++;
        ip++;
        bp++;
        p++;
    }
    for (j = 0; j < 10; j++) {
        for (k = 9; k > j; k--) {
            if (index[k] == 0xFF) {
                swap = 0;
            } else if (behind[k] == 0 && behind[k - 1] == 1) {
                swap = 1;
                behind[k] = 1;
                behind[k - 1] = 0;
            } else if (behind[k] == 1 && behind[k - 1] == 0) {
                swap = 0;
            } else {
                swap = dist[k] < dist[k - 1];
            }
            if (swap == 1) {
                f32 tf = dist[k];
                dist[k] = dist[k - 1];
                dist[k - 1] = tf;
                s32 ti = index[k];
                index[k] = index[k - 1];
                index[k - 1] = ti;
            }
        }
    }
    if (index[rank] == 0xFF) {
        return NULL;
    }
    return &w->point[index[rank]];
}

/* 0x802BFEC0 (0xD8): the point light's colour attenuated at `pos`. */
extern "C" u32 light_point_color_at(LightPoint* p, math::VEC3* pos)
{
    union {
        GXColor color;
        u32 word;
    } out;
    f32 d = vec3_distance(pos, &p->pos);
    f32 t = p->attn * (d * d);
    f32 s = 1.0f / (1.0f + t);

    out.color.r = p->color.r * s;
    out.color.g = p->color.g * s;
    out.color.b = p->color.b * s;
    out.color.a = p->color.a;
    return out.word;
}

/* 0x802BFF98 (0x9C): lights `obj` with the nearest point light to `pos`, or leaves it cleared. */
extern "C" void light_obj_point_apply(LightPointWork* w, g3d::LightObj* obj, math::VEC3* pos)
{
    LightPoint* p;
    GXColor color;
    u32 at;

    obj->Clear();
    p = light_point_nearest_get(w, pos, 0, 0);
    if (p != NULL) {
        at = light_point_color_at(p, pos);
        color_rgba_copy((u8*)&color, (u8*)&at);
        light_obj_init(obj, &p->pos, color, 1);
    }
}

/* 0x802C0034 (0x1A4): steps the colour word `prev` a fifth of the way towards `cur` (at least one step). */
extern "C" u32 light_color_fade(GXColor* cur, u32 prev)
{
    union {
        GXColor color;
        u32 word;
    } out;
    u32 r;
    u32 g;
    u32 b;
    f32 dr;
    f32 dg;
    f32 db;
    f32 sr;
    f32 sg;
    f32 sb;

    if (prev == 0) {
        return *(u32*)cur;
    }
    r = prev >> 24;
    dr = (((f32)cur->r - (f32)r) / 255.0f) / 5.0f;
    g = (prev >> 16) & 0xFF;
    dg = (((f32)cur->g - (f32)g) / 255.0f) / 5.0f;
    b = (prev >> 8) & 0xFF;
    db = (((f32)cur->b - (f32)b) / 255.0f) / 5.0f;
    if (255.0f * dr > 0.0f && 255.0f * dr < 1.0f) {
        sr = 1.0f;
    } else {
        sr = 255.0f * dr;
    }
    if (255.0f * dg > 0.0f && 255.0f * dg < 1.0f) {
        sg = 1.0f;
    } else {
        sg = 255.0f * dg;
    }
    if (255.0f * db > 0.0f && 255.0f * db < 1.0f) {
        sb = 1.0f;
    } else {
        sb = 255.0f * db;
    }
    out.color.r = sr + r;
    out.color.g = sg + g;
    out.color.b = sb + b;
    return out.word;
}

/* 0x802C01D8 (0x4C): writes the scene light setting's light set `index` into `out`. */
extern "C" void light_set_get(g3d::LightSet* out, s32 index)
{
    light_setting_get_light_set(out, root_light_setting_get((LightSceneRoot*)pRoot), index);
}

/* 0x802C0224 (0x2E0): selects the default light objects of light set `index`. */
extern "C" void light_set_default_select(s32 index)
{
    g3d::LightSet set;
    s32 i;

    light_setting_get_light_set(&set, root_light_setting_get((LightSceneRoot*)pRoot), index);
    set.SelectLightObj(4, -1);
    set.SelectLightObj(5, -1);
    set.SelectLightObj(6, -1);
    set.SelectLightObj(7, -1);
    set.SelectAmbLightObj(0);
    if (index >= 8 && index <= 40) {
        return;
    }
    set.SelectLightObj(0, -1);
    set.SelectLightObj(1, -1);
    set.SelectLightObj(2, -1);
    set.SelectLightObj(3, -1);
    switch (index) {
    case 0:
        if (light_map_lobby_ck() != 0) {
            set.SelectLightObj(0, -1);
            set.SelectLightObj(1, 1);
            set.SelectLightObj(2, 2);
            break;
        }
        set.SelectLightObj(0, 0);
        set.SelectLightObj(1, 1);
        set.SelectLightObj(2, 2);
        break;
    case 1:
        set.SelectLightObj(0, 33);
        set.SelectLightObj(1, 34);
        set.SelectLightObj(2, 35);
        break;
    case 2:
        if (GameMode_ck() == 1) {
            set.SelectLightObj(0, 3);
            set.SelectLightObj(1, 4);
        } else {
            set.SelectLightObj(0, -1);
            set.SelectLightObj(1, -1);
        }
        set.SelectLightObj(2, -1);
        set.SelectAmbLightObj(-1);
        break;
    case 3:
    case 4:
    case 5:
    case 6:
        if (light_map_lobby_ck() != 0) {
            set.SelectLightObj(0, -1);
            set.SelectLightObj(1, 1);
            set.SelectLightObj(2, 2);
            break;
        }
        for (i = 0, index = (index - 3) * 4 + 5; i < 4; i++) {
            set.SelectLightObj(i, index);
            index++;
        }
        break;
    case 7:
        set.SelectLightObj(0, 1);
        set.SelectLightObj(1, 2);
        set.SelectLightObj(2, 3);
        set.SelectLightObj(3, 4);
        break;
    }
}

/* ------------------------------------------------------------------------------------------------ */
/* enemy light slots                                                                                 */
/* ------------------------------------------------------------------------------------------------ */

/* 0x802C0504 (0xCC): the lowest light slot no enemy holds (3 when all are taken). */
extern "C" u8 light_em_slot_find_free(void)
{
    u8 slot = 0;
    LightPointWork* w = &light_point_work;
    u32 n;
    u32 free;
    u8 i;

    for (n = 0; n < 3; n++) {
        free = 1;
        for (i = 0; i < 32; i++) {
            if (slot == w->em_slot[i]) {
                slot++;
                free = 0;
            }
        }
        if (free == 1) {
            return slot;
        }
    }
    return slot;
}

/* 0x802C05D0 (0x13C): selects the enemy's light set and, for a lit enemy, takes a light slot; returns the light set
 * number. */
s32 light_em_slot_alloc(_ENEMY_WORK* em)
{
    s32 base;
    LightPointWork* w = &light_point_work;
    s32 i;
    u8 set_no;
    g3d::LightSet set;
    u8 slot;

    if (em->team == 40) {
        return 7;
    }
    g3d::LightSetting* setting = root_light_setting_get((LightSceneRoot*)pRoot);
    set_no = em->group + 8;
    light_setting_get_light_set(&set, setting, set_no);
    if (em->field_0x1C8 & 1) {
        slot = light_em_slot_find_free();
        if (slot >= 3) {
            return set_no;
        }
        w->em_slot[em->group] = slot;
        base = slot * 3;
        for (i = 0; i < 3; i++) {
            set.SelectLightObj(i, i + base + 21);
        }
    } else {
        for (i = 0; i < 3; i++) {
            set.SelectLightObj(i, i + 30);
        }
    }
    if (em->team == 20) {
        set.SelectLightObj(i, -1);
    } else {
        set.SelectLightObj(i, em->group + 36);
    }
    return set_no;
}

/* 0x802C070C (0x1C): releases the enemy's light slot. */
void light_em_slot_free(_ENEMY_WORK* em)
{
    LightPointWork* w = &light_point_work;

    w->em_slot[em->group] = 0xFF;
}

/* ------------------------------------------------------------------------------------------------ */
/* spot channels and flares                                                                          */
/* ------------------------------------------------------------------------------------------------ */

/* 0x802C0728 (0xAC): stops every flare and spot channel and clears the point lights. */
extern "C" void light_point_all_clear(void)
{
    LightPointWork* w = &light_point_work;
    u8 i;

    for (i = 0; i < 10; i++) {
        w->flare[i].active = 0;
        w->flare[i].phase = 0;
        w->flare[i].step = 0;
    }
    for (i = 0; i < 2; i++) {
        w->spot[i].active = 0;
        w->spot[i].phase = 0;
    }
    light_point_clear(w);
}

/* 0x802C07D4 (0xCC): the first inactive flare's index, or 0xFF. */
extern "C" u8 light_flare_free_get(LightPointWork* w)
{
    u8 i;

    for (i = 0; i < 10; i++) {
        if (w->flare[i].active == 0) {
            return i;
        }
    }
    return 0xFF;
}

/* 0x802C08A0 (0x10C): the index of the active flare following point light `id`, or 0xFF. */
extern "C" u8 light_flare_find(LightPointWork* w, u16 id)
{
    u8 i;

    for (i = 0; i < 10; i++) {
        if (w->flare[i].active != 0 && id == w->flare[i].point_id) {
            return i;
        }
    }
    return 0xFF;
}

/* 0x802C09AC (0x104): starts a spot channel on the point light unless one already follows it. */
extern "C" void light_spot_start(LightPointWork* w, LightPoint* p)
{
    u32 slot = 0xFF;
    u8 i;

    for (i = 0; i < 2; i++) {
        if (w->spot[i].active != 0 && p->id == w->spot[i].point.id) {
            slot = i;
            break;
        }
    }
    if (slot != 0xFF) {
        return;
    }
    for (i = 0; i < 2; i++) {
        if (w->spot[i].active == 0) {
            slot = i;
            break;
        }
    }
    if (slot != 0xFF) {
        w->spot[slot].active = 1;
        w->spot[slot].phase = 1;
        light_point_assign(&w->spot[slot].point, p);
        w->spot[slot].intensity = 0.0f;
    }
}

/* 0x802C0AB0 (0x4C): copies a point light record. */
extern "C" void light_point_assign(LightPoint* dst, const LightPoint* src)
{
    *dst = *src;
}

/* 0x802C0AFC (0x88): lets the spot channel following the point light fade out. */
extern "C" void light_spot_release(LightPointWork* w, LightPoint* p)
{
    u32 slot = 0xFF;
    u8 i;

    for (i = 0; i < 2; i++) {
        if (w->spot[i].active != 0 && p->id == w->spot[i].point.id) {
            slot = i;
            break;
        }
    }
    if (slot == 0xFF) {
        return;
    }
    if (w->spot[slot].phase == 2) {
        w->spot[slot].phase++;
    }
}

/* 0x802C0B84 (0x1D0): fades the two spot channels and feeds them to light objects 3 and 4. */
extern "C" void light_spot_update(LightPointWork* w, g3d::LightObj* objs)
{
    LightSpot* s;
    LightPoint* p;
    u8 i;

    for (i = 0; i < 2; i++) {
        s = &w->spot[i];
        if (s->active == 0) {
            continue;
        }
        p = light_point_find(w, s->point.id);
        switch (s->phase) {
        case 0:
            s->phase++;
            s->timer = 0.0f;
            break;
        case 1:
            s->intensity += s->point.range / light_spot_fade_in;
            if (s->intensity > s->point.range) {
                s->intensity = s->point.range;
                s->phase++;
            }
            break;
        case 2:
            if (p == NULL) {
                s->phase++;
            }
            break;
        case 3:
            s->intensity -= s->point.range / light_spot_fade_out;
            if (s->intensity <= 0.0f) {
                s->intensity = 0.0f;
                s->phase = 0;
                s->active = 0;
                continue;
            }
            break;
        }
        if (p != NULL) {
            copyVec3(&s->point.pos, &p->pos);
            color_rgba_copy((u8*)&s->point.color, (u8*)&p->color);
        }
        light_obj_point_init(&objs[i + 3], &s->point.pos, s->point.color, s->intensity, s->point.falloff);
    }
}

/* 0x802C0D54 (0x78): starts a flare on the point light unless one already follows it. */
extern "C" void light_flare_start(LightPointWork* w, LightPoint* p)
{
    LightFlare* flare;

    if (light_flare_find(w, p->id) == 0xFF) {
        flare = &w->flare[light_flare_free_get(w)];
        flare->phase = 0;
        flare->active = 1;
        flare->chr.field_0x34 = 1;
        flare->point_id = p->id;
    }
}

/* 0x802C0DCC (0x58): lets the flare following the point light fade out. */
extern "C" void light_flare_release(LightPointWork* w, LightPoint* p)
{
    u8 index = light_flare_find(w, p->id);
    LightFlare* flare;

    if (index != 0xFF) {
        flare = &w->flare[index];
        if (flare->phase == 2) {
            flare->phase++;
        }
    }
}

/* 0x802C0E24 (0x298): fades the flares and draws each one between the camera and its point light. */
extern "C" void light_flare_update(LightPointWork* w)
{
    math::MTX34 mtx;
    math::VEC3 ofs;
    math::VEC3 cam;
    GXColor color;
    LightFlare* flare;
    s32 i;
    LightPoint* p;
    MHchar* chr;
    s16 alpha;

    VEC3_ctor(&ofs);
    VEC3_ctor(&cam);
    copyVec3(&cam, &get_camera_pos());
    MTX34_ctor(&mtx);
    for (i = 0, flare = w->flare; i < 10; flare++, i++) {
        if (flare->active == 0) {
            continue;
        }
        p = light_point_find(w, flare->point_id);
        chr = &flare->chr;
    switch (flare->phase) {
        case 0:
            flare->phase++;
            flare->alpha = 0;
            break;
        case 1:
            alpha = flare->alpha + light_flare_alpha_max / light_flare_fade_in;
            if (alpha >= light_flare_alpha_max) {
                flare->alpha = light_flare_alpha_max;
                flare->phase++;
            } else {
                flare->alpha = alpha;
            }
            break;
        case 2:
            if (p == NULL) {
                flare->phase++;
            }
            break;
        case 3:
            alpha = flare->alpha - light_flare_alpha_max / light_flare_fade_out;
            if (alpha < 0) {
                flare->alpha = 0;
                flare->phase = 0;
                flare->active = 0;
                continue;
            }
            flare->alpha = alpha;
            break;
        }
        if (chr->field_0x118 == 0) {
            continue;
        }
        if (p != NULL) {
            math::VEC3 diff;
            math::VEC3 sum;
            math::VEC3 out;

            copyVec3(&chr->pos_0x04, &p->pos);
            subVec3(&diff, &chr->pos_0x04, &cam);
            copyVec3(&ofs, &diff);
            vec3_scale_in_place(&ofs, light_flare_pull);
            addVec3(&sum, &cam, &ofs);
            copyVec3(&ofs, &sum);
            u32 at = light_point_color_at(p, &ofs);
            color_rgba_copy((u8*)&color, (u8*)&at);
            color.a = flare->alpha;
            GXColor arg = color;
            mhchar_mat_color_set_all(chr, GX_COLOR0A0, &arg, 0);
            setVector3(&ofs, 0.0f, 0.0f, 5.0f);
            mtx34_copy(&mtx, &get_current_view_mtx());
            mulVecMat(&ofs, &mtx);
            vec3_scale_in_place(&ofs, -1.0f);
            addVec3(&out, &p->pos, &ofs);
            copyVec3(&chr->pos_0x04, &out);
        } else {
            mhchar_mat_alpha_set(chr, 4, flare->alpha, 0);
        }
        chr->setScaleAll(light_flare_scale);
        chr->move(0);
        g3d_root_model_bind(pRoot, chr->field_0x118);
    }
}

/* ------------------------------------------------------------------------------------------------ */
/* per-frame update                                                                                  */
/* ------------------------------------------------------------------------------------------------ */

/* The player record as the light update reads it (`get_move_work_adrs(2)`, 0xB20-byte stride).
 * size: 0xB20 (approximation: only the fields read here are traced) */
struct LightPlayerView {
    /* +0x000 */ u8 active;
    /* +0x001 */ u8 unused_0x001[0x15];
    /* +0x016 */ u8 area;
    /* +0x017 */ u8 unused_0x017[0x25];
    /* +0x03C */ math::VEC3 pos;
    /* +0x048 */ u8 unused_0x048[0x1C];
    /* +0x064 */ f32 water_y;
    /* +0x068 */ u8 unused_0x068[0x543];
    /* +0x5AB */ u8 cell;
    /* +0x5AC */ u8 unused_0x5AC[0x88];
    /* +0x634 */ u32 light_color[4];
    /* +0x644 */ u8 unused_0x644[0x4DC];
};

/* The AI companion record as the light update reads it (`ainpc_work_get`).
 * size: 0x4A8 (approximation: only the fields read here are traced) */
struct LightNpcView {
    /* +0x000 */ u8 active;
    /* +0x001 */ u8 unused_0x001[0x177];
    /* +0x178 */ math::VEC3 pos;
    /* +0x184 */ u8 unused_0x184[0x1A7];
    /* +0x32B */ u8 cell;
    /* +0x32C */ u8 unused_0x32C[0x15C];
    /* +0x488 */ u32 light_color[4];
    /* +0x498 */ u8 unused_0x498[0x10];
};

/* 0x802C10BC (0xA1C): drives the point lights, spot channels and flares, then the players', the companion's and
 * the enemies' light objects from the stage cells, fading each towards its new colour. */
extern "C" void light_update(LightPointWork* w)
{
    math::VEC3 vec;
    math::VEC3 lpos;
    math::VEC3 cam;
    GXColor color;
    GXColor faded;
    g3d::LightSet set0;
    g3d::LightSet set1;
    g3d::LightObj* objs;
    u32 under0;
    u32 under1;
    u32 water;
    s32 base;
    g3d::LightObj* obj;
    s32 i;
    u16 max;
    s32 j;
    LightPlayerView* pl;
    LightPoint* p;
    u8 me;
    g3d::LightSetting* setting;
    u32 under;
    g3d::LightSet* pset0;
    g3d::LightSet* pset1;
    LightNpcView* npc;
    _ENEMY_WORK* em;
    s32* src;
    u32* word;
    s32 obj_no;
    u8 k;
    u8 c;
    f32 depth;

    get_move_work_adrs(0);
    setting = root_light_setting_get((LightSceneRoot*)pRoot);
    VEC3_ctor(&vec);
    VEC3_ctor(&lpos);
    VEC3_ctor(&cam);
    if (get_now_mapno() != 0xFF) {
        if (light_map_lobby_ck() == 1) {
            light_lobby_update(w);
            return;
        }
        objs = light_setting_get_light_obj_array(setting);
        pl = (LightPlayerView*)get_move_work_adrs(2);
        max = get_move_work_max(2);
        if (PlayMode_ck() == 4) {
            max = 1;
        }
        copyVec3(&cam, &get_camera_pos());
        if (light_map_lobby_ck() == 0) {
            objs[3].Clear();
            objs[4].Clear();
            for (i = 0; i < 10; i++) {
                p = light_point_nearest_get(w, &cam, i, 1);
                if (p != NULL) {
                    if (i < 2) {
                        light_spot_start(w, p);
                        light_flare_release(w, p);
                    } else {
                        light_flare_start(w, p);
                        light_spot_release(w, p);
                    }
                }
            }
            light_spot_update(w, objs);
            light_flare_update(w);
        }
        under0 = 0;
        water = 0;
        under1 = 0;
        if ((u32)stage_water_enabled_ck() == 1) {
            water = 1;
            if (camera_underwater_ck() == 1) {
                under0 = 1;
            }
            if (screen_split_ck() == 1) {
                me = my_player_no();
                my_player_no_set(1);
                if (camera_underwater_ck() == 1) {
                    under1 = 1;
                }
                my_player_no_set(me);
            }
        }
        for (i = 0, base = 5; i < max; i++, base += 4, pl++) {
            if (pl->active == 0 || pl->area != get_now_areano()) {
                continue;
            }
            for (j = 0; j < 4; j++) {
                obj = &objs[base + j];
                *obj = objs[light_player_src_index[j]];
                if (water == 1) {
                    if (screen_split_ck() == 1) {
                        if (j != 0) {
                            if (i == 0 && under0 == 1) {
                                under = 1;
                            } else if (i == 1 && under1 == 1) {
                                under = 1;
                            } else {
                                under = 0;
                            }
                        } else {
                            under = 0;
                        }
                    } else if (under0 == 1) {
                        under = 1;
                    }
                    if (under == 1) {
                        u32 cell = stage_cell_get(pl->cell + 3, j);
                        color_rgba_copy((u8*)&color, (u8*)&cell);
                        math::VEC3 cell_vec;
                        stage_cell_vec_get(&cell_vec, 3, j);
                        copyVec3(&lpos, &cell_vec);
                        depth = pl->water_y - pl->pos.y;
                        if (depth < 300.0f) {
                            color.r = light_clamp(color.r + 64, 0, 255);
                            color.g = light_clamp(color.g + 64, 0, 255);
                            color.b = light_clamp(color.b + 64, 0, 255);
                        } else if (depth < 600.0f) {
                            color.r = light_clamp(color.r + 32, 0, 255);
                            color.g = light_clamp(color.g + 32, 0, 255);
                            color.b = light_clamp(color.b + 32, 0, 255);
                        }
                    } else {
                        u32 cell = stage_cell_get(pl->cell, j);
                        color_rgba_copy((u8*)&color, (u8*)&cell);
                        math::VEC3 cell_vec;
                        stage_cell_vec_get(&cell_vec, 0, j);
                        copyVec3(&lpos, &cell_vec);
                    }
                    if (j != 0) {
                        light_obj_dir_set(obj, &lpos);
                    }
                } else if (pl->cell != 0) {
                    u32 cell = stage_cell_get(pl->cell, j);
                    color_rgba_copy((u8*)&color, (u8*)&cell);
                } else {
                    obj->GetLightColor(&color);
                }
                u32 at = light_color_fade(&color, pl->light_color[j]);
                color_rgba_copy((u8*)&faded, (u8*)&at);
                pl->light_color[j] = (faded.r << 24) | (faded.g << 16) | (faded.b << 8) | 0xFF;
                obj->InitLightColor(faded);
            }
            light_obj_point_apply(w, &objs[base + 3], &pl->pos);
        }
        npc = (LightNpcView*)ainpc_work_get();
        if (npc->active != 0) {
            src = light_player_src_index;
            obj_no = 0;
            word = npc->light_color;
            for (k = 0; k < 4; k++, src++, obj_no++, word++) {
                obj = &objs[17] + obj_no;
                *obj = objs[*src];
                if (water == 1) {
                    if (under0 == 0) {
                        u32 cell = stage_cell_get(npc->cell, k);
                        color_rgba_copy((u8*)&color, (u8*)&cell);
                        math::VEC3 cell_vec;
                        stage_cell_vec_get(&cell_vec, 0, k);
                        copyVec3(&lpos, &cell_vec);
                    } else {
                        u32 cell = stage_cell_get(npc->cell + 3, k);
                        color_rgba_copy((u8*)&color, (u8*)&cell);
                        math::VEC3 cell_vec;
                        stage_cell_vec_get(&cell_vec, 3, k);
                        copyVec3(&lpos, &cell_vec);
                    }
                    if (k != 0) {
                        light_obj_dir_set(obj, &lpos);
                    }
                } else if (npc->cell != 0) {
                    u32 cell = stage_cell_get(npc->cell, k);
                    color_rgba_copy((u8*)&color, (u8*)&cell);
                } else {
                    obj->GetLightColor(&color);
                }
                u32 at = light_color_fade(&color, *word);
                color_rgba_copy((u8*)&faded, (u8*)&at);
                *word = (faded.r << 24) | (faded.g << 16) | (faded.b << 8) | 0xFF;
                obj->InitLightColor(faded);
            }
            light_obj_point_apply(w, &objs[20], &npc->pos);
        }
        light_obj_get_light_pos(&objs[light_enemy_src_index[0]], &lpos);
        light_obj_pos_set(&objs[30], &lpos);
        light_obj_pos_set(&objs[33], &lpos);
        em = (_ENEMY_WORK*)get_move_work_adrs(3);
        max = get_move_work_max(3);
        for (i = 0, obj_no = 0, pset0 = &set0, pset1 = &set1; i < max; i++, obj_no++, em++) {
            if (em->active == 0) {
                continue;
            }
            if (em->field_0x1C8 & 1) {
                c = w->em_slot[em->group];
                if (c == 0xFF) {
                    continue;
                }
                base = c * 3 + 21;
                if (em->team == 18) {
                    light_set_get(&set0, i + 8);
                    obj_delete_flagged(pset0, -1);
                    if (em018_aim_flag2_ck(em) == 1) {
                        for (j = 0; j < 3; j++) {
                            pset0->SelectLightObj(j, -1);
                        }
                        pset0->SelectAmbLightObj(-1);
                    } else {
                        for (j = 0; j < 3; j++) {
                            pset0->SelectLightObj(j, base + j);
                        }
                        pset0->SelectAmbLightObj(0);
                    }
                }
                src = light_enemy_src_index;
                word = &em->field_0x750;
                for (k = 0; k < 3; k++, src++, word++) {
                    obj = &objs[base + k];
                    *obj = objs[*src];
                    c = (em->field_0x760 == 1) ? em->cell_0x22F : 0;
                    if (water == 1) {
                        if (under0 == 1 || under1 == 1) {
                            u32 cell = stage_cell_get(c + 3, k);
                            color_rgba_copy((u8*)&color, (u8*)&cell);
                            math::VEC3 cell_vec;
                            stage_cell_vec_get(&cell_vec, 3, k);
                            copyVec3(&lpos, &cell_vec);
                        } else {
                            u32 cell = stage_cell_get(c, k);
                            color_rgba_copy((u8*)&color, (u8*)&cell);
                            math::VEC3 cell_vec;
                            stage_cell_vec_get(&cell_vec, 0, k);
                            copyVec3(&lpos, &cell_vec);
                        }
                        if (k != 0) {
                            light_obj_dir_set(obj, &lpos);
                        }
                    } else if (c != 0) {
                        u32 cell = stage_cell_get(c, k);
                        color_rgba_copy((u8*)&color, (u8*)&cell);
                    } else {
                        obj->GetLightColor(&color);
                    }
                    u32 at = light_color_fade(&color, *word);
                    color_rgba_copy((u8*)&faded, (u8*)&at);
                    *word = (faded.r << 24) | (faded.g << 16) | (faded.b << 8) | 0xFF;
                    if (light_demo_color_override(em, k, &color) == 1) {
                        color_rgba_copy((u8*)&faded, (u8*)&color);
                    }
                    obj->InitLightColor(faded);
                }
            } else {
                if (em->area_no != get_now_areano()) {
                    continue;
                }
                if (water == 1 || under1 == 1) {
                    light_set_get(&set1, em->group + 8);
                    obj_delete_flagged(pset1, -1);
                    if (under0 == 1) {
                        for (j = 0; j < 3; j++) {
                            pset1->SelectLightObj(j, j + 33);
                        }
                    } else {
                        for (j = 0; j < 3; j++) {
                            pset1->SelectLightObj(j, j + 30);
                        }
                    }
                }
            }
            light_obj_point_apply(w, &objs[36] + obj_no, &em->pos);
        }
    }
    light_point_clear(w);
}

/* ------------------------------------------------------------------------------------------------ */
/* light objects and the lobby                                                                       */
/* ------------------------------------------------------------------------------------------------ */

/* 0x802C1AD8 (0x10): sets the light object's position when one is given. */
extern "C" void light_obj_pos_set(g3d::LightObj* obj, math::VEC3* pos)
{
    if (pos != NULL) {
        light_obj_pos_vec_set(obj, &pos->x);
    }
}

/* 0x802C1AE8 (0x78): sets the lobby's three directional lights (light objects 21-23). */
extern "C" void light_lobby_dir_init(void)
{
    GXColor* color;
    math::VEC3* dir;
    s32 i;

    for (i = 0, color = light_lobby_color, dir = light_lobby_dir; i < 3; color++, dir++, i++) {
        make_dir_light2(i + 21, dir, *color, 0);
    }
}

/* 0x802C1B60 (0x16C): points the master players' light sets at the lobby lights (`lobby` 0) or the default ones,
 * and sets the matching ambient colour. */
extern "C" void light_lobby_select(u8 lobby)
{
    g3d::LightSet set;
    s32 i;
    u16 max;
    LightPlayerView* pl;
    g3d::LightSet* pset;

    light_setting_get_light_obj_array(root_light_setting_get((LightSceneRoot*)pRoot));
    pl = (LightPlayerView*)get_move_work_adrs(2);
    max = get_move_work_max(2);
    if (PlayMode_ck() == 4) {
        max = 1;
    }
    for (i = 0, pset = &set; i < max; i++, pl++) {
        if (pl->active != 0 && Pl_master_ck((_PLW*)pl) != 0 && pl->area == get_now_areano()) {
            light_set_get(&set, i + 3);
            obj_delete_flagged(pset, -1);
            if (lobby == 0) {
                pset->SelectLightObj(0, 21);
                pset->SelectLightObj(1, 22);
                pset->SelectLightObj(2, 23);
            } else {
                pset->SelectLightObj(0, -1);
                pset->SelectLightObj(1, 1);
                pset->SelectLightObj(2, 2);
            }
        }
    }
    if (lobby == 0) {
        set_amblight(0, light_lobby_color[3]);
    } else {
        stage_area_ambient_apply();
    }
}

/* 0x802C1CCC (0x64): switches the lobby lights when the lobby flag changes. */
extern "C" void light_lobby_update(LightPointWork* w)
{
    u8 prev = w->lobby_flag;
    u8 flag = system_lobby_flag_get();

    w->lobby_flag = flag;
    if (prev != flag) {
        if (flag != 0) {
            light_lobby_select(0);
        } else {
            light_lobby_select(1);
        }
    }
}

/* 0x802C1D30 (0x38): runs the flare setup once, then the per-frame update. */
void light_move(void)
{
    LightPointWork* w = &light_point_work;

    switch (w->state) {
    case 0:
        w->state++;
        light_flare_setup(w);
        break;
    case 1:
        light_update(w);
        break;
    }
}

/* 0x802C1D68 (0x40): returns the scene's light object `index`. */
extern "C" g3d::LightObj* light_obj_area_select(u8 index)
{
    return &light_setting_get_light_obj_array(root_light_setting_get((LightSceneRoot*)pRoot))[index];
}

/* 0x802C1DA8 (0x6C): sets ambient light `id` to `color`. */
void set_amblight(u8 id, _GXColor color)
{
    g3d::AmbLightObj* amb = light_setting_get_amb_light_obj_array(root_light_setting_get((LightSceneRoot*)pRoot));

    amb[id].r = color.r;
    amb[id].g = color.g;
    amb[id].b = color.b;
    amb[id].a = color.a;
}

/* 0x802C1E14 (0x8C): places the light object far along the direction `dir` (relative to the light centre). */
extern "C" void light_obj_dir_set(g3d::LightObj* obj, math::VEC3* dir)
{
    math::VEC3 pos;
    math::VEC3 norm;
    math::VEC3 far;
    math::VEC3 diff;

    VEC3_ctor(&pos);
    VEC3_ctor(&norm);
    vec3_normalize_into(&norm, dir);
    subVec3(&diff, &norm, &light_dir_center);
    vec3_scale(&far, &diff, 8000000.0f);
    copyVec3(&pos, &far);
    light_obj_pos_set(obj, &pos);
}

/* 0x802C1EA0 (0xC4): sets a plain light object: a direction (`mode` 0) or a position, a colour and no spot or
 * distance attenuation. */
extern "C" void light_obj_init(g3d::LightObj* obj, math::VEC3* pos, GXColor color, s32 mode)
{
    obj->Clear();
    if (mode == 0) {
        light_obj_dir_set(obj, pos);
    } else {
        light_obj_pos_set(obj, pos);
    }
    obj->InitLightColor(color);
    obj->InitLightSpot(0.0f, 0);
    obj->InitLightDistAttn(0.0f, 0.0f, 0);
    light_obj_color_enable(obj);
    light_obj_alpha_disable(obj);
    light_obj_enable(obj);
}

/* 0x802C1F64 (0x10): clears the light object's colour-disable flag. */
extern "C" void light_obj_color_enable(g3d::LightObj* obj)
{
    obj->mFlag &= ~0x10;
}

/* 0x802C1F74 (0x60): sets scene light object `index` as a plain light. */
void make_dir_light2(s32 index, math::VEC3* vec, _GXColor color, s32 flag)
{
    g3d::LightObj* obj = light_obj_area_select(index);

    light_obj_init(obj, vec, color, flag);
}

/* 0x802C1FD4 (0xD0): sets a point light object with distance attenuation. */
extern "C" void light_obj_point_init(g3d::LightObj* obj, math::VEC3* pos, GXColor color, f32 ref_dist, f32 ref_bright)
{
    obj->Clear();
    light_obj_pos_set(obj, pos);
    obj->InitLightColor(color);
    obj->InitLightSpot(0.0f, 0);
    obj->InitLightDistAttn(ref_dist, ref_bright, 3);
    light_obj_color_enable(obj);
    light_obj_alpha_disable(obj);
    light_obj_enable(obj);
}

/* 0x802C20A4 (0x38): points the scene's main light along `dir`. */
void light_dir_set(math::VEC3* dir)
{
    light_obj_dir_set(light_obj_area_select(0), dir);
}

/* 0x802C20DC (0xDC): sets light objects 30-35 from the area's two cell rows. */
void light_area_setup(void)
{
    math::VEC3 pos;
    math::VEC3 vec;
    GXColor color;
    u32 cell;
    g3d::LightObj* objs;
    s32 row;
    s32 n;
    s32 k;

    VEC3_ctor(&pos);
    objs = light_setting_get_light_obj_array(root_light_setting_get((LightSceneRoot*)pRoot));
    for (n = 0, row = 0; n < 2; row += 3, n++) {
        for (k = 0; k < 3; k++) {
            cell = stage_cell_get(row, k);
            color_rgba_copy((u8*)&color, (u8*)&cell);
            stage_cell_vec_get(&vec, row, k);
            copyVec3(&pos, &vec);
            light_obj_init(&objs[k + row + 30], &pos, color, 0);
        }
    }
}

/* 0x802C21B8 (0x78): the nearest point light's colour at `pos`; returns 0 when no point light is registered. */
extern "C" s32 light_point_color_get(math::VEC3* pos, GXColor* out)
{
    LightPoint* p = light_point_nearest_get(&light_point_work, pos, 0, 0);
    u32 at;

    if (p != NULL) {
        at = light_point_color_at(p, pos);
        color_rgba_copy((u8*)out, (u8*)&at);
        return 1;
    }
    return 0;
}

/* 0x802C2230 (0x44): selects light object 0 into light set 0. */
void light_area_select(void)
{

    g3d::LightSet set;

    light_setting_get_light_set(&set, root_light_setting_get((LightSceneRoot*)pRoot), 0);
    set.SelectLightObj(0, 0);
}

/* 0x802C2274 (0x44): removes light object 0 from light set 0. */
void light_area_release(void)
{

    g3d::LightSet set;

    light_setting_get_light_set(&set, root_light_setting_get((LightSceneRoot*)pRoot), 0);
    set.SelectLightObj(0, -1);
}

/* 0x802C22B8 (0x5C): demo 33 takes a point-light id and dims the ambient light. */
void light_demo_start(u8 demo)
{
    if (demo == 33) {
        light_demo_id = light_id_next();
        light_demo_amb.r = 3;
        light_demo_amb.g = 3;
        light_demo_amb.b = 10;
        set_amblight(0, light_demo_amb);
    }
}

/* 0x802C2314 (0x24): restores the area's ambient light and stops every point light. */
void light_demo_end(void)
{
    stage_area_ambient_apply();
    light_point_all_clear();
}

/* 0x802C2338 (0x108): demo 33 moves its point light along the demo's frame table. */
void light_demo_update(u8 demo)
{
    math::VEC3 pos;
    GXColor color;
    f32* table;
    s16 frame;
    u8 i;

    if (demo == 33) {
        setVec3(&pos, -603.0f, 255.0f, -2463.0f);
        color.r = 255;
        color.g = 128;
        color.b = 0;
        table = light_demo_point_table;
        i = 0;
        frame = get_demo_data() * 2;
        for (; table[i] > 0.0f; i += 4) {
            if ((s16)table[i] <= frame && (s16)table[i + 1] > frame) {
                pos.x = table[i + 2];
                pos.z = table[i + 3];
            }
        }
        light_point_set(light_demo_id, &pos, &color, light_demo_range, light_demo_falloff);
    }
}

/* 0x802C2440 (0xD0): the demo's fixed colour for light `index` of the enemy, when demo 27 or 33 overrides it. */
extern "C" u32 light_demo_color_override(_ENEMY_WORK* em, u8 index, GXColor* color)
{
    switch (get_demo_no()) {
    case 27:
        if (em->team == 3 && index == 0) {
            color->r = 100;
            color->g = 100;
            color->b = 100;
            return 1;
        }
        break;
    case 33:
        if ((em->team == 4 || em->team == 26 || em->team == 35) && index == 0) {
            color->r = 0;
            color->g = 0;
            color->b = 0;
            return 1;
        }
        break;
    }
    return 0;
}

/* 0x802C2510 (0x20): clamps `value` into [low, high]. */
extern "C" s32 light_clamp(s32 value, s32 low, s32 high)
{
    if (value < low) {
        return low;
    }
    if (value > high) {
        return high;
    }
    return value;
}

/* ------------------------------------------------------------------------------------------------ */
/* static initialisation                                                                             */
/* ------------------------------------------------------------------------------------------------ */

/* 0x802C2530 (0xA0): constructs the module work and sets the light centre and the lobby directions. */
extern "C" void light_static_init(void)
{
    light_point_work_ctor(&light_point_work);
    setVec3(&light_dir_center, 0.0f, 0.0f, 0.0f);
    setVec3(&light_lobby_dir[0], 0.0f, 0.0f, 50.0f);
    setVec3(&light_lobby_dir[1], -50.0f, 70.0f, 20.0f);
    setVec3(&light_lobby_dir[2], 70.0f, -30.0f, -20.0f);
    setVec3(&light_lobby_dir[3], 0.0f, 0.0f, 0.0f);
}

/* 0x802C25D0 (0x94): constructs the work's point lights, flares and spot channels. */
extern "C" LightPointWork* light_point_work_ctor(LightPointWork* self)
{
    LightPoint* p;
    LightFlare* f;
    LightSpot* s;

    p = self->point;
    do {
        light_point_ctor(p);
        p++;
    } while (p < &self->point[10]);
    f = self->flare;
    do {
        light_flare_ctor(f);
        f++;
    } while (f < (LightFlare*)self->spot);
    s = self->spot;
    do {
        light_spot_ctor(s);
        s++;
    } while (s < (LightSpot*)self->em_slot);
    return self;
}

/* 0x802C2664 (0x34): constructs a spot channel's point record. */
extern "C" LightSpot* light_spot_ctor(LightSpot* self)
{
    light_point_ctor(&self->point);
    return self;
}

/* 0x802C2698 (0x34): constructs a flare's model. */
extern "C" LightFlare* light_flare_ctor(LightFlare* self)
{
    mhchar_construct(&self->chr);
    return self;
}

/* 0x802C26CC (0x34): constructs a point light's position. */
extern "C" LightPoint* light_point_ctor(LightPoint* self)
{
    VEC3_ctor(&self->pos);
    return self;
}

/* The module work and the light centre / lobby light directions `light_static_init` sets (.bss). */
LightPointWork light_point_work;
math::VEC3 light_dir_center;
math::VEC3 light_lobby_dir[4];
