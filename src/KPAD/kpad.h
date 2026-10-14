/*
 * KPAD/kpad.h - the SDK KPAD library's types and entry points (`KPAD/kpad.c`): the public read record `KPADStatus`,
 *   the per-channel work record `KPADInsideStatus` and the ring entry `KPADUnifiedWpadStatus`.
 *   Sizes come from the copy loops and strides of `KPADiRead`/`KPADInitEx` (0xB0 / 0x578 / 0x38); field names follow
 *   the SDK where its public header names them, the rest are GUESS names listed on the NAMES line of `kpad.c`.
 */
#ifndef KPAD_KPAD_H
#define KPAD_KPAD_H

#include "types.h"
#include "MTX/mtx.h"
#include "WPAD/wpad.h"

#ifdef __cplusplus
extern "C" {
#endif

/* size: 0x08 - a 2D vector. */
typedef struct Vec2 {
    /* +0x00 */ f32 x;
    /* +0x04 */ f32 y;
} Vec2; /* size: 0x08 */

/* size: 0x50 - the attached extension's processed state; which member is live follows `KPADStatus.dev_type`. */
typedef union KPADEXStatus {
    struct {
        /* +0x00 */ Vec2 stick;
        /* +0x08 */ Vec acc;
        /* +0x14 */ f32 acc_value;
        /* +0x18 */ f32 acc_speed;
    } fs; /* size: 0x1C */
    struct {
        /* +0x00 */ u32 hold;
        /* +0x04 */ u32 trig;
        /* +0x08 */ u32 release;
        /* +0x0C */ Vec2 lstick;
        /* +0x14 */ Vec2 rstick;
        /* +0x1C */ f32 ltrigger;
        /* +0x20 */ f32 rtrigger;
    } cl; /* size: 0x24 */
    struct {
        /* +0x00 */ f64 tgc_weight;
        /* +0x08 */ f64 weight[4];
        /* +0x28 */ f64 weight_ave[4];
        /* +0x48 */ s32 weight_err;
        /* +0x4C */ s32 tgc_weight_err;
    } bl; /* size: 0x50 */
} KPADEXStatus; /* size: 0x50 */

/* size: 0xB0 - one processed sample as `KPADRead` hands it out. */
typedef struct KPADStatus {
    /* +0x00 */ u32 hold;
    /* +0x04 */ u32 trig;
    /* +0x08 */ u32 release;
    /* +0x0C */ Vec acc;
    /* +0x18 */ f32 acc_value;
    /* +0x1C */ f32 acc_speed;
    /* +0x20 */ Vec2 pos;
    /* +0x28 */ Vec2 vec;
    /* +0x30 */ f32 speed;
    /* +0x34 */ Vec2 horizon;
    /* +0x3C */ Vec2 hori_vec;
    /* +0x44 */ f32 hori_speed;
    /* +0x48 */ f32 dist;
    /* +0x4C */ f32 dist_vec;
    /* +0x50 */ f32 dist_speed;
    /* +0x54 */ Vec2 acc_vertical;
    /* +0x5C */ u8 dev_type;
    /* +0x5D */ s8 wpad_err;
    /* +0x5E */ s8 dpd_valid_fg;
    /* +0x5F */ u8 data_format;
    /* +0x60 */ KPADEXStatus ex_status;
} KPADStatus; /* size: 0xB0 */

/* size: 0x2E - the core record followed by the train controller's buttons, brake and throttle levers. */
typedef struct KPADTrainStatus {
    /* +0x00 */ WPADStatus core;
    /* +0x2A */ u16 trButton;
    /* +0x2C */ u8 brake;
    /* +0x2D */ u8 mascon;
} KPADTrainStatus; /* size: 0x2E */

/* Maps a stick reading to `stick` with a dead zone of `min` and full scale at `max`. */
typedef void (*KPADClampStick)(Vec2* stick, s32 sx, s32 sy, s32 min, s32 max);

/* size: 0x38 - one raw WPAD sample in the per-channel ring, with the data format it was read in. */
typedef struct KPADUnifiedWpadStatus {
    /* +0x00 */ union {
        WPADStatus core;
        WPADFSStatus fs;
        WPADCLStatus cl;
        WPADBLStatus bl;
        KPADTrainStatus tr;
    } u; /* size: 0x36 */
    /* +0x36 */ u8 fmt;
    /* +0x37 */ u8 pad_0x37;
} KPADUnifiedWpadStatus; /* size: 0x38 */

/* size: 0x0C - one pointer object: its position in pointer coordinates and its state flags. */
typedef struct KPADObj {
    /* +0x00 */ Vec2 center;
    /* +0x08 */ s8 error_fg;
    /* +0x09 */ s8 state_fg;
    /* +0x0A */ u8 pad_0x0A[2];
} KPADObj; /* size: 0x0C */

/* Reports a pointer state change of channel `chan` (`reason` 1 = enabled, 0 = disabled). */
typedef void (*KPADControlDpdCallback)(s32 chan, s32 reason);

/* size: 0x578 - the per-channel work record of the library. */
typedef struct KPADInsideStatus {
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
    /* +0x13A */ u8 wpad_ring_idx;
    /* +0x13B */ u8 wpad_ring_lvl;
    /* +0x13C */ KPADUnifiedWpadStatus wpad_ring_bf[16];
    /* +0x4BC */ KPADUnifiedWpadStatus* uni_ring_bf;
    /* +0x4C0 */ u32 uni_ring_len;
    /* +0x4C4 */ f32 sec_dist;
    /* +0x4C8 */ Vec2 sec_nrm;
    /* +0x4D0 */ f32 dist_last;
    /* +0x4D4 */ f32 trust_sec_dist;
    /* +0x4D8 */ Vec hard_acc;
    /* +0x4E4 */ Vec2 dpd_horizon;
    /* +0x4EC */ Vec2 acc_horizon;
    /* +0x4F4 */ Vec2 ah_circle_pos;
    /* +0x4FC */ u16 ah_circle_ct;
    /* +0x4FE */ u8 trust_sec_ct;
    /* +0x4FF */ u8 read_busy;
    /* +0x500 */ u16 btn_repeat_time;
    /* +0x502 */ u16 btn_repeat_next;
    /* +0x504 */ u16 btn_repeat_delay;
    /* +0x506 */ u16 btn_repeat_pulse;
    /* +0x508 */ u16 btn_cl_repeat_time;
    /* +0x50A */ u16 btn_cl_repeat_next;
    /* +0x50C */ KPADControlDpdCallback control_dpd_cb;
    /* +0x510 */ Vec acc_scale;
    /* +0x51C */ Vec fs_acc_scale;
    /* +0x528 */ Vec2 frame_min;
    /* +0x530 */ Vec2 frame_max;
    /* +0x538 */ f32 dist_speed_inc;
    /* +0x53C */ f32 dist_speed_dec;
    /* +0x540 */ f32 ah_circle_radius2;
    /* +0x544 */ f32 dist_scale;
    /* +0x548 */ f32 dist_min;
    /* +0x54C */ WPADSamplingCallback sampling_cb;
    /* +0x550 */ WPADCallback connect_cb;
    /* +0x554 */ u8 reset_req;
    /* +0x555 */ u8 dev_changed;
    /* +0x556 */ u8 dpd_request;
    /* +0x557 */ u8 dpd_enabled;
    /* +0x558 */ u8 dpd_ctrl_busy;
    /* +0x559 */ u8 dpd_command;
    /* +0x55A */ u8 dpd_off_notified;
    /* +0x55B */ u8 dpd_on_notified;
    /* +0x55C */ u8 center_update;
    /* +0x55D */ u8 use_sensor_bar;
    /* +0x55E */ u8 fs_acc_rotate;
    /* +0x55F */ u8 btn_proc_mode;
    /* +0x560 */ s32 pos_play_mode;
    /* +0x564 */ s32 hori_play_mode;
    /* +0x568 */ s32 dist_play_mode;
    /* +0x56C */ s32 acc_play_mode;
    /* +0x570 */ u32 dev_type_last;
    /* +0x574 */ u8 acc_scale_update;
    /* +0x575 */ u8 pad_0x575[3];
} KPADInsideStatus; /* size: 0x578 */

/* The library's entry points. */
void KPADInit(void);
void KPADInitEx(KPADUnifiedWpadStatus* uniRingBufs, u32 length);
void KPADShutdown(void);
void KPADSetBtnRepeat(s32 chan, f32 delay_sec, f32 pulse_sec);
s32 KPADRead(s32 chan, KPADStatus* samplingBufs, u32 length);

#ifdef __cplusplus
}
#endif

#endif
