/*
 * Player skill module (Pl_skill): the cluster the previous session split off between pl_master and pl_act.
 * .text 0x80270018-0x80273B14 (50 functions, 0x3AFC B) with its own exception tables - extab
 * 0x800126D4-0x8001280C, extabindex 0x8002F8BC-0x8002FA90.
 *
 * Left edge pinned by the `.sdata2` pool run (`lbl_8079A03C`), right edge by the closure; the reasoning is in
 * configure.py beside the Pl lib entry.
 *
 * C++ (`Pl_Skill_ck__FP4_PLWUs`, `Get_pl_type__FP6_EQUIPP6_EQUIP`), but every symbol the map still calls
 * `fn_XXXXXXXX` is *defined* with `extern "C"` here: an unmangled name is what the target object exports and
 * what objdiff pairs on. The named ones keep their C++ signatures (`Pl_Skill_ck(_PLW*, u16)` and friends).
 *
 * Flags - all four are measured against the target object, none is in configure.py yet (Pl lib entry):
 *   -O3  not `-O4,p`: fn_80270018 is 87.6 % at -O3 and 65.0 % at -O4,p (level 4 = `schedule for gekko`).
 *        Function alignment is 4 (`fn_8027035C` sits at ...35C), so `,p`/func_align 16 is out too.
 *   -opt nopeephole: level 3's constant-merge turns the target's `lis/stw 8(r1); lis/stw 16(r1)` literal
 *        pair into one `lis` + two `stw`, and hoists the `li r0,1 / stb` of each skill branch above the
 *        `addi r31,r31,N`. fn_80270018 87.6 -> 99.7 %, size 832 -> 836 B (target 836).
 *   -inline noauto: fn_80270CA4 must keep its five real `bl fn_80270C64` calls; `auto` inlines them and the
 *        function grows 684 -> 828 B. `-opt noautoinline` is NOT the spelling that works, `-inline noauto` is.
 *   -sdata 0: the target addresses `lobby_w` with lis/addi (ABS16), not `@sda21`. As a flag it now *hurts*:
 *        it fixes Pl_Skill_ck 98.7 -> 100 and fn_802715A0 97.0 -> 100, but breaks fn_802738B8 100 -> 72.5
 *        and fn_802738D8 100 -> 45.0 (the `lbl_80792140`/`lbl_80792148` byte tables *are* small data in the
 *        target), net -0.12 pt. The two functions are closed in the source instead, by declaring `lobby_w`
 *        as an unsized array (`lobby_w[0]`) so MWCC stops putting it in the small-data area: +0.085 pt,
 *        nothing regresses, no flag touched.
 * With the registered `cflags_base` instead, the same source measures 65.0 / 82.1 / 68.0 / 80.2 / 98.6 / ...
 * and fn_80270F50 0 % (auto-inlining blows it up to 2852 B).
 *
 * Source shapes that are load-bearing (both were found by matching, not guessed):
 *   - A `switch` on an *unsigned* value is what puts the case blocks out of line with a taken `ble`/`beq`
 *     and the default reached by a forward `b` (fn_8027176C, fn_80271674, fn_80271A18); the same chain
 *     written as if/else-if emits the inverted branch and lays the first block inline. `switch (u8)` is not
 *     the same thing - it promotes to a signed switch and emits a `cmpwi` chain.
 *   - The equipment-slot lookups index a 4-byte `_SLOTENT` table at 0x278 with `(slot & 0x7F) + 26` for the
 *     spare-slot half, and the `(u8)slot` re-mask on each use is what the target does.
 *   - The literal-pool floats are `extern` declarations used as load operands, never definitions: the 21
 *     plain-float pool entries are named `lbl_8079A0xx` and the multiply is written `lbl * value`, which is
 *     the operand order the target's `fmuls` rows have. Only the two implicit int->float magics are left.
 *
 * Residuals (all measured with the four flags above; a function not listed is 100 %):
 *   fn_80270018 99.88 fn_8027035C 99.87 fn_802703F4 99.88 fn_80270728 99.71 fn_80270CA4 99.71
 *   fn_80270F50 99.92 fn_802739F0 99.93 - only the two implicit int->float magic rows differ (0x8079A008
 *     `(f32)(s32)` and 0x8079A048 `(f32)(u32)`). MWCC synthesises those for a cast, so they cannot be named
 *     from source; the unit's own pool run is 0x8079A030-0x8079A080 (80 B, 19 entries) but the magics live
 *     in the preceding unit's run, outside any contiguous claim.
 *   fn_80271BD4 77.07 / fn_80271E0C 78.56 - the kind dispatch needs `(u32)(kind-7) <= 8` with taken
 *     branches; a switch gives the branches but MWCC's own range form, an if/else chain the range form but
 *     inverted branches. Ours also saves two more GPRs (frame 0x30 vs 0x20).
 *   fn_80273044 55.84 - the target unrolls the eight-entry inner scan (`mtctr 3` outer, fixed
 *     displacements); ours keeps it rolled, 236 B against the target's 368 B.
 *   fn_802736A0 67.56 - the target saves r28-r31 with four `stw`; ours allocates r26-r29 and so uses
 *     `_savegpr_26`. Declaration order does not move it.
 *   fn_80272DB4 70.16, Pl_cat_skill_ck 79.92, fn_80272D5C 90.45, fn_80273998 90.91, fn_8027373C 96.55,
 *     fn_802738E8 97.73 - single-row residuals: register choice, one redundant `clrlwi` on the
 *     `& ~(1<<i)` form (MWCC folds it into `andc`), or a branchless `subf/cntlzw` select where the target
 *     keeps four separate `li r3,1; blr` returns.
 *   fn_80273228 97.40 - the 24-entry scan's loop: ours carries a byte-offset induction variable and
 *     materialises the array base for the second half of the eight unrolled checks, where the target
 *     advances the base pointer (`addi r28,r28,0x20`) and folds the dead counter into `addi r3,r3,7`
 *     (584 B against the target's 580).
 *   fn_8027350C 95.20 - after the fn_802693C4/fn_80269474 call ours recomputes `plw + i*4` for the
 *     `unk218[i] = unk234[i]` copy; the target reuses the base it had already kept for `&plw->unk234[i]`
 *     (three instructions and 12 B over the target).
 *
 * Source shapes in these five that are load-bearing, not guesses:
 *   - A helper's narrow return type is *not* trusted sign-extended, so it decides where MWCC re-emits the
 *     conversion: `fn_8004BA3C` must return `s16` (the `(u32)(s16)v` tests then keep their own `extsh` and
 *     the two 8/24-slot call blocks stay separate) and fn_80272E30 must return `s16` for `return v` to stay
 *     a bare `mr`. fn_802724E8's `a` and 7th parameter are signed-byte typed (`s8*`, `s8 aval`) - as `u8`
 *     it masks the level the target passes raw - and fn_8027252C's third parameter follows it to `s8*`.
 *   - fn_80272E30's `case 4` compares `plw->unk26E` against the *first* slot lookup, not the resolved one;
 *     only `case 0` and the 1-3 block use the resolved slot.
 *   - fn_8027252C's locals are declared `tv, tb, ta` - that order fixes both the stack slots (0x20/0x18/
 *     0x10) and the register order (r7/r8/the table pointer/ta). Its value loop has to be
 *     `if (v != 0) { if (v > 0) ... else ... } else { zeros }` so the zero block lands out of line, the
 *     swap has to do tv/tb first and ta in its own block, and the epilogue stores `a[i] = (s8)tv[i]`.
 *   - fn_80272B10's second slot lookup indexes `plw->unk278` with a `?:` *and* calls GetItemData in each
 *     arm of the if/else; written as one arm the two loads get merged.
 *
 * All 50 functions are written in address order (unit fuzzy 96.1 %, up from 75.6 % at 45 written).
 */

#include "types.h"

extern "C" {
extern f32 lbl_8079A000;
extern f32 lbl_8079A004;
extern f32 lbl_8079A010;
extern f32 lbl_8079A030;
extern f32 lbl_8079A034;
extern f32 lbl_8079A038;
extern f32 lbl_8079A03C;
extern f32 lbl_8079A040;
extern f32 lbl_8079A044;
extern f32 lbl_8079A050;
extern f32 lbl_8079A054;
extern f32 lbl_8079A058;
extern f32 lbl_8079A05C;
extern f32 lbl_8079A060;
extern f32 lbl_8079A064;
extern f32 lbl_8079A068;
extern f32 lbl_8079A06C;
extern f32 lbl_8079A070;
extern f32 lbl_8079A074;
extern f32 lbl_8079A078;
extern f32 lbl_8079A07C;
}


struct _EQUIP {
    u8 unk[12];
};

/* One 4-byte equipment-slot entry: an item id and a signed value. */
struct _SLOTENT {
    /* 0x0 */ u16 unk0;
    /* 0x2 */ s16 unk2;
};

struct _PLW {
    /* 0x000 */ u8 unk000[2];
    /* 0x002 */ u8 unk2;
    /* 0x003 */ u8 unk003[0x008 - 0x003];
    /* 0x008 */ u8 unk8;
    /* 0x009 */ u8 unk009[0x13C - 0x009];
    /* 0x13C */ u8* unk13C;
    /* 0x140 */ _EQUIP equipA[6];
    /* 0x188 */ u8 unk188[0x1D0 - 0x188];
    /* 0x1D0 */ _EQUIP equipB;
    /* 0x1DC */ u8 unk1DC[0x1E8 - 0x1DC];
    /* 0x1E8 */ _EQUIP equipC;
    /* 0x1F4 */ _EQUIP equipD;
    /* 0x200 */ u8 unk200[0x218 - 0x200];
    /* 0x218 */ s32 unk218[7];
    /* 0x234 */ s32 unk234[7];
    /* 0x250 */ u16 unk250;
    /* 0x252 */ u16 unk252;
    /* 0x254 */ u16 unk254;
    /* 0x256 */ u8 unk256[0x269 - 0x256];
    /* 0x269 */ u8 unk269;
    /* 0x26A */ u8 unk26A;
    /* 0x26B */ u8 unk26B;
    /* 0x26C */ u8 unk26C;
    /* 0x26D */ u8 unk26D;
    /* 0x26E */ u16 unk26E;
    /* 0x270 */ s16 unk270;
    /* 0x272 */ s16 unk272;
    /* 0x274 */ u8 unk274;
    /* 0x275 */ u8 unk275;
    /* 0x276 */ u8 unk276[0x278 - 0x276];
    /* 0x278 */ _SLOTENT unk278[24];
    /* 0x2D8 */ u8 unk2D8[0x2E0 - 0x2D8];
    /* 0x2E0 */ _SLOTENT unk2E0[8];
    /* 0x300 */ u8 unk300[0x304 - 0x300];
    /* 0x304 */ u16 unk304;
    /* 0x306 */ u8 unk306[0x370 - 0x306];
    /* 0x370 */ s16 unk370;
    /* 0x372 */ s16 unk372;
    /* 0x374 */ u8 unk374[0x37A - 0x374];
    /* 0x37A */ s16 unk37A;
    /* 0x37C */ u8 unk37C[0x380 - 0x37C];
    /* 0x380 */ s16 unk380;
    /* 0x382 */ u8 unk382[0x3B8 - 0x382];
    /* 0x3B8 */ u16 unk3B8;
    /* 0x3BA */ u16 unk3BA;
    /* 0x3BC */ f32 unk3BC;
    /* 0x3C0 */ f32 unk3C0;
    /* 0x3C4 */ f32 unk3C4;
    /* 0x3C8 */ f32 unk3C8;
    /* 0x3CC */ f32 unk3CC;
    /* 0x3D0 */ f32 unk3D0;
    /* 0x3D4 */ f32 unk3D4;
    /* 0x3D8 */ u8 unk3D8[0x422 - 0x3D8];
    /* 0x422 */ s16 unk422;
    /* 0x424 */ u8 unk424[0x446 - 0x424];
    /* 0x446 */ u8 unk446;
    /* 0x447 */ u8 unk447;
    /* 0x448 */ s8 unk448;
    /* 0x449 */ s8 unk449;
    /* 0x44A */ u8 unk44A[0x44C - 0x44A];
    /* 0x44C */ s8 unk44C;
    /* 0x44D */ s8 unk44D;
    /* 0x44E */ u8 unk44E[0x5F2 - 0x44E];
    /* 0x5F2 */ u16 unk5F2[8];
    /* 0x602 */ u8 unk602[8];
    /* 0x60A */ u8 unk60A[0x612 - 0x60A];
    /* 0x612 */ u16 unk612;
    /* 0x614 */ u16 unk614;
    /* 0x616 */ u16 unk616;
    /* 0x618 */ u16 unk618;
    /* 0x61A */ u16 unk61A[8];
    /* 0x62A */ u8 unk62A[8];
    /* 0x632 */ u8 unk632[0x634 - 0x632];
    /* 0x634 */ u32 unk634;
    /* 0x638 */ u32 unk638;
    /* 0x63C */ u32 unk63C;
    /* 0x640 */ u32 unk640;
};

extern "C" {
void fn_8004A20C(_EQUIP*, _EQUIP*);
u32 fn_8026FFBC(_PLW*);
u32 fn_80273ED8(_PLW*, int, s8);

int fn_802753E4(_PLW*, u16);
void fn_80272A08(_PLW*);
u32 fn_8026FE44(_PLW*);
void fn_8027885C(_PLW*, int, int);
void fn_802789EC(_PLW*, int);
extern u32 lbl_805C5FC8[];
extern u32 lbl_805C5FE0[];
u8 fn_800CF208(void);
u32 fn_80363A2C(void);
int fn_80274AB8(int);
u32 fn_8027E29C(u8);
int fn_802731B4(_PLW*, u16);
u32 fn_8029F6B4(u16);
u32 fn_8027E290(u8);
void fn_8027E98C(u8*);
s32 fn_8027EBA8(_PLW*, u8*);
u32 fn_80274B20(u16);
void fn_8027252C(_EQUIP*, u16*, s8*, u8*);
void* memset(void*, int, u32);
s8 Get_pl_type__FP6_EQUIPP6_EQUIP(_EQUIP*, _EQUIP*);
u16 fn_8027993C(_PLW*, u16, int);
void fn_80279B84(_PLW*);
u8* GetItemData__FUs(u16);
u16 fn_80273044(_PLW*, u16);
s32 fn_8004BD30(s16*);

void fn_8027350C(_PLW*, s32);


void* fn_8027E344(void);

u32 fn_80274AEC(u8*, u32, u8);

s16 fn_8004BA3C(u16, s16, void*, int, int, int);
u16 fn_8025DF78(_PLW*, u16, int);
u32 fn_80269394(void*);
u32 fn_802693C4(u8, int, s32);
u32 fn_80269474(u8, int, s32);
void fn_802695A4(u8, void*, void*);
void fn_80223258(_PLW*, u8);
void fn_80272B10(_PLW*, s32);
s8 fn_8027234C(u8*, u8);
void fn_802736A0(_PLW*);

/* One 0x14C-byte entry of the player-object table behind lbl_80794B28. */
struct _PLOBJ {
    /* 0x000 */ u8 unk000[0x14C];
};

/* The small-data pointer object the table hangs off (only the table base is named). */
struct _PLGLOBAL {
    /* 0x00 */ u8 unk00[0x10];
    /* 0x10 */ _PLOBJ* table;
};
extern _PLGLOBAL* lbl_80794B28;

extern u16 lbl_805C0198[];
extern u16 lbl_805C01B8[];
extern u8 lobby_w[];
extern u8 lbl_805C01C8[];
extern u8 lbl_80792140;
extern u8 lbl_80792148;
}

/* One 16-byte record of the skill-id table at `lbl_805C01C8`. */
struct _SKILLREC {
    /* 0x0 */ u16 id;
    /* 0x2 */ u8 unk2;
    /* 0x3 */ u8 unk3;
    /* 0x4 */ u8 unk4;
    /* 0x5 */ u8 unk5;
    /* 0x6 */ u8 unk6;
    /* 0x7 */ u8 unk7;
    /* 0x8 */ u8 unk8;
    /* 0x9 */ u8 unk9;
    /* 0xA */ u8 unkA;
    /* 0xB */ u8 unkB;
    /* 0xC */ u8 unkC;
    /* 0xD */ u8 unkD;
    /* 0xE */ u8 unkE;
    /* 0xF */ u8 unkF;
};

/* One equipment/decoration record; only the skill-id/skill-level pairs are named. */
struct _SKILLITEM {
    /* 0x0 */ u8 unk0[8];
    /* 0x8 */ u8 unk8;
    /* 0x9 */ u8 unk9;
    /* 0xA */ u8 unkA;
    /* 0xB */ u8 unkB;
};

bool Pl_master_ck(_PLW*);
bool Pl_Skill_ck(_PLW*, u16);
u32 Pl_cat_skill_ck(_PLW*, u16);
bool Pl_condition_ck(_PLW*, u32);

/* Sums the player's skill deltas into a display value and writes the rank flag to `out`. */
extern "C" u16 fn_80270018(_PLW* plw, u32 param, u8* out) {
    int value = (u16)param;

    *out = 0;
    if (Pl_master_ck(plw) == 1) {
        if (fn_802731B4(plw, 596) > 0) {
            value += 6;
            *out = 1;
        }
        if (fn_802731B4(plw, 597) > 0) {
            value += 9;
            *out = 1;
        }
        value += plw->unk448;
        value += plw->unk449;
        if (Pl_Skill_ck(plw, 74) == 1) {
            value += 10;
            *out = 1;
        } else if (Pl_Skill_ck(plw, 75) == 1) {
            value += 15;
            *out = 1;
        } else if (Pl_Skill_ck(plw, 76) == 1) {
            value += 20;
            *out = 1;
        }
        if (Pl_Skill_ck(plw, 77) == 1) {
            value += -5;
            *out = 2;
        } else if (Pl_Skill_ck(plw, 78) == 1) {
            value += -10;
            *out = 2;
        } else if (Pl_Skill_ck(plw, 79) == 1) {
            value += -15;
            *out = 2;
        }
        if (plw->unk370 <= 10 && Pl_cat_skill_ck(plw, 18) == 1) {
            value = (int)(lbl_8079A030 * value);
            *out = 1;
        } else if (fn_8026FFBC(plw) == 1) {
            if (Pl_Skill_ck(plw, 165) == 1) {
                value = (int)(lbl_8079A034 * value);
                *out = 2;
            } else if (Pl_Skill_ck(plw, 164) == 1) {
                value = (int)(lbl_8079A038 * value);
                *out = 1;
            }
        }
        if (Pl_Skill_ck(plw, 202) == 1) {
            if (plw->unk446 >= 2) {
                value = (int)(lbl_8079A03C * value);
            } else if (plw->unk446 >= 1) {
                value = (int)(lbl_8079A040 * value);
            }
        }
        if (value <= 0) {
            value = 1;
        }
        if (*out != 0) {
            if ((u16)value >= (u16)param) {
                *out = 1;
            } else {
                *out = 2;
            }
        }
    }
    return (u16)value;
}

/* Recomputes the player's skill point total and the fraction of its 700 point cap. */
extern "C" void fn_8027035C(_PLW* plw) {
    if (Pl_master_ck(plw) == 1) {
        u8 flag;
        int v = fn_80273ED8(plw, 6, 0);

        plw->unk3B8 = fn_80270018(plw, (u16)v, &flag);
    }
    if (plw->unk3B8 >= 700) {
        plw->unk3B8 = 700;
    }
    plw->unk3BC = (f32)plw->unk3B8 / lbl_8079A044;
}

/* Recomputes the second skill set's point total and its rank. */
extern "C" u16 fn_802703F4(_PLW* plw, u32 param, u8* out) {
    int value = (u16)param;

    *out = 0;
    if (Pl_master_ck(plw) == 1) {
        if (fn_802731B4(plw, 598) > 0) {
            value += 8;
            *out = 1;
        }
        if (fn_802731B4(plw, 599) > 0) {
            value += 12;
            *out = 1;
        }
        value += plw->unk44C;
        value += plw->unk44D;
        if (Pl_Skill_ck(plw, 80) == 1) {
            value += 10;
            *out = 1;
        } else if (Pl_Skill_ck(plw, 81) == 1) {
            value = (int)(lbl_8079A050 * value);
            value += 10;
            *out = 1;
        } else if (Pl_Skill_ck(plw, 82) == 1) {
            value = (int)(lbl_8079A040 * value);
            value += 10;
            *out = 1;
        } else if (Pl_Skill_ck(plw, 83) == 1) {
            value += -10;
            *out = 2;
        } else if (Pl_Skill_ck(plw, 84) == 1) {
            value = (int)(lbl_8079A054 * value);
            value += -10;
            *out = 2;
        } else if (Pl_Skill_ck(plw, 85) == 1) {
            value = (int)(lbl_8079A058 * value);
            value += -10;
            *out = 2;
        }
        if (fn_8026FFBC(plw) == 1) {
            *out = 1;
            if (Pl_Skill_ck(plw, 165) == 1) {
                value += 21;
            } else if (Pl_Skill_ck(plw, 163) == 1 || Pl_Skill_ck(plw, 164) == 1) {
                value += 45;
            } else {
                value += 30;
            }
        }
        if (plw->unk370 <= 10 && Pl_cat_skill_ck(plw, 18) == 1) {
            value = (int)(lbl_8079A05C * value);
            *out = 1;
        }
        if (value <= 0) {
            value = 1;
        }
        if (plw->unk422 > 0) {
            value -= value / 5;
            if (value < 1) {
                value = 1;
            }
            *out = 2;
        }
    }
    return (u16)value;
}

/* Recomputes the second skill set's point meter, clamped to a minimum of ten points. */
extern "C" void fn_80270728(_PLW* plw) {
    if (Pl_master_ck(plw) == 1) {
        u8 flag;
        int v = fn_80273ED8(plw, 0, 0) + 1;

        plw->unk3BA = fn_802703F4(plw, (u16)v, &flag);
    }
    plw->unk3C0 = (f32)plw->unk3BA;
    if (plw->unk3C0 < lbl_8079A010) {
        plw->unk3C0 = lbl_8079A010;
    }
}

/*
 * Applies one weapon-type skill group to one attack-bonus field: the +20/+15/+10 levels of the group's
 * three positive skills, then the -15/-10 levels of its two negative ones.
 */
#define SKILL_GROUP(field, base)                                 if (Pl_Skill_ck(plw, base) == 1) {                               plw->field += lbl_8079A060;                                     } else if (Pl_Skill_ck(plw, base - 1) == 1) {                    plw->field += lbl_8079A064;                                     } else if (Pl_Skill_ck(plw, base - 2) == 1) {                    plw->field += lbl_8079A010;                                     }                                                            if (Pl_Skill_ck(plw, base + 2) == 1) {                           plw->field -= lbl_8079A064;                                     } else if (Pl_Skill_ck(plw, base + 1) == 1) {                    plw->field -= lbl_8079A010;                                     }

/* Recomputes the five weapon-type attack-bonus fields from the player's active skills. */
extern "C" void fn_802707B4(_PLW* plw) {
    SKILL_GROUP(unk3C4, 96)
    SKILL_GROUP(unk3C8, 102)
    SKILL_GROUP(unk3CC, 108)
    SKILL_GROUP(unk3D0, 120)
    SKILL_GROUP(unk3D4, 114)
}

/* Clamps the five attack-bonus fields to the +-99 range the display and the status code use. */
extern "C" void fn_80270B98(_PLW* plw) {
    if (plw->unk3C4 < lbl_8079A068) {
        plw->unk3C4 = lbl_8079A068;
    }
    if (plw->unk3C8 < lbl_8079A068) {
        plw->unk3C8 = lbl_8079A068;
    }
    if (plw->unk3CC < lbl_8079A068) {
        plw->unk3CC = lbl_8079A068;
    }
    if (plw->unk3D0 < lbl_8079A068) {
        plw->unk3D0 = lbl_8079A068;
    }
    if (plw->unk3D4 < lbl_8079A068) {
        plw->unk3D4 = lbl_8079A068;
    }
    if (plw->unk3C4 > lbl_8079A06C) {
        plw->unk3C4 = lbl_8079A06C;
    }
    if (plw->unk3C8 > lbl_8079A06C) {
        plw->unk3C8 = lbl_8079A06C;
    }
    if (plw->unk3CC > lbl_8079A06C) {
        plw->unk3CC = lbl_8079A06C;
    }
    if (plw->unk3D0 > lbl_8079A06C) {
        plw->unk3D0 = lbl_8079A06C;
    }
    if (plw->unk3D4 > lbl_8079A06C) {
        plw->unk3D4 = lbl_8079A06C;
    }
}

/* The defence delta of one armour-piece slot: -1 for a negative skill, +1 for a positive one. */
extern "C" f32 fn_80270C64(s16* table, s16 slot) {
    f32 delta = lbl_8079A000;

    if (((s16*)((u8*)table + 0x42E))[slot] > 0) {
        delta -= lbl_8079A070;
    }
    if (((s16*)((u8*)table + 0x438))[slot] > 0) {
        delta += lbl_8079A074;
    }
    return delta;
}

/* Rebuilds the five attack-bonus fields from the player's armour skills and the weapon's own bonuses. */
extern "C" void fn_80270CA4(_PLW* plw) {
    plw->unk3C4 = (f32)(s16)fn_80273ED8(plw, 1, 0) + fn_80270C64((s16*)plw, 0);
    plw->unk3C8 = (f32)(s16)fn_80273ED8(plw, 2, 0) + fn_80270C64((s16*)plw, 1);
    plw->unk3CC = (f32)(s16)fn_80273ED8(plw, 3, 0) + fn_80270C64((s16*)plw, 2);
    plw->unk3D0 = (f32)(s16)fn_80273ED8(plw, 4, 0) + fn_80270C64((s16*)plw, 4);
    plw->unk3D4 = (f32)(s16)fn_80273ED8(plw, 5, 0) + fn_80270C64((s16*)plw, 3);
    fn_802707B4(plw);
    plw->unk3C4 += (f32)(s16)fn_802753E4(plw, 6);
    plw->unk3C8 += (f32)(s16)fn_802753E4(plw, 7);
    plw->unk3CC += (f32)(s16)fn_802753E4(plw, 8);
    plw->unk3D0 += (f32)(s16)fn_802753E4(plw, 10);
    plw->unk3D4 += (f32)(s16)fn_802753E4(plw, 9);
    if (Pl_condition_ck(plw, 256) == 1) {
        plw->unk3D4 -= lbl_8079A070;
    }
    fn_80270B98(plw);
}

/*
 * Swaps the player's nine equipment slots for another set, re-derives every skill field from it, and
 * fills in the skill summary the status screen reads. Passing a null equipment set only fills the
 * summary in.
 */
extern "C" void fn_80270F50(_PLW* plw, _EQUIP* equip, u8* out) {
    _EQUIP saved[9];
    int i;

    if (equip != 0) {
        fn_8004A20C(&saved[0], &plw->equipA[0]);
        fn_8004A20C(&saved[1], &plw->equipA[1]);
        fn_8004A20C(&saved[2], &plw->equipA[2]);
        fn_8004A20C(&saved[3], &plw->equipA[3]);
        fn_8004A20C(&saved[4], &plw->equipA[4]);
        fn_8004A20C(&saved[5], &plw->equipA[5]);
        fn_8004A20C(&saved[6], &plw->equipB);
        fn_8004A20C(&saved[7], &plw->equipC);
        fn_8004A20C(&saved[8], &plw->equipD);
        fn_8004A20C(&plw->equipA[0], &equip[0]);
        fn_8004A20C(&plw->equipA[1], &equip[1]);
        fn_8004A20C(&plw->equipA[2], &equip[2]);
        fn_8004A20C(&plw->equipA[3], &equip[3]);
        fn_8004A20C(&plw->equipA[4], &equip[4]);
        fn_8004A20C(&plw->equipA[5], &equip[5]);
        fn_8004A20C(&plw->equipB, &equip[6]);
        fn_8004A20C(&plw->equipC, &equip[7]);
        fn_8004A20C(&plw->equipD, &equip[8]);
        fn_80272A08(plw);
        for (int i = 0; i < 8; i++) {
            plw->unk61A[i] = plw->unk5F2[i];
        }
        fn_8027885C(plw, 0, 0);
        fn_802789EC(plw, 0);
        fn_8027035C(plw);
        fn_80270728(plw);
        fn_80270CA4(plw);
    }
    out[10] = 0;
    *(u16*)(out + 12) = 0;
    for (i = 1; i < 6; i++) {
        s16 v = (s16)fn_80273ED8(plw, lbl_805C5FC8[i], 0);

        if (v > 0) {
            out[10] = (u8)i;
            *(s16*)(out + 12) = v;
            break;
        }
    }
    out[11] = 0;
    *(u16*)(out + 14) = 0;
    for (i = 0; i < 3; i++) {
        s16 v = (s16)fn_80273ED8(plw, lbl_805C5FE0[i], 0);

        if (v > 0) {
            out[11] = (u8)(i + 12);
            *(s16*)(out + 14) = v;
            break;
        }
    }
    *(s16*)(out + 0) = plw->unk372;
    *(s16*)(out + 2) = plw->unk37A;
    *(s16*)(out + 4) = plw->unk380;
    *(u16*)(out + 6) = plw->unk3B8;
    *(u16*)(out + 8) = plw->unk3BA;
    *(s16*)(out + 16) = (s16)plw->unk3C4;
    *(s16*)(out + 18) = (s16)plw->unk3C8;
    *(s16*)(out + 20) = (s16)plw->unk3CC;
    *(s16*)(out + 24) = (s16)plw->unk3D0;
    *(s16*)(out + 22) = (s16)plw->unk3D4;
    for (i = 0; i < 7; i++) {
        out[26 + i] = 0;
    }
    if (equip != 0) {
        fn_8004A20C(&plw->equipA[0], &saved[0]);
        fn_8004A20C(&plw->equipA[1], &saved[1]);
        fn_8004A20C(&plw->equipA[2], &saved[2]);
        fn_8004A20C(&plw->equipA[3], &saved[3]);
        fn_8004A20C(&plw->equipA[4], &saved[4]);
        fn_8004A20C(&plw->equipA[5], &saved[5]);
        fn_8004A20C(&plw->equipB, &saved[6]);
        fn_8004A20C(&plw->equipC, &saved[7]);
        fn_8004A20C(&plw->equipD, &saved[8]);
        fn_80272A08(plw);
        for (int i = 0; i < 8; i++) {
            plw->unk61A[i] = 0;
        }
        fn_8027885C(plw, 0, 0);
        fn_802789EC(plw, 0);
        fn_8027035C(plw);
        fn_80270728(plw);
        fn_80270CA4(plw);
    }
}

/* Whether the player's active skill set contains `skill`. */
bool Pl_Skill_ck(_PLW* plw, u16 skill) {
    bool ok = false;
    int i;

    if (fn_800CF208() == 2) {
        if (lobby_w[0] == 6) {
            ok = true;
        } else if (lobby_w[0] == 15 && fn_80363A2C() == 1) {
            ok = true;
        }
    }
    if (ok == 1) {
        for (int i = 0; i < 8; i++) {
            if (skill == plw->unk61A[i]) {
                return true;
            }
        }
    } else {
        for (int i = 0; i < 8; i++) {
            if (skill == plw->unk5F2[i]) {
                return true;
            }
        }
    }
    return false;
}

/* Whether `skill` is one of the player's eight base skill ids. */
extern "C" u32 fn_802714F0(_PLW* plw, u16 skill) {
    int i;

    for (i = 0; i < 8; i++) {
        if (skill == plw->unk5F2[i]) {
            return 1;
        }
    }
    return 0;
}

/* The level of the skill in one slot: the live set in the lobby menu, the base set otherwise. */
extern "C" u32 fn_802715A0(_PLW* plw, u32 slot) {
    int i;

    if (fn_800CF208() == 2 && lobby_w[0] == 6) {
        for (int i = 0; i < 8; i++) {
            if ((u8)slot == plw->unk62A[i]) {
                return (u8)plw->unk61A[i];
            }
        }
    } else {
        for (int i = 0; i < 8; i++) {
            if ((u8)slot == plw->unk602[i]) {
                return (u8)plw->unk5F2[i];
            }
        }
    }
    return 0;
}

/* Whether the player currently provides `kind` of skill: kind 15-17 ask the equipment, 74 the player
 * type, and anything else the two skill-id tables. */
extern "C" u32 fn_80271674(_PLW* plw, u8 kind) {
    u16 mode = (fn_8026FE44(plw) == 1);

    switch (kind) {
    case 15:
    case 16:
    case 17:
        return (u32)(fn_80274AB8((int)plw) == 1);
    case 74: {
        int type = plw->unk2;

        if ((u32)(type - 7) <= 1 || type == 0 || type == 2) {
            return 1;
        }
        return 0;
    }
    default: {
        u16* table;

        if (mode == 0) {
            table = lbl_805C0198;
        } else {
            table = lbl_805C01B8;
        }
        while (*table != 0xFFFF) {
            if (kind == *table++) {
                return 0;
            }
        }
        return 1;
    }
    }
}

/* Whether the player's active skill set satisfies one skill query: the equipment-provided kinds
 * 7-10/14-15, the level query kinds 11-13, and the 74 player-type special case. */
extern "C" u32 fn_8027176C(_PLW* plw, u8* p, u32 a, u8 kind) {
    u32 lvl;
    u32 mode;

    if (p == 0) {
        return 0;
    }
    {
        u8 v = *p;

        switch (v) {
        case 7:
        case 8:
        case 9:
        case 10:
        case 14:
        case 15:
            lvl = fn_8027E29C(v);
            mode = 0;
            break;
        case 11:
        case 12:
        case 13:
            lvl = 6;
            mode = 1;
            break;
        default:
            return fn_80271674(plw, kind);
        }
    }
    switch (kind) {
    case 15:
    case 16:
    case 17:
        return (fn_80274AEC(p, a, lvl) - 1) == 0;
    case 74: {
        u8 l = (u8)lvl;

        if ((u32)(l - 7) <= 1 || (s32)l == 0 || (s32)l == 2) {
            return 1;
        }
        return 0;
    }
    default: {
        u16* table;

        if (mode == 0) {
            table = lbl_805C0198;
        } else {
            table = lbl_805C01B8;
        }
        while (*table != 0xFFFF) {
            if (kind == *table++) {
                return 0;
            }
        }
        return 1;
    }
    }
}

/* Maps a display skill id to its internal id through the 16-byte record table, or 0 when absent. */
extern "C" u8 fn_802718C8(u16 skill) {
    _SKILLREC* rec = (_SKILLREC*)lbl_805C01C8;

    if (skill == 0) {
        return 0;
    }
    while (rec->id != 0xFFFF) {
        if (rec->unk3 == skill) {
            return (u8)rec->id;
        }
        if (rec->unk5 == skill) {
            return (u8)rec->id;
        }
        if (rec->unk7 == skill) {
            return (u8)rec->id;
        }
        if (rec->unk9 == skill) {
            return (u8)rec->id;
        }
        if (rec->unkB == skill) {
            return (u8)rec->id;
        }
        if (rec->unkD == skill) {
            return (u8)rec->id;
        }
        rec++;
    }
    return 0;
}

/* The one-argument form of fn_80271674: the skill is resolved through the id table first. */
extern "C" void fn_80271978(_PLW* plw, u16 skill) {
    fn_80271674(plw, fn_802718C8(skill));
}

/* The three-argument form of fn_8027176C: the skill is resolved through the id table first. */
extern "C" void fn_802719B8(_PLW* plw, u8* p, u32 a, u16 skill) {
    fn_8027176C(plw, p, a, fn_802718C8(skill));
}

/* Whether a skill is available in the current quest context: three kinds are gated on the
 * quest mode, one on the player type, everything else is always available. */
extern "C" u32 fn_80271A18(_PLW* plw, u16 skill) {
    u16 mode = (fn_8026FE44(plw) == 1);

    switch (skill) {
    case 15:
    case 30:
        if (mode == 0) {
            return 0;
        }
        break;
    case 19:
    case 38:
    case 50:
        if (mode == 1) {
            return 0;
        }
        break;
    case 40:
        if ((u32)(plw->unk2 - 7) <= 1) {
            return 0;
        }
        break;
    default:
        break;
    }
    return 1;
}

/* The skill level an equipment record contributes for `skill`, or 0. */
extern "C" s8 fn_80271AD8(u16 item, u8 skill) {
    _SKILLITEM* rec = (_SKILLITEM*)fn_8029F6B4(item);
    s8 lvl = 0;

    if (rec != 0) {
        if (rec->unk8 == skill) {
            lvl = (s8)rec->unk9;
        }
        if (rec->unkA == skill) {
            lvl += rec->unkB;
        }
    }
    return lvl;
}

/* Whether an equipment record provides `skill` at a non-zero level. */
extern "C" u32 fn_80271B4C(u16 item, u8 skill) {
    _SKILLITEM* rec = (_SKILLITEM*)fn_8029F6B4(item);
    u32 ok = 0;

    if (rec != 0) {
        if (rec->unk8 == skill) {
            if ((s8)rec->unk9 != 0) {
                ok = 1;
            }
        }
        if (rec->unkA == skill) {
            if ((s8)rec->unkB != 0) {
                ok = 1;
            }
        }
    }
    return ok;
}

/* The 4-byte slot-table entry at 0x278: an item id and a signed value. */
#define SLOT_IDX(slot) (((slot) & 0x80) ? (((slot) & 0x7F) + 26) : (slot))

/* Appends one (value, a, b) triple to a bounded parallel-array set and bumps the count. */
extern "C" void fn_802724E8(u16* table, s8* a, u8* b, u8* count, u16 v, u8 bval, s8 aval) {
    if (v == 0) {
        return;
    }
    if (*count >= 8) {
        return;
    }
    table[*count] = v;
    a[*count] = aval;
    b[*count] = bval;
    *count += 1;
}

/* The saved-skill set behind fn_80272A08: looks each skill id up in the 16-byte record table, keeps the
 * rank the player's equipment reaches, converts it to a display value and sorts the eight entries. */
extern "C" void fn_8027252C(_EQUIP* equips, u16* table, s8* a, u8* b) {
    _SKILLREC* rec = (_SKILLREC*)lbl_805C01C8;
    s16 tv[8];
    u8 tb[8];
    u8 ta[8];
    u8 count = 0;
    s8 level;
    int i;
    int j;

    for (i = 0; i < 8; i++) {
        table[i] = 0;
        b[i] = 0;
        a[i] = 0;
    }
    while (rec->id != 0xFFFF) {
        level = fn_8027234C((u8*)equips, (u8)rec->id);
        if (level <= (s8)rec->unk2) {
            fn_802724E8(table, a, b, &count, (u16)rec->unk3, (u8)rec->id, level);
        } else if (level <= (s8)rec->unk4) {
            fn_802724E8(table, a, b, &count, (u16)rec->unk5, (u8)rec->id, level);
        } else if (level <= (s8)rec->unk6) {
            fn_802724E8(table, a, b, &count, (u16)rec->unk7, (u8)rec->id, level);
        } else if (level >= (s8)rec->unkC) {
            fn_802724E8(table, a, b, &count, (u16)rec->unkD, (u8)rec->id, level);
        } else if (level >= (s8)rec->unkA) {
            fn_802724E8(table, a, b, &count, (u16)rec->unkB, (u8)rec->id, level);
        } else if (level >= (s8)rec->unk8) {
            fn_802724E8(table, a, b, &count, (u16)rec->unk9, (u8)rec->id, level);
        }
        rec++;
    }
    for (i = 0; i < 8; i++) {
        s8 v = (s8)a[i];

        if (v != 0) {
            if (v > 0) {
                tv[i] = (s16)(v + 200);
                tb[i] = b[i];
                ta[i] = (u8)table[i];
            } else {
                tv[i] = (s16)((s16)(-v) + 100);
                tb[i] = b[i];
                ta[i] = (u8)table[i];
            }
        } else {
            tv[i] = 0;
            tb[i] = 0;
            ta[i] = 0;
        }
    }
    for (i = 0; i < 7; i++) {
        for (j = i + 1; j < 8; j++) {
            if (tv[i] < tv[j] || (tv[i] == tv[j] && tb[i] < tb[j])) {
                s16 s = tv[i];
                u8 t = tb[i];

                tv[i] = tv[j];
                tb[i] = tb[j];
                tv[j] = s;
                tb[j] = t;
                {
                    u8 u = ta[i];

                    ta[i] = ta[j];
                    ta[j] = u;
                }
            }
        }
    }
    for (i = 0; i < 8; i++) {
        a[i] = (s8)tv[i];
        b[i] = tb[i];
        table[i] = ta[i];
    }
}

/* Whether `skill` is one of the four decoration-slot skill ids. */
u32 Pl_cat_skill_ck(_PLW* plw, u16 skill) {
    if (skill == plw->unk612) {
        return 1;
    }
    if (skill == plw->unk614) {
        return 1;
    }
    {
        _PLW* q = (_PLW*)((u8*)plw + 4);

        if (skill == q->unk612) {
            return 1;
        }
        if (skill == q->unk614) {
            return 1;
        }
    }
    return 0;
}

/* The item id in one equipment slot, or 0 for the empty slot. */
extern "C" u16 fn_80272C80(_PLW* plw, u8 slot) {
    if (slot == 0xFF) {
        return 0;
    }
    if (slot & 0x80) {
        return *(u16*)((u8*)plw + ((slot & 0x7F) + 26) * 4 + 0x278);
    }
    return *(u16*)((u8*)plw + slot * 4 + 0x278);
}

/* The signed skill value of one equipment slot, or 20 for the fixed-value item. */
extern "C" s16 fn_80272CC8(_PLW* plw, u8 slot) {
    if (slot == 0xFF) {
        return 0;
    }
    if (fn_80272C80(plw, slot) == 0x35) {
        return 20;
    }
    if (slot & 0x80) {
        return *(s16*)((u8*)plw + ((slot & 0x7F) + 26) * 4 + 0x27A);
    }
    return *(s16*)((u8*)plw + slot * 4 + 0x27A);
}

/* Recomputes the current slot's skill value and lowers the running minimum. */
extern "C" void fn_80272D5C(_PLW* plw) {
    s16 v = fn_80272CC8(plw, (u8)plw->unk26E);

    plw->unk270 = v;
    plw->unk272 = v;
    if (v < plw->unk269) {
        plw->unk269 = (u8)v;
        plw->unk274 = (u8)v;
    }
}

/* The weapon-type slot count for one equipment slot, 2 for the empty slot. */
extern "C" s32 fn_80272DB4(_PLW* plw, u16 slot) {
    u16 id = fn_80273044(plw, slot);

    if (id == 0xFFFF) {
        return 2;
    }
    return fn_8004BD30((s16*)((u8*)plw + SLOT_IDX(id) * 4 + 0x278));
}

/* Clears the four per-set skill summary words. */
extern "C" void fn_8027346C(_PLW* plw) {
    plw->unk634 = 0;
    plw->unk638 = 0;
    plw->unk63C = 0;
    plw->unk640 = 0;
}

/* Removes the equipment set selected by `idx` and clears its valid bit. */
extern "C" void fn_8027373C(_PLW* plw, u8 idx) {
    u32 mask = 1 << idx;

    if ((plw->unk250 & mask) == 0) {
        return;
    }
    fn_8004A20C((_EQUIP*)((u8*)plw + idx * 12 + 0x140), (_EQUIP*)((u8*)plw + idx * 12 + 0x188));
    {
        u16 clear = (u16)~mask;

        plw->unk250 = plw->unk250 & clear;
    }
}

/* One byte of the seven-entry decoration skill-id table. */
extern "C" u8 fn_802738B8(u8 idx) {
    if (idx >= 7) {
        return 0;
    }
    return (&lbl_80792140)[idx];
}

/* One byte of the second seven-entry decoration table. */
extern "C" u8 fn_802738D8(u8 idx) {
    return (&lbl_80792148)[idx];
}

/* Stores one skill value into a set slot and updates its valid bit. */
extern "C" void fn_80273998(_PLW* plw, u8 idx, s32 val) {
    if (val == plw->unk218[idx]) {
        plw->unk252 = plw->unk252 & (u16)~(1 << idx);
        return;
    }
    plw->unk234[idx] = val;
    plw->unk252 = plw->unk252 | (u16)(1 << idx);
}

/* Copies the player's nine equipment slots into a local save area and rebuilds the skill set from it. */
extern "C" void fn_80272A08(_PLW* plw) {
    _EQUIP saved[9];

    fn_8004A20C(&saved[0], &plw->equipA[0]);
    fn_8004A20C(&saved[1], &plw->equipA[1]);
    fn_8004A20C(&saved[2], &plw->equipA[2]);
    fn_8004A20C(&saved[3], &plw->equipA[3]);
    fn_8004A20C(&saved[4], &plw->equipA[4]);
    fn_8004A20C(&saved[5], &plw->equipA[5]);
    fn_8004A20C(&saved[6], &plw->equipB);
    fn_8004A20C(&saved[7], &plw->equipC);
    fn_8004A20C(&saved[8], &plw->equipD);
    fn_8027252C(&saved[0], plw->unk5F2, (s8*)plw->unk60A, plw->unk602);
}

/* Re-selects the skill slot: keeps the current one when it still holds the same skill id, otherwise
 * falls back through the resolved-slot table and drops it when the item is not a valid one. */
extern "C" void fn_80272B10(_PLW* plw, s32 slot) {
    u16* ent;
    u32 v;

    if (slot & 0x80) {
        ent = (u16*)((u8*)plw + ((slot & 0x7F) + 26) * 4 + 0x278);
    } else {
        ent = (u16*)((u8*)plw + slot * 4 + 0x278);
    }
    v = fn_80274B20(*ent);
    if (slot != plw->unk26E) {
        if (plw->unk26C != (u8)v) {
            return;
        }
        plw->unk26E = (u16)slot;
    }
    if (slot == plw->unk26E) {
        plw->unk26E = fn_8027993C(plw, (u16)slot, 2);
        fn_80279B84(plw);
        plw->unk272 = plw->unk270;
        plw->unk275 = plw->unk26A;
        plw->unk274 = 0;
        return;
    }
    if (fn_8026FE44(plw) != 0) {
        u16 cur = plw->unk26E;

        if (cur == 0xFF) {
            plw->unk26E = fn_8027993C(plw, 0, 2);
            fn_80279B84(plw);
            return;
        }
        u8* item;

        if (cur & 0x80) {
            item = GetItemData__FUs(plw->unk278[(cur & 0x7F) + 26].unk0);
        } else {
            item = GetItemData__FUs(plw->unk278[cur].unk0);
        }
        if (item[0] != 1) {
            plw->unk26E = fn_8027993C(plw, plw->unk26E, 0);
            fn_80279B84(plw);
        }
    }
}

/* Resolves the equipment slot for a changed slot and re-selects the skill to display. */
extern "C" s16 fn_80272E30(_PLW* plw, u16 item, s16 value) {
    u8* data = GetItemData__FUs(item);
    u16 cur = fn_80273044(plw, item);
    s16 v;
    u16 slot;

    if (data[0] == 1 && fn_8026FE44(plw) == 1 && (cur == 0xFFFF || (cur & 0x80) != 0)) {
        v = fn_8004BA3C(item, value, plw->unk2E0, 8, 1, 0);
        slot = fn_80273044(plw, item);
        if ((u32)v <= 4) {
            goto found;
        }
    }
    v = fn_8004BA3C(item, value, plw->unk278, 24, 1, 0);
    slot = fn_80273044(plw, item);
found:
    switch (v) {
    case 0:
        {
            u8* d = GetItemData__FUs(plw->unk278[plw->unk304].unk0);

            if ((d[2] & 8) == 0 || d[0] == 1) {
                plw->unk304 = fn_8025DF78(plw, plw->unk304, 0);
            }
        }
        fn_80272B10(plw, slot);
        break;
    case 1:
    case 2:
    case 3:
        if (data[0] == 1 && plw->unk26E == slot) {
            fn_80272D5C(plw);
        }
        break;
    case 4:
        if (data[0] == 1) {
            if (fn_8027993C(plw, plw->unk26E, 1) == 0xFF) {
                plw->unk26E = 0xFF;
                plw->unk270 = 0;
                plw->unk26A = 0;
                plw->unk269 = 0;
            } else if (plw->unk26E == cur) {
                plw->unk270 = 0;
                plw->unk26A = 0;
                plw->unk269 = 0;
            }
            if (plw->unk26E == cur) {
                fn_80272D5C(plw);
            }
        }
        break;
    }
    return v;
}

/* The value one equipment slot contributes to `value`, clamped to the item's own limit. */
extern "C" s32 fn_80273228(_PLW* plw, u16 item, s16 value) {
    u16 slot = fn_80273044(plw, item);
    u8* data = GetItemData__FUs(item);
    s16 ent;

    if (slot != 0xFFFF) {
        if (slot & 0x80) {
            ent = *(s16*)((u8*)plw + ((slot & 0x7F) + 26) * 4 + 0x27A);
            if ((s16)value + ent >= data[3]) {
                return data[3] - ent;
            }
            return value;
        }
        ent = *(s16*)((u8*)plw + slot * 4 + 0x27A);
        if ((s16)value + ent >= data[3]) {
            return data[3] - ent;
        }
        return value;
    }
    if (data[0] == 1 && fn_8026FE44(plw) == 1) {
        for (int i = 0; i < 8; i++) {
            if (plw->unk2E0[i].unk0 == 0) {
                return value;
            }
        }
    }
    for (int j = 0; j < 3; j++) {
        for (int k = 0; k < 8; k++) {
            if (plw->unk278[j * 8 + k].unk0 == 0) {
                return value;
            }
        }
    }
    return 0;
}

/* Flushes the skill sets whose cached values no longer resolve, and the decoration slots. */
extern "C" void fn_8027350C(_PLW* plw, s32 arg) {
    u32 ok;
    u8 id;
    u32 mask;
    u32 val;
    u8 i;

    if (!(plw->unk252 != 0 || plw->unk250 != 0 || plw->unk254 != 0)) {
        return;
    }
    ok = fn_80269394(lbl_80794B28->table + plw->unk8);
    for (i = 0; i < 7; i++) {
        id = fn_802738B8(i);
        mask = 1 << i;
        if (plw->unk252 & mask) {
            if (ok == 1) {
                if (*(u32*)((u8*)plw->unk13C + i * 0x164 + 0x11C) == 0) {
                    val = fn_802693C4(plw->unk8, i, plw->unk234[i]);
                } else {
                    val = fn_80269474(plw->unk8, i, plw->unk234[i]);
                }
                if (val == 1) {
                    plw->unk252 &= (u16)~mask;
                    plw->unk218[i] = plw->unk234[i];
                    if (id != 0xFF) {
                        fn_8027373C(plw, id);
                    }
                }
            }
        } else {
            plw->unk252 &= (u16)~mask;
            if (id != 0xFF) {
                fn_8027373C(plw, id);
                fn_80223258(plw, id);
            }
        }
    }
    if (plw->unk254 != 0 && ok == 1) {
        fn_802695A4(plw->unk8, (u8*)plw + 0x1DC, (u8*)plw + 0x200);
        fn_802736A0(plw);
    }
}

/* Clears every derived skill array and the three valid-bit words. */
extern "C" void fn_80273484(_PLW* plw) {
    memset((u8*)plw + 0x188, 0, 72);
    memset((u8*)plw + 0x1DC, 0, 12);
    memset((u8*)plw + 0x200, 0, 24);
    memset((u8*)plw + 0x218, -1, 28);
    memset((u8*)plw + 0x234, -1, 28);
    plw->unk250 = 0;
    plw->unk252 = 0;
    plw->unk254 = 0;
}

/* Re-derives the player type after restoring the two decoration equipment slots. */
extern "C" void fn_802736A0(_PLW* plw) {
    _EQUIP* src;
    _EQUIP* dst;
    int i;

    if (plw->unk254 == 0) {
        return;
    }
    fn_8004A20C((_EQUIP*)((u8*)plw + 0x1D0), (_EQUIP*)((u8*)plw + 0x1DC));
    dst = (_EQUIP*)((u8*)plw + 0x1E8);
    src = (_EQUIP*)((u8*)plw + 0x200);
    for (i = 0; i < 2; i++) {
        fn_8004A20C(dst, src);
        dst++;
        src++;
    }
    plw->unk254 = 0;
    plw->unk2 = Get_pl_type__FP6_EQUIPP6_EQUIP((_EQUIP*)((u8*)plw + 0x1D0), (_EQUIP*)((u8*)plw + 0x1E8));
}

/* The skill level one player equipment record contributes for `skill`: a per-kind record layout,
 * with the per-decoration levels summed for the kinds that carry them. */
extern "C" s8 fn_80271BD4(u8* rec, u8 skill) {
    s8 v = 0;

    if (*(u16*)(rec + 2) == 0 || skill == 0) {
        return 0;
    }
    {
    u32 kind = rec[0];

    if ((u32)(kind - 7) <= 8) {
        int i;
        u8* p = rec;

        for (i = 0; i < 3; i++) {
            if (*(u16*)(p + 6) != 0) {
                v += fn_80271AD8(*(u16*)(p + 6), skill);
            }
            p += 2;
        }
    } else if ((u32)(kind - 1) <= 4) {
        u8* r = (u8*)fn_8027E344();
        int i;
        u8* p = rec;

        if (r[14] == skill) {
            v = (s8)r[15];
        }
        if (r[16] == skill) {
            v += r[17];
        }
        if (r[18] == skill) {
            v += r[19];
        }
        if (r[20] == skill) {
            v += r[21];
        }
        if (r[22] == skill) {
            v += r[23];
        }
        for (i = 0; i < 3; i++) {
            if (*(u16*)(p + 6) != 0) {
                v += fn_80271AD8(*(u16*)(p + 6), skill);
            }
            p += 2;
        }
    } else if (kind == 6) {
        u8 n = rec[1];
        int i;
        u8* p = rec;

        for (i = 0; i < rec[1]; i++) {
            if (*(u16*)(p + 6) != 0) {
                v += fn_80271AD8(*(u16*)(p + 6), skill);
            }
            p += 2;
        }
        if (n < 3) {
            if (*(u16*)(rec + n * 2 + 6) == skill) {
                v += (s8)((s8)(u8)*(u16*)(rec + 4) - 10);
            }
            if (n + 1 < 3 && *(u16*)(rec + (n + 1) * 2 + 6) == skill) {
                v += (s8)((s8)(u8)(*(u16*)(rec + 4) >> 8) - 10);
            }
        }
    }
    }
    return v;
}

/* Scales the skill level of one record by how many of the nine player records carry it. */
extern "C" s8 fn_80272084(u8* rec, u8 skill) {
    s8 v = fn_80271BD4(rec, skill);

    if (v != 0) {
        s8 n = 1;

        if (fn_80271BD4(rec + 0x30, 1)) {
            n = 2;
        }
        if (fn_80271BD4(rec + 0x00, 1)) {
            n++;
        }
        if (fn_80271BD4(rec + 0x0C, 1)) {
            n++;
        }
        if (fn_80271BD4(rec + 0x18, 1)) {
            n++;
        }
        if (fn_80271BD4(rec + 0x24, 1)) {
            n++;
        }
        if (fn_80271BD4(rec + 0x3C, 1)) {
            n++;
        }
        if (fn_80271BD4(rec + 0x48, 1)) {
            n++;
        }
        if (rec[0x54] == 12 && fn_80271BD4(rec + 0x54, 1)) {
            n++;
        }
        if (rec[0x60] == 13 && fn_80271BD4(rec + 0x60, 1)) {
            n++;
        }
        v = v * n;
    }
    return v;
}

/* Whether one player equipment record provides `skill` at a non-zero level. */
extern "C" u32 fn_80271E0C(u8* rec, u8 skill) {
    u32 ok = 0;

    if (*(u16*)(rec + 2) == 0 || skill == 0) {
        return 0;
    }
    {
        u32 kind = rec[0];

        if ((u32)(kind - 7) <= 8) {
            int i;
            u8* p = rec;

            for (i = 0; i < 3; i++) {
                if (*(u16*)(p + 6) != 0 && fn_80271B4C(*(u16*)(p + 6), skill) == 1) {
                    ok = 1;
                }
                p += 2;
            }
        } else if ((u32)(kind - 1) <= 4) {
            u8* r = (u8*)fn_8027E344();
            int i;
            u8* p = rec;

            if (r[14] == skill && (s8)r[15] != 0) {
                ok = 1;
            }
            if (r[16] == skill && (s8)r[17] != 0) {
                ok = 1;
            }
            if (r[18] == skill && (s8)r[19] != 0) {
                ok = 1;
            }
            if (r[20] == skill && (s8)r[21] != 0) {
                ok = 1;
            }
            if (r[22] == skill && (s8)r[23] != 0) {
                ok = 1;
            }
            for (i = 0; i < 3; i++) {
                if (*(u16*)(p + 6) != 0 && fn_80271B4C(*(u16*)(p + 6), skill) == 1) {
                    ok = 1;
                }
                p += 2;
            }
        } else if (kind == 6) {
            u8 n = rec[1];
            int i;
            u8* p = rec;

            for (i = 0; i < rec[1]; i++) {
                if (*(u16*)(p + 6) != 0 && fn_80271B4C(*(u16*)(p + 6), skill) == 1) {
                    ok = 1;
                }
                p += 2;
            }
            if (n < 3) {
                if (*(u16*)(rec + n * 2 + 6) == skill && (s8)((s8)(u8)*(u16*)(rec + 4) - 10) != 0) {
                    ok = 1;
                }
                if (n + 1 < 3 && *(u16*)(rec + (n + 1) * 2 + 6) == skill &&
                    (s8)((s8)(u8)(*(u16*)(rec + 4) >> 8) - 10) != 0) {
                    ok = 1;
                }
            }
        }
    }
    return ok;
}

/* Whether any of the nine player records provides `skill`. */
extern "C" u32 fn_80272218(u8* rec, u8 skill) {
    u32 ok = 0;

    if (fn_80271E0C(rec + 0x30, skill) == 1) {
        ok = 1;
    }
    if (fn_80271E0C(rec + 0x0C, skill) == 1) {
        ok = 1;
    }
    if (fn_80271E0C(rec + 0x18, skill) == 1) {
        ok = 1;
    }
    if (fn_80271E0C(rec + 0x24, skill) == 1) {
        ok = 1;
    }
    if (fn_80271E0C(rec + 0x3C, skill) == 1) {
        ok = 1;
    }
    if (fn_80271E0C(rec + 0x48, skill) == 1) {
        ok = 1;
    }
    if (rec[0x54] == 12 && fn_80271E0C(rec + 0x54, skill) == 1) {
        ok = 1;
    }
    if (rec[0x60] == 13 && fn_80271E0C(rec + 0x60, skill) == 1) {
        ok = 1;
    }
    if (fn_80271E0C(rec, skill) == 1) {
        ok = 1;
    }
    return ok;
}

/* Sums the skill level of all nine player records, the main record scaled by its carrier count. */
extern "C" s8 fn_8027234C(u8* rec, u8 skill) {
    s8 v = fn_80271BD4(rec + 0x30, skill);

    v += fn_80271BD4(rec + 0x0C, skill);
    v += fn_80271BD4(rec + 0x18, skill);
    v += fn_80271BD4(rec + 0x24, skill);
    v += fn_80271BD4(rec + 0x3C, skill);
    v += fn_80271BD4(rec + 0x48, skill);
    if (rec[0x54] == 12) {
        v += fn_80271BD4(rec + 0x54, skill);
    }
    if (rec[0x60] == 13) {
        v += fn_80271BD4(rec + 0x60, skill);
    }
    v += fn_80272084(rec, skill);
    return v;
}

/* Sums the skill level of the nine saved equipment slots. */
extern "C" void fn_8027243C(_PLW* plw, u8 skill) {
    _EQUIP saved[9];

    fn_8004A20C(&saved[0], &plw->equipA[0]);
    fn_8004A20C(&saved[1], &plw->equipA[1]);
    fn_8004A20C(&saved[2], &plw->equipA[2]);
    fn_8004A20C(&saved[3], &plw->equipA[3]);
    fn_8004A20C(&saved[4], &plw->equipA[4]);
    fn_8004A20C(&saved[5], &plw->equipA[5]);
    fn_8004A20C(&saved[6], &plw->equipB);
    fn_8004A20C(&saved[7], &plw->equipC);
    fn_8004A20C(&saved[8], &plw->equipD);
    fn_8027234C((u8*)&saved[0], skill);
}

/* The signed skill value of the equipment slot `slot`, or 0 for the empty slot. */
extern "C" int fn_802731B4(_PLW* plw, u16 slot) {
    u16 id = fn_80273044(plw, slot);

    if (id != 0xFFFF) {
        if (id & 0x80) {
            return *(s16*)((u8*)plw + ((id & 0x7F) + 26) * 4 + 0x27A);
        }
        return *(s16*)((u8*)plw + id * 4 + 0x27A);
    }
    return 0;
}

/* Stores one equipment record into the saved set selected by its own slot id and marks it valid. */
extern "C" void fn_802738E8(_PLW* plw, u8* rec) {
    u8 idx = (u8)fn_8027E290(rec[0]);

    fn_8027E98C(rec);
    fn_8004A20C((_EQUIP*)((u8*)plw + idx * 12 + 0x188), (_EQUIP*)rec);
    plw->unk250 = plw->unk250 | (u16)(1 << idx);
    idx = fn_802738D8(idx);
    if (idx != 0xFF) {
        fn_80273998(plw, idx, fn_8027EBA8(plw, rec));
    }
}

/* Scales one attack value by the attack-boost skills the player has and writes the rank flag. */
extern "C" s32 fn_802739F0(_PLW* plw, s16 value, s32 mode, s8* out) {
    f32 v = (f32)value;

    if (mode == 0) {
        if (Pl_Skill_ck(plw, 65) == 1) {
            v *= lbl_8079A03C;
            *out = 1;
        } else if (Pl_Skill_ck(plw, 66) == 1) {
            v *= lbl_8079A078;
            *out = 2;
        }
    } else {
        if (Pl_Skill_ck(plw, 63) == 1 || Pl_cat_skill_ck(plw, 6) == 1) {
            v *= lbl_8079A07C;
            *out = 1;
        } else if (Pl_Skill_ck(plw, 64) == 1) {
            v *= lbl_8079A058;
            *out = 2;
        }
    }
    return (s32)v;
}

/* Stores two decoration equipment records into the spare slots, clearing whichever is absent. */
extern "C" void fn_802737B0(_PLW* plw, u8* a2, u8* a3, u8* a4) {
    fn_8027E98C(a2);
    fn_8004A20C((_EQUIP*)((u8*)plw + 0x1DC), (_EQUIP*)a2);
    if (a3 != 0) {
        if (a3[0] != 0) {
            fn_8027E98C(a3);
            fn_8004A20C((_EQUIP*)((u8*)plw + 0x200), (_EQUIP*)a3);
        } else {
            memset((u8*)plw + 0x200, 0, 12);
        }
    } else {
        memset((u8*)plw + 0x200, 0, 12);
    }
    if (a4 != 0) {
        if (a4[0] != 0) {
            fn_8027E98C(a4);
            fn_8004A20C((_EQUIP*)((u8*)plw + 0x20C), (_EQUIP*)a4);
        } else {
            memset((u8*)plw + 0x20C, 0, 12);
        }
    } else {
        memset((u8*)plw + 0x20C, 0, 12);
    }
    plw->unk254 = 1;
}

/* Maps a slot's item id to the display slot number, the spare-slot half being marked with 0x80. */
extern "C" u16 fn_80273044(_PLW* plw, u16 slot) {
    int i;
    int j;
    int k;

    if (GetItemData__FUs(slot)[0] == 1 && fn_8026FE44(plw) == 1) {
        for (int i = 0; i < 8; i++) {
            if (slot == plw->unk2E0[i].unk0) {
                return (u16)(i | 0x80);
            }
        }
    }
    for (int j = 0; j < 3; j++) {
        for (int k = 0; k < 8; k++) {
            if (slot == plw->unk278[j * 8 + k].unk0) {
                return (u16)(j * 8 + k);
            }
        }
    }
    return 0xFFFF;
}
