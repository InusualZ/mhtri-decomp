#ifndef MHTRI_UNSPLIT_PL_H
#define MHTRI_UNSPLIT_PL_H

#include "types.h"

struct _se_w;

/* Declarations for symbols with no registered owner whose address band names the `Pl` module
 * (docs/plan.md 6.5, rule 2).  Plain C-linkage names, so C-visible.
 */
struct _PLW;
struct _se_w;

/* One 24-byte vector pair of the motion layer's placement tables; `Pl/fn_80224AC4.cpp` hands these to
 * the model layer. size: 0x18 */
typedef struct PlSeVecPair {
    /* +0x00 */ f32 x_0x00;
    /* +0x04 */ f32 y_0x04;
    /* +0x08 */ f32 z_0x08;
    /* +0x0C */ f32 x_0x0C;
    /* +0x10 */ f32 y_0x10;
    /* +0x14 */ f32 z_0x14;
} PlSeVecPair; /* size: 0x18 */

#ifdef __cplusplus
extern "C" {
#endif

s8 fn_802748C8(void* a);

/* Pl-band helpers with no registered owner, called by `Pl/fn_80262940.cpp` (proposal /80262940,
 * `.text` 0x80262940-0x802693C4): they sit in the unclaimed runs 0x80258FCC-0x80262940 and
 * 0x80273B14-0x80276B58 / 0x8027D684-... , so this band header is their rule-2 home.  (`fn_80257E70`
 * and `fn_8025FA00` moved to `Pl/fn_8024F200.h` when that unit claimed 0x8024F200-0x80258FCC.) */
u16 fn_80260A18(struct _PLW* self);
s32 fn_80261770(struct _PLW* self, u8* a, u16* b, u16* c, s32* d, u16* e, u16* f);
u32 fn_802621B0(struct _PLW* self, void* b);
s32 fn_80262688(struct _PLW* self);
u32 fn_802745DC(struct _PLW* self, u32 v);
s32 fn_80274AB8(s32 a);
s32 fn_80276254(struct _PLW* self, s32 v);
u32 fn_802764B0(struct _PLW* self, s32 v);
u32 fn_80276514(struct _PLW* self, s32 v);
void fn_80275AC4(struct _PLW* self, s32 a, u16 b, u16 c);
s32 fn_8027D7EC(struct _PLW* self, u8 flag);
s32 fn_8027E1E4(struct _PLW* self);
u32 fn_8027E220(struct _PLW* self, s32 v);



/* The 0x80229xxx motion/SE helper family `Pl/fn_80229ECC.cpp` dispatches into (all unregistered and
 * unmangled; the actor itself is the `_PLW` at their r3). */
void fn_80229CB4(struct _PLW* self);
void fn_80229E10(struct _se_w* work, s32 code, s32 val);
void fn_80229EA8(struct _se_w* work, s32 a, s32 b, s32 c);

/* 0x80244E88 - the per-motion effect dispatcher `Pl/fn_80229ECC.cpp` hands `&self->field_0xAF4`. */
void fn_80244E88(void* p, u32 a, u32 b, u32 c);

/* The Pl helpers `Pl/fn_802489D4.cpp` (0x802489D4-0x8024F200) and `Pl/fn_8024F200.cpp`
 * (0x8024F200-0x80258FCC) call that sit in the band's unclaimed runs (0x802430E8-0x80258FCC and
 * 0x80273B14-0x80276B58; the third run main listed here, 0x802693C4-0x8026BA1C, is owned by
 * `Pl/fn_802693C4.cpp`).  This band header is their rule-2 home, and the C
 * linkage here is the map's (every name is a bare `fn_XXXXXXXX`).  Each signature is the owner's own
 * body where one exists, the call site's register width otherwise - one declaration serves both
 * consumers, so the widths are the ones the two units' call sites agree on.  The
 * `fn_8026A224`/`fn_8026A33C`/`fn_8026A644` trio moved into `Pl/fn_802693C4.h` with the unit that now
 * owns 0x802693C4-0x8026BA1C. */
/* The second argument is `u16`: retail keeps the `clrlwi r4,r4,16` that narrows the `lis`/`subi`
 * constant at the `0x8001`/`0x8003` call sites, which MWCC only emits for a narrower parameter.
 * `Pl/fn_8024F200.cpp`'s calls pass 0/3, so the width is immaterial there. */
void fn_80275B04(struct _PLW* self, u16 a, u32 b, u32 c);
void fn_802761B8(struct _PLW* self, u32 a, u32 b, u32 c);
void fn_80276868(struct _PLW* self, s16 value);
u32 fn_80276800(struct _PLW* self, s32 v);

/* The unclaimed `.data` tables this unit's act handlers index (no registered `.data` range covers
 * them, so - like the band's code - this header is their rule-2 home). */
extern u32 lbl_805BE824[]; /* 0x805BE824 - the per-act SE/motion table `fn_802770E8` is handed */
extern u32 lbl_805BE5B8[]; /* 0x805BE5B8 - the sibling table `fn_8024A640` is handed */
extern u16 lbl_805C4A54[]; /* 0x805C4A54 - 3 rows of {u16 motion, u16 param} `fn_8024A8EC` reads */
extern u16 lbl_805C4A60[]; /* 0x805C4A60 - the sibling motion row table `fn_8024B35C` indexes */
extern u16 lbl_805C4A6C[]; /* 0x805C4A6C - the sibling motion row table `fn_8024B46C` indexes */
extern u32 lbl_805C9118[]; /* 0x805C9118 - an effect/motion table `fn_802770E8` is handed */
extern u32 lbl_805CAD74[]; /* 0x805CAD74 - the sibling table for the other actor kind */
extern u32 lbl_805E2048[]; /* 0x805E2048 - the sibling table for the third actor kind */
/* The `.sdata2` floats this band gates its frame checks on. */
extern const f32 lbl_80799E00; /* 0x80799E00 - the zero/identity angle the Pl frame checks compare
                                * against; `const` because `Pl/fn_8024F200.cpp` declares the same
                                * pool word `const f32` in its own file (a bare `f32` redeclaration is
                                * `(10563)`), and the pool is never written */
extern const f32 lbl_80799E38;
extern const f32 lbl_80799E48;
extern const f32 lbl_80799E2C;
extern const f32 lbl_80799E84;
extern const f32 lbl_80799EAC;
extern const f32 lbl_80799EB0;
extern const f32 lbl_80799EB4;
extern const f32 lbl_80799EC0;
extern const f32 lbl_80799EC4;
extern const f32 lbl_80799E4C;
extern const f32 lbl_80799E54;

/* Pl-band callees with no registered owner (`Pl/fn_80224AC4.cpp`, `.text` 0x80224AC4-0x80229ECC).
 * The 0x8026A3xx trio (`fn_8026A328`/`fn_8026A34C`/`fn_8026A3A0`) sits inside the range
 * `Pl/fn_802693C4.cpp` now owns (`.text` 0x802693C4-0x8026BA1C), so it is declared in
 * `Pl/fn_802693C4.h`; 0x8025EFF4 is owned by `Pl/fn_80258FCC.cpp` and is declared in
 * `Pl/fn_80258FCC.h`; the two 0x8027Exxx helpers sit in the runs the header above already
 * documents. */
s32 fn_8027EE24(void);
s32 fn_8027EBA8(struct _PLW* self, void* equip);

/* The range's private pooled constants (playbook 29: declared, never defined).  `.sdata2`
 * 0x80799CD8-0x80799Dxx is the run `Pl/fn_80224AC4.cpp` and the lobby unit next door share; `.sdata`
 * 0x807913C0-0x807913D8 is the per-`se_name_set` decoration table pair; `.sdata` 0x80792010 is the
 * per-bank SE gain table. */
extern const f32 lbl_80799CD8;
extern const f32 lbl_80799CDC;
extern const f32 lbl_80799CE0;
extern const f32 lbl_80799CE4;
extern const f32 lbl_80799CF8;
extern const f32 lbl_80799CFC;
extern const f32 lbl_80799D00;
extern const f32 lbl_80799D04;
extern const f32 lbl_80799D08;
extern const f32 lbl_80799D0C;
extern const f32 lbl_80799D10;
extern const f32 lbl_80799D14;
extern const f32 lbl_80799D18;
extern const f32 lbl_80799D1C;
extern const f32 lbl_80799D20;
extern const f32 lbl_80799D24;
extern const f32 lbl_80799D28;
extern const f32 lbl_80799D2C;
extern const f32 lbl_80799D30;
extern const f32 lbl_80799D34;
extern const f32 lbl_80799D38;
extern const f32 lbl_80799D3C;
extern const f32 lbl_80799D40;
extern const f32 lbl_80799D44;
extern const f32 lbl_80799D48;
extern const f32 lbl_80799D4C;
extern const f32 lbl_80799D50;
extern const f32 lbl_80799D54;
extern const f32 lbl_80799D58;
extern const f32 lbl_80799D5C;
extern const f32 lbl_80799D60;
extern const f32 lbl_80799D64;
extern const f32 lbl_80799D68;
extern const f32 lbl_80799D6C;
extern const f32 lbl_80799D70;
extern const f32 lbl_80799D74;
extern const f32 lbl_80799D78;
extern const f32 lbl_80799D7C;
extern const f32 lbl_80799D80;
extern const f32 lbl_80799D84;
extern const f32 lbl_80799D88;
extern const f32 lbl_80799D8C;
extern const f32 lbl_80799D90;
extern const f32 lbl_80799D94;
extern const f32 lbl_80799D98;
extern u8* lbl_807913C0[];
extern u8* lbl_807913C8[];
extern u8* lbl_807913D0[];
extern u8* lbl_807913D8[];
extern const f32 lbl_80792010[2];
extern u8 lbl_805BAAD4[];
/* The 0x8079A000 `.sdata2` run the Pl band's part/motion accessors load (0.0f and 1.0f; the eight
 * bytes at +0x08 are the 2^52 double MWCC's u32->f32 conversion uses, which overlaps the -0.0f at
 * +0x0C).  No registered unit claims the run, so this band header is its rule-2 home - declared, never
 * defined (defining it would rebuild the pool). */
extern const f32 lbl_8079A000; /* 0.0f */
extern const f32 lbl_8079A004; /* 1.0f */

/* The `.data` lookup tables the motion-number helpers at 0x8026A00C/0x8026A068 index: ten row
 * pointers (one per hundred of the 0-999 range) and, for the 1000+ range, a two-level table keyed by
 * the actor's `_PLW`+0x002 byte.  Both are unclaimed. */
extern s16* lbl_805C0C98[];
extern s16** lbl_805C1584[];

extern PlSeVecPair lbl_805BB028[];
extern PlSeVecPair lbl_805BB130[];

/* The four sibling motion banks `Pl/fn_80230FBC.cpp`'s kind byte dispatches to (kinds 3 and 7), and
the motion bank that unit's range stops at.  All are unregistered, so this band header is their
rule-2 home.  Each takes the player work and the SE part index the dispatcher read. */
void fn_802373AC(struct _PLW* self, u8 part);
void fn_802399C8(struct _PLW* self, u8 part);
void fn_8023C2D0(struct _PLW* self, u8 part);
void fn_8023FC20(struct _PLW* self, u8 part);
void fn_802430E8(struct _PLW* self, u8 part);

/* The 0x8026A224 / 0x8026A33C / 0x8026A644 group above is owned by `Pl/fn_802693C4.cpp` and declared
 * in `Pl/fn_802693C4.h`, which the three player-act consumers (`Pl/fn_802489D4.cpp`,
 * `Pl/fn_8024F200.cpp`, `Pl/fn_80258FCC.cpp`) include.  The 0x80275B04 / 0x802761B8 pair sits in the
 * band's unclaimed run 0x80273B14-0x80276B58, so the band header is its rule-2 home; where the two
 * lanes spelled one argument differently the MAIN spelling is kept (M7). */

/* The act tail's predicates and setters, called by `Pl/fn_80258FCC.cpp` (`.text`
 * 0x80258FCC-0x8025F088).  `fn_80245DA0` sits in the unclaimed run 0x802430E8-0x80258FCC, so the band
 * header is its rule-2 home; `fn_8026A3A8` sits inside `Pl/fn_802693C4.cpp`'s range and is declared
 * in `Pl/fn_802693C4.h`. */
u32 fn_80245DA0(struct _PLW* self, u32 a);

/* The act band's remaining Pl helpers, added with the rest of `Pl/fn_80258FCC.cpp`.  Each signature
 * is the callee's own body (its prologue's argument saves and the width it narrows them to), not a
 * guess from the call site. */
void fn_80276238(struct _PLW* self, s32 a, s32 b, s32 c);
u32 fn_8027D8A0(struct _PLW* self, s32 a);

/* The 0x80273B14-0x80276B58 run's act/motion request entry point `Pl/fn_8027D684.cpp`'s
 * `fn_8027D6DC` calls; unregistered, so this band header is its rule-2 home.  The four argument
 * registers are the callee's own prologue's (`Pl/fn_802756F0` saves r3..r6). */
void fn_802756F0(struct _PLW* self, u8 a, u16 b, u32 c);

/* The per-slot gate table at 0x806BB7A0 (`.bss`, 0x18 B = three 8-byte entries, the map's size).
 * size: 0x8 */
typedef struct PlSlotGate {
    /* +0x00 */ u8 flag_0x00;   /* non-zero means the slot is occupied (`fn_8027DC64`/`78`/`90`) */
    /* +0x01 */ u8 pad_0x01[0x7];
} PlSlotGate;

extern PlSlotGate lbl_806BB7A0[3];

/* 0x806AB810 (.bss, 0x20 B = eight 4-byte table pointers, the map's size): the two per-kind row
 * tables `fn_8027E2A8`/`fn_8027E354` index after `fn_8027EFB4` validates the kind. */
extern u8** lbl_806AB810[8];
extern u32 lbl_805706C0[];  /* the per-kind row counts the two lookups clamp against */
extern u32 lbl_805706D8[];  /* the sibling counts `fn_8027E918` reads for kinds 7-15 */

/* The three per-kind item-id tables `fn_8027DE88`/`fn_8027DF38` walk until the 0xFFFF sentinel
 * (`.sdata` 0x80792030/0x80792038 at 0x8 B each, `.data` 0x805BFFE0 at 0x18 B - the map's sizes). */
extern u16 lbl_80792030[];
extern u16 lbl_80792038[];
extern u16 lbl_805BFFE0[];

/* This unit's pooled `.sdata2` constants (playbook 29: declared, never defined - the run is
 * unclaimed, so this band header is their rule-2 home). */
extern u8 lbl_805C4F5C[];  /* the per-act `.data` record `fn_802770E8` installs */
extern const f32 lbl_80799E00;
extern const f32 lbl_80799E2C;
extern const f32 lbl_80799EB0;
extern const f32 lbl_80799F00;
extern const f32 lbl_80799F04;
extern const f32 lbl_80799F08;

#ifdef __cplusplus
}

/* 0x803C4814 - `event_demo_ck__Fv`, a C++ free function the Pl/ef band reads.  Unregistered, and the
 * files that spell it `int` locally would clash with a `u32` in `unsplit/unknown.h`, so it lives
 * here. */
u32 event_demo_ck(void);

struct _PLW;

#endif

#endif /* MHTRI_UNSPLIT_PL_H */
