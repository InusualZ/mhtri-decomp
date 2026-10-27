/*
 * `userdata_item.cpp` - the face-sprite queue, the sub-transfer lists, the frame draw and the save-block (user data)
 * helpers: init/new game/save image, the player look, the community profile, the item pouches, the item box and the
 * equipment box.
 * RANGE. extab 0x80006AF0..0x80006CF0, extabindex 0x8001E9D8..0x8001ECD8, .text 0x80048964..0x8004C9A0 (87 functions,
 *   all written), .data 0x805813E8..0x80581544, .bss 0x806699B8..0x80669F68, .sbss 0x80794878..0x80794884,
 *   .sdata2 0x80795BA8..0x80795BC0.  The seam at 0x80048964 is where the extab/extabindex records and the `.bss` object
 *   the sub-transfer code owns start.  `.data` and `.sdata2` are emitted byte-identical to the target.
 * FLAGS. `#pragma peephole off`: retail re-extends narrow parameters at entry (`clrlwi`/`extsh`) and keeps
 *   `addi`+`clrlwi` pairs.
 * NAMES. Class 4: no `__FILE__` string and no runtime-dump name covers the range.  GUESS (from each body and its callers):
 *   the unit name; every own row renamed here (face_spr_*, subTransReset, frame_draw, scene_draw_single/split,
 *   scn_root_camera_id_get, scn_mdl_draw, userdata_init, userdata_pl_look_set, equip_record_validate*,
 *   weapon_record_validate, weapon_part_record_validate, userdata_box_pages_get, userdata_equip_box_capacity,
 *   userdata_equip_item_value_get, item_id_excluded_ck, item_pouch_take, userdata_item_consume, item_room_get,
 *   item_room_or_full_get, userdata_pouch_room_get, userdata_item_room_get, item_box_slot_take, item_box_take,
 *   userdata_pouch_item_give, item_pair_clear, item_slots_store, item_slots_merge, userdata_equip_box_free_index,
 *   userdata_equip_box_rec_get, userdata_equip_box_count, equip_record_register, userdata_start_equip_table,
 *   userdata_default_flag_bits); the registry names of the callees (equip_kind_item_count, face_draw,
 *   frame_draw_end_hook, fade_draw_upper/lower, screen_tex_draw, layout_overlay_draw, sys_tex_info_get,
 *   sys_tex_obj_get, nw_res_view_mtx_apply, prim_list_exec, prim_queue_draw); the Q_UserData / NetUserProfile /
 *   SystemWork field names added for this range.  `userdata_item_total_count` is the map's older name for a pouch-room
 *   query (the sibling of `userdata_item_room_get`), kept.
 * RESIDUALS. register allocation or scheduling only (no source shape found): userdata_save_import and userdata_init
 *   (the `system_w` base is materialised after the first load), item_box_take, item_box_store, item_slots_store,
 *   userdata_item_consume, userdata_equip_box_rec_get/set/add/add_new, userdata_new_game_init (the arena-row loop's
 *   registers and the flag-bit loop's test), item_room_get/item_room_or_full_get (retail keeps the found count in r31),
 *   drawSpr2TF (`fmuls` operand order), userdata_pl_look_set.
 *   face_spr_draw / face_spr_add: retail leaves each search loop straight into the draw/queue code (a goto-shaped
 *   exit rule 8 forbids); the loop-then-test form keeps the counter live (+32 B / +12 B).
 *   Flip blockers: the 11 registry callees resolve only once their names land in the map; retail force-actives
 *   userdata_item_room_get, userdata_equip_box_free_index and userdata_equip_box_rec_get (.comment), ours does not.
 */

#pragma peephole off

#include "types.h"
#include "MTX/mtx.h"                       /* PSMTXIdentity, Mtx */
#include "MTX/mtx44.h"                     /* C_MTXOrtho, Mtx44 */
#include "id_value.h"
#include "gx.h"
#include "RVLGX/GXAttr.h"
#include "quest/quest_types.h"             /* Q_UserData, the save block */
#include "get_userdata.h"                  /* get_userdata (fn_8004CAD8.cpp) */
#include "fn_8004CAD8.h"                   /* the save-block helpers fn_8004CAD8.cpp owns */
#include "my_set_clear.h"                  /* my_set_clear (fn_8004CAD8.cpp) */
#include "gun_set_clear.h"                 /* gun_set_clear (fn_8004CAD8.cpp) */
#include "equip_record_wear.h"             /* equip_record_wear (fn_8004CAD8.cpp) */
#include "userdata_item_seen_set.h"        /* userdata_item_seen_set (fn_8004CAD8.cpp) */
#include "gallery_open.h"                  /* gallery_open (fn_8004CAD8.cpp) */
#include "mh3_pad/system_w.h"              /* system_w (mh3_pad.cpp) */
#include "Pl/plw.h"                        /* _PLW, _EQUIP */
#include "Pl/Get_equip_rare.h"             /* Get_pl_type (Pl/pl_act.cpp) */
#include "Pl/fn_8027D684.h"                /* the equipment colour helpers (Pl/pl_act.cpp) */
#include "Pl/equip_color_set.h"            /* equip_color_set (Pl/pl_act.cpp) */
#include "Pl/pl_item_add.h"                /* pl_item_add (Pl/pl_act.cpp) */
#include "Pl/pl_item_page2_ck.h"           /* pl_item_page2_ck (Pl/pl_act.cpp) */
#include "Pl/pl_net_slot_bind.h"           /* pl_equip_model_set & co (Pl/pl_act.cpp) */
#include "Pl/equip_kind_item_count.h"      /* equip_kind_item_count (Pl/pl_act.cpp) */
#include "menu/menu_item.h"                /* GetItemData, ItemDataRecord (menu/menu_item.cpp) */
#include "Network/net_session_close.h"     /* NetUserProfile */
#include "ef/fn_800CDB2C.h"                /* ran_suu, get_str_tbl (ef/system_core.cpp) */
#include "font/flfnt.h"                    /* utf82unicode2 */
#include "Runtime.PPCEABI.H/memset.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "OS/OSTime.h"                     /* OSGetTime */
#include "RVLGX/GXFrameBuf.h"
#include "RVLGX/GXGeometry.h"
#include "RVLGX/GXLight.h"
#include "RVLGX/GXMisc.h"
#include "RVLGX/GXPixel.h"
#include "RVLGX/GXTev.h"
#include "RVLGX/GXBump.h"
#include "RVLGX/GXTexture.h"
#include "RVLGX/GXTransform.h"
#include "g3d/g3d_scnmdlsmpl.h"            /* nw4r::g3d::ScnMdlSimple (GetDrawMode is defined here) */
#include "g3d/g3d_scnroot.h"               /* nw4r::g3d::ScnRoot */
#include "g3d/g3d_state.h"                 /* g3d_state_invalidate */
#include "g3d/scn_root_get_current_camera.h"
#include "ef/pRoot.h"                      /* pRoot (nw_resource.cpp) */
#include "ef/sys_tex_info_get.h"           /* sys_tex_info_get (ef/system_core.cpp) */
#include "ef/system_lobby_flag_get.h"      /* system_lobby_flag_get (ef/system_core.cpp) */
#include "mh3_pad/Screen_w.h"              /* Screen_w (mh3_pad.cpp) */
#include "pad_connect.h"                   /* screen_split_mode_ck */
#include "font/dbg_drawgraph.h"           /* dbg_drawgraph_init (font/dbg_drawgraph.cpp) */
#include "stage/stage_area_colour_update.h"
#include "sound/fn_800E3CBC.h"             /* sound_free_lists_reset */
#include "sound/prim_list_exec.h"          /* prim_list_exec (sound/fn_800E3CBC.cpp) */
#include "sound/prim_queue_draw.h"         /* prim_queue_draw (sound/fn_800E3CBC.cpp) */
#include "camera/camera_lobby_viewport_fit.h"
#include "camera/camera_lobby_viewport_copy.h"
#include "nw_res_view_mtx_apply.h"         /* nw_res_view_mtx_apply (nw_resource.cpp) */
#include "face_buf_tbl.h"                  /* face_buf_tbl (fn_80047398.cpp) */
#include "face_spr.h"                       /* FaceSpr, FaceSprBuf */
#include "face_draw.h"                     /* face_draw (fn_80047398.cpp) */
#include "frame_draw_end_hook.h"           /* frame_draw_end_hook (fn_80047398.cpp) */
#include "fade_draw_upper.h"               /* fade_draw_upper (fn_80056F24.cpp) */
#include "fade_draw_lower.h"               /* fade_draw_lower (fn_80056F24.cpp) */
#include "screen_tex_draw.h"               /* screen_tex_draw (fn_80056F24.cpp) */
#include "layout_overlay_draw.h"           /* layout_overlay_draw (fn_80056F24.cpp) */
#include "spr2tf.h"                        /* fltSpr2TF */

/* --- the unit's own types ------------------------------------------------------------------------------- */

/* The equipment record as `equip_record_copy` moves it: the three decoration skill ids' first two as one word.
 * size: 0xC */
#pragma pack(push, 2)
struct EquipRecordWords {
    /* +0x0 */ u8 kind;
    /* +0x1 */ u8 deco_count;
    /* +0x2 */ u16 item_id;
    /* +0x4 */ u16 color;
    /* +0x6 */ u32 skills_01;
    /* +0xA */ u16 skill_2;
};
#pragma pack(pop)

/* The 0x100-byte look record `arena_userdata_apply` dresses a player work from (`quest/arenatask.cpp`'s
 * `arena_user_data_buf` records): the save's look bytes, nine equipment records (weapon, two parts, the armour) and
 * the pouch with its ammo page.  size: 0x100 */
struct UserLookRecord {
    /* +0x00 */ u8 sex_0x00;
    /* +0x01 */ u8 face_0x01;
    /* +0x02 */ u8 voice_0x02;
    /* +0x03 */ u8 hair_0x03;
    /* +0x04 */ u32 hair_color_0x04;
    /* +0x08 */ u8 equip_set_0x08;
    /* +0x09 */ u8 face_type_0x09;
    /* +0x0A */ u8 face_eye_0x0A;
    /* +0x0B */ u8 appearance_0x0B;
    /* +0x0C */ u8 makeup_0x0C;
    /* +0x0D */ u8 makeup_color_0x0D;
    /* +0x0E */ u8 pad_0x0E[2];
    /* +0x10 */ u32 inner_color_0x10;
    /* +0x14 */ _EQUIP equip_0x14[9];
    /* +0x80 */ IdValue pouch_0x80[0x18];
    /* +0xE0 */ IdValue page2_0xE0[8];
};

/* One starting equipment set of `userdata_start_equip_table`: the weapon, an empty part and armour kinds 1-5.
 * size: 0x54 */
struct UserStartEquip {
    /* +0x00 */ _EQUIP weapon;
    /* +0x0C */ _EQUIP part;
    /* +0x18 */ _EQUIP armour[5];
};

/* A queued sub-transfer call: `func(argc, args)`.  size: 0x2C */
typedef void (*SubTransFunc)(u8 argc, u32* args);
struct SubTrans {
    /* +0x00 */ SubTransFunc func;
    /* +0x04 */ u8 used;
    /* +0x05 */ u8 argc;
    /* +0x06 */ u8 pad_0x06[2];
    /* +0x08 */ u32 args[8];
    /* +0x28 */ SubTrans* next;
};

/* The eleven priority lists of queued sub-transfer calls.  size: 0x30 */
struct SubTransState {
    /* +0x00 */ SubTrans* head[11];
    /* +0x2C */ u8 pad_0x2C[4];
};

/* The scene root `pRoot` holds. */
static inline nw4r::g3d::ScnRoot* scn_root(void)
{
    return (nw4r::g3d::ScnRoot*)pRoot;
}

/* The community profile's equipment record `index` (the profile spells the run as bytes). */
static inline _EQUIP* profile_equip(NetUserProfile* profile, s32 index)
{
    return (_EQUIP*)profile->equip_0x10[index];
}

/* --- the unit's own data -------------------------------------------------------------------------------- */

/* .data 0x805813E8: the four starting equipment sets `userdata_new_game_init` picks the weapon kind from. */
UserStartEquip userdata_start_equip_table[4] = {
    {{7, 0, 1}, {0}, {{1, 0, 1}, {2, 0, 1}, {3, 0, 1}, {4, 0, 1}, {5, 0, 1}}},
    {{8, 0, 1}, {0}, {{1, 0, 3}, {2, 0, 3}, {3, 0, 3}, {4, 0, 3}, {5, 0, 3}}},
    {{11, 0, 1}, {0}, {{1, 0, 6}, {2, 0, 6}, {3, 0, 6}, {4, 0, 6}, {5, 0, 6}}},
    {{8, 0, 1}, {0}, {{1, 0, 0}, {2, 0, 0}, {3, 0, 0}, {4, 0, 0}, {5, 0, 0}}},
};

/* .data 0x80581538: the bits of the save's +0x3960 set a new game raises, ended by 0xFF. */
u8 userdata_default_flag_bits[12] = {0x31, 0x09, 0x32, 0x17, 0x2C, 0x5C, 0x38, 0x48, 0x72, 0x3A, 0xFF};

#include "userdata_item/bg_tex_disp_hook.h" /* BgTexDispHook (rule 1) */

extern u8* lobby_world_block;
extern SubTrans sub_trans_buf[0x20];
extern SubTransState sub_trans_state;
extern BgTexDispHook bg_tex_disp_func;

/* --- the unit's own functions, declared once so each has one signature ----------------------------------- */

extern "C" {
_EQUIP* equip_record_copy(_EQUIP* dst, const _EQUIP* src);
void item_pair_copy(IdValue* dst, const IdValue* src);
void applyNetUserProfile(NetUserProfile* profile);
u16 userdata_box_capacity(Q_UserData* user);
u32 userdata_gunner_ck(Q_UserData* user);
IdValue* userdata_pouch_get(Q_UserData* user, s32 kind);
IdValue* userdata_equip_item_slots_get(Q_UserData* user);
s32 item_count_find(u16 id, const IdValue* table, s32 count);
s32 item_slots_count_sum(u16 id, const IdValue* table, s32 count);
s16 item_box_take(u16 id, s16 delta);
s32 item_pair_index_find(u16 id, const IdValue* table, s32 count);
s16 item_take(u16 id, s16 count, IdValue* table, s32 n, u8 seen, s16 max);
u32 item_pair_clear(IdValue* entry);
u32 item_slots_store(IdValue* table, u16 n, u16 id, u32 count, u16* slot);
/* free: retail C linkage evidenced (unmangled map name) */
void color_rgba_copy(u8* dst, const u8* src);
u16 userdata_equip_box_add_new(Q_UserData* user, u8 kind, u16 item_id);
s32 userdata_equip_box_worn_ck(Q_UserData* user, u16 index);
void face_spr_gx_end(void);
void face_spr_gx_texcoord(f32 s, f32 t);
void face_spr_gx_color(u32 color);
void face_spr_gx_position(s16 x, s16 y);
void face_spr_texcoordgen_set(u32 id, u32 type, u32 src, u32 mtx);
void face_spr_add(u8 tex, const FaceSpr* spr, u8 index);
void subTransSetStackSetup(s32 from, s32 to);
void scene_draw_single(void);
void scene_draw_split(void);
s32 scn_root_camera_id_get(nw4r::g3d::ScnRoot* root);
}
/* --- the face sprite queue (`face_buf_tbl`) ----------------------------------------------------------- */

/* Draws face-sprite buffer `index`: an orthographic screen pipeline, then one quad per queued sprite, run by run with
 * each run's texture loaded. */
extern "C" void face_spr_draw(u8 index)
{
    FaceSprBuf* buf = face_buf_tbl[index];
    Mtx44 proj;
    Mtx view;
    s32 run;
    s32 i;
    FaceSpr* spr;

    for (run = 0; run < 0x20; run++) {
        if (buf->count[run] > 0) {
            break;
        }
    }
    if (run >= 0x20) {
        return;
    }
    dbg_drawgraph_init();
    GXSetCopyFilter(0, 0, 0, 0);
    GXInvalidateTexAll();
    GXSetBlendMode(1, 4, 5, 0);
    GXSetColorUpdate(1);
    C_MTXOrtho(proj, 0.0f, Screen_w.height_f, 0.0f, Screen_w.width_f, 0.0f, 1.0f);
    GXSetProjection(proj, 1);
    PSMTXIdentity(view);
    GXLoadPosMtxImm(view, 0);
    GXSetCurrentMtx(0);
    GXSetCullMode(0);
    GXSetZMode(1, 7, 0);
    GXSetNumTexGens(1);
    face_spr_texcoordgen_set(0, 1, 4, 0x3C);
    GXSetNumTevStages(1);
    GXSetTevDirect(0);
    GXSetTevOp(0, 0);
    GXSetTevOrder(0, 0, 0, 4);
    GXSetNumChans(1);
    GXSetChanCtrl(4, 0, 0, 1, 0, 0, 2);
    GXClearVtxDesc();
    GXSetVtxDesc(9, 1);
    GXSetVtxDesc(11, 1);
    GXSetVtxDesc(13, 1);
    GXSetVtxAttrFmt(0, 9, 0, 3, 0);
    GXSetVtxAttrFmt(0, 11, 1, 5, 0);
    GXSetVtxAttrFmt(0, 13, 1, 4, 0);
    for (run = 0; run < 0x20; run++) {
        if (buf->tex[run] != -1 && buf->count[run] > 0) {
            GXLoadTexObj((GXTexObj*)sys_tex_obj_get(buf->tex[run]), 0);
            GXBegin(0x80, 0, buf->count[run] * 4);
            spr = buf->spr[run];
            for (i = 0; i < buf->count[run]; i++) {
                face_spr_gx_position(spr->x0, spr->y0);
                face_spr_gx_color(spr->color);
                face_spr_gx_texcoord(spr->s0, spr->t0);
                face_spr_gx_position(spr->x1, spr->y0);
                face_spr_gx_color(spr->color);
                face_spr_gx_texcoord(spr->s1, spr->t0);
                face_spr_gx_position(spr->x1, spr->y1);
                face_spr_gx_color(spr->color);
                face_spr_gx_texcoord(spr->s1, spr->t1);
                face_spr_gx_position(spr->x0, spr->y1);
                face_spr_gx_color(spr->color);
                face_spr_gx_texcoord(spr->s0, spr->t1);
                spr++;
            }
            face_spr_gx_end();
        }
    }
}

/* Ends a sprite primitive (the inline `GXEnd`). */
extern "C" void face_spr_gx_end(void)
{
}

/* Writes one texture coordinate pair to the GX FIFO. */
extern "C" void face_spr_gx_texcoord(f32 s, f32 t)
{
    GXWGFifo.f32 = s;
    GXWGFifo.f32 = t;
}

/* Writes one colour word to the GX FIFO. */
extern "C" void face_spr_gx_color(u32 color)
{
    GXWGFifo.u32 = color;
}

/* Writes one 2D position to the GX FIFO. */
extern "C" void face_spr_gx_position(s16 x, s16 y)
{
    GXWGFifo.s16 = x;
    GXWGFifo.s16 = y;
}

/* Sets the sprites' texture-coordinate generator (identity post-transform). */
extern "C" void face_spr_texcoordgen_set(u32 id, u32 type, u32 src, u32 mtx)
{
    GXSetTexCoordGen2(id, type, src, mtx, 0, 0x7D);
}

/* Queues sprite `spr` in buffer `index` under texture `tex`: in that texture's run, or in the first free run; a full
 * run (16 sprites) drops it. */
extern "C" void face_spr_add(u8 tex, const FaceSpr* spr, u8 index)
{
    s32 run;
    FaceSprBuf* buf = face_buf_tbl[index];

    for (run = 0; run < 0x20; run++) {
        if (tex == buf->tex[run]) {
            break;
        }
    }
    if (run == 0x20) {
        for (run = 0; run < 0x20; run++) {
            if (buf->tex[run] == -1) {
                buf->tex[run] = tex;
                break;
            }
        }
        if (run == 0x20) {
            return;
        }
    }
    if (buf->count[run] < 0x10) {
        memcpy(&buf->spr[run][buf->count[run]], spr, sizeof(FaceSpr));
        buf->count[run]++;
    }
}

/* Queues a textured 2D sprite on face-sprite buffer `index`: its screen rectangle, depth, colour and texel rectangle
 * normalised by texture `tex`'s size. */
void drawSpr2TF(u8 tex, fltSpr2TF* src, u8 index)
{
    FaceSpr spr;
    SysTask* info = sys_tex_info_get(tex);
    f32 w;
    f32 h;

    if (info != NULL) {
        w = info->width_0x28 - 1;
        h = info->height_0x2A - 1;
        spr.x0 = src->x;
        spr.y0 = src->y;
        spr.x1 = src->x + src->w;
        spr.y1 = src->y + src->h;
        spr.s0 = src->r / w;
        spr.s1 = src->b / w;
        spr.t0 = src->g / h;
        spr.t1 = src->a / h;
        spr.depth = (1.0f / 1024.0f) * src->angle;
        spr.color = src->color;
        face_spr_add(tex, &spr, index);
    }
}

/* --- the sub-transfer lists (deferred 2D draw calls) ---------------------------------------------------- */

/* Frees every sub-transfer entry and empties the eleven priority lists. */
extern "C" void subTransReset(void)
{
    SubTrans* t = sub_trans_buf;
    s32 i;

    for (i = 0; i < 0x20; i++) {
        t->used = 0;
        t->next = NULL;
        t++;
    }
    for (i = 0; i < 11; i++) {
        sub_trans_state.head[i] = NULL;
    }
}

/* Queues call `func(argc, args)` at the end of priority list `prio`, copying up to 8 argument words. */
void subTransSetPrio(u8 prio, u32 func, s32 argc, u32* args)
{
    SubTrans* t = sub_trans_buf;
    SubTrans* last;
    s32 i;
    s32 j;

    for (i = 0; i < 0x20; i++, t++) {
        if (t->used == 0) {
            t->used = 1;
            t->func = (SubTransFunc)func;
            if (argc > 8) {
                argc = 8;
            }
            t->argc = argc;
            if (args != NULL) {
                for (j = 0; j < argc; j++) {
                    t->args[j] = *args++;
                }
            }
            last = sub_trans_state.head[prio];
            if (last == NULL) {
                sub_trans_state.head[prio] = t;
            } else {
                for (;;) {
                    if (last->next == NULL) {
                        last->next = t;
                        break;
                    }
                    last = last->next;
                }
            }
            t->next = NULL;
            return;
        }
    }
}

/* Queues call `func(argc, args)` on priority list 3. */
void subTransSet(u32 func, s32 prio, u32* handle)
{
    subTransSetPrio(3, func, prio, handle);
}

/* Runs the queued calls of priority lists `from` .. `to - 1` (the font is flushed before lists 8 and 9). */
extern "C" void subTransSetStackSetup(s32 from, s32 to)
{
    SubTrans* t;
    s32 prio;

    for (prio = from; prio < to; prio++) {
        t = sub_trans_state.head[prio];
        if ((u32)(prio - 8) <= 1) {
            font_flush();
        }
        for (; t != NULL; t = t->next) {
            if (t->used == 1) {
                t->func(t->argc, t->args);
            }
        }
    }
}

/* --- the frame draw ------------------------------------------------------------------------------------- */

/* Draws one frame: the captured screen while one is shown, else the faces, the first sprite pass, the background
 * hook, the scene between the sub-transfer lists (split or single screen, the lobby's viewport) and the second
 * sprite pass. */
extern "C" void frame_draw(void)
{
    if (system_w.field_0x21 != 0 && pRoot != 0) {
        if (system_w.screen_tex_mode_0x86F != 0 && GameMode_ck() != 1) {
            screen_tex_draw(0);
            return;
        }
        face_draw();
        face_spr_draw(0);
        if (screen_split_mode_ck() == 0) {
            if (system_w.screen_tex_on_0x869 == 1) {
                screen_tex_draw(1);
            } else if (system_w.field_0x866 != 0 && bg_tex_disp_func.func != NULL) {
                bg_tex_disp_func.func();
            }
        }
        if (system_lobby_flag_get() == 0) {
            if (GameMode_ck() == 3) {
                subTransSetStackSetup(0, 4);
                if (screen_split_mode_ck() != 0) {
                    scene_draw_split();
                } else {
                    scene_draw_single();
                }
                subTransSetStackSetup(4, 11);
            } else {
                subTransSetStackSetup(0, 1);
                if (screen_split_mode_ck() != 0) {
                    scene_draw_split();
                } else {
                    scene_draw_single();
                }
                subTransSetStackSetup(1, 11);
            }
        } else {
            subTransSetStackSetup(0, 7);
            camera_lobby_viewport_fit();
            if (screen_split_mode_ck() != 0) {
                scene_draw_split();
            } else {
                scene_draw_single();
            }
            camera_lobby_viewport_copy();
            scn_root()->SetGlbSettings();
            subTransSetStackSetup(7, 11);
        }
        face_spr_draw(1);
        frame_draw_end_hook();
    }
}

/* The primitive queue's layer/pass pairs, in draw order. */
static inline void scene_prim_queue_draw(void)
{
    prim_queue_draw(0, 2);
    prim_queue_draw(5, 0);
    prim_queue_draw(0, 3);
    prim_queue_draw(2, 2);
    prim_queue_draw(1, 2);
    prim_queue_draw(4, 0);
    prim_queue_draw(2, 3);
    prim_queue_draw(1, 3);
    prim_queue_draw(3, 0);
}

/* Draws the scene for one screen: the primitive lists for the current camera, the global settings, the primitive
 * queue, then resets the TEV stages and (outside the lobby) draws the 2D layout layers. */
extern "C" void scene_draw_single(void)
{
    s32 camera;
    s32 i;

    sound_free_lists_reset();
    if (pRoot != 0) {
        camera = scn_root_get_current_camera(pRoot);
        prim_list_exec(&camera, 0);
        scn_root()->SetGlbSettings();
    }
    nw_res_view_mtx_apply();
    scene_prim_queue_draw();
    g3d_state_invalidate(4);
    GXSetNumIndStages(0);
    for (i = 0; i < 0x10; i++) {
        GXSetTevDirect(i);
    }
    if (system_lobby_flag_get() == 0) {
        layout_overlay_draw();
    }
}

/* Draws the scene twice, once per player's camera, then restores the current camera, resets the TEV stages and
 * draws the two half-screen fades. */
extern "C" void scene_draw_split(void)
{
    s32 camera_a;
    s32 camera_b;
    s32 current;
    s32 i;

    sound_free_lists_reset();
    if (pRoot != 0) {
        current = scn_root_camera_id_get(scn_root());
        my_player_no_set(0);
        scn_root()->SetCurrentCamera(Screen_w.camera_view.player_camera_0x50[0]);
        stage_area_colour_update();
        camera_a = scn_root_get_current_camera(pRoot);
        prim_list_exec(&camera_a, 0);
        scn_root()->SetGlbSettings();
        nw_res_view_mtx_apply();
        scene_prim_queue_draw();
        GXDrawDone();
        sound_free_lists_reset();
        my_player_no_set(1);
        scn_root()->SetCurrentCamera(Screen_w.camera_view.player_camera_0x50[1]);
        stage_area_colour_update();
        camera_b = scn_root_get_current_camera(pRoot);
        prim_list_exec(&camera_b, 0);
        scn_root()->SetGlbSettings();
        nw_res_view_mtx_apply();
        scene_prim_queue_draw();
        my_player_no_set(0);
        scn_root()->SetCurrentCamera(current);
        scn_root()->SetGlbSettings();
    }
    g3d_state_invalidate(4);
    GXSetNumIndStages(0);
    for (i = 0; i < 0x10; i++) {
        GXSetTevDirect(i);
    }
    fade_draw_upper();
    fade_draw_lower();
}

/* The scene root's current camera index. */
extern "C" s32 scn_root_camera_id_get(nw4r::g3d::ScnRoot* root)
{
    return root->mCurrentCameraID;
}

/* Draws a simple model's opaque and translucent passes in draw mode `mode` (the model's own when -1). */
extern "C" void scn_mdl_draw(nw4r::g3d::ScnMdlSimple* mdl, u32 mode)
{
    u32 draw_mode;

    if (mode == 0xFFFFFFFF) {
        draw_mode = mdl->GetDrawMode();
    } else {
        draw_mode = mode;
    }
    mdl->G3dProc(6, 0, &draw_mode);
    mdl->G3dProc(7, 0, &draw_mode);
}

namespace nw4r {
namespace g3d {

/* The model's draw mode. */
u32 ScnMdlSimple::GetDrawMode() const
{
    return mDrawMode;
}

}  // namespace g3d
}  // namespace nw4r

/* --- the save block (user data): init, new game, save image -------------------------------------------- */

/* The save block the band caches in `lobby_world_block`. */
static inline Q_UserData* userdata_block(void)
{
    return (Q_UserData*)lobby_world_block;
}

/* Caches the save block, clears it and seeds its random-seed run (and `system_w`'s ring) from the clock. */
extern "C" void userdata_init(void)
{
    u16 seed;
    s32 i;

    lobby_world_block = (u8*)get_userdata();
    memset(lobby_world_block, 0, 0x6000);
    seed = OSGetTime();
    for (i = 0; i < 0x20; i++) {
        userdata_block()->seed_0x3920[i] = seed;
    }
    system_w.field_0x18[0] = userdata_block()->seed_0x3920[0];
    system_w.field_0x18[1] = userdata_block()->seed_0x3920[1];
    system_w.field_0x18[2] = userdata_block()->seed_0x3920[2];
    system_w.field_0x18[3] = userdata_block()->seed_0x3920[3];
}

/* Caches the save block and clears it. */
extern "C" void userdata_clear(void)
{
    lobby_world_block = (u8*)get_userdata();
    memset(lobby_world_block, 0, 0x6000);
}

/* Sets a new character's save up: clears the equipment, pouches, box and progress, files the starting weapon of
 * `kind` (0/2 male, 1 female, -1 the fourth set) and the starting items, and names the arena rows. */
extern "C" void userdata_new_game_init(s32 kind)
{
    Q_UserData* user = get_userdata();
    s8 class_change;
    u16 box;
    Q_ArenaRow* row;
    u8** names;
    s32 i;
    u8* src;
    u8* dst;
    s32 rest;
    s32 len;
    s32 j;
    const u8* bit;

    memset(&user->weapon_0x0020, 0, 0xC);
    memset(user->parts_0x002C, 0, 0x18);
    memset(&user->armour_0x0044[0], 0, 0xC);
    memset(&user->armour_0x0044[1], 0, 0xC);
    memset(&user->armour_0x0044[2], 0, 0xC);
    memset(&user->armour_0x0044[3], 0, 0xC);
    memset(&user->armour_0x0044[4], 0, 0xC);
    memset(&user->armour_0x0044[5], 0, 0xC);
    memset(user->pouch_a_0x00A0, 0, 0x60);
    memset(user->pouch_b_0x0100, 0, 0x80);
    memset(user->box_0x180, 0, 0xC80);
    memset(user->progress_flags_0x47DA, 0, 0x58);
    user->state_0x3E01 = 0;
    user->points_0x3F04 = 0;
    user->state_0x3E02 = 0;
    user->zenny_0x18 = 0;
    user->pig_name_0x483A = 0x24;
    user->cat_name_0x3F95[0] = 0x2C;
    user->cat_name_0x3F95[1] = 9;
    user->cat_name_0x3F95[2] = 0x10;
    user->pig_dress_0x483B = 0;
    user->state_0x3E03 = 0;
    user->timer_0x519C = 60;
    user->count_0x51A0 = 0;
    user->count_0x51A2 = 0;
    user->flag_0x51A4 = 0;
    user->flag_0x51A5 = 1;
    memset(user->record_0x51A6, 0, 0xE);
    memset(user->record_0x3F38, 0, 0x24);
    memset(&user->em_set_0x3F98, 0, 0xE0);
    memset(&user->ship_0x4654, 0, 0x1E);
    memset(&user->progress_flags_0x47DA[0x2C], 0, 8);
    memset(user->record_0x4850, 0, 0x10);
    memset(user->record_0x4860, 0, 0x78);
    memset(user->record_0x48D8, 0, 0x100);
    memset(user->record_0x49D8, 0, 0x100);
    memset(user->profile_0x483C, 0xFF, 0x11);
    memset(&user->note_timer_0x5270, 0, 0x1C);

    switch (kind) {
    case 0:
        user->hunter_sex_0x0000 = 0;
        break;
    case 1:
        user->hunter_sex_0x0000 = 1;
        break;
    case 2:
        user->hunter_sex_0x0000 = 0;
        break;
    case -1:
        kind = 3;
        break;
    }

    user->box_armour_0x0092[3] = 0xFFFF;
    user->box_armour_0x0092[4] = 0xFFFF;
    user->box_armour_0x0092[0] = 0xFFFF;
    user->box_armour_0x0092[1] = 0xFFFF;
    user->box_armour_0x0092[2] = 0xFFFF;
    user->box_armour_0x0092[5] = 0xFFFF;
    box = userdata_equip_box_add_new(user, userdata_start_equip_table[kind].weapon.kind, 1);
    equip_record_wear(user, box, &class_change);
    userdata_worn_refresh(user);
    userdata_equip_box_add_new(user, 7, 1);
    item_slots_store(user->box_0x180, userdata_box_capacity(user), 0x20A, 1, &box);
    if (kind < 3) {
        user->hair_color_0x0014 = 0x473735FF;
    }
    user->appearance_0x39BA = 0;
    for (i = 0; i < 20; i++) {
        my_set_clear(user, i);
    }
    for (i = 0; i < 20; i++) {
        gun_set_clear(user, i);
    }
    for (i = 0; i < 4; i++) {
        user->note_timer_0x5270.seed_0x10[i] = ran_suu(1);
        user->note_timer_0x5270.reward_0x08[i] = ran_suu(1) % 6;
    }
    memset(user->flags_0x3960, 0, 0x40);
    for (bit = userdata_default_flag_bits; *bit != 0xFF; bit++) {
        userdata_block()->flags_0x3960[*bit >> 3] |= 1 << (*bit & 7);
    }
    memset(user->slot_bytes_0x39B2, 0, 8);
    user->slots_0x39A2[0] = 0xFFFF;
    user->slots_0x39A2[1] = 0xFFFF;
    user->slots_0x39A2[2] = 0xFFFF;
    user->slots_0x39A2[3] = 0xFFFF;
    user->slots_0x39A2[4] = 0xFFFF;
    user->slots_0x39A2[5] = 0xFFFF;
    user->slots_0x39A2[6] = 0xFFFF;
    user->slots_0x39A2[7] = 0xFFFF;
    memset(user->item_rows_0x4078, 0, 0x80);
    user->face_type_0x39A0 = 2;
    user->face_eye_0x39A1 = 0;
    user->inner_color_0x39BC = 0xFFFFFFFF;
    user->makeup_0x3DE6 = 0;
    user->makeup_color_0x3DE7 = 0;
    user->event_bits_0x52E2 = 0;

    names = get_str_tbl(0x20);
    row = user->arena_rows_0x4114;
    for (i = 0; i < 12; i++) {
        memset(row, 0, 0x25);
        src = names[i];
        dst = row->data_0x00;
        rest = 30;
        for (;;) {
            len = utf82unicode2(src);
            if (len == 1 && *(s8*)src == 0) {
                break;
            }
            if (rest - len < 0) {
                break;
            }
            for (j = 0; j < len; j++) {
                *dst++ = *src++;
            }
            rest -= len;
        }
        row++;
    }
    gallery_open(3);
}

/* Copies the save block out to `dst` (the save image), with `system_w`'s random ring and the profile word. */
extern "C" void userdata_save_export(u8* dst)
{
    Q_UserData* user = get_userdata();

    user->seed_0x3920[0] = system_w.field_0x18[0];
    user->seed_0x3920[1] = system_w.field_0x18[1];
    user->seed_0x3920[2] = system_w.field_0x18[2];
    user->seed_0x3920[3] = system_w.field_0x18[3];
    user->profile_0x001C = system_w.field_0x890;
    memcpy(dst, user, 0x6000);
}

/* Copies a save image in and restores `system_w`'s random ring and profile word from it. */
extern "C" void userdata_save_import(const u8* src)
{
    Q_UserData* user = get_userdata();

    memcpy(user, src, 0x6000);
    system_w.field_0x18[0] = user->seed_0x3920[0];
    system_w.field_0x18[1] = user->seed_0x3920[1];
    system_w.field_0x18[2] = user->seed_0x3920[2];
    system_w.field_0x18[3] = user->seed_0x3920[3];
    system_w.field_0x890 = user->profile_0x001C;
}

/* --- the player look: save block / records -> player work ----------------------------------------------- */

/* Dresses the player work from the save: sex, armour, hair, voice, colours, worn weapon, face, name and the
 * pouch (with its second page when the work has one). */
extern "C" void userdata_pl_look_set(_PLW* plw)
{
    Q_UserData* user = get_userdata();
    u32 color;
    IdValue* slots;
    s32 i;

    plw->se_name_set = user->hunter_sex_0x0000;
    equip_record_copy(&plw->equipA[0], &user->armour_0x0044[0]);
    equip_record_copy(&plw->equipA[1], &user->armour_0x0044[1]);
    equip_record_copy(&plw->equipA[2], &user->armour_0x0044[2]);
    equip_record_copy(&plw->equipA[3], &user->armour_0x0044[3]);
    equip_record_copy(&plw->equipA[4], &user->armour_0x0044[4]);
    equip_record_copy(&plw->equipA[5], &user->armour_0x0044[5]);
    plw->field_0x25C[6] = user->appearance_0x39BA;
    plw->user_profile_0x1C = user->hunter_equip_0x009E;
    plw->field_0x25C[5] = user->hunter_face_0x0001;
    plw->field_0x25C[4] = user->hunter_hair_0x0002;
    plw->se_name_idx = user->hunter_voice_0x009F;
    color = user->hair_color_0x0014;
    plw->field_0x258[0] = (color & 0xFF000000) >> 24;
    plw->field_0x258[1] = (color & 0xFF0000) >> 16;
    plw->field_0x258[2] = (color & 0xFF00) >> 8;
    plw->field_0x258[3] = color;
    color = user->inner_color_0x39BC;
    plw->field_0x25C[0] = (color & 0xFF000000) >> 24;
    plw->field_0x25C[1] = (color & 0xFF0000) >> 16;
    plw->field_0x25C[2] = (color & 0xFF00) >> 8;
    plw->field_0x25C[3] = color;
    equip_record_copy(&plw->equipB, &user->weapon_0x0020);
    equip_record_copy(&plw->equipC, &user->parts_0x002C[0]);
    equip_record_copy(&plw->equipD, &user->parts_0x002C[1]);
    plw->user_profile_0xB1C[0] = user->face_type_0x39A0;
    plw->user_profile_0xB1C[1] = user->face_eye_0x39A1;
    plw->user_profile_0xB1C[2] = user->makeup_0x3DE6;
    plw->user_profile_0xB1C[3] = user->makeup_color_0x3DE7;
    plw->field_0x002 = Get_pl_type(&plw->equipB, &plw->equipC);
    for (i = 0; i < 0x11; i++) {
        plw->user_profile_0x5CA[i] = user->hunter_name_0x0003[i];
    }
    slots = userdata_equip_item_slots_get(user);
    for (i = 0; i < 24; i++, slots++) {
        item_pair_copy((IdValue*)&plw->slot_id[i], slots);
    }
    if (pl_item_page2_ck(plw) == 1) {
        for (i = 0; i < 8; i++) {
            item_pair_copy((IdValue*)&plw->slot_id[i + 26], slots);
            slots++;
        }
    }
}

/* Copies one `{id, value}` pair field by field. */
extern "C" void item_pair_copy(IdValue* dst, const IdValue* src)
{
    dst->id = src->id;
    dst->value = src->value;
}

/* Copies one 12-byte equipment record field by field (two bytes, two halfwords, the decoration word at +0x06 and the
 * halfword at +0x0A); `dst` comes back. */
extern "C" _EQUIP* equip_record_copy(_EQUIP* dst, const _EQUIP* src)
{
    EquipRecordWords* d = (EquipRecordWords*)dst;
    const EquipRecordWords* s = (const EquipRecordWords*)src;

    d->kind = s->kind;
    d->deco_count = s->deco_count;
    d->item_id = s->item_id;
    d->color = s->color;
    d->skills_01 = s->skills_01;
    d->skill_2 = s->skill_2;
    return dst;
}

/* Dresses the player work from a look record (the arena's user-data record): the same fields as the save's, and the
 * record's pouch items added through `pl_item_add`. */
extern "C" void arena_userdata_apply(const UserLookRecord* rec, _PLW* plw)
{
    u32 color;
    s32 i;

    plw->se_name_set = rec->sex_0x00;
    equip_record_copy(&plw->equipA[0], &rec->equip_0x14[4]);
    equip_record_copy(&plw->equipA[1], &rec->equip_0x14[6]);
    equip_record_copy(&plw->equipA[2], &rec->equip_0x14[5]);
    equip_record_copy(&plw->equipA[3], &rec->equip_0x14[7]);
    equip_record_copy(&plw->equipA[4], &rec->equip_0x14[3]);
    equip_record_copy(&plw->equipA[5], &rec->equip_0x14[8]);
    plw->field_0x25C[6] = rec->appearance_0x0B;
    plw->field_0x25C[5] = rec->face_0x01;
    plw->se_name_idx = rec->voice_0x02;
    plw->field_0x25C[4] = rec->hair_0x03;
    color = rec->hair_color_0x04;
    plw->field_0x258[0] = (color & 0xFF000000) >> 24;
    plw->field_0x258[1] = (color & 0xFF0000) >> 16;
    plw->field_0x258[2] = (color & 0xFF00) >> 8;
    plw->field_0x258[3] = color;
    plw->user_profile_0x1C = rec->equip_set_0x08;
    color = rec->inner_color_0x10;
    plw->field_0x25C[0] = (color & 0xFF000000) >> 24;
    plw->field_0x25C[1] = (color & 0xFF0000) >> 16;
    plw->field_0x25C[2] = (color & 0xFF00) >> 8;
    plw->field_0x25C[3] = color;
    equip_record_copy(&plw->equipB, &rec->equip_0x14[0]);
    equip_record_copy(&plw->equipC, &rec->equip_0x14[1]);
    equip_record_copy(&plw->equipD, &rec->equip_0x14[2]);
    plw->field_0x002 = Get_pl_type(&plw->equipB, &plw->equipC);
    for (i = 0; i < 8; i++) {
        if (rec->page2_0xE0[i].id != 0 && rec->page2_0xE0[i].value > 0) {
            pl_item_add(plw, rec->page2_0xE0[i].id, rec->page2_0xE0[i].value);
        }
    }
    for (i = 0; i < 24; i++) {
        if (rec->pouch_0x80[i].id != 0 && rec->pouch_0x80[i].value > 0) {
            pl_item_add(plw, rec->pouch_0x80[i].id, rec->pouch_0x80[i].value);
        }
    }
    plw->user_profile_0xB1C[0] = rec->face_type_0x09;
    plw->user_profile_0xB1C[1] = rec->face_eye_0x0A;
    plw->user_profile_0xB1C[2] = rec->makeup_0x0C;
    plw->user_profile_0xB1C[3] = rec->makeup_color_0x0D;
}

/* Dresses the player work from a received community profile (validated first). */
extern "C" void decodePlayerCard(NetUserProfile* profile, _PLW* plw)
{
    applyNetUserProfile(profile);
    plw->se_name_set = profile->sex_0x95;
    plw->field_0x25C[5] = profile->face_0x96;
    plw->se_name_idx = profile->voice_0x97;
    plw->field_0x25C[4] = profile->field_0x02;
    plw->field_0x258[0] = (profile->word_0x08 & 0xFF000000) >> 24;
    plw->field_0x258[1] = (profile->word_0x08 & 0xFF0000) >> 16;
    plw->field_0x258[2] = (profile->word_0x08 & 0xFF00) >> 8;
    plw->field_0x258[3] = profile->word_0x08;
    plw->field_0x25C[0] = (profile->word_0x0C & 0xFF000000) >> 24;
    plw->field_0x25C[1] = (profile->word_0x0C & 0xFF0000) >> 16;
    plw->field_0x25C[2] = (profile->word_0x0C & 0xFF00) >> 8;
    plw->field_0x25C[3] = profile->word_0x0C;
    plw->user_profile_0x1C = profile->field_0x03;
    plw->user_profile_0xB1C[0] = profile->face_type_0x9A;
    plw->user_profile_0xB1C[1] = profile->face_eye_0x9B;
    plw->user_profile_0xB1C[2] = profile->makeup_0x98;
    plw->user_profile_0xB1C[3] = profile->makeup_color_0x99;
    equip_record_copy(&plw->equipB, profile_equip(profile, 0));
    equip_record_copy(&plw->equipC, profile_equip(profile, 1));
    equip_record_copy(&plw->equipD, profile_equip(profile, 2));
    equip_record_copy(&plw->equipA[4], profile_equip(profile, 3));
    equip_record_copy(&plw->equipA[0], profile_equip(profile, 4));
    equip_record_copy(&plw->equipA[2], profile_equip(profile, 5));
    equip_record_copy(&plw->equipA[1], profile_equip(profile, 6));
    equip_record_copy(&plw->equipA[3], profile_equip(profile, 7));
    equip_record_copy(&plw->equipA[5], profile_equip(profile, 8));
    plw->field_0x25C[6] = profile->status_0x04;
}

/* --- the community profile: save block -> profile ------------------------------------------------------- */

/* Clears the community profile and fills it from the save (0 when there is no save block). */
extern "C" s32 buildNetUserProfile(NetUserProfile* profile)
{
    Q_UserData* user = get_userdata();

    if (user == NULL) {
        return 0;
    }
    memset(profile, 0, 0x100);
    profile->sex_0x95 = user->hunter_sex_0x0000;
    profile->face_0x96 = user->hunter_face_0x0001;
    profile->voice_0x97 = user->hunter_voice_0x009F;
    profile->field_0x02 = user->hunter_hair_0x0002;
    profile->word_0x08 = user->hair_color_0x0014;
    profile->word_0x0C = user->inner_color_0x39BC;
    profile->field_0x03 = user->hunter_equip_0x009E;
    profile->face_type_0x9A = user->face_type_0x39A0;
    profile->face_eye_0x9B = user->face_eye_0x39A1;
    profile->makeup_0x98 = user->makeup_0x3DE6;
    profile->makeup_color_0x99 = user->makeup_color_0x3DE7;
    equip_record_copy(profile_equip(profile, 0), &user->weapon_0x0020);
    equip_record_copy(profile_equip(profile, 1), &user->parts_0x002C[0]);
    equip_record_copy(profile_equip(profile, 2), &user->parts_0x002C[1]);
    equip_record_copy(profile_equip(profile, 3), &user->armour_0x0044[4]);
    equip_record_copy(profile_equip(profile, 4), &user->armour_0x0044[0]);
    equip_record_copy(profile_equip(profile, 5), &user->armour_0x0044[2]);
    equip_record_copy(profile_equip(profile, 6), &user->armour_0x0044[1]);
    equip_record_copy(profile_equip(profile, 7), &user->armour_0x0044[3]);
    equip_record_copy(profile_equip(profile, 8), &user->armour_0x0044[5]);
    profile->id_0x00 = user->hunter_rank_0x3DE4;
    profile->region_0x05 = Get_pl_type(&user->weapon_0x0020, &user->parts_0x002C[0]);
    profile->status_0x04 = user->appearance_0x39BA;
    profile->profile_0x92 = user->profile_0x3F0A;
    memcpy(profile->field_0x7C, user->profile_0x40FC, 5);
    profile->profile_0x93 = user->profile_0x484D;
    profile->partner_0x81[0] = user->profile_0x483C[0];
    profile->partner_0x81[1] = user->profile_0x483C[1];
    profile->partner_0x81[2] = user->profile_0x483C[2];
    profile->partner_0x81[3] = user->profile_0x483C[3];
    profile->partner_0x81[4] = user->profile_0x483C[4];
    profile->partner_0x81[5] = user->profile_0x483C[5];
    profile->partner_0x81[6] = user->profile_0x483C[6];
    profile->partner_0x81[7] = user->profile_0x483C[7];
    profile->partner_0x81[8] = user->profile_0x483C[8];
    profile->partner_0x81[9] = user->profile_0x483C[9];
    profile->partner_0x81[10] = user->profile_0x483C[10];
    profile->partner_0x81[11] = user->profile_0x483C[11];
    profile->partner_0x81[12] = user->profile_0x483C[12];
    profile->partner_0x81[13] = user->profile_0x483C[13];
    profile->partner_0x81[14] = user->profile_0x483C[14];
    profile->partner_0x81[15] = user->profile_0x483C[15];
    profile->partner_0x81[16] = user->profile_0x483C[16];
    profile->dress_0x94 = user->pig_dress_0x483B;
    memcpy(profile->record_0x9C, user->comment_0x528C, 0x56);
    profile->rank_0xF2 = user->net_rank_0x3F0B;
    profile->level_0xF3 = user->net_level_0x3F0C;
    applyNetUserProfile(profile);
    return 1;
}

/* Refreshes the profile's head fields (looks, equipment, rank) from the save. */
extern "C" s32 fillNetUserProfileHead(NetUserProfile* profile)
{
    Q_UserData* user = get_userdata();

    if (user == NULL) {
        return 0;
    }
    profile->field_0x02 = user->hunter_hair_0x0002;
    profile->field_0x03 = user->hunter_equip_0x009E;
    profile->status_0x04 = user->appearance_0x39BA;
    profile->region_0x05 = Get_pl_type(&user->weapon_0x0020, &user->parts_0x002C[0]);
    profile->word_0x08 = user->hair_color_0x0014;
    profile->word_0x0C = user->inner_color_0x39BC;
    equip_record_copy(profile_equip(profile, 0), &user->weapon_0x0020);
    equip_record_copy(profile_equip(profile, 1), &user->parts_0x002C[0]);
    equip_record_copy(profile_equip(profile, 2), &user->parts_0x002C[1]);
    equip_record_copy(profile_equip(profile, 3), &user->armour_0x0044[4]);
    equip_record_copy(profile_equip(profile, 4), &user->armour_0x0044[0]);
    equip_record_copy(profile_equip(profile, 5), &user->armour_0x0044[2]);
    equip_record_copy(profile_equip(profile, 6), &user->armour_0x0044[1]);
    equip_record_copy(profile_equip(profile, 7), &user->armour_0x0044[3]);
    equip_record_copy(profile_equip(profile, 8), &user->armour_0x0044[5]);
    profile->status_0x04 = user->appearance_0x39BA;
    profile->id_0x00 = user->hunter_rank_0x3DE4;
    profile->rank_0xF2 = user->net_rank_0x3F0B;
    profile->level_0xF3 = user->net_level_0x3F0C;
    applyNetUserProfile(profile);
    return 1;
}

/* Refreshes the profile's bytes +0x7C..+0x94 from the save. */
extern "C" s32 fillNetUserProfileRange7C(NetUserProfile* profile)
{
    Q_UserData* user = get_userdata();

    if (user == NULL) {
        return 0;
    }
    memcpy(profile->field_0x7C, user->profile_0x40FC, 5);
    memcpy(profile->partner_0x81, user->profile_0x483C, 0x11);
    profile->profile_0x92 = user->profile_0x3F0A;
    profile->profile_0x93 = user->profile_0x484D;
    profile->dress_0x94 = user->pig_dress_0x483B;
    return 1;
}

/* Refreshes the profile's rank and level bytes from the save. */
extern "C" s32 fillNetUserProfileRank(NetUserProfile* profile)
{
    Q_UserData* user = get_userdata();

    if (user == NULL) {
        return 0;
    }
    profile->rank_0xF2 = user->net_rank_0x3F0B;
    profile->level_0xF3 = user->net_level_0x3F0C;
    applyNetUserProfile(profile);
    return 1;
}

/* Refreshes the profile's comment record from the save. */
extern "C" s32 fillNetUserProfileRecord(NetUserProfile* profile)
{
    Q_UserData* user = get_userdata();

    if (user == NULL) {
        return 0;
    }
    memcpy(profile->record_0x9C, user->comment_0x528C, 0x56);
    return 1;
}

/* Whether the opening movie has not been seen yet (0 when there is no save block). */
extern "C" s32 isOnlineFlagClear(void)
{
    Q_UserData* user = get_userdata();

    if (user == NULL) {
        return 0;
    }
    return user->opening_seen_0x3F0D == 0;
}

/* Marks the opening movie seen. */
extern "C" void userdata_opening_seen_set(void)
{
    Q_UserData* user = get_userdata();

    if (user != NULL) {
        user->opening_seen_0x3F0D = 1;
    }
}

/* --- the community profile: validation ------------------------------------------------------------------ */

/* Resets an armour record to an empty one of `kind` unless it is of that kind with an id the kind's table holds. */
extern "C" void equip_record_validate(s32 kind, _EQUIP* rec)
{
    if (rec->kind != kind || rec->item_id >= equip_kind_item_count(rec->kind)) {
        memset(rec, 0, 0xC);
        rec->kind = kind;
    }
}

/* Validates the kind-6 armour record and its decoration skills (each below 206), else resets it to an empty kind 6. */
extern "C" void equip_record_validate_deco(_EQUIP* rec)
{
    s32 count = equip_kind_item_count(6);
    u8 deco;
    u16 next;

    if (rec->kind != 6 || rec->item_id >= count || (deco = rec->deco_count) > 3 ||
        (deco != 3 && (rec->skill_id[deco] >= 206 || ((next = deco + 1) < 3 && rec->skill_id[next] >= 206)))) {
        memset(rec, 0, 0xC);
        rec->kind = 6;
    }
}

/* Resets a weapon record that is no weapon kind (7-11, 14, 15) or has an unknown id to the default weapon (kind 8,
 * id 1). */
extern "C" void weapon_record_validate(_EQUIP* rec)
{
    u8 kind = rec->kind;

    if (((u32)(kind - 7) <= 4 || (u32)(kind - 14) <= 1) && rec->item_id < equip_kind_item_count(kind)) {
        return;
    }
    memset(rec, 0, 0xC);
    rec->kind = 8;
    rec->item_id = 1;
}

/* Clears a bowgun part record that is no part kind (12, 13) or has an unknown id. */
extern "C" void weapon_part_record_validate(_EQUIP* rec)
{
    u8 kind = rec->kind;

    if ((u32)(kind - 12) <= 1) {
        if (rec->item_id >= equip_kind_item_count(kind)) {
            memset(rec, 0, 0xC);
        }
    } else {
        memset(rec, 0, 0xC);
    }
}

/* Validates a received community profile in place: every equipment record and every look byte is brought back into
 * range. */
extern "C" void applyNetUserProfile(NetUserProfile* profile)
{
    weapon_record_validate(profile_equip(profile, 0));
    weapon_part_record_validate(profile_equip(profile, 1));
    weapon_part_record_validate(profile_equip(profile, 2));
    equip_record_validate(5, profile_equip(profile, 3));
    equip_record_validate(1, profile_equip(profile, 4));
    equip_record_validate(3, profile_equip(profile, 5));
    equip_record_validate(2, profile_equip(profile, 6));
    equip_record_validate(4, profile_equip(profile, 7));
    equip_record_validate_deco(profile_equip(profile, 8));
    if (profile->field_0x02 >= 11) {
        profile->field_0x02 = 0;
    }
    if (profile->field_0x03 >= 4) {
        profile->field_0x03 = 0;
    }
    if (profile->status_0x04 > 3) {
        profile->status_0x04 = 0;
    }
    if (profile->region_0x05 > 8) {
        profile->region_0x05 = 1;
    }
    if (profile->sex_0x95 >= 2) {
        profile->sex_0x95 = 0;
    }
    if (profile->face_0x96 >= 9) {
        profile->face_0x96 = 0;
    }
    if (profile->makeup_color_0x99 >= 13) {
        profile->makeup_color_0x99 = 0;
    }
    if (profile->voice_0x97 >= 20) {
        profile->voice_0x97 = 0;
    }
    if (profile->makeup_0x98 >= 10) {
        profile->makeup_0x98 = 0;
    }
    if (profile->id_0x00 > 999) {
        profile->id_0x00 = 999;
    }
    if (profile->rank_0xF2 > 117) {
        profile->rank_0xF2 = 0;
    }
    if (profile->level_0xF3 > 8) {
        profile->level_0xF3 = 0;
    }
    if (profile->face_type_0x9A > 2 || profile->face_eye_0x9B > 100) {
        profile->face_type_0x9A = 2;
        profile->face_eye_0x9B = 0;
    }
}

/* --- the item pouches and the item box ------------------------------------------------------------------ */

/* The item box's page count: 4, 6 with box item 580, two more with box item 581. */
extern "C" s16 userdata_box_pages_get(Q_UserData* user)
{
    s16 pages = 4;

    if ((u32)userdata_box_item_ck((LbItemDb*)user, 580) == 1) {
        pages = 6;
    }
    if ((u32)userdata_box_item_ck((LbItemDb*)user, 581) == 1) {
        pages += 2;
    }
    return pages;
}

/* The item box's slot count (100 per page). */
extern "C" u16 userdata_box_capacity(Q_UserData* user)
{
    return userdata_box_pages_get(user) * 100;
}

/* The equipment box's record count (100 per page, the same pages as the item box). */
extern "C" u16 userdata_equip_box_capacity(Q_UserData* user)
{
    return userdata_box_pages_get(user) * 100;
}

/* Whether the worn weapon makes the hunter a gunner (player type 4, 5 or 6). */
extern "C" u32 userdata_gunner_ck(Q_UserData* user)
{
    u8 type = Get_pl_type(&user->weapon_0x0020, &user->parts_0x002C[0]);

    return (u32)(type - 4) <= 2;
}

/* The record count of one pouch kind (0 blademaster, 1 gunner). */
extern "C" u32 userdata_pouch_size(s32 kind)
{
    return kind == 1 ? 0x20 : 0x18;
}

/* The record count of the pouch the worn weapon uses. */
extern "C" u32 userdata_item_slot_count(Q_UserData* user)
{
    u32 kind = 0;

    if (userdata_gunner_ck(user) == 1) {
        kind = 1;
    }
    return userdata_pouch_size(kind);
}

/* The pouch of one kind (0 blademaster, 1 gunner). */
extern "C" IdValue* userdata_pouch_get(Q_UserData* user, s32 kind)
{
    if (kind == 1) {
        return user->pouch_b_0x0100;
    }
    return user->pouch_a_0x00A0;
}

/* The pouch the worn weapon uses. */
extern "C" IdValue* userdata_equip_item_slots_get(Q_UserData* user)
{
    u32 kind = 0;

    if (userdata_gunner_ck(user) == 1) {
        kind = 1;
    }
    return userdata_pouch_get(user, kind);
}

/* The item id in record `index` of the pouch the worn weapon uses. */
extern "C" u16 userdata_equip_item_id_get(Q_UserData* user, s32 index)
{
    return userdata_equip_item_slots_get(user)[index].id;
}

/* The count in record `index` of the pouch the worn weapon uses. */
extern "C" s16 userdata_equip_item_value_get(Q_UserData* user, s32 index)
{
    return userdata_equip_item_slots_get(user)[index].value;
}

/* Whether an item id is one the pouches and the box never take (0, 223, 438 and 439). */
extern "C" u32 item_id_excluded_ck(u16 id)
{
    if ((u32)(id - 0x1B6) <= 1 || id == 0 || (s32)id == 0xDF) {
        return 1;
    }
    return 0;
}

/* Looks up an id in a 4-byte `{id, value}` table; 0 when it is absent. */
extern "C" s32 item_count_find(u16 id, const IdValue* table, s32 count)
{
    s32 value = 0;
    s32 i;

    if (id != 0) {
        for (i = 0; i < count; i++, table++) {
            if (table->id == id) {
                value = table->value;
                break;
            }
        }
    }
    return value;
}

/* How many of item `id` the save holds in the two pouches and the box, capped at 999. */
extern "C" s32 userdata_item_count_total(u16 id, Q_UserData* user)
{
    s32 total = item_count_find(id, user->pouch_a_0x00A0, 0x18);

    total += item_count_find(id, user->pouch_b_0x0100, 0x20);
    total += item_slots_count_sum(id, user->box_0x180, userdata_box_capacity(user));
    if (total > 999) {
        total = 999;
    }
    return total;
}

/* Takes up to `count` of item `id` out of one pouch; what is still to take comes back. */
extern "C" s16 item_pouch_take(Q_UserData* user, u16 id, s16 count, u8 kind)
{
    s16 held;
    s32 size;
    IdValue* pouch;

    if (kind == 0) {
        pouch = user->pouch_a_0x00A0;
        size = 0x18;
    } else {
        pouch = user->pouch_b_0x0100;
        size = 0x20;
    }
    held = item_count_find(id, pouch, size);
    if (held > 0) {
        if (count <= held) {
            item_take(id, -count, pouch, size, 1, 0);
            count = 0;
        } else {
            item_take(id, -held, pouch, size, 1, 0);
            count -= held;
        }
    }
    return count;
}

/* Consumes `-delta` of item `id`: from the box first, then from the worn weapon's pouch and the other one; what could
 * not be taken comes back. */
extern "C" s16 userdata_item_consume(Q_UserData* user, u16 id, s16 delta)
{
    s16 rest;

    if (delta > 0) {
        return 0;
    }
    rest = item_box_take(id, -(s16)-delta);
    if (rest > 0) {
        if (userdata_gunner_ck(user) == 1) {
            rest = item_pouch_take(user, id, rest, 0);
            if (rest > 0) {
                rest = item_pouch_take(user, id, rest, 1);
            }
        } else {
            rest = item_pouch_take(user, id, rest, 1);
            if (rest > 0) {
                rest = item_pouch_take(user, id, rest, 0);
            }
        }
    }
    return rest;
}

/* The room a table has for item `id`: the item's cap less the count held, or the cap when it holds none. */
extern "C" s32 item_room_get(u16 id, const IdValue* table, s32 count)
{
    s32 room = 0;
    ItemDataRecord* data;
    s32 i;

    if (id != 0) {
        data = GetItemData(id);
        if (data != NULL) {
            for (i = 0; i < count; i++, table++) {
                if (table->id == id) {
                    return data->max_num_0x003 - table->value;
                }
            }
            room = data->max_num_0x003;
        }
    }
    return room;
}

/* The same, but 0xFF (no room) when the table holds neither the item nor an empty record. */
extern "C" s32 item_room_or_full_get(u16 id, const IdValue* table, s32 count)
{
    s32 room = 0;
    ItemDataRecord* data;
    s32 empty;
    s32 i;

    if (id != 0) {
        data = GetItemData(id);
        if (data != NULL) {
            empty = 0;
            for (i = 0; i < count; i++, table++) {
                if (table->id == id) {
                    return data->max_num_0x003 - table->value;
                }
                if (table->id == 0) {
                    empty++;
                }
            }
            if (empty == 0) {
                room = -1;
            } else {
                room = data->max_num_0x003;
            }
        }
    }
    return room;
}

/* The room the save has for item `id` in pouch `kind`: an item of class 1 goes to the gunner's ammo page first. */
extern "C" s32 userdata_pouch_room_get(Q_UserData* user, u16 id, u8 kind)
{
    s32 room = 0;
    ItemDataRecord* data;

    if (id != 0) {
        data = GetItemData(id);
        if (data != NULL) {
            if (data->kind_0x00 != 1 || kind != 1 ||
                item_pair_index_find(id, userdata_pouch_get(user, kind), 0x18) != -1 ||
                (room = item_room_or_full_get(id, &user->pouch_b_0x0100[24], 8)) < 0) {
                room = item_room_or_full_get(id, userdata_pouch_get(user, kind), 0x18);
            }
        }
    }
    return room;
}

/* The room the worn weapon's pouch has for item `id` (the ammo page first for a gunner's class-1 item). */
extern "C" s32 userdata_item_room_get(Q_UserData* user, u16 id)
{
    s32 room = 0;
    ItemDataRecord* data;

    if (id != 0) {
        data = GetItemData(id);
        if (data != NULL) {
            if (data->kind_0x00 != 1 || userdata_gunner_ck(user) != 1 ||
                item_pair_index_find(id, userdata_equip_item_slots_get(user), 0x18) != -1 ||
                ((room = item_room_get(id, &user->pouch_b_0x0100[24], 8)) <= 0 &&
                 item_count_find(id, &user->pouch_b_0x0100[24], 8) <= 0)) {
                room = item_room_get(id, userdata_equip_item_slots_get(user), 0x18);
            }
        }
    }
    return room;
}

/* The same as `userdata_item_room_get`, 0xFF-aware (no room when the pouch is full). */
extern "C" s32 userdata_item_total_count(Q_UserData* user, u16 id)
{
    s32 room = 0;
    ItemDataRecord* data;

    if (id != 0) {
        data = GetItemData(id);
        if (data != NULL) {
            if (data->kind_0x00 != 1 || userdata_gunner_ck(user) != 1 ||
                item_pair_index_find(id, userdata_equip_item_slots_get(user), 0x18) != -1 ||
                ((room = item_room_or_full_get(id, &user->pouch_b_0x0100[24], 8)) <= 0 &&
                 item_count_find(id, &user->pouch_b_0x0100[24], 8) <= 0)) {
                room = item_room_or_full_get(id, userdata_equip_item_slots_get(user), 0x18);
            }
        }
    }
    return room;
}

/* The total count of item `id` over a table, capped at 999. */
extern "C" s32 item_slots_count_sum(u16 id, const IdValue* table, s32 count)
{
    s32 total = 0;
    s32 i;

    if (id != 0 && GetItemData(id) != NULL) {
        for (i = 0; i < count; i++, table++) {
            if (table->id == id) {
                total += table->value;
                if (total > 999) {
                    return 999;
                }
            }
        }
    }
    return total;
}

/* The room a table has for item `id` (99 per record of it or per empty record), capped at 999. */
extern "C" s32 item_slots_room_get(u16 id, const IdValue* table, s32 count)
{
    s32 room = 0;
    s32 i;

    if (item_id_excluded_ck(id) != 1 && GetItemData(id) != NULL) {
        for (i = 0; i < count; i++, table++) {
            if (table->id == id) {
                room += 99 - table->value;
            } else if (table->id == 0) {
                room += 99;
            }
            if (room > 999) {
                return 999;
            }
        }
    }
    return room;
}

/* Takes `-delta` from box record `index`, clearing the record when it runs out. */
extern "C" void item_box_slot_take(IdValue* box, u16 index, s16 delta)
{
    IdValue* slot;

    if (delta > 0) {
        return;
    }
    if (index >= 800) {
        return;
    }
    slot = &box[index];
    if (slot->value <= -delta) {
        slot->id = 0;
        slot->value = 0;
        return;
    }
    slot->value += delta;
}

/* Takes `-delta` of item `id` out of the item box, last record first; what could not be taken comes back. */
extern "C" s16 item_box_take(u16 id, s16 delta)
{
    Q_UserData* user = userdata_block();
    u16 cap;
    s16 rest;
    u32 n;
    s32 i;
    IdValue* slot;

    if (item_id_excluded_ck(id) == 1) {
        return 0;
    }
    if (delta > 0) {
        return 0;
    }
    cap = userdata_box_capacity(user);
    rest = -delta;
    for (n = 0; n < cap; n++) {
        if (user->box_0x180[n].id != 0 && user->box_0x180[n].value <= 0) {
            user->box_0x180[n].id = 0;
            user->box_0x180[n].value = 0;
        }
    }
    slot = &user->box_0x180[cap - 1];
    for (i = cap - 1; i >= 0; i--, slot--) {
        if (slot->id == id) {
            if (slot->value >= rest) {
                slot->value -= rest;
                rest = 0;
            } else {
                rest -= slot->value;
                slot->value = 0;
            }
            if (slot->value <= 0) {
                slot->id = 0;
            }
            if (rest <= 0) {
                return 0;
            }
        }
    }
    return rest;
}

/* The index of an id in a 4-byte `{id, value}` table, or -1. */
extern "C" s32 item_pair_index_find(u16 id, const IdValue* table, s32 count)
{
    s32 index;

    for (index = 0; index < count; table++, index++) {
        if (id == table->id) {
            return index;
        }
    }
    return -1;
}

/* Adds `count` (negative to take) of item `id` to a table of `n` records, each capped at `max` (the item's own cap
 * when 0).  The result: 0 a new record, 1 added, 2 capped, 3 already full, 4 used up, 5 no room, 6 not an item;
 * `seen` marks the item seen in the save unless the result is 4..6. */
extern "C" s16 item_take(u16 id, s16 count, IdValue* table, s32 n, u8 seen, s16 max)
{
    s16 result;
    ItemDataRecord* data;
    s32 i;

    if (item_id_excluded_ck(id) == 1) {
        result = 6;
    } else {
        data = GetItemData(id);
        if (data == NULL) {
            result = 6;
        } else {
            if (max == 0) {
                max = data->max_num_0x003;
            }
            if (item_count_find(id, table, n) == 0) {
                result = 5;
                for (i = 0; i < n; i++, table++) {
                    if (table->id == 0 && count > 0) {
                        result = 0;
                        table->id = id;
                        table->value = count;
                        if (table->value > max) {
                            table->value = max;
                        }
                        break;
                    }
                }
            } else {
                for (i = 0; i < n; i++, table++) {
                    if (table->id == id) {
                        if (count > 0 && table->value >= max) {
                            result = 3;
                            table->value = max;
                        } else {
                            table->value += count;
                            if (table->value <= 0) {
                                result = 4;
                                item_pair_clear(table);
                            } else if (table->value > max) {
                                result = 2;
                                table->value = max;
                            } else {
                                result = 1;
                            }
                        }
                        break;
                    }
                }
            }
        }
    }
    if (seen != 0 && (u32)(result - 4) > 2) {
        userdata_item_seen_set((LbNpcUserData*)lobby_world_block, id);
    }
    return result;
}

/* Gives `count` of item `id` to pouch `kind`: a gunner's class-1 item goes to the ammo page unless the pouch already
 * holds it or the page has no room. */
extern "C" void userdata_pouch_item_give(Q_UserData* user, u16 id, s16 count, u8 kind, u8 seen)
{
    if (GetItemData(id)->kind_0x00 != 1 || kind != 1 ||
        item_pair_index_find(id, userdata_pouch_get(user, kind), 0x18) != -1 ||
        (u32)item_take(id, count, &user->pouch_b_0x0100[24], 8, seen, 0) > 4) {
        item_take(id, count, userdata_pouch_get(user, kind), 0x18, seen, 0);
    }
}

/* Gives `count` of item `id` to the pouch the worn weapon uses. */
extern "C" void userdata_item_give(Q_UserData* user, u16 id, s16 count, u8 seen)
{
    userdata_pouch_item_give(user, id, count, userdata_gunner_ck(user) == 1, seen);
}

/* Clears one record and reports whether it had been in use. */
extern "C" u32 item_pair_clear(IdValue* entry)
{
    entry->value = 0;
    if (entry->id == 0) {
        return 0;
    }
    entry->id = 0;
    return 1;
}

/* Stores `count` of item `id` in a table of `n` records (99 per record): it tops up the records already holding it,
 * then fills empty ones; the last record touched goes to `*slot`, and what did not fit comes back. */
extern "C" u32 item_slots_store(IdValue* table, u16 n, u16 id, u32 count, u16* slot)
{
    IdValue* rec;
    u32 i;
    u32 room;

    if (item_id_excluded_ck(id) == 1) {
        return 0;
    }
    *slot = 0;
    rec = table;
    for (i = 0; i < n; i++, rec++) {
        if (rec->id == id && rec->value < 99) {
            room = 99 - rec->value;
            if (room >= count) {
                rec->value += (s16)count;
                userdata_item_seen_set((LbNpcUserData*)lobby_world_block, id);
                *slot = i;
                return 0;
            }
            count -= room;
            rec->value = 99;
            *slot = i;
        }
    }
    for (i = 0; i < n; i++, table++) {
        if (table->id == 0) {
            userdata_item_seen_set((LbNpcUserData*)lobby_world_block, id);
            table->id = id;
            if (count <= 99) {
                table->value = (s16)count;
                *slot = i;
                return 0;
            }
            table->value = 99;
            count -= 99;
            *slot = i;
        }
    }
    return count;
}

/* Stores `count` of item `id` in the save's item box; what did not fit comes back. */
extern "C" u32 item_box_store(u16 id, u32 count, u16* slot)
{
    Q_UserData* user = userdata_block();

    if (item_id_excluded_ck(id) == 1) {
        return 0;
    }
    return item_slots_store(user->box_0x180, userdata_box_capacity(user), id, count, slot);
}

/* Merges the partial stacks of a table: each record below 99 takes from the later records of the same item. */
extern "C" void item_slots_merge(IdValue* table, u32 n)
{
    u32 i;
    u32 j;
    s16 room;
    IdValue* other;

    for (i = 0; i < n - 1; i++, table++) {
        if (table->id != 0 && (u16)(table->value - 1) <= 97) {
            room = 99 - table->value;
            other = table + 1;
            for (j = i + 1; j < n; j++, other++) {
                if (other->id != 0 && other->value > 0 && other->id == table->id) {
                    if (room >= other->value) {
                        table->value += other->value;
                        room -= other->value;
                        other->id = 0;
                        other->value = 0;
                    } else {
                        table->value += room;
                        other->value -= room;
                        break;
                    }
                }
            }
        }
    }
}

/* Counts the free entries of a 4-byte `{id, value}` table. */
extern "C" u16 item_slots_free_count(const IdValue* table, s32 count)
{
    u16 free = 0;
    s32 i;

    for (i = 0; i < count; i++, table++) {
        if (table->id == 0) {
            free++;
        }
    }
    return free;
}

/* --- the equipment box and the worn records -------------------------------------------------------------- */

/* Dresses the player work in the save's worn equipment: worn records refreshed, sex, hair and face colour parts, the
 * colours and every piece's model. */
extern "C" void userdata_pl_look_apply(_PLW* plw, Q_UserData* user)
{
    userdata_worn_refresh(user);
    plw->se_name_set = user->hunter_sex_0x0000;
    pl_parts_color_set(plw, 6, user->hunter_face_0x0001);
    pl_parts_color_set(plw, 5, user->hunter_hair_0x0002);
    plw->field_0x258[0] = user->hair_color_0x0014 >> 24;
    plw->field_0x258[1] = (user->hair_color_0x0014 >> 16) & 0xFF;
    plw->field_0x258[2] = (user->hair_color_0x0014 >> 8) & 0xFF;
    plw->field_0x258[3] = user->hair_color_0x0014;
    plw->field_0x25C[0] = user->inner_color_0x39BC >> 24;
    plw->field_0x25C[1] = (user->inner_color_0x39BC >> 16) & 0xFF;
    plw->field_0x25C[2] = (user->inner_color_0x39BC >> 8) & 0xFF;
    plw->field_0x25C[3] = user->inner_color_0x39BC;
    pl_equip_models_set(plw, (u8*)&user->weapon_0x0020, (u8*)&user->parts_0x002C[0], (u8*)&user->parts_0x002C[1]);
    pl_equip_model_set(plw, (u8*)&user->armour_0x0044[0]);
    pl_equip_model_set(plw, (u8*)&user->armour_0x0044[4]);
    pl_equip_model_set(plw, (u8*)&user->armour_0x0044[1]);
    pl_equip_model_set(plw, (u8*)&user->armour_0x0044[2]);
    pl_equip_model_set(plw, (u8*)&user->armour_0x0044[3]);
    pl_equip_model_set(plw, (u8*)&user->armour_0x0044[5]);
}

/* Copies each worn armour piece back from its equipment-box record, then dresses the player work. */
extern "C" void equip_worn_records_refresh(_PLW* plw, Q_UserData* user)
{
    equip_record_copy(&user->armour_0x0044[3], &user->equip_box_0x0E00[user->box_armour_0x0092[3]]);
    equip_record_copy(&user->armour_0x0044[0], &user->equip_box_0x0E00[user->box_armour_0x0092[0]]);
    equip_record_copy(&user->armour_0x0044[4], &user->equip_box_0x0E00[user->box_armour_0x0092[4]]);
    equip_record_copy(&user->armour_0x0044[1], &user->equip_box_0x0E00[user->box_armour_0x0092[1]]);
    equip_record_copy(&user->armour_0x0044[2], &user->equip_box_0x0E00[user->box_armour_0x0092[2]]);
    equip_record_copy(&user->armour_0x0044[5], &user->equip_box_0x0E00[user->box_armour_0x0092[5]]);
    userdata_pl_look_apply(plw, user);
}

/* The first empty equipment-box record, or 0xFFFF when the box is full. */
extern "C" u16 userdata_equip_box_free_index(void)
{
    Q_UserData* user = userdata_block();
    u16 cap = userdata_box_capacity(user);
    u16 i;

    for (i = 0; i < cap; i++) {
        if (user->equip_box_0x0E00[i].kind == 0) {
            return i;
        }
    }
    return 0xFFFF;
}

/* How many equipment-box records are empty. */
extern "C" u16 userdata_equip_box_free_count(void)
{
    Q_UserData* user = userdata_block();
    u16 cap = userdata_box_capacity(user);
    u16 i;
    u16 free = 0;

    for (i = 0; i < cap; i++) {
        if (user->equip_box_0x0E00[i].kind == 0) {
            free++;
        }
    }
    return free;
}

/* Equipment-box record `index`, or NULL past the box's capacity. */
extern "C" _EQUIP* userdata_equip_box_rec_get(u16 index)
{
    if (index >= userdata_box_capacity(userdata_block())) {
        return NULL;
    }
    return &userdata_block()->equip_box_0x0E00[index];
}

/* How many equipment-box records hold item `item_id` of `kind`. */
extern "C" u16 userdata_equip_box_count(u8 kind, u16 item_id)
{
    _EQUIP* rec = userdata_block()->equip_box_0x0E00;
    u16 cap = userdata_box_capacity(userdata_block());
    u16 count = 0;
    s32 i;

    for (i = 0; i < cap; i++, rec++) {
        if (rec->kind == kind && rec->item_id == item_id) {
            count++;
        }
    }
    return count;
}

/* Registers a record newly filed in the box: its "new" flag, the progress flags its kind, colour tier and dye raise,
 * and a dyeable piece's colour set to its colour-table entry. */
extern "C" void equip_record_register(_EQUIP* rec)
{
    GXColor color;
    s32 rgba;

    userdata_equip_rec_flag_set(rec, 0);
    if (rec->kind == 14) {
        userdata_progress_flag_set(14);
    } else if (rec->kind == 15) {
        userdata_progress_flag_set(15);
    }
    if (equip_color_tier_get(rec) != 0) {
        userdata_progress_flag_set(17);
    }
    if ((u8)equip_kind_table_class(rec->kind) == 0 && EnableChangeColor(rec) == 1) {
        userdata_progress_flag_set(16);
        rgba = equip_color_rgba_get(equip_color_index_get(rec));
        color_rgba_copy((u8*)&color, (const u8*)&rgba);
        equip_color_set(rec, ((color.r & 0xF8) << 8) | ((color.g & 0xFC) << 3) | ((color.b & 0xF8) >> 3));
    }
}

/* Copies one 4-byte RGBA colour byte by byte. */
/* free: retail C linkage evidenced (unmangled map name) */
extern "C" void color_rgba_copy(u8* dst, const u8* src)
{
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
    dst[3] = src[3];
}

/* Files a new record of `kind`/`item_id` in the first empty equipment-box record; its index, or 0xFFFF when the box
 * is full. */
extern "C" u16 userdata_equip_box_add_new(Q_UserData* user, u8 kind, u16 item_id)
{
    u16 cap = userdata_box_capacity(user);
    _EQUIP* rec;
    u16 i;

    for (i = 0; i < cap; i++) {
        if (user->equip_box_0x0E00[i].kind == 0) {
            rec = &user->equip_box_0x0E00[i];
            memset(rec, 0, 0xC);
            user->equip_box_0x0E00[i].kind = kind;
            user->equip_box_0x0E00[i].item_id = item_id;
            equip_record_register(rec);
            return i;
        }
    }
    return 0xFFFF;
}

/* Files a copy of `equip` in the first empty equipment-box record; its index, or 0xFFFF when the box is full. */
extern "C" u16 userdata_equip_box_add(Q_UserData* user, const _EQUIP* equip)
{
    _EQUIP* rec;
    u16 cap = userdata_box_capacity(user);
    u16 i;

    for (i = 0; i < cap; i++) {
        rec = &user->equip_box_0x0E00[i];
        if (rec->kind == 0) {
            equip_record_copy(rec, equip);
            equip_record_register(rec);
            return i;
        }
    }
    return 0xFFFF;
}

/* Overwrites equipment-box record `index` with a copy of `equip` and registers it. */
extern "C" void userdata_equip_box_set(Q_UserData* user, u16 index, const _EQUIP* equip)
{
    _EQUIP* rec;

    if (index < userdata_box_capacity(user)) {
        rec = &user->equip_box_0x0E00[index];
        equip_record_copy(rec, equip);
        userdata_equip_rec_flag_set(rec, 0);
        equip_record_register(rec);
    }
}

/* Discards equipment-box record `index` unless it is worn, dropping it from every saved set; 1 when discarded. */
extern "C" s32 equip_record_discard(Q_UserData* user, u16 index)
{
    u8 i;

    if (userdata_equip_box_worn_ck(user, index) == 0) {
        memset(&user->equip_box_0x0E00[index], 0, 0xC);
        for (i = 0; i < 20; i++) {
            if (my_set_box_used_ck(user, index, i) == 1) {
                my_set_clear(user, i);
            }
        }
        for (i = 0; i < 20; i++) {
            if (gun_set_box_used_ck(user, index, i) == 1) {
                gun_set_clear(user, i);
            }
        }
        return 1;
    }
    return 0;
}

/* Whether equipment-box record `index` is worn (any armour piece, the weapon or a bowgun part). */
extern "C" s32 userdata_equip_box_worn_ck(Q_UserData* user, u16 index)
{
    if (user->box_armour_0x0092[0] == index) {
        return 1;
    }
    if (user->box_armour_0x0092[1] == index) {
        return 1;
    }
    if (user->box_armour_0x0092[2] == index) {
        return 1;
    }
    if (user->box_armour_0x0092[3] == index) {
        return 1;
    }
    if (user->box_armour_0x0092[4] == index) {
        return 1;
    }
    if (user->box_armour_0x0092[5] == index) {
        return 1;
    }
    if (user->box_weapon_0x008C == index) {
        return 1;
    }
    if (user->box_parts_0x008E[0] == index) {
        return 1;
    }
    return user->box_parts_0x008E[1] == index;
}

/* Dyes worn armour piece `kind` (1-5) and its equipment-box record: `color` as given, or the piece's own colour-table
 * entry when `use_table` is 1. */
extern "C" void equip_worn_color_set(Q_UserData* user, u8 kind, u16 color, u8 use_table)
{
    GXColor rgba_color;
    s32 rgba;
    u16 index;
    _EQUIP* rec;

    switch (kind) {
    case 1:
        index = user->box_armour_0x0092[0];
        rec = &user->armour_0x0044[0];
        break;
    case 2:
        index = user->box_armour_0x0092[1];
        rec = &user->armour_0x0044[1];
        break;
    case 3:
        index = user->box_armour_0x0092[2];
        rec = &user->armour_0x0044[2];
        break;
    case 4:
        index = user->box_armour_0x0092[3];
        rec = &user->armour_0x0044[3];
        break;
    case 5:
        index = user->box_armour_0x0092[4];
        rec = &user->armour_0x0044[4];
        break;
    default:
        return;
    }
    if (EnableChangeColor(rec) == 1) {
        if (use_table == 1) {
            rgba = equip_color_rgba_get(equip_color_index_get(rec));
            color_rgba_copy((u8*)&rgba_color, (const u8*)&rgba);
            color = ((rgba_color.r & 0xF8) << 8) | ((rgba_color.g & 0xFC) << 3) | ((rgba_color.b & 0xF8) >> 3);
        }
        rec->color_rgb565 = color;
        equip_color_set(&user->equip_box_0x0E00[index], color);
    }
}

/* --- the unit's own .bss / .sbss (`splits.txt` `.bss 0x806699B8..0x80669F68`, `.sbss 0x80794878..0x80794884`) ---- */

SubTrans sub_trans_buf[0x20];          /* 0x806699B8: the sub-transfer entries `subTransSetPrio` fills */
SubTransState sub_trans_state;         /* 0x80669F38: the priority lists */
BgTexDispHook bg_tex_disp_func;        /* 0x80794878 */
u8* lobby_world_block;                 /* 0x80794880: the cached save-block pointer */
