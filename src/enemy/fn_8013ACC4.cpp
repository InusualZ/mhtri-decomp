/* enemy/fn_8013ACC4.cpp - the enemy user-data command interpreter and its 0x100-entry dispatch table.
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with dumpmap.py lookup:
 * every `fn_` name this file uses is a bare .text entry in config/RMHE08/symbols.txt, and the dump
 * answers only `zz_` placeholders for all three of this range's symbols)
 * `.text` 0x8013ACC4..0x8013BE60 (3 functions), extab 0x8000D414..0x8000D424,
 * extabindex 0x80027D08..0x80027D20.
 *
 * What it is.  `fn_8013ACC4` is the second half of the enemy user-data state machine the unit above
 * (`enemy/fn_80138074.c`, the `ResUserDataAc` accessor) installs and the unit below
 * (`enemy/fn_8013BE60.c`, the parameter interpreter) feeds: it runs one *program* - a byte stream at
 * `work->stream_0x958` - through a 256-entry `switch`, dispatching each command to the handler
 * `enemy/fn_8013BE60.c` owns.  The loop is:
 *
 *     len = fn_80140768(work->stream_0x958);              // the command's length in bytes
 *     if ((u8)len == 0xFF && work->stream_0x958[0] != 0xFF) { end the run; continue; }
 *     switch (*work->stream_0x958++) { case 0x00: ... case 0xFF: ... }
 *     work->stream_0x958 += (u8)len;                       // the shared tail every `break` reaches
 *
 * The table is `jumptable_805A15D0` (256 entries): cases 0x00..0x6C call one handler each (the handler
 * returning `s16` advances the cursor by that delta, the rest just run), `0x10` and `0xFF` share the
 * range's biggest body - a second `switch` over `work->field_0x95C`, the interpreter's *state*
 * (`jumptable_805A15A0`, 12 entries) - `0x39` is an empty case, and 0x6D..0xFE are the default
 * (`fn_80140AF8(work, 0xD, *work->stream_0x958)`).
 *
 * `fn_8013BDE4` is the stream reader the two neighbouring units call through
 * `fn_8013BDE4(&work->stream_0x958, code, &value)`, and `fn_8013BDC8` is the 8-byte record copier the
 * interpreter uses to save and restore the work's `recs_0x9AC` array across a state switch.
 *
 * Language.  C++: the range's own symbols are plain, but one callee is the mangled `ran_suu__Fl`
 * (`u16 ran_suu(long)`), so the file must be C++ for the front-end to reproduce that mangling; the
 * range's own three functions are `extern "C"` so they pair with the map's plain stems (rule 9).
 *
 * Types.  `_ENEMY_WORK` (= `EmWork` here) is reconstructed from this range's own field offsets.  It is
 * a *view*, like the ones `enemy/fn_80138074.c` and `enemy/fn_8013BE60.c` carry: the shared home
 * `include/enemy/ENEMY_WORK.h` cannot be used as-is because its view of this range's own fields
 * disagrees with the disassembly - it calls +0x424 a word of `values_0x3A4[]`, +0x958 an `s32` (this
 * range loads it and dereferences it as the stream cursor), +0x95F/+0x999/+0x99B padding or array
 * bytes (this range reads and writes all three as flags/counters), and +0x9A4 an `EmCmdRec*` (this
 * range reads a record-table pointer at +0x4 of the object 0x9A4 points at, with a 0xC stride).  The
 * outbox asks for those fields to be folded into the header (rule 1's follow-up).
 *
 * Measured result.  `fn_8013ACC4` 99.22865 (4356 B / 4356 B, size exact), `fn_8013BDC8` 100.0,
 * `fn_8013BDE4` 100.0; unit fuzzy_match_percent 99.25466 with 2 of 3 functions byte-identical.  All 256
 * dispatch cases, both state-switch bodies, the pre-loop state snapshot/restore and the exit block are
 * written - nothing is stubbed.
 *
 * Residual (every remaining objdiff row of `fn_8013ACC4`, all of them register names or a spelling):
 *   * retail materialises the loop's two constants as 0 in r15 / 1 in r16, ours as 0 in r16 / 1 in r15
 *     (four `stb` sites); the record copiers' index is r14 in retail and r15 here, which is also why
 *     retail hoists `lis r14, jumptable_805A15D0@ha` out of the loop while ours re-materialises it per
 *     dispatch;
 *   * the `continue` edges: retail branches to the loop's own test (`.text+0x10BC`), ours to the loop
 *     head (`+0x258`) - MWCC proved that path cannot set `finished`.  The other source shape (the exit
 *     test written at the TOP of `for (;;)`, which is what retail's rotated layout implies) measures
 *     97.60 and grows `.text` by 8 B here, so the bottom-test form is the better of the two and is
 *     what is applied;
 *   * the `switch (work->field_0x95C)` range probe: retail `subi r0,r3,3 / cmplwi r0,2 / ble`, ours the
 *     two-sided `cmpwi r0,3 / blt / cmpwi r0,5 / ble` chain (same cases 3,4,5);
 *   * the head's schedule: `bl VEC3_ctor` runs one slot earlier in retail than here.
 *
 * Source shapes worth keeping (each measured):
 *   * a `switch` beats `if/else if` for the inner `case 10` sub-command (retail's compare chain is
 *     `cmpwi 0 / beq body0 / cmpwi 1 / beq body1 / b continue`);
 *   * a branch's polarity follows its textual order - retail emits the *then* block first, so inner
 *     `case 0` is `if (stack_0x961[1] == 0) fn_8013AB6C(); else fn_801408B4();`;
 *   * do not cache a field in a local where retail re-reads it per use: `case 2`'s two
 *     `table_0x9A4 != NULL` blocks (and the `else key = 0;` shape) each reload +0x9A4;
 *   * the local *declaration order* drives MWCC's callee-saved assignment: the order below
 *     (finished, aborted, roll, save380..383, save384, save424, seeded, frames, flag, byte, count, len,
 *     i) is retail's r31, r30, r29, r28..r24, r23, r22, r21, r20, r19 and was worth 99.06 -> 99.23 by
 *     itself;
 *   * a counter declared inside its loop (not at function scope) keeps a callee-saved register free;
 *   * the work's saved-command array is 7 records (0x9AC..0x9E4): `case 0x03`'s flag is at +0x9E4.
 */

#include "types.h"
#include "nw4r/math.h"
#include "enemy/fn_8013ACC4.h"
#include "stage/niku_find.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */

/* One scoped pragma, measured on this unit's own command line: with the -O3 peephole MWCC fuses the
 * interpreter's record index into `clrlslwi` (0x8013AE34's `clrlwi`+`slwi` pair) and folds the
 * `switch (work->field_0x95C)` range test into a two-sided compare chain; retail keeps both unfused.
 * with: fn_8013ACC4 99.23 / fn_8013BDE4 100.00 / fn_8013BDC8 100.00;
 * without: fn_8013ACC4 98.69.  No other symbol moves. */
#pragma peephole off

/* ----------------------------------------------------------------------------------------------- */
/* The enemy work record, as this range's accesses measure it (rule 4: every field carries its        */
/* offset; rule 5: every field is named for what it holds).                                           */
/* ----------------------------------------------------------------------------------------------- */

/* size: 0x0C
 * One record of the table `fn_8013AC08` walks (`work->table_0x9A4->recs_0x04[index]`, stride 0xC). */
struct EmStateRec {
    /* +0x00 */ u8 unused_0x00[0x08];
    /* +0x08 */ u8 field_0x08; /* the sub-command `fn_8013AB74` re-enters the state with */
    /* +0x09 */ u8 pad_0x09;
    /* +0x0A */ u8 pad_0x0A;
    /* +0x0B */ u8 field_0x0B; /* the match key `fn_8013AC08` compares its second argument against */
};

/* size: 0x0C - an approximation: only +0x04 (the record array) is read by this range, and +0x00 is
 * never touched, so the object could be larger. */
struct EmStateTable {
    /* +0x00 */ u8 unused_0x00[0x04];
    /* +0x04 */ EmStateRec* recs_0x04;
};

/* One record of the work's saved-command array `recs_0x9AC` (`fn_8013BDC8` copies one field by field,
 * which is why the two zero bytes at +0x02 are not written).
 * size: 0x08 */
struct EmSaveRec {
    /* +0x00 */ u8 code;
    /* +0x01 */ u8 sub;
    /* +0x02 */ u8 pad_0x02[0x02];
    /* +0x04 */ u32 value;
};

/* size: 0x08 - an approximation: the record `fn_80130DF8` reports, of which this range reads +0x05
 * and +0x06 only. */
struct EmRunRec {
    /* +0x00 */ u8 unused_0x00[0x05];
    /* +0x05 */ u8 field_0x05;
    /* +0x06 */ u8 field_0x06;
    /* +0x07 */ u8 unused_0x07;
};

/* The interpreter's own record: a lower bound of 0xB18 (the tail is the unit's `open_level_no` band,
 * which this range never touches).  See the unit header for why this is a view rather than
 * `include/enemy/ENEMY_WORK.h`.
 * size: 0xB18 */
struct EmWork {
    /* +0x0000 */ u8 unused_0x0000[0x036C];
    /* +0x036C */ nw4r::math::VEC3 vec_0x36C;  /* the position `copyVec3` snapshots with the state */
    /* +0x0378 */ u8 unused_0x0378[0x0380 - 0x0378];
    /* +0x0380 */ u8 field_0x380;  /* the state snapshot's first byte (`fn_8013A0xx` writes it) */
    /* +0x0381 */ u8 field_0x381;  /* the state snapshot's second byte */
    /* +0x0382 */ u8 field_0x382;  /* the state snapshot's third byte; `fn_8013AC08`'s record index */
    /* +0x0383 */ u8 field_0x383;  /* the state snapshot's fourth byte */
    /* +0x0384 */ f32 field_0x384; /* the state snapshot's float */
    /* +0x0388 */ u8 unused_0x0388[0x038E - 0x0388];
    /* +0x038E */ u8 field_0x38E;  /* `case 0x6B` clears it */
    /* +0x038F */ u8 field_0x38F;  /* `case 0x6B` clears it */
    /* +0x0390 */ u8 unused_0x0390[0x0424 - 0x0390];
    /* +0x0424 */ u32 field_0x424; /* the state snapshot's word (`fn_8013A0xx` writes it) */
    /* +0x0428 */ u8 unused_0x0428[0x0798 - 0x0428];
    /* +0x0798 */ u8 field_0x798;  /* the byte the snapshot's `fn_8012B380` command carries */
    /* +0x0799 */ u8 unused_0x0799;
    /* +0x079A */ u8 field_0x79A;  /* nonzero: the interpreter saves the state before it runs */
    /* +0x079B */ u8 unused_0x079B[0x089F - 0x079B];
    /* +0x089F */ u8 field_0x89F;  /* 3: the state-end block takes the 9-entry path */
    /* +0x08A0 */ u8 unused_0x08A0[0x0916 - 0x08A0];
    /* +0x0916 */ s16 field_0x916; /* <= 0: the state-end block's second path */
    /* +0x0918 */ u8 unused_0x0918[0x0954 - 0x0918];
    /* +0x0954 */ u32** field_0x954; /* the state table `fn_8013A884`/`fn_8013AB74` index */
    /* +0x0958 */ u8* stream_0x958;  /* the program cursor this interpreter walks */
    /* +0x095C */ u8 field_0x95C;    /* the interpreter's state (the second switch's key) */
    /* +0x095D */ u8 field_0x95D;    /* the sub-command the state re-enters with */
    /* +0x095E */ u8 unused_0x095E;
    /* +0x095F */ u8 field_0x95F;    /* nonzero: the work is disabled, the interpreter returns 0 */
    /* +0x0960 */ u8 unused_0x0960;
    /* +0x0961 */ u8 stack_0x961[0x0F]; /* the interpreter's byte stack (its depth is stack_0x961[1],
                                        * the value `case 0x10` tests before it re-enters state 0) */
    /* +0x0970 */ u32 values_0x970[10]; /* the words belonging to stack_0x961 */
    /* +0x0998 */ u8 unused_0x0998;
    /* +0x0999 */ u8 field_0x999;    /* the state-2 sub-counter `case 0x10` advances */
    /* +0x099A */ u8 unused_0x099A;
    /* +0x099B */ u8 field_0x99B;    /* set by `case 0x5D` */
    /* +0x099C */ u8 unused_0x099C[0x09A4 - 0x099C];
    /* +0x09A4 */ EmStateTable* table_0x9A4; /* the record table `fn_8013AC08` walks */
    /* +0x09A8 */ u8 count_0x9A8;    /* the count the state switch saves and restores */
    /* +0x09A9 */ u8 unused_0x09A9[0x09AC - 0x09A9];
    /* +0x09AC */ EmSaveRec recs_0x9AC[7]; /* the 8-byte commands the state switch saves (the
                                        * array runs to +0x9E4, the flag `case 0x03` reads) */
    /* +0x09E4 */ u8 field_0x9E4;    /* zero: `case 0x03`..`case 0x07` finishes the run */
    /* +0x09E5 */ u8 unused_0x09E5[0x0B18 - 0x09E5];
};

/* The state snapshot `case 0x10` swaps out and back: the work's position, its saved commands and the
 * count that bounds them.  `fn_8013BDC8` fills and drains it one record at a time.
 * size: 0x40 */
struct EmUserSave {
    /* +0x00 */ nw4r::math::VEC3 vec_0x00;
    /* +0x0C */ u32 unused_0x0C;
    /* +0x10 */ EmSaveRec recs_0x10[6];
};

/* ----------------------------------------------------------------------------------------------- */
/* Callees.  The map spells every one of them as a C symbol, so the whole set - this unit's own three */
/* entry points included - sits in one `extern "C"` block: without it this C++ front-end would mangle */
/* every name and objdiff would pair nothing (docs/matching.md row 42, and the trap              */
/* `enemy/fn_8013F764.cpp` documents).  They are plain prototypes rather than `extern` declarations  */
/* (the band's interim home for another unit's symbol: rule 2 keys on the `extern` keyword, and these */
/* are views of the arity this range's own call sites prove - a handler this range reads `r3` from    */
/* is declared to return `s16` even where its owner's reconstruction calls it `void`).               */
/* ----------------------------------------------------------------------------------------------- */

extern "C" {

/* `enemy/fn_80138074.c` - the user-data accessor this interpreter drives */
u32 fn_8013A900(EmWork* self);
u32 fn_8013A884(EmWork* self, s32 value);
u32 fn_8013AB74(EmWork* self, u32 state, u32 sub);
void fn_8013AACC(EmWork* self, u8 arg);
void fn_8013AB6C(EmWork* self);
void fn_8013AAC4(EmWork* self);
void fn_8013817C(EmWork* self);

/* This unit's own entry points.  The map spells them as C symbols, so they are `extern "C"` here and at
 * their definitions - without it this C++ front-end would mangle them and objdiff would pair nothing
 * (the same trap `enemy/fn_8013F764.cpp` documents).  `fn_8013BDE4` is declared in the owner's header
 * above (its neighbours call it); `fn_8013BDC8` is internal and stays here. */
extern "C" void fn_8013BDC8(EmSaveRec* dst, EmSaveRec* src);

/* `enemy/fn_8013BE60.c` and `enemy/fn_8013F764.cpp` - the per-command handlers.  The `s16` group
 * advances the stream cursor by the delta it returns. */
s16 fn_8013C794(EmWork* self, u8* in, u32 value);
s16 fn_8013CBE4(EmWork* self, u8* in);
s16 fn_8013CC0C(EmWork* self, u8* in);
s16 fn_8013CC34(EmWork* self, u8* in);
s16 fn_8013CC5C(EmWork* self, u8* in);
s16 fn_8013CC84(EmWork* self, u8* in);
s16 fn_8013CCA8(EmWork* self, u8* in);
s16 fn_8013CCD0(EmWork* self, u8* in);
s16 fn_8013CEB0(EmWork* self, u8* in);
s16 fn_8013CF6C(EmWork* self, u8* in);
s16 fn_8013D054(EmWork* self, u8* in);
s16 fn_8013D1E0(EmWork* self, u8* in);
s16 fn_8013D2C8(EmWork* self, u8* in);
s16 fn_8013D2EC(EmWork* self, u8* in);
s16 fn_8013D310(EmWork* self, u8* in);
s16 fn_8013D4C4(EmWork* self, u8* in);
s16 fn_8013D588(EmWork* self, u8* in);
s16 fn_8013D694(EmWork* self, u8* in);
s16 fn_8013D80C(EmWork* self, u8* in);
s16 fn_8013D8C8(EmWork* self, u8* in, u32 param);
s16 fn_8013D990(EmWork* self, u8* in);
s16 fn_8013D9B8(EmWork* self, u8* in);
s16 fn_8013DA7C(EmWork* self, u8* in);
s16 fn_8013DB70(EmWork* self, u8* in);
s16 fn_8013DC58(EmWork* self, u8* in);
s16 fn_8013DD48(EmWork* self, u8* in);
s16 fn_8013DE08(EmWork* self, u8* in);
s16 fn_8013DE30(EmWork* self, u8* in);
s16 fn_8013DE58(EmWork* self, u8* in);
s16 fn_8013DF78(EmWork* self, u8* in);
s16 fn_8013E06C(EmWork* self, u8* in);
s16 fn_8013E180(EmWork* self, u8* in);
s16 fn_8013E2B0(EmWork* self, u8* in);
s16 fn_8013E388(EmWork* self, u8* in);
s16 fn_8013E464(EmWork* self, u8* in);
s16 fn_8013E700(EmWork* self, u8* in);
s16 fn_8013E7C0(EmWork* self, u8* in);
s16 fn_8013E900(EmWork* self, u8* in);
s16 fn_8013E9D4(EmWork* self, u8* in);
s16 fn_8013E9FC(EmWork* self, u8* in);
s16 fn_8013EAE4(EmWork* self, u8* in);
s16 fn_8013EBA0(EmWork* self, u8* in);
s16 fn_8013EBC8(EmWork* self, u8* in);
s16 fn_8013ECA4(EmWork* self, u8* in);
s16 fn_8013ED68(EmWork* self, u8* in);
s16 fn_8013EE2C(EmWork* self, u8* in);
s16 fn_8013EF14(EmWork* self, u8* in);
s16 fn_8013EF3C(EmWork* self, u8* in);
s16 fn_8013EF64(EmWork* self, u8* in);
s16 fn_8013EF8C(EmWork* self, u8* in);
s16 fn_8013EFD4(EmWork* self, u8* in);
s16 fn_8013F5B8(EmWork* self, u8* in);
s16 fn_8013F8D8(EmWork* self, u8* in);
s16 fn_8013F9D8(EmWork* self, u8* in);
s16 fn_8013FA9C(EmWork* self, u8* in);
s16 fn_8013FB08(EmWork* self, u8* in);
s16 fn_8013FB9C(EmWork* self, u8* in);
s16 fn_8013FC60(EmWork* self, u8* in);
s16 fn_8013FD28(EmWork* self, u8* in);
s16 fn_8013FD50(EmWork* self, u8* in);
s16 fn_8013FD74(EmWork* self, u8* in);
s16 fn_8013FD98(EmWork* self, u8* in);
s16 fn_8013FF08(EmWork* self, u8* in);
s16 fn_8013FF38(EmWork* self, u8* in);
s16 fn_8013FF5C(EmWork* self, u8* in);
s16 fn_8014001C(EmWork* self, u8* in);
s16 fn_801400BC(EmWork* self, u8* in);
s16 fn_80140178(EmWork* self, u8* in);
s16 fn_80140244(EmWork* self, u8* in);
s16 fn_8014026C(EmWork* self, u8* in, u32 arg);
s16 fn_80140298(EmWork* self, u8* in);
s16 fn_80140354(EmWork* self, u8* in);
s16 fn_801404B8(EmWork* self, u8* in);
s16 fn_80140580(EmWork* self, u8* in);
s16 fn_80140648(EmWork* self, u8* in);
s16 fn_80140670(EmWork* self, u8* in);

void fn_8013C458(EmWork* self, u8* in);
void fn_8013C57C(EmWork* self, u8* in, u32 flag);
void fn_8013C7E0(EmWork* self, u8* in);
void fn_8013C97C(EmWork* self, u8* in);
void fn_8013C988(EmWork* self, u8* in);
void fn_8013CAA0(EmWork* self, u8* in);
void fn_8013CDB4(EmWork* self, u8* in);
void fn_8013D1D0(EmWork* self, u8* in);
void fn_8013D34C(EmWork* self, u8* in);
void fn_8013D41C(EmWork* self, u8* in);
void fn_8013D45C(EmWork* self, u8* in, u32 flag);
void fn_8013D684(EmWork* self, u8* in);
void fn_8013D750(EmWork* self, u8* in);
void fn_8013DE80(EmWork* self, u8* in);
void fn_8013DED4(EmWork* self, u8* in);
void fn_8013DEFC(EmWork* self, u8* in);
void fn_8013E05C(EmWork* self, u8* in);
void fn_8013E068(EmWork* self, u8* in);
void fn_8013E528(EmWork* self, u8* in);
void fn_8013E6F4(EmWork* self, u8* in);
void fn_8013E9C8(EmWork* self, u8* in);
void fn_8013F994(EmWork* self, u8* in);
void fn_8013FD1C(EmWork* self, u8* in);
void fn_8013FEF4(EmWork* self, u8* in);
void fn_801400B0(EmWork* self, u8* in);
u32 fn_8013F764(EmWork* self, u8* in);
u32 fn_8013C254(EmWork* self);

/* the rest of the enemy band */
u32 fn_80130134(EmWork* self, u32 value);
u32 fn_8013AC00(EmWork* self, u16 index); /* `ran_suu(1)`'s tail: the state's next roll */
u8 fn_8013AC08(EmWork* self, u8 index, u8 key);
void fn_8012A254(EmWork* self, u32 index);
void fn_8012B380(EmWork* self, u32 a, u32 b, u8 c);
void em_target_pos_set(EmWork* self, s32 mode);
u32 fn_8012EC3C(EmWork* self);
u8 fn_801408B4(EmWork* self);
void fn_801409C8(EmWork* self);
u8* fn_80130DF8(EmWork* self);
u32 fn_80130778(EmWork* self, u32 kind);
void fn_80133BC0(EmWork* self);
void fn_80140AF8(EmWork* self, u32 a, u8 b);

/* the stream readers `enemy/fn_8013F764.cpp` owns */
u8 fn_80140768(u8* in);
s16 fn_80140778(u8* in, u8 code, u8 mode);


} /* extern "C" */

/* The one mangled callee.  Declared as the C++ function it is (outside the `extern "C"` block, with no
 * mangled spelling of its own): the front-end emits the map's `ran_suu__Fl` from `u16 ran_suu(long)`
 * (rule 9). */
s32 ran_suu(long index);

/* ----------------------------------------------------------------------------------------------- */
/* 0x8013BDC8 - the 8-byte record copier.                                                             */
/* ----------------------------------------------------------------------------------------------- */

extern "C" void fn_8013BDC8(EmSaveRec* dst, EmSaveRec* src) {
    dst->code = src->code;
    dst->sub = src->sub;
    dst->value = src->value;
}

/* ----------------------------------------------------------------------------------------------- */
/* 0x8013BDE4 - the stream reader: consume one `id` record at `*in`, report its value in `*out` and     */
/* advance the cursor by the record's own length.                                                      */
/* ----------------------------------------------------------------------------------------------- */

extern "C" void fn_8013BDE4(u8** in, u32 id, s16* out) {
    s16 n = fn_80140778(*in, (u8)id, 0);

    *out += n;
    *in += n;
    *out += (s16)((u8)fn_80140768(*in) + 1);
}

/* ----------------------------------------------------------------------------------------------- */
/* 0x8013ACC4 - the interpreter.                                                                      */
/* ----------------------------------------------------------------------------------------------- */

extern "C" u32 fn_8013ACC4(EmWork* self) {
    EmUserSave save;
    u32 finished = 0;
    u32 aborted = 0;
    u32 roll;
    u8 save380, save381, save382, save383;
    f32 save384;
    u32 save424;
    u32 seeded = 0;
    s16 frames = 0;
    u32 flag = 0;
    u8 byte = 0;
    u8 count = 0;
    u8 len;
    u32 i = 0;

    VEC3_ctor(&save.vec_0x00);

    if (self->stream_0x958 == NULL) {
        return 0;
    }
    if (self->field_0x95F != 0) {
        self->field_0x95F--;
        return 0;
    }

    if ((u8)fn_8013A900(self) != 0xc && fn_8013A884(self, 0xc) == 1 && fn_80130134(self, 0) == 1) {
        fn_8013AB74(self, 0xc, 0);
    }

    roll = ran_suu(1);

    if (self->field_0x79A != 0) {
        save380 = self->field_0x380;
        save381 = self->field_0x381;
        save382 = self->field_0x382;
        save383 = self->field_0x383;
        save384 = self->field_0x384;
        save424 = self->field_0x424;

        copyVec3(&save.vec_0x00, &self->vec_0x36C);

        switch (self->field_0x95C) {
        case 3:
        case 4:
        case 5:
            if (self->field_0x798 != 0xff) {
                fn_8012B380(self, 1, 2, self->field_0x798);
            } else {
                fn_8012B380(self, 4, 0, 0);
            }
            break;
        case 6:
            fn_8012B380(self, 4, 0, 0);
            break;
        }

        count = self->count_0x9A8;
        for (i = 0; (u8)i < count; i++) {
            fn_8013BDC8(&save.recs_0x10[(u8)i], &self->recs_0x9AC[(u8)i]);
        }
        self->count_0x9A8 = 0;
    }

    em_target_pos_set(self, 0);

    for (;;) {
        frames++;
        if ((s16)frames > 0x3e8) {
            fn_8013AAC4(self);
        }

        if (self->stream_0x958 == (u8*)self->field_0x954[0][0]) {
            if (self->field_0x916 > 0) {
                if (fn_8013A884(self, 8) == 1) {
                    fn_8013AB74(self, 8, 0);
                }
            } else if (self->field_0x89F == 3 && fn_8013A884(self, 9) == 1) {
                fn_8013AB74(self, 9, 0);
            } else if (seeded == 0) {
                fn_8012A254(self, (u16)roll);
                roll = fn_8013AC00(self, (u16)roll);
                seeded = 1;
            }
        }

        len = fn_80140768(self->stream_0x958);
        if ((u8)len == 0xff && self->stream_0x958[0] != 0xff) {
            fn_8013AAC4(self);
            continue;
        }

        switch (*self->stream_0x958++) {
        case 0x00:
            if (self->field_0x79A != 0) {
                fn_8013AACC(self, 1);
            }
            fn_8013C458(self, self->stream_0x958);
            finished = 1;
            break;
        case 0x01:
            fn_8013C57C(self, self->stream_0x958, flag);
            flag = 0;
            break;
        case 0x02:
            self->stream_0x958 += (s16)fn_8013C794(self, self->stream_0x958, (u16)roll);
            roll = fn_8013AC00(self, (u16)roll);
            break;
        case 0x03:
            fn_8013C7E0(self, self->stream_0x958);
            break;
        case 0x04:
            fn_8013C97C(self, self->stream_0x958);
            break;
        case 0x05:
            fn_8013C988(self, self->stream_0x958);
            break;
        case 0x06:
            fn_8013CAA0(self, self->stream_0x958);
            continue;
        case 0x07:
            self->stream_0x958 += (s16)fn_8013CBE4(self, self->stream_0x958);
            break;
        case 0x08:
            self->stream_0x958 += (s16)fn_8013CC0C(self, self->stream_0x958);
            break;
        case 0x09:
            self->stream_0x958 += (s16)fn_8013CC34(self, self->stream_0x958);
            break;
        case 0x0A:
            self->stream_0x958 += (s16)fn_8013CC5C(self, self->stream_0x958);
            break;
        case 0x0B:
            self->stream_0x958 += (s16)fn_8013CC84(self, self->stream_0x958);
            break;
        case 0x0C:
            self->stream_0x958 += (s16)fn_8013CCA8(self, self->stream_0x958);
            break;
        case 0x0D:
            fn_8013AAC4(self);
            break;
        case 0x0E:
            self->stream_0x958 += (s16)fn_8013CCD0(self, self->stream_0x958);
            break;
        case 0x0F:
            fn_8013CDB4(self, self->stream_0x958);
            break;
        case 0x11:
            self->stream_0x958 += (s16)fn_8013CEB0(self, self->stream_0x958);
            break;
        case 0x12:
            self->stream_0x958 += (s16)fn_8013CF6C(self, self->stream_0x958);
            break;
        case 0x13:
            self->stream_0x958 += (s16)fn_8013D054(self, self->stream_0x958);
            break;
        case 0x14:
            fn_8013D1D0(self, self->stream_0x958);
            continue;
        case 0x15:
            self->stream_0x958 += (s16)fn_8013D1E0(self, self->stream_0x958);
            break;
        case 0x16:
            self->stream_0x958 += (s16)fn_8013D2C8(self, self->stream_0x958);
            break;
        case 0x17:
            self->stream_0x958 += (s16)fn_8013D2EC(self, self->stream_0x958);
            break;
        case 0x18:
            self->stream_0x958 += (s16)fn_8013D310(self, self->stream_0x958);
            break;
        case 0x19:
            fn_8013D34C(self, self->stream_0x958);
            break;
        case 0x1A:
            fn_8013D41C(self, self->stream_0x958);
            break;
        case 0x1B:
            fn_8013D45C(self, self->stream_0x958, flag);
            flag = 0;
            break;
        case 0x1C:
            self->stream_0x958 += (s16)fn_8013D4C4(self, self->stream_0x958);
            break;
        case 0x1D:
            self->stream_0x958 += (s16)fn_8013D588(self, self->stream_0x958);
            break;
        case 0x1E:
            fn_8013D684(self, self->stream_0x958);
            break;
        case 0x1F:
            self->stream_0x958 += (s16)fn_8013D694(self, self->stream_0x958);
            break;
        case 0x20:
            fn_8013D750(self, self->stream_0x958);
            continue;
        case 0x21:
            self->stream_0x958 += (s16)fn_8013D80C(self, self->stream_0x958);
            break;
        case 0x22:
            self->stream_0x958 += (s16)fn_8013D8C8(self, self->stream_0x958, (u16)roll);
            roll = fn_8013AC00(self, (u16)roll);
            break;
        case 0x23:
            self->stream_0x958 += (s16)fn_8013D990(self, self->stream_0x958);
            break;
        case 0x24:
            self->stream_0x958 += (s16)fn_8013D9B8(self, self->stream_0x958);
            break;
        case 0x25:
            self->stream_0x958 += (s16)fn_8013DA7C(self, self->stream_0x958);
            break;
        case 0x26:
            self->stream_0x958 += (s16)fn_8013DB70(self, self->stream_0x958);
            break;
        case 0x27:
            self->stream_0x958 += (s16)fn_8013DC58(self, self->stream_0x958);
            break;
        case 0x28:
            self->stream_0x958 += (s16)fn_8013DD48(self, self->stream_0x958);
            break;
        case 0x29:
            self->stream_0x958 += (s16)fn_8013DE08(self, self->stream_0x958);
            break;
        case 0x2A:
            self->stream_0x958 += (s16)fn_8013DE58(self, self->stream_0x958);
            break;
        case 0x2B:
            fn_8013DE80(self, self->stream_0x958);
            break;
        case 0x2C:
            fn_8013DED4(self, self->stream_0x958);
            break;
        case 0x2D:
            fn_8013DEFC(self, self->stream_0x958);
            break;
        case 0x2E:
            self->stream_0x958 += (s16)fn_8013DF78(self, self->stream_0x958);
            break;
        case 0x2F:
            if (self->field_0x79A != 0) {
                fn_8013AACC(self, 1);
            }
            fn_8013E05C(self, self->stream_0x958);
            finished = 1;
            aborted = 1;
            break;
        case 0x30:
            fn_8013E068(self, self->stream_0x958);
            break;
        case 0x31:
            if (self->field_0x79A != 0) {
                fn_8013AACC(self, 1);
            }
            {
                s16 n = fn_8013E06C(self, self->stream_0x958);
                if (n < 0) {
                    continue;
                }
                self->stream_0x958 += n;
            }
            break;
        case 0x32:
            self->stream_0x958 += (s16)fn_8013E180(self, self->stream_0x958);
            break;
        case 0x33:
            self->stream_0x958 += (s16)fn_8013E2B0(self, self->stream_0x958);
            break;
        case 0x34:
            self->stream_0x958 += (s16)fn_8013E388(self, self->stream_0x958);
            break;
        case 0x35:
            self->stream_0x958 += (s16)fn_8013E464(self, self->stream_0x958);
            break;
        case 0x36:
            fn_8013E528(self, self->stream_0x958);
            break;
        case 0x37:
            fn_8013E6F4(self, self->stream_0x958);
            break;
        case 0x38:
            self->stream_0x958 += (s16)fn_8013E700(self, self->stream_0x958);
            break;
        case 0x39:
            break;
        case 0x3A:
            self->stream_0x958 += (s16)fn_8013E7C0(self, self->stream_0x958);
            if (self->field_0x95C == 0xa) {
                continue;
            }
            break;
        case 0x3B:
            self->stream_0x958 += (s16)fn_8013E900(self, self->stream_0x958);
            break;
        case 0x3C:
            fn_8013E9C8(self, self->stream_0x958);
            break;
        case 0x3D:
            self->stream_0x958 += (s16)fn_8013E9D4(self, self->stream_0x958);
            break;
        case 0x3E:
            self->stream_0x958 += (s16)fn_8013E9FC(self, self->stream_0x958);
            break;
        case 0x3F:
            self->stream_0x958 += (s16)fn_8013EAE4(self, self->stream_0x958);
            break;
        case 0x40:
            self->stream_0x958 += (s16)fn_8013EBA0(self, self->stream_0x958);
            break;
        case 0x41:
            self->stream_0x958 += (s16)fn_8013EBC8(self, self->stream_0x958);
            break;
        case 0x42:
            self->stream_0x958 += (s16)fn_8013ECA4(self, self->stream_0x958);
            break;
        case 0x43:
            self->stream_0x958 += (s16)fn_8013ED68(self, self->stream_0x958);
            break;
        case 0x44:
            self->stream_0x958 += (s16)fn_8013DE30(self, self->stream_0x958);
            break;
        case 0x45:
            self->stream_0x958 += (s16)fn_8013EE2C(self, self->stream_0x958);
            break;
        case 0x46:
            self->stream_0x958 += (s16)fn_8013EF14(self, self->stream_0x958);
            break;
        case 0x47:
            self->stream_0x958 += (s16)fn_8013EF3C(self, self->stream_0x958);
            break;
        case 0x48:
            flag = 1;
            break;
        case 0x49:
            self->stream_0x958 += (s16)fn_8013EF64(self, self->stream_0x958);
            break;
        case 0x4A:
            self->stream_0x958 += (s16)fn_8013EF8C(self, self->stream_0x958);
            break;
        case 0x4B:
            self->stream_0x958 += (s16)fn_8013EFD4(self, self->stream_0x958);
            break;
        case 0x4C:
            self->stream_0x958 += (s16)fn_8013F5B8(self, self->stream_0x958);
            break;
        case 0x4D:
            if (fn_8013F764(self, self->stream_0x958) == 1) {
                continue;
            }
            break;
        case 0x4E:
            self->stream_0x958 += (s16)fn_8013F8D8(self, self->stream_0x958);
            break;
        case 0x4F:
            fn_8013F994(self, self->stream_0x958);
            break;
        case 0x50:
            self->stream_0x958 += (s16)fn_8013F9D8(self, self->stream_0x958);
            break;
        case 0x51:
            self->stream_0x958 += (s16)fn_8013FA9C(self, self->stream_0x958);
            break;
        case 0x52:
            self->stream_0x958 += (s16)fn_8013FB08(self, self->stream_0x958);
            break;
        case 0x53:
            self->stream_0x958 += (s16)fn_8013FB9C(self, self->stream_0x958);
            break;
        case 0x54:
            self->stream_0x958 += (s16)fn_8013FC60(self, self->stream_0x958);
            break;
        case 0x55:
            fn_8013FD1C(self, self->stream_0x958);
            break;
        case 0x56:
            self->stream_0x958 += (s16)fn_8013FD28(self, self->stream_0x958);
            break;
        case 0x57:
            self->stream_0x958 += (s16)fn_8013FD50(self, self->stream_0x958);
            break;
        case 0x58:
            self->stream_0x958 += (s16)fn_8013FD74(self, self->stream_0x958);
            break;
        case 0x59:
            self->stream_0x958 += (s16)fn_8013FD98(self, self->stream_0x958);
            break;
        case 0x5A:
            fn_8013FEF4(self, self->stream_0x958);
            break;
        case 0x5B:
            self->stream_0x958 += (s16)fn_8013FF08(self, self->stream_0x958);
            break;
        case 0x5C:
            self->stream_0x958 += (s16)fn_8013FF38(self, self->stream_0x958);
            break;
        case 0x5D:
            self->field_0x99B = 1;
            break;
        case 0x5E:
            self->stream_0x958 += (s16)fn_8013FF5C(self, self->stream_0x958);
            break;
        case 0x5F:
            self->stream_0x958 += (s16)fn_8014001C(self, self->stream_0x958);
            break;
        case 0x60:
            fn_801400B0(self, self->stream_0x958);
            break;
        case 0x61:
            self->stream_0x958 += (s16)fn_801400BC(self, self->stream_0x958);
            break;
        case 0x62:
            self->stream_0x958 += (s16)fn_80140178(self, self->stream_0x958);
            break;
        case 0x63:
            self->stream_0x958 += (s16)fn_80140244(self, self->stream_0x958);
            break;
        case 0x64:
            byte = self->stream_0x958[0];
            break;
        case 0x65:
            self->stream_0x958 += (s16)fn_8014026C(self, self->stream_0x958, byte);
            break;
        case 0x66:
            self->stream_0x958 += (s16)fn_80140298(self, self->stream_0x958);
            break;
        case 0x67:
            self->stream_0x958 += (s16)fn_80140354(self, self->stream_0x958);
            break;
        case 0x68:
            self->stream_0x958 += (s16)fn_801404B8(self, self->stream_0x958);
            break;
        case 0x69:
            self->stream_0x958 += (s16)fn_80140580(self, self->stream_0x958);
            break;
        case 0x6A:
            self->stream_0x958 += (s16)fn_80140648(self, self->stream_0x958);
            break;
        case 0x6B:
            self->field_0x38E = 0;
            self->field_0x38F = 0;
            fn_80130778(self, 0);
            break;
        case 0x6C:
            self->stream_0x958 += (s16)fn_80140670(self, self->stream_0x958);
            break;

        case 0x10:
        case 0xFF:
            switch (self->field_0x95C) {
            default:
                fn_8013AAC4(self);
                continue;
            case 0:
                if (self->stack_0x961[1] == 0) {
                    fn_8013AB6C(self);
                } else {
                    fn_801408B4(self);
                }
                continue;
            case 2:
                self->field_0x999++;
                if (self->field_0x999 >= 0xa || fn_8013C254(self) == 1) {
                    if (self->field_0x382 == 0) {
                        fn_8013817C(self);
                    } else {
                        u8 key;
                        u8 v;

                        self->field_0x999 = 0;
                        if (self->table_0x9A4 != NULL) {
                            key = self->table_0x9A4->recs_0x04[self->field_0x382].field_0x0B;
                        } else {
                            key = 0;
                        }
                        self->field_0x382--;
                        self->field_0x382 = fn_8013AC08(self, self->field_0x382, key);
                        if (self->table_0x9A4 != NULL) {
                            v = self->table_0x9A4->recs_0x04[self->field_0x382].field_0x08;
                        } else {
                            v = 0;
                        }
                        em_target_pos_set(self, 0);
                        fn_8013AB74(self, self->field_0x95C, v);
                    }
                } else {
                    u8 key;
                    u8 found;
                    u8 v;

                    if (self->table_0x9A4 != NULL) {
                        key = self->table_0x9A4->recs_0x04[self->field_0x382].field_0x0B;
                    } else {
                        key = 0;
                    }
                    found = fn_8013AC08(self, self->field_0x382, key);
                    v = self->field_0x95D;
                    if (found != self->field_0x382) {
                        self->field_0x382 = found;
                        if (self->table_0x9A4 != NULL) {
                            v = self->table_0x9A4->recs_0x04[found].field_0x08;
                        }
                    }
                    fn_8013AB74(self, self->field_0x95C, v);
                }
                continue;
            case 3:
            case 4:
            case 5:
            case 6:
            case 7:
            case 11:
                if (self->field_0x79A != 0) {
                    self->field_0x79A = 0;
                    self->count_0x9A8 = count;
                    for (i = 0; (u8)i < count; i++) {
                        fn_8013BDC8(&self->recs_0x9AC[(u8)i], &save.recs_0x10[(u8)i]);
                    }
                    fn_801408B4(self);
                    self->field_0x380 = save380;
                    self->field_0x381 = save381;
                    self->field_0x382 = save382;
                    self->field_0x383 = save383;
                    self->field_0x384 = save384;
                    self->field_0x424 = save424;
                    em_target_pos_set(self, (s32)&save);
                    if (self->field_0x9E4 == 0) {
                        finished = 1;
                    }
                } else {
                    fn_8013AAC4(self);
                }
                continue;
            case 1:
                fn_801408B4(self);
                continue;
            case 8:
                if (self->field_0x95D == 0 && self->field_0x916 <= 0) {
                    self->field_0x95D = 1;
                    fn_8013AB74(self, self->field_0x95C, 1);
                } else {
                    fn_8013AAC4(self);
                }
                continue;
            case 9:
                if (fn_8012EC3C(self) == 1) {
                    fn_801409C8(self);
                }
                fn_8013AAC4(self);
                continue;
            case 10:
                switch (self->field_0x95D) {
                case 0:
                    if (niku_enemy_serial_matches(niku_find(self->field_0x381, self->field_0x382), (struct _ENEMY_WORK*)self) == 1) {
                        fn_8013AB74(self, self->field_0x95C, 1);
                    } else {
                        EmRunRec* run = (EmRunRec*)fn_80130DF8(self);

                        if (run != NULL) {
                            fn_8013AB74(self, 0xa, 0);
                            fn_8012B380(self, 7, run->field_0x05, run->field_0x06);
                            em_target_pos_set(self, 0);
                        } else {
                            fn_801408B4(self);
                        }
                    }
                    break;
                case 1:
                    fn_801408B4(self);
                    break;
                }
                continue;
            }
            continue;

        default:
            fn_80140AF8(self, 0xd, self->stream_0x958[0]);
            break;
        }

        self->stream_0x958 += (u8)len;

        if (finished != 0) {
            if (aborted == 0) {
                fn_80133BC0(self);
                return 1;
            }
            return 0;
        }
    }
}
