/*
 * KPAD/kpad.c - the SDK KPAD library (Wii Remote sample processing, calibration, read).
 *
 * RANGE. `.text` 0x804BD780-0x804C1760; `.data` 0x8061ADA0-0x8061AE00; `.bss` 0x80747540-0x80748B90; `.sdata`
 *   0x80793EB0-0x80793F00; `.sbss` 0x80795260-0x80795298; `.sdata2` 0x8079D200-0x8079D270 (26 functions / 0x3F30
 *   B). One TU: one version string `<< RVL_SDK - KPAD ... >>` (`.data` 0x8061ADA0, registered through `.sdata`
 *   0x80793EB0), one contiguous run per section; the run ends at `MEM/mem_heap.c` (0x804C1760). The tunables the
 *   bodies read live in `mh3_pad.cpp`'s `.sdata` (0x80790EA0..0x80790ED3) and `.sbss` 0x80794800, declared in
 *   `mh3_pad.h`. The last 4 bytes of the `.sdata` and `.sbss` claims are zero alignment fill (no symbol).
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`) with `cflags_base` and `-fp_contract off` on this object:
 *   the target has 0 fused ops across 406 fadd/fsub/fmul; a calc_acc probe gives 42 insns with 3 fmadds under
 *   `-fp_contract on` and 45 insns with 0 fused under `off`, the target's opcode sequence. `#pragma auto_inline off`:
 *   under `cflags_base`'s `-inline auto` MWCC defers code generation and emits callees ahead of their callers
 *   (KPADiRead before KPADRead, KPADInitEx before KPADInit, the callbacks ahead of KPADShutdown); with auto-inlining
 *   off (or `-inline on` on the command line) the functions come out in the target's source order, every row scores
 *   the same, and the explicitly `inline` helpers are still inlined.
 * NAMES. functions: the map's SDK-style names (GUESS by shape: reset_kpad ... KPADiSamplingCallback; KPADRead is
 *   the map's). KPADSetBtnRepeat is the SDK API name; its body (the 200.0f samples-per-second scale) is a GUESS.
 *   Inline helpers clamp_acc, reset_wbc, clear_wpad_ring, calc_dpd2pos_scale, set_obj_interval, request_reset_all,
 *   calc_ex_analog, clear_wbc_weights, reset_cl_status, finish_control_dpd and get_ring_sample are GUESS
 *   names for code the target repeats or inlines. Types: `KPADStatus`/`KPADEXStatus`/`Vec2` follow the SDK's
 *   public header; `KPADInsideStatus`, `KPADObj`, `KPADUnifiedWpadStatus`, `KPADTrainStatus` (brake/mascon) and
 *   `KPADClampStick` are GUESS from use (offsets/sizes from the code: 0x578 stride, 0xB0 copy loop, 0x38 ring
 *   stride). Device ids KP_DEV_* (0 core, 1 Nunchuk, 2 classic, 3 balance board, 16 train, 17 guitar, 251/252/255
 *   future/unsupported/unknown), data formats KP_FMT_* (3..12), WPAD sample errors KP_WPAD_ERR_* (-2/-4/-7) and the
 *   KPADRead codes KPAD_READ_ERR_* (0..-5) are GUESS names for the values the code tests. Data statics:
 *   `__KPADVersion` is the SDK's; the rest are the map's GUESS SDK-style names (idist_org, iaccXY_nrm_hori,
 *   isec_nrm11, icenter_org, kp_mode_table, kp_fs_fstick_*, kp_cl_stick_*, kp_cl_trigger_*, kp_rm_acc_max,
 *   kp_fs_acc_max, kp_ex_trigger_*, kp_ex_analog_*, kp_stick_clamp_mode, kp_dpd_center_init, kp_initialized,
 *   kp_init_angle, kp_dist_init, kp_acc_mtx, inside_kpads, kp_wbc_*).
 * RESIDUALS.
 *   - KPADSetBtnRepeat has no target bytes: mwld strips it (no caller). It is what puts 0.0f/0.5f at the head of
 *     `.sdata2` and `inside_kpads` at `.bss`+0 (MWCC orders both by first use). Our object carries it (+0x88
 *     `.text`) and its 200.0f literal (`.sdata2`+0x8, 9 differing bytes); a flip needs a trial link proving both
 *     are stripped.
 *   - KPADiRead: `&samplingBufs[count]` is CSE'd into r27 across the ring copy loop (target recomputes it after
 *     OSRestoreInterrupts), the u16 `count` argument of read_kpad_button gets a `clrlwi` the target lacks, and
 *     kp/chan/length take r24/r26/r25 (target r26/r25/r24).
 *   - calc_dpd_variable: the pointer-position block schedules `-sec_nrm.y * sec_nrm11.x` after
 *     `sec_nrm.x * sec_nrm11.x` and loads `dpd_valid_fg` late (target: early); temps at block or function scope
 *     and declaration order do not move it.
 *   - KPADInitEx: `lis KPADiSamplingCallback@ha` is scheduled after `lis KPADiConnectCallback@ha` (target: before).
 *   - select_2obj_first: the hoisted constants 1.0f/0.0f and `max` take f29..f31 in a different order.
 *   - `mh3_pad.cpp` still declares KPADInit/KPADShutdown locally instead of including `kpad.h`.
 * SHAPES. `.sbss` statics are defined in reverse of their target order (MWCC emits `.sbss` reversed); loops over
 *   `kobj_*` are do-while with the end compare at the bottom; read_kpad_button stages the classic word in a local
 *   before reading the old hold; select_2obj_continue reuses `dist` for the reciprocal length. A chained
 *   `a = b = x` stores right to left (`kp_dist_init = kp_err_dist_min = ...` writes kp_err_dist_min first), so the
 *   relocation order depends on the chain's order; the WBC flags are separate statements for that reason.
 *   read_kpad_dpd's object selection is a `for (;;)` with one shared `valid = 0` fall-through; its pair loops share
 *   one `last` end pointer. KPADiSamplingCallback leaves its device-mode block for an unknown device through a
 *   `continue` of a `do { } while (0)`, and declares `kp` ahead of `uwp` (the other order swaps r29/r30).
 *   KPADiRead's probe-failure path inlines finish_control_dpd, which KPADiControlDpdCallback also calls (a later
 *   definition is not inlined).
 */
#include "KPAD/kpad.h"
#include "MSL/s_cos.h"
#include "MSL/s_sin.h"
#include "MSL/sqrt.h"
#include "MTX/mtxvec.h"
#include "OS/OS.h"
#include "OS/OSInterrupt.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "mh3_pad.h"

#pragma auto_inline off

/* WPAD device types and data formats the library tests. */
#define KP_DEV_CORE 0
#define KP_DEV_FREESTYLE 1
#define KP_DEV_CLASSIC 2
#define KP_DEV_BALANCE 3
#define KP_DEV_TRAIN 16
#define KP_DEV_GUITAR 17
#define KP_DEV_FUTURE 251
#define KP_DEV_NOT_SUPPORTED 252
#define KP_DEV_NOT_FOUND 253
#define KP_DEV_UNKNOWN 255

#define KP_FMT_CORE_ACC 1
#define KP_FMT_CORE_ACC_DPD 2
#define KP_FMT_FS 3
#define KP_FMT_FS_ACC 4
#define KP_FMT_FS_ACC_DPD 5
#define KP_FMT_CLASSIC 6
#define KP_FMT_CLASSIC_ACC 7
#define KP_FMT_CLASSIC_ACC_DPD 8
#define KP_FMT_TRAIN 10
#define KP_FMT_GUITAR 11
#define KP_FMT_BALANCE 12

/* WPAD sample errors the library tests. */
#define KP_WPAD_ERR_BUSY -2
#define KP_WPAD_ERR_INVALID -4
#define KP_WPAD_ERR_CORRUPTED -7

/* KPADRead error codes. */
#define KPAD_READ_ERR_NONE 0
#define KPAD_READ_ERR_NO_DATA -1
#define KPAD_READ_ERR_NO_CONTROLLER -2
#define KPAD_READ_ERR_SETUP -3
#define KPAD_READ_ERR_LOCKED -4
#define KPAD_READ_ERR_INIT -5

#define KPAD_BUTTON_RPT 0x80000000
#define KPAD_RING_BUFS 16

const char* __KPADVersion = "<< RVL_SDK - KPAD \trelease build: Jun 22 2009 18:32:13 (0x4302_145) >>";

static KPADInsideStatus inside_kpads[4];
static Mtx kp_acc_mtx;
static f64 kp_wbc_weight[4];
static f64 kp_wbc_zero[4];

static f32 idist_org = 1.0f;
static Vec2 iaccXY_nrm_hori = { 0.0f, -1.0f };
static Vec2 isec_nrm11 = { 1.0f, 0.0f };
static s32 kp_fs_fstick_min = 15;
static s32 kp_fs_fstick_max = 71;
static s32 kp_cl_stick_min = 60;
static s32 kp_cl_stick_max = 308;
static s32 kp_cl_trigger_min = 30;
static s32 kp_cl_trigger_max = 180;
static f32 kp_rm_acc_max = 3.4f;
static f32 kp_fs_acc_max = 2.1f;
static s32 kp_ex_trigger_max = 0x100;
static s32 kp_ex_analog_max = 0x400;
static u8 kp_wbc_setup_retry = 50;
static f32 kp_wbc_weight_max = 400.0f;
static f32 kp_init_angle = 24.0f;

static f32 kp_dist_init;
static u8 kp_wbc_busy;
static u8 kp_wbc_setup_done;
static u8 kp_wbc_setup_busy;
static u8 kp_wbc_zero_pending;
static u8 kp_wbc_state;
static u16 kp_wbc_wait_count;
static f64 kp_wbc_tgc_weight;
static Vec2 icenter_org;
static u8 kp_initialized;
static s32 kp_ex_analog_min;
static s32 kp_ex_trigger_min;
static s32 kp_stick_clamp_mode;
static Vec2 kp_dpd_center_init;

/* The DPD command and data format for each device mode (pointer off / on). */
static u8 kp_mode_table[12][2] = {
    { 0, 1 }, { 3, 2 }, { 0, 4 }, { 1, 5 }, { 0, 7 }, { 1, 8 },
    { 0, 11 }, { 1, 11 }, { 0, 12 }, { 0, 12 }, { 0, 10 }, { 0, 10 },
};

s32 KPADiRead(s32 chan, KPADStatus* samplingBufs, u32 length, s32* err, BOOL readEx);
void KPADiSamplingCallback(s32 chan);
static void KPADiConnectCallback(s32 chan, s32 reason);

/* Sets the button auto-repeat delay and interval of channel `chan` in seconds; a zero interval turns repeat off. */
void KPADSetBtnRepeat(s32 chan, f32 delay_sec, f32 pulse_sec)
{
    KPADInsideStatus* kp = &inside_kpads[chan];

    if (pulse_sec != 0.0f) {
        kp->btn_repeat_delay = (u16)(s32)(delay_sec * 200.0f + 0.5f);
        kp->btn_repeat_pulse = (u16)(s32)(pulse_sec * 200.0f + 0.5f);
    } else {
        kp->btn_repeat_delay = 40000;
        kp->btn_repeat_pulse = 0;
    }
    kp->btn_repeat_time = 0;
    kp->btn_repeat_next = kp->btn_repeat_delay;
    kp->btn_cl_repeat_time = 0;
    kp->btn_cl_repeat_next = kp->btn_repeat_delay;
}

/* 0x804BD780 (0x1D0): Clears the processed state of one channel back to its resting values. */
static void reset_kpad(KPADInsideStatus* kp)
{
    KPADStatus* sp = &kp->status;
    KPADObj* op;

    kp->reset_req = 0;
    kp->frame_min.x = -1.0f + kp_err_outside_frame;
    kp->frame_max.x = 1.0f - kp_err_outside_frame;
    kp->frame_min.y = -0.75f + kp_err_outside_frame;
    kp->frame_max.y = 0.75f - kp_err_outside_frame;
    kp->dist_speed_inc = 1.0f / kp_err_dist_speed;
    kp->dist_speed_dec = -1.0f / kp_err_dist_speed;
    kp->ah_circle_radius2 = kp_ah_circle_radius * kp_ah_circle_radius;
    kp->dist_min = kp_err_dist_min;
    kp->dist_scale = kp_dist_init;

    sp->hold = sp->trig = sp->release = 0;
    kp->btn_repeat_time = 0;
    kp->btn_repeat_next = kp->btn_repeat_delay;

    sp->dpd_valid_fg = 0;
    kp->trust_sec_ct = 0;
    sp->pos = sp->vec = icenter_org;
    sp->speed = 0.0f;
    sp->horizon.x = kp->acc_horizon.x = kp->dpd_horizon.x = 1.0f;
    sp->horizon.y = kp->acc_horizon.y = kp->dpd_horizon.y = 0.0f;
    sp->hori_vec = icenter_org;
    sp->hori_speed = 0.0f;
    sp->acc_vertical.x = 1.0f;
    sp->acc_vertical.y = 0.0f;
    sp->dist = kp->dist_org;
    sp->dist_vec = sp->dist_speed = 0.0f;
    kp->dist_last = kp->dist_org;
    kp->sec_dist = kp->trust_sec_dist = kp->dist_scale / kp->dist_org;
    kp->sec_nrm = kp->sec_nrm11;

    sp->acc.x = sp->acc.z = 0.0f;
    sp->acc.y = -1.0f;
    sp->acc_value = 1.0f;
    sp->acc_speed = 0.0f;
    kp->hard_acc = sp->acc;

    kp->ah_circle_pos = kp->acc_horizon;
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

    kp->wpad_ring_lvl = 0;
    kp->dev_changed = 1;
}

/* 0x804BD950 (0x1A0): Sets the repeat bit of the core and classic button words while a button is held. */
static void calc_button_repeat(KPADInsideStatus* kp, u32 dev_type, u16 count)
{
    if (kp->status.trig != 0 || kp->status.release != 0) {
        kp->btn_repeat_time = 0;
        kp->btn_repeat_next = kp->btn_repeat_delay;
        if (kp->status.trig != 0 && kp->btn_repeat_pulse != 0) {
            kp->status.hold |= KPAD_BUTTON_RPT;
        }
    } else if (kp->status.hold != 0) {
        kp->btn_repeat_time += count;
        if (kp->btn_repeat_time >= 40000) {
            kp->btn_repeat_time -= 40000;
        }
        if (kp->btn_repeat_time >= kp->btn_repeat_next) {
            kp->status.hold |= KPAD_BUTTON_RPT;
            kp->btn_repeat_next += kp->btn_repeat_pulse;
            if (kp->btn_repeat_time >= 20000) {
                kp->btn_repeat_time -= 20000;
                kp->btn_repeat_next -= 20000;
            }
        }
    }

    if (dev_type == KP_DEV_CLASSIC || dev_type == KP_DEV_TRAIN || dev_type == KP_DEV_GUITAR) {
        if (kp->status.ex_status.cl.trig != 0 || kp->status.ex_status.cl.release != 0) {
            kp->btn_cl_repeat_time = 0;
            kp->btn_cl_repeat_next = kp->btn_repeat_delay;
            if (kp->status.ex_status.cl.trig != 0 && kp->btn_repeat_pulse != 0) {
                kp->status.ex_status.cl.hold |= KPAD_BUTTON_RPT;
            }
        } else if (kp->status.ex_status.cl.hold != 0) {
            kp->btn_cl_repeat_time += count;
            if (kp->btn_cl_repeat_time >= 40000) {
                kp->btn_cl_repeat_time -= 40000;
            }
            if (kp->btn_cl_repeat_time >= kp->btn_cl_repeat_next) {
                kp->status.ex_status.cl.hold |= KPAD_BUTTON_RPT;
                kp->btn_cl_repeat_next += kp->btn_repeat_pulse;
                if (kp->btn_cl_repeat_time >= 20000) {
                    kp->btn_cl_repeat_time -= 20000;
                    kp->btn_cl_repeat_next -= 20000;
                }
            }
        }
    }
}

/* 0x804BDAF0 (0xA0): Latches the button words of one sample and derives the trigger and release bits. */
static void read_kpad_button(KPADInsideStatus* kp, u32 dev_type, u16 count, u32 core_btn, u32 fs_btn, u32 cl_btn)
{
    u32 old, change;

    old = kp->status.hold & 0xFFFF;
    kp->status.hold = core_btn & 0x9FFF;
    if (dev_type == KP_DEV_FREESTYLE) {
        kp->status.hold |= (kp->dev_changed ? 0 : fs_btn) & 0x6000;
    }
    change = kp->status.hold ^ old;
    kp->status.trig = change & kp->status.hold;
    kp->status.release = change & old;

    if (dev_type == KP_DEV_CLASSIC || dev_type == KP_DEV_TRAIN || dev_type == KP_DEV_GUITAR) {
        u32 btn = kp->dev_changed ? 0 : cl_btn;

        old = kp->status.ex_status.cl.hold & 0xFFFF;
        kp->status.ex_status.cl.hold = btn & 0xFFFF;
        change = kp->status.ex_status.cl.hold ^ old;
        kp->status.ex_status.cl.trig = change & kp->status.ex_status.cl.hold;
        kp->status.ex_status.cl.release = change & old;
    }

    calc_button_repeat(kp, dev_type, count);
}

/* 0x804BDB90 (0xB4): Moves one smoothed acceleration axis towards a new reading by the play parameters. */
static void calc_acc(KPADInsideStatus* kp, f32* acc, f32 acc2)
{
    f32 f1, f2;

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

/* 0x804BDC50 (0x194): Tracks the remote's roll from the X/Y acceleration and counts how long it stays still. */
static void calc_acc_horizon(KPADInsideStatus* kp)
{
    f32 f1, vx, vy, ax, ay;

    f1 = (f32)sqrt(kp->hard_acc.x * kp->hard_acc.x + kp->hard_acc.y * kp->hard_acc.y);
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
    f1 = (f32)sqrt(vx * vx + vy * vy);
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

/* 0x804BDDF0 (0x124): Tracks the remote's pitch from the acceleration. */
static void calc_acc_vertical(KPADInsideStatus* kp)
{
    f32 vx, f1, vy, ax, ay;

    f1 = kp->hard_acc.x * kp->hard_acc.x + kp->hard_acc.y * kp->hard_acc.y;
    vx = (f32)sqrt(f1);
    ay = -kp->hard_acc.z;
    f1 = (f32)sqrt(f1 + ay * ay);
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
    f1 = (f32)sqrt(vx * vx + vy * vy);
    if (f1 == 0.0f) {
        return;
    }
    kp->status.acc_vertical.x = vx / f1;
    kp->status.acc_vertical.y = vy / f1;
}

/* Limits an acceleration reading to +/- `max`. */
static inline f32 clamp_acc(f32 acc, f32 max)
{
    if (acc < 0.0f) {
        if (acc < -max) {
            return -max;
        }
    } else {
        if (acc > max) {
            return max;
        }
    }
    return acc;
}

/* 0x804BDF20 (0x460): Converts the core and Nunchuk acceleration of one sample and smooths it. */
static void read_kpad_acc(KPADInsideStatus* kp, KPADUnifiedWpadStatus* uwp)
{
    Vec vec;
    Vec prev;
    Vec fs_prev;

    switch (uwp->fmt) {
    case KP_FMT_CORE_ACC:
    case KP_FMT_CORE_ACC_DPD:
    case KP_FMT_FS_ACC:
    case KP_FMT_FS_ACC_DPD:
    case KP_FMT_CLASSIC_ACC:
    case KP_FMT_CLASSIC_ACC_DPD:
    case KP_FMT_GUITAR:
        break;
    default:
        return;
    }

    kp->hard_acc.x = clamp_acc((f32)-uwp->u.core.accX * kp->acc_scale.x, kp_rm_acc_max);
    kp->hard_acc.y = clamp_acc((f32)-uwp->u.core.accZ * kp->acc_scale.z, kp_rm_acc_max);
    kp->hard_acc.z = clamp_acc((f32)uwp->u.core.accY * kp->acc_scale.y, kp_rm_acc_max);

    prev = kp->status.acc;
    calc_acc(kp, &kp->status.acc.x, kp->hard_acc.x);
    calc_acc(kp, &kp->status.acc.y, kp->hard_acc.y);
    calc_acc(kp, &kp->status.acc.z, kp->hard_acc.z);
    kp->status.acc_value = (f32)sqrt(kp->status.acc.x * kp->status.acc.x + kp->status.acc.y * kp->status.acc.y
                                     + kp->status.acc.z * kp->status.acc.z);
    prev.x -= kp->status.acc.x;
    prev.y -= kp->status.acc.y;
    prev.z -= kp->status.acc.z;
    kp->status.acc_speed = (f32)sqrt(prev.x * prev.x + prev.y * prev.y + prev.z * prev.z);

    calc_acc_horizon(kp);
    calc_acc_vertical(kp);

    if (uwp->u.core.err != 0 || uwp->u.core.dev != KP_DEV_FREESTYLE
        || (uwp->fmt != KP_FMT_FS_ACC && uwp->fmt != KP_FMT_FS_ACC_DPD)) {
        return;
    }

    vec.x = clamp_acc((f32)-uwp->u.fs.fsAccX * kp->fs_acc_scale.x, kp_fs_acc_max);
    vec.y = clamp_acc((f32)-uwp->u.fs.fsAccZ * kp->fs_acc_scale.z, kp_fs_acc_max);
    vec.z = clamp_acc((f32)uwp->u.fs.fsAccY * kp->fs_acc_scale.y, kp_fs_acc_max);
    if (kp->fs_acc_rotate) {
        PSMTXMultVec(kp_acc_mtx, &vec, &vec);
    }

    fs_prev = kp->status.ex_status.fs.acc;
    calc_acc(kp, &kp->status.ex_status.fs.acc.x, vec.x);
    calc_acc(kp, &kp->status.ex_status.fs.acc.y, vec.y);
    calc_acc(kp, &kp->status.ex_status.fs.acc.z, vec.z);
    kp->status.ex_status.fs.acc_value = (f32)sqrt(kp->status.ex_status.fs.acc.x * kp->status.ex_status.fs.acc.x
                                                  + kp->status.ex_status.fs.acc.y * kp->status.ex_status.fs.acc.y
                                                  + kp->status.ex_status.fs.acc.z * kp->status.ex_status.fs.acc.z);
    fs_prev.x -= kp->status.ex_status.fs.acc.x;
    fs_prev.y -= kp->status.ex_status.fs.acc.y;
    fs_prev.z -= kp->status.ex_status.fs.acc.z;
    kp->status.ex_status.fs.acc_speed = (f32)sqrt(fs_prev.x * fs_prev.x + fs_prev.y * fs_prev.y + fs_prev.z * fs_prev.z);
}

/* 0x804BE380 (0x1E8): Picks the first pair of pointer objects that lines up with the remote's roll. */
static s8 select_2obj_first(KPADInsideStatus* kp)
{
    KPADObj *op1, *op2, *rp1, *rp2;
    Vec2 nrm;
    f32 vy, vx, f1, dist;
    f32 max = kp_err_first_inpr;

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
            dist = 1.0f / (f32)sqrt(vx * vx + vy * vy);
            vx *= dist;
            vy *= dist;
            nrm.x = kp->sec_nrm11.x * vx + kp->sec_nrm11.y * vy;
            nrm.y = kp->sec_nrm11.y * vx - kp->sec_nrm11.x * vy;
            dist = kp->dist_scale * dist;
            if (dist <= kp->dist_min || dist >= kp_err_dist_max) {
                continue;
            }
            f1 = kp->acc_horizon.x * nrm.x + kp->acc_horizon.y * nrm.y;
            if (f1 < 0.0f) {
                f1 = -f1;
                if (f1 > max) {
                    max = f1;
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
    kp->kobj_regular[0] = *rp1;
    kp->kobj_regular[1] = *rp2;
    return 2;
}

/* 0x804BE570 (0x228): Picks the pair of pointer objects closest to last frame's pair in distance and direction. */
static s8 select_2obj_continue(KPADInsideStatus* kp)
{
    KPADObj *op1, *op2, *rp1, *rp2;
    Vec2 nrm;
    s32 rev_fg;
    f32 vx, vy, f1, dist;
    f32 min = 2.0f;

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
            dist = 1.0f / (f32)sqrt(vx * vx + vy * vy);
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
    kp->kobj_regular[0] = *rp1;
    kp->kobj_regular[1] = *rp2;
    return 2;
}

/* 0x804BE7A0 (0x1B4): Pairs a lone pointer object with a partner placed outside the frame along the roll. */
static s8 select_1obj_first(KPADInsideStatus* kp)
{
    KPADObj* op1;
    Vec2 p1, p2;
    f32 vx, vy;

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
                kp->kobj_regular[1] = *op1;
                kp->kobj_regular[0].center = p1;
                kp->kobj_regular[0].error_fg = 0;
                kp->kobj_regular[0].state_fg = -1;
                return -1;
            }
        } else {
            if (p2.x <= kp->frame_min.x || p2.x >= kp->frame_max.x || p2.y <= kp->frame_min.y
                || p2.y >= kp->frame_max.y) {
                kp->kobj_regular[0] = *op1;
                kp->kobj_regular[1].center = p2;
                kp->kobj_regular[1].error_fg = 0;
                kp->kobj_regular[1].state_fg = -1;
                return -1;
            }
        }
    } while (++op1 < &kp->kobj_sample[4]);
    return 0;
}

/* 0x804BE960 (0x174): Follows last frame's tracked object to the nearest sample and rebuilds its partner. */
static s8 select_1obj_continue(KPADInsideStatus* kp)
{
    KPADObj *op1, *op2, *rp1, *rp2;
    f32 vx, vy, f1;
    f32 min = kp_err_near_pos * kp_err_near_pos;

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
    *rp1 = *rp2;

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

/* 0x804BEAE0 (0x5CC): Derives the pointer's horizon, distance and screen position from the tracked object pair. */
static void calc_dpd_variable(KPADInsideStatus* kp, s8 valid)
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
        kp->status.horizon = pos;
        kp->status.hori_vec = icenter_org;
        kp->status.hori_speed = 0.0f;
    } else {
        vec.x = pos.x - kp->status.horizon.x;
        vec.y = pos.y - kp->status.horizon.y;
        f1 = (f32)sqrt(vec.x * vec.x + vec.y * vec.y);
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
            f1 = (f32)sqrt(vec.x * vec.x + vec.y * vec.y);
            vec.x /= f1;
            vec.y /= f1;
            kp->status.hori_vec.x = vec.x - kp->status.horizon.x;
            kp->status.hori_vec.y = vec.y - kp->status.horizon.y;
            kp->status.hori_speed = (f32)sqrt(kp->status.hori_vec.x * kp->status.hori_vec.x
                                              + kp->status.hori_vec.y * kp->status.hori_vec.y);
            kp->status.horizon = vec;
        } else if (f1 > kp->hori_play_radius) {
            f2 = kp->hori_sensitivity * ((f1 - kp->hori_play_radius) / f1);
            vec.x = kp->status.horizon.x + vec.x * f2;
            vec.y = kp->status.horizon.y + vec.y * f2;
            f1 = (f32)sqrt(vec.x * vec.x + vec.y * vec.y);
            vec.x /= f1;
            vec.y /= f1;
            kp->status.hori_vec.x = vec.x - kp->status.horizon.x;
            kp->status.hori_vec.y = vec.y - kp->status.horizon.y;
            kp->status.hori_speed = (f32)sqrt(kp->status.hori_vec.x * kp->status.hori_vec.x
                                              + kp->status.hori_vec.y * kp->status.hori_vec.y);
            kp->status.horizon = vec;
        } else {
            kp->status.hori_vec = icenter_org;
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

    {
        f32 nx = kp->sec_nrm.x * kp->sec_nrm11.x + kp->sec_nrm.y * kp->sec_nrm11.y;
        f32 ny = -kp->sec_nrm.y * kp->sec_nrm11.x + kp->sec_nrm.x * kp->sec_nrm11.y;
        f32 mx = 0.5f * (kp->kobj_regular[0].center.x + kp->kobj_regular[1].center.x);
        f32 my = 0.5f * (kp->kobj_regular[0].center.y + kp->kobj_regular[1].center.y);

        vec.x = kp->dpd2pos_scale * (kp->center_org.x - (nx * mx - ny * my));
        vec.y = kp->dpd2pos_scale * (kp->center_org.y - (ny * mx + nx * my));
    }
    pos.x = -kp->accXY_nrm_hori.y * vec.x + kp->accXY_nrm_hori.x * vec.y;
    pos.y = -kp->accXY_nrm_hori.x * vec.x - kp->accXY_nrm_hori.y * vec.y;
    if (kp->status.dpd_valid_fg == 0) {
        kp->status.pos = pos;
        kp->status.vec = icenter_org;
        kp->status.speed = 0.0f;
    } else {
        vec.x = pos.x - kp->status.pos.x;
        vec.y = pos.y - kp->status.pos.y;
        f1 = (f32)sqrt(vec.x * vec.x + vec.y * vec.y);
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
            kp->status.speed = (f32)sqrt(kp->status.vec.x * kp->status.vec.x + kp->status.vec.y * kp->status.vec.y);
            kp->status.pos.x += kp->status.vec.x;
            kp->status.pos.y += kp->status.vec.y;
        } else if (f1 > kp->pos_play_radius) {
            f2 = kp->pos_sensitivity * ((f1 - kp->pos_play_radius) / f1);
            kp->status.vec.x = f2 * vec.x;
            kp->status.vec.y = f2 * vec.y;
            kp->status.speed = (f32)sqrt(kp->status.vec.x * kp->status.vec.x + kp->status.vec.y * kp->status.vec.y);
            kp->status.pos.x += kp->status.vec.x;
            kp->status.pos.y += kp->status.vec.y;
        } else {
            kp->status.vec = icenter_org;
            kp->status.speed = 0.0f;
        }
    }

    kp->status.dpd_valid_fg = valid;
}

/* 0x804BF0B0 (0x47C): Converts the pointer objects of one sample, picks the tracked pair and updates the pointer
 * state. */
static void read_kpad_dpd(KPADInsideStatus* kp, KPADUnifiedWpadStatus* uwp)
{
    WPADDpdObject* wp;
    KPADObj *op1, *op2, *last;
    f32 vy, vx, f1;
    s8 valid;

    if (uwp->fmt == KP_FMT_CORE_ACC_DPD || uwp->fmt == KP_FMT_FS_ACC_DPD || uwp->fmt == KP_FMT_CLASSIC_ACC_DPD
        || (uwp->fmt == KP_FMT_GUITAR && kp->dpd_enabled)) {
        wp = &uwp->u.core.obj[3];
        op1 = &kp->kobj_sample[3];
        do {
            if ((u16)wp->size != 0) {
                op1->center.x = 0.001953125f * (f32)wp->x - 0.99902344f;
                op1->center.y = 0.001953125f * (f32)wp->y - 0.74902344f;
                op1->error_fg = 0;
                op1->state_fg = 0;
            } else {
                op1->error_fg = -1;
            }
            --wp;
        } while (--op1 >= kp->kobj_sample);
    } else {
        op1 = &kp->kobj_sample[3];
        do {
            op1->error_fg = -1;
        } while (--op1 >= kp->kobj_sample);
    }

    last = &kp->kobj_sample[3];
    op1 = last;
    do {
        if (op1->error_fg < 0) {
            continue;
        }
        if (op1->center.x <= kp->frame_min.x || op1->center.x >= kp->frame_max.x || op1->center.y <= kp->frame_min.y
            || op1->center.y >= kp->frame_max.y) {
            op1->error_fg |= 1;
        }
    } while (--op1 >= kp->kobj_sample);

    op2 = kp->kobj_sample;
    do {
        if (op2->error_fg != 0) {
            continue;
        }
        op1 = op2 + 1;
        do {
            if (op1->error_fg == 0 && op2->center.x == op1->center.x && op2->center.y == op1->center.y) {
                op1->error_fg |= 2;
            }
        } while (++op1 <= last);
    } while (++op2 < last);

    kp->valid_objs = 0;
    op1 = &kp->kobj_sample[3];
    do {
        if (op1->error_fg == 0) {
            ++kp->valid_objs;
        }
    } while (--op1 >= kp->kobj_sample);

    for (;;) {
        if (!(kp->status.acc_vertical.x <= kp_err_up_inpr)) {
            if (kp->status.dpd_valid_fg == 2 || kp->status.dpd_valid_fg == -2) {
                if (kp->valid_objs >= 2 && (valid = select_2obj_continue(kp)) != 0) {
                    break;
                }
                if (kp->valid_objs >= 1 && (valid = select_1obj_continue(kp)) != 0) {
                    break;
                }
            } else if (kp->status.dpd_valid_fg == 1 || kp->status.dpd_valid_fg == -1) {
                if (kp->valid_objs >= 2 && (valid = select_2obj_first(kp)) != 0) {
                    break;
                }
                if (kp->valid_objs >= 1 && (valid = select_1obj_continue(kp)) != 0) {
                    break;
                }
            } else {
                if (kp->valid_objs >= 2 && (valid = select_2obj_first(kp)) != 0) {
                    break;
                }
                if (kp->valid_objs == 1 && (valid = select_1obj_first(kp)) != 0) {
                    break;
                }
            }
        }
        valid = 0;
        break;
    }

    if (valid != 0) {
        vx = kp->kobj_regular[1].center.x - kp->kobj_regular[0].center.x;
        vy = kp->kobj_regular[1].center.y - kp->kobj_regular[0].center.y;
        f1 = (f32)sqrt(vx * vx + vy * vy);
        kp->sec_dist = f1;
        f1 = 1.0f / f1;
        kp->sec_nrm.x = vx *= f1;
        kp->dist_last = kp->dist_scale * f1;
        kp->sec_nrm.y = vy *= f1;
        kp->dpd_horizon.x = kp->sec_nrm11.x * vx + kp->sec_nrm11.y * vy;
        kp->dpd_horizon.y = kp->sec_nrm11.y * vx - kp->sec_nrm11.x * vy;
        if (kp->ah_circle_ct == 0
            && kp->dpd_horizon.x * kp->acc_horizon.x + kp->dpd_horizon.y * kp->acc_horizon.y <= kp_err_acc_inpr) {
            kp->kobj_regular[0].error_fg = kp->kobj_regular[1].error_fg = 1;
            valid = 0;
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

    calc_dpd_variable(kp, valid);
}

/* 0x804BF530 (0x128): Maps a stick reading to the unit circle with a dead zone of `min` and full scale at `max`. */
static void clamp_stick_circle(Vec2* stick, s32 sx, s32 sy, s32 min, s32 max)
{
    f32 fx = (f32)sx;
    f32 fy = (f32)sy;
    f32 fmin = (f32)min;
    f32 fmax = (f32)max;
    f32 r = (f32)sqrt(fx * fx + fy * fy);

    if (r <= fmin) {
        stick->x = stick->y = 0.0f;
    } else if (r >= fmax) {
        stick->x = fx / r;
        stick->y = fy / r;
    } else {
        r = (r - fmin) / (fmax - fmin) / r;
        stick->x = fx * r;
        stick->y = fy * r;
    }
}

/* 0x804BF660 (0x224): Maps each stick axis separately with a dead zone of `min` and full scale at `max`. */
static void clamp_stick_cross(Vec2* stick, s32 sx, s32 sy, s32 min, s32 max)
{
    f32 f1;

    if (sx < 0) {
        if (-sx <= min) {
            stick->x = 0.0f;
        } else if (-sx >= max) {
            stick->x = 1.0f;
        } else {
            stick->x = (f32)(-sx - min) / (f32)(max - min);
        }
        stick->x = -stick->x;
    } else {
        if (sx <= min) {
            stick->x = 0.0f;
        } else if (sx >= max) {
            stick->x = 1.0f;
        } else {
            stick->x = (f32)(sx - min) / (f32)(max - min);
        }
    }

    if (sy < 0) {
        if (-sy <= min) {
            stick->y = 0.0f;
        } else if (-sy >= max) {
            stick->y = 1.0f;
        } else {
            stick->y = (f32)(-sy - min) / (f32)(max - min);
        }
        stick->y = -stick->y;
    } else {
        if (sy <= min) {
            stick->y = 0.0f;
        } else if (sy >= max) {
            stick->y = 1.0f;
        } else {
            stick->y = (f32)(sy - min) / (f32)(max - min);
        }
    }

    f1 = stick->x * stick->x + stick->y * stick->y;
    if (f1 > 1.0f) {
        f1 = (f32)sqrt(f1);
        stick->x /= f1;
        stick->y /= f1;
    }
}

/* Maps an analog extension reading to 0..1 with a dead zone below `min` and full scale at `max`. */
static inline void calc_ex_analog(f32* dst, s32 value, s32 min, s32 max)
{
    if (value <= min) {
        *dst = 0.0f;
    } else if (value >= max) {
        *dst = 1.0f;
    } else {
        *dst = (f32)(value - min) / (f32)(max - min);
    }
}

/* Restarts the balance-board zero-point calibration and clears its weight buffers. */
static inline void clear_wbc_weights(void)
{
    s32 i;

    kp_wbc_state = 0;
    kp_wbc_wait_count = 0;
    kp_wbc_zero_pending = 0;
    for (i = 0; i < 4; i++) {
        kp_wbc_weight[i] = 0.0;
        kp_wbc_zero[i] = 0.0;
    }
}

/* Clears the classic-style extension state after a device change. */
static inline void reset_cl_status(KPADInsideStatus* kp, KPADEXStatus* ex)
{
    if (kp->dev_changed) {
        kp->dev_changed = 0;
        ex->cl.lstick = icenter_org;
        ex->cl.rstick = icenter_org;
        ex->cl.ltrigger = ex->cl.rtrigger = 0.0f;
        ex->cl.hold = ex->cl.trig = ex->cl.release = 0;
        kp->btn_cl_repeat_time = 0;
        kp->btn_cl_repeat_next = kp->btn_repeat_delay;
    }
}

/* 0x804BF890 (0x8D0): Converts the attached extension's sticks, triggers or load cells of one sample. */
static void read_kpad_ext(KPADInsideStatus* kp, KPADUnifiedWpadStatus* uwp)
{
    KPADClampStick clamp = clamp_stick_circle;
    KPADEXStatus* ex = &kp->status.ex_status;
    s32 i;

    if (kp_stick_clamp_mode != 0) {
        clamp = clamp_stick_cross;
    }

    if (uwp->u.core.dev == KP_DEV_FREESTYLE && uwp->fmt >= KP_FMT_FS && uwp->fmt <= KP_FMT_FS_ACC_DPD) {
        if (kp->dev_changed) {
            kp->dev_changed = 0;
            ex->fs.stick = icenter_org;
            ex->fs.acc.x = ex->fs.acc.z = 0.0f;
            ex->fs.acc.y = -1.0f;
            ex->fs.acc_value = 1.0f;
            ex->fs.acc_speed = 0.0f;
        }
        clamp(&ex->fs.stick, uwp->u.fs.fsStickX, uwp->u.fs.fsStickY, kp_fs_fstick_min, kp_fs_fstick_max);
    } else if (uwp->u.core.dev == KP_DEV_CLASSIC && uwp->fmt >= KP_FMT_CLASSIC && uwp->fmt <= KP_FMT_CLASSIC_ACC_DPD) {
        reset_cl_status(kp, ex);
        clamp(&ex->cl.lstick, uwp->u.cl.clLStickX, uwp->u.cl.clLStickY, kp_cl_stick_min, kp_cl_stick_max);
        clamp(&ex->cl.rstick, uwp->u.cl.clRStickX, uwp->u.cl.clRStickY, kp_cl_stick_min, kp_cl_stick_max);
        calc_ex_analog(&ex->cl.ltrigger, uwp->u.cl.clTriggerL, kp_cl_trigger_min, kp_cl_trigger_max);
        calc_ex_analog(&ex->cl.rtrigger, uwp->u.cl.clTriggerR, kp_cl_trigger_min, kp_cl_trigger_max);
    } else if (uwp->u.core.dev == KP_DEV_GUITAR && uwp->fmt == KP_FMT_GUITAR) {
        reset_cl_status(kp, ex);
        clamp(&ex->cl.lstick, uwp->u.cl.clLStickX, uwp->u.cl.clLStickY, kp_cl_stick_min, kp_cl_stick_max);
        calc_ex_analog(&ex->cl.rstick.x, uwp->u.cl.clRStickX, kp_ex_analog_min, kp_ex_analog_max);
        calc_ex_analog(&ex->cl.rstick.y, uwp->u.cl.clRStickY, kp_ex_analog_min, kp_ex_analog_max);
        calc_ex_analog(&ex->cl.ltrigger, uwp->u.cl.clTriggerL, kp_ex_trigger_min, kp_ex_trigger_max);
        calc_ex_analog(&ex->cl.rtrigger, uwp->u.cl.clTriggerR, kp_ex_trigger_min, kp_ex_trigger_max);
    } else if (uwp->u.core.dev == KP_DEV_TRAIN && uwp->fmt == KP_FMT_TRAIN) {
        reset_cl_status(kp, ex);
        ex->cl.lstick.x = ex->cl.lstick.y = 0.0f;
        ex->cl.rstick.x = ex->cl.rstick.y = 0.0f;
        calc_ex_analog(&ex->cl.ltrigger, uwp->u.tr.brake, kp_ex_trigger_min, kp_ex_trigger_max);
        calc_ex_analog(&ex->cl.rtrigger, uwp->u.tr.mascon, kp_ex_trigger_min, kp_ex_trigger_max);
    } else if (uwp->u.core.dev == KP_DEV_BALANCE && uwp->fmt == KP_FMT_BALANCE) {
        if (kp->dev_changed) {
            kp->dev_changed = 0;
            clear_wbc_weights();
        }
        if (WPADiReturnZeroC(uwp->u.bl.battery) == 0) {
            ex->bl.weight_err = -1;
        } else if (kp_wbc_state < 3) {
            ex->bl.weight_err = -2;
        } else if (uwp->u.bl.temp == 127 || uwp->u.bl.temp == -128) {
            ex->bl.weight_err = -3;
        } else {
            ex->bl.weight_err = WBCReadDummy(&uwp->u.bl, ex->bl.weight, 4);
            if (ex->bl.weight_err >= 0) {
                for (i = 0; i < 4; i++) {
                    kp_wbc_zero[i] = (kp_wbc_weight_max * kp_wbc_zero[i] + ex->bl.weight[i])
                                     / (1.0f + kp_wbc_weight_max);
                }
                if (kp_wbc_zero_pending) {
                    kp_wbc_wait_count++;
                    for (i = 0; i < 4; i++) {
                        kp_wbc_weight[i] = ((kp_wbc_wait_count - 1) * kp_wbc_weight[i] + ex->bl.weight[i])
                                           / kp_wbc_wait_count;
                    }
                    if (kp_wbc_wait_count == kp_wbc_weight_max) {
                        kp_wbc_zero_pending = 0;
                        ex->bl.weight_err = WBCGetTGCWeightDummy(kp_wbc_weight[0] + kp_wbc_weight[1]
                                                                     + kp_wbc_weight[2] + kp_wbc_weight[3],
                                                                 &kp_wbc_tgc_weight, &uwp->u.bl);
                        if (kp_wbc_tgc_weight < -0.5) {
                            ex->bl.weight_err = -4;
                        }
                    }
                }
            }
            for (i = 0; i < 4; i++) {
                ex->bl.weight_ave[i] = kp_wbc_zero[i];
            }
            ex->bl.tgc_weight = kp_wbc_tgc_weight;
            ex->bl.tgc_weight_err = kp_wbc_zero_pending;
        }
    }
}

/* 0x804C0160 (0xC): Reads up to `length` processed samples of channel `chan`, newest first. */
s32 KPADRead(s32 chan, KPADStatus* samplingBufs, u32 length)
{
    return KPADiRead(chan, samplingBufs, length, NULL, FALSE);
}

/* Notifies the pointer-enable callback once the pointer is on and latches WPAD's pointer state. */
static inline void finish_control_dpd(KPADInsideStatus* kp, s32 chan, s32 result)
{
    if (result == 0 && kp->control_dpd_cb != NULL && !kp->dpd_on_notified) {
        kp->dpd_on_notified = 1;
        kp->control_dpd_cb(chan, 1);
        kp->dpd_off_notified = 0;
    }
    kp->dpd_enabled = WPADIsDpdEnabled(chan);
    kp->dpd_ctrl_busy = 0;
}

/* Returns raw sample `idx` of the channel's ring: the work record's own ring first, then the caller's. */
static inline KPADUnifiedWpadStatus* get_ring_sample(KPADInsideStatus* kp, s32 idx)
{
    if (idx >= KPAD_RING_BUFS) {
        return &kp->uni_ring_bf[idx - KPAD_RING_BUFS];
    }
    return &kp->wpad_ring_bf[idx];
}

/* 0x804C0170 (0x72C): Moves channel `chan`'s raw samples out of its ring and processes them into `samplingBufs`,
 * newest first; returns the sample count and the read state through `err`. */
s32 KPADiRead(s32 chan, KPADStatus* samplingBufs, u32 length, s32* err, BOOL readEx)
{
    KPADInsideStatus* kp = &inside_kpads[chan];
    KPADUnifiedWpadStatus* uwp;
    KPADStatus* sp;
    KPADStatus saved;
    BOOL enabled;
    s32 read_err = KPAD_READ_ERR_NONE;
    s32 count = 0;
    s32 idx, i;
    u32 n;
    s32 corrupt, dev_mismatch;
    u32 dev_type;
    u32 core_btn, fs_btn, cl_btn;

    if (!kp_initialized) {
        read_err = KPAD_READ_ERR_INIT;
    } else if (WPADGetStatus() != 3) {
        read_err = KPAD_READ_ERR_SETUP;
    } else {
        enabled = OSDisableInterrupts();
        if (kp->read_busy) {
            OSRestoreInterrupts(enabled);
            read_err = KPAD_READ_ERR_LOCKED;
        } else {
            kp->read_busy = 1;
            if (WPADProbe(chan, NULL) == -1) {
                reset_kpad(kp);
                if (kp->control_dpd_cb != NULL && kp->dpd_off_notified && !kp->dpd_on_notified) {
                    finish_control_dpd(kp, chan, 0);
                }
                kp->read_busy = 0;
                OSRestoreInterrupts(enabled);
                read_err = KPAD_READ_ERR_NO_CONTROLLER;
            } else {
                OSRestoreInterrupts(enabled);
                if (kp->reset_req) {
                    kp->status.wpad_err = KP_WPAD_ERR_INVALID;
                    reset_kpad(kp);
                }
                saved = *samplingBufs;
                if (kp->wpad_ring_lvl > 1 && samplingBufs != NULL && length != 0) {
                    enabled = OSDisableInterrupts();
                    n = kp->wpad_ring_lvl;
                    if (n > length) {
                        n = length;
                    }
                    i = n;
                    corrupt = 0;
                    dev_mismatch = 0;
                    idx = kp->wpad_ring_idx - 1;
                    if (idx < 0) {
                        idx = idx + kp->uni_ring_len + KPAD_RING_BUFS;
                    }
                    uwp = get_ring_sample(kp, idx);
                    do {
                        if (corrupt) {
                            if (uwp->u.core.err == 0) {
                                uwp->u.core.err = KP_WPAD_ERR_CORRUPTED;
                                corrupt = 0;
                            } else if (uwp->u.core.err == KP_WPAD_ERR_BUSY) {
                                corrupt = 0;
                            }
                        } else if (uwp->u.core.err == KP_WPAD_ERR_CORRUPTED) {
                            corrupt = 1;
                        }
                        if (kp->status.dev_type != uwp->u.core.dev) {
                            dev_mismatch = 1;
                        }
                        if (dev_mismatch) {
                            uwp->u.core.err = KP_WPAD_ERR_INVALID;
                        }
                        if (--idx < 0) {
                            idx = idx + kp->uni_ring_len + KPAD_RING_BUFS;
                        }
                        uwp = get_ring_sample(kp, idx);
                    } while (--i > 0);

                    kp->wpad_ring_lvl = 1;
                    if (n > 1) {
                        count = n - 1;
                    } else {
                        count = 1;
                    }
                    i = count;
                    idx = kp->wpad_ring_idx - count - 1;
                    sp = &samplingBufs[count];
                    if (idx < 0) {
                        idx = idx + kp->uni_ring_len + KPAD_RING_BUFS;
                    }
                    do {
                        uwp = get_ring_sample(kp, idx);
                        ++idx;
                        *(KPADUnifiedWpadStatus*)--sp = *uwp;
                        if (idx >= kp->uni_ring_len + KPAD_RING_BUFS) {
                            idx = 0;
                        }
                    } while (--i > 0);
                    OSRestoreInterrupts(enabled);

                    i = count;
                    sp = &samplingBufs[count];
                    if (kp->btn_proc_mode == 1) {
                        do {
                            uwp = (KPADUnifiedWpadStatus*)--sp;
                            if (kp->status.dev_type != uwp->u.core.dev) {
                                kp->status.dev_type = uwp->u.core.dev;
                                kp->dev_changed = 1;
                            }
                            kp->status.wpad_err = uwp->u.core.err;
                            core_btn = fs_btn = cl_btn = 0xFFFF;
                            kp->status.data_format = uwp->fmt;
                            dev_type = kp->status.dev_type;
                            switch (kp->status.wpad_err) {
                            case 0:
                                if (dev_type == KP_DEV_FREESTYLE) {
                                    fs_btn = uwp->u.core.button;
                                    cl_btn = 0;
                                } else if (dev_type == KP_DEV_CLASSIC || dev_type == KP_DEV_TRAIN
                                           || dev_type == KP_DEV_GUITAR) {
                                    cl_btn = uwp->u.cl.clButton;
                                    fs_btn = 0;
                                } else {
                                    cl_btn = 0;
                                    fs_btn = 0;
                                }
                                core_btn = uwp->u.core.button & 0x9F1F;
                                break;
                            case KP_WPAD_ERR_CORRUPTED:
                                cl_btn = 0;
                                fs_btn = 0;
                            case KP_WPAD_ERR_BUSY:
                                core_btn = uwp->u.core.button & 0x9F1F;
                                break;
                            }
                            if (core_btn == 0xFFFF) {
                                core_btn = kp->status.hold & 0x9F1F;
                            }
                            if (fs_btn == 0xFFFF) {
                                fs_btn = kp->status.hold;
                            }
                            if (cl_btn == 0xFFFF) {
                                cl_btn = kp->status.ex_status.cl.hold;
                            }
                            read_kpad_button(kp, dev_type, 1, core_btn, fs_btn, cl_btn);
                            switch (uwp->u.core.err) {
                            case 0:
                                read_kpad_ext(kp, uwp);
                            case KP_WPAD_ERR_CORRUPTED:
                                read_kpad_acc(kp, uwp);
                                read_kpad_dpd(kp, uwp);
                                break;
                            default:
                                if (!readEx) {
                                    kp->status.dpd_valid_fg = 0;
                                }
                                break;
                            }
                            *sp = kp->status;
                        } while (--i > 0);
                    } else {
                        core_btn = fs_btn = cl_btn = 0xFFFF;
                        do {
                            uwp = (KPADUnifiedWpadStatus*)--sp;
                            if (kp->status.dev_type != uwp->u.core.dev) {
                                kp->status.dev_type = uwp->u.core.dev;
                                kp->dev_changed = 1;
                            }
                            dev_type = uwp->u.core.dev;
                            switch (uwp->u.core.err) {
                            case 0:
                                if (dev_type == KP_DEV_FREESTYLE) {
                                    fs_btn = uwp->u.core.button;
                                    cl_btn = 0;
                                } else if (dev_type == KP_DEV_CLASSIC || dev_type == KP_DEV_TRAIN
                                           || dev_type == KP_DEV_GUITAR) {
                                    cl_btn = uwp->u.cl.clButton;
                                    fs_btn = 0;
                                } else {
                                    cl_btn = 0;
                                    fs_btn = 0;
                                }
                            case KP_WPAD_ERR_CORRUPTED:
                            case KP_WPAD_ERR_BUSY:
                                core_btn = uwp->u.core.button & 0x9F1F;
                                break;
                            }
                        } while (--i > 0);
                        if (core_btn == 0xFFFF) {
                            core_btn = kp->status.hold & 0x9F1F;
                        }
                        if (fs_btn == 0xFFFF) {
                            fs_btn = kp->status.hold;
                        }
                        if (cl_btn == 0xFFFF) {
                            cl_btn = kp->status.ex_status.cl.hold;
                        }
                        read_kpad_button(kp, dev_type, count, core_btn, fs_btn, cl_btn);

                        i = count;
                        sp = &samplingBufs[count];
                        do {
                            uwp = (KPADUnifiedWpadStatus*)--sp;
                            kp->status.wpad_err = uwp->u.core.err;
                            kp->status.data_format = uwp->fmt;
                            switch (kp->status.wpad_err) {
                            case 0:
                                read_kpad_ext(kp, uwp);
                            case KP_WPAD_ERR_CORRUPTED:
                                read_kpad_acc(kp, uwp);
                                read_kpad_dpd(kp, uwp);
                                break;
                            default:
                                if (!readEx) {
                                    kp->status.dpd_valid_fg = 0;
                                }
                                break;
                            }
                            *sp = kp->status;
                        } while (--i > 0);
                    }
                }
                kp->read_busy = 0;
            }
        }
    }

    if (count == 0) {
        if (read_err == KPAD_READ_ERR_NONE) {
            if (readEx) {
                *samplingBufs = saved;
            }
            read_err = KPAD_READ_ERR_NO_DATA;
        } else if (read_err == KPAD_READ_ERR_NO_CONTROLLER && readEx) {
            samplingBufs->dev_type = KP_DEV_NOT_FOUND;
            samplingBufs->data_format = 0;
            samplingBufs->wpad_err = -1;
        }
    }
    if (err != NULL) {
        *err = read_err;
    }
    return count;
}

/* 0x804C08A0 (0xC): Initialises the library without a caller-supplied sample ring. */
void KPADInit(void)
{
    KPADInitEx(NULL, 0);
}

/* Clears the balance-board command state and its weight buffers. */
static inline void reset_wbc(void)
{
    s32 i;

    kp_wbc_busy = 0;
    kp_wbc_setup_done = 0;
    kp_wbc_setup_busy = 0;
    kp_wbc_state = 0;
    kp_wbc_wait_count = 0;
    kp_wbc_zero_pending = 0;
    for (i = 0; i < 4; i++) {
        kp_wbc_weight[i] = 0.0;
        kp_wbc_zero[i] = 0.0;
    }
}

/* Marks every raw sample of the channel's rings invalid. */
static inline void clear_wpad_ring(KPADInsideStatus* kp)
{
    u32 i;

    for (i = 0; i < KPAD_RING_BUFS; i++) {
        kp->wpad_ring_bf[i].u.core.err = -1;
    }
    for (i = 0; i < kp->uni_ring_len; i++) {
        kp->uni_ring_bf[i].u.core.err = -1;
    }
}

/* Recomputes the pointer-to-screen scale from the pointer centre. */
static inline void calc_dpd2pos_scale(KPADInsideStatus* kp)
{
    f32 fx = 1.0f;
    f32 fy = 0.75f;
    f32 f1 = (f32)sqrt(fx * fx + fy * fy);

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

/* Sets the distance between the two sensor-bar objects and the distances derived from it. */
static inline void set_obj_interval(f32 interval)
{
    BOOL enabled = OSDisableInterrupts();

    kp_obj_interval = interval;
    kp_dist_init = kp_err_dist_min = interval / 0.383864f;
    OSRestoreInterrupts(enabled);
}

/* Stops every remote's motor and asks each channel to reset at its next read. */
static inline void request_reset_all(void)
{
    s32 i;

    for (i = 3; i >= 0; i--) {
        if (WPADGetStatus() == 3) {
            WPADControlMotor(i, 0);
        }
        inside_kpads[i].reset_req = 1;
    }
}

/* 0x804C08B0 (0x444): Initialises WPAD and every channel's work record and installs the WPAD callbacks. */
void KPADInitEx(KPADUnifiedWpadStatus* uniRingBufs, u32 length)
{
    s32 i;
    KPADInsideStatus* kp;

    if (kp_initialized) {
        return;
    }

    WPADInit();
    memset(inside_kpads, 0, sizeof(inside_kpads));
    kp_err_dist_max = 1.0f + (f32)WPADGetDpdSensitivity();

    for (i = 0; i < 4; i++) {
        kp = &inside_kpads[i];
        kp->connect_cb = WPADSetConnectCallback(i, KPADiConnectCallback);
        kp->sampling_cb = WPADSetSamplingCallback(i, KPADiSamplingCallback);
        kp->dpd_enabled = 0;
        reset_wbc();
        kp->dpd_request = 1;
        kp->dpd_command = WPADGetDpdFormat(i);
        kp->status.dev_type = KP_DEV_NOT_FOUND;
        kp->status.data_format = 0;
        kp->dist_org = idist_org;
        kp->accXY_nrm_hori = iaccXY_nrm_hori;
        kp->sec_nrm11 = isec_nrm11;
        kp->center_org = kp_dpd_center_init;
        calc_dpd2pos_scale(kp);

        kp->pos_play_radius = kp->hori_play_radius = kp->dist_play_radius = kp->acc_play_radius = 0.0f;
        kp->pos_sensitivity = kp->hori_sensitivity = kp->dist_sensitivity = kp->acc_sensitivity = 1.0f;
        kp->pos_play_mode = kp->hori_play_mode = kp->dist_play_mode = kp->acc_play_mode = 0;
        kp->btn_repeat_delay = 40000;
        kp->btn_repeat_pulse = 0;
        kp->btn_repeat_time = 0;
        kp->btn_repeat_next = 40000;
        kp->btn_cl_repeat_time = 0;
        kp->btn_cl_repeat_next = 40000;
        kp->btn_proc_mode = 0;
        kp->center_update = 1;
        kp->use_sensor_bar = 1;
        kp->acc_scale_update = 1;
        kp->fs_acc_rotate = 0;

        if (length != 0 && uniRingBufs != NULL) {
            kp->uni_ring_len = length / 4;
            kp->uni_ring_bf = &uniRingBufs[i * kp->uni_ring_len];
        } else {
            kp->uni_ring_len = 0;
            kp->uni_ring_bf = NULL;
        }
        clear_wpad_ring(kp);
    }

    WPADSetCallbackByKPAD(TRUE);
    set_obj_interval(kp_obj_interval);

    request_reset_all();

    kp_acc_mtx[0][0] = 1.0f;
    kp_acc_mtx[0][1] = 0.0f;
    kp_acc_mtx[0][2] = 0.0f;
    kp_acc_mtx[0][3] = 0.0f;
    kp_acc_mtx[1][0] = 0.0f;
    kp_acc_mtx[1][1] = (f32)cos(0.017453292f * kp_init_angle);
    kp_acc_mtx[1][2] = (f32)-sin(0.017453292f * kp_init_angle);
    kp_acc_mtx[1][3] = 0.0f;
    kp_acc_mtx[2][0] = 0.0f;
    kp_acc_mtx[2][1] = (f32)sin(0.017453292f * kp_init_angle);
    kp_acc_mtx[2][2] = (f32)cos(0.017453292f * kp_init_angle);
    kp_acc_mtx[2][3] = 0.0f;

    kp_initialized = 1;
    OSRegisterVersion(__KPADVersion);
}

/* 0x804C0D00 (0xA0): Restores the WPAD callbacks KPADInitEx replaced and marks the library uninitialised. */
void KPADShutdown(void)
{
    s32 i;

    WPADSetCallbackByKPAD(FALSE);
    for (i = 0; i < 4; i++) {
        if (inside_kpads[i].sampling_cb != NULL) {
            WPADSetSamplingCallback(i, inside_kpads[i].sampling_cb);
        } else {
            WPADSetSamplingCallback(i, NULL);
        }
        if (inside_kpads[i].connect_cb != NULL) {
            WPADSetConnectCallback(i, inside_kpads[i].connect_cb);
        } else {
            WPADSetConnectCallback(i, NULL);
        }
    }
    kp_initialized = 0;
}

/* 0x804C0DA0 (0x164): Resets a channel's state when a remote connects or disconnects and forwards the event. */
static void KPADiConnectCallback(s32 chan, s32 reason)
{
    KPADInsideStatus* kp = &inside_kpads[chan];

    if (reason == 0) {
        kp->dev_type_last = 0;
        kp->acc_scale_update = 1;
        WPADSetCallbackByKPAD(FALSE);
        WPADSetSamplingCallback(chan, KPADiSamplingCallback);
        WPADSetCallbackByKPAD(TRUE);
        kp->dpd_enabled = 0;
        kp->dpd_command = 0;
        if (chan == 3) {
            reset_wbc();
        }
    } else {
        clear_wpad_ring(kp);
    }
    if (kp->connect_cb != NULL) {
        kp->connect_cb(chan, reason);
    }
}

/* 0x804C0F10 (0x18): Records whether the balance-board setup command succeeded. */
static void KPADiWbcSetupCallback(s32 chan, s32 result)
{
    kp_wbc_setup_done = (result == 0);
    kp_wbc_busy = 0;
}

/* 0x804C0F30 (0x18): Records whether the balance-board zero-point command succeeded. */
static void KPADiWbcZeroCallback(s32 chan, s32 result)
{
    kp_wbc_state = (result == 0);
    kp_wbc_busy = 0;
}

/* 0x804C0F50 (0x94): Notifies the pointer-enable callback once WPAD has switched the pointer on. */
static void KPADiControlDpdCallback(s32 chan, s32 result)
{
    KPADInsideStatus* kp = &inside_kpads[chan];

    finish_control_dpd(kp, chan, result);
}

/* 0x804C0FF0 (0x768): Latches each WPAD sample into the channel's ring, keeps the remote's scales, pointer centre,
 * pointer mode and data format in step with the attached device, then calls the caller's sampling callback. */
void KPADiSamplingCallback(s32 chan)
{
    KPADInsideStatus* kp = &inside_kpads[chan];
    KPADUnifiedWpadStatus* uwp;
    u32 type;
    WPADAccGravityUnit acc = { 1, 1, 1 };
    u32 idx;
    s32 mode;
    u32 dpd_command;
    u8* ent;
    s32 i;

    if (WPADProbe(chan, &type) != -1) {
        idx = kp->wpad_ring_idx;
        if (idx >= kp->uni_ring_len + KPAD_RING_BUFS) {
            idx = 0;
        }
        if (idx >= KPAD_RING_BUFS) {
            uwp = &kp->uni_ring_bf[idx - KPAD_RING_BUFS];
        } else {
            uwp = &kp->wpad_ring_bf[idx];
        }
        wpadCopyCurrentSample(chan, uwp);
        uwp->fmt = WPADGetDataFormat(chan);
        kp->wpad_ring_idx = idx + 1;
        if (kp->wpad_ring_lvl < kp->uni_ring_len + KPAD_RING_BUFS) {
            kp->wpad_ring_lvl++;
        }

        if (type != kp->dev_type_last && type == KP_DEV_FREESTYLE) {
            kp->acc_scale_update = 1;
        }
        if (kp->acc_scale_update) {
            WPADGetAccGravityUnit(chan, 0, &acc);
            if (acc.x * acc.y * acc.z != 0) {
                kp->acc_scale.x = 1.0f / (f32)acc.x;
                kp->acc_scale.y = 1.0f / (f32)acc.y;
                kp->acc_scale.z = 1.0f / (f32)acc.z;
            } else {
                kp->acc_scale.x = 0.01f;
                kp->acc_scale.y = 0.01f;
                kp->acc_scale.z = 0.01f;
            }
            WPADGetAccGravityUnit(chan, 1, &acc);
            if (acc.x * acc.y * acc.z != 0) {
                kp->fs_acc_scale.x = 1.0f / (f32)acc.x;
                kp->fs_acc_scale.y = 1.0f / (f32)acc.y;
                kp->fs_acc_scale.z = 1.0f / (f32)acc.z;
            } else {
                kp->fs_acc_scale.x = 0.005f;
                kp->fs_acc_scale.y = 0.005f;
                kp->fs_acc_scale.z = 0.005f;
            }
            kp->acc_scale_update = 0;
        }

        if (kp->center_update) {
            f32 ofs;

            if (kp->use_sensor_bar) {
                if (WPADGetSensorBarPosition() == 1) {
                    ofs = 0.2f;
                } else {
                    ofs = -0.2f;
                }
            } else {
                ofs = 0.0f;
            }
            kp->center_org.x = 0.0f;
            kp->center_org.y = -ofs;
            calc_dpd2pos_scale(kp);
            kp->center_update = 0;
        }

        do {
            switch (type) {
            case KP_DEV_CORE:
            case KP_DEV_FUTURE:
            case KP_DEV_NOT_SUPPORTED:
            case KP_DEV_UNKNOWN:
                mode = 0;
                break;
            case KP_DEV_FREESTYLE:
                mode = 2;
                break;
            case KP_DEV_CLASSIC:
                mode = 4;
                break;
            case KP_DEV_GUITAR:
                mode = 6;
                break;
            case KP_DEV_BALANCE:
                mode = 8;
                break;
            case KP_DEV_TRAIN:
                mode = 10;
                break;
            default:
                continue;
            }
            if (kp->dpd_request) {
                mode++;
            }

            if (WPADIsDpdEnabled(chan)) {
                dpd_command = kp->dpd_command;
            } else {
                dpd_command = 0;
            }
            ent = kp_mode_table[mode];
            if (dpd_command != ent[0]) {
                if (kp->control_dpd_cb != NULL && !kp->dpd_off_notified) {
                    kp->dpd_off_notified = 1;
                    kp->control_dpd_cb(chan, 0);
                    kp->dpd_on_notified = 0;
                }
                if (!kp->dpd_ctrl_busy) {
                    kp->dpd_ctrl_busy = 1;
                    if (WPADControlDpd(chan, ent[0], KPADiControlDpdCallback) == 0) {
                        kp->dpd_command = ent[0];
                    }
                }
            } else if (uwp->fmt != ent[1]) {
                WPADSetDataFormat(chan, ent[1]);
            }

            if (type == KP_DEV_BALANCE) {
                if (!kp_wbc_busy && !kp_wbc_setup_done) {
                    if (wpadSendWbcCommand(chan, 0xAA, KPADiWbcSetupCallback) == 0) {
                        kp_wbc_busy = 1;
                    }
                } else if (kp_wbc_setup_done && !kp_wbc_setup_busy) {
                    if (!kp_wbc_busy) {
                        if (WPADiReturnZeroA() == 0) {
                            kp_wbc_busy = 1;
                        }
                    } else {
                        kp_wbc_setup_busy = WPADiReturnZeroB();
                        kp_wbc_busy = !kp_wbc_setup_busy;
                    }
                } else if (!kp_wbc_busy && kp_wbc_setup_busy && kp_wbc_state < 3) {
                    switch (kp_wbc_state) {
                    case 0:
                        if (wpadSendWbcCommand(chan, 0, KPADiWbcZeroCallback) == 0) {
                            kp_wbc_busy = 1;
                        }
                        break;
                    case 1:
                        if (uwp->u.bl.temp == 127 || uwp->u.bl.temp == -128) {
                            clear_wbc_weights();
                        } else if (++kp_wbc_wait_count > kp_wbc_setup_retry) {
                            kp_wbc_state = 2;
                            kp_wbc_wait_count = 0;
                        }
                        break;
                    case 2:
                        if (uwp->u.core.err == 0) {
                            kp_wbc_wait_count++;
                            for (i = 0; i < 4; i++) {
                                kp_wbc_weight[i] = ((kp_wbc_wait_count - 1) * kp_wbc_weight[i] + uwp->u.bl.press[i])
                                                   / kp_wbc_wait_count;
                            }
                            if (kp_wbc_wait_count > kp_wbc_weight_max) {
                                kp_wbc_state = 3;
                                WBCSetZEROPointDummy(kp_wbc_weight, 4);
                            }
                        }
                        break;
                    }
                }
            }
        } while (0);
    }
    if (kp->sampling_cb != NULL) {
        kp->sampling_cb(chan);
    }
}
