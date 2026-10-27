/*
 * fn_80059550.cpp - the boot strap screen (the health-and-safety notice) and the boot save-data check task.
 * RANGE. .text 0x80059550-0x8005AA28 (12 functions: strap_screen_draw, strap_task, save_msg_window_draw, fn_8005A63C,
 *   save_msg_text_draw, then the nw4r `ResMat`/`ResMatChan` accessor copies), extab 0x800072D0-0x80007310,
 *   extabindex 0x8001F5A8-0x8001F608, .data 0x8058B220-0x8058B350 (strap_task's three jump tables, four s16 layout
 *   tables, the accessor strings), .sdata 0x80791110-0x80791124, .sbss 0x807948C8-0x807948D8,
 *   .sdata2 0x80795CA0-0x80795CF0.
 * FLAGS. `cflags_main`; `#pragma pool_data off` (each table address is its own `lis`/`addi`); `#pragma peephole off`
 *   around strap_task, save_msg_window_draw and the ResMatChan constructor (retail keeps `extsh`/`rlwinm` + `cmpwi`).
 * NAMES. Class 4 (no `__FILE__` string; the runtime dump has only `zz_` placeholders).  GUESS: strap_task (the task
 *   body `pad_connect.cpp`'s registrar starts: strap fade in, wait for a button or 600 frames, fade out, then the save
 *   check/creation, the option load and the arena task), strap_screen_draw, save_msg_window_draw, save_msg_text_draw,
 *   res_common_copy, res_mat_chan_copy_ctor, strap_fade_color, save_msg_font_size, save_error_code,
 *   save_sys_file_read, save_data_file_index, save_msg_blink_count, save_msg_blink_spare, save_msg_tex_rect,
 *   save_msg_size, save_msg_button_pos, save_msg_text_pos, save_error_enter, return_to_data_manager.
 *   The file keeps the map's stem; a unit-rename to `strap_task.cpp` is requested.
 * RESIDUALS. strap_task: retail jumps from every failing save step into one shared save-error block that falls into
 *   sub-state 2's body; written with `goto` (rule 8 forbids it) the function measures 100.00, while this source
 *   re-dispatches through `for (;;)` + `continue` and measures 83.38 - the loop makes MWCC hoist its constants into
 *   r22-r31 (`_savegpr_22`, 0xDF0 B against 0xD88 B) and each failing step keeps its own copy of the entry stores.
 *   save_msg_text_draw: retail's button switch leaves a dead `b` after each case (an inlined return threaded into
 *   the caller) and keeps `scr.x` in f3; this source keeps the plain switch.
 *   save_msg_window_draw: retail sign-extends each `base + offset` (`add` + `extsh`) where MWCC folds the `extsh`.
 *   fn_8005A63C keeps its stem: `ef/ef_util.cpp` (Matching) spells it.
 *   Data: `.sdata2`/`.sbss` are byte-identical; `.data` is 0x11A against 0x130 and `.sdata` 0x10 against 0x14 because
 *   retail emits the accessors' strings as the weak inline tail (ref's then the constructor's, "g3d_resmat_ac.h"
 *   twice, "ref" 8-aligned) while this source defines the accessors out of line, so `-str reuse` merges the file
 *   name (playbook 163/174: `inline` accessors under `dont_inline` plus the dead-stripped referencer).  That model's
 *   copy order (copy constructor, copy, GetResMatChan, ResMatChan constructor, ResCommon constructor, ref, ptr,
 *   GetClassName, IsValid) puts 0x8005AA28-0x8005AA44 in this TU: the seam with `g3d/fn_8005AA28` is likely
 *   0x8005AA44.
 * SHAPES. The s16 tables are file-scope globals defined after strap_task (their `.data` follows its jump tables);
 *   "%s" and "ref" are `.sdata` literals; each failing save step's result is a `switch` (`case 1` first, `-2`/`-1`
 *   after: the `addi`/`cmplwi 1` range test).
 */

#include "types.h"
#include "gx.h"
#include "nw4r/math.h"
#include "ef.h"               /* Vec / nw4r::db::Panic */
#include "main.h"             /* get_ScreenSize / ck_WideMode / _MH_VEC2 (rule 2) */
#include "fn_80047398.h"      /* drawSpr2TF / subTransSet / fltSpr2TF (rule 2) */
#include "fn_8004CAD8.h"      /* wii_sysmsg_gen (rule 2) */
#include "g3d/g3d_resmat.h"   /* nw4r::g3d::ResMat (rule 2) */
#include "ef/fn_800CDB2C.h"   /* sys_tex_obj_get (rule 2) */
#include "ef/load_queue_reset.h" /* load_queue_reset / load_queue_task / rand_ring_seed / stream_bounds_arm (rule 2) */
#include "ef/resident_file_load.h" /* resident_file_load / work_stack_reset (rule 2) */
#include "menu/sysmsg_text.h" /* sysmsg_save_text, sysmsg_yes_text, sysmsg_manager_text (rule 2) */
#include "menu/menu_plsearch.h" /* the save steps (rule 2) */
#include "menu/get_pop_dat_ptr.h" /* option_cfg_load (rule 2) */
#include "mh3_pad.h"          /* setSoftresetFlag / arena_task_start / get_ControlType / TaskSlot (rule 2) */
#include "mh3_pad/tsk_start.h" /* tsk_start / tsk_timer_clear / strap_tpl_load (rule 2) */
#include "mh3_pad/system_w.h" /* system_w (rule 2) */
#include "mh3_pad/Psw.h"      /* Psw (rule 2) */
#include "mh3_pad/option_w.h" /* option_w (rule 2) */
#include "ef/tex_palette_bind.h" /* tex_palette_bind (rule 2) */
#include "font/flfnt_frame_reset.h" /* flfnt_frame_reset (rule 2) */
#include "draw_shape.h"       /* draw_shape_reset (rule 2) */
#include "VI/vi.h"
#include "OS/OSReset.h"
#include "Runtime.PPCEABI.H/memcpy.h"

#pragma pool_data off

/* _GXTexObj is an SDK type with no shared header yet; forward-declared for the mangled spelling. */
struct _GXTexObj;

/* The sprite rectangle's 2-float position, `disp_beta_tex`'s second parameter. size: 0x08 */
typedef struct Vec2 {
    /* +0x00 */ f32 x;
    /* +0x04 */ f32 y;
} Vec2; /* size: 0x08 */

/* The NW4R-`ResMat`-style validity-checked handle the `res_mat_chan_copy_ctor` pair copies. size: 0x04 */
typedef struct ResHandle {
    /* +0x00 */ u32 mPtr;
} ResHandle; /* size: 0x04 */

namespace nw4r {
namespace math {
/* `nw4r::math::CosFIdx(f32)` - the table cosine fn_8005A63C wraps. */
f32 CosFIdx(f32 idx);
} // namespace math
} // namespace nw4r

/* The message-box draw helper whose map name is a C++ mangling, declared with C++ linkage. */
void disp_beta_tex(Vec* a, Vec2* b, _GXColor color, _GXTexObj* c);
void font_print_ex(s16 x, s16 y, s16 flag, signed char* fmt, ...);
void font_set_size(s16 w, s16 h);

extern "C" {

void strap_screen_draw(u32 count, TaskSlot** args);
void save_msg_window_draw(s32 id, s32 flag);
f32 fn_8005A63C(f32 x);
void save_msg_text_draw(u32 count, s32* args);

/* ---------------------------------------------------------------------------------------------------
 * The unit's data.
 * ------------------------------------------------------------------------------------------------- */

/* The strap texture's colour; its alpha is the fade. */
_GXColor strap_fade_color = {0xFF, 0xFF, 0xFF, 0x00};
/* The message font's base size. */
s16 save_msg_font_size = 22;

/* The last save error code (`save_error_get`), 0 while no error is pending. */
s32 save_error_code;
/* 1 once the system file was read at boot. */
u8 save_sys_file_read;
/* The data file `strap_task` is creating (0..5). */
u8 save_data_file_index;
/* The message box's blink phase (0..0x7FFF). */
u32 save_msg_blink_count;
/* The word after the blink phase; nothing in the unit reads it. */
u32 save_msg_blink_spare;

/* 0x80059550 (0x94): draws the strap texture page `task->page_0x0E` (a `subTransSet` callback). */
void strap_screen_draw(u32 count, TaskSlot** args) {
    TaskSlot* task = *args;
    Vec2 pos;
    Vec size;
    pos.x = 854.0f;
    pos.y = 480.0f;
    if ((u32)ck_WideMode() == 1) {
        size.z = 0.0f;
        size.y = 0.0f;
        size.x = 0.0f;
    } else {
        size.x = -107.0f;
        size.z = 0.0f;
        size.y = 0.0f;
    }
    disp_beta_tex(&size, &pos, strap_fade_color, (_GXTexObj*)sys_tex_obj_get(task->page_0x0E + 5));
}

/* Enters the save-error sub-state: the error message then waits for a button. */
static inline void save_error_enter(TaskSlot* task) {
    system_w.field_0x865 = 0;
    task->wait_0x0C = 30;
    task->sub_0x09 = 2;
    task->step_0x0A = 0;
    save_msg_blink_count = 0;
}

/* Blacks the screen out and returns to the Wii data management screen. */
static inline void return_to_data_manager(void) {
    VISetBlack(1);
    VIFlush();
    VIWaitForRetrace();
    OSReturnToDataManager();
}

/* 0x800595E4 (0xD88): the boot task: fades the strap screen in and out, checks and creates the save data, then loads
 * the options and starts the arena task. */
#pragma peephole off
void strap_task(TaskSlot* task) {
    u32 args[2];
    s32 result;

    args[0] = (u32)task;
    switch (task->step_0x08) {
    case 0:
        load_queue_reset();
        tsk_start((void (*)(TaskSlot*))load_queue_task, 15);
        task->step_0x08++;
        task->sub_0x09 = 0;
        task->step_0x0A = 0;
        setSoftresetFlag(0);
        system_w.strap_active_0x2b = 1;
        system_w.field_0x31 = 1;
        flfnt_frame_reset();
        strap_tpl_load();
        tex_palette_bind((struct TPLPalette*)0x90F49000, 5);
        tex_palette_bind((struct TPLPalette*)0x90001000, 255);
        system_w.reset_func_0x8c4();
        tex_palette_bind((struct TPLPalette*)0x90001660, 253);
        break;
    case 1:
        rand_ring_seed(16);
        task->step_0x08++;
        task->wait_0x0C = 15;
        task->page_0x0E = 0;
        task->clear_0x10 = 0;
        break;
    case 2: {
        f32 alpha = 0.0f;
        alpha = 255.0f * (1.0f - (f32)task->wait_0x0C / 15.0f);
        if (alpha < 0.0f) {
            alpha = 0.0f;
        }
        if (alpha > 255.0f) {
            alpha = 255.0f;
        }
        strap_fade_color.a = alpha;
        subTransSet((u32)strap_screen_draw, 1, args);
        if (--task->wait_0x0C == 0) {
            task->step_0x08++;
        }
        break;
    }
    case 3: {
        subTransSet((u32)strap_screen_draw, 1, args);
        if (task->wait_0x0C > 30) {
            u16 trig = Psw[0].field_0xce & 0x3FF0;
            u16 cl_trig = 0;
            if ((u8)get_ControlType(0) == 2) {
                cl_trig = Psw[0].field_0xda & 0xFFFC;
            }
            if (trig != 0 || cl_trig != 0) {
                task->step_0x08++;
                task->wait_0x0C = 0;
                break;
            }
            if (task->wait_0x0C > 600) {
                task->step_0x08++;
                task->wait_0x0C = 0;
                break;
            }
        }
        s16 wait = ++task->wait_0x0C;
        if (wait < 600 && wait % 150 == 0) {
            task->page_0x0E ^= 1;
        }
        break;
    }
    case 4: {
        f32 alpha = 0.0f;
        alpha = 255.0f * (1.0f - (f32)task->wait_0x0C / 15.0f);
        if (alpha < 0.0f) {
            alpha = 0.0f;
        }
        if (alpha > 255.0f) {
            alpha = 255.0f;
        }
        strap_fade_color.a = alpha;
        subTransSet((u32)strap_screen_draw, 1, args);
        if (++task->wait_0x0C > 20) {
            system_w.field_0x31 = 0;
            system_w.field_0xa58 = 0;
            resident_file_load();
            task->step_0x08++;
            task->sub_0x09 = 0;
        }
        break;
    }
    case 5: {
        u16 trig = Psw[0].button_0x2C0.pressed_0x04;
        save_msg_blink_count = (save_msg_blink_count + 410) & 0x7FFF;
        for (;;) {
            switch (task->sub_0x09) {
            case 0:
                system_w.field_0x865 = 1;
                save_error_code = 0;
                banner_file_check_init();
                task->sub_0x09 = 1;
                task->step_0x0A = 0;
                task->page_0x0E = 0;
                break;
            case 1:
                save_error_code = 0;
                save_msg_blink_count = 0;
                switch (banner_file_check()) {
                case 1:
                    system_w.field_0x865 = 1;
                    task->sub_0x09 = 4;
                    task->step_0x0A = 0;
                    task->wait_0x0C = 30;
                    system_w.banner_valid_0x857 = 0;
                    system_file_read_init();
                    break;
                case -2:
                case -1:
                    system_w.field_0x865 = 0;
                    if ((save_error_code = save_error_get()) == -4) {
                        task->sub_0x09 = 3;
                        task->step_0x0A = 0;
                        break;
                    }
                    save_error_enter(task);
                    continue;
                }
                save_msg_window_draw(0, 0);
                args[0] = -200;
                subTransSet((u32)save_msg_text_draw, 1, args);
                break;
            case 2:
                if (task->wait_0x0C > 0) {
                    task->wait_0x0C--;
                }
                if ((save_error_code = save_error_get()) == -4 || save_error_code == -15) {
                    args[0] = 1;
                    args[1] = 0;
                    if (trig & 0x10) {
                        task->sub_0x09 = 6;
                        task->step_0x0A = 0;
                        args[0] = 7;
                        args[1] = 0;
                    }
                } else {
                    args[0] = chg_nand_err2msgcode();
                    args[1] = 0;
                    if (args[0] - 16 <= 1) {
                        args[1] = 0;
                        if (trig & 0x10) {
                            return_to_data_manager();
                            return;
                        }
                    }
                }
                break;
            case 3:
                switch (task->step_0x0A) {
                case 0:
                    save_space_check_init();
                    system_w.field_0x865 = 1;
                    task->step_0x0A++;
                    break;
                case 1:
                    switch (save_space_check_step()) {
                    case 1:
                        task->step_0x0A++;
                        break;
                    case -2:
                    case -1:
                        system_w.field_0x865 = 0;
                        save_error_enter(task);
                        continue;
                    }
                    break;
                case 2:
                    createSystemFile_init();
                    system_w.field_0x865 = 1;
                    task->step_0x0A++;
                    break;
                case 3:
                    save_error_code = 0;
                    switch (createSystemFile()) {
                    case 1:
                        system_w.sys_file_valid_0x856 = 1;
                        save_data_file_index = 0;
                        task->step_0x0A++;
                        break;
                    case -2:
                    case -1:
                        system_w.field_0x865 = 0;
                        save_error_enter(task);
                        continue;
                    }
                    break;
                case 4:
                    createDataFile_init(save_data_file_index, -2, 0);
                    system_w.field_0x865 = 1;
                    task->step_0x0A++;
                    break;
                case 5:
                    save_error_code = 0;
                    switch (createDataFile(1)) {
                    case 1:
                        if (++save_data_file_index == 6) {
                            task->step_0x0A++;
                        } else {
                            task->step_0x0A--;
                        }
                        break;
                    case -2:
                    case -1:
                        system_w.field_0x865 = 0;
                        save_error_enter(task);
                        continue;
                    }
                    break;
                case 6:
                    account_file_create_init();
                    system_w.field_0x865 = 1;
                    task->step_0x0A++;
                    break;
                case 7:
                    save_error_code = 0;
                    switch (account_file_create()) {
                    case 1:
                        save_data_file_index = 0;
                        task->step_0x0A++;
                        break;
                    case -2:
                    case -1:
                        system_w.field_0x865 = 0;
                        save_error_enter(task);
                        continue;
                    }
                    break;
                case 8:
                    save_ctl_reset();
                    system_w.field_0x865 = 1;
                    task->step_0x0A++;
                    break;
                case 9:
                    save_error_code = 0;
                    switch (banner_file_create()) {
                    case 1:
                        system_w.field_0x865 = 0;
                        task->step_0x0A++;
                        system_w.banner_valid_0x857 = 1;
                        task->wait_0x0C = 60;
                        system_w.field_0x865 = 0;
                        break;
                    case -2:
                    case -1:
                        system_w.field_0x865 = 0;
                        save_error_enter(task);
                        continue;
                    }
                    break;
                case 10:
                    if (task->wait_0x0C > 0) {
                        task->wait_0x0C--;
                    } else {
                        task->step_0x08 = 6;
                        system_w.sys_file_valid_0x856 = 1;
                    }
                    break;
                }
                if (task->step_0x0A <= 1) {
                    save_msg_window_draw(0, 0);
                    args[0] = -200;
                    subTransSet((u32)save_msg_text_draw, 1, args);
                } else {
                    args[0] = ((u8)(task->step_0x0A + 0xFE) <= 7) ? 5 : 6;
                    args[1] = 0;
                    if (save_sys_file_read == 0) {
                        save_msg_window_draw(args[0], 0);
                        subTransSet((u32)save_msg_text_draw, 2, args);
                    }
                }
                break;
            case 4:
                save_error_code = 0;
                save_msg_blink_count = 0;
                switch (task->step_0x0A) {
                case 0:
                    switch (system_file_read()) {
                    case 1:
                        save_sys_file_read = 1;
                        task->step_0x0A += 2;
                        task->wait_0x0C = 30;
                        account_file_read_init();
                        system_w.sys_file_valid_0x856 = 1;
                        system_w.field_0x865 = 1;
                        break;
                    case -2:
                    case -1:
                        system_w.field_0x865 = 0;
                        if (save_error_get() == -4) {
                            task->step_0x0A++;
                            task->wait_0x0C = 30;
                            createSystemFile_init();
                            system_w.field_0x865 = 1;
                            break;
                        }
                        system_w.field_0x865 = 0;
                        task->page_0x0E = 1;
                        save_error_enter(task);
                        continue;
                    }
                    break;
                case 1:
                    switch (createSystemFile()) {
                    case 1:
                        system_w.sys_file_valid_0x856 = 1;
                        system_w.field_0x865 = 1;
                        task->step_0x0A++;
                        task->wait_0x0C = 30;
                        account_file_read_init();
                        break;
                    case -2:
                    case -1:
                        system_w.field_0x865 = 0;
                        save_error_enter(task);
                        continue;
                    }
                    break;
                case 2:
                    switch (account_file_read()) {
                    case 1:
                        system_w.field_0x865 = 0;
                        task->sub_0x09++;
                        task->step_0x0A = 0;
                        task->wait_0x0C = 30;
                        break;
                    case -2:
                    case -1:
                        system_w.field_0x865 = 0;
                        result = save_error_get();
                        if (result == -4 || result == -15) {
                            task->step_0x0A++;
                            task->wait_0x0C = 30;
                            account_file_create_init();
                            system_w.field_0x865 = 1;
                            break;
                        }
                        task->page_0x0E = 1;
                        save_error_enter(task);
                        continue;
                    }
                    break;
                case 3:
                    switch (account_file_create()) {
                    case 1:
                        system_w.field_0x865 = 0;
                        task->sub_0x09++;
                        task->step_0x0A = 0;
                        task->sub_step_0x0B = 0;
                        task->wait_0x0C = 30;
                        break;
                    case -2:
                    case -1:
                        system_w.field_0x865 = 0;
                        save_error_enter(task);
                        continue;
                    }
                    break;
                }
                save_msg_window_draw(0, 0);
                args[0] = -200;
                args[1] = 0;
                subTransSet((u32)save_msg_text_draw, 1, args);
                break;
            case 5:
                save_error_code = 0;
                save_msg_blink_count = 0;
                switch (task->step_0x0A) {
                case 0:
                    createDataFile_init(task->sub_step_0x0B, -2, 0);
                    system_w.field_0x865 = 1;
                    task->step_0x0A++;
                    break;
                case 1:
                    result = createDataFile(1);
                    if (result == -2 || result == -1) {
                        result = save_error_get();
                        if (result != -15 && result != -4) {
                            task->page_0x0E = 1;
                            save_error_enter(task);
                            continue;
                        }
                    } else if (result != 1) {
                        break;
                    }
                    {
                        if (task->sub_step_0x0B < 5) {
                            createDataFile_init(++task->sub_step_0x0B, -2, 0);
                        } else {
                            system_w.field_0x865 = 0;
                            task->step_0x08++;
                            task->sub_0x09 = 0;
                            task->step_0x0A = 0;
                            task->sub_step_0x0B = 0;
                        }
                    }
                    break;
                }
                save_msg_window_draw(0, 0);
                args[0] = -200;
                args[1] = 0;
                subTransSet((u32)save_msg_text_draw, 1, args);
                break;
            case 6:
                switch (task->step_0x0A) {
                case 0:
                    args[0] = 7;
                    system_file_delete_init();
                    task->step_0x0A++;
                    break;
                case 1:
                    args[0] = 7;
                    switch (system_file_delete()) {
                    case 1:
                        system_w.banner_valid_0x857 = 0;
                        task->wait_0x0C = 60;
                        task->step_0x0A++;
                        break;
                    case -2:
                    case -1:
                        save_error_enter(task);
                        continue;
                    }
                    break;
                case 2:
                    args[0] = 2;
                    if (task->wait_0x0C > 0) {
                        task->wait_0x0C--;
                    } else {
                        banner_file_check_init();
                        task->sub_0x09 = 1;
                        task->step_0x0A = 0;
                        task->page_0x0E = 0;
                        args[0] = 3;
                        args[1] = 0;
                    }
                    break;
                }
                args[1] = 0;
                break;
            case 7:
                return_to_data_manager();
                return;
            }
            break;
        }
        if ((task->sub_0x09 == 2 && save_error_code != 0) || task->sub_0x09 == 6) {
            save_msg_window_draw(args[0], args[1]);
            subTransSet((u32)save_msg_text_draw, 2, args);
        }
        break;
    }
    case 6:
        memcpy(system_w.option_copy_0x912, option_w, 31);
        task->step_0x08 = 7;
        break;
    case 7:
        option_cfg_load(system_w.option_copy_0x912);
        system_w.strap_active_0x2b = 0;
        system_w.field_0x22 = 2;
        stream_bounds_arm();
        setSoftresetFlag(1);
        work_stack_reset();
        draw_shape_reset();
        arena_task_start();
        tsk_timer_clear(&task->state);
        break;
    }
}
#pragma peephole on

/* The texel rectangles (s0, t0, s1, t1) of the message box's four sprites. */
s16 save_msg_tex_rect[16] = {
    0x000, 0x000, 0x1DF, 0x0F7,
    0x000, 0x0E0, 0x1DF, 0x161,
    0x000, 0x162, 0x0EF, 0x1B1,
    0x0F0, 0x162, 0x1DF, 0x1B1,
};
/* The four sprites' sizes (w, h): the window, the one-line window and the two buttons. */
s16 save_msg_size[8] = {0x1E0, 0xF8, 0x1E0, 0x82, 0xF0, 0x50, 0xF0, 0x50};
/* The button positions (x, y) relative to the window. */
s16 save_msg_button_pos[6] = {0x07, 0xA4, 0xE5, 0xA4, 0x76, 0xA4};
/* The text positions (x, y) relative to the window. */
s16 save_msg_text_pos[14] = {0x66, 0xC0, 0x13C, 0xC0, 0x5E, 0xC0, 0x118, 0xB6, 0xAA, 0xB6, 0x62, 0xC0, 0xDE, 0xC0};

/* 0x8005A36C (0x2D0): draws the message box: the window and, for the yes/no messages, the two buttons with the
 * selected one blinking. */
#pragma peephole off
void save_msg_window_draw(s32 id, s32 flag) {
    _MH_VEC2 scr;
    fltSpr2TF spr;

    get_ScreenSize(&scr);

    spr.w = save_msg_size[0];
    spr.h = save_msg_size[1];
    spr.x = (s16)((scr.x - (f32)save_msg_size[0]) * 0.5f);
    spr.y = (s16)((scr.y - (f32)save_msg_size[1]) * 0.5f);
    spr.angle = 0;
    spr.r = save_msg_tex_rect[0];
    spr.g = save_msg_tex_rect[1];
    spr.b = save_msg_tex_rect[2];
    spr.a = save_msg_tex_rect[3];
    spr.color = 0xFFFFFFFF;
    drawSpr2TF(12, &spr, 0);

    s16 x = spr.x;
    s16 y = spr.y;

    if ((id >= 16 && id <= 17) || id == 1) {
        spr.w = save_msg_size[6];
        spr.h = save_msg_size[7];
        spr.x = (s16)(x + save_msg_button_pos[4]);
        spr.y = (s16)(y + save_msg_button_pos[5]);
        spr.angle = 0;
        spr.r = save_msg_tex_rect[12];
        spr.g = save_msg_tex_rect[13];
        spr.b = save_msg_tex_rect[14];
        spr.a = save_msg_tex_rect[15];
        spr.color = (flag == 0) ? 0xFFFFFFFF : 0xC0C0C0FF;
        drawSpr2TF(12, &spr, 0);

        spr.w = save_msg_size[4];
        spr.h = save_msg_size[5];
        spr.x = (s16)(x + save_msg_button_pos[4]);
        spr.y = (s16)(y + save_msg_button_pos[5]);
        spr.angle = 0;
        spr.r = save_msg_tex_rect[8];
        spr.g = save_msg_tex_rect[9];
        spr.b = save_msg_tex_rect[10];
        spr.a = save_msg_tex_rect[11];
        f32 c = fn_8005A63C((f32)(save_msg_blink_count & 0xFFFF) * 6.2831855f / 65536.0f);
        if (c < 0.0f) {
            c = c * -1.0f;
        }
        u8 a8 = (u8)((u32)(c * 95.0f) + 160);
        spr.color = ((u32)a8 << 24) | ((u32)a8 << 16) | ((u32)a8 << 8) | 0xFF;
        drawSpr2TF(12, &spr, 0);
    }
}
#pragma peephole on

/* 0x8005A63C (0xC): the table cosine of `x` radians (256 / 2pi steps per radian). */
f32 fn_8005A63C(f32 x) {
    return nw4r::math::CosFIdx(x * 40.743664f);
}

/* 0x8005A648 (0x298): draws the message text for message id `args[0]` (a `subTransSet` callback). */
void save_msg_text_draw(u32 count, s32* args) {
    s16 x;
    s16 y;
    s32 id = *args;
    _MH_VEC2 scr;
    char buf[512];
    s16 pos;
    s32 kind;
    char** text;

    get_ScreenSize(&scr);
    f32 width = scr.x;
    f32 half = 0.5f;
    y = (s16)((scr.y - (f32)save_msg_size[1]) * half);
    x = (width == 640.0f) ? 64 : 171;
    font_set_size(save_msg_font_size + 2, save_msg_font_size + 2);

    switch (id) {
    case 1:
        font_print_ex(x, 144, 0, (signed char*)"%s", sysmsg_save_text[system_w.field_0x09][id]);
        break;
    case 16:
    case 17:
    case 19:
        wii_sysmsg_gen(id, buf, 0);
        font_print_ex(x, 134, 0, (signed char*)"%s", buf);
        break;
    case -200:
        font_print_ex(x, 144, 0, (signed char*)"%s", sysmsg_save_text[system_w.field_0x09][0]);
        break;
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
        font_print_ex(x, 144, 0, (signed char*)"%s", sysmsg_save_text[system_w.field_0x09][id]);
        break;
    }

    switch (id) {
    case 1:
        pos = 3;
        kind = 1;
        break;
    case 16:
    case 17:
        pos = 2;
        kind = 2;
        break;
    case 19:
        return;
    default:
        return;
    }
    {
        switch (kind) {
        case 0:
        case 1:
            text = sysmsg_yes_text[system_w.field_0x09];
            break;
        case 2:
            text = sysmsg_manager_text[system_w.field_0x09];
            break;
        }
        font_print_ex(x, (s16)(y + save_msg_text_pos[pos * 4 + 1]), 5, (signed char*)"%s", text[0]);
    }
}

/* ---------------------------------------------------------------------------------------------------
 * The NW4R ResMat/ResMatChan accessor copies, 0x8005A8E0..0x8005AA28, in retail's address order.
 * ------------------------------------------------------------------------------------------------- */

void res_common_copy(ResHandle* dst, ResHandle* src);

/* 0x8005A8E0 (0x30): copy-constructs a handle from `src` and returns it. */
ResHandle* res_mat_chan_copy_ctor(ResHandle* self, ResHandle* src) {
    res_common_copy(self, src);
    return self;
}

/* 0x8005A910 (0xC): copies the handle word. */
void res_common_copy(ResHandle* dst, ResHandle* src) {
    dst->mPtr = src->mPtr;
}

} /* extern "C" */

namespace nw4r {
namespace g3d {

/* 0x8005A91C (0x34): returns the material's MatChan block. */
ResMatChan ResMat::GetResMatChan() {
    return ResMatChan((u8*)&ref() + 0x3F0);
}

/* 0x8005A950 (0x64): wraps `pData`, asserting its alignment. */
#pragma peephole off
NW4R_G3D_RESOURCE_CTOR_ALIGNED(ResMatChan, "g3d_resmat_ac.h", 465)
#pragma peephole on

} // namespace g3d
} // namespace nw4r

/* 0x8005A9B4 (0x8): stores the block address. */
NW4R_G3D_RESCOMMON_CTOR(nw4r::g3d::ResMatChanData)

namespace nw4r {
namespace g3d {

/* 0x8005A9BC (0x64): returns the material block, panicking on a NULL handle. */
NW4R_G3D_RESOURCE_REF(ResMat, "g3d_resmat_ac.h", 621)

/* 0x8005AA20 (0x8): returns the material block. */
NW4R_G3D_RESOURCE_PTR(ResMat)

} // namespace g3d
} // namespace nw4r
