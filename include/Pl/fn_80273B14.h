/* The player act-entry/parameter unit `Pl/fn_80273B14.cpp` (`.text` 0x80273B14-0x80276B58).
 *
 * Declarations for the symbols this unit owns that other translation units call (docs/plan.md 6.5
 * rule 2), plus the one global the file reads whose address band names no module.
 */
#ifndef MHTRI_PL_FN_80273B14_H
#define MHTRI_PL_FN_80273B14_H

#include "types.h"

struct _PLW;
struct _EQUIP;

/* The pad/input status block at 0x80659350.  The lobby and `mh3_pad.cpp` each carry their own,
 * larger view of it, and `.bss` has no registered range in splits.txt, so the address band gives no
 * module: only the four control words `Pl_decide_mot_get` masks its pick table with are named here.
 * size: 0xD40 (the `Psw[]` array `mh3_pad.cpp` declares, 0x350 per player) */
struct PswBlock {
    /* +0x000 */ u8 pad_0x000[0xD6];
    /* +0x0D6 */ u16 control_0x0D6;  /* the second control word's low pair (masked with 0x88) */
    /* +0x0D8 */ u8 pad_0x0D8[0x6];
    /* +0x0DE */ u16 control_0x0DE;
    /* +0x0E0 */ u8 pad_0x0E0[0x1C];
    /* +0x0FC */ u16 control_0x0FC;  /* the held-button word */
    /* +0x0FE */ u8 pad_0x0FE[0x6];
    /* +0x104 */ u16 control_0x104;  /* the pressed-button word */
    /* +0x106 */ u8 pad_0x106[0xC3A];
};
extern struct PswBlock Psw;

/* 0x806590B4 - the lobby/act parameter block.  `.bss` has no registered range in splits.txt, so the
 * address band gives no module and only the three id/value rows `fn_802753E4` reads are named here.
 * size: 0x9C */
struct LbParamBlock {
    /* +0x00 */ u8 pad_0x00[0xC];
    /* +0x0C */ u8 id_a;   /* the three act ids the per-act bonus rows key on */
    /* +0x0D */ u8 id_b;
    /* +0x0E */ u8 id_c;
    /* +0x0F */ u8 pad_0x0F[0x1];
    /* +0x10 */ s16 bonus_a;
    /* +0x12 */ s16 bonus_b;
    /* +0x14 */ s16 bonus_c;
    /* +0x16 */ u8 pad_0x16[0x86];
};
extern struct LbParamBlock lb_param_w;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802756F0 - resets every per-act field of the player work and arms `+0x00A`/`+0x00C` from the
 * act's flag word. */
void fn_802756F0(struct _PLW* plw, u8 kind, u16 no, u16 mask);

/* 0x80275AC4 / 0x80275ADC - the two act-state entries that also set a marker byte. */
void fn_80275AC4(struct _PLW* self, s32 a, u16 b, u16 c);
void fn_80275ADC(struct _PLW* self, s32 a, u16 b, u16 c);

/* 0x80275B04 - arms the act's three status bits from a packed mask. */
void fn_80275B04(struct _PLW* self, u16 a, u32 b, u32 c);

/* 0x80275C18 - the act dispatcher's motion hand-off (`Pl_chr_setX` or its blended sibling). */
void fn_80275C18(struct _PLW* plw, u32 motion, u32 a, u32 b, u32 c);

/* 0x80275C34 and the three no-argument entries that drive it. */
void fn_80275C34(struct _PLW* plw, u32 kind, u32 a, u32 b, u32 c, u32 d);
void fn_802761B8(struct _PLW* self, u32 a, u32 b, u32 c);
void fn_802761C4(struct _PLW* self, u32 a, u32 b, u32 c);
void fn_802761D0(struct _PLW* self, u32 a, u32 b, u32 c);

/* 0x80276238 - clears the act's hold latch and re-enters the act state. */
void fn_80276238(struct _PLW* self, s32 a, s32 b, s32 c);

/* 0x80276254 / 0x80276270 - the two gauge-budget predicates. */
s32 fn_80276254(struct _PLW* self, s32 value);
s32 fn_80276270(struct _PLW* plw, s32 value);

/* 0x8027628C - adds `delta` to the act's hold gauge, clamped to 0..`+0x372`. */
void fn_8027628C(struct _PLW* plw, s16 delta);

/* 0x8027633C and its two wrappers - the act's frame step. */
s32 fn_8027633C(struct _PLW* plw, s16 delta, s8* out);
s32 fn_802764B0(struct _PLW* plw, s16 delta, s8* out);
s32 fn_80276514(struct _PLW* plw, s16 delta, s8* out);

/* 0x802765B4 / 0x80276690 / 0x80276778 / 0x802767B4 - the gauge and stagger setters. */
void fn_802765B4(struct _PLW* plw, s16 amount);
void fn_80276690(struct _PLW* plw, s32 amount, u8 a);
void fn_80276778(struct _PLW* plw, s16 amount);
void fn_802767B4(struct _PLW* plw, s16 amount);

/* 0x80276800 / 0x8027681C - the two act-state predicates. */
u32 fn_80276800(struct _PLW* self, s32 v);
u32 fn_8027681C(struct _PLW* plw);

/* 0x80276868 / 0x802768F8 / 0x80276A3C - the stagger-budget setters. */
void fn_80276868(struct _PLW* plw, s16 amount);
void fn_802768F8(struct _PLW* plw, s16 amount);
void fn_80276A3C(struct _PLW* plw);

/* 0x802745DC / 0x802748C8 / 0x80274AB8 / 0x80274AEC - the armed-slot and act-kind predicates.
 * `fn_802745DC` returns `u32` (not the pre-merge `u16`): its own body is byte-identical either way,
 * and the landed caller `Pl/fn_8027D684.cpp` only reproduces retail's `mr r0,r3` result copy with
 * the wider return.  The `s16` parameter is narrowed by retail's `extsh`. */
u32 fn_802745DC(struct _PLW* plw, s16 kind);
s32 fn_802748C8(void* self);
s32 fn_80274AB8(struct _PLW* plw);
s32 fn_80274AEC(u8 kind);

/* The equipment-slot resolvers this unit drives.  They sit inside `Pl/fn_8027D684.cpp`'s range
 * (0x8027D684-0x80288CEC), so they belong to that unit - which has no header of its own yet, so
 * they are declared here, with that unit's own definition signatures, and only this unit sees them
 * (rule 2: the band header must not declare a symbol a registered unit owns, and a second spelling
 * of a name the owner defines is the `illegal function overloading` class).  When
 * `Pl/fn_8027D684.cpp` grows a header these move there. */
void* fn_8027E344(struct _EQUIP* equip);
s32 fn_8027E510(void* slot);
s16 fn_8027E5E4(struct _EQUIP* equip);
s32 fn_8027E708(struct _EQUIP* equip);
void* fn_8027ECAC(struct _EQUIP* equip);
void* fn_8027ED18(void* equip);
void fn_8027ED28(struct _EQUIP* equip);
void* fn_8027EE08(struct _EQUIP* equip, u32 arg2, u32 arg3);
s32 fn_8027F008(struct _EQUIP* equip, u8 kind);
u32 fn_8027FFFC(struct _EQUIP* equip);

/* 0x80274B20 / 0x80274D98 / 0x80274DCC / 0x80274E00 - the act-id and bonus-sum helpers. */
u8 fn_80274B20(u16 id);
u8 fn_80274D98(struct _PLW* plw, u8 kind);
u8 fn_80274DCC(struct _PLW* plw, u8 kind);
s32 fn_80274E00(struct _PLW* plw, u16 id);

/* 0x802753E4 / 0x802754B4 / 0x802754C0 / 0x8027552C - the per-act bonus and the mode entries. */
s16 fn_802753E4(struct _PLW* plw, u16 id);
void fn_802754B4(struct _PLW* plw);
void fn_802754C0(struct _PLW* plw, u8 flag);
void fn_8027552C(struct _PLW* plw, const char* src);

/* 0x80273ED8 - the single-slot wrapper of the act-kind sum `pl_act.cpp`/`pl_skill.cpp` call. */
s32 fn_80273ED8(struct _PLW* plw, s32 kind, s32 flag);

/* 0x802740F4 / 0x802744A0 / 0x80274748 / 0x80274794 - the armed-value and follow-up stage helpers. */
void fn_802740F4(struct _PLW* plw, struct _EQUIP* equip);
void fn_802744A0(struct _PLW* plw);
void fn_80274748(struct _PLW* plw, s8 stage);
s32 fn_80274794(struct _PLW* plw);

/* 0x80274808 / 0x80275AFC - the follow-up stage byte and the act kind byte. */
s8 fn_80274808(struct _PLW* plw);
void fn_80275AFC(struct _PLW* plw, s8 value);

/* 0x802761DC - the motion-parameter act-state entry. */
void fn_802761DC(struct _PLW* self, u32 a, u32 b, u32 c);

/* 0x80274810 / 0x80274850 - the player's own move work record and its sibling scan. */
void* fn_80274810(void);
void* fn_80274850(struct _PLW* plw);

/* 0x80274918 - the act-name table row picker. */
u8* fn_80274918(struct _PLW* plw, s32 force, s32 index);

/* 0x80274E6C / 0x80275014 / 0x802751B4 - the three weapon-record bonus sums. */
u8 fn_80274E6C(struct _PLW* plw, struct _EQUIP* equip0, struct _EQUIP* equip1, s8* out);
u8 fn_80275014(struct _PLW* plw, struct _EQUIP* equip0, struct _EQUIP* equip1, s8* out);
f32 fn_802751B4(struct _EQUIP* equip0, struct _EQUIP* equip1, s8* out);

/* 0x802752C8 / 0x80275394 - the act-id and act-state pair predicates.  `Pl_item_id_usable_ck`'s second
 * argument is never read by its body, but every retail call site passes it (0 in
 * `menu/arena_result.cpp`, 1 in `enemy/em_pop.cpp`, 2 in `ai/fn_802D44F4.cpp`), so the declaration
 * carries it - the call sites' `li r4,<mode>` is otherwise unrepresentable.  The parameter's NAME
 * is a GUESS: the sites' 0/1/2 are the only evidence for it, and the body's own masks are 1/2/0x10. */
s32 Pl_item_id_usable_ck(u16 id, s32 mode);
s32 fn_80275394(struct _PLW* plw);

/* 0x8027403C / 0x8027408C / 0x802740EC / 0x80274174 / 0x80274370 / 0x80274570 / 0x80274584 /
 * 0x80274624 / 0x80274904 / 0x80274988 / 0x80274A04 / 0x80274B5C - the rest of the unit's own
 * entry points, kept here so a later consumer never declares them locally. */
u8 fn_8027403C(u16 kind);
s32 fn_8027408C(struct _PLW* plw);
void fn_802740EC(struct _PLW* plw);
s16 fn_80274174(struct _PLW* plw, s16 base, u8 ranged, s8* out);
s32 fn_80274370(struct _PLW* plw, struct _EQUIP* equip0, struct _EQUIP* equip1,
                struct _EQUIP* equip2, s8* out);
s32 fn_80274570(struct _PLW* plw);
void fn_80274584(s8 value);
s32 fn_80274624(struct _PLW* plw);
s32 fn_80274904(struct _PLW* plw);
s32 fn_80274988(struct _PLW* plw);
s32 fn_80274A04(struct _EQUIP* equip0, struct _EQUIP* equip1, u8 kind);
s32 fn_80274B5C(struct _PLW* plw, u8 kind, struct _EQUIP* equip0, struct _EQUIP* equip1,
                struct _EQUIP* equip2, s8* out);

/* 0x80273B14 - the act-kind delta sum, and the two C++-scope entry points whose map names are
 * manglings (`Pl_critical_get__FP4_PLW`, `Pl_decide_mot_get__FPUsPUs`); a C++ consumer calls these
 * spellings and the front-end mangles them back (rule 9). */
s32 fn_80273B14(struct _PLW* plw, s32 mode, u8 flag, struct _EQUIP* equip0, struct _EQUIP* equip1,
                struct _EQUIP* equip2, s8* out);
#ifdef __cplusplus
}
#endif

/* 0x80274340 / 0x8027554C - the two entry points whose map names are C++ manglings
 * (`Pl_critical_get__FP4_PLW`, `Pl_decide_mot_get__FPUsPUs`): a C++ consumer calls these spellings
 * and the front-end mangles them back (docs/plan.md 6.5 rule 9). */
#ifdef __cplusplus
s32 Pl_critical_get(struct _PLW* plw);
void Pl_decide_mot_get(u16* motion, u16* param);
#endif

#endif /* MHTRI_PL_FN_80273B14_H */
