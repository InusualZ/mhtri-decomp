/*
 * KPAD/kpad.c - the SDK KPAD library (Wii Remote sample processing, calibration, read).
 *
 * RANGE. `.text` 0x804BD780-0x804C1760; `.data` 0x8061ADA0-0x8061AE00; `.bss` 0x80747540-0x80748B90; `.sdata`
 *   0x80793EB0-0x80793F00; `.sbss` 0x80795260-0x80795298; `.sdata2` 0x8079D200-0x8079D270 (26 functions / 0x3F30
 *   B). One TU: one version string `<< RVL_SDK - KPAD ... >>` (`.data` 0x8061ADA0, registered through `.sdata`
 *   0x80793EB0), one contiguous run per section; the run ends at `MEM/mem_heap.c` (0x804C1760). The tunables the
 *   bodies read live in `mh3_pad.cpp`'s `.sdata` (0x80790EA0..0x80790ED3) and `.sbss` 0x80794800, declared in
 *   `mh3_pad.h`.
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`) with `cflags_base` and `-fp_contract off` on this object:
 *   the target has 0 fused ops across 406 fadd/fsub/fmul; a calc_acc probe gives 42 insns with 3 fmadds under
 *   `-fp_contract on` and 45 insns with 0 fused under `off`, the target's opcode sequence.
 * NAMES. functions: the map's SDK-style names (GUESS by shape: reset_kpad ... KPADiSamplingCallback; KPADRead is
 *   the map's). Types: `KPADStatus`/`KPADEXStatus`/`Vec2` follow the SDK's public header; `KPADInsideStatus`,
 *   `KPADObj`, `KPADUnifiedWpadStatus` field names are GUESS from use (offsets/sizes from the code: 0x578 stride,
 *   0xB0 copy loop, 0x38 ring stride). Inline helpers reset_wbc, clear_wpad_ring, calc_dpd2pos_scale,
 *   set_obj_interval, request_reset_all are GUESS names for code repeated in KPADInitEx/KPADiConnectCallback.
 *   KP_DEV_TRAIN (16) / KP_DEV_GUITAR (17) are GUESS names for the two extra classic-button device types. Data
 *   statics: `__KPADVersion` is the SDK's; the rest are the map's GUESS SDK-style names (idist_org,
 *   iaccXY_nrm_hori, isec_nrm11, icenter_org, kp_mode_table, kp_fs_fstick_*, kp_cl_stick_*, kp_cl_trigger_*,
 *   kp_rm_acc_max, kp_fs_acc_max, kp_ex_trigger_*, kp_ex_analog_*, kp_stick_clamp_mode, kp_dpd_center_init,
 *   kp_initialized, kp_init_angle, kp_dist_init, kp_acc_mtx, inside_kpads, kp_wbc_*).
 * RESIDUALS.
 *   - unwritten: calc_dpd_variable 0x804BEAE0 (0x5CC), read_kpad_dpd 0x804BF0B0 (0x47C), read_kpad_ext 0x804BF890
 *     (0x8D0), KPADiRead 0x804C0170 (0x72C), KPADiSamplingCallback 0x804C0FF0 (0x768); declared only.
 *   - `.bss` order: MWCC lays `.bss` out in first-reference order (measured with a probe), so `kp_acc_mtx`
 *     (first read in read_kpad_acc) lands before `inside_kpads`; the target has `inside_kpads` at +0. Inference,
 *     not proven: the original referenced `inside_kpads` ahead of reset_kpad in public setters the link
 *     dead-stripped (the `DVD/dvd_broadway.c` precedent); `.sdata2` order (0.0f, 0.5f first) points the same way.
 *     KPADInitEx and KPADiConnectCallback keep the `...bss.0+0x30` vs `inside_kpads` operand difference.
 *   - KPADInitEx: `lis KPADiSamplingCallback@ha` is scheduled after `lis KPADiConnectCallback@ha` (target: before).
 *   - select_2obj_first: the hoisted constants 1.0f/0.0f and `max` take f29..f31 in a different order.
 *   - `mh3_pad.cpp` still declares KPADInit/KPADShutdown locally instead of including `kpad.h`.
 * SHAPES. `.sbss` statics are defined in reverse of their target order (MWCC emits `.sbss` reversed); loops over
 *   `kobj_*` are do-while with the end compare at the bottom; read_kpad_button stages the classic word in a local
 *   before reading the old hold; select_2obj_continue reuses `dist` for the reciprocal length. A chained
 *   `a = b = x` stores right to left (`kp_dist_init = kp_err_dist_min = ...` writes kp_err_dist_min first), so the
 *   relocation order depends on the chain's order; the WBC flags are separate statements for that reason.
 *   KPADiRead and KPADiSamplingCallback are non-static because slice-1 code references them before their
 *   definitions exist, and their map rows are global.
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

/* WPAD device types and data formats the library tests. */
#define KP_DEV_FREESTYLE 1
#define KP_DEV_CLASSIC 2
#define KP_DEV_TRAIN 16
#define KP_DEV_GUITAR 17
#define KP_DEV_NOT_FOUND 253

#define KP_FMT_CORE_ACC 1
#define KP_FMT_CORE_ACC_DPD 2
#define KP_FMT_FS_ACC 4
#define KP_FMT_FS_ACC_DPD 5
#define KP_FMT_CLASSIC_ACC 7
#define KP_FMT_CLASSIC_ACC_DPD 8
#define KP_FMT_CORE_ACC_DPD_FULL 11

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

static void calc_dpd_variable(KPADInsideStatus* kp, s8 valid);
static void read_kpad_dpd(KPADInsideStatus* kp, KPADUnifiedWpadStatus* uwp);
static void read_kpad_ext(KPADInsideStatus* kp, KPADUnifiedWpadStatus* uwp);
s32 KPADiRead(s32 chan, KPADStatus* samplingBufs, u32 length, s32* err, BOOL readEx);
static void KPADiConnectCallback(s32 chan, s32 reason);
void KPADiSamplingCallback(s32 chan);

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
    case KP_FMT_CORE_ACC_DPD_FULL:
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

/* 0x804C0160 (0xC): Reads up to `length` processed samples of channel `chan`, newest first. */
s32 KPADRead(s32 chan, KPADStatus* samplingBufs, u32 length)
{
    return KPADiRead(chan, samplingBufs, length, NULL, FALSE);
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

    if (result == 0 && kp->control_dpd_cb != NULL && !kp->dpd_on_notified) {
        kp->dpd_on_notified = 1;
        kp->control_dpd_cb(chan, 1);
        kp->dpd_off_notified = 0;
    }
    kp->dpd_enabled = WPADIsDpdEnabled(chan);
    kp->dpd_ctrl_busy = 0;
}
