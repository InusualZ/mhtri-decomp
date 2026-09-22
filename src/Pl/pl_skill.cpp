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
 *   -sdata 0: the target addresses `lobby_w` with lis/addi (ABS16), not `@sda21`; Pl_Skill_ck 98.7 -> 100 %.
 * With the registered `cflags_base` instead, the same source measures 65.0 / 82.1 / 68.0 / 80.2 / 98.6 / ...
 * and fn_80270F50 0 % (auto-inlining blows it up to 2852 B).
 *
 * Residuals (per function below 100 %, all with the four flags above):
 *   fn_80270018 99.76 fn_8027035C 99.74 fn_802703F4 99.76 fn_80270728 99.71 fn_802707B4 99.50
 *   fn_80270B98 99.02 fn_80270CA4 99.68 fn_80270F50 99.92 - only the `.sdata2` pool rows differ.
 *     This unit's `.sdata2` is *not* claimed in splits.txt, so the compiler-built pool entries stay local
 *     (`@NN`) where the target references `lbl_8079A008` (the 2^52+2^31 int->float magic) and the
 *     `lbl_8079A030`-`lbl_8079A05C` float literals; objdiff scores each as an ARG row. Playbook 29 fixes
 *     this by claiming the pool, which needs a splits.txt edit and a before/after measurement.
 *   fn_80270C64 91.56 - target keeps `table + slot*2` in r5/r4 and re-derives it for the second `lha`
 *     (`add r4,r3,r5` ... `add r3,r3,r5`); ours mutates the base, one `add` fewer (60 vs 64 B).
 *   fn_80271674 86.29 - the target has the two `return` blocks sunk out of line (`ble`/`beq` forward,
 *     fall-through into the table walk) and holds `mode` in r4/`k` in r3; ours keeps them inline with the
 *     registers the other way round. The unsigned range tests and the table walk itself match.
 *
 * Work in progress: 13 of 50 functions written, in address order, through fn_80271674; next is fn_8027176C.
 */

#include "types.h"

struct _EQUIP {
    u8 unk[12];
};

struct _PLW {
    /* 0x000 */ u8 unk000[2];
    /* 0x002 */ u8 unk2;
    /* 0x003 */ u8 unk003[0x140 - 0x003];
    /* 0x140 */ _EQUIP equipA[6];
    /* 0x188 */ u8 unk188[0x1D0 - 0x188];
    /* 0x1D0 */ _EQUIP equipB;
    /* 0x1DC */ u8 unk1DC[0x1E8 - 0x1DC];
    /* 0x1E8 */ _EQUIP equipC;
    /* 0x1F4 */ _EQUIP equipD;
    /* 0x200 */ u8 unk200[0x370 - 0x200];
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
    /* 0x60A */ u8 unk60A[0x61A - 0x60A];
    /* 0x61A */ u16 unk61A[8];
    /* 0x62A */ u8 unk62A[8];
};

extern "C" {
void fn_8004A20C(_EQUIP*, _EQUIP*);
u32 fn_8026FFBC(_PLW*);
u32 fn_80273ED8(_PLW*, int, s8);
int fn_802731B4(_PLW*, u16);
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
extern u16 lbl_805C0198[];
extern u16 lbl_805C01B8[];
extern u8 lobby_w;
}

bool Pl_master_ck(_PLW*);
bool Pl_Skill_ck(_PLW*, u16);
bool Pl_cat_skill_ck(_PLW*, u16);
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
            value = (int)(value * 1.35f);
            *out = 1;
        } else if (fn_8026FFBC(plw) == 1) {
            if (Pl_Skill_ck(plw, 165) == 1) {
                value = (int)(value * 0.7f);
                *out = 2;
            } else if (Pl_Skill_ck(plw, 164) == 1) {
                value = (int)(value * 1.3f);
                *out = 1;
            }
        }
        if (Pl_Skill_ck(plw, 202) == 1) {
            if (plw->unk446 >= 2) {
                value = (int)(value * 1.2f);
            } else if (plw->unk446 >= 1) {
                value = (int)(value * 1.1f);
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
    plw->unk3BC = (f32)plw->unk3B8 / 100.0f;
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
            value = (int)(value * 1.05f);
            value += 10;
            *out = 1;
        } else if (Pl_Skill_ck(plw, 82) == 1) {
            value = (int)(value * 1.1f);
            value += 10;
            *out = 1;
        } else if (Pl_Skill_ck(plw, 83) == 1) {
            value += -10;
            *out = 2;
        } else if (Pl_Skill_ck(plw, 84) == 1) {
            value = (int)(value * 0.95f);
            value += -10;
            *out = 2;
        } else if (Pl_Skill_ck(plw, 85) == 1) {
            value = (int)(value * 0.9f);
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
            value = (int)(value * 1.5f);
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
    if (plw->unk3C0 < 10.0f) {
        plw->unk3C0 = 10.0f;
    }
}

/*
 * Applies one weapon-type skill group to one attack-bonus field: the +20/+15/+10 levels of the group's
 * three positive skills, then the -15/-10 levels of its two negative ones.
 */
#define SKILL_GROUP(field, base)                                 if (Pl_Skill_ck(plw, base) == 1) {                               plw->field += 20.0f;                                     } else if (Pl_Skill_ck(plw, base - 1) == 1) {                    plw->field += 15.0f;                                     } else if (Pl_Skill_ck(plw, base - 2) == 1) {                    plw->field += 10.0f;                                     }                                                            if (Pl_Skill_ck(plw, base + 2) == 1) {                           plw->field -= 15.0f;                                     } else if (Pl_Skill_ck(plw, base + 1) == 1) {                    plw->field -= 10.0f;                                     }

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
    if (plw->unk3C4 < -99.0f) {
        plw->unk3C4 = -99.0f;
    }
    if (plw->unk3C8 < -99.0f) {
        plw->unk3C8 = -99.0f;
    }
    if (plw->unk3CC < -99.0f) {
        plw->unk3CC = -99.0f;
    }
    if (plw->unk3D0 < -99.0f) {
        plw->unk3D0 = -99.0f;
    }
    if (plw->unk3D4 < -99.0f) {
        plw->unk3D4 = -99.0f;
    }
    if (plw->unk3C4 > 99.0f) {
        plw->unk3C4 = 99.0f;
    }
    if (plw->unk3C8 > 99.0f) {
        plw->unk3C8 = 99.0f;
    }
    if (plw->unk3CC > 99.0f) {
        plw->unk3CC = 99.0f;
    }
    if (plw->unk3D0 > 99.0f) {
        plw->unk3D0 = 99.0f;
    }
    if (plw->unk3D4 > 99.0f) {
        plw->unk3D4 = 99.0f;
    }
}

/* The defence delta of one armour-piece slot: -1 for a negative skill, +1 for a positive one. */
extern "C" f32 fn_80270C64(s16* table, s16 slot) {
    f32 delta = 0.0f;
    s16* p = &table[slot];

    if (p[535] > 0) {
        delta -= 30.0f;
    }
    if (p[540] > 0) {
        delta += 5.0f;
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
        plw->unk3D4 -= 30.0f;
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
        for (i = 0; i < 8; i++) {
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
        for (i = 0; i < 8; i++) {
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
        if (lobby_w == 6) {
            ok = true;
        } else if (lobby_w == 15 && fn_80363A2C() == 1) {
            ok = true;
        }
    }
    if (ok == 1) {
        for (i = 0; i < 8; i++) {
            if (skill == plw->unk61A[i]) {
                return true;
            }
        }
    } else {
        for (i = 0; i < 8; i++) {
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

    if (fn_800CF208() == 2 && lobby_w == 6) {
        for (i = 0; i < 8; i++) {
            if ((u8)slot == plw->unk62A[i]) {
                return (u8)plw->unk61A[i];
            }
        }
    } else {
        for (i = 0; i < 8; i++) {
            if ((u8)slot == plw->unk602[i]) {
                return (u8)plw->unk5F2[i];
            }
        }
    }
    return 0;
}

/* Whether the player currently provides `kind` of skill: kind 15-17 ask the equipment, 74 the player
 * type, and anything else the two skill-id tables. */
extern "C" u32 fn_80271674(_PLW* plw, u32 kind) {
    u16 mode = (fn_8026FE44(plw) == 1);
    int k = (u8)kind;
    u16* table;

    if ((u32)(k - 15) <= 2) {
        return (u32)(fn_80274AB8((int)plw) == 1);
    }
    if (k == 74) {
        int type = plw->unk2;

        if ((u32)(type - 7) <= 1 || type == 0 || type == 2) {
            return 1;
        }
        return 0;
    }
    table = (mode == 0) ? lbl_805C0198 : lbl_805C01B8;
    while (*table != 0xFFFF) {
        if (*table++ == k) {
            return 0;
        }
    }
    return 1;
}
