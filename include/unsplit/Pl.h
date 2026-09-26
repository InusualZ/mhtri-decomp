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

s32 fn_802748C8(void* a);

/* Pl-band helpers with no registered owner, called by `Pl/fn_80262940.cpp` (proposal /80262940,
 * `.text` 0x80262940-0x802693C4): they sit in the unclaimed runs 0x80258FCC-0x80262940 and
 * 0x80273B14-0x80276B58 / 0x8027D684-... , so this band header is their rule-2 home.  (`fn_80257E70`
 * moved to `Pl/fn_8024F200.h` when that unit claimed 0x8024F200-0x80258FCC.  The
 * `fn_8025FA00`/`fn_80260A18`/`fn_80261770`/`fn_802621B0`/`fn_80262688` declarations that stood here
 * are owned by `Pl/fn_8025F088.cpp` (proposal 8025F088, `.text` 0x8025F088-0x80262940) and live in
 * `Pl/fn_8025F088.h` - that unit's range, not `Pl/fn_8024F200.h`, covers their addresses.) */
/* `fn_802745DC`'s return is the owner's `u32` (`Pl/fn_80273B14.cpp`), not the pre-merge `u16`: the
 * callee's own body is byte-identical either way (every path ends in an `lhzx`/`li`), while the
 * landed caller `Pl/fn_8027D684.cpp`'s measured row needs the `u32` - retail materialises the
 * result with `mr r0,r3` before the caller's `(u16)` cast, which only a `u32` return produces.  The
 * `s16` parameter is the owner's too (retail's callee narrows it with `extsh`). */
u32 fn_802745DC(struct _PLW* self, s16 kind);
s32 fn_80274AB8(struct _PLW* self);  /* the owner's own definition (fn_80273B14.cpp) */
s32 fn_80276254(struct _PLW* self, s32 v);
s32 fn_802764B0(struct _PLW* self, s16 delta, s8* out);  /* the owner's own definition */
s32 fn_80276514(struct _PLW* self, s16 delta, s8* out);  /* the owner's own definition */
void fn_80275AC4(struct _PLW* self, s32 a, u16 b, u16 c);
s32 fn_8027D7EC(struct _PLW* self, u8 flag);
s32 fn_8027E1E4(struct _PLW* self);
u32 fn_8027E220(struct _PLW* self, s32 v);
/* 0x80260198 is NOT declared here: it sits inside the registered unit `Pl/fn_8025F088.cpp`
 * (0x8025F088-0x80262940), so that unit's header `include/Pl/fn_8025F088.h` is its owner's
 * declaration (`s32 (_PLW*)`) and a second, differently-typed copy here is the `illegal
 * function overloading` class (rule 2).  Consumers include the owner's header. */



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
/* 0x805C4898 - the per-chunk rotation-offset pair table `Pl/fn_802430E8.cpp:fn_80246158` reads
 * (`chunk_ofs * 2` and `chunk_ofs * 2 + 1`); the `.data` run is unclaimed, so this band header is its
 * rule-2 home. */
extern f32 lbl_805C4898[];
extern const f32 lbl_80799E20; /* 0x80799E20 - the +1.0 angle `fn_80247CC4` rotates its motion vector by */
extern const f32 lbl_80799E24; /* 0x80799E24 - the frame gate `fn_80247EF0` hands `Pl_frame_check` */

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
/* The return was `u32`; the owner's own body ends without ever setting r3 (the value callers would
 * read is `Pl_master_ck`'s), and `Pl/fn_80258FCC.cpp` drops it at all three call sites, so the
 * declaration is `void` (docs/plan.md 6.5 rule 2: the owner owns the spelling). */
void fn_80245DA0(struct _PLW* self, u8 a);

/* The act band's remaining Pl helpers, added with the rest of `Pl/fn_80258FCC.cpp`.  Each signature
 * is the callee's own body (its prologue's argument saves and the width it narrows them to), not a
 * guess from the call site. */
void fn_80276238(struct _PLW* self, s32 a, s32 b, s32 c);
u32 fn_8027D8A0(struct _PLW* self, s32 a);

/* Pl-band callees and tables with no registered owner, added with `Pl/fn_80273B14.cpp`
 * (`.text` 0x80273B14-0x80276B58), which is their only consumer so far.  The addresses all sit in
 * the band's unclaimed runs, so this header is their rule-2 home.  The 0x8026A224 / 0x8026A2DC /
 * 0x8026A2F8 trio the same unit drives is NOT here: `Pl/fn_802693C4.cpp` owns
 * 0x802693C4-0x8026BA1C and declares them in `Pl/fn_802693C4.h`, which the consumer includes
 * (rule 2 - the owner's header wins, and a second spelling of the same name is the
 * `illegal function overloading` class).  The `fn_8027Exxx` equipment helpers this branch used to
 * declare here moved the same way: `Pl/fn_8027D684.cpp` now owns 0x8027D684-0x80288CEC, so its
 * signatures live in the consumer's own header (`include/Pl/fn_80273B14.h`, which is the only
 * consumer) - the band must not declare a symbol a registered unit owns. */

/* The `.data` tables the same unit reads. */
extern const u16 lbl_805C6030[];  /* the act-id row table `fn_802745DC`/`fn_80274624` walk */
extern const u16 lbl_805C604C[];  /* its sibling for the ranged family */
extern const u16 lbl_805BF490[];  /* 4-byte {u16 id, u16 value} rows, `fn_80274B20` looks ids up */
extern u8 lbl_805C60A8[];         /* the 11-row act/motion pick table `Pl_decide_mot_get` walks */
extern u8 lbl_805C5F30[];         /* the melee act-name table rows `fn_80274918` picks */
extern u8 lbl_805C5F50[];
extern u8 lbl_805C5F70[];

/* This unit's pooled `.sdata2` constants (playbook 29: declared, never defined).  `lbl_8079A000`
 * is the same word the run above names, so it is declared once, there. */
extern const f32 lbl_8079A044;
extern const f32 lbl_8079A080;
extern const f32 lbl_8079A084;

/* The 0x80273B14-0x80276B58 run's act/motion request entry point, declared for
 * `Pl/fn_8027D684.cpp`'s `fn_8027D6DC`.  The symbol is owned by `Pl/fn_80273B14.cpp`, so the
 * signature here is that owner's (`include/Pl/fn_80273B14.h`) and not a second, differently-typed
 * spelling of it: two C-linkage declarations of one name with different parameter types are the
 * `illegal function overloading` class (rule 2).  The owner's `u16` third parameter is what
 * retail's own callers narrow to (`fn_80275AC4`/`fn_80275ADC` emit `clrlwi r6,r6,16`) and what
 * reproduces the callee's own `clrlwi` on the mask. */
void fn_802756F0(struct _PLW* self, u8 kind, u16 no, u16 mask);

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

/* The shell band `Pl/fn_802840DC.cpp` (`.text` 0x802840DC-0x80288CEC) reads these tables and
 * constants out of the band's unclaimed data runs, so this header is their rule-2 home.  Every name
 * below is the map's own.  The band's own call targets - its 16 per-act handlers, which sit inside
 * `Pl/fn_8027D684.cpp`'s registered range 0x8027D684-0x802840DC - are declared in the consumer's
 * header `Pl/fn_802840DC.h` instead: a declaration here of a symbol a registered unit owns is the
 * `illegal function overloading` class, and this band must not declare one. */

/* The band's own tables, all unclaimed: `lbl_805C9608` is the 0x1A-byte-row attack table
 * `Pl/pl_act.cpp`'s `fn_80277974` walks (its `base` argument), and the `ShellAtkRow` arrays below are
 * the per-shell-kind attack rows `fn_802842D4` is handed (`&row[kind]`, 0x12 bytes per row).  The
 * second run of labels are the `u32` tables `fn_802842D4` passes on to `fn_802770E8`.  Declared,
 * never defined (invariant 8.4). */
extern u8 lbl_805C9608[];
extern s32 lbl_80792188[2]; /* 0x80792188 - the 8-byte id list (two zero-initialised ids) the attack
                              * rows' +0xF byte indexes; a *sized* declaration is what makes MWCC
                              * address it through r13 (`@sda21`, the target's form) instead of
                              * `lis`/`addi` (playbook: the absolute form comes from an unsized one) */
extern s32 lbl_80792190[1]; /* 0x80792190 - the single-id sibling list `fn_8028738C` passes */
extern s32 lbl_807921A8[2]; /* 0x807921A8 - the sibling list `fn_80288B98` passes */

/* One 18-byte per-shell-kind attack row: `mode` selects which half of the motion/parameter fields
 * applies. size: 0x12 */
typedef struct ShellAtkRow {
    /* +0x00 */ u16 mode;       /* the motion when `mode == 0xFFFF` picks the second field set, the
                                 * row's alternate-selector when the caller asks for kind 1 */
    /* +0x02 */ s16 value_0x02; /* second set: the motion's first argument */
    /* +0x04 */ s16 value_0x04; /* second set: the motion's second argument */
    /* +0x06 */ u16 add_0x06;   /* second set: frames added to the actor's own frame counters */
    /* +0x08 */ s16 gate_0x08;  /* > 0 arms the motion gate (`fn_80277C50`) with this value */
    /* +0x0A */ u16 motion_0x0A;  /* first set: the motion `fn_8026A224` sets */
    /* +0x0C */ s16 param_0x0C;   /* first set: the motion's first argument */
    /* +0x0E */ s16 param_0x0E;   /* first set: the motion's second argument */
    /* +0x10 */ s16 attack_0x10;  /* first set: the attack value `fn_80284204` is run with */
} ShellAtkRow;

/* The per-shell-kind attack-row tables (`mulli ...,<kind>,18` at every call site). */
extern ShellAtkRow lbl_805C9A78[];
extern ShellAtkRow lbl_805C9AC0[];
extern ShellAtkRow lbl_805C9AF8[];
extern ShellAtkRow lbl_805C9B78[];
extern ShellAtkRow lbl_805C9BC0[];
extern ShellAtkRow lbl_805C9BF8[];
extern ShellAtkRow lbl_805C9C1C[];
extern ShellAtkRow lbl_805C9CAC[];
extern ShellAtkRow lbl_805C9CD0[];

/* The attack-row tables' companion records: `fn_802842D4` hands these to `fn_802770E8` as a `u32` and
 * reads the `s16` at +0x8 of one as the row's motion gate. */
extern u8 lbl_805C9DC8[];
extern u8 lbl_805C9E4C[];
extern u8 lbl_805C9EBC[];
extern u8 lbl_805CA938[];
extern u8 lbl_805CACC4[];
/* The further attack tables of the band's single-shot entries (`fn_8028738C`, `fn_80288B98`). */
extern u8 lbl_805CB0B0[];
extern u8 lbl_805CBC80[];

/* The band's private `.sdata2` pool, 0x8079A1D8-0x8079A268 - declared as loads, never defined
 * (playbook 29).  The 30.0f/1.25f/0.83f trio is the charge-rate chain `fn_802843F0` applies. */
extern const f32 lbl_8079A1D8; /* 30.0f  - the base charge rate */
extern const f32 lbl_8079A1DC; /* 1.25f  - the skill-191 rate divisor */
extern const f32 lbl_8079A1E0; /* 0.83f  - the skill-192 rate divisor */
extern const f32 lbl_8079A1E4; /* 0.0f */
extern const f32 lbl_8079A1E8; /* -1.0f */
extern const f32 lbl_8079A1EC; /* 56.0f */
extern const f32 lbl_8079A1F0; /* 52.0f */
extern const f32 lbl_8079A1F4; /* 40.0f */
extern const f32 lbl_8079A1F8; /* 27.0f */
extern const f32 lbl_8079A1FC; /* 2*pi */
extern const f32 lbl_8079A200; /* 90.0f */
extern const f32 lbl_8079A204; /* 24.0f */
extern const f32 lbl_8079A208; /* 360.0f */
extern const f32 lbl_8079A20C; /* 18.0f */
extern const f32 lbl_8079A210; /* 74.0f */
extern const f32 lbl_8079A214; /* 80.0f */
extern const f32 lbl_8079A218; /* 26.0f */
extern const f32 lbl_8079A21C; /* 44.0f */
extern const f32 lbl_8079A220; /* 38.0f */
extern const f32 lbl_8079A228; /* 1.25f */
extern const f32 lbl_8079A22C; /* 1.0f */
extern const f32 lbl_8079A230; /* 0.75f */
extern const f32 lbl_8079A234; /* 4.0f */
extern const f32 lbl_8079A238; /* 0.0f */
extern const f32 lbl_8079A23C; /* 0.01f */
extern const f32 lbl_8079A240; /* 28.0f */
extern const f32 lbl_8079A244; /* 8.0f */
extern const f32 lbl_8079A248; /* 110.0f */
extern const f32 lbl_8079A24C; /* -10.0f */
extern const f32 lbl_8079A250; /* 2*pi */
extern const f32 lbl_8079A254; /* 90.0f */
extern const f32 lbl_8079A258; /* 102.0f */
extern const f32 lbl_8079A25C; /* 360.0f */
extern const f32 lbl_8079A260; /* 26.0f */
extern const f32 lbl_8079A264; /* 34.0f */
extern const f32 lbl_8079A268; /* 176.0f */

#ifdef __cplusplus
}

/* 0x8027E1C8 - the attack-flag predicate the shell band's `fn_80284204` gates a hit entry on.  The
 * symbol is owned by `Pl/fn_8027D684.cpp` (0x8027D684-0x80288CEC) since that unit landed, so the
 * signature here is the owner's own definition's (`s32 (_PLW*, u8)`) and not a second,
 * differently-typed spelling of it: two C-linkage declarations of one name with different types
 * are the `illegal function overloading` class (rule 2).  The map name is its mangling and the
 * owner grows no header yet, so the callable spelling of the map name still lives here
 * (docs/plan.md 6.5 rule 9). */
s32 Pl_atk_act_flag_ck(struct _PLW* self, u8 mask);

/* 0x803C4814 - `event_demo_ck__Fv`, a C++ free function the Pl/ef band reads.  Unregistered, and the
 * files that spell it `int` locally would clash with a `u32` in `unsplit/unknown.h`, so it lives
 * here. */
u32 event_demo_ck(void);

/* 0x8029F6DC - the item-record lookup, spelled `GetItemData__FUs` in the map: a C++ free function
 * taking the 16-bit item id and returning the record, so it is declared at C++ scope and the
 * front-end reproduces the map spelling (docs/plan.md 6.5 rule 9).  The range is unregistered, but
 * the declaration now lives in `include/Pl/fn_8025F088.h` (the unit that first needed it), so it is
 * NOT repeated here - a second copy is the rule-2 duplicate this header exists to avoid.  The
 * callers read the record as bytes (`Pl/fn_802430E8.cpp:fn_802466C4` tests bit 3 of +0x2 and byte
 * +0x0). */

struct _PLW;

#endif

#endif /* MHTRI_UNSPLIT_PL_H */
