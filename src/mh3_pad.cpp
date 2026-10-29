/*
 * mh3_pad.cpp - the game-root pad / mode file: the RSO sub-overlay loader, the task-slot table, the game-mode
 * entry points, the WPAD/KPAD set-up, the per-frame pad reads (remote, Nunchuk, Classic Controller, GameCube pad)
 * merged into the menu button block, rumble, and the game's own copy of the KPAD pointer tracker.
 *
 * RANGE. `.text` 0x800408A8-0x80046C80 (125 functions), `extab` 0x80006820-0x80006A48, `extabindex`
 *   0x8001E5A0-0x8001E8DC, `.ctors` 0x8056F2C4-0x8056F2C8, `.data` 0x8057C880-0x80581088, `.bss`
 *   0x806585B8-0x806694D0, `.sdata` 0x80790E28-0x80790EE0, `.sbss` 0x807947D0-0x80794868, `.sdata2`
 *   0x80795AD8-0x80795B58.  Name: the one-copy `__FILE__` string "mh3_pad.cpp" (.data 0x80580EF0) that
 *   `pad_wpad_init`'s OSPanic cites.  Left edge: `fn_80040598.cpp`'s extab group ends at 0x800408A8; right edge:
 *   `pad_connect.cpp` from 0x80046C80.
 * FLAGS. The unit's own `#pragma peephole off` (retail keeps `cmpwi` after `rlwinm`/`and`, `li r0,N` before
 *   `psq_l`, and the unfused `subi`+`cmpwi`; turning it on loses 1262 B of matched code) with `peephole on`
 *   around `kpad_obj_copy`, and `#pragma fp_contract off` (retail keeps `fmuls`+`fadds`; +393 B).
 * NAMES. GUESS (from each body, its callers and the data it touches): every name this unit gave its own rows -
 *   the RSO loaders (`rso_overlay_*`, `rso_sel_load`, `rso_pool_reset`), `lz16_decode`, `sys_font_decode`,
 *   `lang_index_set`, `strap_tpl_load`, `str_rfind`, `sdk_link_refs` (a never-called block of SDK calls with
 *   uninitialised arguments), `sdk_link_text_ptr`/`sdk_link_text`, the `gx_*` immediate helpers, `task_slot_table_run` and the
 *   `tsk_*` slot setters, `wpad_mem2_alloc/free`, `pad_*` (init, frame read, device reads, menu merge, rumble,
 *   accelerometer history, callbacks), `rc_get_*` (the `DemoPad` PADStatus fields), `stick_dir8_get`,
 *   `pointer_*` (the pointer ring and smoothing), `kpad_*` (named after the SDK KPAD routines each one copies:
 *   calc_acc, calc_acc_horizon/vertical, select_2obj_first/continue, select_1obj_first/continue,
 *   calc_dpd_variable, read_kpad_acc/dpd, reset_kpad, calc_dpd2pos_scale), and the data rows
 *   (`lang_select_table`, `lang_suffix_table`, `strap_tpl_name_table`, `pad_btn_*`, `pad_map_*`,
 *   `stick_dir8_table`, `mem2_lz_image_a/b`, `sys_font_lz_image`, `kpad_i*`, `pad_motor_level_table`,
 *   `pad_rm_acc_max`, `pad_kp_dist_init`, `wpad_*`, `pointer_*`, `pad_probe_*`, the RSO loader state
 *   `rso_sel_image`/`rso_boot_module`/`rso_overlay_module0..3`, `rso_slot_pool_index`, `rso_sel_path`,
 *   `rso_path_table`, `stage_map_rso_table`, `rso_boot_name`) and `game_mode_nop` (an empty game-mode entry
 *   between `field_game_task_start` and `ArenaSelExec`).  Registry proposal for the
 *   foreign callee `face_buf_reset` (0x8004882C).  `SystemWork.language_0x01`, `pause_0x10` and `pad_lock_ck_0x8d8` are GUESS field names.
 * RESIDUALS. `pad_records_construct`/`player_pad_construct`/`pad_fs_block_construct` are byte-identical plain
 *   functions standing for MWCC's static initialiser and the PlayerPad / Nunchuk-block constructors (a class
 *   `PlayerPad` whose +0xFC block holds 32 constructed VEC3s cannot be expressed in the flat header layout its 150
 *   consumers use), so the `.ctors` word (0x8056F2C4) that points at the initialiser is not emitted.
 *   Register allocation or scheduling only: `kpad_read_acc` (`&kpad_work` is formed before the first sample
 *   load), `pad_acc_y_delta`/`pad_fs_acc_y_delta` (r3/r4 swapped for the two sample pointers),
 *   `stage_map_set` (the flush size's constant is formed before `lbl_807947C0` loads), `pad_wpad_init`,
 *   `pad_menu_merge`, `kpad_pointer_read` and `kpad_read_dpd` (colouring), `lang_index_set` (`add` operand
 *   order), `pad_wait_table_set` (the unrolled loop's source-pointer bump is scheduled early).
 *   `pad_wpad_read`: retail re-forms `Psw + chan*0x350` for the second (core/Nunchuk) half and reads
 *   `pad_probe_type` through a copy of the pad pointer; ours keeps the base (`opt_common_subs off` re-forms
 *   both the multiply and the base: worse).  `pad_fs_stick_read`: retail keeps the two s16 stick values in
 *   saved registers and re-extends them at every use; ours spills the one-element arrays to the stack.
 *   `pad_gc_stick_read`: retail stores the `rc_get_*` results without `extsb` (a `u8` getter into the `s8`
 *   arrays).  `pad_motor_start`: retail stores `time` without the `clrlwi` (`SystemWork.field_0x33` is `u8` in
 *   the shared `system_work.h`; `s8` would match, 95 includers).  `pointer_smooth`: `vec3_distance` (owner
 *   `fn_8004CAD8.h`) is declared `f32` where retail rounds its result with `frsp`.  `kpad_obj_copy`: retail
 *   copies `center` as two words (`lwz`/`stw`); a `Vec2` assignment gives `lfs`/`stfs` under either peephole
 *   setting.
 *   Data: the unit's `.data`/`.sdata`/`.sbss`/`.sdata2` are claimed but not emitted (the bodies `extern` them),
 *   so `datagap.py` reports target-extra for them (flipcheck: 22 symbols other units reference are not defined
 *   by our object, and 8 force-active functions need `__declspec(export)`); `lbl_807947B4..lbl_807947C8`
 *   (`fn_80040598.cpp`'s pool state) keep their generated names.
 *   `config.yml` `block_relocations` covers `sys_font_lz_image` (0x8057CED0..0x80580058): its packed bytes read
 *   as addresses to dtk.
 * SHAPES. The Classic Controller (+0xD6) and Nunchuk (+0xFC) blocks of `PlayerPad` are read through the
 *   `PadStickBlock`/`PadFsBlock` views (retail addresses them from one block pointer); a one-stick loop for the
 *   Nunchuk stick; the button-map tables are hoisted into locals before the frame's first test.  The RSO
 *   loaders read each later overlay's name into one reused `name` local before the slot store; the screen
 *   centre is `width / 2.0f` (playbook 151); `kpad_sec_update` divides by the stored `sec_dist` (playbook 181);
 *   the accelerometer deltas are `prev - cur` (the magnitude is taken); `pad_menu_merge` seeds `dirbtn` from
 *   the masked hold it tests; `pad_motor_start` tests `get_cfg(...) ^ 1` (retail's `xori`: the getter's result
 *   is used as a bool).
 */

#pragma peephole off
#pragma fp_contract off
#include "types.h"
#include "gx.h"
#include "nw4r/math.h"
#include "mh3_pad/system_w.h" /* SystemWork / system_w (rule 1/2) */
#include "mh3_pad/Psw.h"        /* PlayerPad / Psw (rule 1/2) */
#include "mh3_pad/Screen_w.h"   /* ScreenWork / Screen_w (rule 1/2) */
#include "mh3_pad/lb_param_w.h" /* LbParamWork / lb_param_w (rule 1/2) */
#include "mh3_pad/option_w.h"   /* option_w (rule 2) */
#include "MEM/mem.h"            /* MEMAllocator (rule 1) */
#include "RSO/runtime.h"     /* RSOModule, RSOStaticLocateObject, RSOUnLinkList (rule 2) */
#include "fn_80040598.h"     /* the game-root RSO loaders (rule 2) */
#include "ef/fn_800CDB2C.h"  /* GameMode_ck / fn_800CEE2C (rule 2) */
#include "mh3_pad/task.h"    /* `TaskSlot` (rule 1) */
#include "quest/arenatask.h" /* arena_task: the arena-select task ArenaSelExec installs (rule 2) */
#include "Runtime.PPCEABI.H/memset.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "g3d/g3d_init.h" /* fn_8007A510 (rule 2) */
#include "PMIC/pmic.h"        /* PMICStop (rule 2) */
#include "WPAD/wpad.h"
#include "WPAD/WPADClamp.h"
#include "KPAD/kpad.h"
#include "MEM/mem_allocator.h"
#include "MEM/mem_expheap.h"
#include "MEM/MEMAllocFromAllocator.h"
#include "OS/OSError.h"
#include "OS/OSCache.h"
#include "MSL/w_math.h"
#include "MSL/mathf.h"
#include "MSL_C/string.h"
#include "MSL_C/strcat.h"
#include "main.h"               /* get_ScreenSize */
#include "menu/movie.h"         /* isMoviePlaying */
#include "userdata_item.h"      /* userdata_frame_reset */
#include "MTX/mtx.h"
#include "MTX/mtx44.h"
#include "RVLGX/GXPixel.h"
#include "RVLGX/GXTransform.h"
#include "RVLGX/GXGeometry.h"
#include "RVLGX/GXAttr.h"
#include "RVLGX/GXTev.h"
#include "RVLGX/GXBump.h"
#include "RVLGX/GXLight.h"
#include "Runtime.PPCEABI.H/CPlusLibPPC.h"
#include "RVLGX/GXTexture.h"
#include "font/flfnt.h"
#include "MSL_C/strstr.h"
#include "MSL_C/extras.h"
#include "MSL_C/vswprintf.h"
#include "AX/AX.h"
#include "AX/AXFXHooks.h"
#include "AX/AXAux.h"
#include "AX/AXCL.h"
#include "AI_SDK/ai.h"
#include "ARC/arc.h"
#include "WENC/wenc.h"
#include "OS/OSAlarm.h"
#include "OS/OSMessage.h"
#include "face_buf_reset.h"
#include "fn_8004CAD8.h"         /* vec3_distance */
#include "mh3_pad/control.h"   /* get_ControlType */
#include "mh3_pad/vec3.h"
#include "mh3_pad/rc_record.h"
#include "mh3_pad/disp_beta.h"
#include "menu/get_pop_dat_ptr.h"   /* option_cfg_eq_ck */
#include "menu/menu_pad_map_set.h"

/* ------------------------------------------------------------------ *
 * Local types
 * ------------------------------------------------------------------ */

/* `TaskSlot` (the 0x20-byte task slot `task_slot_table` indexes by `slot << 5`) lives in `mh3_pad/task.h`. */

/* `PlayerPad` (the 0x350-byte per-player pad record at `Psw`) lives in `mh3_pad/Psw.h`. */

/* The 0x18-byte per-channel state record of `pad_chan_state` (`pad_wpad_init` clears one per channel next to
 * the `Psw` records); only the motor byte at +0x14 is read so far. */
typedef struct PadChanState {
    /* +0x00 */ u8 pad_0x00[0x14];
    /* +0x14 */ u8 motor_level_0x14;  /* copied into `system_w.field_0x7dc` (the per-channel motor-on state) */
    /* +0x15 */ u8 pad_0x15[3];
} PadChanState; /* size: 0x18 */

/* ------------------------------------------------------------------ *
 * Extern declarations (all unsplit addresses - rule 2's named gap; see config_requests)
 * ------------------------------------------------------------------ */

extern "C" {
/* --- RSO load state (.sbss 0x807947B4..0x807947F0) --- */
extern u32 lbl_807947B4; /* load base */
extern u32 lbl_807947B8;
extern u32 lbl_807947BC;
extern u32 lbl_807947C0;
extern s8 lbl_807947C8[8]; /* [0] state, [1] sub-state */
extern u32 rso_sel_image;
extern RSOModule* rso_boot_module;
extern RSOModule* rso_overlay_module0;
extern RSOModule* rso_overlay_module1;
extern RSOModule* rso_overlay_module2;
extern RSOModule* rso_overlay_module3;
extern u8 pad_kpad_ready; /* KPAD initialised flag */

/* --- .sdata --- */
extern u8 rso_slot_pool_index[8];   /* per-slot RSO pool index */
extern char rso_sel_path[8]; /* "mh3.sel" */

/* --- .data string / table pool --- */
extern char* rso_path_table[]; /* RSO path pointers */
extern u8 stage_map_rso_table[];
extern char rso_boot_name[];

/* --- .bss --- */
extern u32 rso_slot_load_base[5];
extern u32 rso_slot_unused_words[5];
extern TaskSlot task_slot_table[16];
extern MEMAllocator wpad_mem2_allocator; /* MEM2 allocator record */

/* --- .sdata2 --- */

/* --- helpers owned by other units (rule 2 headers included above) --- */
extern char* strrchr(const char* s, int c);

/* task-entry bodies referenced by address from the mode dispatchers (.text, other units) */
extern s32 task_func;
extern void field_game_task(void);
extern void vs_game_task(void);
extern void quest_game_task(void);
extern void game_mode_flow_task(void);
extern void lobby_local_task(void);
extern PadChanState pad_chan_state[4];
}

/* ------------------------------------------------------------------ *
 * Pad tables and work (this unit's .data/.sbss/.bss)
 * ------------------------------------------------------------------ */

/* One 16-entry button bit table (the GameCube, Classic Controller and remote masks a mapped bit comes from).
 * size: 0x20 */
typedef struct PadBtnTable {
    /* +0x00 */ u16 bit[16];
} PadBtnTable;

/* One row of a device's menu button map: which device block (`kind` 0 the extension, else the remote) and
 * the mask tested in it. size: 0x4 */
typedef struct PadMapEntry {
    /* +0x00 */ u16 kind;
    /* +0x02 */ u16 mask;
} PadMapEntry;

/* One row of `pad_map_out`: the menu button's bit number and its mask. size: 0x4 */
typedef struct PadOutEntry {
    /* +0x00 */ u16 bit;
    /* +0x02 */ u16 mask;
} PadOutEntry;

/* One row of `lang_select_table` per console language: the strap image and the message language. size: 0x2 */
typedef struct LangSelect {
    /* +0x00 */ u8 strap;
    /* +0x01 */ u8 lang;
} LangSelect;

/* Swaps the two bytes of a 16-bit word. */
#define LZ16_SWAP(v) ((u16)((((v) >> 8) & 0xFF) | (((v) & 0xFF) << 8)))

extern "C" {
extern char** sdk_link_text_ptr;
extern u16 mem2_lz_image_a[];
extern u16 sys_font_lz_image[];
extern u16 mem2_lz_image_b[];
extern LangSelect lang_select_table[8];
extern char* lang_suffix_table[7];
extern char* strap_tpl_name_table[6];
extern u16 pad_btn_map_out[16];
extern PadBtnTable pad_btn_default_0;
extern PadBtnTable pad_btn_default_1;
extern PadBtnTable pad_btn_default_2;
extern u8 stick_dir8_table[16];
extern PadMapEntry pad_map_fs[17];
extern PadMapEntry pad_map_cl_a[17];
extern PadMapEntry pad_map_cl_b[17];
extern PadMapEntry pad_map_core[17];
extern PadOutEntry pad_map_out[18];
extern u16 wpad_latest_index_prev;
extern u16 wpad_latest_index;
extern u8 pointer_active;
extern MEMiHeapHead* wpad_mem2_heap;
extern s32 pad_probe_result;
extern u32 pad_probe_type;

extern PlayerPad Psw_prev[4];
extern PadBtnTable pad_btn_table_0;
extern PadBtnTable pad_btn_table_1;
extern PadBtnTable pad_btn_table_2;
extern WPADCLStatus wpad_sampling_buf_fmt8[4][100];
extern WPADFSStatus wpad_sampling_buf_fmt5[4][100];
extern WPADStatus wpad_sampling_buf_fmt2[4][100];

/* This unit's functions used before their definitions. */
char* str_rfind(char* s, int c);
void gx_end(void);
void gx_color4u8(u32 r, u32 g, u32 b, u32 a);
void gx_position3f32(f32 x, f32 y, f32 z);
void* wpad_mem2_alloc(u32 size); /* untyped: a byte range for WPAD */
s32 wpad_mem2_free(void* block);
MEMiHeapHead* exp_heap_create(void* base, u32 size);
void pad_motor_info_cb(s32 chan, s32 on);
void pad_wpad_read(s32 chan);
void pad_wpad_clear(s32 chan);
void pad_input_clear(s32 chan);
void pad_wpad_busy(s32 chan);
void pad_gc_read(s32 chan);
u16 rc_get_button(s32 idx);
u8 rc_get_err(s32 idx);
void pad_gc_stick_read(s32 chan, PlayerPad* pad);
f64 pad_sqrt(s32 v);
f64 pad_atan2_s8(s8 y, s8 x);
u8 rc_get_substick_y(s32 idx);
u8 rc_get_substick_x(s32 idx);
u8 rc_get_stick_y(s32 idx);
u8 rc_get_stick_x(s32 idx);
u8 rc_get_trigger_r(s32 idx);
u8 rc_get_trigger_l(s32 idx);
void pad_cl_stick_read(s32 chan, PlayerPad* pad);
f64 pad_atan2_s16(s16 y, s16 x);
void pad_fs_stick_read(s32 chan, PlayerPad* pad);
void pad_connect_cb(s32 chan, s32 result);
void pad_extension_cb(s32 chan, s32 result);
void pad_sampling_cb(s32 chan);
void pad_sync_device_cb(s32 result);
void pad_clear_device_cb(s32 result);
void pad_menu_merge(u8 chan);
void kpad_dpd_idle(u8 chan);
void kpad_work_init(void);
void kpad_work_reset(void);
}

/* The pointer work `kpad_work`: this unit's own copy of the KPAD inside status (`KPAD/kpad.h`'s
 * `KPADInsideStatus` before the ring buffer and the button repeat), filled from channel 0's samples.
 * size: 0x1B8 */
typedef struct KpadWork {
    /* +0x000 */ KPADStatus status;
    /* +0x0B0 */ f32 pos_play_radius;
    /* +0x0B4 */ f32 pos_sensitivity;
    /* +0x0B8 */ f32 hori_play_radius;
    /* +0x0BC */ f32 hori_sensitivity;
    /* +0x0C0 */ f32 dist_play_radius;
    /* +0x0C4 */ f32 dist_sensitivity;
    /* +0x0C8 */ f32 acc_play_radius;
    /* +0x0CC */ f32 acc_sensitivity;
    /* +0x0D0 */ f32 dist_org;
    /* +0x0D4 */ Vec2 accXY_nrm_hori;
    /* +0x0DC */ Vec2 sec_nrm11;
    /* +0x0E4 */ Vec2 center_org;
    /* +0x0EC */ f32 dpd2pos_scale;
    /* +0x0F0 */ KPADObj kobj_sample[4];
    /* +0x120 */ KPADObj kobj_regular[2];
    /* +0x138 */ s16 valid_objs;
    /* +0x13A */ u8 pad_0x13a[0x2];
    /* +0x13C */ f32 sec_dist;
    /* +0x140 */ Vec2 sec_nrm;
    /* +0x148 */ f32 dist_last;
    /* +0x14C */ f32 trust_sec_dist;
    /* +0x150 */ Vec hard_acc;
    /* +0x15C */ Vec2 dpd_horizon;
    /* +0x164 */ Vec2 acc_horizon;
    /* +0x16C */ Vec2 ah_circle_pos;
    /* +0x174 */ u16 ah_circle_ct;
    /* +0x176 */ u8 trust_sec_ct;
    /* +0x177 */ u8 pad_0x177[0x1];
    /* +0x178 */ Vec acc_scale;          /* 1 / the remote's gravity unit per axis */
    /* +0x184 */ Vec2 frame_min;
    /* +0x18C */ Vec2 frame_max;
    /* +0x194 */ f32 dist_speed_inc;
    /* +0x198 */ f32 dist_speed_dec;
    /* +0x19C */ f32 ah_circle_radius2;
    /* +0x1A0 */ f32 dist_scale;
    /* +0x1A4 */ f32 dist_min;
    /* +0x1A8 */ u32 pos_play_mode;
    /* +0x1AC */ u32 hori_play_mode;
    /* +0x1B0 */ u32 dist_play_mode;
    /* +0x1B4 */ u32 acc_play_mode;
} KpadWork;

extern "C" {
extern KpadWork kpad_work;
extern f32 kpad_idist_org;
extern Vec2 kpad_iaccXY_nrm_hori;
extern Vec2 kpad_isec_nrm11;
extern Vec2 kpad_dpd_center_init;
extern Vec2 kpad_icenter_org;
extern f32 pad_kp_dist_init;
extern s8 pad_motor_level_table[8];
extern f32 kp_obj_interval;
extern f32 kp_ah_circle_radius;
extern u16 kp_ah_circle_ct;
extern f32 kp_err_outside_frame;
extern f32 kp_err_dist_max;
extern f32 kp_err_dist_speed;
extern f32 kp_err_dist_min;

extern s16 pointer_hist_x[16];
extern s16 pointer_hist_y[16];
extern f32 pointer_center[3];
extern f32 pointer_offset[3];
extern s32 pointer_dpd_state;
extern s32 pointer_valid;
extern f32 pointer_half_x;
extern f32 pointer_half_y;
extern f32 pointer_prev_x;
extern f32 pointer_prev_y;
extern f32 pointer_norm_x;
extern f32 pointer_norm_y;
extern u8 kpad_dpd_on;
extern s32 pointer_timeout;
extern u8 pointer_stage;
extern s32 pointer_hist_count;
extern s16 pointer_last_a[2];
extern s16 pointer_last_b[2];
extern f32 pointer_dist;
extern f32 pointer_dist_limit;
extern u8 pointer_freeze;
extern u8 pointer_flag_b;

void pointer_calib_reset(void);
void pointer_hist_push(void);
void pointer_smooth(void);
void pad_motor_start(u8 chan, s8 time);
void kpad_dpd_enable(u8 chan);
void kpad_calc_dpd2pos_scale(KpadWork* kp);
void kpad_work_reset(void);
Vec2* vec2_copy(Vec2* dst, const Vec2* src);
void vec3_float_copy(f32* dst, f32* src);
void kpad_read_acc(WPADStatus* sample);
void kpad_read_dpd(WPADStatus* sample);
PlayerPad* player_pad_construct(PlayerPad* pad);
PadFsBlock* pad_fs_block_construct(PadFsBlock* blk);
}

extern "C" {
extern f32 kp_acc_horizon_pw;
extern f32 kp_ah_circle_pw;
extern f32 kp_err_first_inpr;
extern f32 kp_err_next_inpr;
extern f32 kp_err_near_pos;
extern f32 kp_err_up_inpr;
extern f32 kp_err_acc_inpr;
extern f32 pad_rm_acc_max;

f32 clamp_acc(f32 value, f32 limit);
void kpad_obj_copy(KPADObj* dst, const KPADObj* src);
void kpad_calc_dpd_variable(KpadWork* kp, s8 valid);
}

void initKPAD(void);

/* Copies the button table `src` into `dst`. */
static inline void pad_btn_table_copy(PadBtnTable* dst, const PadBtnTable* src)
{
    s32 i;

    for (i = 0; i < 16; i++) {
        dst->bit[i] = src->bit[i];
    }
}

/* Maps the bits of `raw` set in `from[i]` onto `to[i]`. */
static inline u16 pad_btn_remap(u16 raw, const u16* from, const u16* to)
{
    s32 i;
    u16 out;

    out = 0;
    for (i = 0; i < 16; i++) {
        if (raw & from[i]) {
            out |= to[i];
        }
    }
    return out;
}

/* ------------------------------------------------------------------ *
 * 0x800408A8 - 0x80040FDC  (RSO sub-overlay loader state machine)
 * ------------------------------------------------------------------ */

/* Boot: clear the sub-overlay state, load `mh3.sel` into the pool top, publish it and run its
 * prolog. */
extern "C" void rso_overlay_boot(void)
{
    u32 i;

    for (i = 0; i < 5; i++) {
        lbl_807947C8[i] = 0xFF;
    }
    rso_sel_image = (u32)CntSdRsoTerminate(rso_sel_path, (void*)0x80810000);
    lbl_807947B8 = lbl_807947B4;
    lbl_807947BC = lbl_807947B4 + 0x170000;
    rso_boot_module = (RSOModule*)fn_80040598(rso_boot_name, (void*)lbl_807947B4, 0xFF);
    fn_804DA7E4(rso_boot_module);
    rso_boot_module->prolog(rso_boot_module);
}

extern "C" void rso_sel_load(void)
{
    rso_sel_image = (u32)CntSdRsoTerminate(rso_sel_path, (void*)0x80810000);
}

/* Release every loaded sub-overlay and reset the loader state. */
extern "C" void rso_overlay_release_all(void)
{
    s32 loadedE4;
    s32 loadedE0;
    s32 loadedDC;
    s32 loadedD8;

    loadedE4 = 0;
    loadedE0 = 0;
    loadedDC = 0;
    loadedD8 = 0;
    lbl_807947C0 = lbl_807947BC;
    rso_slot_load_base[0] = 0;
    rso_slot_unused_words[0] = 0;
    rso_slot_load_base[1] = 0;
    rso_slot_unused_words[1] = 0;
    rso_slot_load_base[2] = 0;
    rso_slot_unused_words[2] = 0;
    rso_slot_load_base[3] = 0;
    rso_slot_unused_words[3] = 0;
    rso_slot_load_base[4] = 0;
    rso_slot_unused_words[4] = 0;
    if (rso_overlay_module0 != NULL) {
        rso_overlay_module0->epilog(rso_overlay_module0);
        loadedD8 = 1;
    }
    if (rso_overlay_module1 != NULL) {
        rso_overlay_module1->epilog(rso_overlay_module1);
        loadedDC = 1;
    }
    if (rso_overlay_module2 != NULL) {
        rso_overlay_module2->epilog(rso_overlay_module2);
        loadedE0 = 1;
    }
    if (rso_overlay_module3 != NULL) {
        rso_overlay_module3->epilog(rso_overlay_module3);
        loadedE4 = 1;
    }
    if (loadedD8 != 0) {
        RSOUnLinkList(rso_overlay_module0);
        rso_overlay_module0 = NULL;
    }
    if (loadedDC != 0) {
        RSOUnLinkList(rso_overlay_module1);
        rso_overlay_module1 = NULL;
    }
    if (loadedE0 != 0) {
        RSOUnLinkList(rso_overlay_module2);
        rso_overlay_module2 = NULL;
    }
    if (loadedE4 != 0) {
        RSOUnLinkList(rso_overlay_module3);
        rso_overlay_module3 = NULL;
    }
    lbl_807947C8[0] = 0xFF;
    lbl_807947C8[1] = 0xFF;
}

/* Load the two-overlay mode (state 2). */
extern "C" void rso_overlay_mode2_load(void)
{
    u32* pools;
    char** names;
    u8* idx;

    if (lbl_807947C8[0] != 2 && lbl_807947C8[0] != 4) {
        rso_overlay_release_all();
        pools = rso_slot_load_base;
        names = rso_path_table;
        idx = rso_slot_pool_index;
        pools[idx[0]] = lbl_807947C0;
        rso_overlay_module0 = (RSOModule*)fn_80040598(names[0], (void*)pools[idx[0]], idx[0]);
        pools[idx[1]] = lbl_807947C0;
        rso_overlay_module1 = (RSOModule*)fn_80040598(names[1], (void*)pools[idx[1]], idx[1]);
        fn_804DA7E4(rso_overlay_module0);
        fn_804DA7E4(rso_overlay_module1);
        rso_overlay_module0->prolog(rso_overlay_module0);
        rso_overlay_module1->prolog(rso_overlay_module1);
        lbl_807947C8[0] = 2;
    }
}

/* Load the three-overlay mode (state 1). */
extern "C" void rso_overlay_mode1_load(void)
{
    u32* pools;
    char** names;
    u8* idx;
    char* name;

    if (lbl_807947C8[0] != 1) {
        rso_overlay_release_all();
        pools = rso_slot_load_base;
        names = rso_path_table;
        idx = rso_slot_pool_index;
        pools[idx[0]] = lbl_807947C0;
        rso_overlay_module0 = (RSOModule*)fn_80040598(names[0], (void*)pools[idx[0]], idx[0]);
        name = names[2];
        pools[idx[2]] = lbl_807947C0;
        rso_overlay_module1 = (RSOModule*)fn_80040598(name, (void*)pools[idx[2]], idx[2]);
        name = names[3];
        pools[idx[3]] = lbl_807947C0;
        rso_overlay_module2 = (RSOModule*)fn_80040598(name, (void*)pools[idx[3]], idx[3]);
        fn_804DA7E4(rso_overlay_module0);
        fn_804DA7E4(rso_overlay_module1);
        fn_804DA7E4(rso_overlay_module2);
        lbl_807947C8[0] = 1;
        rso_overlay_module0->prolog(rso_overlay_module0);
        rso_overlay_module1->prolog(rso_overlay_module1);
        rso_overlay_module2->prolog(rso_overlay_module2);
        lbl_807947C8[0] = 1;
    }
}

/* Load the four-overlay mode (state 3). */
extern "C" void rso_overlay_mode3_load(void)
{
    u32* pools;
    char** names;
    u8* idx;
    char* name;

    if (lbl_807947C8[0] != 3) {
        rso_overlay_release_all();
        pools = rso_slot_load_base;
        names = rso_path_table;
        idx = rso_slot_pool_index;
        pools[idx[0]] = lbl_807947C0;
        rso_overlay_module0 = (RSOModule*)fn_80040598(names[0], (void*)pools[idx[0]], idx[0]);
        name = names[1];
        pools[idx[1]] = lbl_807947C0;
        rso_overlay_module1 = (RSOModule*)fn_80040598(name, (void*)pools[idx[1]], idx[1]);
        name = names[4];
        pools[idx[3]] = lbl_807947C0;
        rso_overlay_module2 = (RSOModule*)fn_80040598(name, (void*)pools[idx[3]], idx[3]);
        name = names[2];
        pools[3] = lbl_807947C0;
        rso_overlay_module3 = (RSOModule*)fn_80040598(name, (void*)lbl_807947C0, 3);
        fn_804DA7E4(rso_overlay_module0);
        fn_804DA7E4(rso_overlay_module1);
        fn_804DA7E4(rso_overlay_module2);
        fn_804DA7E4(rso_overlay_module3);
        lbl_807947C8[0] = 3;
        rso_overlay_module0->prolog(rso_overlay_module0);
        rso_overlay_module1->prolog(rso_overlay_module1);
        rso_overlay_module2->prolog(rso_overlay_module2);
        rso_overlay_module3->prolog(rso_overlay_module3);
        lbl_807947C8[0] = 3;
    }
}

/* Reload the language overlay (state byte 1) selected by `which`. */
extern "C" void stage_map_set(u8 which)
{
    u8 sel;

    sel = stage_map_rso_table[which];
    if (lbl_807947C8[1] == -1 || lbl_807947C8[1] != (s8)sel) {
        if (rso_overlay_module3 != NULL) {
            rso_overlay_module3->epilog(rso_overlay_module3);
            RSOUnLinkList(rso_overlay_module3);
            rso_overlay_module3 = NULL;
        }
        DCFlushRange((void*)lbl_807947C0, (u32)(0x80C4CE00 - lbl_807947C0));
        rso_slot_load_base[rso_slot_pool_index[4]] = lbl_807947C0;
        rso_overlay_module3 = (RSOModule*)fn_80040598(rso_path_table[sel], (void*)rso_slot_load_base[rso_slot_pool_index[4]],
                                                rso_slot_pool_index[4]);
        fn_804DA7E4(rso_overlay_module3);
        rso_overlay_module3->prolog(rso_overlay_module3);
        lbl_807947C8[1] = sel;
    }
}

/* Load the fifth-overlay mode (state 4). */
extern "C" void rso_overlay_mode4_load(void)
{
    u32* pools;
    char** names;
    u8* idx;
    char* name;

    if (lbl_807947C8[0] != 4) {
        rso_overlay_release_all();
        pools = rso_slot_load_base;
        names = rso_path_table;
        idx = rso_slot_pool_index;
        pools[idx[0]] = lbl_807947C0;
        rso_overlay_module0 = (RSOModule*)fn_80040598(names[0], (void*)pools[idx[0]], idx[0]);
        name = names[1];
        pools[idx[1]] = lbl_807947C0;
        rso_overlay_module1 = (RSOModule*)fn_80040598(name, (void*)pools[idx[1]], idx[1]);
        name = names[0x13];
        pools[idx[5]] = lbl_807947C0;
        rso_overlay_module2 = (RSOModule*)fn_80040598(name, (void*)pools[idx[5]], idx[5]);
        fn_804DA7E4(rso_overlay_module0);
        fn_804DA7E4(rso_overlay_module1);
        fn_804DA7E4(rso_overlay_module2);
        rso_overlay_module0->prolog(rso_overlay_module0);
        rso_overlay_module1->prolog(rso_overlay_module1);
        rso_overlay_module2->prolog(rso_overlay_module2);
        lbl_807947C8[0] = 4;
    }
}

/* Reset the two pool tops. */
extern "C" void rso_pool_reset(void)
{
    lbl_807947C0 = lbl_807947B8;
    lbl_807947BC = lbl_807947B8;
}

/* Decodes the 16-bit LZ stream at `src` (big-endian flag words over little-endian payload) into `dst`;
 * returns the number of bytes written. */
extern "C" u32 lz16_decode(u16* src, u16* dst)
{
    u16 flags;
    s32 mask;
    u32 count;
    u16* start;
    u16* from;
    u32 word;

    start = dst;
    mask = 0;
    flags = 0;
    for (;;) {
        if (mask == 0) {
            mask = 0x8000;
            flags = LZ16_SWAP(*src);
            src++;
        }
        if (flags & mask) {
            word = LZ16_SWAP(*src);
            src++;
            count = word >> 11;
            if (count != 0) {
                word &= 0x7FF;
            } else {
                count = LZ16_SWAP(*src);
                src++;
            }
            if (word == 0) {
                if (count == 0) {
                    break;
                }
                do {
                    *dst++ = 0;
                } while (--count != 0);
            } else {
                from = dst - word;
                do {
                    *dst++ = *from++;
                } while (--count != 0);
            }
        } else {
            *dst++ = *src++;
        }
        mask >>= 1;
    }
    return (u8*)dst - (u8*)start;
}

extern "C" void pad_module_init(void)
{
    lz16_decode(mem2_lz_image_a, (u16*)0x90001000);
    lz16_decode(mem2_lz_image_b, (u16*)0x90001660);
}

/* Decodes the system font image into `dst`. */
extern "C" u32 sys_font_decode(u16* dst)
{
    return lz16_decode(sys_font_lz_image, dst);
}

/* Never called: a block of SDK calls, most with uninitialised arguments, that keeps those entry points
 * referenced. */
extern "C" void sdk_link_refs(void)
{
    Mtx m;
    u16 text[16];
    u16 format[16];
    va_list args;
    char axis;
    f32 x;
    f32 y;
    f32 z;
    GXTexObj* tex;
    u32 wrap_s;
    u32 wrap_t;
    ARCHandle* handle;
    s32 entry;
    char* path;
    ARCFileInfo* info;
    ARCDir* dir;
    ARCDirEntry* dirent;
    WENCInfo* wenc;
    s16* samples;
    s32 count;
    u8* out;
    u32 flags;
    OSMessageQueue* queue;
    void* msg;
    u16 volume;

    WPADEnableMotor(0);
    KPADRead(0, 0, 0);
    flfntPrintf(*sdk_link_text_ptr);
    PSMTXScale(m, x, y, z);
    PSMTXRotRad(m, axis, x);
    PSMTXTrans(m, x, y, z);
    log10(65536000.0);
    vswprintf(text, 0, format, args);
    wcslen(text);
    stricmp(0, 0);
    GXInitTexObjWrapMode(tex, wrap_s, wrap_t);
    GXInitTexObjTlut(tex, wrap_s);
    GXGetTexObjUserData(tex);
    GXInitTexObjUserData(tex, 0);
    AXInit();
    AICheckInit();
    ARCFastOpen(handle, entry, info);
    ARCOpen(handle, path, info);
    ARCConvertPathToEntrynum(handle, path);
    ARCOpenDir(handle, path, dir);
    ARCReadDir(dir, dirent);
    ARCCloseDir(dir);
    ARCGetLength(info);
    ARCChangeDir(handle, path);
    ARCGetStartAddrInMem(info);
    ARCClose(info);
    ARCInitHandle(0, 0);
    WENCGetEncodeData(wenc, flags, samples, count, out);
    WPADCanSendStreamData(0);
    WPADSendStreamData(0, 0, 0);
    WPADGetRadioSensitivity(0);
    WPADSetSamplingCallback(0, 0);
    WPADGetSpeakerVolume();
    WPADSetSpeakerVolume(0);
    WPADIsSpeakerEnabled(0);
    WPADSetSimpleSyncCallback(0);
    WPADDisconnect(0);
    WPADStartFastSimpleSync();
    WPADStopSimpleSync();
    WPADSaveConfig(0);
    WPADIsSpeakerEnabled(0);
    OSSetAlarmUserData(0, 0);
    AXFXGetHooks(0, 0);
    AXGetAuxACallback(0, 0);
    AXGetAuxAReturnVolume();
    AXGetAuxBReturnVolume();
    volume = AXGetAuxCReturnVolume();
    AXSetAuxAReturnVolume(volume);
    AXSetAuxBReturnVolume(volume);
    AXSetAuxCReturnVolume(volume);
    OSJamMessage(queue, msg, 1);
}

/* Sets the message-language index from the console language. */
extern "C" void lang_index_set(void)
{
    SystemWork* sys;
    LangSelect* sel;

    sys = &system_w;
    sel = &lang_select_table[sys->language_0x01];
    sys->field_0x09 = sel->lang;
}

/* Loads the console language's strap image into its MEM2 slot. */
extern "C" void strap_tpl_load(void)
{
    load_file(strap_tpl_name_table[lang_select_table[system_w.language_0x01].strap], 0x90F49000, 0x65000);
}

/* Copies `name` into `out` with the language suffix before its extension. */
void cnvt_eur_fname(char* out, char* name)
{
    u8 lang;
    char* dot;

    lang = system_w.field_0x09;
    strcpy(out, name);
    dot = str_rfind(out, '.');
    if (dot != NULL) {
        *dot = 0;
        strcat(out, lang_suffix_table[lang]);
        strcat(out, str_rfind(name, '.'));
    }
}

/* ------------------------------------------------------------------ *
 * 0x80041404 - 0x800418B0  (font helpers and the task-slot table)
 * ------------------------------------------------------------------ */

extern "C" char* str_rfind(char* s, int c)
{
    return strrchr(s, c);
}

extern "C" void pmic_stop(void)
{
    PMICStop();
}

/* Draws a flat `color` rectangle at `pos` of size `size` in screen space. */
void disp_beta(Vec* pos, Vec2* size, _GXColor color)
{
    _MH_VEC2 screen;
    Mtx44 proj;
    Mtx mv;

    get_ScreenSize(&screen);
    GXSetBlendMode(1, 4, 5, 0);
    GXSetColorUpdate(1);
    C_MTXOrtho(proj, 0.0f, screen.y, 0.0f, screen.x, 0.0f, 1.0f);
    GXSetProjection(proj, 1);
    PSMTXIdentity(mv);
    GXLoadPosMtxImm(mv, 0);
    GXSetCurrentMtx(0);
    GXSetCullMode(0);
    GXSetZMode(1, 7, 0);
    GXSetNumTexGens(0);
    GXSetNumTevStages(1);
    GXSetTevDirect(0);
    GXSetTevOp(0, 4);
    GXSetTevOrder(0, 0xFF, 0xFF, 4);
    GXSetNumChans(1);
    GXSetChanCtrl(4, 0, 0, 1, 1, 0, 2);
    GXClearVtxDesc();
    GXSetVtxDesc(9, 1);
    GXSetVtxDesc(11, 1);
    GXSetVtxAttrFmt(0, 9, 1, 4, 0);
    GXSetVtxAttrFmt(0, 11, 1, 5, 0);
    GXBegin(0x80, 0, 4);
    gx_position3f32(pos->x, pos->y, pos->z);
    gx_color4u8(color.r, color.g, color.b, color.a);
    gx_position3f32(pos->x + size->x, pos->y, pos->z);
    gx_color4u8(color.r, color.g, color.b, color.a);
    gx_position3f32(pos->x + size->x, pos->y + size->y, pos->z);
    gx_color4u8(color.r, color.g, color.b, color.a);
    gx_position3f32(pos->x, pos->y + size->y, pos->z);
    gx_color4u8(color.r, color.g, color.b, color.a);
    gx_end();
}

extern "C" void gx_end(void)
{
}

/* GX immediate-mode colour (writes the 0xCC008000 write-gather pipe). */
extern "C" void gx_color4u8(u32 r, u32 g, u32 b, u32 a)
{
    *(volatile u8*)0xCC008000 = (u8)r;
    *(volatile u8*)0xCC008000 = (u8)g;
    *(volatile u8*)0xCC008000 = (u8)b;
    *(volatile u8*)0xCC008000 = (u8)a;
}

extern "C" void gx_position3f32(f32 x, f32 y, f32 z)
{
    *(volatile f32*)0xCC008000 = x;
    *(volatile f32*)0xCC008000 = y;
    *(volatile f32*)0xCC008000 = z;
}

extern "C" void task_slot_table_clear(void)
{
    memset(task_slot_table, 0, 0x200);
}

/* Runs one frame of the task slot table: starts, ticks and calls every slot by its state. */
extern "C" void task_slot_table_run(void)
{
    TaskSlot* task;
    s32 i;
    SystemWork* sys;
    s32 hold;

    sys = &system_w;
    if (sys->field_0x05 == 0 || (GameMode_ck() != 1 && isMoviePlaying() == 0)) {
        subTransReset();
        face_buf_reset();
    }
    hold = sys->pause_0x10 != 0;
    sys->field_0x0c++;
    ran_suu(1);
    if (sys->field_0x05 != 0) {
        hold = 1;
    }
    for (i = 0, task = task_slot_table; i < 16; i++, task++) {
        if (hold != 0 && (u32)(i - 2) <= 8) {
            continue;
        }
        switch (task->state) {
        case 4:
            task->state = 8;
            break;
        case 12:
            task->state = 8;
            break;
        case 8:
            task->func(task);
            break;
        case 2:
            task->timer = 0;
            task->state = 4;
            break;
        case 16:
            task->timer--;
            if (task->timer < 0) {
                task->timer = 0;
            } else if (task->timer == 0) {
                task->state = 4;
            }
            break;
        }
    }
}

/* Start a task slot running `func` (state 0xC). */
extern "C" void tsk_start(void* func, s16 slot)
{
    TaskSlot* task;

    task = &task_slot_table[slot];
    memset(task, 0, 0x20);
    task->state = 0xC;
    task->func = (void (*)(TaskSlot*))func;
}

extern "C" void tsk_timer_clear(s16* timer)
{
    *timer = 0;
}

extern "C" void tsk_set_wait(s16 slot)
{
    task_slot_table[slot].state = 0x10;
}

extern "C" void tsk_set_state1(s16 slot)
{
    task_slot_table[slot].state = 1;
}

extern "C" void tsk_set_state2(s16 slot)
{
    task_slot_table[slot].state = 2;
}

extern "C" void tsk_stop(s16 slot)
{
    task_slot_table[slot].state = 0;
}

/* Start a task slot with a fresh body (state 8).  `Tsk_Change__FPvs`: (void*, short). */
void Tsk_Change(void* func, s16 slot)
{
    TaskSlot* task;

    task = &task_slot_table[slot];
    memset(task, 0, 0x20);
    task->state = 8;
    task->func = (void (*)(TaskSlot*))func;
}

extern "C" TaskSlot* tsk_get(s16 slot)
{
    return &task_slot_table[slot];
}

/* The arena-select task entry: load the four-overlay mode and hand slot 4 to `arena_task`. */
extern "C" void arena_task_start(void)
{
    rso_overlay_mode3_load();
    if (task_func != 0) {
        Tsk_Change((void*)task_func, 4);
    }
}

extern "C" void field_game_task_start(void)
{
    Tsk_Change((void*)&field_game_task, 4);
}

extern "C" void game_mode_nop(void)
{
}

void ArenaSelExec(void)
{
    rso_overlay_mode3_load();
    Tsk_Change((void*)&arena_task, 4);
}

/* The WPAD allocator: takes `size` bytes from the MEM2 pad heap. */
extern "C" void* wpad_mem2_alloc(u32 size) /* untyped: a byte range for WPAD */
{
    return MEMAllocFromAllocator(&wpad_mem2_allocator, size);
}

extern "C" s32 wpad_mem2_free(void* block)
{
    MEMFreeToAllocator(&wpad_mem2_allocator, block);
    return 1;
}

void VsGameModeExec(void)
{
    Tsk_Change((void*)&vs_game_task, 4);
}

void TestModeExec(void)
{
}

/* Sets up the remote side: the MEM2 heap WPAD allocates from, the WPAD callbacks, the button tables and every
 * channel's pad records. */
extern "C" void pad_wpad_init(void)
{
    PlayerPad* pad;
    PlayerPad* prev;
    PadChanState* state;
    s32 chan;
    s32 i;

    DEMOPadInit();
    if (wpad_mem2_heap == NULL) {
        wpad_mem2_heap = exp_heap_create((void*)0x90308000, 0x40000);
        if (wpad_mem2_heap == NULL) {
            OSPanic(__FILE__, 464, "MEM2 heap allocation error.\n");
        }
    }
    MEMInitAllocatorForExpHeap(&wpad_mem2_allocator, (MEMiHeapHead*)wpad_mem2_heap, 32);
    WPADRegisterAllocator(wpad_mem2_alloc, wpad_mem2_free);
    WPADInit();
    kpad_work_init();
    for (chan = 0; chan < 4; chan++) {
        WPADSetConnectCallback(chan, pad_connect_cb);
    }
    WPADSetSyncDeviceCallback(pad_sync_device_cb);
    WPADSetClearDeviceCallback(pad_clear_device_cb);
    WPADSetAutoSleepTime(5);
    pad_btn_table_copy(&pad_btn_table_0, &pad_btn_default_0);
    pad_btn_table_copy(&pad_btn_table_1, &pad_btn_default_1);
    pad_btn_table_copy(&pad_btn_table_2, &pad_btn_default_2);
    for (chan = 0, pad = Psw, prev = Psw_prev, state = pad_chan_state; chan < 4; chan++) {
        memset(pad, 0, sizeof(PlayerPad));
        memset(prev, 0, sizeof(PlayerPad));
        memset(state, 0, sizeof(PadChanState));
        for (i = 0; i < 16; i++) {
            VEC3 zero_acc;
            VEC3 zero_fs_acc;

            copyVec3((VEC3*)pad->acc_hist_0x1E4[i], setVec3(&zero_acc, 0.0f, 0.0f, 0.0f));
            copyVec3((VEC3*)pad->fs_acc_hist_0x124[i], setVec3(&zero_fs_acc, 0.0f, 0.0f, 0.0f));
        }
        for (i = 0; i < 16; i++) {
            pad->button_0x2C0.wait_table_0x24[0][i] = 3;
        }
        pad++;
        prev++;
        state++;
    }
}

/* Reads every pad of this frame: the GameCube pads, then each channel's remote and GameCube records. */
extern "C" void pad_frame_read(void)
{
    PlayerPad* prev;
    PlayerPad* pad;
    s32 chan;

    wpad_latest_index_prev = wpad_latest_index;
    wpad_latest_index = WPADGetLatestIndexInBuf(0);
    if (WPADIsMotorEnabled() == 0) {
        system_w.field_0x32 = 0;
    } else {
        system_w.field_0x32 = 1;
    }
    if (system_w.pad_lock_ck_0x8d8() == 0) {
        DEMOPadRead();
    }
    for (chan = 0, prev = Psw_prev, pad = Psw; chan < 4; prev++, pad++, chan++) {
        pad_wpad_read(chan);
        pad_gc_read(chan);
        memcpy(pad, prev, 0x2E);
    }
    if (system_w.pad_lock_ck_0x8d8() == 1) {
        pointer_active = 1;
    } else {
        kpad_dpd_idle(0);
        pointer_active = 0;
    }
}

/* Reads channel `chan`'s remote: probes it, copies the sample of its device and derives the button blocks. */
extern "C" void pad_wpad_read(s32 chan)
{
    PlayerPad* pad;
    WPADStatus* core;
    WPADCLStatus* cl;
    WPADFSStatus* fs;
    PlayerPad* probe;
    WPADStatus* sample;
    PadStickBlock* clb;
    PlayerPad* pad2;
    PadFsBlock* fsb;
    PlayerPad* chan_pad;
    const u16* from;
    const u16* to;

    pad = &Psw[chan];
    cl = (WPADCLStatus*)pad->cl_status_0x62;
    fs = (WPADFSStatus*)pad->fs_status_0x98;
    core = (WPADStatus*)pad->core_status_0x38;
    from = pad_btn_table_1.bit;
    to = pad_btn_map_out;
    clb = (PadStickBlock*)&pad->control_0x0D6;
    clb->prev = clb->hold;
    clb->stick_prev = clb->stick_hold;
    if (system_w.pad_lock_ck_0x8d8() == 1) {
        pad_wpad_clear(chan);
        return;
    }
    Psw[chan].field_0x30 = WPADProbe(chan, &pad->mode);
    if (chan == 0) {
        pad_probe_result = Psw[chan].field_0x30;
        pad_probe_type = Psw[chan].mode;
    }
    if ((s32)Psw[chan].field_0x30 == -2 || (u32)isMoviePlaying() == 1) {
        pad_wpad_busy(chan);
        return;
    }
    if (Psw[chan].field_0x30 == 0) {
        switch (Psw[chan].mode) {
        case 1:
            sample = &fs->core;
            break;
        case 2:
            sample = &cl->core;
            break;
        default:
            sample = core;
            break;
        }
        wpadCopyCurrentSample(chan, sample);
        WPADClampStick(chan, sample, 2);
        WPADClampAcc(chan, sample, 1);
    }
    if (Psw[chan].field_0x30 == 0 && Psw[chan].mode == 2) {
        clb->hold = pad_btn_remap(cl->clButton, from, to);
        pad_cl_stick_read(chan, pad);
    } else {
        clb->hold = 0;
        clb->stick_hold = 0;
        clb->angle[1] = 0;
        clb->angle[0] = 0;
        clb->mag[1] = 0;
        clb->mag[0] = 0;
        pad_cl_stick_read(chan, pad);
    }
    clb->trig = clb->hold & ~clb->prev;
    clb->release = clb->prev & ~clb->hold;
    clb->stick_trig = clb->stick_hold & ~clb->stick_prev;
    clb->stick_release = clb->stick_prev & ~clb->stick_hold;
    clb->rept = 0;
    if (clb->prev == clb->hold) {
        clb->rept_count++;
        if (clb->rept_count > 12) {
            clb->rept_count = 10;
            clb->rept = clb->hold;
        }
    } else {
        clb->rept_count = 0;
    }

    pad2 = &Psw[chan];
    fsb = (PadFsBlock*)&pad2->control_0x0FC;
    from = pad_btn_table_2.bit;
    to = pad_btn_map_out;
    pad2->core_prev_0xCC = pad2->field_0xca;
    fsb->btn.prev = fsb->btn.hold;
    fsb->btn.stick_prev = fsb->btn.stick_hold;
    if (pad2->field_0x30 == 0) {
        u16 raw;

        if (system_w.field_0x7e4[chan] == 1 && (s8)system_w.field_0x7e0[chan] == 120) {
            WPADGetInfoAsync(chan, &pad_chan_state[chan], pad_motor_info_cb);
            system_w.field_0x7e4[chan] = 0;
        }
        if ((s8)system_w.field_0x7e0[chan] > 0) {
            system_w.field_0x7e0[chan]--;
        }
        switch (Psw[chan].mode) {
        case 1:
            raw = fs->core.button;
            break;
        case 2:
            raw = cl->core.button;
            break;
        default:
            raw = core->button;
            break;
        }
        pad2->field_0xca = pad_btn_remap(raw, from, to);
    } else {
        pad2->field_0xca = 0;
    }
    pad2->field_0xce = pad2->field_0xca & ~pad2->core_prev_0xCC;
    pad2->core_release_0xD0 = pad2->core_prev_0xCC & ~pad2->field_0xca;
    pad2->field_0xd2 = 0;
    if (pad2->core_prev_0xCC == pad2->field_0xca) {
        pad2->core_rept_count_0xD4++;
        if (pad2->core_rept_count_0xD4 > 12) {
            pad2->core_rept_count_0xD4 = 10;
            pad2->field_0xd2 = pad2->field_0xca;
        }
    } else {
        pad2->core_rept_count_0xD4 = 0;
    }
    fsb->index = (fsb->index + 1) & 0xF;
    chan_pad = &Psw[chan];
    if (chan_pad->field_0x30 == 0 && chan_pad->mode == 1) {
        fsb->btn.hold = pad2->field_0xca;
        fsb->btn.prev = pad2->core_prev_0xCC;
        fsb->btn.trig = pad2->field_0xce;
        fsb->btn.release = pad2->core_release_0xD0;
        fsb->btn.rept = pad2->field_0xd2;
        fsb->btn.rept_count = pad2->core_rept_count_0xD4;
        pad_fs_stick_read(chan, chan_pad);
        fsb->acc[fsb->index][0] = fs->core.accX;
        fsb->acc[fsb->index][1] = fs->core.accY;
        fsb->acc[fsb->index][2] = fs->core.accZ;
        fsb->fs_acc[fsb->index][0] = fs->fsAccX;
        fsb->fs_acc[fsb->index][1] = fs->fsAccY;
        fsb->fs_acc[fsb->index][2] = fs->fsAccZ;
    } else {
        VEC3 zero_acc;
        VEC3 zero_fs_acc;

        fsb->btn.hold = 0;
        fsb->btn.stick_hold = 0;
        fsb->btn.angle[1] = 0;
        fsb->btn.angle[0] = 0;
        fsb->btn.mag[1] = 0;
        fsb->btn.mag[0] = 0;
        copyVec3((VEC3*)fsb->acc[fsb->index], setVec3(&zero_acc, 0.0f, 0.0f, 0.0f));
        copyVec3((VEC3*)fsb->fs_acc[fsb->index], setVec3(&zero_fs_acc, 0.0f, 0.0f, 0.0f));
        pad_fs_stick_read(chan, &Psw[chan]);
    }
    fsb->btn.trig = fsb->btn.hold & ~fsb->btn.prev;
    fsb->btn.release = fsb->btn.prev & ~fsb->btn.hold;
    fsb->btn.stick_trig = fsb->btn.stick_hold & ~fsb->btn.stick_prev;
    fsb->btn.stick_release = fsb->btn.stick_prev & ~fsb->btn.stick_hold;
    fsb->btn.rept = 0;
    if (fsb->btn.prev == fsb->btn.hold) {
        fsb->btn.rept_count++;
        if (fsb->btn.rept_count > 12) {
            fsb->btn.rept_count = 10;
            fsb->btn.rept = fsb->btn.hold;
        }
    } else {
        fsb->btn.rept_count = 0;
    }
    pad_menu_merge(chan);
}

/* ------------------------------------------------------------------ *
 * 0x80041E40 - 0x80041E9C  (vector helpers)
 * ------------------------------------------------------------------ */

/* The record these three work on is `nw4r::math::VEC3` (0xC, x/y/z at +0/+4/+8) - see
 * `mh3_pad/vec3.h` for the evidence (`setVec3`'s body is byte-identical to the map's
 * `setVector3__FPQ34nw4r4math4VEC3fff`).  The parameters were `void*` until the type-fix pass. */
extern "C" void vec3_assign(VEC3* dst, const VEC3* src)
{
    dst->x = src->x;
    dst->y = src->y;
    dst->z = src->z;
}

extern "C" VEC3* copyVec3(VEC3* dst, const VEC3* src)
{
    vec3_assign(dst, src);
    return dst;
}

/* The return is the first argument, as the callers use it (`mr r4,r3` after the `bl`); the body
 * leaves r3 alone, so `return out;` costs no instruction. */
extern "C" VEC3* setVec3(VEC3* out, f32 x, f32 y, f32 z)
{
    out->x = x;
    out->y = y;
    out->z = z;
    return out;
}

/* Creates an expandable heap over `size` bytes at `base`. */
extern "C" MEMiHeapHead* exp_heap_create(void* base, u32 size)
{
    return (MEMiHeapHead*)MEMCreateExpHeapEx(base, size, 0);
}

/* ------------------------------------------------------------------ *
 * 0x80042B34 - 0x80042F60  (DemoPad accessors)
 * ------------------------------------------------------------------ */


/* Clears pad `chan`'s Classic Controller and Nunchuk button and stick words (keeping the Classic trigger's
 * bit 1). */
extern "C" void pad_input_clear(s32 chan)
{
    PlayerPad* pad;

    pad = &Psw[chan];
    pad->control_0x0D6 = 0;
    pad->field_0xda &= 2;
    pad->cc_word2_0x0DC = 0;
    pad->control_0x0DE = 0;
    pad->cl_stick_trig_0xE2 = 0;
    pad->cl_stick_release_0xE4 = 0;
    pad->cl_angle_0xE6 = pad->field_0xe8 = 0;
    pad->cl_mag_0xEA = pad->field_0xec = 0;
    pad->cl_deg_0xEE[0] = pad->cl_deg_0xEE[1] = 0;
    pad->cl_dist_0xF2[0] = pad->cl_dist_0xF2[1] = 0;
    pad->cl_dir_0xF6[0] = pad->cl_dir_0xF6[1] = 0;
    pad->control_0x0FC = 0;
    pad->field_0x100 = 0;
    pad->wpad_word2_0x102 = 0;
    pad->control_0x104 = 0;
    pad->fs_stick_trig_0x108 = 0;
    pad->fs_stick_release_0x10A = 0;
    pad->fs_angle_0x10C[0] = pad->fs_angle_0x10C[1] = 0;
    pad->fs_mag_0x110[0] = pad->fs_mag_0x110[1] = 0;
    pad->fs_deg_0x114[0] = pad->fs_deg_0x114[1] = 0;
    pad->fs_dist_0x118[0] = pad->fs_dist_0x118[1] = 0;
    pad->fs_dir_0x11C[0] = pad->fs_dir_0x11C[1] = 0;
}

/* Clears channel `chan`'s remote and menu button words while the pads are locked. */
extern "C" void pad_wpad_clear(s32 chan)
{
    PlayerPad* pad;

    pad = &Psw[chan];
    pad->field_0xca = 0;
    pad->core_prev_0xCC = 0;
    pad->field_0xce = 0;
    pad->core_release_0xD0 = 0;
    pad->button_0x2C0.hold_0x00 = 0;
    pad->button_0x2C0.pressed_0x04 = 0;
    pad->button_0x2C0.released_0x06 = 0;
    pad->button_0x2C0.held_0x14 = 0;
    pad->button_0x2C0.press_0x08 = 0;
    pad->button_0x2C0.trigger_0x0C = 0;
    pad->button_0x2C0.stick_released_0x0E = 0;
    pad->button_0x2C0.clear_0x10 = 0;
    pad->button_0x2C0.clear_0x12 = 0;
    pad->button_0x2C0.edge_on_0x1F = 0;
    pad->button_0x2C0.stick_deg_0x18 = 0;
    pad->button_0x2C0.stick_dist_0x1A = 0;
    pad->button_0x2C0.stick_dir_0x1C = 0;
    pad->button_0x2C0.edge_0x1D = 0;
    pad_input_clear(chan);
    pad->field_0xda = 0;
}

/* Parks channel `chan`'s button words while its remote is busy (or a movie plays). */
extern "C" void pad_wpad_busy(s32 chan)
{
    PlayerPad* pad;

    pad = &Psw[chan];
    pad->cl_prev_0xD8 = 2;
    pad->control_0x0D6 = 2;
    pad->field_0xda = 0;
    pad->cc_word2_0x0DC = 0;
    pad->cl_stick_trig_0xE2 = 0;
    pad->cl_stick_release_0xE4 = 0;
    pad->fs_prev_0xFE = 0x8000;
    pad->control_0x0FC = 0x8000;
    pad->field_0x100 = 0;
    pad->wpad_word2_0x102 = 0;
    pad->fs_stick_trig_0x108 = 0;
    pad->fs_stick_release_0x10A = 0;
    pad->core_prev_0xCC = 0x8000;
    pad->field_0xca = 0x8000;
    pad->field_0xce = 0;
    pad->core_release_0xD0 = 0;
    pad->button_0x2C0.pressed_0x04 = 0;
    pad->button_0x2C0.released_0x06 = 0;
    pad->button_0x2C0.held_0x14 = 0;
    pad->button_0x2C0.trigger_0x0C = 0;
    pad->button_0x2C0.stick_released_0x0E = 0;
    pad->button_0x2C0.edge_on_0x1F = 0;
    pad->button_0x2C0.prev_0x02 = 0;
    pad->button_0x2C0.hold_0x00 = 0;
}

/* Reads channel `chan`'s GameCube pad from the DemoPad table into its `Psw_prev` record. */
extern "C" void pad_gc_read(s32 chan)
{
    PlayerPad* pad;
    const u16* from;
    const u16* to;

    pad = &Psw_prev[chan];
    from = pad_btn_table_0.bit;
    to = pad_btn_map_out;
    pad->gc_err_prev_0x1E = pad->gc_err_0x1C;
    pad->gc_prev_0x02 = pad->gc_hold_0x00;
    pad->gc_stick_prev_0x0A = pad->gc_stick_hold_0x08;
    pad->gc_err_0x1C = (s8)rc_get_err(chan);
    if (pad->gc_err_0x1C == 0) {
        pad->gc_hold_0x00 = pad_btn_remap(rc_get_button(chan), from, to);
        pad_gc_stick_read(chan, pad);
    } else {
        pad->gc_hold_0x00 = 0;
        pad->gc_stick_hold_0x08 = 0;
        pad->gc_stick_angle_0x10[1] = 0;
        pad->gc_stick_angle_0x10[0] = 0;
        pad->gc_stick_mag_0x14[1] = 0;
        pad->gc_stick_mag_0x14[0] = 0;
    }
    pad->gc_trig_0x04 = pad->gc_hold_0x00 & ~pad->gc_prev_0x02;
    pad->gc_release_0x06 = pad->gc_prev_0x02 & ~pad->gc_hold_0x00;
    pad->gc_stick_trig_0x0C = pad->gc_stick_hold_0x08 & ~pad->gc_stick_prev_0x0A;
    pad->gc_stick_release_0x0E = pad->gc_stick_prev_0x0A & ~pad->gc_stick_hold_0x08;
    pad->gc_rept_0x18 = 0;
    if (pad->gc_prev_0x02 == pad->gc_hold_0x00) {
        pad->gc_rept_count_0x1A++;
        if (pad->gc_rept_count_0x1A > 12) {
            pad->gc_rept_count_0x1A = 10;
            pad->gc_rept_0x18 = pad->gc_hold_0x00;
        }
    } else {
        pad->gc_rept_count_0x1A = 0;
    }
}

extern "C" u16 rc_get_button(s32 idx)
{
    return DemoPad[idx].pst.button;
}

extern "C" u8 rc_get_err(s32 idx)
{
    return DemoPad[idx].pst.err;
}

/* Returns the 8-way direction bits of a stick angle `deg` (degrees). */
extern "C" u8 stick_dir8_get(u16 deg)
{
    return stick_dir8_table[(s32)(deg / 22.5f)];
}

/* Derives the GameCube pad's triggers, stick angles, distances and direction buttons from its DemoPad record. */
extern "C" void pad_gc_stick_read(s32 chan, PlayerPad* pad)
{
    s8 x[2];
    s8 y[2];
    s32 i;
    f32 ang;
    u16 bits;
    u8 dir;

    pad->gc_trigger_l_0x2C = rc_get_trigger_l(chan);
    pad->gc_trigger_r_0x2D = rc_get_trigger_r(chan);
    x[0] = rc_get_stick_x(chan);
    y[0] = rc_get_stick_y(chan);
    x[1] = rc_get_substick_x(chan);
    y[1] = rc_get_substick_y(chan);
    for (i = 0; i < 2; i++) {
        if (x[i] == 0 && y[i] == 0) {
            ang = 0.0f;
        } else {
            ang = pad_atan2_s8(y[i], x[i]);
            if (ang < 0.0f) {
                ang += 6.2831855f;
            }
        }
        pad->gc_stick_deg_0x22[i] = 360.0f * (ang / 6.2831855f);
        pad->gc_stick_dist_0x26[i] = pad_sqrt(x[i] * x[i] + y[i] * y[i]);
        if (pad->gc_stick_dist_0x26[i] > 127) {
            pad->gc_stick_dist_0x26[i] = 127;
        }
        if (pad->gc_stick_dist_0x26[i] < 30) {
            pad->gc_stick_deg_0x22[i] = 0;
            pad->gc_stick_dist_0x26[i] = 0;
        }
        pad->gc_stick_angle_0x10[i] = (pad->gc_stick_deg_0x22[i] << 16) / 360;
        pad->gc_stick_mag_0x14[i] = pad->gc_stick_dist_0x26[i];
        if (pad->gc_stick_dist_0x26[i] < 37) {
            pad->gc_stick_dir_0x2A[i] = 0;
        } else {
            pad->gc_stick_dir_0x2A[i] = stick_dir8_get(pad->gc_stick_deg_0x22[i]);
        }
    }
    bits = 0;
    dir = pad->gc_stick_dir_0x2A[0];
    if (dir & 1) {
        bits |= 0x2000;
    }
    if (dir & 2) {
        bits |= 0x1000;
    }
    if (dir & 4) {
        bits |= 0x800;
    }
    if (dir & 8) {
        bits |= 0x400;
    }
    dir = pad->gc_stick_dir_0x2A[1];
    if (dir & 1) {
        bits |= 0x20;
    }
    if (dir & 2) {
        bits |= 0x10;
    }
    if (dir & 4) {
        bits |= 0x8;
    }
    if (dir & 8) {
        bits |= 0x4;
    }
    pad->gc_stick_hold_0x08 = bits;
}

/* Returns sqrt(`v`). */
extern "C" f64 pad_sqrt(s32 v)
{
    return sqrt(v);
}

/* Returns atan2(`y`, `x`) of two signed bytes. */
extern "C" f64 pad_atan2_s8(s8 y, s8 x)
{
    return atan2(y, x);
}

extern "C" u8 rc_get_substick_y(s32 idx)
{
    return DemoPad[idx].pst.substickY;
}

extern "C" u8 rc_get_substick_x(s32 idx)
{
    return DemoPad[idx].pst.substickX;
}

extern "C" u8 rc_get_stick_y(s32 idx)
{
    return DemoPad[idx].pst.stickY;
}

extern "C" u8 rc_get_stick_x(s32 idx)
{
    return DemoPad[idx].pst.stickX;
}

extern "C" u8 rc_get_trigger_r(s32 idx)
{
    return DemoPad[idx].pst.triggerR;
}

extern "C" u8 rc_get_trigger_l(s32 idx)
{
    return DemoPad[idx].pst.triggerL;
}

/* Derives the Classic Controller's stick angles, distances and direction buttons (cleared for another device). */
extern "C" void pad_cl_stick_read(s32 chan, PlayerPad* pad)
{
    WPADCLStatus* cl;
    s16 x[2];
    s16 y[2];
    s32 i;
    PadStickBlock* blk;
    f32 ang;
    u16 bits;
    u8 dir;

    blk = (PadStickBlock*)&pad->control_0x0D6;
    if (pad->mode != 2) {
        memset(blk, 0, sizeof(PadStickBlock));
        return;
    }
    cl = (WPADCLStatus*)pad->cl_status_0x62;
    x[0] = cl->clLStickX;
    y[0] = cl->clLStickY;
    x[1] = cl->clRStickX;
    y[1] = cl->clRStickY;
    for (i = 0; i < 2; i++) {
        if (x[i] == 0 && y[i] == 0) {
            ang = 0.0f;
        } else {
            ang = pad_atan2_s16(y[i], x[i]);
            if (ang < 0.0f) {
                ang += 6.2831855f;
            }
        }
        blk->deg[i] = 360.0f * (ang / 6.2831855f);
        blk->dist[i] = pad_sqrt(x[i] * x[i] + y[i] * y[i]);
        if (blk->dist[i] > 127) {
            blk->dist[i] = 127;
        }
        if (blk->dist[i] < 30) {
            blk->deg[i] = 0;
            blk->dist[i] = 0;
        }
        blk->angle[i] = (blk->deg[i] << 16) / 360;
        blk->mag[i] = blk->dist[i];
        if (blk->dist[i] < 37) {
            blk->dir[i] = 0;
        } else {
            blk->dir[i] = stick_dir8_get(blk->deg[i]);
        }
    }
    bits = 0;
    dir = blk->dir[0];
    if (dir & 1) {
        bits |= 0x2000;
    }
    if (dir & 2) {
        bits |= 0x1000;
    }
    if (dir & 4) {
        bits |= 0x800;
    }
    if (dir & 8) {
        bits |= 0x400;
    }
    dir = blk->dir[1];
    if (dir & 1) {
        bits |= 0x20;
    }
    if (dir & 2) {
        bits |= 0x10;
    }
    if (dir & 4) {
        bits |= 0x8;
    }
    if (dir & 8) {
        bits |= 0x4;
    }
    blk->stick_hold = bits;
}

/* Returns atan2(`y`, `x`) of two signed shorts. */
extern "C" f64 pad_atan2_s16(s16 y, s16 x)
{
    return atan2(y, x);
}

/* Derives stick `i`'s degree, distance, angle, magnitude and direction bits of `blk` from its reading. */
static inline void pad_stick_calc(PadStickBlock* blk, s32 i, s16 x, s16 y)
{
    f32 ang;

    if (x == 0 && y == 0) {
        ang = 0.0f;
    } else {
        ang = pad_atan2_s16(y, x);
        if (ang < 0.0f) {
            ang += 6.2831855f;
        }
    }
    blk->deg[i] = 360.0f * (ang / 6.2831855f);
    blk->dist[i] = pad_sqrt(x * x + y * y);
    if (blk->dist[i] > 127) {
        blk->dist[i] = 127;
    }
    if (blk->dist[i] < 30) {
        blk->deg[i] = 0;
        blk->dist[i] = 0;
    }
    blk->angle[i] = (blk->deg[i] << 16) / 360;
    blk->mag[i] = blk->dist[i];
    if (blk->dist[i] < 37) {
        blk->dir[i] = 0;
    } else {
        blk->dir[i] = stick_dir8_get(blk->deg[i]);
    }
}

/* Derives the Nunchuk stick's angle, distance and direction buttons (the block is cleared for another device). */
extern "C" void pad_fs_stick_read(s32 chan, PlayerPad* pad)
{
    PadFsBlock* fsb;
    WPADFSStatus* fs;
    s16 y[1];
    s16 x[1];
    s32 i;
    u16 bits;
    u8 dir;

    fsb = (PadFsBlock*)&pad->control_0x0FC;
    if (pad->mode != 1) {
        memset(fsb, 0, sizeof(PadFsBlock));
        return;
    }
    fs = (WPADFSStatus*)pad->fs_status_0x98;
    x[0] = fs->fsStickX << 1;
    y[0] = fs->fsStickY << 1;
    for (i = 0; i < 1; i++) {
        pad_stick_calc(&fsb->btn, i, x[i], y[i]);
    }
    bits = 0;
    dir = fsb->btn.dir[0];
    if (dir & 1) {
        bits |= 0x2000;
    }
    if (dir & 2) {
        bits |= 0x1000;
    }
    if (dir & 4) {
        bits |= 0x800;
    }
    if (dir & 8) {
        bits |= 0x400;
    }
    dir = fsb->btn.dir[1];
    if (dir & 1) {
        bits |= 0x20;
    }
    if (dir & 2) {
        bits |= 0x10;
    }
    if (dir & 4) {
        bits |= 0x8;
    }
    if (dir & 8) {
        bits |= 0x4;
    }
    fsb->btn.stick_hold = bits;
}

/* The WPAD connect callback: on a connection it picks the data format and sampling ring for the device. */
extern "C" void pad_connect_cb(s32 chan, s32 result)
{
    u32 type;

    switch (result) {
    case 0:
        if (system_w.pad_lock_ck_0x8d8() == 1) {
            if (pad_kpad_ready == 0) {
                WPADSetExtensionCallback(chan, pad_extension_cb);
                WPADSetSamplingCallback(chan, pad_sampling_cb);
                initKPAD();
            }
        } else {
            WPADSetExtensionCallback(chan, pad_extension_cb);
            WPADSetSamplingCallback(chan, pad_sampling_cb);
        }
        WPADProbe(chan, &type);
        Psw[chan].field_0x30 = result;
        switch (type) {
        case 0:
        case 251:
        case 252:
            WPADControlDpd(chan, 0, 0);
            WPADSetDataFormat(chan, 2);
            WPADSetAutoSamplingBuf(chan, wpad_sampling_buf_fmt2[chan], 100);
            WPADControlSpeaker(chan, 0, 0);
            system_w.field_0x7e0[chan] = 120;
            break;
        case 1:
            WPADControlDpd(chan, 0, 0);
            WPADSetDataFormat(chan, 5);
            WPADSetAutoSamplingBuf(chan, wpad_sampling_buf_fmt5[chan], 100);
            WPADControlSpeaker(chan, 0, 0);
            system_w.field_0x7e0[chan] = 120;
            break;
        case 2:
            WPADControlDpd(chan, 0, 0);
            WPADSetDataFormat(chan, 8);
            WPADSetAutoSamplingBuf(chan, wpad_sampling_buf_fmt8[chan], 100);
            WPADControlSpeaker(chan, 0, 0);
            system_w.field_0x7e0[chan] = 120;
            break;
        default:
            system_w.field_0x7e0[chan] = 120;
            break;
        }
        break;
    case -1:
        Psw[chan].field_0x30 = result;
        pad_wpad_clear(chan);
        break;
    default:
        Psw[chan].field_0x30 = result;
        break;
    }
    kpad_work_reset();
}

/* ------------------------------------------------------------------ *
 * 0x800438A4 - 0x80045330  (pad callbacks and vector copies)
 * ------------------------------------------------------------------ */

/* The WPAD extension callback: records the new device type and picks its data format and sampling ring. */
extern "C" void pad_extension_cb(s32 chan, s32 type)
{
    Psw[chan].mode = type;
    switch (type) {
    case 0:
        WPADControlDpd(chan, 0, 0);
        WPADSetDataFormat(chan, 2);
        WPADSetAutoSamplingBuf(chan, wpad_sampling_buf_fmt2[chan], 100);
        break;
    case 1:
        WPADControlDpd(chan, 0, 0);
        WPADSetDataFormat(chan, 5);
        WPADSetAutoSamplingBuf(chan, wpad_sampling_buf_fmt5[chan], 100);
        break;
    case 2:
        WPADControlDpd(chan, 0, 0);
        WPADSetDataFormat(chan, 8);
        WPADSetAutoSamplingBuf(chan, wpad_sampling_buf_fmt8[chan], 100);
        break;
    case 251:
        WPADControlDpd(chan, 0, 0);
        WPADSetDataFormat(chan, 2);
        WPADSetAutoSamplingBuf(chan, wpad_sampling_buf_fmt2[chan], 100);
        break;
    case 252:
        WPADControlDpd(chan, 0, 0);
        WPADSetDataFormat(chan, 2);
        WPADSetAutoSamplingBuf(chan, wpad_sampling_buf_fmt2[chan], 100);
        break;
    }
}

/* The WPAD sampling callback: nothing to do per sample. */
extern "C" void pad_sampling_cb(s32 chan)
{
}

extern "C" void pad_sync_device_cb(s32 arg0)
{
    if (arg0 == 0) {
        WPADStartSyncDevice();
    }
}

extern "C" void pad_clear_device_cb(s32 arg0)
{
    if (arg0 == 0) {
        WPADStartClearDevice();
    }
}

/* The retail body is a bare `blr`; the parameter is the record the call sites pass. */
extern "C" void VEC3_ctor(VEC3* out)
{
    (void)out;
}


/* Copies the float pair `src` into `dst` and returns `dst`. */
extern "C" Vec2* vec2_copy(Vec2* dst, const Vec2* src)
{
    dst->x = src->x;
    dst->y = src->y;
    return dst;
}

extern "C" void vec3_float_copy(f32* dst, f32* src)
{
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
}

/* ------------------------------------------------------------------ *
 * 0x800438C8 - 0x800441F8  (Psw pad-record accessors)
 * ------------------------------------------------------------------ */

/* Copies the newest `count` remote accelerometer samples of channel `chan` into `out`, newest first. */
extern "C" void pad_acc_hist_get(s32 chan, VEC3* out, u8 count)
{
    PadFsBlock* fsb;
    u8 i;
    u8 idx;

    fsb = (PadFsBlock*)&Psw[chan].control_0x0FC;
    if (out != NULL) {
        idx = fsb->index;
        for (i = 0; i < count; i++) {
            copyVec3(out++, (VEC3*)fsb->acc[idx]);
            idx = (idx - 1) & 0xF;
        }
    }
}

/* Copies the newest `count` Nunchuk accelerometer samples of channel `chan` into `out`, newest first. */
extern "C" void pad_fs_acc_hist_get(s32 chan, VEC3* out, u8 count)
{
    PadFsBlock* fsb;
    u8 i;
    u8 idx;

    fsb = (PadFsBlock*)&Psw[chan].control_0x0FC;
    if (out != NULL) {
        idx = fsb->index;
        for (i = 0; i < count; i++) {
            copyVec3(out++, (VEC3*)fsb->fs_acc[idx]);
            idx = (idx - 1) & 0xF;
        }
    }
}

/* Returns how far the remote's Y acceleration moved since the previous frame. */
extern "C" s32 pad_acc_y_delta(s32 chan)
{
    PadFsBlock* fsb;
    u8 idx;
    f32* prev;
    f32* cur;
    f32 delta;

    fsb = (PadFsBlock*)&Psw[chan].control_0x0FC;
    idx = fsb->index;
    cur = fsb->acc[idx];
    prev = fsb->acc[(idx - 1) & 0xF];
    delta = prev[1] - cur[1];
    if (delta < 0.0f) {
        delta = -delta;
    }
    return delta;
}

/* Returns how far the Nunchuk's Y acceleration moved since the previous frame. */
extern "C" s32 pad_fs_acc_y_delta(s32 chan)
{
    PadFsBlock* fsb;
    u8 idx;
    f32* prev;
    f32* cur;
    f32 delta;

    fsb = (PadFsBlock*)&Psw[chan].control_0x0FC;
    idx = fsb->index;
    cur = fsb->fs_acc[idx];
    prev = fsb->fs_acc[(idx - 1) & 0xF];
    delta = prev[1] - cur[1];
    if (delta < 0.0f) {
        delta = -delta;
    }
    return delta;
}

/* Feeds the pointer the samples channel 0 buffered since the last frame (Classic Controller or Nunchuk format). */
extern "C" void kpad_pointer_read(void)
{
    u32 fmt;
    u16 idx;
    KpadWork* kp;

    pointer_valid = pointer_dpd_state;
    if ((u8)get_ControlType(0) != 255) {
        fmt = WPADGetDataFormat(0);
        if (fmt == 8 || fmt == 5) {
            kp = &kpad_work;
            WPADAccGravityUnit grav = { 1, 1, 1 };
            WPADGetAccGravityUnit(0, 0, &grav);
            if (grav.x * grav.y * grav.z != 0) {
                kp->acc_scale.x = 1.0f / grav.x;
                kp->acc_scale.y = 1.0f / grav.y;
                kp->acc_scale.z = 1.0f / grav.z;
            } else {
                kp->acc_scale.x = 0.01f;
                kp->acc_scale.y = 0.01f;
                kp->acc_scale.z = 0.01f;
            }
            idx = wpad_latest_index_prev;
            switch (fmt) {
            case 8:
                while (idx != wpad_latest_index) {
                    kpad_read_acc(&wpad_sampling_buf_fmt8[0][idx].core);
                    kpad_read_dpd(&wpad_sampling_buf_fmt8[0][idx].core);
                    idx = (idx + 101) % 100;
                }
                break;
            case 5:
                while (idx != wpad_latest_index) {
                    kpad_read_acc(&wpad_sampling_buf_fmt5[0][idx].core);
                    kpad_read_dpd(&wpad_sampling_buf_fmt5[0][idx].core);
                    idx = (idx + 101) % 100;
                }
                break;
            }
        }
    }
    if (pointer_dpd_state != 0 && pointer_valid == 0) {
        pointer_dpd_state = 2;
    }
}

/* Recomputes the pointer-to-screen scale from the pointer centre. */
extern "C" void kpad_calc_dpd2pos_scale(KpadWork* kp)
{
    f32 fx;
    f32 fy;
    f32 f1;

    fx = 1.0f;
    fy = 0.75f;
    f1 = sqrtf(fx * fx + fy * fy);
    if (kp->center_org.x < 0.0f) {
        fx += kp->center_org.x;
    } else {
        fx -= kp->center_org.x;
    }
    if (kp->center_org.y < 0.0f) {
        fy += kp->center_org.y;
    } else {
        fy -= kp->center_org.y;
    }
    kp->dpd2pos_scale = f1 / ((fx < fy) ? fx : fy);
}

/* Initialises the pointer work: the calibration defaults, the play radii and the derived distances. */
extern "C" void kpad_work_init(void)
{
    KpadWork* kp;
    f32 y;

    memset(&kpad_work, 0, sizeof(KpadWork));
    kp_err_dist_max = 1.0f + (f32)WPADGetDpdSensitivity();
    kp = &kpad_work;
    kp->dist_org = kpad_idist_org;
    vec2_copy(&kp->accXY_nrm_hori, &kpad_iaccXY_nrm_hori);
    vec2_copy(&kp->sec_nrm11, &kpad_isec_nrm11);
    vec2_copy(&kp->center_org, &kpad_dpd_center_init);
    if (WPADGetSensorBarPosition() == 1) {
        y = 0.2f;
    } else {
        y = -0.2f;
    }
    kp->center_org.x = 0.0f;
    kp->center_org.y = -y;
    kpad_calc_dpd2pos_scale(kp);
    kp->pos_play_radius = kp->hori_play_radius = kp->dist_play_radius = kp->acc_play_radius = 0.0f;
    kp->pos_sensitivity = kp->hori_sensitivity = kp->dist_sensitivity = kp->acc_sensitivity = 1.0f;
    kp->pos_play_mode = kp->hori_play_mode = kp->dist_play_mode = kp->acc_play_mode = 0;
    pad_kp_dist_init = kp_err_dist_min = kp_obj_interval / 0.383864f;
    kpad_work_reset();
}

/* Resets the pointer work's derived limits and its pointer, distance and acceleration state. */
extern "C" void kpad_work_reset(void)
{
    KpadWork* kp;
    KPADStatus* sp;
    KPADObj* op;

    kp = &kpad_work;
    sp = &kp->status;
    kp->frame_min.x = -1.0f + kp_err_outside_frame;
    kp->frame_max.x = 1.0f - kp_err_outside_frame;
    kp->frame_min.y = -0.75f + kp_err_outside_frame;
    kp->frame_max.y = 0.75f - kp_err_outside_frame;
    kp->dist_speed_inc = 1.0f / kp_err_dist_speed;
    kp->dist_speed_dec = -1.0f / kp_err_dist_speed;
    kp->ah_circle_radius2 = kp_ah_circle_radius * kp_ah_circle_radius;
    kp->dist_min = kp_err_dist_min;
    kp->dist_scale = pad_kp_dist_init;
    sp->dpd_valid_fg = 0;
    kp->trust_sec_ct = 0;
    vec2_copy(&sp->pos, vec2_copy(&sp->vec, &kpad_icenter_org));
    sp->speed = 0.0f;
    sp->horizon.x = kp->acc_horizon.x = kp->dpd_horizon.x = 1.0f;
    sp->horizon.y = kp->acc_horizon.y = kp->dpd_horizon.y = 0.0f;
    vec2_copy(&sp->hori_vec, &kpad_icenter_org);
    sp->hori_speed = 0.0f;
    sp->acc_vertical.x = 1.0f;
    sp->acc_vertical.y = 0.0f;
    sp->dist = kp->dist_org;
    sp->dist_vec = sp->dist_speed = 0.0f;
    kp->dist_last = kp->dist_org;
    kp->sec_dist = kp->trust_sec_dist = kp->dist_scale / kp->dist_org;
    vec2_copy(&kp->sec_nrm, &kp->sec_nrm11);
    sp->acc.x = sp->acc.z = 0.0f;
    sp->acc.y = -1.0f;
    sp->acc_value = 1.0f;
    sp->acc_speed = 0.0f;
    vec3_float_copy(&kp->hard_acc.x, &sp->acc.x);
    vec2_copy(&kp->ah_circle_pos, &kp->acc_horizon);
    kp->ah_circle_ct = kp_ah_circle_ct;
    kp->valid_objs = 0;
    op = &kp->kobj_sample[3];
    do {
        op->error_fg = -1;
    } while (--op >= kp->kobj_sample);
    op = &kp->kobj_regular[1];
    do {
        op->error_fg = -1;
    } while (--op >= kp->kobj_regular);
    wpad_latest_index_prev = 0;
    wpad_latest_index = 0;
}

/* Moves `*acc` towards `acc2` by the acceleration play radius and sensitivity. */
extern "C" void kpad_calc_acc(KpadWork* kp, f32* acc, f32 acc2)
{
    f32 f1;
    f32 f2;

    f2 = acc2 - *acc;
    if (kp->acc_play_mode == 0) {
        if (f2 < 0.0f) {
            f1 = -f2;
        } else {
            f1 = f2;
        }
        if (f1 >= kp->acc_play_radius) {
            f1 = 1.0f;
        } else {
            f1 /= kp->acc_play_radius;
            f1 *= f1;
            f1 *= f1;
        }
        f1 *= kp->acc_sensitivity;
        *acc += f1 * f2;
    } else {
        if (f2 < -kp->acc_play_radius) {
            *acc += kp->acc_sensitivity * (f2 + kp->acc_play_radius);
        } else if (f2 > kp->acc_play_radius) {
            *acc += kp->acc_sensitivity * (f2 - kp->acc_play_radius);
        }
    }
}

/* Tracks the remote's roll from the X/Y acceleration and counts how long it stays still. */
extern "C" void kpad_calc_acc_horizon(KpadWork* kp)
{
    f32 f1;
    f32 vx;
    f32 vy;
    f32 ax;
    f32 ay;

    f1 = sqrtf(kp->hard_acc.x * kp->hard_acc.x + kp->hard_acc.y * kp->hard_acc.y);
    if (f1 == 0.0f || f1 >= 2.0f) {
        return;
    }
    ax = kp->hard_acc.x / f1;
    ay = kp->hard_acc.y / f1;
    if (f1 > 1.0f) {
        f1 = 2.0f - f1;
    }
    f1 *= f1 * kp_acc_horizon_pw;
    vx = kp->accXY_nrm_hori.x * ax + kp->accXY_nrm_hori.y * ay;
    vy = kp->accXY_nrm_hori.y * ax - kp->accXY_nrm_hori.x * ay;
    vx = kp->acc_horizon.x + f1 * (vx - kp->acc_horizon.x);
    vy = kp->acc_horizon.y + f1 * (vy - kp->acc_horizon.y);
    f1 = sqrtf(vx * vx + vy * vy);
    if (f1 == 0.0f) {
        return;
    }
    kp->acc_horizon.x = vx / f1;
    kp->acc_horizon.y = vy / f1;
    kp->ah_circle_pos.x += kp_ah_circle_pw * (kp->acc_horizon.x - kp->ah_circle_pos.x);
    kp->ah_circle_pos.y += kp_ah_circle_pw * (kp->acc_horizon.y - kp->ah_circle_pos.y);
    vx = kp->acc_horizon.x - kp->ah_circle_pos.x;
    vy = kp->acc_horizon.y - kp->ah_circle_pos.y;
    if (vx * vx + vy * vy <= kp->ah_circle_radius2) {
        if (kp->ah_circle_ct != 0) {
            --kp->ah_circle_ct;
        }
    } else {
        kp->ah_circle_ct = kp_ah_circle_ct;
    }
}

/* Tracks the remote's pitch from the acceleration. */
extern "C" void kpad_calc_acc_vertical(KpadWork* kp)
{
    f32 vx;
    f32 f1;
    f32 vy;
    f32 ax;
    f32 ay;

    f1 = kp->hard_acc.x * kp->hard_acc.x + kp->hard_acc.y * kp->hard_acc.y;
    vx = sqrtf(f1);
    ay = -kp->hard_acc.z;
    f1 = sqrtf(f1 + ay * ay);
    if (f1 == 0.0f || f1 >= 2.0f) {
        return;
    }
    ax = vx / f1;
    ay /= f1;
    if (f1 > 1.0f) {
        f1 = 2.0f - f1;
    }
    f1 *= f1 * kp_acc_horizon_pw;
    vx = kp->status.acc_vertical.x + f1 * (ax - kp->status.acc_vertical.x);
    vy = kp->status.acc_vertical.y + f1 * (ay - kp->status.acc_vertical.y);
    f1 = sqrtf(vx * vx + vy * vy);
    if (f1 == 0.0f) {
        return;
    }
    kp->status.acc_vertical.x = vx / f1;
    kp->status.acc_vertical.y = vy / f1;
}

/* Converts one sample's remote acceleration and smooths it into the pointer work. */
extern "C" void kpad_read_acc(WPADStatus* s)
{
    KpadWork* kp;
    Vec prev;

    kp = &kpad_work;
    kp->hard_acc.x = clamp_acc((f32)-s->accX * kp->acc_scale.x, pad_rm_acc_max);
    kp->hard_acc.y = clamp_acc((f32)-s->accZ * kp->acc_scale.z, pad_rm_acc_max);
    kp->hard_acc.z = clamp_acc((f32)s->accY * kp->acc_scale.y, pad_rm_acc_max);
    vec3_float_copy(&prev.x, &kp->status.acc.x);
    kpad_calc_acc(kp, &kp->status.acc.x, kp->hard_acc.x);
    kpad_calc_acc(kp, &kp->status.acc.y, kp->hard_acc.y);
    kpad_calc_acc(kp, &kp->status.acc.z, kp->hard_acc.z);
    kp->status.acc_value = sqrtf(kp->status.acc.x * kp->status.acc.x + kp->status.acc.y * kp->status.acc.y
                                 + kp->status.acc.z * kp->status.acc.z);
    prev.x -= kp->status.acc.x;
    prev.y -= kp->status.acc.y;
    prev.z -= kp->status.acc.z;
    kp->status.acc_speed = sqrtf(prev.x * prev.x + prev.y * prev.y + prev.z * prev.z);
    kpad_calc_acc_horizon(kp);
    kpad_calc_acc_vertical(kp);
}

/* Converts the four pointer objects of a sample (`obj` is the last one) into the work's object samples. */
extern "C" void kpad_obj_from_dpd(KpadWork* kp, WPADDpdObject* obj)
{
    KPADObj* op;

    op = &kp->kobj_sample[3];
    do {
        if (obj->size != 0) {
            op->center.x = 0.001953125f * obj->x - 0.9990234375f;
            op->center.y = 0.001953125f * obj->y - 0.7490234375f;
            op->error_fg = 0;
            op->state_fg = 0;
        } else {
            op->error_fg = -1;
        }
        obj--;
    } while (--op >= kp->kobj_sample);
}

/* Flags (bit 0) every valid object sample that lies outside the usable frame. */
extern "C" void kpad_obj_frame_check(KpadWork* kp, KPADObj* objs)
{
    KPADObj* op;

    op = &objs[3];
    do {
        if (op->error_fg < 0) {
            continue;
        }
        if (op->center.x <= kp->frame_min.x || op->center.x >= kp->frame_max.x || op->center.y <= kp->frame_min.y
            || op->center.y >= kp->frame_max.y) {
            op->error_fg |= 1;
        }
    } while (--op >= objs);
}

/* Flags (bit 1) every object sample that repeats an earlier one's position. */
extern "C" void kpad_obj_dup_check(KPADObj* objs)
{
    KPADObj* op2;
    KPADObj* op1;

    op2 = objs;
    do {
        if (op2->error_fg != 0) {
            continue;
        }
        op1 = op2 + 1;
        do {
            if (op1->error_fg == 0 && op2->center.x == op1->center.x && op2->center.y == op1->center.y) {
                op1->error_fg |= 2;
            }
        } while (++op1 <= &objs[3]);
    } while (++op2 < &objs[3]);
}

/* Picks the pair of pointer objects closest to last frame's pair in distance and direction. */
extern "C" s8 kpad_select_2obj_continue(KpadWork* kp)
{
    KPADObj* op1;
    KPADObj* op2;
    KPADObj* rp1;
    KPADObj* rp2;
    Vec2 nrm;
    s32 rev_fg;
    f32 vx;
    f32 vy;
    f32 f1;
    f32 dist;
    f32 min;

    min = 2.0f;
    op1 = kp->kobj_sample;
    do {
        if (op1->error_fg != 0) {
            continue;
        }
        op2 = op1 + 1;
        do {
            if (op2->error_fg != 0) {
                continue;
            }
            vx = op2->center.x - op1->center.x;
            vy = op2->center.y - op1->center.y;
            dist = 1.0f / sqrtf(vx * vx + vy * vy);
            nrm.x = vx * dist;
            nrm.y = vy * dist;
            dist *= kp->dist_scale;
            if (dist <= kp->dist_min || dist >= kp_err_dist_max) {
                continue;
            }
            dist -= kp->dist_last;
            if (dist < 0.0f) {
                dist *= kp->dist_speed_dec;
            } else {
                dist *= kp->dist_speed_inc;
            }
            if (dist >= 1.0f) {
                continue;
            }
            f1 = kp->sec_nrm.x * nrm.x + kp->sec_nrm.y * nrm.y;
            if (f1 < 0.0f) {
                f1 = -f1;
                rev_fg = 1;
            } else {
                rev_fg = 0;
            }
            if (f1 <= kp_err_next_inpr) {
                continue;
            }
            dist += (1.0f - f1) / (1.0f - kp_err_next_inpr);
            if (dist < min) {
                min = dist;
                if (rev_fg) {
                    rp1 = op2;
                    rp2 = op1;
                } else {
                    rp1 = op1;
                    rp2 = op2;
                }
            }
        } while (++op2 <= &kp->kobj_sample[3]);
    } while (++op1 < &kp->kobj_sample[3]);
    if (min == 2.0f) {
        return 0;
    }
    kpad_obj_copy(&kp->kobj_regular[0], rp1);
    kpad_obj_copy(&kp->kobj_regular[1], rp2);
    return 2;
}

#pragma peephole on
/* Copies the pointer object `src` into `dst`. */
extern "C" void kpad_obj_copy(KPADObj* dst, const KPADObj* src)
{
    dst->center = src->center;
    dst->error_fg = src->error_fg;
    dst->state_fg = src->state_fg;
    dst->pad_0x0A[0] = src->pad_0x0A[0];
    dst->pad_0x0A[1] = src->pad_0x0A[1];
}
#pragma peephole off

/* Follows last frame's tracked object to the nearest sample and rebuilds its partner. */
extern "C" s8 kpad_select_1obj_continue(KpadWork* kp)
{
    KPADObj* op1;
    KPADObj* op2;
    KPADObj* rp1;
    KPADObj* rp2;
    f32 vx;
    f32 vy;
    f32 f1;
    f32 min;

    min = kp_err_near_pos * kp_err_near_pos;
    op1 = kp->kobj_regular;
    do {
        if (op1->error_fg != 0 || op1->state_fg != 0) {
            continue;
        }
        op2 = kp->kobj_sample;
        do {
            if (op2->error_fg != 0) {
                continue;
            }
            vx = op1->center.x - op2->center.x;
            vy = op1->center.y - op2->center.y;
            f1 = vx * vx + vy * vy;
            if (f1 < min) {
                min = f1;
                rp1 = op1;
                rp2 = op2;
            }
        } while (++op2 < &kp->kobj_sample[4]);
    } while (++op1 < &kp->kobj_regular[2]);
    if (min == kp_err_near_pos * kp_err_near_pos) {
        return 0;
    }
    kpad_obj_copy(rp1, rp2);
    kp->sec_nrm.x = kp->sec_nrm11.x * kp->acc_horizon.x + kp->sec_nrm11.y * kp->acc_horizon.y;
    kp->sec_nrm.y = kp->sec_nrm11.y * kp->acc_horizon.x - kp->sec_nrm11.x * kp->acc_horizon.y;
    vx = kp->sec_dist * kp->sec_nrm.x;
    vy = kp->sec_dist * kp->sec_nrm.y;
    if (rp1 == &kp->kobj_regular[0]) {
        kp->kobj_regular[1].center.x = rp1->center.x + vx;
        kp->kobj_regular[1].center.y = rp1->center.y + vy;
        kp->kobj_regular[1].error_fg = 0;
        kp->kobj_regular[1].state_fg = -1;
    } else {
        kp->kobj_regular[0].center.x = rp1->center.x - vx;
        kp->kobj_regular[0].center.y = rp1->center.y - vy;
        kp->kobj_regular[0].error_fg = 0;
        kp->kobj_regular[0].state_fg = -1;
    }
    if (kp->status.dpd_valid_fg < 0) {
        return -1;
    }
    return 1;
}

/* Returns the distance a pair of pointer objects implies and writes their direction (rotated by the sensor
 * bar's normal) to `nrm`. */
extern "C" f32 kpad_obj_pair_dist(KpadWork* kp, KPADObj* op1, KPADObj* op2, Vec2* nrm)
{
    f32 vx;
    f32 vy;
    f32 dist;

    vx = op2->center.x - op1->center.x;
    vy = op2->center.y - op1->center.y;
    dist = 1.0f / sqrtf(vx * vx + vy * vy);
    vx *= dist;
    vy *= dist;
    nrm->x = kp->sec_nrm11.x * vx + kp->sec_nrm11.y * vy;
    nrm->y = kp->sec_nrm11.y * vx - kp->sec_nrm11.x * vy;
    return kp->dist_scale * dist;
}

/* Picks the first pair of pointer objects that lines up with the remote's roll. */
extern "C" s8 kpad_select_2obj_first(KpadWork* kp)
{
    KPADObj* op1;
    KPADObj* op2;
    KPADObj* rp1;
    KPADObj* rp2;
    Vec2 nrm;
    f32 f1;
    f32 dist;
    f32 max;

    max = kp_err_first_inpr;
    op1 = kp->kobj_sample;
    do {
        if (op1->error_fg != 0) {
            continue;
        }
        op2 = op1 + 1;
        do {
            if (op2->error_fg != 0) {
                continue;
            }
            dist = kpad_obj_pair_dist(kp, op1, op2, &nrm);
            if (dist <= kp->dist_min || dist >= kp_err_dist_max) {
                continue;
            }
            f1 = kp->acc_horizon.x * nrm.x + kp->acc_horizon.y * nrm.y;
            if (f1 < 0.0f) {
                if (-f1 > max) {
                    max = -f1;
                    rp1 = op2;
                    rp2 = op1;
                }
            } else {
                if (f1 > max) {
                    max = f1;
                    rp1 = op1;
                    rp2 = op2;
                }
            }
        } while (++op2 <= &kp->kobj_sample[3]);
    } while (++op1 < &kp->kobj_sample[3]);
    if (max == kp_err_first_inpr) {
        return 0;
    }
    kpad_obj_copy(&kp->kobj_regular[0], rp1);
    kpad_obj_copy(&kp->kobj_regular[1], rp2);
    return 2;
}

/* Pairs a lone pointer object with a partner placed outside the frame along the roll. */
extern "C" s8 kpad_select_1obj_first(KpadWork* kp)
{
    KPADObj* op1;
    Vec2 p1;
    Vec2 p2;
    f32 vx;
    f32 vy;

    vx = kp->sec_nrm11.x * kp->acc_horizon.x + kp->sec_nrm11.y * kp->acc_horizon.y;
    vy = kp->sec_nrm11.y * kp->acc_horizon.x - kp->sec_nrm11.x * kp->acc_horizon.y;
    vx *= kp->trust_sec_dist;
    vy *= kp->trust_sec_dist;
    op1 = kp->kobj_sample;
    do {
        if (op1->error_fg != 0) {
            continue;
        }
        p1.x = op1->center.x - vx;
        p1.y = op1->center.y - vy;
        p2.x = op1->center.x + vx;
        p2.y = op1->center.y + vy;
        if (p1.x <= kp->frame_min.x || p1.x >= kp->frame_max.x || p1.y <= kp->frame_min.y
            || p1.y >= kp->frame_max.y) {
            if (p2.x > kp->frame_min.x && p2.x < kp->frame_max.x && p2.y > kp->frame_min.y
                && p2.y < kp->frame_max.y) {
                kpad_obj_copy(&kp->kobj_regular[1], op1);
                vec2_copy(&kp->kobj_regular[0].center, &p1);
                kp->kobj_regular[0].error_fg = 0;
                kp->kobj_regular[0].state_fg = -1;
                return -1;
            }
        } else {
            if (p2.x <= kp->frame_min.x || p2.x >= kp->frame_max.x || p2.y <= kp->frame_min.y
                || p2.y >= kp->frame_max.y) {
                kpad_obj_copy(&kp->kobj_regular[0], op1);
                vec2_copy(&kp->kobj_regular[1].center, &p2);
                kp->kobj_regular[1].error_fg = 0;
                kp->kobj_regular[1].state_fg = -1;
                return -1;
            }
        }
    } while (++op1 < &kp->kobj_sample[4]);
    return 0;
}

/* Derives the sensor-bar distance, direction and horizon from the tracked object pair. */
extern "C" void kpad_sec_update(KpadWork* kp)
{
    f32 vx;
    f32 vy;
    f32 f1;

    vx = kp->kobj_regular[1].center.x - kp->kobj_regular[0].center.x;
    vy = kp->kobj_regular[1].center.y - kp->kobj_regular[0].center.y;
    kp->sec_dist = sqrtf(vx * vx + vy * vy);
    f1 = 1.0f / kp->sec_dist;
    kp->dist_last = kp->dist_scale * f1;
    kp->sec_nrm.x = vx *= f1;
    kp->sec_nrm.y = vy *= f1;
    kp->dpd_horizon.x = kp->sec_nrm11.x * vx + kp->sec_nrm11.y * vy;
    kp->dpd_horizon.y = kp->sec_nrm11.y * vx - kp->sec_nrm11.x * vy;
}

/* Derives the pointer's horizon, distance and screen position from the tracked object pair. */
extern "C" void kpad_calc_dpd_variable(KpadWork* kp, s8 valid)
{
    f32 f1, f2;
    Vec2 pos, vec;

    if (valid == 0) {
        kp->status.dpd_valid_fg = 0;
        return;
    }

    pos.x = kp->sec_nrm11.x * kp->sec_nrm.x + kp->sec_nrm11.y * kp->sec_nrm.y;
    pos.y = kp->sec_nrm11.y * kp->sec_nrm.x - kp->sec_nrm11.x * kp->sec_nrm.y;
    if (kp->status.dpd_valid_fg == 0) {
        vec2_copy(&kp->status.horizon, &pos);
        vec2_copy(&kp->status.hori_vec, &kpad_icenter_org);
        kp->status.hori_speed = 0.0f;
    } else {
        vec.x = pos.x - kp->status.horizon.x;
        vec.y = pos.y - kp->status.horizon.y;
        f1 = sqrtf(vec.x * vec.x + vec.y * vec.y);
        if (kp->hori_play_mode == 0) {
            if (f1 >= kp->hori_play_radius) {
                f2 = 1.0f;
            } else {
                f2 = f1 / kp->hori_play_radius;
                f2 *= f2;
                f2 *= f2;
            }
            f2 *= kp->hori_sensitivity;
            vec.x = kp->status.horizon.x + f2 * vec.x;
            vec.y = kp->status.horizon.y + f2 * vec.y;
            f1 = sqrtf(vec.x * vec.x + vec.y * vec.y);
            vec.x /= f1;
            vec.y /= f1;
            kp->status.hori_vec.x = vec.x - kp->status.horizon.x;
            kp->status.hori_vec.y = vec.y - kp->status.horizon.y;
            kp->status.hori_speed = sqrtf(kp->status.hori_vec.x * kp->status.hori_vec.x
                                              + kp->status.hori_vec.y * kp->status.hori_vec.y);
            vec2_copy(&kp->status.horizon, &vec);
        } else if (f1 > kp->hori_play_radius) {
            f2 = kp->hori_sensitivity * ((f1 - kp->hori_play_radius) / f1);
            vec.x = kp->status.horizon.x + vec.x * f2;
            vec.y = kp->status.horizon.y + vec.y * f2;
            f1 = sqrtf(vec.x * vec.x + vec.y * vec.y);
            vec.x /= f1;
            vec.y /= f1;
            kp->status.hori_vec.x = vec.x - kp->status.horizon.x;
            kp->status.hori_vec.y = vec.y - kp->status.horizon.y;
            kp->status.hori_speed = sqrtf(kp->status.hori_vec.x * kp->status.hori_vec.x
                                              + kp->status.hori_vec.y * kp->status.hori_vec.y);
            vec2_copy(&kp->status.horizon, &vec);
        } else {
            vec2_copy(&kp->status.hori_vec, &kpad_icenter_org);
            kp->status.hori_speed = 0.0f;
        }
    }

    f1 = kp->dist_scale / kp->sec_dist;
    if (kp->status.dpd_valid_fg == 0) {
        kp->status.dist = f1;
        kp->status.dist_vec = 0.0f;
        kp->status.dist_speed = 0.0f;
    } else {
        f2 = f1 - kp->status.dist;
        if (f2 < 0.0f) {
            f1 = -f2;
        } else {
            f1 = f2;
        }
        if (kp->dist_play_mode == 0) {
            if (f1 >= kp->dist_play_radius) {
                f1 = 1.0f;
            } else {
                f1 /= kp->dist_play_radius;
                f1 *= f1;
                f1 *= f1;
            }
            f1 *= kp->dist_sensitivity;
            kp->status.dist_vec = f1 * f2;
            if (kp->status.dist_vec < 0.0f) {
                kp->status.dist_speed = -kp->status.dist_vec;
            } else {
                kp->status.dist_speed = kp->status.dist_vec;
            }
            kp->status.dist += kp->status.dist_vec;
        } else if (f1 > kp->dist_play_radius) {
            f1 = kp->dist_sensitivity * ((f1 - kp->dist_play_radius) / f1);
            kp->status.dist_vec = f1 * f2;
            if (kp->status.dist_vec < 0.0f) {
                kp->status.dist_speed = -kp->status.dist_vec;
            } else {
                kp->status.dist_speed = kp->status.dist_vec;
            }
            kp->status.dist += kp->status.dist_vec;
        } else {
            kp->status.dist_vec = 0.0f;
            kp->status.dist_speed = 0.0f;
        }
    }

    pos.x = 0.5f * (kp->kobj_regular[0].center.x + kp->kobj_regular[1].center.x);
    pos.y = 0.5f * (kp->kobj_regular[0].center.y + kp->kobj_regular[1].center.y);
    {
        f32 nx = kp->sec_nrm.x * kp->sec_nrm11.x + kp->sec_nrm.y * kp->sec_nrm11.y;
        f32 ny = -kp->sec_nrm.y * kp->sec_nrm11.x + kp->sec_nrm.x * kp->sec_nrm11.y;

        vec.x = nx * pos.x - ny * pos.y;
        vec.y = ny * pos.x + nx * pos.y;
    }
    vec.x = kp->dpd2pos_scale * (kp->center_org.x - vec.x);
    vec.y = kp->dpd2pos_scale * (kp->center_org.y - vec.y);
    pos.x = -kp->accXY_nrm_hori.y * vec.x + kp->accXY_nrm_hori.x * vec.y;
    pos.y = -kp->accXY_nrm_hori.x * vec.x - kp->accXY_nrm_hori.y * vec.y;
    if (kp->status.dpd_valid_fg == 0) {
        vec2_copy(&kp->status.pos, &pos);
        vec2_copy(&kp->status.vec, &kpad_icenter_org);
        kp->status.speed = 0.0f;
    } else {
        vec.x = pos.x - kp->status.pos.x;
        vec.y = pos.y - kp->status.pos.y;
        f1 = sqrtf(vec.x * vec.x + vec.y * vec.y);
        if (kp->pos_play_mode == 0) {
            if (f1 >= kp->pos_play_radius) {
                f2 = 1.0f;
            } else {
                f2 = f1 / kp->pos_play_radius;
                f2 *= f2;
                f2 *= f2;
            }
            f2 *= kp->pos_sensitivity;
            kp->status.vec.x = f2 * vec.x;
            kp->status.vec.y = f2 * vec.y;
            kp->status.speed = sqrtf(kp->status.vec.x * kp->status.vec.x + kp->status.vec.y * kp->status.vec.y);
            kp->status.pos.x += kp->status.vec.x;
            kp->status.pos.y += kp->status.vec.y;
        } else if (f1 > kp->pos_play_radius) {
            f2 = kp->pos_sensitivity * ((f1 - kp->pos_play_radius) / f1);
            kp->status.vec.x = f2 * vec.x;
            kp->status.vec.y = f2 * vec.y;
            kp->status.speed = sqrtf(kp->status.vec.x * kp->status.vec.x + kp->status.vec.y * kp->status.vec.y);
            kp->status.pos.x += kp->status.vec.x;
            kp->status.pos.y += kp->status.vec.y;
        } else {
            vec2_copy(&kp->status.vec, &kpad_icenter_org);
            kp->status.speed = 0.0f;
        }
    }

    kp->status.dpd_valid_fg = valid;
}

/* Converts one sample's pointer objects, picks the tracked pair, updates the pointer work and publishes the
 * pointer state and its clamped position. */
extern "C" void kpad_read_dpd(WPADStatus* s)
{
    KpadWork* kp;
    KPADObj* op;
    s8 valid;

    kp = &kpad_work;
    if (WPADIsDpdEnabled(0) == 0) {
        op = &kp->kobj_sample[3];
        do {
            op->error_fg = -1;
        } while (--op >= kp->kobj_sample);
    } else {
        kpad_obj_from_dpd(kp, &s->obj[3]);
    }
    kpad_obj_frame_check(kp, kp->kobj_sample);
    kpad_obj_dup_check(kp->kobj_sample);
    kp->valid_objs = 0;
    op = &kp->kobj_sample[3];
    do {
        if (op->error_fg == 0) {
            ++kp->valid_objs;
        }
    } while (--op >= kp->kobj_sample);
    for (;;) {
        if (!(kp->status.acc_vertical.x <= kp_err_up_inpr)) {
            if (kp->status.dpd_valid_fg == 2 || kp->status.dpd_valid_fg == -2) {
                if (kp->valid_objs >= 2 && (valid = kpad_select_2obj_continue(kp)) != 0) {
                    break;
                }
                if (kp->valid_objs >= 1 && (valid = kpad_select_1obj_continue(kp)) != 0) {
                    break;
                }
            } else if (kp->status.dpd_valid_fg == 1 || kp->status.dpd_valid_fg == -1) {
                if (kp->valid_objs >= 2 && (valid = kpad_select_2obj_first(kp)) != 0) {
                    break;
                }
                if (kp->valid_objs >= 1 && (valid = kpad_select_1obj_continue(kp)) != 0) {
                    break;
                }
            } else {
                if (kp->valid_objs >= 2 && (valid = kpad_select_2obj_first(kp)) != 0) {
                    break;
                }
                if (kp->valid_objs == 1 && (valid = kpad_select_1obj_first(kp)) != 0) {
                    break;
                }
            }
        }
        valid = 0;
        break;
    }
    if (valid != 0) {
        kpad_sec_update(kp);
        if (kp->ah_circle_ct == 0
            && kp->dpd_horizon.x * kp->acc_horizon.x + kp->dpd_horizon.y * kp->acc_horizon.y <= kp_err_acc_inpr) {
            valid = 0;
            kp->kobj_regular[0].error_fg = kp->kobj_regular[1].error_fg = 1;
        }
        if (kp->status.dpd_valid_fg == 2 && valid == 2) {
            if (kp->trust_sec_ct == 200) {
                kp->trust_sec_dist = kp->sec_dist;
            } else {
                ++kp->trust_sec_ct;
            }
        } else {
            kp->trust_sec_ct = 0;
        }
    } else {
        kp->trust_sec_ct = 0;
    }
    kpad_calc_dpd_variable(kp, valid);
    pointer_dpd_state = 1;
    if (valid == 0) {
        pointer_dpd_state = 0;
    }
    if (kp->status.pos.x < -1.0f || kp->status.pos.x > 1.0f) {
        pointer_dpd_state = 0;
    }
    if (kp->status.pos.y < -1.0f || kp->status.pos.y > 1.0f) {
        pointer_dpd_state = 0;
    }
    {
        f32 x = kp->status.pos.x;
        f32 y = kp->status.pos.y;

        pointer_norm_x = x < -1.0f ? -1.0f : (x > 1.0f ? 1.0f : x);
        pointer_norm_y = y < -1.0f ? -1.0f : (y > 1.0f ? 1.0f : y);
    }
}

u16 get_rcSwitch_on(s32 chan)
{
    PlayerPad* pad;

    pad = &Psw[chan];
    switch (pad->mode) {
    case 0:
        return pad->field_0xce;
    case 1:
        return pad->field_0x100;
    case 2:
        return pad->field_0xda;
    default:
        return 0;
    }
}

u16 get_rcSwitch_rept(s32 chan)
{
    PlayerPad* pad;

    pad = &Psw[chan];
    switch (pad->mode) {
    case 0:
        return pad->field_0xd2;
    case 1:
        return pad->field_0x11e;
    case 2:
        return pad->field_0xf8;
    default:
        return 0;
    }
}

s32 get_ControlType(s32 chan)
{
    PlayerPad* pad;

    pad = &Psw[chan];
    if (pad->field_0x30 != 0) {
        return 0xFF;
    }
    switch (pad->mode) {
    case 0:
        return 0;
    case 1:
        return 1;
    case 2:
        return 2;
    default:
        return 0xFF;
    }
}

/* Resets the pointer state: the position rings, the counters and the smoothed position (screen centre). */
extern "C" void pointer_reset(void)
{
    s32 i;

    for (i = 0; i < 16; i++) {
        pointer_hist_x[i] = 0;
        pointer_hist_y[i] = 0;
    }
    pointer_stage = 0;
    pointer_last_a[0] = 0;
    pointer_last_b[0] = 0;
    pointer_last_a[1] = 0;
    pointer_last_b[1] = 0;
    pointer_hist_count = 0;
    pointer_center[0] = Screen_w.width_f / 2.0f;
    pointer_center[1] = Screen_w.height_f / 2.0f;
    pointer_offset[0] = 0.0f;
    pointer_offset[1] = 0.0f;
    pointer_dist = 0.0f;
    pointer_dist_limit = 8.0f;
    pointer_freeze = 0;
    pointer_flag_b = 0;
}

/* Moves the smoothed pointer position a quarter of the way towards the newest sample (when it moved far
 * enough). */
extern "C" void pointer_smooth(void)
{
    VEC3 sample;
    VEC3 center;

    VEC3_ctor(&sample);
    VEC3_ctor(&center);
    if (pointer_freeze != 0) {
        return;
    }
    if (pointer_valid == 0) {
        pointer_center[0] = pointer_hist_x[0];
        pointer_center[1] = pointer_hist_y[0];
        return;
    }
    {
        VEC3 tmp;

        copyVec3(&sample, setVec3(&tmp, pointer_hist_x[0], pointer_hist_y[0], 0.0f));
    }
    {
        VEC3 tmp;

        copyVec3(&center, setVec3(&tmp, pointer_center[0], pointer_center[1], 0.0f));
    }
    if ((pointer_dist = vec3_distance(&sample, &center)) < pointer_dist_limit) {
        pointer_offset[0] = 0.0f;
        pointer_offset[1] = 0.0f;
    } else {
        f32 dx;
        f32 dy;

        dx = pointer_hist_x[0] - pointer_center[0];
        dy = pointer_hist_y[0] - pointer_center[1];
        pointer_offset[0] = dx * 0.25f;
        pointer_offset[1] = dy * 0.25f;
    }
    pointer_center[0] += pointer_offset[0];
    pointer_center[1] += pointer_offset[1];
}

/* Shifts the position rings and pushes the predicted pointer position, clamped to the screen. */
extern "C" void pointer_hist_push(void)
{
    s32 i;
    f32 x;
    f32 y;

    for (i = 15; i > 0; i--) {
        pointer_hist_x[i] = pointer_hist_x[i - 1];
        pointer_hist_y[i] = pointer_hist_y[i - 1];
    }
    x = pointer_half_x + pointer_norm_x * pointer_half_x;
    y = pointer_half_y + pointer_norm_y * pointer_half_y;
    x = x < 0.0f ? 0.0f : (x > Screen_w.width_f ? Screen_w.width_f : x);
    y = y < 0.0f ? 0.0f : (y > Screen_w.height_f ? Screen_w.height_f : y);
    pointer_hist_x[0] = x;
    pointer_hist_y[0] = y;
}

/* Runs the pointer for this frame: the first frame resets it, later frames poll KPAD and smooth it. */
extern "C" void pointer_update(void)
{
    s32 i;

    pointer_active = 1;
    pointer_timeout = 300;
    switch (pointer_stage) {
    case 0:
        pointer_calib_reset();
        pointer_stage = 1;
        pointer_hist_count = 16;
        pointer_norm_x = 0.0f;
        pointer_norm_y = 0.0f;
        for (i = 0; i < 16; i++) {
            pointer_hist_x[i] = 0;
            pointer_hist_y[i] = 0;
        }
        break;
    case 1:
        kpad_dpd_enable(0);
        if (pointer_dpd_state != 0 && pointer_freeze == 0) {
            pointer_hist_push();
            pointer_smooth();
        }
        break;
    }
}

/* Resets the pointer calibration: the raw and previous positions to the screen centre, no velocity. */
extern "C" void pointer_calib_reset(void)
{
    pointer_dpd_state = 0;
    pointer_valid = 0;
    pointer_half_x = Screen_w.width_f / 2.0f;
    pointer_half_y = Screen_w.height_f / 2.0f;
    pointer_prev_x = pointer_half_x;
    pointer_prev_y = pointer_half_y;
    pointer_norm_x = 0.0f;
    pointer_norm_y = 0.0f;
}

void set_menu_key_ad_flag(u8 chan, bool flag)
{
    PlayerPad* pad;

    pad = &Psw[chan];
    if (flag == true) {
        pad->button_0x2C0.menu_key_ad_0x75 = 1;
    } else {
        pad->button_0x2C0.menu_key_ad_0x75 = 0;
    }
}

/* Copies `table` (16 hold-off frames) into channel `chan`'s active hold-off row. */
extern "C" void pad_wait_table_set(u8 chan, u8* table)
{
    PadButtons* btn;
    s32 i;

    btn = &Psw[chan].button_0x2C0;
    for (i = 0; i < 16; i++) {
        btn->wait_table_0x24[btn->wait_set_0x74][i] = *table++;
    }
}

/* Merges channel `chan`'s device blocks into its menu button block through the device's button map, with the
 * per-button hold-off and the direction-to-button mapping. */
extern "C" void pad_menu_merge(u8 chan)
{
    PlayerPad* pad;
    PadButtons* btn;
    const PadMapEntry* map;
    const PadMapEntry* m;
    const PadOutEntry* o;
    s32 i;
    u16 kind;
    u16 mask;
    u16 bit;
    u16 out;
    u16 dirbtn;
    u8 dir;

    pad = &Psw[chan];
    btn = &pad->button_0x2C0;
    btn->prev_0x02 = btn->hold_0x00;
    btn->stick_prev_0x0A = btn->press_0x08;
    btn->edge_off_0x1E = btn->edge_0x1D;
    btn->hold_0x00 = 0;
    btn->press_0x08 = 0;
    btn->trigger_0x0C = 0;
    btn->stick_released_0x0E = 0;
    btn->clear_0x10 = 0;
    btn->clear_0x12 = 0;
    btn->stick_deg_0x18 = 0;
    btn->stick_dist_0x1A = 0;
    btn->stick_dir_0x1C = 0;
    btn->edge_0x1D = 0;
    btn->held_0x14 = 0;
    btn->edge_on_0x1F = 0;
    if (pad->field_0x30 == 0) {
        switch (pad->mode) {
        case 1:
            btn->device_0x22 = 1;
            map = pad_map_fs;
            break;
        case 2:
            btn->device_0x22 = 2;
            if (option_cfg_eq_ck(chan, 7, 1) == 1) {
                map = pad_map_cl_a;
            } else {
                map = pad_map_cl_b;
            }
            break;
        default:
            btn->device_0x22 = 3;
            map = pad_map_core;
            break;
        }
    } else {
        btn->device_0x22 = 0;
    }
    if (btn->device_0x22 == 0) {
        btn->repeat_count_0x16 = 0;
        btn->edge_count_0x20 = 0;
        return;
    }
    o = pad_map_out;
    m = map;
    for (i = 0; i < 17; i++) {
        kind = m->kind;
        mask = m->mask;
        m++;
        bit = o->bit;
        out = o->mask;
        o++;
        if (mask == 0) {
            continue;
        }
        if (btn->wait_0x76[bit] != 0) {
            btn->hold_0x00 |= (u16)(btn->prev_0x02 & (1 << bit));
            continue;
        }
        switch (btn->device_0x22) {
        case 1:
            if (kind == 0) {
                if (pad->control_0x0FC & mask) {
                    btn->hold_0x00 |= out;
                }
            } else {
                if (pad->field_0xca & mask) {
                    btn->hold_0x00 |= out;
                }
            }
            break;
        case 2:
            if (kind == 0) {
                if (pad->field_0xca & mask) {
                    btn->hold_0x00 |= out;
                }
            } else {
                if (pad->control_0x0D6 & mask) {
                    btn->hold_0x00 |= out;
                }
            }
            break;
        case 3:
            if (pad->field_0xca & mask) {
                btn->hold_0x00 |= out;
            }
            break;
        }
    }
    if (btn->device_0x22 == 1) {
        btn->press_0x08 = pad->control_0x104;
        btn->stick_deg_0x18 = pad->fs_deg_0x114[0];
        btn->stick_dist_0x1A = pad->fs_dist_0x118[0];
        btn->stick_dir_0x1C = pad->fs_dir_0x11C[0];
    }
    if (btn->device_0x22 == 2) {
        btn->press_0x08 = pad->control_0x0DE;
        btn->stick_deg_0x18 = pad->cl_deg_0xEE[0];
        btn->stick_dist_0x1A = pad->cl_dist_0xF2[0];
        btn->stick_dir_0x1C = pad->cl_dir_0xF6[0];
    }
    btn->press_0x08 &= 0x3C00;
    btn->trigger_0x0C = btn->press_0x08 & ~btn->stick_prev_0x0A;
    btn->stick_released_0x0E = btn->stick_prev_0x0A & ~btn->press_0x08;
    if (btn->stick_dir_0x1C & 1) {
        btn->edge_0x1D |= 1;
    }
    if (btn->stick_dir_0x1C & 2) {
        btn->edge_0x1D |= 2;
    }
    if (btn->stick_dir_0x1C & 4) {
        btn->edge_0x1D |= 4;
    }
    if (btn->stick_dir_0x1C & 8) {
        btn->edge_0x1D |= 8;
    }
    if (btn->edge_off_0x1E == btn->edge_0x1D) {
        btn->edge_count_0x20++;
        if (btn->edge_count_0x20 > 12) {
            btn->edge_count_0x20 = 10;
            btn->edge_on_0x1F = btn->edge_0x1D;
        }
    } else {
        btn->edge_count_0x20 = 0;
    }
    if (btn->menu_key_ad_0x75 == 1 && (dirbtn = btn->hold_0x00 & 0xF) == 0) {
        dir = btn->edge_0x1D & 0xF;
        if (dir & 1) {
            dirbtn |= 0x2000;
        }
        if (dir & 2) {
            dirbtn |= 0x1000;
        }
        if (dir & 4) {
            dirbtn |= 0x800;
        }
        if (dir & 8) {
            dirbtn |= 0x400;
        }
        m = map;
        o = pad_map_out;
        for (i = 0; i < 17; i++) {
            kind = m->kind;
            mask = m->mask;
            m++;
            bit = o->bit;
            out = o->mask;
            o++;
            if (mask == 0) {
                continue;
            }
            if (btn->wait_0x76[bit] != 0) {
                continue;
            }
            switch (btn->device_0x22) {
            case 1:
                if (kind == 0) {
                    if (dirbtn & mask) {
                        btn->hold_0x00 |= out;
                    }
                } else {
                    if (dirbtn & mask) {
                        btn->hold_0x00 |= out;
                    }
                }
                break;
            case 2:
                if (kind == 0) {
                    if (dirbtn & mask) {
                        btn->hold_0x00 |= out;
                    }
                } else {
                    if (dirbtn & mask) {
                        btn->hold_0x00 |= out;
                    }
                }
                break;
            case 3:
                if (dirbtn & mask) {
                    btn->hold_0x00 |= out;
                }
                break;
            }
        }
    }
    if (btn->prev_0x02 == btn->hold_0x00) {
        btn->repeat_count_0x16++;
        if (btn->repeat_count_0x16 > 12) {
            btn->repeat_count_0x16 = 10;
            btn->held_0x14 = btn->hold_0x00;
        }
    } else {
        btn->repeat_count_0x16 = 0;
    }
    for (i = 0; i < 16; i++) {
        if ((btn->prev_0x02 & (1 << i)) != (btn->hold_0x00 & (1 << i))) {
            btn->wait_0x76[i] = btn->wait_table_0x24[btn->wait_set_0x74][i];
        } else if (btn->wait_0x76[i] != 0) {
            btn->wait_0x76[i]--;
        }
    }
    btn->pressed_0x04 = btn->hold_0x00 & ~btn->prev_0x02;
    btn->released_0x06 = btn->prev_0x02 & ~btn->hold_0x00;
    if (GameMode_ck() == 1) {
        menu_pad_map_set(chan, 0);
    } else {
        menu_pad_map_set(chan, 1);
    }
}

/* ------------------------------------------------------------------ *
 * 0x80044C8C - 0x80044CF4  (KPAD / WPAD setup)
 * ------------------------------------------------------------------ */

void initKPAD(void)
{
    if (pad_kpad_ready == 0) {
        KPADInit();
        pad_kpad_ready = 1;
    }
}

void shutdownKPAD(void)
{
    if (pad_kpad_ready != 0) {
        KPADShutdown();
        pad_kpad_ready = 0;
    }
}

void setWpadCallback(void)
{
    PlayerPad* pad;
    s32 chan;

    chan = 0;
    pad = Psw;
    do {
        if ((s32)pad->field_0x30 != -1) {
            WPADSetExtensionCallback(chan, pad_extension_cb);
            WPADSetSamplingCallback(chan, pad_sampling_cb);
        }
        pad++;
        chan++;
    } while (chan < 4);
}


/* `GameModeExec__Fv` - pick the task body for the current game mode. */
void GameModeExec(void)
{
    switch (GameMode_ck()) {
    case 1:
        system_w.field_0x868 = 0;
        Tsk_Change((void*)&quest_game_task, 4);
        return;
    case 2:
        if (system_w.field_0x7d3 == 1) {
            Tsk_Change((void*)&game_mode_flow_task, 4);
            return;
        }
        Tsk_Change((void*)&lobby_local_task, 4);
        return;
    }
}

extern "C" void pad_motor_stop(u8 chan);

/* Starts channel `chan`'s rumble for `time` frames when rumble is enabled for the remote and in the options. */
extern "C" void pad_motor_start(u8 chan, s8 time)
{
    if (system_w.field_0x32 != 0 && option_cfg_eq_ck(chan, 7, 0) != 0 && (u8)(get_cfg(chan, 20) ^ 1)) {
        system_w.field_0x33[chan] = time;
        system_w.field_0x37[chan] = 1;
        WPADControlMotor(chan, 1);
    }
}

/* Motor-off for one channel, plus the two per-channel motor flags. */
extern "C" void pad_motor_stop(u8 chan)
{
    system_w.field_0x33[chan] = 0;
    system_w.field_0x37[chan] = 0;
    WPADControlMotor(chan, 0);
}

extern "C" void pad_motor_stop_all(void)
{
    s32 chan;

    for (chan = 0; chan < 4; chan++) {
        pad_motor_stop(chan);
    }
}

/* Tick the per-channel motor timers. */
extern "C" void pad_motor_tick(void)
{
    s32 chan;

    chan = 0;
    do {
        if ((s8)system_w.field_0x33[chan] > 0) {
            system_w.field_0x33[chan]--;
        }
        if (system_w.field_0x37[chan] == 1 && (s8)system_w.field_0x33[chan] <= 0) {
            pad_motor_stop((u8)chan);
        }
        chan++;
    } while (chan < 4);
}

/* Per-channel motor / LED state (pad_motor_info_cb in the map). */
extern "C" void pad_motor_info_cb(s32 chan, s32 on)
{
    if (on != 0) {
        system_w.field_0x7e4[chan] = 1;
        system_w.field_0x7e0[chan] = 0x78;
        if (system_w.field_0x7dc[chan] == 0xFF) {
            system_w.field_0x7dc[chan] = 0;
        }
    } else {
        system_w.field_0x7dc[chan] = pad_chan_state[chan].motor_level_0x14;
    }
}

/* ------------------------------------------------------------------ *
 * 0x80044180  (acceleration clamp)
 * ------------------------------------------------------------------ */

/* Starts channel `chan`'s rumble at strength `level` (0..4, out of range is 0). */
extern "C" void pad_motor_start_level(u8 chan, u8 level)
{
    if (level > 4) {
        level = 0;
    }
    pad_motor_start(chan, pad_motor_level_table[level]);
}

/* Turns the pointer camera of channel `chan` on (with the extension's data format) and records whether it is. */
extern "C" void kpad_dpd_enable(u8 chan)
{
    s32 type;

    if (WPADIsDpdEnabled(chan) == 1) {
        kpad_dpd_on = 1;
        return;
    }
    type = get_ControlType(chan);
    if ((u8)type == 1 || (u8)type == 2) {
        if (WPADControlDpd(chan, 1, 0) == 0) {
            if ((u8)type == 1) {
                WPADSetDataFormat(chan, 5);
            }
            if ((u8)type == 2) {
                WPADSetDataFormat(chan, 8);
            }
            kpad_dpd_on = 1;
        } else {
            kpad_dpd_on = 0;
        }
    }
}

/* Turns the pointer camera of channel `chan` off once it has been unused for the timeout. */
extern "C" void kpad_dpd_idle(u8 chan)
{
    if (pointer_active != 0) {
        return;
    }
    if (pointer_timeout > 0) {
        pointer_timeout--;
        return;
    }
    if (WPADIsDpdEnabled(chan) == 0) {
        kpad_dpd_on = 0;
        return;
    }
    kpad_dpd_on = WPADControlDpd(chan, 0, 0) != 0;
}

extern "C" f32 clamp_acc(f32 value, f32 limit)
{
    f32 neg;

    if (value < 0.0f) {
        neg = -limit;
        if (value < neg) {
            return neg;
        }
        return value;
    }
    if (value > limit) {
        return limit;
    }
    return value;
}

/* Constructs both pad record arrays (`Psw`, `Psw_prev`) element by element. */
extern "C" void pad_records_construct(void)
{
    __construct_array(Psw, (void*)player_pad_construct, NULL, sizeof(PlayerPad), 4);
    __construct_array(Psw_prev, (void*)player_pad_construct, NULL, sizeof(PlayerPad), 4);
}

/* Constructs one pad record: its Nunchuk block. */
extern "C" PlayerPad* player_pad_construct(PlayerPad* pad)
{
    pad_fs_block_construct((PadFsBlock*)&pad->control_0x0FC);
    return pad;
}

/* Constructs a Nunchuk block: the 32 accelerometer samples of its two rings. */
extern "C" PadFsBlock* pad_fs_block_construct(PadFsBlock* blk)
{
    VEC3* v;

    v = (VEC3*)blk->fs_acc;
    do {
        VEC3_ctor(v);
        v++;
    } while (v < (VEC3*)&blk->index);
    return blk;
}

/* ------------------------------------------------------------------ *
 * This unit's own `.bss` (`splits.txt` `.bss 0x806585B8..0x806694E8`), in address order.  Defined at the
 * foot of the file, after every use: with the definitions above the bodies MWCC folds the addresses
 * into one section base plus displacements, where the target emits a `lis`/`addi` pair per symbol.
 * ------------------------------------------------------------------ */
u32 rso_slot_load_base[5];             /* +0x806585B8: per-slot RSO load base, cleared by `rso_overlay_release_all` */
u32 rso_slot_unused_words[5];          /* +0x806585CC: cleared with `rso_slot_load_base`, read nowhere else */
SystemWork system_w;                   /* +0x806585E0: the game/system state block (`mh3_pad/system_w.h`) */
ScreenWork Screen_w;                   /* +0x8065903C: the screen geometry block (`mh3_pad/Screen_w.h`) */
u8 option_w[0x24];                     /* +0x80659090: the option table (`mh3_pad/option_w.h`) */
LbParamWork lb_param_w;                /* +0x806590B4: the lobby parameter block (`mh3_pad/lb_param_w.h`) */
TaskSlot task_slot_table[16];          /* +0x80659150: the task slot table `Tsk_Change` indexes */
PlayerPad Psw[4];                      /* +0x80659350: the per-player pad records (`mh3_pad/Psw.h`) */
PlayerPad Psw_prev[4];                 /* +0x8065A090: the second pad-record array `fn_80046B94` constructs (GUESS: previous frame) */
PadChanState pad_chan_state[4];        /* +0x8065ADD0: per-channel state records, cleared with `Psw` by `pad_wpad_init` */
PadBtnTable pad_btn_table_0;              /* +0x8065AE30: copy of the .data button table 0x80580E90 (GUESS name) */
PadBtnTable pad_btn_table_1;              /* +0x8065AE50: copy of the .data button table 0x80580EB0 (GUESS name) */
PadBtnTable pad_btn_table_2;              /* +0x8065AE70: copy of the .data button table 0x80580ED0 (GUESS name) */
KpadWork kpad_work;                    /* +0x8065AE90: KPAD work area `kpad_work_init`/`kpad_pointer_read` use (GUESS name) */
WPADCLStatus wpad_sampling_buf_fmt8[4][100];/* +0x8065B048: per-channel 100-sample buffer for data format 8 */
WPADFSStatus wpad_sampling_buf_fmt5[4][100];/* +0x806604A8: per-channel 100-sample buffer for data format 5 */
WPADStatus wpad_sampling_buf_fmt2[4][100];  /* +0x806652C8: per-channel 100-sample buffer for data format 2 */
MEMAllocator wpad_mem2_allocator;      /* +0x80669468: allocator over the MEM2 pad heap (align 0x20) */
s16 pointer_hist_x[16];                /* +0x80669478: pointer x history ring, zeroed by `pointer_reset` (GUESS) */
s16 pointer_hist_y[16];                /* +0x80669498: pointer y history ring (GUESS) */
f32 pointer_center[3];                 /* +0x806694B8: half of `Screen_w.width_f`/`height_f` (GUESS name) */
f32 pointer_offset[3];                 /* +0x806694C4: zeroed pointer offset (GUESS name) */
