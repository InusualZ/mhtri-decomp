/*
 * `Psw` - the per-player pad record array (.bss 0x80659350, 4 x 0x350 B = 0xD40), defined by `src/mh3_pad.cpp`
 * (its `.bss` 0x806585B8-0x806694E8 is that unit's own; its static initialiser `fn_80046B94` constructs the
 * array with `__construct_array` over 0x350-byte elements, as it does the twin `Psw_prev`).  The one home of
 * the record type (rule 1; the object is declared in `Psw.h`); it merges the views the tree
 * carried before: `mh3_pad.cpp`'s own (`mode`, `field_0x..`), `Pl/fn_80273B14.h`'s control words and the
 * lobby band's button words (`unsplit/lobby.h`).
 *
 * Layout evidence: `mh3_pad.cpp`'s readers.  +0x00 is the GameCube-pad block `Psw_prev` fills (the frame copy
 * `memcpy(Psw, Psw_prev, 0x2E)`); +0x38/+0x62/+0x98 are the three `WPADStatus`/`WPADCLStatus`/`WPADFSStatus`
 * buffers `wpadCopyCurrentSample` fills by device type; +0xCA/+0xD6/+0xFC are the remote, Classic Controller
 * and Nunchuk button blocks the frame read derives from them; +0x2C0 is the merged menu block.
 */
#ifndef MHTRI_MH3_PAD_PLAYER_PAD_H
#define MHTRI_MH3_PAD_PLAYER_PAD_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The merged menu button block at +0x2C0 of a pad record (the per-frame words the menus and the arena task
 * read through a pointer to it), filled from the device blocks through the button map tables. size: 0x90 */
typedef struct PadButtons {
    /* +0x00 */ u16 hold_0x00;       /* the raw hold word (0x80 = the start-style confirm bit the arena tests) */
    /* +0x02 */ u16 prev_0x02;       /* last frame's `hold_0x00` */
    /* +0x04 */ u16 pressed_0x04;    /* buttons that went down this frame */
    /* +0x06 */ u16 released_0x06;   /* buttons that went up this frame */
    /* +0x08 */ u16 press_0x08;      /* the stick-direction button word (masked to 0x3C00) */
    /* +0x0A */ u16 stick_prev_0x0A; /* last frame's `press_0x08` */
    /* +0x0C */ u16 trigger_0x0C;    /* the stick-direction buttons that went down this frame */
    /* +0x0E */ u16 stick_released_0x0E;
    /* +0x10 */ u16 clear_0x10;      /* GUESS name: cleared every frame, never written otherwise */
    /* +0x12 */ u16 clear_0x12;      /* GUESS name: cleared every frame, never written otherwise */
    /* +0x14 */ u16 held_0x14;       /* the auto-repeat word, ORed with `pressed_0x04` for cursor steps */
    /* +0x16 */ u16 repeat_count_0x16;
    /* +0x18 */ u16 stick_deg_0x18;  /* the device stick's angle in degrees */
    /* +0x1A */ u16 stick_dist_0x1A; /* the device stick's distance (0..127) */
    /* +0x1C */ u8 stick_dir_0x1C;   /* the device stick's direction bits */
    /* +0x1D */ u8 edge_0x1D;        /* the direction bits of this frame */
    /* +0x1E */ u8 edge_off_0x1E;    /* last frame's `edge_0x1D` */
    /* +0x1F */ u8 edge_on_0x1F;     /* the auto-repeat of `edge_0x1D` */
    /* +0x20 */ u16 edge_count_0x20; /* the auto-repeat counter of `edge_0x1D` */
    /* +0x22 */ u8 device_0x22;      /* 0 none, 1 Nunchuk, 2 Classic Controller, 3 remote alone */
    /* +0x23 */ u8 pad_0x23[0x1];
    /* +0x24 */ u8 wait_table_0x24[5][16];  /* per-set, per-button frames a changed button is held */
    /* +0x74 */ u8 wait_set_0x74;    /* the row of `wait_table_0x24` in use */
    /* +0x75 */ u8 menu_key_ad_0x75; /* `set_menu_key_ad_flag`: 1 maps the direction bits onto the buttons */
    /* +0x76 */ u8 wait_0x76[16];    /* per-button hold-off countdown */
    /* +0x86 */ u8 pad_0x86[0xA];
} PadButtons; /* size: 0x90 */

/* The button/stick block the Classic Controller (+0xD6) and the Nunchuk (+0xFC) records share: the button
 * words, the stick-direction words and two sticks' angle/magnitude/degree/distance/direction. size: 0x26 */
typedef struct PadStickBlock {
    /* +0x00 */ u16 hold;
    /* +0x02 */ u16 prev;
    /* +0x04 */ u16 trig;
    /* +0x06 */ u16 release;
    /* +0x08 */ u16 stick_hold;
    /* +0x0A */ u16 stick_prev;
    /* +0x0C */ u16 stick_trig;
    /* +0x0E */ u16 stick_release;
    /* +0x10 */ u16 angle[2];        /* 0x10000 per turn */
    /* +0x14 */ u16 mag[2];
    /* +0x18 */ u16 deg[2];
    /* +0x1C */ u16 dist[2];
    /* +0x20 */ u8 dir[2];           /* the 8-way direction bits (`stick_dir8_table`) */
    /* +0x22 */ u16 rept;
    /* +0x24 */ u16 rept_count;
} PadStickBlock; /* size: 0x26 */

/* The Nunchuk block at +0xFC: the shared button/stick block, then the two 16-sample accelerometer rings and
 * their write index. size: 0x1C4 */
typedef struct PadFsBlock {
    /* +0x000 */ PadStickBlock btn;
    /* +0x026 */ u8 pad_0x026[0x2];
    /* +0x028 */ f32 fs_acc[16][3];  /* the Nunchuk accelerometer, one sample per frame */
    /* +0x0E8 */ f32 acc[16][3];     /* the remote accelerometer, one sample per frame */
    /* +0x1A8 */ u8 index;           /* the ring slot of this frame (mod 16) */
    /* +0x1A9 */ u8 pad_0x1a9[0x1B];
} PadFsBlock; /* size: 0x1C4 */

typedef struct PlayerPad {
    /* +0x000 */ u16 gc_hold_0x00;          /* the GameCube pad's mapped buttons */
    /* +0x002 */ u16 gc_prev_0x02;
    /* +0x004 */ u16 gc_trig_0x04;
    /* +0x006 */ u16 gc_release_0x06;
    /* +0x008 */ u16 gc_stick_hold_0x08;    /* the GameCube sticks' direction buttons */
    /* +0x00A */ u16 gc_stick_prev_0x0A;
    /* +0x00C */ u16 gc_stick_trig_0x0C;
    /* +0x00E */ u16 gc_stick_release_0x0E;
    /* +0x010 */ u16 gc_stick_angle_0x10[2];
    /* +0x014 */ u16 gc_stick_mag_0x14[2];
    /* +0x018 */ u16 gc_rept_0x18;
    /* +0x01A */ u16 gc_rept_count_0x1A;
    /* +0x01C */ s16 gc_err_0x1C;           /* the pad's PAD error this frame */
    /* +0x01E */ s16 gc_err_prev_0x1E;
    /* +0x020 */ u8 pad_0x020[0x2];
    /* +0x022 */ u16 gc_stick_deg_0x22[2];
    /* +0x026 */ u16 gc_stick_dist_0x26[2];
    /* +0x02A */ u8 gc_stick_dir_0x2A[2];
    /* +0x02C */ u8 gc_trigger_l_0x2C;
    /* +0x02D */ u8 gc_trigger_r_0x2D;
    /* +0x02E */ u8 pad_0x02e[0x2];
    /* +0x030 */ u32 field_0x30;            /* the `WPADProbe` result (0 connected, -1 none, -2 busy) */
    /* +0x034 */ u32 mode;                  /* the `WPADProbe` device type (0 remote, 1 Nunchuk, 2 Classic) */
    /* +0x038 */ u8 core_status_0x38[0x2A]; /* the remote's `WPADStatus` */
    /* +0x062 */ u8 cl_status_0x62[0x36];   /* the `WPADCLStatus` */
    /* +0x098 */ u8 fs_status_0x98[0x32];   /* the `WPADFSStatus` */
    /* +0x0CA */ u16 field_0xca;     /* the remote's mapped hold word */
    /* +0x0CC */ u16 core_prev_0xCC;
    /* +0x0CE */ u16 field_0xce;     /* the remote's buttons that went down */
    /* +0x0D0 */ u16 core_release_0xD0;
    /* +0x0D2 */ u16 field_0xd2;     /* the remote's auto-repeat word */
    /* +0x0D4 */ u16 core_rept_count_0xD4;
    /* +0x0D6 */ u16 control_0x0D6;  /* the Classic Controller block (`PadStickBlock`): its hold word */
    /* +0x0D8 */ u16 cl_prev_0xD8;
    /* +0x0DA */ u16 field_0xda;
    /* +0x0DC */ u16 cc_word2_0x0DC;  /* the Classic Controller's released word (the software keyboard reads it) */
    /* +0x0DE */ u16 control_0x0DE;
    /* +0x0E0 */ u16 cl_stick_prev_0xE0;
    /* +0x0E2 */ u16 cl_stick_trig_0xE2;
    /* +0x0E4 */ u16 cl_stick_release_0xE4;
    /* +0x0E6 */ u16 cl_angle_0xE6;
    /* +0x0E8 */ u16 field_0xe8;     /* the right stick's angle the camera keeps (`camera/camera_main.cpp`) */
    /* +0x0EA */ u16 cl_mag_0xEA;
    /* +0x0EC */ u16 field_0xec;     /* the right stick's magnitude the camera keeps */
    /* +0x0EE */ u16 cl_deg_0xEE[2];
    /* +0x0F2 */ u16 cl_dist_0xF2[2];
    /* +0x0F6 */ u8 cl_dir_0xF6[2];
    /* +0x0F8 */ u16 field_0xf8;
    /* +0x0FA */ u16 cl_rept_count_0xFA;
    /* +0x0FC */ u16 control_0x0FC;  /* the Nunchuk block (`PadFsBlock`): its held-button word */
    /* +0x0FE */ u16 fs_prev_0xFE;
    /* +0x100 */ u16 field_0x100;
    /* +0x102 */ u16 wpad_word2_0x102; /* the Nunchuk block's released word (the software keyboard reads it) */
    /* +0x104 */ u16 control_0x104;  /* the Nunchuk stick's direction buttons */
    /* +0x106 */ u16 fs_stick_prev_0x106;
    /* +0x108 */ u16 fs_stick_trig_0x108;
    /* +0x10A */ u16 fs_stick_release_0x10A;
    /* +0x10C */ u16 fs_angle_0x10C[2];
    /* +0x110 */ u16 fs_mag_0x110[2];
    /* +0x114 */ u16 fs_deg_0x114[2];
    /* +0x118 */ u16 fs_dist_0x118[2];
    /* +0x11C */ u8 fs_dir_0x11C[2];
    /* +0x11E */ u16 field_0x11e;
    /* +0x120 */ u16 fs_rept_count_0x120;
    /* +0x122 */ u8 pad_0x122[0x2];
    /* +0x124 */ f32 fs_acc_hist_0x124[16][3];
    /* +0x1E4 */ f32 acc_hist_0x1E4[16][3];
    /* +0x2A4 */ u8 acc_hist_index_0x2A4;
    /* +0x2A5 */ u8 pad_0x2a5[0x1B];
    /* +0x2C0 */ PadButtons button_0x2C0;
} PlayerPad; /* size: 0x350 */


#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MH3_PAD_PLAYER_PAD_H */
