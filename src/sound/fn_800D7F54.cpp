/* sound/fn_800D7F54.cpp - the SE (`se_w`) request cluster, .text 0x800D7F54..0x800DD40C.
 *
 * Phase 4 fold: the old `sound/fn_800D7F54.cpp` (0x800D7F54..0x800DCFEC), `sound/fn_800DCFEC.cpp` (the map-dependent SE request, one function)
 * and the head of the old `sound/fn_800DD1F0.cpp` (0x800DD1F0..0x800DD40C: `fn_800DD38C`, `st_ice_se_req`, `st_ice_break_se_req`,
 * `fn_800DD3B8`) are one TU in the reconciled candidate.  The folded functions sit at the end of the file under
 * `#pragma optimization_level reset`: the old units that held them were measured with the project's default level and `peephole off`
 * only (the `optimization_level 4` pragma below is the old `fn_800D7F54.cpp`'s, which the cflags of the three absorbed rows share).
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with symedit: every fn_ name this file uses is a bare .text entry in config/RMHE08/symbols.txt)
 *
 * 141 functions. This is the game's sound-effect request layer: a request names a sound id plus a world
 * position, lands in a free slot of the `_se_w` work object, and the per-frame drivers (`fn_800D84E8` /
 * `fn_800D85B0` / `fn_800D8678` / `fn_800D8730`) walk the slots and turn each live one into playback
 * calls (`fn_800F0C14` bank lookup, `fn_800F2890`/`fn_800F27C4` id resolve, `fn_800E80DC`/`fn_800E81BC`
 * open, `fn_800E8354` volume, `fn_800E83CC` pitch, `fn_800E802C`/`fn_800E8228` stop).
 *
 * The core shapes, all read off the disassembly:
 *   - `_se_w` is the work object; `fn_800D843C()` is the "SE disabled" predicate (1 = skip), and
 *     `fn_800D8E58(_se_w*, s32)` returns the first free slot (byte +0 is the in-use flag, 32 slots of
 *     0x50 bytes at +0x3C).
 *   - `fn_800DA72C(s32 kind, s32 id, nw4r::math::VEC3* pos)` is the common "start a positional sound"
 *     helper. The dozens of 16-byte `fn_*` wrappers are `fn_800DA72C(0, id, pos)` with a fixed id; the
 *     72-byte ones pick the id from `fn_800D8D8C(pos)` first.
 *   - `fn_800DBB78(s32 bank, s32 id)` is the non-positional (`sysSE_req`/`titleSE_req`) path.
 *
 * Build: `cflags_main` plus a file-scope `#pragma peephole off` and a file-scope
 * `#pragma optimization_level 4`. The retail object keeps `clrlwi`+`cmpwi`
 * unfused and uses non-record forms in `sysSE_req`, `sysSE_stop`, `fn_800DB4EC`, `fn_800DC53C` and
 * `fn_800DAADC`; `-opt nopeephole` reproduces all of them and changes none of the other functions, so the
 * pragma is a stand-in for the per-unit cflags group the outbox requests (playbook 33). The level-4
 * pragma is measured too: it moves `fn_800D9B6C` 96.667 -> 99.483 (retail's contiguous-case dispatch is
 * the fused `addi r,r,-9; cmplwi r,1` form) and changes no other symbol in the unit; the whole-unit
 * `-O4,p` that would also imply `-func_align 16` is disproven (it drops `fn_800D7F54` to 56 %).
 *
 * Residuals (measured against the target object, symbol by symbol; everything lands
 * `Object(NonMatching, ...)`, so none of it reaches the link). The per-symbol table and the full
 * residual write-up are in the outbox; the short version:
 *   - every symbol below 100 % is instruction-for-instruction equal to retail - the differences are
 *     register colouring, one range-test encoding, or a pool name, never a missing operation.
 *   - `fn_800DB044` (73.3 %): the two-voice crossfade. Every operation is present, but the callee-saved
 *     colouring puts `work` in r31 where retail has r30 (~10 declaration orders and source shapes tried).
 *   - `fn_800D8404` (81.9 %): the switch's 3/4 range test is two compares where retail uses
 *     `addi r,r,-3; cmplwi r,1`.
 *   - `fn_800D87B8` (89.6 %): retail reaches the stop body by a direct jump from case 4; the `goto`-free
 *     shape (a `stop` flag with the body written twice) costs 15 instructions.
 *   - The int->float magic in `fn_800DABF0`/`snd_item_fail_play`/`fn_800DB044` is the compiler's own `.sdata2`
 *     pool entry: the target names it `lbl_80796400`, ours emits an anonymous `@NN` (playbook 29).
 *
 * Data runs in this range are recorded in `splits.txt` as comments and not claimed (playbook 23,
 * docs/plan.md 8.4); the `extab`/`extabindex` fragments travel with the code unit.
 */
#pragma optimization_level 4
#pragma peephole off
#include "sound/fn_800E80DC.h" /* fn_800E80DC (rule 2: the owner's header) */
#include "enemy/em_after_frame_check__FP11_ENEMY_WORKUsff.h" /* em_after_frame_check__FP11_ENEMY_WORKUsff (rule 2: the owner's header) */
#include "sound/fn_800E802C.h" /* fn_800E802C (rule 2: the owner's header) */
#include "sound/fn_800E8228.h" /* fn_800E8228 (rule 2: the owner's header) */
#include "sound/fn_800E8354.h" /* fn_800E8354 (rule 2: the owner's header) */
#include "sound/fn_800EF56C.h" /* fn_800EF56C (rule 2: the owner's header) */
#include "sound/fn_800E8498.h" /* fn_800E8498 (rule 2: the owner's header) */
#include "enemy/em_area_ck__FP11_ENEMY_WORK.h" /* em_area_ck__FP11_ENEMY_WORK (rule 2: the owner's header) */
#include "sound/fn_800E8294.h" /* fn_800E8294 (rule 2: the owner's header) */
#include "sound/fn_800E81BC.h" /* fn_800E81BC (rule 2: the owner's header) */
#include "sound/fn_800E8444.h" /* fn_800E8444 (rule 2: the owner's header) */
#include "sound/fn_800E83CC.h" /* fn_800E83CC (rule 2: the owner's header) */
#include "sound/fn_800E8080.h" /* fn_800E8080 (rule 2: the owner's header) */
#include "sound/fn_800E8150.h" /* fn_800E8150 (rule 2: the owner's header) */
#include "types.h"
#include "nw4r/math.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "draw_shape/fn_800532DC.h" /* fn_800532DC, owned by draw_shape.cpp's range (rule 2) */
#include "sound/se_req.h" /* fn_800DFDCC / fn_800E0428, owned by se_req.cpp's range (rule 2) */
#include "Pl/plw.h"
#include "pl.h"
#include "Pl/fn_802693C4.h"
#include "lobby/lobby_w.h"
#include "lobby/lb_npc_func.h"
/* signatures the calls below use, when they differ from the owner header's: a cast call is the same direct call. */
#define fn_800E80DC_c1 ((s32 (*)(s32, s32, s32))fn_800E80DC)
#define fn_800E802C_c1 ((s32 (*)(s32))fn_800E802C)
#define fn_800E8228_c1 ((void (*)(s32))fn_800E8228)
#define fn_800E8354_c1 ((void (*)(s32, s16))fn_800E8354)
#define fn_800E8498_c1 ((void (*)(Mtx34*, s32))fn_800E8498)
#define fn_800E81BC_c1 ((s32 (*)(s32, s32, nw4r::math::VEC3*, s32))fn_800E81BC)
#define fn_800E8444_c1 ((s32 (*)(void))fn_800E8444)
#define fn_800E83CC_c1 ((void (*)(s32, s16))fn_800E83CC)
#define fn_800E8080_c1 ((void (*)(s32, s32))fn_800E8080)
#define fn_800E8150_c1 ((s32 (*)(s32, s32, nw4r::math::VEC3*, s32))fn_800E8150)
/* One of the 32 sound-slot records the SE work object carries at +0x3C. `fn_800D8E58` walks them
 * looking for a clear `in_use` byte and `fn_800DA72C` fills one in; the offsets below are read off that
 * pair and off `se_req_pos_ps`. Fields whose meaning is not established yet keep their offset as a name.
 * size: 0x50 */
struct SeSlot {
    /* +0x00 */ u8 in_use;
    /* +0x01 */ u8 state;
    /* +0x02 */ u8 kind;
    /* +0x03 */ u8 pad_0x03;
    /* +0x04 */ nw4r::math::VEC3 pos;
    /* +0x10 */ nw4r::math::VEC3 field_0x10;
    /* +0x1C */ f32 field_0x1C;
    /* +0x20 */ f32 field_0x20;
    /* +0x24 */ u8 field_0x24;
    /* +0x25 */ u8 field_0x25;
    /* +0x26 */ u8 pad_0x26[2];
    /* +0x28 */ u32 owner;
    /* +0x2C */ u32 id;
    /* +0x30 */ s32 field_0x30;
    /* +0x34 */ s32 field_0x34;
    /* +0x38 */ u32 param;
    /* +0x3C */ u32 field_0x3C;
    /* +0x40 */ u32 field_0x40;
    /* +0x44 */ u8 pad_0x44[6];
    /* +0x4A */ u8 field_0x4A;
    /* +0x4B */ u8 field_0x4B;
    /* +0x4C */ s16 field_0x4C;
    /* +0x4E */ u8 pad_0x4E;
    /* +0x4F */ u8 field_0x4F;
};

/* The SE work object: one per sound source. +0x08 and +0x0C are read by `se_req_pos_ps` and by
 * `fn_800DA72C`'s callee; the 32 slots start at +0x3C and `fn_800D7F54` clears 0x2966C bytes and writes
 * a 16-step float ramp at +0x29238. The bytes between the slots and the ramp are not reached yet.
 * size: 0x295F4 (at least; the object is cleared to 0x2966C) */
struct _se_w {
    /* +0x000 */ u8 pad_0x000[8];
    /* +0x008 */ s32 field_0x08;
    /* +0x00C */ s32 field_0x0C;
    /* +0x010 */ u8 pad_0x010[0x2C];
    /* +0x03C */ SeSlot slots[32];
    /* +0x0A3C */ u8 pad_0x0A3C[0x287FC];
    /* +0x29238 */ f32 ramp[16];
    /* +0x29278 */ f32 field_0x29278;
    /* +0x2927C */ u8 pad_0x2927C[0x368];
    /* +0x295E4 */ s32 voices[2];
    /* +0x295EC */ u8 pad_0x295EC[2];
    /* +0x295EE */ u8 voice_slot;
    /* +0x295EF */ u8 field_0x295EF;
    /* +0x295F0 */ s32 field_0x295F0;
};


/* `_PLW`, the player work record, comes from `Pl/plw.h` - one definition, in the owner's header (rule 1). */
/* The per-kind container `get_move_work_adrs` returns: a status byte at +0x112 and a pointer array at
 * +0x138 indexed by the kind argument, each entry an `_se_w`. The array bound is an approximation (the
 * kind values in this unit stay well below it).
 * size: 0x238 (approximate) */
struct SeWork {
    /* +0x000 */ u8 pad_0x000[0x112];
    /* +0x112 */ u8 field_0x112;
    /* +0x113 */ u8 pad_0x113[0x25];
    /* +0x138 */ _se_w* se[64];
};
extern "C" SeSlot* fn_800DA72C(s32 kind, s32 id, nw4r::math::VEC3* pos);
extern "C" SeSlot* fn_800DBE94(s32 kind, s32 id, nw4r::math::VEC3* pos);
extern "C" SeSlot* fn_800DBFAC(s32 kind, s32 id, nw4r::math::VEC3* pos);
extern "C" SeSlot* fn_800D8E58(_se_w* work, s32 id);
extern "C" u32 fn_800D8D8C(nw4r::math::VEC3* pos);
extern "C" s32 fn_800DBB78(s32 bank, s32 id);
extern "C" s32 se_slot_req(s32 value);
extern "C" SeWork* get_move_work_adrs__FUc(u8 kind);

/* The non-positional entry point; `sysSE_req__Fl` is the map's mangled spelling of it. */
void sysSE_req(s32 id);

/* --- the fixed-id positional wrappers (16 bytes each) and the id-selecting ones (72 bytes) --------- */
extern "C" void fn_800D8E44(u8* p) {
    if (p != NULL) {
        *p = 0;
    }
}

extern "C" void fn_800DA864(nw4r::math::VEC3* pos) {
    u32 id = (fn_800D8D8C(pos) == 0) ? 51 : 151;
    fn_800DA72C(0, id, pos);
}

extern "C" void fn_800DA8AC(nw4r::math::VEC3* pos) {
    u32 id = (fn_800D8D8C(pos) == 0) ? 55 : 155;
    fn_800DA72C(0, id, pos);
}

extern "C" void fn_800DA8F4(nw4r::math::VEC3* pos) {
    u32 id = (fn_800D8D8C(pos) == 0) ? 50 : 150;
    fn_800DA72C(0, id, pos);
}

extern "C" void fn_800DA93C(nw4r::math::VEC3* pos) { fn_800DBFAC(0, 50, pos); }

extern "C" void fn_800DA94C(nw4r::math::VEC3* pos) { fn_800DA72C(0, 59, pos); }

extern "C" void fn_800DA95C(nw4r::math::VEC3* pos) {
    u32 id = (fn_800D8D8C(pos) == 0) ? 36 : 136;
    fn_800DA72C(0, id, pos);
}

extern "C" void fn_800DA9A4(nw4r::math::VEC3* pos) { fn_800DA72C(0, 57, pos); }

extern "C" void fn_800DA9B4(nw4r::math::VEC3* pos) { fn_800DA72C(0, 56, pos); }

extern "C" void fn_800DA9C4(nw4r::math::VEC3* pos) { fn_800DA72C(0, 58, pos); }

extern "C" void fn_800DA9D4(nw4r::math::VEC3* pos) { fn_800DA72C(0, 157, pos); }

extern "C" void fn_800DA9E4(nw4r::math::VEC3* pos) { fn_800DA72C(0, 156, pos); }

extern "C" void fn_800DA9F4(nw4r::math::VEC3* pos) { fn_800DA72C(0, 158, pos); }

void utiagebom_SE_req(nw4r::math::VEC3* pos) {
    u32 id = (fn_800D8D8C(pos) == 0) ? 79 : 179;
    fn_800DA72C(0, id, pos);
}

extern "C" void fn_800DAA4C(nw4r::math::VEC3* pos) {
    u32 id = (fn_800D8D8C(pos) == 0) ? 52 : 152;
    fn_800DA72C(0, id, pos);
}

extern "C" void fn_800DAA94(nw4r::math::VEC3* pos) {
    u32 id = (fn_800D8D8C(pos) == 0) ? 54 : 154;
    fn_800DA72C(0, id, pos);
}

extern "C" void fn_800DAD60(nw4r::math::VEC3* pos) {
    u32 id = (fn_800D8D8C(pos) == 1) ? 162 : 62;
    fn_800DA72C(0, id, pos);
}

extern "C" void fn_800DADA8(nw4r::math::VEC3* pos) {
    u32 id = (fn_800D8D8C(pos) == 0) ? 85 : 185;
    fn_800DA72C(0, id, pos);
}

extern "C" void fn_800DADF0(nw4r::math::VEC3* pos) {
    u32 id = (fn_800D8D8C(pos) == 0) ? 86 : 186;
    fn_800DA72C(0, id, pos);
}

extern "C" void fn_800DAE38(nw4r::math::VEC3* pos) { fn_800DA72C(0, 186, pos); }

extern "C" void fn_800DB4BC(nw4r::math::VEC3* pos) { fn_800DA72C(0, 27, pos); }

extern "C" void fn_800DB4CC(nw4r::math::VEC3* pos) { fn_800DA72C(0, 228, pos); }

extern "C" void fn_800DB4DC(nw4r::math::VEC3* pos) { fn_800DA72C(0, 250, pos); }

extern "C" void fn_800DB684(nw4r::math::VEC3* pos) {
    u32 id = (fn_800D8D8C(pos) == 1) ? 174 : 74;
    fn_800DA72C(0, id, pos);
}

extern "C" void fn_800DB6CC(nw4r::math::VEC3* pos) {
    u32 id = (fn_800D8D8C(pos) == 1) ? 226 : 126;
    fn_800DA72C(0, id, pos);
}

extern "C" void fn_800DB714(nw4r::math::VEC3* pos) {
    u32 id = (fn_800D8D8C(pos) == 1) ? 226 : 126;
    fn_800DA72C(0, id, pos);
}

extern "C" void fn_800DB91C(nw4r::math::VEC3* pos) {
    u32 id = (fn_800D8D8C(pos) == 0) ? 84 : 184;
    fn_800DA72C(0, id, pos);
}

extern "C" void fn_800DB964(nw4r::math::VEC3* pos) { fn_800DA72C(0, 97, pos); }

extern "C" void fn_800DBC00(void) { fn_800DBB78(0, 36); }

extern "C" void fn_800DBC0C(void) { fn_800DBB78(0, 37); }

extern "C" void fn_800DBC18(void) { fn_800DBB78(0, 31); }

extern "C" void fn_800DBC24(void) { fn_800DBB78(0, 12); }

extern "C" void fn_800DC098(nw4r::math::VEC3* pos) { fn_800DBE94(0, 23, pos); }

extern "C" void fn_800DC338(nw4r::math::VEC3* pos) { fn_800DA72C(0, 101, pos); }

extern "C" void fn_800DC3F4(nw4r::math::VEC3* pos) { fn_800DA72C(0, 63, pos); }

extern "C" void fn_800DC404(nw4r::math::VEC3* pos) {
    u32 id = (fn_800D8D8C(pos) == 0) ? 53 : 153;
    fn_800DA72C(0, id, pos);
}

extern "C" void fn_800DC44C(nw4r::math::VEC3* pos) { fn_800DA72C(0, 99, pos); }

extern "C" void fn_800DC45C(nw4r::math::VEC3* pos) { fn_800DA72C(0, 98, pos); }

extern "C" void fn_800DC46C(nw4r::math::VEC3* pos) {
    u32 id = (fn_800D8D8C(pos) == 0) ? 70 : 170;
    fn_800DA72C(0, id, pos);
}

extern "C" void fn_800DC4B4(nw4r::math::VEC3* pos) { fn_800DA72C(0, 71, pos); }

extern "C" void fn_800DC4C4(nw4r::math::VEC3* pos) { fn_800DA72C(0, 72, pos); }

extern "C" void fn_800DC4D4(nw4r::math::VEC3* pos) { fn_800DA72C(0, 73, pos); }

extern "C" void fn_800DC4E4(nw4r::math::VEC3* pos) { fn_800DA72C(0, 46, pos); }

extern "C" void fn_800DC4F4(nw4r::math::VEC3* pos) {
    u32 id = (fn_800D8D8C(pos) == 0) ? 74 : 174;
    fn_800DA72C(0, id, pos);
}

void bomb_tenka_se_req(nw4r::math::VEC3* pos) {
    u32 id = (fn_800D8D8C(pos) == 1) ? 168 : 68;
    fn_800DA72C(0, id, pos);
}

void bomb_doukasen_se_req(nw4r::math::VEC3* pos) {
    u32 id = (fn_800D8D8C(pos) == 1) ? 169 : 69;
    fn_800DA72C(0, id, pos);
}

void kemuri_hit_se_req(nw4r::math::VEC3* pos) { fn_800DA72C(0, 48, pos); }

void koyasi_hit_se_req(nw4r::math::VEC3* pos) { fn_800DA72C(0, 126, pos); }

extern "C" void fn_800DC7F0(nw4r::math::VEC3* pos) {
    u32 id = (fn_800D8D8C(pos) == 1) ? 149 : 49;
    fn_800DA72C(0, id, pos);
}

void ana_set_se_req(nw4r::math::VEC3* pos) { fn_800DA72C(0, 4, pos); }

void ana_eff_se_req(nw4r::math::VEC3* pos) { fn_800DA72C(0, 5, pos); }

extern "C" void fn_800DC858(nw4r::math::VEC3* pos) { fn_800DA72C(0, 78, pos); }

extern "C" void fn_800DC868(nw4r::math::VEC3* pos) { fn_800DA72C(0, 0, pos); }

extern "C" void fn_800DC878(nw4r::math::VEC3* pos) {
    u32 id = (fn_800D8D8C(pos) == 1) ? 187 : 87;
    fn_800DA72C(0, id, pos);
}

extern "C" void fn_800DCA08(nw4r::math::VEC3* pos) {
    u32 id = (fn_800D8D8C(pos) == 1) ? 167 : 67;
    fn_800DA72C(0, id, pos);
}

extern "C" void fn_800DCA50(nw4r::math::VEC3* pos) {
    u32 id = (fn_800D8D8C(pos) == 1) ? 183 : 83;
    fn_800DA72C(0, id, pos);
}

extern "C" void fn_800DCC0C(void) { fn_800DBB78(28, 27); }

extern "C" void fn_800DCC18(void) { fn_800DBB78(28, 35); }

extern "C" void fn_800DCFC0(void) { se_slot_req(0); }

extern "C" void fn_800DCFE4(void) { sysSE_req(12); }


/* --- hand-reconstructed functions ---------------------------------------------------------------- */

/* --- the non-positional path: `fn_800DBB78` plus its `sysSE_req`/`titleSE_req` family ------------- */

extern "C" s32 fn_800F0C14(s32 bank);
extern "C" s32 fn_800F04FC(s32 id);
extern "C" u8 fn_803C482C(void);
extern "C" u32 fn_800D843C(void);
extern "C" u8 system_w[];
extern "C" u8 lbl_80597A20[];

/* Resolves the bank to a sound handle and hands it to the playback helper; `-1` and the disabled
 * states short-circuit to 0. */
extern "C" s32 fn_800DBB78(s32 bank, s32 id) {
    s32 handle = fn_800F0C14(bank);
    if (handle == -1) {
        return 0;
    }
    if (system_w[0x30] == 0 || handle != 0) {
        if (fn_800D843C() == 1) {
            return 0;
        }
    }
    return fn_800E80DC_c1(handle, id, 0);
}

/* The `sysSE_req` family: bank 0, gated on the `fn_803C482C` predicate and the id being playable. */
void sysSE_req(s32 id) {
    if (fn_803C482C() == 0 && fn_800F04FC(0) != 0) {
        fn_800DBB78(0, id);
    }
}

extern "C" void sysSE_stop(s32 id) {
    if (fn_803C482C() == 0 && fn_800F04FC(30) != 0) {
        fn_800DBB78(30, id);
    }
}

extern "C" void fn_800DBCD8(s32 id) {
    if (fn_800F04FC(32) != 0) {
        fn_800DBB78(32, id);
    }
}

/* Picks the id out of the 3x6 byte table `lbl_80597A20`, indexed `row * 5 + column`. */
extern "C" void fn_800DBD1C(u32 row, u32 column) {
    if (row > 2) {
        return;
    }
    if (column > 5) {
        return;
    }
    if (fn_800F04FC(21) != 0) {
        fn_800DBB78(21, lbl_80597A20[row * 5 + column]);
    }
}

extern "C" void fn_800DBD90(s32 id) {
    if (fn_800F04FC(20) != 0) {
        fn_800DBB78(20, id);
    }
}

extern "C" void fn_800DBDD4(void) {
    if (fn_800F04FC(24) != 0) {
        fn_800DBB78(24, 0);
    }
}

extern "C" void fn_800DBE0C(s32 id) {
    if (fn_800F04FC(2) != 0) {
        fn_800DBB78(2, id);
    }
}

void titleSE_req(s32 id) {
    if (fn_800F04FC(46) != 0) {
        fn_800DBB78(46, id);
    }
}

/* --- positional wrappers with a mode switch ------------------------------------------------------ */

extern "C" void fn_800DC60C(nw4r::math::VEC3* pos, s32 mode) {
    s32 id = mode;
    switch (mode) {
    case 0:
        id = 22;
        break;
    case 1:
        id = 23;
        break;
    case 2:
        id = 24;
        break;
    }
    fn_800DA72C(0, id, pos);
}

extern "C" void fn_800DB4EC(u8 mode, nw4r::math::VEC3* pos) {
    s32 id = (s32)pos;
    switch (mode) {
    case 0:
        id = 1;
        break;
    case 1:
        id = 19;
        break;
    case 2:
        id = 20;
        break;
    }
    fn_800DA72C(0, id, pos);
}

/* 232 when the flag is clear, 233 otherwise. */
extern "C" void fn_800DC53C(u8 flag, nw4r::math::VEC3* pos) {
    fn_800DA72C(0, (flag == 0) ? 232 : 233, pos);
}

/* The two-title sound ids; both tail-call `sysSE_req`. */
extern "C" void fn_800DCFC8(u8 which) {
    if (which == 0) {
        sysSE_req(11);
    } else {
        sysSE_req(10);
    }
}

/* --- positional wrappers that forward to `se_req_pos_ps` ----------------------------------------- */

struct _se_w;
void se_req_pos_ps(_se_w* self, s32 id, s32 param, nw4r::math::VEC3* pos);

extern "C" void fn_800DCA98(_se_w* self, nw4r::math::VEC3* pos, u8 id) {
    se_req_pos_ps(self, id + 137, 2, pos);
}


/* part a - the core `_se_w`/slot cluster (address order).
 *
 * Residuals and shared-type notes are collected at the bottom of the file.
 */



/* `_PLW`'s +0x4A4 counter as `se_req_frame_set` touches it (a byte increment and reload); the prelude's
 * `_PLW` types the field as a word. Same object, same layout.
 * size: 0x4A8 (at least) */
struct _PLWCounter {
    /* +0x000 */ u8 pad_0x000[0x4A4];
    /* +0x4A4 */ u8 field_0x4A4;
};

/* The head of `_se_w` as this unit reads it: the prelude's `_se_w` models the slots only, so the fields
 * the request helpers touch outside them are declared here. Same object, same layout.
 * size: 0xB00 (at least) */
struct SeWorkObj {
    /* +0x000 */ u8 pad_0x000[2];
    /* +0x002 */ u8 field_0x002;
    /* +0x003 */ u8 pad_0x003;
    /* +0x004 */ _PLW* field_0x004;
    /* +0x008 */ s32 field_0x008;
    /* +0x00C */ s32 field_0x00C;
    /* +0x010 */ u8 pad_0x010[0x2C];
    /* +0x03C */ SeSlot slots[32];
    /* +0x0A3C */ u8 pad_0x0A3C[0xC0];
    /* +0x0AFC */ _se_w* field_0x0AFC;
};

/* `SeSlot` with the sub-sound byte `fn_800D9B6C` fills in; the prelude's copy folds +0x25 into
 * `pad_0x25`. Same object, same layout.
 * size: 0x50 */
struct SeSlotObj {
    /* +0x00 */ u8 in_use;
    /* +0x01 */ u8 state;
    /* +0x02 */ u8 kind;
    /* +0x03 */ u8 pad_0x03;
    /* +0x04 */ nw4r::math::VEC3 pos;
    /* +0x10 */ nw4r::math::VEC3 field_0x10;
    /* +0x1C */ u8 pad_0x1C[8];
    /* +0x24 */ u8 field_0x24;
    /* +0x25 */ u8 field_0x25;
    /* +0x26 */ u8 pad_0x26[2];
    /* +0x28 */ u32 owner;
    /* +0x2C */ u32 id;
    /* +0x30 */ s32 field_0x30;
    /* +0x34 */ s32 field_0x34;
    /* +0x38 */ u32 param;
    /* +0x3C */ u32 field_0x3C;
    /* +0x40 */ u8 pad_0x40[0x0A];
    /* +0x4A */ u8 field_0x4A;
    /* +0x4B */ u8 field_0x4B;
    /* +0x4C */ u8 pad_0x4C[4];
};

/* `_LB_NPC`: the NPC/actor object `se_req_frame_set_npc` reads its kind byte from.
 * size: 0x3E04 (at least) */
struct _LB_NPC {
    /* +0x000 */ u8 pad_0x000[2];
    /* +0x002 */ u8 field_0x002;
    /* +0x003 */ u8 pad_0x003[0x3E01];
};

/* The game's user-data block; only the byte the NPC request reads is modelled.
 * size: 0x3E04 (at least) */
struct _UserData {
    /* +0x000 */ u8 pad_0x000[0x3E03];
    /* +0x3E03 */ u8 field_0x3E03;
};

extern "C" void* memset(void* dst, s32 c, u32 n);
extern "C" u32 stage_water_area_ck(void);
extern "C" u32 fn_800D843C(void);
extern "C" void fn_800D8EA8(_se_w* work, SeSlot* slot);
extern "C" s32 fn_800F0C14(s32 owner);
extern "C" s32 fn_800F2890(s32 bank, s32 id);
extern "C" u8 fn_800F0C74(s32 owner);
extern "C" u32 fn_802D29A0(_PLW* plw, s32 zero, f32 a, f32 b);
extern "C" u32 fn_801FE1DC(_LB_NPC* npc, s32 zero, f32 a, f32 b);
extern "C" u32 fn_80385C80(_PLW* plw, s32 zero, f32 a, f32 b);
extern "C" u32 ai_get_motion_no__FP8_AINPC_W(_PLW* plw);
extern "C" _UserData* get_userdata__Fv(void);
extern "C" f32 lbl_807963E0;
extern u16 lbl_80791438;
extern u8 lbl_805979EC[];

/* ---------------------------------------------------------------------------------------------------
 * 0x800D8D8C  "should the louder variant of this id be used?" - true when the SE system is active and
 * the position sits below the world's audible band.
 */
extern "C" u32 fn_800D8D8C(nw4r::math::VEC3* pos) {
    if (stage_water_area_ck() == 1 && pos->y < lbl_807963E0) {
        return 1;
    }
    return 0;
}

/* ---------------------------------------------------------------------------------------------------
 * 0x800D8DDC  claims the first clear entry of the 64-entry `_se_w` pool and clears it.
 */
extern "C" u8* fn_800D8DDC(u8* base) {
    u8* p = base + 0x34;
    for (s32 i = 0; i < 64; i++) {
        if (*p == 0) {
            memset(p, 0, 0xA48);
            *p = 1;
            return p;
        }
        p += 0xA48;
    }
    return NULL;
}

/* ---------------------------------------------------------------------------------------------------
 * 0x800D8E58  claims the first free slot of one `_se_w` work object and clears its state.
 */
extern "C" SeSlot* fn_800D8E58(_se_w* work, s32 id) {
    SeSlot* s = work->slots;
    for (s32 i = 0; i < 32; i++) {
        if (s->in_use == 0) {
            s->in_use = 1;
            s->kind = 0;
            s->pad_0x03 = 0;
            s->field_0x24 = 0;
            s->field_0x3C = 0;
            s->field_0x4A = 0;
            s->field_0x4B = 0;
            return s;
        }
        s++;
    }
    return NULL;
}

/* ---------------------------------------------------------------------------------------------------
 * 0x800D9B6C  fills in a claimed slot's "which sub-sound" byte from the work object's state: the motion
 * number is looked up in a per-kind table (and, for the alternate state, the actor's own table).
 */
extern "C" void fn_800D9B6C(_se_w* work, SeSlot* slot) {
    SeWorkObj* w = (SeWorkObj*)work;
    SeSlotObj* sl = (SeSlotObj*)slot;
    _PLW* plw;
    u16* p;
    u8* q;
    s32 found;

    sl->field_0x25 = 0;
    found = 0;
    s32 v = w->field_0x008;
    switch (v) {
    case 1:
    case 3:
        plw = w->field_0x004;
        p = &lbl_80791438;
        {
            u32 motion = Get_motion_no(plw) & 0xFFFF;
            while (*p != 0xFFFF) {
                if (*p == motion) {
                    found = 1;
                    break;
                }
                p++;
            }
        }
        if (found == 0) {
            sl->field_0x25 = plw->chunk_ofs;
        } else {
            sl->field_0x25 = 0;
        }
        break;
    case 4:
        plw = w->field_0x004;
        if (plw->field_0x1C8 & 1) {
            sl->field_0x25 = plw->field_0x01A;
        } else {
            q = lbl_805979EC;
            while (*q != 0) {
                if (*q == plw->field_0x003) {
                    found = 1;
                    break;
                }
                q++;
            }
            if (found == 0) {
                sl->field_0x25 = plw->field_0x01A - 32;
            } else {
                sl->field_0x25 = 0;
            }
        }
        break;
    case 9:
    case 10:
    case 12:
        sl->field_0x25 = 0;
        break;
    }
}

/* ---------------------------------------------------------------------------------------------------
 * 0x800D9CC8  starts a "frame" sound from an id plus a packed (kind-high-byte, sub-kind) parameter.
 */
extern "C" void fn_800D9CC8(_se_w* work, s32 id, u32 param, s32 value) {
    SeWorkObj* w = (SeWorkObj*)work;
    if (work == NULL) {
        return;
    }
    if (fn_800D843C() == 1) {
        return;
    }
    SeSlot* s = fn_800D8E58(work, id);
    if (s == NULL) {
        return;
    }
    u32 hi = (param & 0xFF000000) >> 24;
    u32 lo = param & 0xFFFFFF;
    if (w->field_0x002 != 0) {
        s->state = 3;
        s->kind = 2;
    } else {
        s->state = 3;
        s->kind = lo;
    }
    if (hi == 0) {
        s->owner = w->field_0x00C;
    } else {
        s->owner = hi;
    }
    s->id = id;
    s->field_0x30 = -1;
    s->field_0x34 = value;
    s->param = 0;
    fn_800D9B6C(work, s);
    fn_800D8EA8(work, s);
}

/* ---------------------------------------------------------------------------------------------------
 * 0x800D9DB4  the same request as `fn_800D9CC8`, plus the actor's two extra ids.
 */
extern "C" void fn_800D9DB4(_se_w* work, s32 id, u32 param, s32 id2, s32 id3) {
    SeWorkObj* w = (SeWorkObj*)work;
    if (work == NULL) {
        return;
    }
    if (fn_800D843C() == 1) {
        return;
    }
    SeSlot* s = fn_800D8E58(work, id);
    if (s == NULL) {
        return;
    }
    s->field_0x24 = 1;
    u32 hi = (param & 0xFF000000) >> 24;
    u32 lo = param & 0xFFFFFF;
    if (w->field_0x002 != 0) {
        s->state = 3;
        s->kind = 2;
    } else {
        s->state = 3;
        s->kind = lo;
    }
    if (hi == 0) {
        s->owner = w->field_0x00C;
    } else {
        s->owner = hi;
    }
    s->id = id;
    s->field_0x30 = id2;
    s->field_0x34 = id3;
    s->param = 0;
    fn_800D9B6C(work, s);
    fn_800D8EA8(work, s);
}

/* ---------------------------------------------------------------------------------------------------
 * 0x800D9EA8  the positional request that goes through the bank/kind machinery: resolve the bank from
 * the work object's owner, remap the id for the "loud" case, then claim a slot and start it.
 */
void se_req_pos_ps(_se_w* work, s32 id, s32 param, nw4r::math::VEC3* pos) {
    s32 p = 0;
    if (work == NULL) {
        return;
    }
    if (fn_800D843C() == 1) {
        return;
    }
    s32 bank = fn_800F0C14(work->field_0x0C);
    if (bank == -1) {
        return;
    }
    if (work->field_0x08 == 4 && id == 36) {
        if (fn_800D8D8C(pos) == 1) {
            id += 100;
        }
    }
    if (fn_800F2890(bank, id) == 0) {
        return;
    }
    SeSlot* s = fn_800D8E58(work, id);
    if (s == NULL) {
        return;
    }
    u32 hi = (param & 0xFF000000) >> 24;
    if (id == 36) {
        p = 8;
    }
    s->state = 3;
    s->kind = 2;
    if (hi == 0) {
        s->owner = work->field_0x0C;
    } else {
        s->owner = hi;
    }
    s->id = id;
    s->field_0x30 = -1;
    s->field_0x34 = -2;
    s->param = p;
    copyVec3(&s->pos, pos);
    s->field_0x10 = s->pos;
    fn_800D9B6C(work, s);
}

/* ---------------------------------------------------------------------------------------------------
 * 0x800DA154  the "frame set" request for the player: only the `state == 2` work object takes it, and
 * only when the actor's range check passes.
 */
extern "C" void fn_800DA428(_se_w* work, s32 a, u32 param, s32 d, s32 e) {
    if (work == NULL) {
        return;
    }
    if (fn_800D843C() == 1) {
        return;
    }
    if (work->field_0x08 != 2) {
        return;
    }
    u32 hi = (param & 0xFF000000) >> 24;
    u8 bank = fn_800F0C74(hi);
    if (bank == 0) {
        return;
    }
    param &= 0xFFFFFF;
    work->field_0x0C = bank;
    if (fn_8026A328(((SeWorkObj*)work)->field_0x004, 0, (f32)a, lbl_807963E0) != 1) {
        return;
    }
    if ((u16)param == 0xFFFF) {
        return;
    }
    fn_800D9CC8(work, param, e, d);
}

/* ---------------------------------------------------------------------------------------------------
 * 0x800DA3C0  the same request for the "aifs" state (11) and its own range predicate.
 */
void se_req_frame_set_aifs(_se_w* work, s32 a, s32 param, s32 d, s32 e) {
    if (work == NULL) {
        return;
    }
    if (fn_800D843C() == 1) {
        return;
    }
    if (work->field_0x08 != 11) {
        return;
    }
    u32 hi = (param & 0xFF000000) >> 24;
    u8 bank = fn_800F0C74(hi);
    if (bank == 0) {
        return;
    }
    param &= 0xFFFFFF;
    work->field_0x0C = bank;
    if (fn_802D29A0(((SeWorkObj*)work)->field_0x004, 0, (f32)a, lbl_807963E0) != 1) {
        return;
    }
    if ((u16)param == 0xFFFF) {
        return;
    }
    fn_800D9CC8(work, param, e, d);
}

/* ---------------------------------------------------------------------------------------------------
 * 0x800DA490  the frame-set request for an NPC: the id is bumped by 10 for the kind-6 user, and the
 * NPC object itself is what the range predicate is asked about.
 */
void se_req_frame_set_npc(_se_w* work, _LB_NPC* npc, s32 a, s32 param, s32 e) {
    s32 bump = 0;
    if (work == NULL) {
        return;
    }
    if (fn_800D843C() == 1) {
        return;
    }
    if (npc->field_0x002 == 1) {
        if ((get_userdata__Fv()->field_0x3E03 & 0x7F) == 6) {
            bump = 10;
        }
    }
    if (fn_801FE1DC(npc, 0, (f32)a, lbl_807963E0) != 1) {
        return;
    }
    if ((u16)param == 0xFFFF) {
        return;
    }
    fn_800D9CC8(work, param + bump, e, (s32)npc);
}

/* ---------------------------------------------------------------------------------------------------
 * 0x800DA154  the frame-set request: one arm per work-object state, each with its own range predicate
 * and its own id fixup (the enemy's 27/28 kinds and motion 42, the AI's motion 103).
 */
void se_req_frame_set(_se_w* work, s32 a, s32 param, s32 d, s32 e) {
    SeWorkObj* w = (SeWorkObj*)work;
    if (work == NULL) {
        return;
    }
    if (fn_800D843C() == 1) {
        return;
    }
    switch (w->field_0x008) {
    case 1:
    case 3:
        if (fn_8026A328(w->field_0x004, 0, (f32)a, lbl_807963E0) != 1) {
            return;
        }
        if ((u16)param == 0xFFFF) {
            return;
        }
        fn_800D9CC8(work, param, e, d);
        break;
    case 4:
        {
            _PLW* plw = w->field_0x004;
            if (em_after_frame_check__FP11_ENEMY_WORKUsff(plw, 0, (f32)a, lbl_807963E0) != 1) {
                if (w->field_0x002 != 2) {
                    return;
                }
            }
            if ((u16)param == 0xFFFF) {
                return;
            }
            /* the two "kind 27/28" values, written the way retail's `addi ...,229` needs them */
            if ((u8)(plw->field_0x003 + 229) <= 1 && plw->field_0x00A == 2) {
                param += 100;
            }
            fn_800D9CC8(work, param, e, d);
        }
        break;
    case 9:
        {
            _PLW* plw;
            if (fn_802D29A0(w->field_0x004, 0, (f32)a, lbl_807963E0) != 1) {
                return;
            }
            if ((u16)param == 0xFFFF) {
                return;
            }
            plw = w->field_0x004;
            if ((u16)ai_get_motion_no__FP8_AINPC_W(plw) == 42 && param == 103) {
                _PLWCounter* counter = (_PLWCounter*)plw;
                counter->field_0x4A4++;
                if (counter->field_0x4A4 == 3) {
                    param = 102;
                }
            }
            fn_800D9CC8(work, param, e, d);
        }
        break;
    case 10:
    case 12:
        if (fn_80385C80(w->field_0x004, 0, (f32)a, lbl_807963E0) != 1) {
            return;
        }
        if ((u16)param == 0xFFFF) {
            return;
        }
        fn_800D9CC8(work, param, e, d);
        break;
    }
}

/* ---------------------------------------------------------------------------------------------------
 * 0x800DA244  the "sp" (special) frame-set request: the same state arms as `se_req_frame_set`, but the
 * slot is started through `fn_800D9DB4` with the actor's two extra ids.
 */
void se_req_frame_set_sp(_se_w* work, s32 a, s32 param, s32 d, s32 e, s32 f) {
    SeWorkObj* w = (SeWorkObj*)work;
    if (work == NULL) {
        return;
    }
    if (fn_800D843C() == 1) {
        return;
    }
    switch (w->field_0x008) {
    case 1:
    case 3:
        if (fn_8026A328(w->field_0x004, 0, (f32)a, lbl_807963E0) != 1) {
            return;
        }
        if ((u16)param == 0xFFFF) {
            return;
        }
        fn_800D9DB4(work, param, d, f, e);
        break;
    case 4:
        if (em_after_frame_check__FP11_ENEMY_WORKUsff(w->field_0x004, 0, (f32)a, lbl_807963E0) != 1) {
            if (w->field_0x002 != 2) {
                return;
            }
        }
        if ((u16)param == 0xFFFF) {
            return;
        }
        fn_800D9DB4(work, param, d, f, e);
        break;
    case 9:
        if (fn_802D29A0(w->field_0x004, 0, (f32)a, lbl_807963E0) != 1) {
            return;
        }
        if ((u16)param == 0xFFFF) {
            return;
        }
        fn_800D9DB4(work, param, d, f, e);
        break;
    case 10:
    case 12:
        if (fn_80385C80(w->field_0x004, 0, (f32)a, lbl_807963E0) != 1) {
            return;
        }
        if ((u16)param == 0xFFFF) {
            return;
        }
        fn_800D9DB4(work, param, d, f, e);
        break;
    }
}

/* ---------------------------------------------------------------------------------------------------
 * 0x800DBE94  the lobby's positional request: same as `fn_800DA72C` but the work object is the lobby's
 * own, so no kind lookup. The id's top bit is the "loud" flag, stripped off and recorded as no owner.
 */
extern "C" SeSlot* fn_800DBE94(s32 kind, s32 id, nw4r::math::VEC3* pos) {
    LbLobbyWork* lw = &lobby_w;
    if (fn_800D843C() == 1) {
        return NULL;
    }
    _se_w* work = lw->field_0x0B8;
    if (work == NULL) {
        return NULL;
    }
    SeSlot* s = fn_800D8E58(work, id);
    if (s == NULL) {
        return NULL;
    }
    s32 loud = 0;
    if (id & 0x80000000) {
        id &= 0x7FFFFFFF;
        loud = 1;
    }
    s->state = 3;
    s->kind = 1;
    if (loud == 0) {
        s->owner = work->field_0x0C;
    } else {
        s->owner = 0;
    }
    s->id = id;
    s->field_0x30 = -1;
    s->field_0x34 = -1;
    s->param = 0;
    copyVec3(&s->pos, pos);
    s->field_0x10 = s->pos;
    return s;
}

/* ---------------------------------------------------------------------------------------------------
 * 0x800DBFAC  the lobby's plain positional request: `fn_800DBE94` without the loud flag, so the slot
 * always takes the work object's owner.
 */
extern "C" SeSlot* fn_800DBFAC(s32 kind, s32 id, nw4r::math::VEC3* pos) {
    LbLobbyWork* lw = &lobby_w;
    if (fn_800D843C() == 1) {
        return NULL;
    }
    _se_w* work = lw->field_0x0BC;
    if (work == NULL) {
        return NULL;
    }
    SeSlot* s = fn_800D8E58(work, id);
    if (s == NULL) {
        return NULL;
    }
    s->state = 3;
    s->kind = 1;
    s->owner = work->field_0x0C;
    s->id = id;
    s->field_0x30 = -1;
    s->field_0x34 = -1;
    s->param = 0;
    copyVec3(&s->pos, pos);
    s->field_0x10 = s->pos;
    return s;
}

/* ---------------------------------------------------------------------------------------------------
 * 0x800DB974  starts one of the "hit" sounds: the work object's kind byte picks the id out of a
 * 9-value table, and the loud variant of the table (id + 100) is used when `fn_800D8D8C` says so.
 */
extern "C" void fn_800DB974(_se_w* work, nw4r::math::VEC3* pos) {
    SeWorkObj* w = (SeWorkObj*)work;
    _se_w* target = w->field_0x0AFC;
    if (target == NULL) {
        return;
    }
    u32 id = 0;
    if (fn_800D8D8C(pos) == 0) {
        switch (w->field_0x002) {
        case 1:
        case 4:
        case 5:
        case 6:
            id = 87;
            break;
        case 0:
        case 2:
        case 3:
        case 7:
        case 8:
            id = 88;
            break;
        }
    } else {
        switch (w->field_0x002) {
        case 1:
        case 4:
        case 5:
        case 6:
            id = 187;
            break;
        case 0:
        case 2:
        case 3:
        case 7:
        case 8:
            id = 188;
            break;
        }
    }
    if (id == 0) {
        return;
    }
    SeSlot* s = fn_800D8E58(target, id);
    if (s == NULL) {
        return;
    }
    s->state = 3;
    s->kind = 1;
    s->owner = 3;
    s->id = id;
    s->field_0x30 = -1;
    s->field_0x34 = -1;
    s->param = 0;
    copyVec3(&s->pos, pos);
    s->field_0x10 = s->pos;
}

/* ---------------------------------------------------------------------------------------------------
 * 0x800DA72C  starts a positional sound: picks the `kind` work object, claims a slot, copies the
 * position in and initialises the slot's playback state. The id's top bit is a "loud" flag that is
 * stripped off and recorded as "no owner".
 */
extern "C" SeSlot* fn_800DA72C(s32 kind, s32 id, nw4r::math::VEC3* pos) {
    if (fn_800D843C() == 1) {
        return NULL;
    }
    s32 loud = 0;
    if (id & 0x80000000) {
        id &= 0x7FFFFFFF;
        loud = 1;
    }
    SeWork* w = get_move_work_adrs__FUc(0);
    if (w == NULL) {
        return NULL;
    }
    if (kind == 46) {
        kind = 2;
    }
    _se_w* work = w->se[kind];
    if (work == NULL) {
        return NULL;
    }
    work->field_0x08 = 6;
    SeSlot* s = fn_800D8E58(work, id);
    if (s == NULL) {
        return NULL;
    }
    s->state = 3;
    s->kind = 1;
    if (loud == 0) {
        s->owner = work->field_0x0C;
    } else {
        s->owner = 0;
    }
    s->id = id;
    s->field_0x30 = -1;
    s->field_0x34 = -1;
    s->param = 0;
    copyVec3(&s->pos, pos);
    s->field_0x10 = s->pos;
    return s;
}

/* ===================================================================================================
 * Residuals (everything here is measured with `.pi/scratch/measure_part.py`; 10 of the 16 symbols are
 * byte-exact, the rest differ only in which callee-saved register the allocator picked).
 *
 * fn_800D9B6C      96.667  the `9/10` range test of the switch: retail lowers it to `addi r,r,-9;
 *                          cmplwi r,1; ble` (3 instrs), ours to `cmpwi r,9; blt; cmpwi r,10; ble` (4).
 *                          `found` also lands in r29 and `p` in r30, retail has them the other way
 *                          round; the bodies, the two lookup loops and the compare chain order are equal.
 * fn_800D9CC8      99.831  `param &= 0xFFFFFF` is folded into the else arm's store (`clrlwi r5,r29,8` +
 *                          `clrlwi r0,r5,24`), retail keeps it in the parameter's own r29
 *                          (`clrlwi r29,r29,8` then `clrlwi r0,r29,24`). Same two instructions.
 * fn_800D9DB4      99.590  same mask residual as `fn_800D9CC8`.
 * se_req_pos_ps    99.062  the `p` counter (r30 retail / r31 ours) and the bank-then-slot pointer
 *                          (r31 retail / r30 ours) are mirrored.
 * fn_800DA72C      98.718  allocator colouring only: retail `kind`/slot r30, `loud` r29, `work` r31;
 *                          ours r29, r31, r30. Declaration order, a `u8`/`bool` `loud`, a local copy of
 *                          `kind`, the arm order and a `? :` remap were all tried and do not move it.
 * fn_800DBE94      99.857  `&lobby_w` shares r31 with the `work` pointer it feeds (`lwz r31,184(r31)`);
 *                          retail keeps the address in r30 (`lwz r31,184(r30)`). A pointer, a reference
 *                          and a `_se_w* work;`-then-assign form all give ours.
 * fn_800DBFAC      99.831  same as `fn_800DBE94` (+0xBC instead of +0xB8).
 *
 * Shared-type notes for the orchestrator (the prelude was NOT edited):
 *   - `_se_w` needs `u8 field_0x002` at +0x02 (a flag: `fn_800D9CC8`/`fn_800D9DB4`/`fn_800DB974` read
 *     it) and `_se_w* field_0x0AFC` at +0xAFC (`fn_800DB974`); the 32 `SeSlot`s really end at +0xA3C,
 *     so +0xAFC is inside the prelude's unmodelled tail. This part carries `SeWorkObj` for both.
 *   - `SeSlot` needs `u8 field_0x25` at +0x25 (the sub-sound byte `fn_800D9B6C` writes); the prelude
 *     folds it into `pad_0x25`. This part carries `SeSlotObj`.
 *   - `_PLW.field_0x4A4` is a *byte* in `se_req_frame_set` (`lbz/addi/stb` then a reload); the prelude
 *     types it as `u32`. This part carries `_PLWCounter` for the byte access.
 *   - `lobby_w` (`.bss`, size 0x17C) needs `_se_w*` at +0xB8 and +0xBC. This part carries `LobbyWork`.
 *   - `_LB_NPC` (+0x02 read by `se_req_frame_set_npc`) and the user-data block (+0x3E03) are declared
 *     here because nothing else in the unit touches them.
 * =================================================================================================== */


/* part b - functions 0x800DB2DC..0x800DCB74 of auto/800D7F54_fn_800D7F54.
 *
 * Residuals / notes live in this header. Nothing here is committed; the orchestrator owns the unit.
 *
 * fn_800DBACC (89.19 %): 12 of 13 functions here are byte-identical; this one is one instruction long.
 *   The target keeps the *biased* base (`base + 0x30000`) in the callee-saved r31 and lets the loaded
 *   pointer die in r3; we keep the loaded pointer in r31 and recompute `addis r3, r31, 3` after the
 *   copyVec3 call. Same instruction count either way, so it is the allocator's tie-break between
 *   "keep the pointer, rematerialise the bias" and "keep the bias" - not source-shaped. Tried: the
 *   target's statement order and the ascending order, the store duplicated, `if (self != NULL)` vs the
 *   early return, a second/`void*`/`u32` local, `lbl_80794978` declared `SeSysWork*`/`void*`/`_se_w*`,
 *   a reference, a separate assignment, a const pointer, nested-scope assignment. All give r31 = base.
 *
 * fn_800F2680 must return u32 here: with u8 the u8->f32 narrowing loses its `mr r0,r3` copy and both
 *   fn_800DB36C and fn_800DB86C drop off 100 % (96.37 / 93.07).
 *
 * `se_req_pos_ps` is declared as a C++ function on purpose - the map's symbol is the mangled
 *   `se_req_pos_ps__FP5_se_wllPQ34nw4r4math4VEC3`, and `extern "C"` emits an unmangled reloc instead.
 *
 * The `(f32)` int conversion pools its 2^52 double locally (`@43`) where the target references the
 *   extern `lbl_80796400`; objdiff ignores reloc names so the score is unaffected, but the unit's
 *   .sdata2 cannot be byte-exact from source. */

/* --- local types ------------------------------------------------------------------------------- */

/* The sound-system work object `lbl_80794978` points at: `fn_800D7F54` memsets 0x2966C bytes of it,
 * and the offsets below are the ones this part touches.
 * size: 0x2966C */
struct SeSysWork {
    /* +0x00000 */ u8 pad_0x00000[0x295E4];
    /* +0x295E4 */ void* field_0x295E4[2];
    /* +0x295EC */ u8 pad_0x295EC[3];
    /* +0x295EF */ u8 field_0x295EF;
    /* +0x295F0 */ void* field_0x295F0;
    /* +0x295F4 */ u8 pad_0x295F4[0x28];
    /* +0x2961C */ u8 field_0x2961C;
    /* +0x2961D */ u8 field_0x2961D;
    /* +0x2961E */ u8 field_0x2961E;
    /* +0x2961F */ u8 field_0x2961F;
    /* +0x29620 */ nw4r::math::VEC3 field_0x29620;
    /* +0x2962C */ nw4r::math::VEC3 field_0x2962C;
    /* +0x29638 */ u8 pad_0x29638[8];
    /* +0x29640 */ u8 field_0x29640;
    /* +0x29641 */ u8 pad_0x29641[3];
    /* +0x29644 */ u32 field_0x29644;
    /* +0x29648 */ u32 field_0x29648;
    /* +0x2964C */ u32 field_0x2964C;
    /* +0x29650 */ u32 field_0x29650;
    /* +0x29654 */ u32 field_0x29654;
    /* +0x29658 */ u32 field_0x29658;
    /* +0x2965C */ u8 pad_0x2965C[0x0A];
    /* +0x29666 */ u8 field_0x29666;
    /* +0x29667 */ u8 field_0x29667;
    /* +0x29668 */ u8 pad_0x29668[4];
};

/* A player work object; only the `_se_w` pointer at +0xAFC is read here.
 * size: 0xB00 (approximate) */

/* --- declarations ------------------------------------------------------------------------------ */

extern "C" u8 GameMode_ck(void);
extern "C" u32 fn_800D843C(void);
extern "C" void fn_800D9DB4(_se_w* se, s32 a, u32 b, s32 c, s32 d);
extern "C" u32 fn_800DAE48(void);
extern "C" void fn_800DB2DC(void);
extern "C" s32 fn_800F0C14(s32 owner);
extern "C" u8 fn_800F2680(s32 id, s32 kind);
void se_req_pos_ps(_se_w* se, s32 id, s32 kind, nw4r::math::VEC3* pos);

extern "C" _se_w* lbl_80794978;
extern "C" s16 lbl_80597A00[];
extern "C" f32 lbl_807963E4;
extern "C" f64 lbl_80796400;

/* --- 0x800DB2DC -------------------------------------------------------------------------------- */

/* Releases the two sound handles the system work keeps in its pointer table. */
extern "C" void fn_800DB2DC(void) {
    SeSysWork* self = (SeSysWork*)lbl_80794978;

    if (self->field_0x295F0 != NULL && fn_800E802C_c1((s32)self->field_0x295F0) != 0) {
        fn_800E8228_c1((s32)self->field_0x295F0);
    }
    if (self->field_0x295E4[self->field_0x295EF] != NULL &&
        fn_800E802C_c1((s32)self->field_0x295E4[self->field_0x295EF]) != 0) {
        fn_800E8228_c1((s32)self->field_0x295E4[self->field_0x295EF]);
    }
}

/* --- 0x800DB36C -------------------------------------------------------------------------------- */

/* Maps a trap kind and its sub-kind onto a handle slot, then applies a volume. */
extern "C" void fn_800DB36C(u8 kind, u32 unused, u8 sub) {
    if (fn_800D843C() == 1) {
        return;
    }
    fn_800DB2DC();
    u8 handle = fn_800DAE48();
    if (handle == 0) {
        return;
    }

    s32 slot;
    switch (kind) {
    case 0:
        slot = 6;
        break;
    case 1:
        slot = 7;
        break;
    case 2:
    case 3:
        if (sub == 1) {
            slot = 8;
        } else {
            slot = 16;
        }
        break;
    case 130:
    case 131:
        if (sub == 1) {
            slot = 13;
        } else {
            slot = 16;
        }
        break;
    case 4:
        slot = 9;
        break;
    default:
        return;
    }

    void* obj = (void*)fn_800E80DC_c1(handle, slot, 0);
    if (obj == NULL) {
        return;
    }
    u8 volume = fn_800F2680(handle, slot);
    fn_800E8354_c1((s32)obj, (s16)(lbl_807963E4 * (f32)(u8)volume));
}

/* --- 0x800DB608 -------------------------------------------------------------------------------- */

/* Looks a fixed sound id up in the trap table and requests it, optionally at the louder variant. */
extern "C" void fn_800DB608(u8 row, nw4r::math::VEC3* pos, u8 col) {
    s32 id = lbl_80597A00[row * 2 + col];

    if (id == 0) {
        return;
    }
    if (fn_800D8D8C(pos) == 1) {
        id += 100;
    }
    fn_800DA72C(0, id, pos);
}

/* --- 0x800DB75C -------------------------------------------------------------------------------- */

/* Requests the player's footstep sound through the player's own sound work. */
extern "C" void fn_800DB75C(_PLW* plw, nw4r::math::VEC3* pos) {
    if (plw == NULL) {
        return;
    }
    _se_w* se = plw->field_0xAFC;
    if (se == NULL) {
        return;
    }
    u32 id = (fn_800D8D8C(pos) == 1) ? 44 : 24;
    se_req_pos_ps(se, id, 2, pos);
}

/* --- 0x800DB7C8 -------------------------------------------------------------------------------- */

/* Requests the player's other footstep sound through the player's own sound work. */
extern "C" void fn_800DB7C8(_PLW* plw, nw4r::math::VEC3* pos) {
    if (plw == NULL) {
        return;
    }
    _se_w* se = plw->field_0xAFC;
    if (se == NULL) {
        return;
    }
    u32 id = (fn_800D8D8C(pos) == 1) ? 45 : 25;
    se_req_pos_ps(se, id, 2, pos);
}

/* --- 0x800DB86C -------------------------------------------------------------------------------- */

/* Plays the weapon-hit sound of the player's currently equipped motion at a fixed volume. */
extern "C" void fn_800DB86C(_PLW* plw) {
    _se_w* se = plw->field_0xAFC;

    if (se == NULL) {
        return;
    }
    s32 motion = fn_800F0C14(se->field_0x0C);
    if (motion == -1) {
        return;
    }
    void* obj = (void*)fn_800E80DC_c1(motion, 21, 0);
    if (obj == NULL) {
        return;
    }
    u8 volume = fn_800F2680(motion, 1);
    fn_800E8354_c1((s32)obj, (s16)(lbl_807963E4 * (f32)(u8)volume));
}

/* --- 0x800DBACC -------------------------------------------------------------------------------- */

/* Resets the sound-system work object and seeds its default position. */
extern "C" void fn_800DBACC(nw4r::math::VEC3* pos) {
    SeSysWork* self = (SeSysWork*)lbl_80794978;

    if (self == NULL) {
        return;
    }
    self->field_0x2961C = 1;
    self->field_0x2961E = 0;
    self->field_0x2961F = 0;
    self->field_0x29640 = 0;
    self->field_0x29658 = 0;
    self->field_0x29666 = 0;
    self->field_0x29667 = 0;
    self->field_0x2961D = 3;
    self->field_0x2961E = 2;
    self->field_0x29644 = 21;
    self->field_0x29648 = 16;
    self->field_0x2964C = -1;
    self->field_0x29650 = -1;
    self->field_0x29654 = 0;
    copyVec3(&self->field_0x29620, pos);
    self->field_0x2962C = self->field_0x29620;
}

/* --- 0x800DC348 -------------------------------------------------------------------------------- */

/* Requests the trap's sound; the id pair depends on the loudness flag. */
extern "C" void fn_800DC348(nw4r::math::VEC3* pos, s32 kind) {
    s32 loud = (fn_800D8D8C(pos) == 1);
    s32 id;

    switch (kind) {
    case 0:
        if (loud == 0) {
            id = 110;
        } else {
            id = 210;
        }
        break;
    case 1:
        if (loud == 0) {
            id = 111;
        } else {
            id = 211;
        }
        break;
    case 2:
        if (loud == 0) {
            id = 112;
        } else {
            id = 212;
        }
        break;
    }
    fn_800DA72C(0, id, pos);
}

/* --- 0x800DC560 -------------------------------------------------------------------------------- */

/* Requests the trap's sound; the id pair depends on the loudness flag. */
extern "C" void fn_800DC560(nw4r::math::VEC3* pos, s32 kind) {
    s32 loud = (fn_800D8D8C(pos) == 1);
    s32 id;

    switch (kind) {
    case 0:
        if (loud == 0) {
            id = 113;
        } else {
            id = 213;
        }
        break;
    case 1:
        if (loud == 0) {
            id = 114;
        } else {
            id = 214;
        }
        break;
    case 2:
        if (loud == 0) {
            id = 115;
        } else {
            id = 215;
        }
        break;
    }
    fn_800DA72C(0, id, pos);
}

/* --- 0x800DC6D8 -------------------------------------------------------------------------------- */

/* Requests a positional sound and marks the resulting slot as a fixed one. */
extern "C" void fn_800DC6D8(nw4r::math::VEC3* pos, s32 kind) {
    if (fn_800D8D8C(pos) == 1) {
        return;
    }
    s32 id;

    switch (kind) {
    case 0:
        id = 92;
        break;
    case 1:
        id = 93;
        break;
    case 2:
        id = 94;
        break;
    }
    SeSlot* slot = fn_800DA72C(0, id, pos);
    if (slot != NULL) {
        slot->param = 8;
    }
}

/* --- 0x800DCAAC -------------------------------------------------------------------------------- */

/* Requests the fixed system sound, preferring the streaming variant when the game mode allows it. */
extern "C" void fn_800DCAAC(nw4r::math::VEC3* pos) {
    u32 id = 0x10;
    id |= 0x80000000;

    if (GameMode_ck() == 2) {
        fn_800DBE94(0, id, pos);
    } else {
        fn_800DA72C(0, id, pos);
    }
}

/* --- 0x800DCB18 -------------------------------------------------------------------------------- */

/* Requests a positional sound on the given work object, louder when the listener is close. */
extern "C" void fn_800DCB18(_se_w* se, nw4r::math::VEC3* pos) {
    u32 id = (fn_800D8D8C(pos) == 1) + 47;
    se_req_pos_ps(se, id, 2, pos);
}

/* --- 0x800DCB74 -------------------------------------------------------------------------------- */

/* Requests a framed positional sound; the id pair depends on the loudness flag. */
extern "C" void fn_800DCB74(_se_w* se, s32 kind, nw4r::math::VEC3* pos) {
    s32 loud = (fn_800D8D8C(pos) == 1);
    s32 id;

    switch (kind) {
    case 0:
        if (loud == 0) {
            id = 43;
        } else {
            id = 45;
        }
        break;
    case 1:
        if (loud == 0) {
            id = 44;
        } else {
            id = 46;
        }
        break;
    }
    fn_800D9DB4(se, id, -1, 2, -1);
}


/* Part `c` of the unit: the SE-request wrappers from fn_800DAE48 to fn_800DCF0C (all 17 symbols at
 * 100 %).
 *
 * Every function here ends in a request to the positional SE entry point
 * `se_req_pos_ps(_se_w*, id, arg, pos)` (or, for the two `fn_800DA*` starters, in the effect-start pair
 * `fn_800E80DC`/`fn_800E8354`). The mangled map names are written as the C++ functions they encode.
 *
 * Residual (reloc-level only, no instruction differs): the `(f32)value` idiom's 2^52 double is the local
 * pool entry `@64` here while the target references the external `lbl_80796400`, because this unit's
 * `.sdata2` range is claimed by another unit in `splits.txt`. Two source shapes are load-bearing and must
 * not be "tidied": `fn_800DAE48` returns `u32` with an explicit `(u8)` cast (the caller then narrows once),
 * and `fn_800F2680` returns `u8` so its callers narrow at the use.
 *
 * Names declared here that a sibling part may also declare (dedupe on merge): `_PLW`, `SeMgrState`,
 * `lbl_80794978`, `lbl_807963E4`, `lbl_80791440`, `lbl_80791444`, `ran_suu`, `get_chacha_*_bank`,
 * `se_req_pos_ps`, `shell_se_req`.
 */

extern "C" u32 fn_800D843C(void);
extern "C" u8 fn_800F2680(s32 id, s32 kind);
extern "C" void fn_800D9CC8(_se_w* work, s32 id, u32 param, s32 value);

void se_req_pos_ps(_se_w* work, s32 id, s32 arg, nw4r::math::VEC3* pos);
void shell_se_req(_se_w* work, nw4r::math::VEC3* pos, u8 id, u32 arg);
s32 ran_suu(s32 range);
s32 get_chacha_dance1_bank(void);
s32 get_chacha_dance2_bank(void);
s32 get_chacha_kamen_bank(void);

/* Pooled constants owned by another unit of the same library (playbook 29: declare, never define). The
 * `[4]` bound is load-bearing: an unknown-size `extern u8 x[]` makes MWCC emit `lis`/`addi` (ADDR16)
 * instead of the `li ...@sda21` the target uses. The values are from the shared runtime dump. */
extern f32 lbl_807963E4;   /* 127.0f - the volume scale */
extern u8 lbl_80791440[4]; /* { 43, 30, 234, 236 } */
extern u8 lbl_80791444[4]; /* { 44, 31, 235, 237 } - the same table, one id louder */

/* The object the two `fn_800DAE*` starters reset: `lbl_80794978` is an `.sbss` pointer to it and the
 * 16-byte state block they clear sits at +0x295E4. Only that block and the handle at +0x295F0 are
 * reached from this unit, so the rest is padding.
 * size: 0x295F4 (lower bound) */
struct SeMgrState {
    /* +0x00000 */ u8 pad_0x00000[0x295E4];
    /* +0x295E4 */ u32 field_0x295E4;
    /* +0x295E8 */ u32 field_0x295E8;
    /* +0x295EC */ u8 field_0x295EC;
    /* +0x295ED */ u8 field_0x295ED;
    /* +0x295EE */ u8 field_0x295EE;
    /* +0x295EF */ u8 field_0x295EF;
    /* +0x295F0 */ s32 field_0x295F0;
};

extern "C" _se_w* lbl_80794978;

/* A player work object; the SE work pointer is the only field this unit reaches.
 * size: 0xB00 (lower bound) */

/* The id of the SE work object slot 1 is holding, or 0 while SE is disabled or the work is missing. */
extern "C" u32 fn_800DAE48(void) {
    if (fn_800D843C() == 1) {
        return 0;
    }
    SeWork* work = get_move_work_adrs__FUc(0);
    if (work == NULL) {
        return 0;
    }
    return (u8)work->se[1]->field_0x0C;
}

/* Resets the manager's effect state and starts the effect `kind` 15 on the current id. */
extern "C" void fn_800DAE9C(void) {
    SeMgrState* mgr = (SeMgrState*)lbl_80794978;
    mgr->field_0x295E4 = 0;
    mgr->field_0x295E8 = 0;
    mgr->field_0x295EC = 0;
    mgr->field_0x295ED = 0;
    mgr->field_0x295EE = 0;
    mgr->field_0x295EF = 0;
    mgr->field_0x295F0 = 0;
    u8 id = fn_800DAE48();
    if (id != 0) {
        s32 handle = fn_800E80DC_c1(id, 15, 0);
        u8 value = fn_800F2680(id, 15);
        fn_800E8354_c1(handle, (s16)(lbl_807963E4 * (f32)value));
        mgr->field_0x295F0 = handle;
    }
}

/* Same as fn_800DAE9C for effect `kind` 10. */
extern "C" void fn_800DAF70(void) {
    SeMgrState* mgr = (SeMgrState*)lbl_80794978;
    mgr->field_0x295E4 = 0;
    mgr->field_0x295E8 = 0;
    mgr->field_0x295EC = 0;
    mgr->field_0x295ED = 0;
    mgr->field_0x295EE = 0;
    mgr->field_0x295EF = 0;
    mgr->field_0x295F0 = 0;
    u8 id = fn_800DAE48();
    if (id != 0) {
        s32 handle = fn_800E80DC_c1(id, 10, 0);
        u8 value = fn_800F2680(id, 10);
        fn_800E8354_c1(handle, (s16)(lbl_807963E4 * (f32)value));
        mgr->field_0x295F0 = handle;
    }
}

/* Requests the gun's "kakusan" hit sound on the player's SE work, id 26 or 46 by `heavy`. */
void gun_kakusan_hitSE_req(_PLW* self, nw4r::math::VEC3* pos, u8 heavy) {
    if (self == NULL) {
        return;
    }
    _se_w* work = self->field_0xAFC;
    if (work == NULL) {
        return;
    }
    s32 id = (heavy == 0) ? 26 : 46;
    se_req_pos_ps(work, id, 2, pos);
}

/* Requests the barrel-put sound; `type` picks id 13 or 15. */
void taru_put_se_req(nw4r::math::VEC3* pos, s32 type) {
    if (fn_800D8D8C(pos) == 1) {
        return;
    }
    s32 id;
    switch (type) {
    case 0:
        id = 13;
        break;
    case 1:
        id = 15;
        break;
    }
    fn_800DA72C(0, id, pos);
}

/* Requests the em015 electric effect sound; `c` indexes the per-channel id table. */
void em015_denki_eft_se_req(_se_w* work, nw4r::math::VEC3* pos, u8 c) {
    u8* table = (fn_800D8D8C(pos) == 1) ? lbl_80791444 : lbl_80791440;
    se_req_pos_ps(work, table[c], 2, pos);
}

/* Requests the em015 electric-ball sound, id `a` (200 higher on the louder channel). */
void em015_denkiball_se_req(_se_w* work, s32 a, s32 b, nw4r::math::VEC3* pos) {
    if (fn_800D8D8C(pos) == 1) {
        a += 200;
    }
    se_req_pos_ps(work, a, b, pos);
}

/* Requests a coin-flip SE, id 9 or 13 by the low bit of the random draw. */
extern "C" void fn_800DC9A4(_se_w* work, nw4r::math::VEC3* pos) {
    s32 id;
    if ((u16)ran_suu(0) & 1) {
        id = 9;
    } else {
        id = 13;
    }
    se_req_pos_ps(work, id, 2, pos);
}

/* Requests the NPC boomerang sound, one id higher on the louder channel. */
void ainpc_boomerang_se_req(_se_w* work, nw4r::math::VEC3* pos) {
    u32 id = 36 + (fn_800D8D8C(pos) == 1);
    shell_se_req(work, pos, (u8)id, 0);
}

/* Starts the Chacha dance sound bank; `kind` 7 takes both dance banks, anything else the sound the
 * `fn_800EF56C` lookup returns for `kind`. */
extern "C" void fn_800DCC24(_se_w* work, s32 kind, u8 c) {
    if (kind == 7) {
        u32 bank = (get_chacha_dance1_bank() << 24) | 3;
        fn_800D9CC8(work, 7, bank, 4);
        fn_800D9CC8(work, 8, bank, 4);
        bank = (get_chacha_dance2_bank() << 24) | 3;
        fn_800D9CC8(work, 7, bank, 4);
        fn_800D9CC8(work, 8, bank, 4);
    } else {
        s32 id = fn_800EF56C(c, kind);
        if (id != -1) {
            fn_800D9CC8(work, 0, (id << 24) | 3, 4);
        }
    }
}

/* Requests the Chacha mask sound, id 10 or 13 by `flag`. */
extern "C" void fn_800DCCF8(_se_w* work, nw4r::math::VEC3* pos, u8 flag) {
    s32 bank = get_chacha_kamen_bank();
    s32 id = (flag == 0) ? 10 : 13;
    se_req_pos_ps(work, id, (bank << 24) | 2, pos);
}

/* The six fixed-id Chacha mask sound requests. */
extern "C" void fn_800DCD68(_se_w* work, nw4r::math::VEC3* pos) {
    s32 bank = get_chacha_kamen_bank();
    se_req_pos_ps(work, 1, (bank << 24) | 2, pos);
}

extern "C" void fn_800DCDBC(_se_w* work, nw4r::math::VEC3* pos) {
    s32 bank = get_chacha_kamen_bank();
    se_req_pos_ps(work, 0, (bank << 24) | 2, pos);
}

extern "C" void fn_800DCE10(_se_w* work, nw4r::math::VEC3* pos) {
    s32 bank = get_chacha_kamen_bank();
    se_req_pos_ps(work, 16, (bank << 24) | 2, pos);
}

extern "C" void fn_800DCE64(_se_w* work, nw4r::math::VEC3* pos) {
    s32 bank = get_chacha_kamen_bank();
    se_req_pos_ps(work, 13, (bank << 24) | 2, pos);
}

extern "C" void fn_800DCEB8(_se_w* work, nw4r::math::VEC3* pos) {
    s32 bank = get_chacha_kamen_bank();
    se_req_pos_ps(work, 17, (bank << 24) | 2, pos);
}

extern "C" void fn_800DCF0C(_se_w* work, nw4r::math::VEC3* pos) {
    s32 bank = get_chacha_kamen_bank();
    se_req_pos_ps(work, 12, (bank << 24) | 2, pos);
}


/* part d - the three per-frame drivers of auto/800D7F54_fn_800D7F54:
 *   0x800D80B8  the camera/enemy-work pass       100.000
 *   0x800D9804  the enemy/SE bank dispatcher      99.977
 *   0x800DC0A8  the per-area SE pass              99.598
 *
 * Residuals:
 *   - 0x800D9804: the `enemy == NULL && kind != 8` guard compiles to `beq <switch dispatch>` where the
 *     target has `beq <case-8 body>` (the target's compiler threaded the branch through the jump table,
 *     or the original source used a goto into the arm). Same instruction count, one branch target.
 *   - 0x800DC0A8: in the second table lookup the target keeps the pair pointer and the selected table
 *     in r25/r3 where we use r23/r0 (six instructions, scratch registers only).
 *
 * Shared-type notes for the orchestrator:
 *   - the prelude's `_se_w` models the *pool entry* (0xA48, 32 `SeSlot` at +0x3C); the *container*
 *     `lbl_80794978` points at is `SePool` here (64 `SeEntry` at +0x34, view matrices at +0x29280, one
 *     trailing `SeSlot` at +0x2961C).
 *   - `em_area_ck` must return a *signed* value (`cmpwi r3,0`); e.cpp/manual.cpp declare it `u32`.
 *   - the camera helpers return their `VEC3`/`Mtx` by value (`get_camera_pos__Fv` has no parameters),
 *     and `copyVec3(&dst, &get_camera_pos())` - taking the address of the returned temporary - is the
 *     only form that reproduces the target (a named local gets an extra copy).
 *   - `fn_800D8DDC` is declared exactly as a.cpp defines it (`extern "C" u8* fn_800D8DDC(u8*)`).
 *
 * Nothing here is committed; the orchestrator owns the unit. */

/* --- local types ------------------------------------------------------------------------------- */

/* The engine's enemy work object; only its address is ever passed on from this part (the map's
 * `em_act_ck__FP11_ENEMY_WORKUcUc`/`em_area_ck__FP11_ENEMY_WORK` spell the class this way). */
struct _ENEMY_WORK;
/* A 3x4 float matrix is `Mtx34` from `nw4r/math.h` (the SDK's `Mtx`): the camera helpers copy one into
 * each of the pool's two view matrices. */

/* The `_ENEMY_WORK` bytes this part reads (the enemy units own the rest of the object): the byte at
 * +0x03 picks the enemy's SE bank and the byte at +0x08 is its own sound code.
 * size: 0x09 (partial view) */
struct EnemyWorkView {
    /* +0x00 */ u8 pad_0x00[3];
    /* +0x03 */ u8 field_0x03;
    /* +0x04 */ u8 pad_0x04[4];
    /* +0x08 */ u8 field_0x08;
};

/* One of the 64 enemy-work records the SE pool carries at +0x34 (0xA48 bytes each): `fn_800D8DDC`
 * claims a clear one, the per-frame driver walks them and dispatches on `kind`, and the callback at
 * +0xA44 is invoked with the record's own work pointer and kind. The 32 sound slots live inside the
 * record at +0x3C (see `SeSlot`).
 * size: 0xA48 */
struct SeEntry {
    /* +0x000 */ u8 in_use;
    /* +0x001 */ u8 state;
    /* +0x002 */ u8 pad_0x002[2];
    /* +0x004 */ _ENEMY_WORK* enemy;
    /* +0x008 */ s32 kind;
    /* +0x00C */ s32 field_0x0C;
    /* +0x010 */ u8 pad_0x010[8];
    /* +0x018 */ nw4r::math::VEC3 field_0x018;
    /* +0x024 */ nw4r::math::VEC3 field_0x024;
    /* +0x030 */ u8 pad_0x030[0x0C];
    /* +0x03C */ nw4r::math::VEC3 field_0x03C;
    /* +0x048 */ u8 pad_0x048[0x9FC];
    /* +0xA44 */ void (*callback)(_ENEMY_WORK*, s32);
};

/* The SE pool object `lbl_80794978` points at: the camera state the driver refreshes, the 64 enemy-work
 * records, the two view matrices, and one extra `SeSlot` at the very end (the record `fn_800D7F54`
 * clears 0x2966C bytes of).
 * size: 0x2966C */
struct SePool {
    /* +0x00000 */ u8 pad_0x00000[4];
    /* +0x00004 */ nw4r::math::VEC3 field_0x004;
    /* +0x00010 */ nw4r::math::VEC3 field_0x010;
    /* +0x0001C */ nw4r::math::VEC3 field_0x01C;
    /* +0x00028 */ nw4r::math::VEC3 field_0x028;
    /* +0x00034 */ SeEntry entries[64];
    /* +0x29234 */ u8 pad_0x29234[0x49];
    /* +0x2927D */ u8 field_0x2927D;
    /* +0x2927E */ u8 pad_0x2927E[2];
    /* +0x29280 */ Mtx34 field_0x29280;
    /* +0x292B0 */ Mtx34 field_0x292B0;
    /* +0x292E0 */ u8 pad_0x292E0[0x33C];
    /* +0x2961C */ SeSlot field_0x2961C;
};

/* The `_PLW` bytes this part reads: the actor kind at +0x02, its motion/state byte at +0x08, and the
 * `_se_w` work object it owns at +0xAFC.
 * size: 0xB00 (partial view) */
struct PlWorkView {
    /* +0x000 */ u8 pad_0x000[2];
    /* +0x002 */ u8 field_0x002;
    /* +0x003 */ u8 pad_0x003[5];
    /* +0x008 */ u8 field_0x008;
    /* +0x009 */ u8 pad_0x009[0xAF3];
    /* +0xAFC */ _se_w* se_work;
};

/* The third argument of the per-area pass: the enemy's SE bank at +0x0E and the bank-table selector
 * byte at +0x4D.
 * size: 0x4E (partial view) */
struct SeReqParamView {
    /* +0x00 */ u8 pad_0x00[0x0E];
    /* +0x0E */ u8 field_0x0E;
    /* +0x0F */ u8 pad_0x0F[0x3E];
    /* +0x4D */ u8 field_0x4D;
};

/* One record of the per-kind "move work" table `get_move_work_adrs` returns; the per-frame driver
 * indexes it by `my_player_no()` (one record per actor) and copies its position into the pool.
 * size: 0xB20 */
struct SeMoveWork {
    /* +0x000 */ u8 pad_0x000[0x3C];
    /* +0x03C */ nw4r::math::VEC3 pos;
    /* +0x048 */ u8 pad_0x048[0xAD8];
};

/* --- declarations ------------------------------------------------------------------------------ */

extern "C" _se_w* lbl_80794978;

extern "C" u8 GameMode_ck(void);
/* Owned by `ef/system_core.cpp` (rule 2).  The declaration stays here, in the block form, because this
 * unit's view is `s32` where the owner's is `u32` and the return type is load-bearing (the two call
 * sites here narrow it with `(s8)`/`(u8)` before indexing the move-work record). */
extern "C" {
s32 my_player_no(void);
}
extern "C" void my_player_no_set(s32 value);
extern "C" u8 fn_802EED0C(SeMoveWork* work);
/* The target object references this callee by its C++ mangling
 * (em_act_ck__FP11_ENEMY_WORKUcUc), so it is C++ - the extern "C" here was the
 * defect (relocaudit). */
u32 em_act_ck(_ENEMY_WORK* enemy, u8 a, u8 b);
extern "C" void fn_800D84E8(_se_w* work);
extern "C" void fn_800D8678(_se_w* work);
extern "C" void fn_800D8730(_se_w* work);
extern "C" void fn_800D85B0(_se_w* work, s32 mode);
extern "C" void fn_800D92E4(_se_w* work, SeSlot* slot);
extern "C" void fn_800D8404(SeSlot* slot);
extern "C" s32 fn_8028F204(void);
extern "C" u8* fn_800D8DDC(u8* base);
extern "C" s32 fn_800F1398(_ENEMY_WORK* enemy, s32 index);
extern "C" u32 fn_80331104(PlWorkView* plw);
extern "C" u8* lbl_80597668[];
extern "C" u8* lbl_805975D8[];
extern "C" u8 lbl_8059768C[];
extern "C" u8 lbl_805975FC[];
extern "C" s32 fn_800F08E0(void);

s32 get_em_se_bank(u8 bank);
u8 PlayMode_ck(void);
nw4r::math::VEC3 get_camera_pos(void);
nw4r::math::VEC3 get_camera_direction(void);
Mtx34 get_current_view_mtx(void);

/* --- 0x800D80B8 -------------------------------------------------------------------------------- */

/* The per-frame SE driver: refreshes the pool's camera state (two view matrices and two position/
 * direction pairs when the play mode asks for it), lets every live enemy-work record push its own
 * position into the pool and dispatch to its kind's slot walker, then runs the pool's trailing slot.
 */
extern "C" void fn_800D80B8(void) {
    SePool* self = (SePool*)lbl_80794978;
    s32 i;
    SeEntry* entry;

    if (GameMode_ck() == 2) {
        lb_npc_func.field_0x04();
    }
    self->field_0x2927D = 0;

    SeMoveWork* work = (SeMoveWork*)get_move_work_adrs__FUc(2);
    if (work != NULL) {
        work += (s8)my_player_no();
        if (GameMode_ck() == 1) {
            self->field_0x2927D = fn_802EED0C(work);
        }
    }

    if (self != NULL) {
        if (PlayMode_ck() == 2) {
            u8 idx = (u8)my_player_no();

            my_player_no_set(0);
            copyVec3(&self->field_0x004, &get_camera_pos());
            copyVec3(&self->field_0x010, &get_camera_direction());
            fn_800532DC(&self->field_0x29280, &get_current_view_mtx());
            fn_800E8498_c1(&self->field_0x29280, 0);

            my_player_no_set(1);
            copyVec3(&self->field_0x01C, &get_camera_pos());
            copyVec3(&self->field_0x028, &get_camera_direction());
            fn_800532DC(&self->field_0x292B0, &get_current_view_mtx());
            fn_800E8498_c1(&self->field_0x292B0, 1);

            my_player_no_set((s8)idx);
        } else {
            copyVec3(&self->field_0x004, &get_camera_pos());
            copyVec3(&self->field_0x010, &get_camera_direction());
            copyVec3(&self->field_0x01C, &self->field_0x004);
            copyVec3(&self->field_0x028, &self->field_0x010);
            fn_800532DC(&self->field_0x29280, &get_current_view_mtx());
            fn_800E8498_c1(&self->field_0x29280, 0);
            fn_800E8498_c1(NULL, 1);
            if (work != NULL) {
                copyVec3(&self->field_0x01C, &work->pos);
            }
        }

        entry = self->entries;
        for (i = 0; i < 64; i++, entry++) {
            if (entry->in_use == 0) {
                continue;
            }
            if (entry->enemy == NULL) {
                continue;
            }
            if (entry->kind == 4 && entry->callback != NULL &&
                em_act_ck(entry->enemy, 12, 255) == 1) {
                continue;
            }
            if (entry->callback != NULL) {
                entry->callback(entry->enemy, entry->kind);
            }
            copyVec3(&entry->field_0x018, &self->field_0x004);
            copyVec3(&entry->field_0x024, &self->field_0x01C);
            switch (entry->kind) {
            case 0:
                break;
            case 1:
            case 2:
            case 3:
                fn_800D84E8((_se_w*)entry);
                break;
            case 4:
                fn_800D8678((_se_w*)entry);
                break;
            case 5:
            case 6:
            case 7:
            case 13:
            case 14:
                fn_800D8730((_se_w*)entry);
                break;
            case 9:
            case 11:
                fn_800D85B0((_se_w*)entry, 0);
                break;
            case 10:
            case 12:
                fn_800D85B0((_se_w*)entry, 1);
                break;
            }
        }

        SeSlot* slot = &self->field_0x2961C;
        if (slot->in_use == 1) {
            fn_800D92E4(NULL, slot);
            fn_800D8404(slot);
        }
        if (fn_8028F204() == 0) {
            fn_800DFDCC(self, 0);
        }
    }
}

/* --- 0x800D9804 -------------------------------------------------------------------------------- */

/* Queues one enemy sound request: claims a pool record for the requested kind and fills in the enemy,
 * the kind and the kind's own sound code (from the enemy's bank, from a frame/state helper, or from a
 * fixed code), then returns the record.
 */
extern "C" SeEntry* fn_800D9804(s32 kind, _ENEMY_WORK* enemy,
                                void (*callback)(_ENEMY_WORK*, s32)) {
    SeEntry* entry;
    _se_w* pool = lbl_80794978;
    EnemyWorkView* enemy_view = (EnemyWorkView*)enemy;
    s32 value;

    if (pool == NULL) {
        return NULL;
    }
    if (enemy == NULL && kind != 8) {
        return NULL;
    }

    switch (kind) {
    case 0:
        break;
    case 1:
        entry = (SeEntry*)fn_800D8DDC((u8*)pool);
        if (entry == NULL) {
            break;
        }
        entry->enemy = enemy;
        entry->kind = kind;
        entry->field_0x0C = enemy_view->field_0x08 + 20;
        entry->callback = callback;
        entry->state = 2;
        return entry;
    case 2:
        entry = (SeEntry*)fn_800D8DDC((u8*)pool);
        if (entry == NULL) {
            break;
        }
        entry->enemy = enemy;
        entry->kind = kind;
        entry->field_0x0C = 4;
        entry->callback = callback;
        entry->state = 2;
        return entry;
    case 3:
        entry = (SeEntry*)fn_800D8DDC((u8*)pool);
        if (entry == NULL) {
            break;
        }
        entry->enemy = enemy;
        entry->kind = kind;
        entry->callback = callback;
        entry->state = 2;
        if (GameMode_ck() != 2) {
            entry->field_0x0C = enemy_view->field_0x08 * 4 + 24;
        } else {
            entry->field_0x0C = fn_800F08E0();
        }
        return entry;
    case 4:
        value = get_em_se_bank(enemy_view->field_0x03);
        if (value < 0) {
            break;
        }
        entry = (SeEntry*)fn_800D8DDC((u8*)pool);
        if (entry == NULL) {
            break;
        }
        entry->enemy = enemy;
        entry->kind = kind;
        entry->field_0x0C = value;
        entry->callback = callback;
        entry->state = 2;
        return entry;
    case 9:
        entry = (SeEntry*)fn_800D8DDC((u8*)pool);
        if (entry == NULL) {
            break;
        }
        entry->enemy = enemy;
        entry->kind = kind;
        entry->field_0x0C = 28;
        entry->callback = callback;
        entry->state = 2;
        return entry;
    case 11:
        entry = (SeEntry*)fn_800D8DDC((u8*)pool);
        if (entry == NULL) {
            break;
        }
        entry->enemy = enemy;
        entry->kind = kind;
        entry->field_0x0C = 4;
        entry->callback = callback;
        entry->state = 2;
        return entry;
    case 10:
        value = fn_800F1398(enemy, 0);
        if (value < 0) {
            break;
        }
        entry = (SeEntry*)fn_800D8DDC((u8*)pool);
        if (entry == NULL) {
            break;
        }
        entry->enemy = enemy;
        entry->kind = kind;
        entry->field_0x0C = value;
        entry->callback = callback;
        entry->state = 2;
        return entry;
    case 12:
        value = fn_800F1398(enemy, 1);
        if (value < 0) {
            break;
        }
        entry = (SeEntry*)fn_800D8DDC((u8*)pool);
        if (entry == NULL) {
            break;
        }
        entry->enemy = enemy;
        entry->kind = kind;
        entry->field_0x0C = value;
        entry->callback = callback;
        entry->state = 2;
        return entry;
    case 5:
    case 6:
    case 7:
        entry = (SeEntry*)fn_800D8DDC((u8*)pool);
        if (entry == NULL) {
            break;
        }
        entry->enemy = enemy;
        entry->kind = kind;
        switch (kind) {
        case 5:
            entry->field_0x0C = 3;
            break;
        case 6:
            entry->field_0x0C = 2;
            break;
        case 7:
            entry->field_0x0C = 46;
            break;
        }
        entry->callback = callback;
        entry->state = 2;
        return entry;
    case 14:
        entry = (SeEntry*)fn_800D8DDC((u8*)pool);
        if (entry == NULL) {
            break;
        }
        entry->enemy = enemy;
        entry->kind = kind;
        entry->field_0x0C = 3;
        entry->callback = callback;
        entry->state = 2;
        return entry;
    case 13:
        entry = (SeEntry*)fn_800D8DDC((u8*)pool);
        if (entry == NULL) {
            break;
        }
        entry->enemy = enemy;
        entry->kind = kind;
        entry->field_0x0C = 32;
        entry->callback = callback;
        entry->state = 2;
        return entry;
    case 8:
        entry = (SeEntry*)fn_800D8DDC((u8*)pool);
        if (entry == NULL) {
            break;
        }
        entry->enemy = NULL;
        entry->kind = kind;
        entry->field_0x0C = 46;
        entry->callback = NULL;
        entry->state = 2;
        return entry;
    }
    return NULL;
}

/* --- 0x800DC0A8 -------------------------------------------------------------------------------- */

/* The per-area SE pass: looks up the actor's kind in the two per-kind bank tables and claims a slot in
 * the actor's work object for each non-zero entry, once with the actor's own owner id and once with
 * the fixed owner 3, copying the request position into the slot both times.
 */
extern "C" void fn_800DC0A8(_ENEMY_WORK* enemy, PlWorkView* plw, SeReqParamView* params,
                            nw4r::math::VEC3* pos) {
    u8 bank = params->field_0x0E;
    _se_w* work;
    u32 owner;
    u32 louder;
    u32 alt;
    SeSlot* slot;
    u8 id;
    u8 (*table)[2];

    if (bank == 255) {
        return;
    }
    if (enemy != NULL && em_area_ck__FP11_ENEMY_WORK(enemy) == 0) {
        return;
    }
    work = plw->se_work;
    if (work == NULL) {
        return;
    }

    alt = 0;
    if ((u8)(plw->field_0x002 - 4) <= 2) {
        if ((s8)my_player_no() == plw->field_0x008) {
            alt = 1;
        }
    }
    louder = 0;
    if (fn_800D8D8C(pos) == 1) {
        louder = 1;
    }
    owner = work->field_0x0C;

    if (alt == 0) {
        s32 kind = plw->field_0x002;
        if (kind == 8) {
            u8** pair = (u8**)lbl_80597668[kind];
            if (fn_80331104(plw) == 1) {
                table = (u8(*)[2])pair[0];
            } else {
                table = (u8(*)[2])pair[1];
            }
        } else {
            table = (u8(*)[2])lbl_80597668[kind];
        }
    } else {
        table = (u8(*)[2])lbl_8059768C;
    }
    if (table != NULL) {
        if (params->field_0x4D == 10) {
            id = table[5][louder];
        } else {
            id = table[bank][louder];
        }
        if (id != 0) {
            slot = fn_800D8E58(work, id);
            if (slot == NULL) {
                return;
            }
            slot->state = 3;
            slot->kind = 1;
            slot->owner = owner;
            slot->id = id;
            slot->field_0x30 = -1;
            slot->field_0x34 = -1;
            slot->param = 0;
            copyVec3(&slot->pos, pos);
            slot->field_0x10 = slot->pos;
        }
    }

    if (alt == 0) {
        s32 kind = plw->field_0x002;
        if (kind == 8) {
            u8** pair = (u8**)lbl_805975D8[kind];
            if (fn_80331104(plw) == 1) {
                table = (u8(*)[2])pair[0];
            } else {
                table = (u8(*)[2])pair[1];
            }
        } else {
            table = (u8(*)[2])lbl_805975D8[kind];
        }
    } else {
        table = (u8(*)[2])lbl_805975FC;
    }
    if (table == NULL) {
        return;
    }
    id = table[bank][louder];
    if (id == 0) {
        return;
    }
    slot = fn_800D8E58(work, id);
    if (slot == NULL) {
        return;
    }
    slot->state = 3;
    slot->kind = 1;
    slot->owner = 3;
    slot->id = id;
    slot->field_0x30 = -1;
    slot->field_0x34 = -1;
    slot->param = 0;
    copyVec3(&slot->pos, pos);
    slot->field_0x10 = slot->pos;
}


/* part e - the per-slot joint lookup (`fn_800D8EA8`), the per-slot sound-state update (`fn_800D92E4`)
 * and the paralyze-trap request (`paralyzeTrap_SE_req__FUcUcPQ34nw4r4math4VEC3`).
 *
 * Measured with `.pi/scratch/measure_part.py` (fuzzy_match_percent):
 *   fn_800D8EA8                                   100.000  byte-exact (1084 B)
 *   paralyzeTrap_SE_req__FUcUcPQ34nw4r4math4VEC3  100.000  byte-exact (220 B)
 *   fn_800D92E4                                    86.552  1268 B vs 1312 B: the bodies are
 *        instruction-for-instruction equal, but the allocator keeps 5 FPRs where retail keeps 8
 *        (f24/f25/f26): `(f32)` conversions pool their 2^52 double locally (`@410`) where retail
 *        references `lbl_80796400`/`lbl_80796408`, so one double web never exists for us, and the
 *        missing FPRs shift the GPR colouring. Same pooling note as b.cpp's header; reloc names do not
 *        affect the score. Tried: the `sys` global as a local vs a per-site deref (the local is +1.5),
 *        `mode * 100` hoisted vs in-loop (hoisted is worse), ids `s32` vs `u32` (`s32` is +1.2).
 *
 * OPTIMIZER LEVEL 4 IS REQUIRED FOR THIS UNIT'S SWITCHES (`#pragma optimization_level 4`).
 * At level 3 a switch whose case labels form a contiguous run is lowered to two compares
 * (`cmpwi rX,3; blt; cmpwi rX,4; ble`); retail has the fused unsigned form (`addi r0,rX,-3; cmplwi r0,1;
 * ble`). With the pragma the same source reproduces retail's dispatch byte-for-byte - calibrated on
 * `fn_800D8404` (16 target instructions reproduced exactly at level 4, not at level 3, and unchanged by
 * `#pragma peephole on`). This also explains part a's `fn_800D9B6C` residual (96.667 %): its 9/10 range
 * test is the same symptom. The unit is most likely a `-O4,p` object (`-O4,p` also implies
 * `-func_align 16`, which the per-function pragma cannot give) - worth a `cflags` change by the
 * orchestrator; the pragma pair is the per-function fallback and is what this part uses.
 *
 * Source shapes that were load-bearing here:
 *   - `fn_800D8EA8`'s two "the +0xC0 flag must skip the shared +0x80 store" paths are a `do { ... }
 *     while (0)` with a `break`: the target jumps *past* the store into the shared tail, and retail's
 *     one-block-per-store layout comes out of that (a `break` out of the inner switch skips the copy too
 *     and costs 6 instructions per case; a `goto` was not used).
 *   - `paralyzeTrap_SE_req` has no `default:` arm: retail's dispatch falls into the play call (the
 *     target's `b <tail>`), and a `default: return;` adds a second epilogue.
 *
 * Shared-type notes for the orchestrator (the prelude / b.cpp were NOT edited):
 *   - `SeSlot` (and a.cpp's `SeSlotObj`) need `f32` at +0x1C and +0x20 (the two fade volumes),
 *     `u8 field_0x4B`, `s8 field_0x4E` and `u32 field_0x30` (the *second* sound id - `id` at +0x2C is the
 *     first). This part carries `SeSlotView`, which extends the prelude's copy over those bytes.
 *   - `SeSysWork` (b.cpp) needs a `VEC3` at +0x04, a `VEC3` at +0x1C and a `u8` at +0x2927D - this part
 *     carries `SeSysWorkVol` for them (same object; b.cpp's copy pads those offsets).
 *   - `_se_w` needs `VEC3`s at +0x18 and +0x24 (`fn_80050EF4`'s distance vectors); this part carries
 *     `SeWorkVol`.
 *   - `fn_800F2680` is declared `u8` in part c and `u32` in part b (and here): that is a hard
 *     redeclaration conflict in the merged TU. b.cpp's header says `u32` is the measured one.
 */
/* --- shared views (the prelude's `SeSlot`/`_se_w`/`_PLW` model other offsets of the same objects) --- */

/* `SeSlot` with the +0x4F flag byte `fn_800D8EA8` sets; the prelude's copy folds 0x4C-0x4F into
 * `pad_0x4C`. Same object, same layout.
 * size: 0x50 */
struct SeSlotView {
    /* +0x00 */ u8 in_use;
    /* +0x01 */ u8 state;
    /* +0x02 */ u8 kind;
    /* +0x03 */ u8 pad_0x03;
    /* +0x04 */ nw4r::math::VEC3 pos;
    /* +0x10 */ nw4r::math::VEC3 field_0x10;
    /* +0x1C */ f32 field_0x1C;
    /* +0x20 */ f32 field_0x20;
    /* +0x24 */ u8 field_0x24;
    /* +0x25 */ u8 pad_0x25[3];
    /* +0x28 */ u32 owner;
    /* +0x2C */ u32 id;
    /* +0x30 */ u32 field_0x30;
    /* +0x34 */ s32 field_0x34;
    /* +0x38 */ u32 param;
    /* +0x3C */ u32 field_0x3C;
    /* +0x40 */ u8 pad_0x40[0x0B];
    /* +0x4B */ u8 field_0x4B;
    /* +0x4C */ u8 pad_0x4C[2];
    /* +0x4E */ s8 field_0x4E;
    /* +0x4F */ u8 field_0x4F;
};

/* The head of the `_se_w` work object as `fn_800D8EA8` reads it: the owner-kind dispatch word at +0x08,
 * the owner pointer at +0x04 and the +0x30 vector. Same object as the prelude's `_se_w`.
 * size: 0x3C (at least) */
struct SeWorkHead {
    /* +0x00 */ u8 pad_0x000[2];
    /* +0x02 */ u8 field_0x002;
    /* +0x03 */ u8 pad_0x003;
    /* +0x04 */ _PLW* field_0x004;
    /* +0x08 */ s32 field_0x008;
    /* +0x0C */ s32 field_0x00C;
    /* +0x10 */ u8 pad_0x010[0x20];
    /* +0x30 */ nw4r::math::VEC3 field_0x030;
};

/* `MHchar` (the actor's character object, the joint count comes from it) is `pl.h`'s: the view below holds it as raw bytes. */

/* The player/enemy actor's joint block: the `MHchar` at +0x24, the fallback vector at +0x3C and the
 * enemy's fallback vector at +0x188. Same object as the prelude's `_PLW`.
 * size: 0x194 (at least) */
struct PlView {
    /* +0x000 */ u8 field_0x000;
    /* +0x001 */ u8 pad_0x001[2];
    /* +0x003 */ u8 field_0x003;
    /* +0x004 */ u8 pad_0x004[0x20];
    /* +0x024 */ u8 field_0x024[0x18]; /* the `MHchar` base, called through `pl.h`'s view */
    /* +0x03C */ nw4r::math::VEC3 field_0x03C;
    /* +0x048 */ u8 pad_0x048[0x140];
    /* +0x188 */ nw4r::math::VEC3 field_0x188;
};

/* The AI-NPC actor's fallback vector (cases 9/11) and the case-10 actor's. */
/* size: 0x184 */
struct AiNpcView {
    /* +0x000 */ u8 pad_0x000[0x178];
    /* +0x178 */ nw4r::math::VEC3 field_0x178;
};

/* size: 0x17C */
struct Case10View {
    /* +0x000 */ u8 pad_0x000[0x170];
    /* +0x170 */ nw4r::math::VEC3 field_0x170;
};

/* --- external symbols ----------------------------------------------------------------------------- */

extern "C" void fn_8027C064(_PLW* plw, nw4r::math::VEC3* pos);
extern "C" void fn_80385B20(_PLW* plw, u32 joint, nw4r::math::VEC3* pos);

/* The sound-system work object `lbl_80794978` points at, as `fn_800D92E4` reads it (b.cpp calls the same
 * object `SeSysWork`): the two distance vectors `fn_80050EF4` is handed and the byte at +0x2927D.
 * size: 0x2927E (at least) */
struct SeSysWorkVol {
    /* +0x00000 */ u8 pad_0x00000[4];
    /* +0x00004 */ nw4r::math::VEC3 field_0x00004;
    /* +0x00010 */ u8 pad_0x00010[0x0C];
    /* +0x0001C */ nw4r::math::VEC3 field_0x0001C;
    /* +0x00028 */ u8 pad_0x00028[0x29255];
    /* +0x2927D */ u8 field_0x2927D;
};

/* The `_se_w` work object as `fn_800D92E4` reads it: the owner-kind word at +0x08 of the prelude's
 * `SeWorkHead`, plus the two volume vectors at +0x18 and +0x24.
 * size: 0x30 (at least) */
struct SeWorkVol {
    /* +0x00 */ u8 pad_0x000[8];
    /* +0x08 */ s32 field_0x008;
    /* +0x0C */ u8 pad_0x00C[0x0C];
    /* +0x18 */ nw4r::math::VEC3 field_0x018;
    /* +0x24 */ nw4r::math::VEC3 field_0x024;
};

struct _ENEMY_WORK;
struct _AINPC_W;

void pl_get_joint_wpos(_PLW* plw, u32 joint, nw4r::math::VEC3* pos);
void get_joint_wpos_em(_ENEMY_WORK* enemy, u32 joint, nw4r::math::VEC3* pos);
void get_joint_wpos_ai(_AINPC_W* npc, u32 joint, nw4r::math::VEC3* pos);
u32 em_area_ck(_ENEMY_WORK* enemy);

extern "C" u32 stage_water_area_ck(void);
extern "C" u8 GameMode_ck(void);
extern "C" u32 fn_800F26B8(s32 handle, u32 id, s32 which);
extern "C" u32 fn_802BE088(void);
extern "C" void fn_800F2714(s32 handle, u32 id, s32 which, u32 value);
extern "C" u8 fn_800F2680(s32 id, s32 kind);
extern "C" void fn_800F2540(s32 handle, u32 id, u16* out);
extern "C" f32 fn_80050EF4(nw4r::math::VEC3* a, nw4r::math::VEC3* b);
u8 PlayMode_ck(void);
extern "C" u32 event_demo_ck__Fv(void);
extern "C" f32 lbl_807963D8;
extern "C" f32 lbl_807963E0;
extern "C" f32 lbl_807963E8;
extern "C" f32 lbl_807963EC;
extern "C" f32 lbl_807963F0;
extern "C" f32 lbl_807963F4;
extern "C" f32 lbl_807963F8;
extern "C" f32 lbl_80597260[];
extern "C" u8* lbl_805971F8[16];
extern "C" _se_w* lbl_80794978;

/* ---------------------------------------------------------------------------------------------------
 * 0x800D8EA8  the per-slot joint lookup: dispatches on the work object's owner kind and then on the
 * slot's kind, and leaves the resulting position in the slot's +0x04 vector.
 */
#pragma optimization_level 4
extern "C" void fn_800D8EA8(_se_w* work, SeSlot* slot) {
    SeWorkHead& w = *(SeWorkHead*)work;
    SeSlotView& s = *(SeSlotView*)slot;

    switch (w.field_0x008) {
    case 1:
    case 2:
    case 3: {
        _PLW* plw = w.field_0x004;

        switch (s.kind) {
        case 1:
        case 2:
            if (s.field_0x34 < 0) {
                if (s.field_0x34 == -1) {
                    copyVec3(&s.pos, &((PlView*)plw)->field_0x03C);
                }
            } else {
                pl_get_joint_wpos(plw, s.field_0x34, &s.pos);
            }
            s.field_0x10 = s.pos;
            break;
        case 3:
            pl_get_joint_wpos(plw, s.field_0x34, &s.pos);
            s.field_0x10 = s.pos;
            break;
        case 4:
            fn_8027C064(plw, &s.pos);
            s.field_0x10 = s.pos;
            break;
        }
        break;
    }
    case 4: {
        _PLW* plw = w.field_0x004;

        if (((PlView*)plw)->field_0x000 == 0) {
            s.in_use = 0;
            break;
        }
        switch (s.kind) {
        case 1:
        case 2:
            /* A single flag store for the three "keep the slot" paths while the +0xC0 path skips
             * it, which the target encodes as one store block and a branch past it: the one-entry
             * loop is a structured `goto` past the store. */
            do {
                if (w.field_0x002 != 0) {
                    if (((PlView*)plw)->field_0x003 == 9) {
                        if (em_area_ck((_ENEMY_WORK*)plw) == 0) {
                            s.field_0x4F = w.field_0x002 | 0xC0;
                            break;
                        }
                    } else {
                        copyVec3(&s.pos, &w.field_0x030);
                    }
                } else if (s.field_0x34 < 0) {
                    if (s.field_0x34 == -1) {
                        copyVec3(&s.pos, &((PlView*)plw)->field_0x188);
                    }
                } else {
                    s32 joints = ((MHchar*)((PlView*)plw)->field_0x024)->get_joint_num();
                    if ((u32)s.field_0x34 < (u32)joints) {
                        get_joint_wpos_em((_ENEMY_WORK*)plw, s.field_0x34, &s.pos);
                    } else {
                        s.in_use = 0;
                        return;
                    }
                }
                s.field_0x4F = w.field_0x002 | 0x80;
            } while (0);
            s.field_0x10 = s.pos;
            break;
        case 3:
            do {
                if (w.field_0x002 != 0) {
                    if (((PlView*)plw)->field_0x003 == 9) {
                        if (em_area_ck((_ENEMY_WORK*)plw) == 0) {
                            s.field_0x4F = w.field_0x002 | 0xC0;
                            break;
                        }
                    } else {
                        copyVec3(&s.pos, &w.field_0x030);
                    }
                } else {
                    s32 joints = ((MHchar*)((PlView*)plw)->field_0x024)->get_joint_num();
                    if ((u32)s.field_0x34 < (u32)joints) {
                        get_joint_wpos_em((_ENEMY_WORK*)plw, s.field_0x34, &s.pos);
                    } else {
                        s.in_use = 0;
                        return;
                    }
                }
                s.field_0x4F = w.field_0x002 | 0x80;
            } while (0);
            s.field_0x10 = s.pos;
            break;
        }
        break;
    }
    case 9:
    case 11: {
        _PLW* plw = w.field_0x004;

        switch (s.kind) {
        case 1:
        case 2:
            if (s.field_0x34 < 0) {
                copyVec3(&s.pos, &((AiNpcView*)plw)->field_0x178);
            } else {
                get_joint_wpos_ai((_AINPC_W*)plw, s.field_0x34, &s.pos);
            }
            s.field_0x10 = s.pos;
            break;
        case 3:
            get_joint_wpos_ai((_AINPC_W*)plw, s.field_0x34, &s.pos);
            s.field_0x10 = s.pos;
            break;
        }
        break;
    }
    case 10: {
        _PLW* plw = w.field_0x004;

        switch (s.kind) {
        case 1:
        case 2:
            if (s.field_0x34 < 0) {
                copyVec3(&s.pos, &((Case10View*)plw)->field_0x170);
            } else {
                fn_80385B20(plw, s.field_0x34, &s.pos);
            }
            s.field_0x10 = s.pos;
            break;
        case 3:
            fn_80385B20(plw, s.field_0x34, &s.pos);
            s.field_0x10 = s.pos;
            break;
        }
        break;
    }
    case 12:
        if ((u32)(s.field_0x34 + 2) > 2) {
            copyVec3(&s.pos, (nw4r::math::VEC3*)(s.field_0x34 + 0x10));
            s.field_0x10 = s.pos;
        }
        break;
    }
}
#pragma optimization_level 3

/* ---------------------------------------------------------------------------------------------------
 * 0x800DA35C  `paralyzeTrap_SE_req` (map name `paralyzeTrap_SE_req__FUcUcPQ34nw4r4math4VEC3`): picks the
 * trap sound's id from the trap's kind and the actor state and plays it at `pos`.
 */
#pragma optimization_level 4
void paralyzeTrap_SE_req(u8 kind, u8 trap, nw4r::math::VEC3* pos) {
    u32 id;

    switch (trap) {
    case 0:
        if (kind == 1) {
            return;
        }
        id = 75;
        break;
    case 1:
        if (kind == 0) {
            id = 68;
        } else {
            id = 160;
        }
        break;
    case 2:
        if (kind == 0) {
            id = 69;
        } else {
            id = 161;
        }
        break;
    case 3:
        if (kind == 0) {
            id = 76;
        } else {
            id = 176;
        }
        break;
    case 4:
        if (kind == 0) {
            id = 77;
        } else {
            id = 177;
        }
        break;
    case 5:
        id = (kind == 0) ? 56 : 156;
        break;
    /* the original has no `default:` arm: an unknown trap falls into the play call with whatever the id
     * register holds - that is what the target's dispatch branches to (the tail) */
    }
    fn_800DA72C(0, id, pos);
}

/* ---------------------------------------------------------------------------------------------------
 * 0x800D92E4  the per-slot sound-state update: for each of the slot's two sound ids it recomputes the
 * bank/level values from the current sound system mode and then the fade volumes from the listener
 * distance.
 */
#pragma optimization_level 4
extern "C" void fn_800D92E4(_se_w* work, SeSlot* slot) {
    SeWorkVol* w = (SeWorkVol*)work;
    SeSlotView* s = (SeSlotView*)slot;
    SeSysWorkVol* sys = (SeSysWorkVol*)lbl_80794978;
    s32 ids[2];
    u8 levels[2];
    u16 params[2];
    f32 scale;
    u32 mode;
    u32 i;

    ids[0] = s->id;
    ids[1] = s->field_0x30;

    for (i = 0; i < 2; i++) {
        if (ids[i] == -1) {
            continue;
        }
        u32 a = fn_800F26B8(s->owner, ids[i], 0);
        u32 b = fn_800F26B8(s->owner, ids[i], 1);

        scale = lbl_807963D8;
        if (stage_water_area_ck() == 1) {
            if (fn_802BE088() == 1) {
                a = 0;
                if (s->pos.y >= lbl_807963E0) {
                    b = (s32)(lbl_807963E8 * (f32)(u8)b);
                    scale = lbl_807963E8;
                }
                if (sys->field_0x2927D == 1) {
                    mode = 3;
                } else {
                    mode = 2;
                }
            } else {
                b = 0;
                if (s->pos.y < lbl_807963E0) {
                    a = (s32)(lbl_807963EC * (f32)(u8)a);
                    scale = lbl_807963F0;
                }
                if (sys->field_0x2927D == 1) {
                    mode = 1;
                } else {
                    mode = 0;
                }
            }
        } else {
            b = fn_800E0428(lbl_80794978, &s->pos);
            mode = (sys->field_0x2927D == 1);
        }
        fn_800F2714(s->owner, ids[i], 0, (u8)a);
        fn_800F2714(s->owner, ids[i], 1, (u8)b);
    }

    for (i = 0; i < 2; i++) {
        if (ids[i] == -1) {
            continue;
        }
        levels[i] = (u8)fn_800F2680(s->owner, ids[i]);
        fn_800F2540(s->owner, ids[i], params);
        if (params[0] >= 20) {
            params[0] = 19;
        }
        if (params[1] >= 16) {
            params[1] = 15;
        }
        f32 near = lbl_80597260[params[0]];
        u8* volumes = lbl_805971F8[params[1]] + mode * 100;
        f32 dist;

        if (PlayMode_ck() == 2) {
            dist = fn_80050EF4(&w->field_0x018, &s->pos);
            f32 other = fn_80050EF4(&w->field_0x024, &s->pos);
            if (other < dist) {
                dist = other;
            }
        } else if (work == NULL) {
            dist = fn_80050EF4(&sys->field_0x00004, &s->pos);
        } else if (GameMode_ck() == 2 && event_demo_ck__Fv() == 0) {
            if (w->field_0x008 == 12 || w->field_0x008 == 8) {
                dist = fn_80050EF4(&sys->field_0x0001C, &s->pos);
            } else {
                dist = fn_80050EF4(&w->field_0x018, &s->pos);
            }
        } else {
            dist = fn_80050EF4(&w->field_0x018, &s->pos);
        }
        if (dist > near) {
            s->field_0x1C = lbl_807963E0;
            continue;
        }
        s32 index = (s32)(dist / (near / lbl_807963F4));
        u8 volume = volumes[index];

        if (i == 0) {
            s->field_0x1C = lbl_807963F8 * (f32)volume;
            s->field_0x1C = s->field_0x1C * (lbl_807963F8 * (f32)levels[i]) * scale;
            if (s->field_0x4B != 0) {
                s->field_0x1C = s->field_0x1C * (lbl_807963F8 * (f32)s->field_0x4E);
            }
        } else {
            s->field_0x20 = lbl_807963F8 * (f32)volume;
            s->field_0x20 = s->field_0x20 * (lbl_807963F8 * (f32)levels[i]) * scale;
            if (s->field_0x4B != 0) {
                s->field_0x20 = s->field_0x20 * (lbl_807963F8 * (f32)s->field_0x4E);
            }
        }
    }

    if (s->field_0x24 != 0) {
        if (stage_water_area_ck() == 0) {
            s->field_0x20 = lbl_807963E0;
        } else if (s->pos.y >= lbl_807963E0) {
            s->field_0x20 = lbl_807963E0;
        } else {
            s->field_0x1C = lbl_807963E0;
        }
    }
}

#pragma optimization_level 3


/* Functions reconstructed by the orchestrating worker (early addresses of the unit). */

extern "C" u8 system_w[];

extern "C" void fn_800D87B8(SeSlot* slot);
extern "C" void fn_800D8AB0(SeSlot* slot);
extern "C" u8 GameMode_ck(void);
extern "C" u32 event_demo_ck__Fv(void);
extern "C" s32 get_fade_stat__Fl(s32 which);

/* A one-line trampoline into the fade/sound-system helper. */
extern "C" void sound_frame_entry(void) {
    fn_800E8294();
}

/* Dispatches a slot to its per-kind handler by the slot's `kind` byte. */
extern "C" void fn_800D8404(SeSlot* slot) {
    switch (slot->kind) {
    case 1:
        fn_800D87B8(slot);
        break;
    case 2:
        fn_800D8AB0(slot);
        break;
    case 3:
    case 4:
        fn_800D8AB0(slot);
        break;
    default:
        return;
    }
}

/* Non-zero while sound effects must stay silent: no work object, the work object's disable byte set, a
 * `GameMode_ck` state of 3, the system's demo flag, or a fade state of 2. */
extern "C" u32 fn_800D843C(void) {
    SeWork* work = get_move_work_adrs__FUc(0);
    if (work != NULL) {
        if (work->field_0x112 != 0) {
            return 1;
        }
    }
    if (GameMode_ck() == 0 || GameMode_ck() == 3) {
        return 0;
    }
    if (system_w[0x30] != 0) {
        return 1;
    }
    if (event_demo_ck__Fv() == 1) {
        return 0;
    }
    return get_fade_stat__Fl(0) == 2;
}

/* --- the per-frame slot walkers ------------------------------------------------------------------- */

extern "C" void fn_800D8EA8(_se_w* work, SeSlot* slot);
extern "C" void fn_800D92E4(_se_w* work, SeSlot* slot);

/* The frame driver: refreshes the shared scratch vector, then walks the 32 slots and lets each active
 * one advance. */
extern "C" void fn_800D84E8(_se_w* work) {
    nw4r::math::VEC3 scratch;
    VEC3_ctor(&scratch);
    s32 i;
    SeSlot* slot = work->slots;
    for (i = 0; i < 32; i++, slot++) {
        if (slot->in_use == 0) {
            continue;
        }
        switch (slot->state) {
        case 3:
            if (slot->kind == 2) {
                fn_800D8EA8(work, slot);
            }
            /* fall through */
        case 4:
            if ((u8)(slot->kind - 3) <= 1) {
                fn_800D8EA8(work, slot);
            }
            fn_800D92E4(work, slot);
            fn_800D8404(slot);
            break;
        default:
            break;
        }
    }
}

/* The frame driver: refreshes the shared scratch vector, then walks the 32 slots and lets each active
 * one advance. */
extern "C" void fn_800D85B0(_se_w* work, s32 mode) {
    (void)mode;
    nw4r::math::VEC3 scratch;
    VEC3_ctor(&scratch);
    s32 i;
    SeSlot* slot = work->slots;
    for (i = 0; i < 32; i++, slot++) {
        if (slot->in_use == 0) {
            continue;
        }
        switch (slot->state) {
        case 3:
            if (slot->kind == 2) {
                fn_800D8EA8(work, slot);
            }
            /* fall through */
        case 4:
            if ((u8)(slot->kind - 3) <= 1) {
                fn_800D8EA8(work, slot);
            }
            fn_800D92E4(work, slot);
            fn_800D8404(slot);
            break;
        default:
            break;
        }
    }
}

/* Same walk, but only the slots in the two "active" states are advanced. */
extern "C" void fn_800D8730(_se_w* work) {
    nw4r::math::VEC3 scratch;
    VEC3_ctor(&scratch);
    s32 i;
    SeSlot* slot = work->slots;
    for (i = 0; i < 32; i++, slot++) {
        if (slot->in_use != 0 && (u32)(slot->state - 3) <= 1) {
            fn_800D92E4(work, slot);
            fn_800D8404(slot);
        }
    }
}

/* The variant that also runs the per-kind joint check for the two active states. */
extern "C" void fn_800D8678(_se_w* work) {
    s32 i;
    SeSlot* slot = work->slots;
    for (i = 0; i < 32; i++, slot++) {
        if (slot->in_use == 0) {
            continue;
        }
        switch (slot->state) {
        case 3:
            if (slot->kind == 2) {
                fn_800D8EA8(work, slot);
            }
            /* fall through */
        case 4:
            if (slot->kind == 3) {
                fn_800D8EA8(work, slot);
            }
            fn_800D92E4(work, slot);
            fn_800D8404(slot);
            break;
        default:
            break;
        }
    }
}

/* --- the sound-code generator --------------------------------------------------------------------- */

extern "C" s32 ran_suu__Fl(s32 param);

/* Picks one of the three codes from a 3-bit random draw: the caller's first code below `b`, the second
 * inside the `b + d` window, and 0xFFFF past it. */
s32 SE_Code_Make(s32 low_code, s16 low, s32 mid_code, s16 span) {
    s16 draw = ran_suu__Fl(0) & 7;
    if (draw < low) {
        return low_code;
    }
    return (draw < low + span) ? mid_code : 0xFFFF;
}

/* --- the two "get a handle and set its rate" helpers ---------------------------------------------- */

extern "C" f32 lbl_807963E4;
extern "C" s32 fn_800F0C14(s32 owner);
extern "C" u8 fn_800F2680(s32 id, s32 kind);

/* Looks the per-frame bank up, opens a handle and scales its rate by the `lbl_807963E4` factor. */
extern "C" void fn_800DABF0(void) {
    SeWork* work = get_move_work_adrs__FUc(0);
    if (work == NULL) {
        return;
    }
    _se_w* se = work->se[1];
    s32 handle = fn_800F0C14(se->field_0x0C);
    if (handle == -1) {
        return;
    }
    s32 voice = fn_800E80DC_c1(handle, 0, 0);
    if (voice == 0) {
        return;
    }
    fn_800E8354_c1(voice, (s16)(s32)(lbl_807963E4 * (f32)fn_800F2680(handle, 1)));
}

/* The same, but the voice is opened with the alternate argument. */
extern "C" void snd_item_fail_play(void) {
    SeWork* work = get_move_work_adrs__FUc(0);
    if (work == NULL) {
        return;
    }
    _se_w* se = work->se[1];
    s32 handle = fn_800F0C14(se->field_0x0C);
    if (handle == -1) {
        return;
    }
    s32 voice = fn_800E80DC_c1(handle, 1, 0);
    if (voice == 0) {
        return;
    }
    fn_800E8354_c1(voice, (s16)(s32)(lbl_807963E4 * (f32)fn_800F2680(handle, 1)));
}

/* --- the SE work-area initialiser ------------------------------------------------------------------ */

extern "C" f32 lbl_807963D8;
extern "C" f32 lbl_807963DC;
extern "C" f32 lbl_807963E0;
extern "C" void fn_800F0EA0(void);
extern "C" void* memset(void* dst, s32 c, u32 n);
extern "C" _se_w* lbl_80794978;

/* The `system_w` offsets this unit reads. `system_w` is owned by `auto/80040598_fn_80040598.cpp`; its
 * type belongs there, so this is a local view of the two fields this unit touches.
 * size: 0x958 (partial) */
struct SystemWorkView {
    /* +0x000 */ u8 pad_0x000[0x30];
    /* +0x030 */ u8 field_0x030;
    /* +0x031 */ u8 pad_0x031[0x923];
    /* +0x954 */ _se_w* se_work;
};

/* Clears the work area and lays a 16-step ramp into it. */
extern "C" void fn_800D7F54(void) {
    SystemWorkView* sys = (SystemWorkView*)system_w;
    lbl_80794978 = sys->se_work;
    memset(lbl_80794978, 0, 0x2966C);
    f32 value = lbl_807963D8;
    f32 step = lbl_807963DC;
    for (s32 i = 0; i < 16; i++) {
        lbl_80794978->ramp[i] = value;
        value -= step;
    }
    lbl_80794978->field_0x29278 = lbl_807963E0;
    fn_800F0EA0();
}

/* --- the code-driven handle helper ----------------------------------------------------------------- */

extern "C" u32 item_se_ck(void);

/* Switches on the caller's sound code: a few codes are silent, two bump the global SE level, and the
 * rest open a per-frame voice whose rate is scaled like `fn_800DABF0`'s. */
extern "C" void fn_800DAADC(u16 code) {
    SeWork* work = get_move_work_adrs__FUc(0);
    if (work == NULL) {
        return;
    }
    switch (code) {
    case 381:
    case 382:
        return;
    case 110:
    case 111:
        se_slot_req(5);
        return;
    case 139:
        return;
    case 395:
        return;
    }
    if (item_se_ck() == 1) {
        se_slot_req(4);
        return;
    }
    _se_w* se = work->se[1];
    s32 handle = fn_800F0C14(se->field_0x0C);
    if (handle == -1) {
        return;
    }
    s32 voice = fn_800E80DC_c1(handle, 0, 0);
    if (voice == 0) {
        return;
    }
    fn_800E8354_c1(voice, (s16)(s32)(lbl_807963E4 * (f32)fn_800F2680(handle, 0)));
}

/* --- the two-voice crossfade helper ---------------------------------------------------------------- */

extern "C" u32 fn_800DAE48(void);

/* Opens a per-frame voice for the caller's mode, files it in one of the two rotating voice slots and
 * starts the other one (or retunes it) so the two crossfade. */
extern "C" void fn_800DB044(s32 unused, u32 mode) {
    s32 id;
    _se_w* work = lbl_80794978;
    u8 handle;
    work->field_0x295F0 = 0;
    handle = fn_800DAE48();
    if (handle == 0) {
        return;
    }
    id = (u8)mode + ((work->voice_slot & 1) ? 111 : 11);
    if ((u8)mode == 0) {
        s32 voice = fn_800E80DC_c1(handle, id, 0);
        work->voices[work->voice_slot] = voice;
        if (work->voice_slot != work->field_0x295EF) {
            fn_800E8354_c1(work->voices[work->voice_slot], 0);
        } else {
            fn_800E8354_c1(work->voices[work->field_0x295EF],
                        (s16)(s32)(lbl_807963E4 * (f32)fn_800F2680(handle, id)));
        }
        work->voice_slot ^= 1;
        return;
    }
    work->field_0x295F0 = fn_800E80DC_c1(handle, id, 0);
    work->voices[work->field_0x295EF] = 0;
    work->field_0x295EF ^= 1;
    s32 v = work->voices[work->field_0x295EF];
    if (v != 0) {
        if (fn_800E802C_c1(v) == 0) {
            return;
        }
        fn_800E8354_c1(work->voices[work->field_0x295EF],
                    (s16)(s32)(lbl_807963E4 * (f32)fn_800F2680(handle, id)));
    } else {
        if (work->voices[work->field_0x295EF] != 0) {
            fn_800E8354_c1(work->voices[work->field_0x295EF],
                        (s16)(s32)(lbl_807963E4 * (f32)fn_800F2680(handle, id)));
        } else {
            fn_800E8354_c1(work->field_0x295F0,
                        (s16)(s32)(lbl_807963E4 * (f32)fn_800F2680(handle, id)));
        }
    }
}

/* --- the slot state machines ----------------------------------------------------------------------- */

extern "C" void fn_800F27C4(s32 handle, s32 id, u8 param);
extern "C" void fn_800F284C(s32 handle, s32 id);
extern "C" f32 lbl_807963E0;

/* Advances one slot by its state: 3 opens the two voices, 4 keeps them running, 6 stops them. */
extern "C" void fn_800D87B8(SeSlot* slot) {
    switch (slot->state) {
    case 3: {
        if ((s32)slot->param > 0) {
            slot->param = (u32)((s32)slot->param - 1);
            return;
        }
        s32 handle = fn_800F0C14(slot->owner);
        if (handle == -1) {
            slot->in_use = 0;
            return;
        }
        if (slot->field_0x4F == 193) {
            slot->state = 7;
            slot->in_use = 0;
            return;
        }
        if (slot->field_0x24 == 0 && lbl_807963E0 == slot->field_0x1C) {
            slot->state = 7;
            slot->in_use = 0;
            return;
        }
        fn_800F27C4(handle, slot->id, slot->field_0x25);
        slot->field_0x3C = fn_800E81BC_c1(handle, slot->id, &slot->field_0x10, 1);
        if (slot->field_0x3C == 0) {
            slot->state = 7;
        } else {
            if (slot->field_0x4A != 0) {
                fn_800E83CC_c1(slot->field_0x3C, (s16)(fn_800E8444_c1() + slot->field_0x4C));
            }
            fn_800E8354_c1(slot->field_0x3C, (s16)(s32)(lbl_807963E4 * slot->field_0x1C));
            slot->state = 4;
            if (slot->field_0x4F == 129) {
                fn_800F284C(slot->owner, slot->id);
                fn_800E8080_c1(slot->field_0x3C, 1300);
            }
        }
        if (slot->field_0x24 != 0 && slot->field_0x30 != -1) {
            slot->field_0x40 = fn_800E8150_c1(handle, slot->field_0x30, &slot->field_0x10, 1);
            if (slot->field_0x40 != 0) {
                if (slot->field_0x4A != 0) {
                    fn_800E83CC_c1(slot->field_0x40, (s16)(fn_800E8444_c1() + slot->field_0x4C));
                }
                fn_800E8354_c1(slot->field_0x40, (s16)(s32)(lbl_807963E4 * slot->field_0x20));
                if (slot->field_0x4F == 129) {
                    fn_800F284C(slot->owner, slot->field_0x30);
                    fn_800E8080_c1(slot->field_0x40, 1300);
                }
            }
        }
        slot->field_0x4F = 0;
        break;
    }
    case 4: {
        u32 stop = 0;
        if (fn_800E802C_c1(slot->field_0x3C) == 0) {
            slot->in_use = 0;
        } else if (slot->field_0x4F != 193 && fn_800F0C14(slot->owner) != -1) {
            fn_800E8354_c1(slot->field_0x3C, (s16)(s32)(lbl_807963E4 * slot->field_0x1C));
        } else {
            stop = 1;
        }
        if (stop) {
            fn_800E8228_c1(slot->field_0x3C);
            if (slot->field_0x24 != 0) {
                fn_800E8228_c1(slot->field_0x40);
            }
            slot->state = 7;
            break;
        }
        if (slot->field_0x24 != 0 && fn_800E802C_c1(slot->field_0x40) != 0) {
            fn_800E8354_c1(slot->field_0x40, (s16)(s32)(lbl_807963E4 * slot->field_0x20));
        }
        break;
    }
    case 6:
        fn_800E8228_c1(slot->field_0x3C);
        if (slot->field_0x24 != 0) {
            fn_800E8228_c1(slot->field_0x40);
        }
        slot->state = 7;
        break;
    default:
        break;
    }
}

/* The two-state variant of the slot machine (no stop state): 3 opens the voices and 4 keeps them. */
extern "C" void fn_800D8AB0(SeSlot* slot) {
    switch (slot->state) {
    case 3: {
        if ((s32)slot->param > 0) {
            slot->param = (u32)((s32)slot->param - 1);
            return;
        }
        s32 handle = fn_800F0C14(slot->owner);
        if (handle == -1) {
            slot->state = 7;
            slot->in_use = 0;
            return;
        }
        if (slot->field_0x24 == 0 && lbl_807963E0 == slot->field_0x1C) {
            slot->state = 7;
            slot->in_use = 0;
            return;
        }
        fn_800F27C4(handle, slot->id, slot->field_0x25);
        slot->field_0x3C = fn_800E81BC_c1(handle, slot->id, &slot->field_0x10, 1);
        if (slot->field_0x3C == 0) {
            slot->state = 7;
        } else {
            if (slot->field_0x4A != 0) {
                fn_800E83CC_c1(slot->field_0x3C, (s16)(fn_800E8444_c1() + slot->field_0x4C));
            }
            fn_800E8354_c1(slot->field_0x3C, (s16)(s32)(lbl_807963E4 * slot->field_0x1C));
            if (slot->field_0x4F == 129) {
                fn_800F284C(slot->owner, slot->id);
                fn_800E8080_c1(slot->field_0x3C, 1300);
            }
            slot->state = 4;
        }
        if (slot->field_0x24 != 0 && slot->field_0x30 != -1) {
            slot->field_0x40 = fn_800E8150_c1(handle, slot->field_0x30, &slot->field_0x10, 1);
            if (slot->field_0x40 != 0) {
                if (slot->field_0x4A != 0) {
                    fn_800E83CC_c1(slot->field_0x40, (s16)(fn_800E8444_c1() + slot->field_0x4C));
                }
                fn_800E8354_c1(slot->field_0x40, (s16)(s32)(lbl_807963E4 * slot->field_0x20));
                if (slot->field_0x4F == 129) {
                    fn_800F284C(slot->owner, slot->field_0x30);
                    fn_800E8080_c1(slot->field_0x40, 1300);
                }
            }
        }
        slot->field_0x4F = 0;
        break;
    }
    case 4:
        if (fn_800E802C_c1(slot->field_0x3C) == 0) {
            slot->state = 7;
            slot->in_use = 0;
        } else {
            if (fn_800F0C14(slot->owner) == -1) {
                fn_800E8228_c1(slot->field_0x3C);
                if (slot->field_0x24 != 0) {
                    fn_800E8228_c1(slot->field_0x40);
                }
                slot->state = 7;
                slot->in_use = 0;
                return;
            }
            fn_800E8354_c1(slot->field_0x3C, (s16)(s32)(lbl_807963E4 * slot->field_0x1C));
        }
        if (slot->field_0x24 != 0 && fn_800E802C_c1(slot->field_0x40) != 0) {
            fn_800E8354_c1(slot->field_0x40, (s16)(s32)(lbl_807963E4 * slot->field_0x20));
        }
        break;
    default:
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * 0x800DCFEC - the map-dependent SE request (folded in from the old `sound/fn_800DCFEC.cpp` in phase 4).
 * That unit was measured with the project's default optimisation level and `peephole off`; this file's
 * `optimization_level 4` pragma is reset around it so the codegen stays the measured one.
 * ------------------------------------------------------------------------------------------------ */
#pragma optimization_level reset

/* The 3-float engine vector: the shared `nw4r::math::VEC3` layout, reached through the header that
 * owns it (rule 1).  It was a second local definition until the type-fix pass. */
typedef nw4r::math::VEC3 VEC3; /* size: 0xC */

/* The two getters are C++-mangled in the target (`get_now_mapno__Fv`/`get_now_areano__Fv`); they are
 * still unsplit, so the declarations live here as ordinary C++ functions.  The `fn_*` callees are
 * plain (C linkage) in the target, so their declarations are wrapped in `extern "C"`. */
u8 get_now_mapno(void);
u8 get_now_areano(void);
extern "C" {
u32 stage_map_kind_get(u8 mapno);
}

/* This unit's private .sdata2 position pool (0x80796410..0x8079641C), referenced but not defined here. */
extern const f32 lbl_80796410;
extern const f32 lbl_80796414;
extern const f32 lbl_80796418;

/* Requests SE work 46 at the caller's position, with the SE code the current map region and area number
 * select. */
extern "C" void fn_800DCFEC(u32 arg0, VEC3* pos)
{
    u32 m = stage_map_kind_get((u8)get_now_mapno());
    u8 areano = get_now_areano();
    u32 se_code;

    switch ((s32)(u8)m) {
    case 1:
        se_code = (u8)arg0 % 3;
        break;
    case 2:
        if (areano == 6) {
            se_code = (u8)arg0 % 2 + 3;
        } else {
            se_code = 10;
        }
        break;
    case 3:
        switch (areano) {
        case 5:
            se_code = 4;
            break;
        case 9:
            se_code = 5;
            break;
        case 6:
        case 10:
            se_code = (u8)arg0 + 3;
            break;
        default:
            return;
        }
        break;
    case 4:
        if (areano != 1) {
            return;
        }
        se_code = 3;
        break;
    case 5:
        switch (areano) {
        case 2:
        case 4:
            se_code = (u8)arg0 + 3;
            break;
        default:
            return;
        }
        break;
    case 7:
        switch (areano) {
        case 0:
            se_code = (u8)arg0 + 3;
            break;
        case 1:
            se_code = 5;
            break;
        case 2:
            if ((u8)arg0 == 0) {
                VEC3 fixed_pos;

                setVec3(&fixed_pos, lbl_80796410, lbl_80796414, lbl_80796418);
                fn_800DA72C(46, 8, &fixed_pos);
                return;
            }
            se_code = (u8)arg0 + 5;
            break;
        default:
            return;
        }
        break;
    case 8:
        if (areano != 1) {
            return;
        }
        se_code = 3;
        break;
    case 9:
        if (areano != 0) {
            return;
        }
        se_code = 3;
        break;
    case 10:
        if (areano != 1) {
            return;
        }
        se_code = (u8)arg0 + 3;
    }
    fn_800DA72C(46, se_code, pos);
}

/* Non-positional SE: bank 46, id 15. */
extern "C" s32 fn_800DD38C(void)
{
    return fn_800DBB78(46, 15);
}

SeSlot* st_ice_se_req(nw4r::math::VEC3* pos)
{
    return fn_800DA72C(46, 18, pos);
}

SeSlot* st_ice_break_se_req(nw4r::math::VEC3* pos)
{
    return fn_800DA72C(46, 19, pos);
}

/* Positional SE: bank 46, id `30 + (arg & 3)`, at the caller's position broadcast over all three axes. */
extern "C" SeSlot* fn_800DD3B8(u32 arg)
{
    nw4r::math::VEC3 v;
    f32 c = lbl_807963E0;
    setVec3(&v, c, c, c);
    return fn_800DA72C(46, ((u8)arg & 3) + 30, &v);
}
