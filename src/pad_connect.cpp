/*
 * pad_connect.cpp - the game's boot and per-frame driver (the system init and the main-loop frame `main.cpp` runs), the
 *   soft-reset and pad-hold flags, and the split-screen state: the two players' viewports and the scene-camera bank.
 * RANGE. .text 0x80046C80-0x80047398 (20 functions, address order: game_system_init, game_frame_run, screen_ortho_set,
 *   g3d_reset_on_request, setSoftresetFlag, pad_hold_set, pad_hold_clear, pad_hold_ck, game_init_hooks_clear,
 *   pad_connect_reset, screen_split_mode_ck, pad_connect_disp_set, screen_split_view_get, split_camera_set, word_read,
 *   word_copy_return_dst, word_copy, screen_split_view_set, player_pad_channel_get, scene_camera_get),
 *   extab 0x80006A48-0x80006A90, extabindex 0x8001E8DC-0x8001E948, .data 0x80581088-0x80581158
 *   (split_camera_view_table), .bss 0x806694D0-0x806694E8 (game_mutex), .sdata 0x80790EE0-0x80790EE8 (the "ttt"
 *   literal), .sdata2 0x80795B58-0x80795B70 (the float pool).
 * FLAGS. `cflags_main` (-O3, -inline noauto, C++ exceptions on); `#pragma peephole off` for the file (retail builds
 *   `&system_w` as `lis r3` + `addi r31,r3` in game_system_init/game_frame_run, which the peephole pass folds into r31).
 * NAMES. Class 4 (no `__FILE__` string covers the range; the runtime dump has only placeholders).  GUESS (from each
 *   body and its callers): game_system_init (the boot init `main.cpp` calls once: mutex, memory layout, pads, draw and
 *   save systems), game_frame_run (the main loop's frame: strap task start, then the calc and draw passes),
 *   screen_ortho_set (640x480 orthographic projection, identity position matrix), g3d_reset_on_request,
 *   game_init_hooks_clear (clears the hook pointers and calls the face/menu entry points with neutral arguments),
 *   split_camera_set (copies one of the four split viewports into the six cameras of a bank), screen_split_view_set,
 *   split_camera_view_table, pad_hold_set/pad_hold_clear/pad_hold_ck, word_read/word_copy/word_copy_return_dst,
 *   player_pad_channel_get, scene_camera_get, and the foreign callees system_clear_color_set (0x800CFBA0) and
 *   display_state_hook_call (0x800D2928).  `pad_connect_disp_set` sets the split-screen mode and `pad_connect_reset`
 *   resets the split-screen state: both rows keep their map names (other units call them).
 * RESIDUALS. none: every row measures 100 and `.data`/`.sdata`/`.sdata2` are byte-identical (the shorter `.sdata`/`.sdata2`
 *   are the trailing 8-byte fill the link adds).  system_clear_color_set/display_state_hook_call are registry names: the
 *   unit flips once they are in the map.
 * SHAPES. game_frame_run tests `(u32)(state - 1) > 1` first (retail's subi/cmplwi with the strap-task branch inline);
 *   screen_ortho_set names its float arguments as locals in retail's pool order (0, 1, 480, 640); the split index is
 *   `(view << 1) + bank` (`* 2` swaps the add operands) and split_camera_set reads the table by index in the loop
 *   (a record pointer colours r29/r30 the other way round).
 */

#include "types.h"
#include "MTX/mtx.h"                 /* Mtx, PSMTXIdentity; before `ef.h`, which then skips its `Vec` */
#include "MTX/mtx44.h"
#include "pad_connect.h"
#include "word_read.h"
#include "pad_hold_set.h"
#include "pad_hold_ck.h"
#include "screen_split_view_get.h"
#include "player_pad_channel_get.h"
#include "scene_camera_get.h"
#include "mh3_pad.h"                 /* task_slot_table_*, pad_*, setSoftresetFlag, word_copy_return_dst (rule 2) */
#include "mh3_pad/system_w.h"        /* SystemWork / system_w (rule 1/2) */
#include "mh3_pad/Screen_w.h"        /* ScreenWork / Screen_w (rule 1/2) */
#include "mh3_pad/tsk_start.h"       /* tsk_start (rule 2) */
#include "strap_task.h"              /* strap_task (rule 2) */
#include "g3d/g3d_init.h"            /* fn_8007A510 (rule 2) */
#include "g3d/g3d_camera_types.h"    /* nw4r::g3d::Camera, CameraData (rule 1) */
#include "g3d/g3d_scnroot.h"         /* nw4r::g3d::ScnRoot::GetCamera (rule 2) */
#include "ef/pRoot.h"                /* pRoot (rule 2) */
#include "ef/memory_layout_init.h"   /* memory_layout_init (rule 2) */
#include "ef/system_core.h"          /* quest_system_reset, hbm_*, loading_disp_draw (rule 2) */
#include "ef/system_clear_color_set.h"
#include "ef/display_state_hook_call.h"
#include "ef/fn_800CDB2C.h"          /* my_player_no (rule 2) */
#include "fn_80047398.h"             /* face_buf_init, face_select_on, face_tex_reload (rule 2) */
#include "fn_80056F24.h"             /* fade_*, game_draw_*, screen_copy_tex_capture (rule 2) */
#include "userdata_item.h"           /* frame_draw, userdata_init, bg_tex_disp_func (rule 2) */
#include "face_work_init.h"          /* face_* (rule 2) */
#include "face_tex_request.h"        /* face_tex_request (rule 2) */
#include "draw_shape.h"              /* set_arena_idx (rule 2) */
#include "sound/snd_bank_loader_init.h"
#include "sound/se_req_frame_set_sp.h"
#include "Network/network_pat_control.h" /* initWorkRecord, runNetworkFrame (rule 2) */
#include "menu/movie.h"              /* dvd_error_* (rule 2) */
#include "menu/kbd_hooks_set.h"
#include "stage/get_stg_w.h"
#include "stage/fn_802B2AA0.h"        /* StageRuntime (rule 1) */
#include "Pl/set_demo_equip_data_func.h"
#include "font/flfnt.h"              /* flfntStackReset, flfntFlush (rule 2) */
#include "main.h"                    /* ck_WideMode (rule 2) */
#include "MSL_C/printf.h"
#include "MSL_C/extras.h"            /* strupr (rule 2) */
#include "OS/OSMutex.h"
#include "SC/SCApi.h"
#include "WPAD/wpad.h"
#include "RVLGX/GXTransform.h"

#pragma peephole off

/* One split-screen viewport: the scissor box, the viewport, the orthographic bounds and the aspect ratio
 * `split_camera_set` copies into a bank's cameras.  size: 0x34 */
typedef struct SplitCameraView {
    /* +0x00 */ s32 scissor_x;
    /* +0x04 */ s32 scissor_y;
    /* +0x08 */ s32 scissor_w;
    /* +0x0C */ s32 scissor_h;
    /* +0x10 */ f32 viewport_x;
    /* +0x14 */ f32 viewport_y;
    /* +0x18 */ f32 viewport_w;
    /* +0x1C */ f32 viewport_h;
    /* +0x20 */ f32 top;
    /* +0x24 */ f32 bottom;
    /* +0x28 */ f32 left;
    /* +0x2C */ f32 right;
    /* +0x30 */ f32 aspect;
} SplitCameraView;

/* The four split viewports, indexed `view * 2 + bank`: top and bottom halves (view 0), left and right halves
 * (view 1). */
SplitCameraView split_camera_view_table[4] = {
    { 0, 0, 640, 224, 0.0f, 0.0f, 640.0f, 224.0f, 0.0f, 224.0f, 0.0f, 640.0f, 2.6666667f },
    { 0, 225, 640, 224, 0.0f, 225.0f, 640.0f, 224.0f, 225.0f, 448.0f, 0.0f, 640.0f, 2.6666667f },
    { 0, 0, 320, 448, 0.0f, 0.0f, 320.0f, 448.0f, 0.0f, 448.0f, 0.0f, 320.0f, 0.6666667f },
    { 321, 0, 320, 448, 321.0f, 0.0f, 320.0f, 448.0f, 0.0f, 448.0f, 321.0f, 640.0f, 0.6666667f },
};

/* 0x80046C80 (0xB4): Initialises the game systems at boot. */
extern "C" void game_system_init(void)
{
    u32 color;

    OSInitMutex(game_mutex);
    memory_layout_init();
    quest_system_reset();
    pad_wpad_init();
    system_w.sound_mode_0x03 = SCGetSoundMode();
    system_w.language_0x01 = SCGetLanguage();
    task_slot_table_clear();
    game_draw_init();
    face_buf_init();
    fade_work_clear();
    userdata_init();
    color = 0;
    system_clear_color_set((u8*)&color);
    hbm_work_reset();
    system_w.field_0x22 = 0;
    system_w.print_mode_0x23 = 0;
    system_w.system_halt_0x871 = 0;
    system_w.vs_load_player_0x7e8 = 0xFF;
    system_w.vs_load_slot_0x7e9 = 0xFF;
    snd_bank_loader_init();
    face_work_init();
    initWorkRecord();
    dvd_error_init();
    game_init_hooks_clear();
    kbd_hooks_set();
}

/* 0x80046D34 (0x100): Runs one frame of the main loop: the boot strap task first, then the calc and draw passes. */
extern "C" s32 game_frame_run(void)
{
    WPADDisconnect(2);
    WPADDisconnect(3);
    pad_frame_read();
    system_w.print_mode_0x23 = 1;
    if ((u32)(system_w.field_0x22 - 1) > 1) {
        if (system_w.field_0x22 == 0) {
            tsk_start(strap_task, 0);
            system_w.field_0x22 = 1;
        }
    } else {
        flfntStackReset();
        g3d_reset_on_request();
        screen_ortho_set();
        dvd_error_step();
        runNetworkFrame();
        system_w.print_mode_0x23 = 2;
        task_slot_table_run();
        game_draw_calc();
        hbm_control();
        pad_motor_tick();
        system_w.print_mode_0x23 = 3;
        frame_draw();
        flfntFlush();
        fade_draw_screen();
        fade_update();
        if (system_w.hbm_pause_0x872 != 0) {
            system_w.hbm_pause_0x872 = 0;
            screen_copy_tex_capture();
        }
        loading_disp_draw();
        dvd_error_draw();
        hbm_draw();
        display_state_hook_call();
    }
    system_w.print_mode_0x23 = 4;
    return 1;
}

/* 0x80046E34 (0x64): Loads a 640x480 orthographic projection and an identity position matrix. */
extern "C" void screen_ortho_set(void)
{
    Mtx44 proj;
    Mtx mv;
    f32 zero = 0.0f;
    f32 one = 1.0f;
    f32 height = 480.0f;
    f32 width = 640.0f;

    C_MTXOrtho(proj, zero, height, zero, width, zero, one);
    GXSetProjection(proj, 1);
    PSMTXIdentity(mv);
    GXLoadPosMtxImm(mv, 0);
    GXSetCurrentMtx(0);
}

/* 0x80046E98 (0x1C): Resets the G3D state while the system work requests it. */
extern "C" void g3d_reset_on_request(void)
{
    if (system_w.field_0x21 != 0) {
        fn_8007A510();
    }
}

/* 0x80046EB4 (0x30): Clears the soft-reset block flag when `flag` is set, raises it otherwise. */
void setSoftresetFlag(bool flag)
{
    if (flag == true) {
        system_w.field_0x20 = 0;
    } else {
        system_w.field_0x20 = 1;
    }
}

/* 0x80046EE4 (0x14): Raises the pad-hold flag. */
extern "C" void pad_hold_set(void)
{
    system_w.field_0x30 = 1;
}

/* 0x80046EF8 (0x14): Clears the pad-hold flag. */
extern "C" void pad_hold_clear(void)
{
    system_w.field_0x30 = 0;
}

/* 0x80046F0C (0x1C): Returns 1 while the pad-hold flag is raised. */
extern "C" u32 pad_hold_ck(void)
{
    return system_w.field_0x30 != 0;
}

/* 0x80046F28 (0xF8): Clears the stage and hook pointers and calls the face and menu entry points with neutral
 * arguments. */
extern "C" void game_init_hooks_clear(void)
{
    char name[16];
    StageRuntime* stage;

    sprintf(name, "ttt");
    strupr(name);
    stage = (StageRuntime*)get_stg_w();
    stage->map_group_direct = 0;
    stage->map_group_remapped = 0;
    bg_tex_disp_func.func = 0;
    task_func[0] = 0;
    task_func[1] = 0;
    set_demo_equip_data_func = 0;
    se_req_frame_set_sp(0, 0, 0, -1, -1, 2);
    face_eye_get(0);
    face_type_get(0);
    face_type_next(0);
    face_type_prev(0);
    face_makeup_set(0, 0, 0);
    face_tex_request(0, 0, 0, 0, 0xFF);
    face_select_on(0, 0xFF);
    face_tex_reload(0xFF, 0);
    face_draw_on(0);
    face_draw_off(0);
    set_menu_key_ad_flag(0, false);
    set_arena_idx(0);
}

/* 0x80047020 (0x38): Resets the split-screen state: single screen, no view, camera banks 8 and 16, 4:3 aspect. */
extern "C" void pad_connect_reset(void)
{
    Screen_w.split_mode = 0;
    Screen_w.split_view_no = 0xFF;
    Screen_w.camera_view.player_camera_0x50[0] = 8;
    Screen_w.camera_view.player_camera_0x50[1] = 16;
    Screen_w.player_aspect[0] = Screen_w.player_aspect[1] = 1.3333334f;
}

/* 0x80047058 (0x1C): Returns 1 while the screen is split. */
extern "C" s32 screen_split_mode_ck(void)
{
    return Screen_w.split_mode != 0;
}

/* 0x80047074 (0x40): Turns the split screen on (selecting view 0 when none is set) or off. */
extern "C" void pad_connect_disp_set(u32 on)
{
    if (on == 1) {
        Screen_w.split_mode = 1;
        if (Screen_w.split_view_no == 0xFF) {
            screen_split_view_set(0);
        }
    } else {
        Screen_w.split_mode = 0;
    }
}

/* 0x800470B4 (0x28): Returns the split view, or -1 on a single screen. */
extern "C" s8 screen_split_view_get(void)
{
    if (Screen_w.split_mode != 0) {
        return (s8)Screen_w.split_view_no;
    }
    return -1;
}

/* 0x800470DC (0x158): Copies split viewport `view` of player `bank` into that bank's six scene cameras. */
extern "C" void split_camera_set(u8 bank, u8 view)
{
    u8 camera_no;
    u8 index;
    f32 aspect;
    s32 i;

    camera_no = (bank == 0) ? 8 : 16;
    index = (view << 1) + bank;
    aspect = split_camera_view_table[index].aspect;
    if ((u32)ck_WideMode() == 1) {
        aspect *= 1.3333334f;
    }
    Screen_w.player_aspect[0] = Screen_w.player_aspect[1] = aspect;
    for (i = 0; i < 6; i++) {
        nw4r::g3d::Camera camera;
        s32 handle;
        CameraData* data;

        handle = ((nw4r::g3d::ScnRoot*)pRoot)->GetCamera(camera_no);
        word_copy_return_dst(&camera, &handle);
        data = (CameraData*)word_read((s32*)&camera);
        data->mScissorX = split_camera_view_table[index].scissor_x;
        data->mScissorY = split_camera_view_table[index].scissor_y;
        data->mScissorW = split_camera_view_table[index].scissor_w;
        data->mScissorH = split_camera_view_table[index].scissor_h;
        data->mViewportX = split_camera_view_table[index].viewport_x;
        data->mViewportY = split_camera_view_table[index].viewport_y;
        data->mViewportW = split_camera_view_table[index].viewport_w;
        data->mViewportH = split_camera_view_table[index].viewport_h;
        data->mUnkBC = split_camera_view_table[index].top;
        data->mUnkC0 = split_camera_view_table[index].bottom;
        data->mUnkC4 = split_camera_view_table[index].left;
        data->mUnkC8 = split_camera_view_table[index].right;
        data->mProjB = Screen_w.player_aspect[view];
        camera_no++;
    }
}

/* 0x80047234 (0x8): Returns the word at `src`. */
extern "C" s32 word_read(s32* src)
{
    return *src;
}

/* 0x8004723C (0x30): Copies the word at `src` into `dst` and returns `dst`. */
extern "C" void* word_copy_return_dst(void* dst, const void* src)
{
    word_copy((s32*)dst, (s32*)src);
    return dst;
}

/* 0x8004726C (0xC): Copies the word at `src` into `dst`. */
extern "C" void word_copy(s32* dst, s32* src)
{
    *dst = *src;
}

/* 0x80047278 (0x64): Selects split view `view` and lays both players' cameras out for it. */
extern "C" void screen_split_view_set(u8 view)
{
    if (pRoot != 0 && Screen_w.split_view_no != view) {
        Screen_w.split_view_no = view;
        split_camera_set(0, view);
        split_camera_set(1, view);
    }
}

/* 0x800472DC (0x3C): Returns the pad channel of player `player` in the current split view. */
extern "C" u8 player_pad_channel_get(u8 player)
{
    u8 view = screen_split_view_get();
    return (u8)((view << 1) + player);
}

/* 0x80047318 (0x80): Returns the scene camera `index` of the local player's bank (the plain index on a single
 * screen). */
extern "C" s32 scene_camera_get(u8 index)
{
    if (Screen_w.split_mode != 0) {
        if ((s8)my_player_no() == 0) {
            return ((nw4r::g3d::ScnRoot*)pRoot)->GetCamera(index + 8);
        }
        return ((nw4r::g3d::ScnRoot*)pRoot)->GetCamera(index + 16);
    }
    return ((nw4r::g3d::ScnRoot*)pRoot)->GetCamera(index);
}

/* .bss 0x806694D0: the game's OS mutex (`game_system_init` initialises it). */
u8 game_mutex[0x18];
