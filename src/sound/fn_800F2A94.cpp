/* sound/fn_800F2A94.cpp - the game's BGM/stream control block, .text 0x800F2A94..0x800F6520.
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with `symedit`: every
 * fn_ name this file uses is a bare .text entry in config/RMHE08/symbols.txt)
 *
 * 45 functions, 14988 bytes.  Registered (whose range was
 * 0x800F2A94..0x800F95A4 / 88 functions / 27408 bytes - see "Seam" for why the registration stops at
 * 0x800F6520).
 *
 * What the unit is: the game's BGM/stream controller.  All of it works on one object, the pointer at
 * `.sdata` `lbl_80791690`, cleared to 0x244 bytes by `fn_800F2A94`.  The object carries a three-channel
 * block (`field_0x00[i]` is the channel's stream status, `field_0x0C[i]` the one before it, `ch_state[i]`
 * at +0x18 the state the whole unit switches on - 2 = live, 4 = finished), the request flags at
 * +0x43..+0x4D, the pending-id word at +0x1D7, the per-map id tables `lbl_8059B268` / `lbl_8059B280` /
 * `lbl_8059B2AC` / `lbl_8059B31C` and the attenuation/delay curves the volume/pitch writers walk
 * (`lbl_805971F8` is the table of curve pointers).  The per-channel API is `srt_ready_ck__Fl`,
 * `GetStreamStatus__FUl` and `PlayStream__FUlUl`; `bgm_stop_all()` is the unit's one named entry.
 *
 * Seam (8.3): the pooled brief pinned 0x800F2A94..0x800F95A4 and discovery warned the cap at --max-bytes
 * made it a guess.  `tudiscover at 0x800F2A94` reports the *certain* TU as 0x800F2A94..0x800F590C (32
 * functions) and a **right boundary it cannot evidence** (`boundaries.right == null`; the strongest weak
 * cut is 0x800F8A44, "codegen fingerprint change").  The range splits, however, on the code itself:
 * everything below 0x800F6520 drives `lbl_80791690`, and everything from 0x800F6520 on drives
 * `eft_control` (`.bss` 0x806A20F0, 0xC44 B) and the `get_move_work_adrs__FUc` work buffers - the effect
 * (eft/efm/efx) resource block.  Rule 2 confirmed it independently: with the whole range registered,
 * 39 `extern` declarations in **twelve `ef` units** (`eft002/004/007/009`, `fn_800FD520`, `fn_800FD718`,
 * `fn_80104BD0`, `fn_8010D1A8`, `fn_80114E34`, `fn_8011722C`, `fn_80119C44`, `fn_803066F0`) resolved to
 * *this* unit as their owner - i.e. the ef block's symbols belong to an `ef` unit, not to a `sound` one.
 * The registration therefore stops at 0x800F6520 and the remainder (0x800F6520..0x800F95A4, 43
 * functions, 12420 bytes, `eft_control` + the eft/efm/efx resource loaders) is left for its own `ef`
 * unit - it needs its own proposal, and `queue.py` marks this one covered because its `.text` start is
 * inside a registered unit.
 *
 * Data: the unit's `.data`/`.sdata2` (`lbl_8059B268`, `lbl_8059B280`, `lbl_8059B2AC`, `lbl_8059B31C`,
 * `lbl_805971F8` and the pool) is reached through `extern` declarations and never defined here
 * (playbook 29, docs/plan.md 8.4); it is requested in the outbox, not claimed in splits.txt.  `extab`
 * 0x8000BA6C..0x8000BB6C and `extabindex` 0x8002568C..0x8002580C *are* the unit's own (32 records each)
 * and are claimed in the splits block.
 *
 * Status (measured symbol by symbol against MAIN's retired per-function / region target objects; the
 * outbox's `measured_with` says exactly how - `recompile.py --measure` cannot run for a proposal unit,
 * because MAIN has neither a target object nor a ninja rule for this range):
 *   * 22 of the 45 symbols are reconstructed and at or above the 80 % bar; 18 of them are byte-identical.
 *     Matched bytes across the unit: 1983 / 14988 (13.23 %).
 *   * `#pragma peephole off` is load-bearing and is this file's first line: `snd_player_mask_set` 86.82 -> 100 and
 *     `snd_player_mask_clear` 87.92 -> 100 are byte-identical only with it, and it is the shared finding of both
 *     neighbouring `sound` units.  It costs `fn_800F51B8` (100 -> 90, still above the bar) and
 *     `fn_800F5A54` (100 -> 96); the scoped-pragma alternative is an outbox `flag` request.
 *   * Below 100 % but above the bar: `fn_800F2E38` 97.47, `fn_800F3218` 96.79, `fn_800F4644` 95.54,
 *     `fn_800F4350` 90.40 - each otherwise instruction-for-instruction equal to retail, differing in
 *     register colouring only.
 *   * 23 symbols are unwritten (0 %).  Biggest first: `snd_quest_frame_begin` (0x728), `bgm_ctrl_frame` (0x67C),
 *     `snd_quest_frame_end` (0x3B8), `fn_800F3C58` (0x340), `fn_800F3604` (0x320), `fn_800F3054` (0x1C4),
 *     `fn_800F2A94` (0x244), `fn_800F2CD8` (0x160), `fn_800F34EC`+`fn_800F3554` (0x118), `snd_quest_scene_set`
 *     (0x138), `fn_800F48F4` (0x174).  The complete per-symbol table is the outbox's `symbols`.
 *   * `fn_800F2A94` (the 0x244-byte block initialiser) is *not* written: its store order interleaves
 *     three parallel 3-entry runs and no source shape tried so far reproduces the sequence.  It is the
 *     largest single body missing here.
 * NAMES. GUESS (from each body and its callers): snd_bgm_hold_ck
 *   GUESS (from each body and its callers): bgm_ctrl_init, bgm_ctrl_frame, bgm_behind_flag_clear
 */
#pragma peephole off
#include "types.h"
#include "nw4r/math.h"
#include "sound/fn_800E46E8.h"

/* ---------------------------------------------------------------------------------------------------
 * The BGM/stream control block: `lbl_80791690` points at one of these, `fn_800F2A94` clears 0x244 bytes.
 * Field meanings are read off the functions below; offsets no reconstructed function reaches keep their
 * offset as a name (rule 5's exception).
 * size: 0x244
 * ------------------------------------------------------------------------------------------------- */
struct BgmCtrl {
    /* +0x000 */ u32 field_0x00[3];
    /* +0x00C */ u32 field_0x0C[3];
    /* +0x018 */ u8 ch_state[3];
    /* +0x01B */ u8 field_0x1B;
    /* +0x01C */ f32 field_0x1C;
    /* +0x020 */ f32 field_0x20;
    /* +0x024 */ f32 field_0x24;
    /* +0x028 */ f32 field_0x28;
    /* +0x02C */ f32 field_0x2C;
    /* +0x030 */ f32 field_0x30;
    /* +0x034 */ u32 field_0x34;
    /* +0x038 */ u32 field_0x38;
    /* +0x03C */ u32 field_0x3C;
    /* +0x040 */ u8 field_0x40[2];
    /* +0x042 */ u8 unused_0x42;
    /* +0x043 */ u8 field_0x43;
    /* +0x044 */ u8 field_0x44;
    /* +0x045 */ u8 field_0x45;
    /* +0x046 */ s16 field_0x46;
    /* +0x048 */ u8 field_0x48;
    /* +0x049 */ u8 field_0x49;
    /* +0x04A */ u8 field_0x4A;
    /* +0x04B */ u8 field_0x4B;
    /* +0x04C */ u8 field_0x4C;
    /* +0x04D */ u8 field_0x4D[2];
    /* +0x04F */ u8 unused_0x4F;
    /* +0x050 */ u8 pad_0x050[4];
    /* +0x054 */ s16 field_0x54;
    /* +0x056 */ u8 pad_0x056[2];
    /* +0x058 */ u8 field_0x58;
    /* +0x059 */ u8 pad_0x059;
    /* +0x05A */ s16 field_0x5A;
    /* +0x05C */ s16 field_0x5C;
    /* +0x05E */ u8 field_0x5E;
    /* +0x05F */ u8 pad_0x05F;
    /* +0x060 */ u8 field_0x60[2];
    /* +0x062 */ u8 field_0x62;
    /* +0x063 */ u8 pad_0x063[3];
    /* +0x066 */ u8 field_0x66;
    /* +0x067 */ u8 pad_0x067[0x10D];
    /* +0x174 */ f32 field_0x174;
    /* +0x178 */ u8 field_0x178;
    /* +0x179 */ u8 field_0x179;
    /* +0x17A */ u8 field_0x17A;
    /* +0x17B */ u8 pad_0x17B[0x40];
    /* +0x1BB */ u8 field_0x1BB;
    /* +0x1BC */ u8 field_0x1BC;
    /* +0x1BD */ u8 pad_0x1BD[7];
    /* +0x1C4 */ u8 field_0x1C4;
    /* +0x1C5 */ u8 field_0x1C5;
    /* +0x1C6 */ u8 field_0x1C6;
    /* +0x1C7 */ u8 field_0x1C7;
    /* +0x1C8 */ u8 field_0x1C8;
    /* +0x1C9 */ u8 field_0x1C9;
    /* +0x1CA */ u8 field_0x1CA;
    /* +0x1CB */ u8 field_0x1CB;
    /* +0x1CC */ u8 field_0x1CC;
    /* +0x1CD */ u8 pad_0x1CD[7];
    /* +0x1D4 */ u8 field_0x1D4;
    /* +0x1D5 */ u8 field_0x1D5;
    /* +0x1D6 */ u8 field_0x1D6;
    /* +0x1D7 */ u8 field_0x1D7;
    /* +0x1D8 */ u8 field_0x1D8;
    /* +0x1D9 */ u8 field_0x1D9;
    /* +0x1DA */ u8 pad_0x1DA[2];
    /* +0x1DC */ u16 field_0x1DC;
    /* +0x1DE */ u8 pad_0x1DE[4];
    /* +0x1E2 */ u8 field_0x1E2;
    /* +0x1E3 */ u8 pad_0x1E3[0xB];
    /* +0x1EE */ u8 field_0x1EE;
    /* +0x1EF */ u8 field_0x1EF;
    /* +0x1F0 */ u8 field_0x1F0;
    /* +0x1F1 */ u8 field_0x1F1;
    /* +0x1F2 */ u8 pad_0x1F2[0x42];
    /* +0x234 */ u16 field_0x234;
    /* +0x236 */ u16 field_0x236;
    /* +0x238 */ u8 field_0x238;
    /* +0x239 */ u8 field_0x239;
    /* +0x23A */ u8 field_0x23A;
    /* +0x23B */ u8 field_0x23B;
    /* +0x23C */ u8 field_0x23C;
    /* +0x23D */ u8 field_0x23D;
    /* +0x23E */ u16 field_0x23E;
    /* +0x240 */ u16 field_0x240;
    /* +0x242 */ u8 pad_0x242[2];
};

/* ---------------------------------------------------------------------------------------------------
 * Declarations for symbols this unit owns (rule 2: the owner's file is their home).
 * ------------------------------------------------------------------------------------------------- */
extern "C" BgmCtrl* lbl_80791690;
extern "C" u8 lbl_8059B31C[];
extern "C" u8 lbl_8059B280[];

extern "C" void fn_800F3054(s32 channel, s32 id);
extern "C" void fn_800F3604(BgmCtrl* work);
extern "C" void fn_800F3924(u8 a, u16 b, u32* out0, u32* out1);
extern "C" void fn_800F4350(void);
extern "C" int fn_800F44F8(u8 channel);

/* Symbols this range calls but does not own: none of them is registered yet, so rule 2's band for them is
 * either this unit's own module (sound) or one of its documented gaps. */
int srt_ready_ck(long channel);
void PlayStream(u32 channel, u32 id);
u32 PlayMode_ck();
s32 GetStreamStatus(u32 channel);
u32 event_demo_ck();
extern "C" u32 demo_play_ck();
extern "C" u32 fn_8028F24C();
u32 get_now_mapno();
u32 get_now_areano();

/* ---------------------------------------------------------------------------------------------------
 * Bodies, in address order.
 * ------------------------------------------------------------------------------------------------- */

/* 0x800F2E38 - 0x13C: "is the requested stream already up on both channels?"  The two bytes of
 * `lbl_8059B31C[id * 2]` say which channels the request wants; a request in the 0x2B/0x2C/0x2E/0x2F
 * group additionally forces channel 2 on when `demo_play_ck` says the demo is running. */
extern "C" int fn_800F2E38(u8 id) {
    u8 ch2 = lbl_8059B31C[id * 2];
    u8 ch0 = lbl_8059B31C[id * 2 + 1];
    u8 up = 0;
    if (demo_play_ck() == 1) {
        if (id == 0x2C || id == 0x2F || id == 0x2B || id == 0x0E) {
            ch2 = 1;
        }
    }
    if (ch2 == 0 && ch0 == 0) {
        return 1;
    }
    if (ch0 != 0) {
        s32 status = GetStreamStatus(0);
        if ((u32)(status - 1) <= 1) {
            PlayStream(0, 2);
        }
        if (status == 3) {
            up = 1;
        }
    } else {
        up = 1;
    }
    if (ch2 != 0) {
        s32 status = GetStreamStatus(2);
        if ((u32)(status - 1) <= 1) {
            PlayStream(2, 2);
        }
        if (status == 3) {
            up++;
        }
    } else {
        up++;
    }
    return up == 2;
}

/* 0x800F2F74 - 0xE0: stop the request's channels once they have run to their end (status 3). */
extern "C" void fn_800F2F74(u8 id) {
    u8 ch2 = lbl_8059B31C[id * 2];
    u8 ch0 = lbl_8059B31C[id * 2 + 1];
    if (demo_play_ck() == 1) {
        if (id == 0x2C || id == 0x2F || id == 0x2B || id == 0x0E) {
            ch2 = 1;
        }
    }
    if (ch2 == 0 && ch0 == 0) {
        return;
    }
    if (ch0 != 0) {
        if (GetStreamStatus(0) == 3) {
            PlayStream(0, 3);
        }
    }
    if (ch2 != 0) {
        if (GetStreamStatus(2) == 3) {
            PlayStream(2, 3);
        }
    }
}

/* 0x800F3218 - 0xD4: the per-frame channel poll.  Each channel keeps its last status in
 * `field_0x00[i]` and the one before it in `field_0x0C[i]`; `ch_state[i]` is the state the rest of the
 * unit switches on (4 = finished, which is re-armed while the stream was playing). */
extern "C" void fn_800F3218() {
    BgmCtrl* work = lbl_80791690;
    if (work == NULL) {
        return;
    }
    if (event_demo_ck() == 1) {
        return;
    }
    for (s32 i = 0; i < 3; i++) {
        work->field_0x0C[i] = work->field_0x00[i];
        if (srt_ready_ck(i) == 1) {
            work->field_0x00[i] = GetStreamStatus(i);
        } else {
            work->field_0x00[i] = 0;
        }
        if (work->ch_state[i] == 4 && work->field_0x00[i] == 0) {
            continue;
        }
        work->ch_state[i] = (u8)work->field_0x00[i];
        if (work->ch_state[i] == 0 && work->field_0x0C[i] == 2) {
            work->ch_state[i] = 4;
        }
    }
}

/* 0x800F4350 - 0xA0: stop and clear the two channel records the id-0x2C path owns. */
extern "C" void fn_800F4350() {
    BgmCtrl* work = lbl_80791690;
    if (work == NULL) {
        return;
    }
    for (s32 i = 0; i < 2; i++) {
        if ((u8)(work->ch_state[i] - 2) <= 1) {
            fn_800F3054(i, 1);
        }
        work->field_0x40[i] = 0;
        work->field_0x4D[i] = 0;
        work->field_0x60[i] = 0;
    }
    if (work->field_0x66 != 0) {
        work->field_0x66 = 1;
    }
}

/* 0x800F4644 - 0x94: once the pending request has drained, hand channel 1 to the game mode's own
 * stream when that mode is not 2. */
extern "C" void fn_800F4644() {
    BgmCtrl* work = lbl_80791690;
    if (work == NULL) {
        return;
    }
    if (work->field_0x48 == 0) {
        return;
    }
    if (work->field_0x44 != 0) {
        return;
    }
    if (work->field_0x45 != 0) {
        return;
    }
    work->field_0x48 = 0;
    if (PlayMode_ck() == 2) {
        return;
    }
    if (fn_800F44F8(1) != 1) {
        return;
    }
    fn_800F3054(1, 1);
    work->field_0x40[1] = 0;
}

/* 0x800F5208 - 0x88: start the channel-2 stream for the current play mode. */
extern "C" void fn_800F5208(u8 arg) {
    if (lbl_80791690 == NULL) {
        return;
    }
    if (srt_ready_ck(2) == 0) {
        return;
    }
    s32 id;
    switch (arg) {
    case 4:
        id = 0x32;
        break;
    case 6:
        id = 0x33;
        break;
    case 7:
        id = 0x34;
        break;
    default:
        return;
    }
    fn_800F3054(2, id);
}

/* 0x800F6334 - 4 B: the named entry, a tail call into the stream stop. */
void bgm_stop_all() {
    resumeSoundEngine();
}

/* 0x800F6514 - 0xC: request id 5 on channel 0. */
extern "C" void fn_800F6514() {
    fn_800F3054(0, 5);
}

/* 0x800F4538 - 0x28: drop the two pending flags and raise 0x44/0x4C. */
extern "C" void snd_quest_start_bgm_set() {
    BgmCtrl* work = lbl_80791690;
    if (work == NULL) {
        return;
    }
    work->field_0x45 = 0;
    work->field_0x48 = 0;
    work->field_0x44 = 1;
    work->field_0x4C = 1;
}

/* 0x800F4A68 - 0x28: is a request pending? */
extern "C" int snd_bgm_hold_ck() {
    BgmCtrl* work = lbl_80791690;
    if (work == NULL) {
        return 0;
    }
    return work->field_0x43 != 0;
}

/* 0x800F46D8 - 0x2C: set bit `id` of the pending word. */
extern "C" void snd_player_mask_set(u8 id) {
    BgmCtrl* work = lbl_80791690;
    if (work == NULL) {
        return;
    }
    work->field_0x1D7 |= (u8)(1 << id);
}

/* 0x800F4704 - 0x30: clear bit `id` of the pending word. */
extern "C" void snd_player_mask_clear(u8 id) {
    BgmCtrl* work = lbl_80791690;
    if (work == NULL) {
        return;
    }
    work->field_0x1D7 &= (u8) ~(1 << id);
}

/* 0x800F486C - 0x50: restart the active request. */
extern "C" void snd_quest_result_bgm_set() {
    BgmCtrl* work = lbl_80791690;
    if (work == NULL) {
        return;
    }
    fn_800F4350();
    work->field_0x48 = 1;
    work->field_0x44 = 1;
    work->field_0x45 = 1;
    work->field_0x46 = -1;
    work->field_0x4C = 1;
}

/* 0x800F48BC - 0x38: play the current map's id on channel 1. */
extern "C" void fn_800F48BC() {
    fn_800F3054(1, lbl_8059B280[get_now_mapno() & 0xFF]);
}

/* 0x800F51B8 - 0x28: stop channel `channel` if it is live. */
extern "C" void fn_800F51B8(s32 channel) {
    BgmCtrl* work = lbl_80791690;
    if (work == NULL) {
        return;
    }
    if (work->ch_state[channel] != 2) {
        return;
    }
    fn_800F3054(channel, 5);
}

/* 0x800F51E0 - 0x28: stop channel 0 if it is live. */
extern "C" void fn_800F51E0() {
    BgmCtrl* work = lbl_80791690;
    if (work == NULL) {
        return;
    }
    if (work->ch_state[0] != 2) {
        return;
    }
    fn_800F3054(0, 1);
}

/* 0x800F590C - 0x18: clear the "behind the scene" flag. */
extern "C" void bgm_behind_flag_clear() {
    BgmCtrl* work = lbl_80791690;
    if (work == NULL) {
        return;
    }
    work->field_0x1BC = 0;
}

/* 0x800F5A54 - 0x50: raise 0x23A and start id 0x36 on channel 1. */
extern "C" void fn_800F5A54() {
    BgmCtrl* work = lbl_80791690;
    if (work == NULL) {
        return;
    }
    work->field_0x23A = 1;
    work->field_0x1CB = 0x36;
    fn_800F3054(1, work->field_0x1CB);
    fn_800F3054(1, 4);
}

/* 0x800F5AA4 - 0x24: drop 0x23A and start id 1 on channel 0. */
extern "C" void fn_800F5AA4() {
    BgmCtrl* work = lbl_80791690;
    if (work == NULL) {
        return;
    }
    work->field_0x23A = 0;
    fn_800F3054(1, 1);
}

/* 0x800F5AC8 - 0x48: start id 0x49 on channel 2. */
extern "C" void fn_800F5AC8() {
    BgmCtrl* work = lbl_80791690;
    if (work == NULL) {
        return;
    }
    work->field_0x1CC = 0x49;
    fn_800F3054(2, work->field_0x1CC);
    fn_800F3054(2, 4);
}

/* 0x800F6338 - 0x7C: restart every live channel except channel 1. */
extern "C" void fn_800F6338() {
    BgmCtrl* work = lbl_80791690;
    if (work == NULL) {
        return;
    }
    for (s32 i = 0; i < 3; i++) {
        if (srt_ready_ck(i) == 0) {
            continue;
        }
        if (work->ch_state[i] != 2) {
            continue;
        }
        if (i == 1) {
            continue;
        }
        fn_800F3054(i, 1);
    }
}

/* 0x800F6420 - 0x54: in the final area, request id 0x15 on channel 1. */
extern "C" void fn_800F6420() {
    BgmCtrl* work = lbl_80791690;
    if (work == NULL) {
        return;
    }
    if ((u8)fn_8028F24C() == 6 && (u8)get_now_areano() != 1) {
        return;
    }
    fn_800F3054(1, 0x15);
}
