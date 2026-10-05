/*
 * `system_w` - the game/system state block (.bss 0x806585E0, 0xA5C B), defined by `src/mh3_pad.cpp`
 * (its `.bss` 0x806585B8-0x806694E8 is that unit's own, `splits.txt`).  This is the one home of its type
 * (rule 1); the object is declared in `system_w.h`, which `include/unsplit/unknown.h` includes for its consumers.
 *
 * Union of the three private copies the type used to exist as:
 *   - `main.cpp`: the largest view, naming the bytes it polls at +0x01, +0x08 and +0x863..+0x931;
 *   - `fn_80040598.cpp`: the keyboard entry points at +0x8E8..+0x908;
 *   - `enemy/fn_8014A1BC.c`: the +0x0C word.
 * The copies disagree on the extent, not on any field's offset; the object is 0xA5C and `main.cpp` zeroes
 * all of it, so the trailing bytes are padding.
 */
#ifndef MHTRI_MH3_PAD_SYSTEM_WORK_H
#define MHTRI_MH3_PAD_SYSTEM_WORK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* One 0x38-byte slot of `system_w`'s task table at +0xBC (fn_800D0568/FDB0 index it by
 * `index * 0x38`); only the +0x00 live flag and the +0x04 body are touched by this unit. */
typedef struct SysTask {
    /* +0x00 */ u8 active;
    /* +0x01 */ u8 pad_0x01[0x3];
    /* +0x04 */ u8 body[0x20];
    /* +0x24 */ u32 field_0x24;
    /* +0x28 */ u8 pad_0x28[0x10];
} SysTask; /* size: 0x38 */

/* The four-byte record at +0x7BC: fn_800CFBE0 reads it whole, fn_800CFBA0 sets its top byte. */
typedef union SysWord {
    u32 value;
    u8 bytes[4];
} SysWord; /* size: 0x04 */

/* `system_w`'s move-work record: seven u16 maxima followed by seven pointers (work_mem_alloc(0x2C)
 * builds it; the get_move_work and set_move_work_max accessors index it). */
typedef struct MoveWork {
    /* +0x00 */ u16 max[7];
    /* +0x0E */ u8 pad_0x0e[0x2];
    /* +0x10 */ void* addr[7];
} MoveWork; /* size: 0x2C */

/* size: 0xA5C */
typedef struct SystemWork {
    /* +0x000 */ u8 pad_0x00[0x1];
    /* +0x001 */ u8 unk1;
    /* +0x002 */ u8 pad_0x02[0x3];
    /* +0x005 */ u8 field_0x05;
    /* +0x006 */ u8 pad_0x06[0x2];
    /* +0x008 */ u8 unk8;
    /* +0x009 */ u8 field_0x09; /* message-language index selecting the 0x8060B1A8/B200/B2F0 tables */
    /* +0x00A */ u8 pad_0x0A[0x2];
    /* +0x00C */ u32 field_0x0c;
    /* +0x010 */ u8 pad_0x10[0x8];
    /* +0x018 */ u16 field_0x18[4];   /* ran_suu's u16 ring, cells +0x18/+0x1A/+0x1C/+0x1E */
    /* +0x020 */ u8 field_0x20;
    /* +0x021 */ u8 field_0x21;
    /* +0x022 */ u8 field_0x22;      /* fn_800CFD20: < 2 selects the C64 path */
    /* +0x023 */ u8 pad_0x23[0x1];
    /* +0x024 */ u8 game_mode;        /* GameMode_set/GameMode_ck (values < 4) */
    /* +0x025 */ u8 play_mode;        /* PlayMode_set/PlayMode_ck (values < 7) */
    /* +0x026 */ u8 field_0x26;
    /* +0x027 */ u8 field_0x27;
    /* +0x028 */ u8 field_0x28;
    /* +0x029 */ u8 field_0x29;
    /* +0x02A */ u8 field_0x2a;     /* non-zero is the system-wide "hold the player" gate `fn_8027E1E4` tests */
    /* +0x02B */ u8 pad_0x2b[0x1];
    /* +0x02C */ u8 field_0x2c;      /* fn_800D2108: 1 or 2, the hbm state */
    /* +0x02D */ u8 field_0x2d;
    /* +0x02E */ u8 pad_0x2e[0x2];
    /* +0x030 */ u8 field_0x30;
    /* +0x031 */ u8 field_0x31;
    /* +0x032 */ u8 field_0x32;
    /* +0x033 */ u8 field_0x33[4]; /* per-channel WPAD motor state */
    /* +0x037 */ u8 field_0x37[4]; /* per-channel WPAD motor timer */
    /* +0x03B */ u8 pad_0x3b[0x11];
    /* +0x04C */ u32 field_0x4c;
    /* +0x050 */ u8 pad_0x50[0x10];
    /* +0x060 */ u32 field_0x60;      /* MEMCreateExpHeapEx base */
    /* +0x064 */ u32 field_0x64;      /* MEMCreateExpHeapEx size */
    /* +0x068 */ u32 field_0x68;
    /* +0x06C */ u32 field_0x6c;
    /* +0x070 */ u32 field_0x70;
    /* +0x074 */ u32 field_0x74;
    /* +0x078 */ u8 pad_0x78[0x18];
    /* +0x090 */ void* field_0x90;    /* the stream handle fn_800CF61C reads */
    /* +0x094 */ u8 mem_allocator[0x10]; /* MEMFreeToAllocator's allocator record */
    /* +0x0A4 */ MoveWork* field_0xa4; /* the move-work record work_mem_alloc builds */
    /* +0x0A8 */ u8 pad_0xa8[0x14];
    /* +0x0BC */ SysTask tasks[0x20];  /* fn_800D0568/FDB0's 0x38-stride table, ends at +0x7BC */
    /* +0x7BC */ SysWord field_0x7bc;
    /* +0x7C0 */ u8 pad_0x7c0[0xC];
    /* +0x7CC */ u8 field_0x7cc;
    /* +0x7CD */ u8 field_0x7cd;
    /* +0x7CE */ u8 field_0x7ce;
    /* +0x7CF */ u8 field_0x7cf;    /* 0x15 selects the second string of each quest entry message pair */
    /* +0x7D0 */ u8 pad_0x7d0[0x1];
    /* +0x7D1 */ u8 field_0x7d1;    /* set to 1 with +0x7D2 by the lobby pre-quest flow */
    /* +0x7D2 */ u8 field_0x7d2;    /* == 1 is the quest-work busy gate (`quest_work_busy_ck`) */
    /* +0x7D3 */ u8 field_0x7d3;    /* game_ready_ck: == 1 */
    /* +0x7D4 */ u8 net_active_0x7d4;   /* GUESS name: the network control's start-up and Pat reset both clear it */
    /* +0x7D5 */ u8 field_0x7d5;
    /* +0x7D6 */ u8 leave_flag_0x7d6;   /* GUESS name: `arena_task` sets it to 1 with +0x7D5 when the arena hands back to the game mode */
    /* +0x7D7 */ u8 pad_0x7d7[0x1];
    /* +0x7D8 */ u32 leave_state_0x7d8;   /* GUESS name: cleared by `arena_task` in the same step as +0x7D6 */
    /* +0x7DC */ u8 field_0x7dc[4]; /* per-channel motor-on state */
    /* +0x7E0 */ u8 field_0x7e0[4];
    /* +0x7E4 */ u8 field_0x7e4[4];
    /* +0x7E8 */ u8 pad_0x7e8[0x68];
    /* +0x850 */ u8 vs_player_pending_0x850[4];   /* GUESS name: per-player flag `arena_result_next` raises through the player's Vs block */
    /* +0x854 */ u8 pad_0x854[0xF];
    /* +0x863 */ u8 unk2147;
    /* +0x864 */ u8 unk2148;
    /* +0x865 */ u8 field_0x865;
    /* +0x866 */ u8 field_0x866;
    /* +0x867 */ u8 field_0x867;      /* TPLtexLoad group: loading display flag */
    /* +0x868 */ u8 field_0x868;
    /* +0x869 */ u8 pad_0x869[0x1];
    /* +0x86A */ u8 field_0x86A;
    /* +0x86B */ u8 field_0x86b;      /* hbm_enable: == 1 keeps the menu suppressed */
    /* +0x86C */ u8 pad_0x86c[0x2];
    /* +0x86E */ u8 unk2158;
    /* +0x86F */ u8 unk2159;
    /* +0x870 */ u8 hbm_disabled;     /* hbm_disable sets 1; hbm_enable follows +0x86B */
    /* +0x871 */ u8 unk2161;
    /* +0x872 */ u8 pad_0x872[0x5];
    /* +0x877 */ u8 field_0x877;
    /* +0x878 */ u32 field_0x878;
    /* +0x87C */ u32 field_0x87c;
    /* +0x880 */ u8 pad_0x880[0x4];
    /* +0x884 */ u8 field_0x884;
    /* +0x885 */ u8 pad_0x885[0x1];
    /* +0x886 */ u8 field_0x886;
    /* +0x887 */ u8 pad_0x887[0x9];
    /* +0x890 */ u32 field_0x890;
    /* +0x894 */ u32 field_0x894;
    /* +0x898 */ u8 pad_0x898[0x4];
    /* +0x89C */ u8 vs_player_done_0x89c[4];   /* GUESS name: per-player flag `arena_result_next` tests before raising +0x850 */
    /* +0x8A0 */ u8 pad_0x8a0[0x3];
    /* +0x8A3 */ char player_name_0x8a3[0xB];   /* the player name the network work record copies (`strcpy`) */
    /* +0x8AE */ u8 online_0x8ae;       /* GUESS name: 0 = offline play (`arena_result_next` then arms the solo flag), 1 = online */
    /* +0x8AF */ u8 field_0x8af;   /* non-zero selects the second column of `multi_arena_clr_time`
                                    * in `quest/quest_entry.cpp`'s arena time formatter */
    /* +0x8B0 */ u8 vs_mode_0x8b0;  /* non-zero in VS/arena mode: `get_cfg`/`ck_cfg` then read the VS
                                    * user work's profile instead of `option_w` (0x803BE30C band) */
    /* +0x8B1 */ u8 field_0x8b1;
    /* +0x8B2 */ u8 pad_0x8b2[0xF];
    /* +0x8C1 */ u8 net_result_wait_0x8c1;   /* GUESS name: `arena_task` step 4 skips its network-result wait unless this is 1 */
    /* +0x8C2 */ u8 pad_0x8c2[0x12];
    /* +0x8D4 */ void (*field_0x8d4)(void);
    /* +0x8D8 */ u32 (*unk2264)(void);
    /* +0x8DC */ void (*field_0x8dc)(void);
    /* +0x8E0 */ void (*unk2272)(void);
    /* +0x8E4 */ void (*unk2276)(void);
    /* +0x8E8 */ void (*kbd_init)(u8);
    /* +0x8EC */ int (*kbd_open)(u8);
    /* +0x8F0 */ int (*kbd_move)(void);
    /* +0x8F4 */ void (*set_kbd_param)(char*, u32);
    /* +0x8F8 */ u8 (*get_kbd_setup_type)(void);
    /* +0x8FC */ void (*kbd_reset)(void);
    /* +0x900 */ int (*kbd_close)(void);
    /* +0x904 */ void (*kbd_exit)(void);
    /* +0x908 */ int (*kbd_input)(void);
    /* +0x90C */ u8 pad_0x90C[0x3];
    /* +0x90F */ u8 net_session_0x90f;   /* GUESS name: non-zero while the arena runs as a network session */
    /* +0x910 */ u8 pad_0x910[0x21];
    /* +0x931 */ u8 unk2353;
    /* +0x932 */ u8 pad_0x932[0x16];
    /* +0x948 */ void* field_0x948;    /* work-heap base, cleared by fn_800CE5B4 */
    /* +0x94C */ u8 pad_0x94c[0x104];
    /* +0xA50 */ u8 transfer_mode_0xa50;    /* the network transfer mode (the network control's start-up sets 1) */
    /* +0xA51 */ u8 transfer_flag_0xa51;    /* cleared with it (0) */
    /* +0xA52 */ u8 transfer_level_0xa52;   /* the transfer level handed to the mediator (start-up sets 4) */
    /* +0xA53 */ u8 pad_0xa53[0x1];
    /* +0xA54 */ void (*field_0xa54)(s32, s32);
    /* +0xA58 */ u8 field_0xa58;      /* pmic_disp_off clears */
    /* +0xA59 */ u8 pad_0xa59[0x3];
} SystemWork;


#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MH3_PAD_SYSTEM_WORK_H */
