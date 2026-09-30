/* The player act-entry/parameter unit, `Pl` band, `.text` 0x80273B14-0x80276B58 (0x3044 B, 68
 * functions).  It sits between `Pl/pl_skill.cpp` (0x80270018-0x80273B14) and `Pl/pl_act.cpp`
 * (0x80276B58-0x8027D684) and is the band's act-selection front end: `pl_act_enter_raw` resets the
 * player work and arms a new act from its flag word, `fn_80275C34` picks the act's entry motion,
 * and the `fn_80274988`/`fn_80274B5C`/`fn_80274E6C`/`fn_80275014`/`fn_802751B4` family sums the
 * weapon/skill bonus records the player's equipment slots point at.
 *
 * Flags: `cflags_pl` (configure.py) - this lib's units were all built with `-O3 -inline noauto
 * -opt nopeephole -Cpp_exceptions on`.
 *
 * Sections: this unit owns `.text` 0x80273B14-0x80276B58, `extab` 0x8001280C-0x8001294C and
 * `extabindex` 0x8002FA90-0x8002FC70.  Both runs are exactly 40 records that bracket the
 * neighbouring units' runs exactly, and every extabindex record's function address (0x80273B14 ..
 * 0x80276A3C) is one of this unit's - read out of the DOL.  The pooled `.sdata2` constants
 * (0x8079A000 / 0x8079A044 / 0x8079A080 / 0x8079A084) are declared, never defined (playbook 29).
 * `.data` 0x805C5FEC-0x805C6100 (276 B: the two switch tables MWCC emits for `fn_80273B14` and
 * `fn_80274B5C`, the two act-number lists and the pick table) is claimed and emitted; retail has a
 * 4-byte zero word between the second table and the pick table (0x805C60A4) that this build does not
 * reproduce - MWCC puts a zero-initialised variable in `.bss`/`.sdata` and 8-aligns the 88-byte table
 * after it (measured: `.data` 280 B with the word forced in, 272 B without, target 276 B) - so the pick
 * table sits 4 bytes early.  The gate's strict data row demanded the claim: this unit's object changed
 * with the shared `include/pl.h` edit of the `hud/net_char_sync` batch.
 *
 * Residuals (measured per function; see the branch's outbox for the numbers):
 *   * `fn_802751B4` is the one row that does not reach 100 %.  Retail saves/restores `f31` through
 *     a `psq_st`/`vmrghb` pair and converts the weapon record's `+0x0A` `s16` with the int->float
 *     idiom; the source below is the faithful shape, and the residual is the compiler's own
 *     float-save form.
 *   * `fn_80273B14` and `fn_80274B5C` both switch through a compiler-emitted `.data` jump table
 *     (0x805C5FEC, 17 entries, and 0x805C6068, 15 entries).  The case bodies are written in the
 *     order the table's addresses put them in, not in case-value order.
 *   * rule 2 residual, `Pl_net_send`: the address is owned by `src/hud/net_char_sync.cpp`, whose
 *     prototype is three-parameter and right (that unit's definition is 100 % byte-identical), while
 *     retail's Pl call sites must keep the two-parameter view they were built with - so the
 *     declaration is local, with both measurements in the comment on it below.  The prototype family
 *     is cross-TU in retail's own build (two arguments here, three at the owner), so the structural
 *     fix is the owner's header family, not this unit's edit: recorded as a tooling-register request.
 *   * This file stopped compiling because a rule-2 warning was obeyed without checking the call
 *     sites: moving the declaration to the owner's header changed its arity under three call sites
 *     here.  Check the callers first, then move the declaration.
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/symedit.py range 0x80273B14 0x80276B58` - 66 of the 68 stems are `fn_`,
 * and the remaining two, `Pl_critical_get__FP4_PLW` and `Pl_decide_mot_get__FPUsPUs`, are the
 * manglings of the two real C++ declarations this file defines).  `dumpmap.py lookup` answers only
 * `zz_0273b14_` placeholders for the whole band and no `__FILE__` string covers it.  `fn_802752C8`
 * is renamed `Pl_item_id_usable_ck` (0x802752C8, 0xCC B, 96.47 %): `menu/arena_result.cpp` calls
 * it, and a call to a `fn_XXXXXXXX` stem from another unit's new source is a rule-7 finding.  Its
 * second parameter is dead in retail - the body never reads r4 - but every call site passes one
 * (0, 1 and 2), so both the declaration and the definition carry it; re-measuring the whole unit
 * after that change moved no row.  The parameter's name `mode` is a GUESS (the three sites' 0/1/2
 * against the body's own 1/2/0x10 masks) and is marked as one at the declaration.
 */
#include "types.h"
#include "hud/Pl_net_send.h"
#include "pl.h"
#include "mh3_pad/control.h"
#include "Pl/pl_act.h"
#include "Pl/pl_master.h"
#include "Pl/pl_skill.h"
#include "Pl/fn_802693C4.h"   /* `Pl/fn_802693C4.cpp` owns 0x802693C4-0x8026BA1C, so its
                          * `Pl_chr_set_attr_default`/`fn_8026A2DC`/`fn_8026A2F8`/`Pl_chr_setX` declarations
                          * come from the owner's header, not from the band header (rule 2) */
#include "ef/fn_800CDB2C.h"
#include "unsplit/unknown.h"
#include "unsplit/Pl.h"
#include "unsplit/ef.h"
#include "Pl/fn_80273B14.h"
#include "menu/menu_item.h"   /* `item_category_ck` (owner: `menu/menu_item.cpp`, rule 2) */
#include "ef/eft001.h"
#include "stage/stg_w.h"
#include "Runtime.PPCEABI.H/memset.h"

/* 0x8045F554 - the runtime string copy `fn_8027552C` uses to arm a hunter name.  Its address band
 * brackets two different modules (`Runtime.PPCEABI.H/Gecko_ExceptionPPC.cp` below, `OS/OSAlarm.c`
 * above), so no band header owns it; `nw_resource.cpp` declares the same shape locally. */
char* strcpy(char* dst, const char* src);

/* 0x80335CE8 - the client-side act-message sender `pl_act_enter_raw`'s tail calls (owner:
 * `src/hud/net_char_sync.cpp`, declared by its leaf header `hud/Pl_net_send.h`).  Retail's own build carried
 * this one address under two prototypes: the owner's definition takes a third `u16 param` and passes it on to
 * the message builders, while this band's call sites pass two arguments (`bl Pl_net_send` is preceded by only
 * `mr r3, r30` and `li r4, X`).  Supplying the third argument would emit a `li r5, X` at all three call
 * sites, so they call the owner's function through its two-parameter view. */
typedef void (*PlNetSend2)(struct _PLW* plw, s32 kind);

/* The three equipment-slot records `fn_8027ECAC`/`fn_8027ED18`/`fn_8027E344` hand back.  Only this
 * unit reads them, so they live here (rule 1); each offset/width is the one its readers narrow to.
 */

/* The skill record an equipment slot resolves to. size: 0x18 */
typedef struct PlSkillRec {
    /* +0x00 */ u8 pad_0x00[0x8];
    /* +0x08 */ u8 level_a;  /* scaled by 0x32 and biased by 0x96 into `+0x56E` (`fn_802740F4`) */
    /* +0x09 */ u8 level_b;  /* the sibling level the same function scales */
    /* +0x0A */ u8 pad_0x0A[0x3];
    /* +0x0D */ s8 value;    /* the value `fn_802744A0` stores into `+0x570` */
    /* +0x0E */ u8 pad_0x0E[0xA];
} PlSkillRec; /* size: 0x18 */

/* The weapon/skill record an equipment slot resolves to (the 0x24-byte sibling table). size: 0x24 */
typedef struct PlWeaponRec {
    /* +0x00 */ u8 pad_0x00[0x7];
    /* +0x07 */ s8 bonus_0x07;   /* the points `fn_80274E6C` accumulates */
    /* +0x08 */ u8 pad_0x08[0x1];
    /* +0x09 */ s8 bonus_0x09;   /* the points `fn_80275014` accumulates */
    /* +0x0A */ s16 bonus_0x0A;  /* the bonus `fn_802751B4` scales by 1/100 */
    /* +0x0C */ u8 pad_0x0C[0x6];
    /* +0x12 */ s8 bonus_0x12;   /* the points `fn_80274370`/`fn_802744A0` add up */
    /* +0x13 */ u8 pad_0x13[0x11];
} PlWeaponRec; /* size: 0x24 */

/* The per-slot record `fn_8027EE08` hands back through its second argument; `fn_80273B14` reads one
 * act-kind delta per slot out of it.  It is a different table from the `fn_8027E344` record below
 * (its +0x0A is a 2-byte value where that one's is a signed byte). size: 0x14 */
typedef struct PlEquipSlot {
    /* +0x00 */ u8 pad_0x00[0xA];
    /* +0x0A */ s16 value_0x0A;   /* the act kind's 2-byte delta (case 6) */
    /* +0x0C */ u8 value_0x0C;    /* the kind-0 delta */
    /* +0x0D */ u8 pad_0x0D[0x1];
    /* +0x0E */ u8 kind_0x0E;     /* the skill-tier id the 7..11 arms compare against */
    /* +0x0F */ s8 value_0x0F;
    /* +0x10 */ u8 kind_0x10;     /* the skill-tier id the 12..14 arms compare against */
    /* +0x11 */ s8 value_0x11;
    /* +0x12 */ u8 pad_0x12[0x2];
} PlEquipSlot; /* size: 0x14 */

/* The per-slot record `fn_8027E344` hands back; `fn_80273ED8` sums one signed byte per act kind out
 * of it. size: 0x10 */
typedef struct PlSlotRec {
    /* +0x00 */ u8 pad_0x00[0x8];
    /* +0x08 */ s8 value_0x08;
    /* +0x09 */ s8 value_0x09;
    /* +0x0A */ s8 value_0x0A;
    /* +0x0B */ s8 value_0x0B;
    /* +0x0C */ s8 value_0x0C;
    /* +0x0D */ u8 pad_0x0D[0x3];
} PlSlotRec; /* size: 0x10 */

/* One 8-byte row of the act/motion pick table `Pl_decide_mot_get` walks (0x805C60A8, 11 rows: the
 * motion, its parameter and the two control words the row is gated on). size: 0x8 */
/* The joint holder `_PLW::physics_0x13C` points at: the `MHchar` the player's joints are read from
 * sits at `+0x04`, the same layout `ef/eft001.cpp` names `joint_0x004`. size: 0x8 + sizeof(MHchar) */
typedef struct PlJointHolder {
    /* +0x00 */ u32 pad_0x00;
    /* +0x04 */ MHchar chr_0x04;
} PlJointHolder;

typedef struct PlMotRow {
    /* +0x0 */ u16 motion;
    /* +0x2 */ u16 param;
    /* +0x4 */ u16 control_a;
    /* +0x6 */ u16 control_b;
} PlMotRow; /* size: 0x8 */

/* --- the act-kind dispatchers ----------------------------------------------------------------- */

/* Sums the act-kind deltas of the three equipment slots; `mode` selects the delta column (0/6 and
 * the 7..16 skill tiers) and `flag` the melee/ranged variant.  `fn_80273ED8` and `fn_80274D98` are
 * its thin wrappers. */
s32 fn_80273B14(struct _PLW* plw, s32 mode, u8 flag, struct _EQUIP* equip0, struct _EQUIP* equip1,
                struct _EQUIP* equip2, s8* out)
{
    s16 value = 0;

    *out = 0;
    if ((u32)(mode - 6) > 10U && mode != 0) {
        return 0;
    }
    if (equip0 != NULL) {
        void* slot;
        void* scratch;
        /* `Pl/fn_8027D684.cpp` (which now owns 0x8027E...E08) defines this as
         * `void*(_EQUIP*, u32, u32)`, so the two out-pointers are passed as their addresses and the
         * register result is read back as the `s32` the caller tests. */
        s32 kind = (s32)fn_8027EE08(equip0, (u32)&slot, (u32)&scratch);

        if (kind == 1) {
            if (mode == 6) {
                value = fn_8027E5E4(equip0);
                if (equip1 != NULL && equip1->kind == 12) {
                    value += fn_8027E5E4(equip1);
                }
            } else if (mode == 0) {
                value = fn_8027E708(equip0);
                if (equip2 != NULL && equip2->kind == 13) {
                    value += fn_8027E708(equip2);
                }
            }
            return value;
        }
        if (kind != 0) {
            return value;
        }
        if (mode <= 16) {
            PlEquipSlot* rec = (PlEquipSlot*)slot;

            switch (mode) {
            case 6:
                value = rec->value_0x0A;
                break;
            case 0:
                value = rec->value_0x0C;
                break;
            case 7:
                if (rec->kind_0x0E == 1) {
                    value = fn_802739F0(plw, rec->value_0x0F, 0, out);
                }
                break;
            case 8:
                if (rec->kind_0x0E == 2) {
                    value = fn_802739F0(plw, rec->value_0x0F, 0, out);
                }
                break;
            case 9:
                if (rec->kind_0x0E == 3) {
                    value = fn_802739F0(plw, rec->value_0x0F, 0, out);
                }
                break;
            case 10:
                if (rec->kind_0x0E == 4) {
                    value = fn_802739F0(plw, rec->value_0x0F, 0, out);
                }
                break;
            case 11:
                if (rec->kind_0x0E == 5) {
                    value = fn_802739F0(plw, rec->value_0x0F, 0, out);
                }
                break;
            case 12:
                if (rec->kind_0x10 == 1) {
                    value = fn_802739F0(plw, rec->value_0x11, 1, out);
                }
                break;
            case 13:
                if (rec->kind_0x10 == 2) {
                    value = fn_802739F0(plw, rec->value_0x11, 1, out);
                }
                break;
            case 14:
                if (rec->kind_0x10 == 3) {
                    value = fn_802739F0(plw, rec->value_0x11, 1, out);
                }
                break;
            case 15:
                value = fn_802739F0(plw, rec->value_0x0F, 0, out);
                break;
            case 16:
                value = fn_802739F0(plw, rec->value_0x11, 1, out);
                break;
            default:
                break;
            }
        }
    } else {
        if (equip1 != NULL && mode == 6 && equip1->kind == 12) {
            value = fn_8027E5E4(equip1);
        }
        if (equip2 != NULL && mode == 0 && equip2->kind == 13) {
            value += fn_8027E708(equip2);
        }
    }
    if (value < 0) {
        if (Pl_Skill_ck(plw, 0xC9) == 1U) {
            *out = 1;
            value = value * 3;
            return value;
        }
        if (flag == 0) {
            return 0;
        }
        value = value * 3;
    }
    return value;
}

/* The single-slot wrapper of `fn_80273B14`: it hands the player's own melee/ranged slots on. */
s32 fn_80273ED8(struct _PLW* plw, s32 kind, s32 flag)
{
    s32 value = 0;

    if ((u32)(kind - 6) > 10U) {
        if ((u32)(kind - 1) > 4U) {
            if (kind != 0) {
                return 0;
            }
            value = fn_80273B14(plw, kind, (u8)flag, &plw->equipB, &plw->equipC, &plw->equipD,
                                (s8*)&plw->field_0x00E);
        } else {
            s32 n = 0;
            struct _EQUIP* slot = plw->equipA;

            do {
                PlSlotRec* rec = (PlSlotRec*)fn_8027E344(slot);

                switch (kind) {
                case 0:
                    value += fn_8027E510(slot);
                    break;
                case 1:
                    value += rec->value_0x08;
                    break;
                case 2:
                    value += rec->value_0x09;
                    break;
                case 3:
                    value += rec->value_0x0A;
                    break;
                case 4:
                    value += rec->value_0x0C;
                    break;
                case 5:
                    value += rec->value_0x0B;
                    break;
                }
                slot++;
                n++;
            } while (n < 6);
        }
    } else {
        value = fn_80273B14(plw, kind, (u8)flag, &plw->equipB, &plw->equipC, &plw->equipD,
                            (s8*)&plw->field_0x00E);
    }
    return value;
}

/* Maps a weapon/skill id to the bonus class the pick tables use. */
u8 fn_8027403C(u16 kind)
{
    switch (kind) {
    case 0x8B:
        return 1;
    case 0x17D:
        return 3;
    case 0x17E:
        return 4;
    case 0x18B:
        return 2;
    default:
        return 0;
    }
}

/* Maps the player's act-state byte to the act kind `fn_80273B14` selects its delta column by. */
s32 fn_8027408C(struct _PLW* plw)
{
    switch (plw->field_0x002) {
    case 6:
        return 1;
    case 5:
        fn_802740EC(plw);
        return 2;
    case 4:
        fn_802740EC(plw);
        return 0;
    default:
        return 0;
    }
}

/* Clears the player's short-range equipment slot record. */
void fn_802740EC(struct _PLW* plw)
{
    fn_8027ED28(&plw->equipB);
}

/* Re-reads the player's armed weapon record into the level/attack bytes at +0x56A/+0x56E. */
void fn_802740F4(struct _PLW* plw, struct _EQUIP* equip)
{
    PlSkillRec* rec = (PlSkillRec*)fn_8027ECAC(equip);

    plw->field_0x56A = rec->level_a;
    plw->field_0x56E = (s16)(rec->level_b * 0x32 + 0x96);
    if (Pl_Skill_ck(plw, 0x17) == 1U) {
        plw->field_0x56E += 0x32;
    }
    if (plw->field_0x56E > 0x1C2) {
        plw->field_0x56E = 0x1C2;
    }
}

/* Adds the act-kind and skill-tier deltas on top of `base`, and records which way the value moved. */
s16 fn_80274174(struct _PLW* plw, s16 base, u8 ranged, s8* out)
{
    s16 value = base;

    if (base > 0 && ranged == 0 && fn_8026FE44(plw) == 0) {
        switch (plw->field_0x56B) {
        case 4:
            value += 5;
            break;
        case 5:
            value += 10;
            break;
        case 6:
            value += 15;
            break;
        default:
            break;
        }
    }
    if (ranged == 0) {
        if (Pl_dm_condition_ck(plw, 0x100) == 1U) {
            value -= 0x32;
            *out = 2;
        } else if (Pl_dm_condition_ck(plw, 0x200) == 1U) {
            value -= 0x32;
            *out = 2;
        }
    }
    if (Pl_Skill_ck(plw, 0x19) == 1U) {
        value += 10;
        *out = 1;
    } else if (Pl_Skill_ck(plw, 0x1A) == 1U) {
        value += 20;
        *out = 1;
    } else if (Pl_Skill_ck(plw, 0x1B) == 1U) {
        value += 30;
        *out = 1;
    } else if (Pl_Skill_ck(plw, 0x1C) == 1U) {
        value -= 5;
        *out = 2;
    } else if (Pl_Skill_ck(plw, 0x1D) == 1U) {
        value -= 10;
        *out = 2;
    } else if (Pl_Skill_ck(plw, 0x1E) == 1U) {
        value -= 15;
        *out = 2;
    }
    return value;
}

/* Re-computes the player's critical-hit flag from the current weapon slot. */
s32 Pl_critical_get(struct _PLW* plw)
{
    s8 flag;

    return fn_80274174(plw, plw->field_0x570, 0, &flag);
}

/* Sums the points of the two weapon records the act is holding, then folds in the act-kind and
 * skill-tier deltas. */
s32 fn_80274370(struct _PLW* plw, struct _EQUIP* equip0, struct _EQUIP* equip1,
                struct _EQUIP* equip2, s8* out)
{
    s32 value;

    *out = 0;
    switch (equip0->kind) {
    case 7:
    case 8:
    case 9:
    case 10:
    case 14:
    case 15:
        value = ((PlSkillRec*)fn_8027ECAC(equip0))->value;
        break;
    case 12:
    case 13:
        value = ((PlWeaponRec*)fn_8027ED18(equip0))->bonus_0x12;
        break;
    case 11:
        value = ((PlWeaponRec*)fn_8027ED18(equip0))->bonus_0x12;
        if (equip1 != NULL && equip1->kind == 12) {
            value += ((PlWeaponRec*)fn_8027ED18(equip1))->bonus_0x12;
        }
        if (equip2 != NULL && equip2->kind == 13) {
            value += ((PlWeaponRec*)fn_8027ED18(equip2))->bonus_0x12;
        }
        break;
    default:
        return 0;
    }
    if (plw != NULL) {
        value = fn_80274174(plw, (s16)value, 1, out);
    }
    return value;
}

/* Refreshes the player's armed weapon class and ranged attack value out of the melee slot. */
void fn_802744A0(struct _PLW* plw)
{
    switch (plw->equipB.kind) {
    case 7:
    case 8:
    case 9:
    case 10:
    case 14:
    case 15:
        plw->field_0x570 = ((PlSkillRec*)fn_8027ECAC(&plw->equipB))->value;
        break;
    case 11:
        plw->field_0x570 = ((PlWeaponRec*)fn_8027ED18(&plw->equipB))->bonus_0x12;
        if (plw->equipD.kind == 13) {
            plw->field_0x570 += ((PlWeaponRec*)fn_8027ED18(&plw->equipD))->bonus_0x12;
        }
        if (plw->equipC.kind == 12) {
            plw->field_0x570 += ((PlWeaponRec*)fn_8027ED18(&plw->equipC))->bonus_0x12;
        }
        break;
    default:
        plw->field_0x570 = 0;
        break;
    }
}

/* True while the player's act is armed with a weapon. */
s32 fn_80274570(struct _PLW* plw)
{
    return plw->field_0x30A != 0;
}

/* Writes the armed byte of the player's own move work record. */
void fn_80274584(s8 value)
{
    struct _PLW* work = (struct _PLW*)get_move_work_adrs(2);

    if (work != NULL) {
        work = (struct _PLW*)((u8*)work + (s8)my_player_no() * 0xB20);
        work->field_0x5C8 = value;
    }
}

/* The act-number lists `fn_802745DC` (by armed slot) and `fn_80274624` (by act number) walk: the
 * melee family, then the ranged family; slot 0 is unused and 0xFFFF ends a list.  size: 0x1C each */
u16 pl_act_no_tbl_melee[14] = {0x0000, 0x000A, 0x0004, 0x0003, 0x0008, 0x0000, 0x0002,
                               0x0001, 0x0009, 0x0005, 0x0006, 0x000B, 0x0007, 0xFFFF};
u16 pl_act_no_tbl_ranged[14] = {0x000E, 0x0018, 0x0012, 0x0011, 0x0016, 0x000E, 0x0010,
                                0x000F, 0x0017, 0x0013, 0x0014, 0x0019, 0x0015, 0xFFFF};

/* Reads the armed slot's value table for the given kind. */
u32 fn_802745DC(struct _PLW* plw, s16 kind)
{
    u8 slot = plw->field_0x5C8;

    if (slot == 0) {
        return 0;
    }
    if (kind == 0) {
        return pl_act_no_tbl_melee[slot];
    }
    return pl_act_no_tbl_ranged[slot];
}

/* Finds the index the player's act number occupies in one of the two act tables. */
s32 fn_80274624(struct _PLW* plw)
{
    const u16* row;
    s32 index;

    if (plw->field_0x00A != 0xB) {
        return 0;
    }
    row = pl_act_no_tbl_melee;
    index = 1;
    while (row[1] != 0xFFFF) {
        switch (row[1]) {
        case 5:
            if (plw->act_no == 5 || plw->act_no == 12) {
                return index;
            }
            break;
        case 11:
            if (plw->act_no == 11 || plw->act_no == 13) {
                return index;
            }
            break;
        default:
            if (plw->act_no == row[1]) {
                return index;
            }
            break;
        }
        index++;
        row += 1;
    }
    row = pl_act_no_tbl_ranged;
    index = 1;
    while (row[1] != 0xFFFF) {
        switch (row[1]) {
        case 19:
            if (plw->act_no == 19 || plw->act_no == 26) {
                return index;
            }
            break;
        case 25:
            if (plw->act_no == 25 || plw->act_no == 27) {
                return index;
            }
            break;
        default:
            if (plw->act_no == row[1]) {
                return index;
            }
            break;
        }
        index++;
        row += 1;
    }
    return 0;
}

/* Latches the act's follow-up stage and arms its two-frame hold. */
void pl_act_stage_latch_set(struct _PLW* plw, u8 stage)
{
    if (Pl_master_ck(plw) != 0) {
        plw->field_0x01F = stage;
        plw->field_0x396 = 2;
    }
}

/* True when the act's follow-up stage is open and the player is in a state that may use it. */
s32 fn_80274794(struct _PLW* plw)
{
    if (Pl_master_ck(plw) == 0) {
        return 0;
    }
    if ((u32)(plw->field_0x00A - 8) <= 1U) {
        return 0;
    }
    if (plw->field_0x370 <= 0) {
        return 0;
    }
    return plw->field_0x01F == 0;
}

/* The act's follow-up stage byte. */
u8 pl_act_stage_get(struct _PLW* plw)
{
    return plw->field_0x01F;
}

/* The player's own move work record. */
struct _PLW* my_player_work_get(void)
{
    return (struct _PLW*)((u8*)get_move_work_adrs(2) + (s8)my_player_no() * 0xB20);
}

/* The first move work record whose slot byte differs from this player's. */
void* fn_80274850(struct _PLW* plw)
{
    u8* work = (u8*)get_move_work_adrs(2);
    u16 count = (u16)get_move_work_max(2);

    if ((s32)count > 0) {
        do {
            if (work[8] != plw->chunk_ofs) {
                return work;
            }
            work += 0xB20;
        } while (--count != 0);
    }
    return NULL;
}

/* Buckets the player's stagger timer into 0-3. */
s32 fn_802748C8(void* self)
{
    struct _PLW* plw = (struct _PLW*)self;
    s32 value = 0;

    if (plw->field_0x386 > 0x50) {
        value = 3;
    } else if (plw->field_0x386 > 0x28) {
        value = 2;
    } else if (plw->field_0x386 > 0) {
        value = 1;
    }
    return value;
}

/* True once the armed-slot table has been read for this player. */
s32 fn_80274904(struct _PLW* plw)
{
    return plw->field_0x5C9 != 0;
}

/* Picks the act-name table row for the player's current stance. */
u8* fn_80274918(struct _PLW* plw, s32 force, s32 index)
{
    if (force == 0) {
        return (u8*)(lbl_805C5F30 + index * 0xA);
    }
    if (plw->field_0x5B8 >= 0xA0) {
        return (u8*)(lbl_805C5F30 + index * 0xA);
    }
    if (plw->field_0x036 == 0) {
        return (u8*)(lbl_805C5F50 + index * 0xA);
    }
    return (u8*)(lbl_805C5F70 + index * 0xA);
}

/* True when either of the player's weapon slots resolves to a bonus. */
s32 fn_80274988(struct _PLW* plw)
{
    if (fn_8026FE44(plw) == 0) {
        return 0;
    }
    if (fn_8027FFFC(&plw->equipB) == 1U) {
        return 1;
    }
    if (plw->equipC.kind == 12 && fn_8027FFFC(&plw->equipC) == 1U) {
        return 1;
    }
    return 0;
}

/* The same test for a caller-supplied slot pair and act kind. */
s32 fn_80274A04(struct _EQUIP* equip0, struct _EQUIP* equip1, u8 kind)
{
    if ((u32)(kind - 4) > 2U) {
        return 0;
    }
    if (equip0 != NULL) {
        if (equip0->kind == 11) {
            if (fn_8027FFFC(equip0) == 1U) {
                return 1;
            }
            if (equip1 != NULL && equip1->kind == 12 && fn_8027FFFC(equip1) == 1U) {
                return 1;
            }
        } else if (equip0->kind == 12 && fn_8027FFFC(equip0) == 1U) {
            return 1;
        }
    }
    return 0;
}

/* True when the player's act kind is one the melee/ranged family owns. */
s32 fn_80274AB8(struct _PLW* plw)
{
    if ((u32)(plw->field_0x002 - 4) > 2U) {
        if (plw->field_0x002 <= 1U || plw->field_0x002 == 3) {
            return 1;
        }
        return 0;
    }
    return fn_80274988(plw);
}

/* The same test for a standalone act kind. */
s32 fn_80274AEC(u8 kind)
{
    if ((u32)(kind - 4) > 2U) {
        if (kind <= 1U || kind == 3) {
            return 1;
        }
        return 0;
    }
    return fn_80274A04(NULL, NULL, kind);
}

/* Looks an id up in the 4-byte-stride id/value table at 0x805BF490. */
u8 fn_80274B20(u16 id)
{
    const u16* row = lbl_805BF490;

    for (;;) {
        u16 key = row[0];

        if (key == 0) {
            return 0xFF;
        }
        if ((u16)id == key) {
            return (u8)row[1];
        }
        row += 2;
    }
}

/* Sums the weapon/skill bonus records of the player's three weapon slots for the given act kind. */
s32 fn_80274B5C(struct _PLW* plw, u8 kind, struct _EQUIP* equip0, struct _EQUIP* equip1,
                struct _EQUIP* equip2, s8* out)
{
    u8 value;

    *out = 0;
    value = (u8)fn_8027F008(equip0, kind);
    if (equip1 != NULL && equip1->kind == 12) {
        value = (u8)(value + (u8)fn_8027F008(equip1, kind));
    }
    if (equip2 != NULL && equip2->kind == 13) {
        value = (u8)(value + (u8)fn_8027F008(equip2, kind));
    }
    switch (kind) {
    case 0:
    case 1:
    case 2:
        if (Pl_Skill_ck(plw, 0x36) == 1U) {
            value += 3;
            *out = 1;
        }
        break;
    case 3:
        if (Pl_Skill_ck(plw, 0x37) == 1U) {
            value += 2;
            *out = 1;
        }
        break;
    case 4:
    case 5:
        if (Pl_Skill_ck(plw, 0x38) == 1U) {
            value += 2;
            *out = 1;
        }
        break;
    case 6:
        if (Pl_Skill_ck(plw, 0x39) == 1U) {
            value += 2;
            *out = 1;
        }
        break;
    case 7:
    case 8:
        if (Pl_Skill_ck(plw, 0x3A) == 1U) {
            value += 2;
            *out = 1;
        }
        break;
    case 9:
        if (Pl_Skill_ck(plw, 0x3B) == 1U) {
            value += 1;
            *out = 1;
        }
        break;
    case 10:
    case 11:
        if (Pl_Skill_ck(plw, 0x3C) == 1U) {
            value += 1;
            *out = 1;
        }
        break;
    case 12:
        if (Pl_Skill_ck(plw, 0x3D) == 1U) {
            value += 1;
            *out = 1;
        }
        break;
    case 13:
    case 14:
        if (Pl_Skill_ck(plw, 0x3E) == 1U) {
            value += 1;
            *out = 1;
        }
        break;
    default:
        break;
    }
    if (value != 0) {
        if (Pl_Skill_ck(plw, 0xAA) == 1U) {
            value += 1;
            *out = 1;
        }
    }
    return value;
}

/* The player-bound wrapper of `fn_80274B5C`. */
u8 fn_80274D98(struct _PLW* plw, u8 kind)
{
    return (u8)fn_80274B5C(plw, kind, &plw->equipB, &plw->equipC, &plw->equipD,
                            (s8*)&plw->field_0x00E);
}

/* True when the player-bound bonus sum is non-zero. */
u8 fn_80274DCC(struct _PLW* plw, u8 kind)
{
    return fn_80274D98(plw, kind) != 0;
}

/* Gates the act's bonus sum on the act id being a live one. */
s32 fn_80274E00(struct _PLW* plw, u16 id)
{
    u8 cls;

    if (fn_8026FE44(plw) == 0) {
        return 1;
    }
    cls = fn_80274B20(id);
    if (cls == 0xFF) {
        return 1;
    }
    return fn_80274DCC(plw, cls);
}

/* Sums the weapon records' `bonus_0x07` column and folds the skill tiers in, clamped to 0..6. */
u8 fn_80274E6C(struct _PLW* plw, struct _EQUIP* equip0, struct _EQUIP* equip1, s8* out)
{
    s32 value = 0;

    *out = 0;
    if (equip0 != NULL && equip0->kind == 11) {
        value = ((PlWeaponRec*)fn_8027ED18(equip0))->bonus_0x07;
        if (value == 0) {
            value = 1;
        }
    }
    if (equip1 != NULL && equip1->kind == 13) {
        value += ((PlWeaponRec*)fn_8027ED18(equip1))->bonus_0x07;
    }
    if (plw != NULL) {
        if (Pl_Skill_ck(plw, 0x2D) == 1U) {
            value += 2;
            *out = 1;
        } else if (Pl_Skill_ck(plw, 0x2E) == 1U) {
            value += 3;
            *out = 1;
        } else if (Pl_Skill_ck(plw, 0x2F) == 1U) {
            value += 4;
            *out = 1;
        }
        if (Pl_Skill_ck(plw, 0x30) == 1U) {
            value -= 1;
            *out = 2;
        } else if (Pl_Skill_ck(plw, 0x31) == 1U) {
            value -= 2;
            *out = 2;
        } else if (Pl_Skill_ck(plw, 0x32) == 1U) {
            value -= 3;
            *out = 2;
        }
    }
    if (value < 0) {
        value = 0;
    }
    if (value > 6) {
        value = 6;
    }
    return (u8)value;
}

/* Sums the weapon records' `bonus_0x09` column plus a fixed 3, clamped to 0..9. */
u8 fn_80275014(struct _PLW* plw, struct _EQUIP* equip0, struct _EQUIP* equip1, s8* out)
{
    s32 value = 0;

    *out = 0;
    if (equip0 != NULL && equip0->kind == 11) {
        value = ((PlWeaponRec*)fn_8027ED18(equip0))->bonus_0x09;
    }
    if (equip1 != NULL && equip1->kind == 13) {
        value += ((PlWeaponRec*)fn_8027ED18(equip1))->bonus_0x09;
    }
    if (plw != NULL) {
        if (Pl_Skill_ck(plw, 0x27) == 1U) {
            value += 2;
            *out = 1;
        } else if (Pl_Skill_ck(plw, 0x28) == 1U) {
            value += 3;
            *out = 1;
        } else if (Pl_Skill_ck(plw, 0x29) == 1U) {
            value += 4;
            *out = 1;
        }
        if (Pl_Skill_ck(plw, 0x2A) == 1U) {
            value -= 1;
            *out = 2;
        } else if (Pl_Skill_ck(plw, 0x2B) == 1U) {
            value -= 2;
            *out = 2;
        } else if (Pl_Skill_ck(plw, 0x2C) == 1U) {
            value -= 3;
            *out = 2;
        }
    }
    value += 3;
    if (value < 0) {
        value = 0;
    }
    if (value > 9) {
        value = 9;
    }
    return (u8)value;
}

/* The two weapon records' elemental bonus product, scaled by 1/100. */
f32 fn_802751B4(struct _EQUIP* equip0, struct _EQUIP* equip1, s8* out)
{
    f32 value = lbl_8079A000;

    *out = 0;
    if (equip0 != NULL) {
        if (equip0->kind == 11) {
            value = (f32)((PlWeaponRec*)fn_8027ED18(equip0))->bonus_0x0A;
        }
        if (equip1 != NULL && equip1->kind == 12) {
            value = value * (f32)((PlWeaponRec*)fn_8027ED18(equip1))->bonus_0x0A / lbl_8079A044;
        }
    } else if (equip1 != NULL && equip1->kind == 12) {
        value = (f32)((PlWeaponRec*)fn_8027ED18(equip1))->bonus_0x0A;
    }
    return value / lbl_8079A044;
}

/* True when the item/act id is one the player may still use.  `mode` is dead in retail - every call
 * site sets r4, the body never reads it (see the owner's header, which marks the name a GUESS). */
s32 Pl_item_id_usable_ck(u16 id, s32 mode)
{
    if (item_category_ck(id, 1) != 0) {
        return 0;
    }
    if (item_category_ck(id, 2) != 0) {
        return 0;
    }
    if (item_category_ck(id, 0x10) != 0) {
        return 0;
    }
    if (fn_8027403C(id) != 0) {
        return 0;
    }
    if ((u32)(id - 0x1B6) <= 1U || id == 0x2B || id == 0x35 || id == 0xDF) {
        return 0;
    }
    return 1;
}

/* True when the player's act/act-state pair is one of the two held combinations. */
s32 fn_80275394(struct _PLW* plw)
{
    switch (plw->field_0x00A) {
    case 0:
        if (plw->act_no == 0xB1 || plw->act_no == 0xA6) {
            return 1;
        }
        break;
    case 12:
        if (plw->act_no == 0xB) {
            return 1;
        }
        break;
    default:
        break;
    }
    return 0;
}

/* The per-act bonus the weapon slots contribute, out of the lobby parameter block's three rows. */
s16 fn_802753E4(struct _PLW* plw, u16 id)
{
    s16 value = 0;

    if (Pl_master_ck(plw) == 0) {
        return 0;
    }
    if (plw->field_0x446 != 0 && Pl_cat_skill_ck(plw, 0x27) == 0) {
        return 0;
    }
    if (id == lb_param_w.flag_0x0C[0]) {
        value = lb_param_w.value_0x10[0];
    }
    if (id == lb_param_w.flag_0x0C[1]) {
        value += lb_param_w.value_0x10[1];
    }
    if (id == lb_param_w.flag_0x0C[2]) {
        value += lb_param_w.value_0x10[2];
    }
    return value;
}

/* Marks the player as gone. */
void fn_802754B4(struct _PLW* plw)
{
    plw->field_0x19 = 1;
}

/* Starts the mode's act for the player's current state. */
void fn_802754C0(struct _PLW* plw, u8 flag)
{
    u8 kind = 0;

    if (Pl_master_ck(plw) == 1U) {
        if (PlayMode_ck() == 2) {
            kind = plw->chunk_ofs;
        }
        fn_80044B14(kind, flag);
    }
}

/* Copies (or clears) the player's hunter name in the move work record. */
void fn_8027552C(struct _PLW* plw, const char* src)
{
    if (src == NULL) {
        memset(plw->name_0xB05, 0, 10);
        return;
    }
    strcpy((char*)plw->name_0xB05, src);
}

/* The 11 act/motion pick rows `Pl_decide_mot_get` walks: motion, parameter and the two control
 * words each row is gated on. size: 0x58 */
PlMotRow pl_mot_pick_tbl[11] = {
    {0x02D1, 0x02BD, 0x2000, 0x2000}, {0x02D3, 0x02BF, 0x0800, 0x0800},
    {0x02D5, 0x02C3, 0x0400, 0x0400}, {0x02D7, 0x02C4, 0x1000, 0x1000},
    {0x02DA, 0x02C7, 0x2000, 0x2000}, {0x02D2, 0x02BE, 0x0800, 0x0800},
    {0x02D4, 0x02C0, 0x0400, 0x0400}, {0x02D6, 0x02C5, 0x1000, 0x1000},
    {0x02D8, 0x02C6, 0x0040, 0x4000}, {0x02D9, 0x02C1, 0x0080, 0x8000},
    {0x02DB, 0x02C2, 0x0030, 0x0014},
};

/* Picks the motion/parameter pair the player's current control state maps to. */
void Pl_decide_mot_get(u16* motion, u16* param)
{
    u16 index = (u16)(ran_suu(1) % 5);
    PlMotRow* row = pl_mot_pick_tbl;
    s32 i;

    for (i = 0; i < 11; i++) {
        switch (get_ControlType(0)) {
        case 1:
            if (i < 4) {
                if ((Psw[0].control_0x104 & row[i].control_a) != 0) {
                    index = (u16)i;
                }
            } else if (i < 8) {
                if ((Psw[0].control_0x104 & row[i].control_a) != 0 && (Psw[0].control_0x0FC & 4) != 0) {
                    index = (u16)i;
                }
            } else if ((Psw[0].control_0x0FC & row[i].control_a) != 0) {
                index = (u16)i;
            }
            break;
        case 2:
            if (i < 4) {
                if ((Psw[0].control_0x0DE & row[i].control_b) != 0) {
                    index = (u16)i;
                }
            } else if (i < 8) {
                if ((Psw[0].control_0x0DE & row[i].control_b) != 0 && (Psw[0].control_0x0D6 & 0x88) != 0) {
                    index = (u16)i;
                }
            } else if ((Psw[0].control_0x0D6 & row[i].control_b) != 0) {
                index = (u16)i;
            }
            break;
        default:
            break;
        }
    }
    *motion = row[index].motion;
    *param = row[index].param;
}

/* Resets every per-act field, then arms the new act's kind/number from its flag word `mask`. */
void pl_act_enter_raw(struct _PLW* plw, u8 kind, u16 no, u16 mask)
{
    u32 i;

    plw->field_0x35C = 0;
    plw->field_0x360 = 0;
    plw->act_step_0x05 = 0;
    plw->field_0x006 = 0;
    plw->field_0x007 = 0;
    plw->prev_act_kind = plw->field_0x00A;
    plw->prev_act_no = plw->act_no;
    plw->field_0x00A = (u8)kind;
    plw->act_no = (u16)no;
    plw->field_0x264 = 0;
    plw->field_0x030 = 0;
    if ((mask & 0x20) != 0) {
        plw->field_0x0B6 = (u16)((ran_suu(1) & 0xFF00) | (plw->field_0x0B6 & 0xFF));
    } else {
        plw->field_0x0B6 = (u16)ran_suu(1);
    }
    plw->field_0x354 = lbl_8079A080;
    plw->field_0x318 = 0;
    plw->field_0x313 = 0;
    plw->field_0x314 = 0;
    plw->field_0x31C = 0;
    plw->field_0x320 = 0;
    plw->field_0x31E = 0;
    plw->field_0x36A = 0;
    plw->field_0x388 = 0;
    for (i = 0; i < 0x30; i++) {
        plw->field_0x322[i] = 0;
    }
    if ((mask & 0x400) != 0) {
        plw->field_0x0A8 = (u16)(plw->field_0x058 - 0x2000);
    }
    if ((mask & 0x800) != 0) {
        plw->field_0x0A8 = (u16)(plw->field_0x058 + 0x2000);
    }
    if (Pl_master_ck(plw) == 1U && (mask & 0x1000) != 0) {
        s32 step;

        mask &= 0xEFFF;
        switch (plw->field_0x002) {
        case 2:
            step = 0x1000;
            break;
        case 7:
            step = 0x1000;
            break;
        case 8:
            step = 0x1000;
            break;
        case 3:
            step = 0x1000;
            break;
        default:
            step = 0;
            break;
        }
        if (Pl_master_ck(plw) == 1U) {
            if (fn_8026FB20(plw, 0x800) == 1U) {
                plw->field_0x0A8 += step;
            } else if (fn_8026FB20(plw, 0x400) == 1U) {
                plw->field_0x0A8 -= step;
            }
        }
    }
    if ((mask & 0x4000) == 0) {
        plw->field_0x445 = 0;
    }
    if ((mask & 0x80) == 0) {
        plw->field_0x0AC = 0;
    }
    if ((mask & 0x100) == 0) {
        plw->field_0x567 = (u8)(plw->field_0x567 & 0xFE);
    }
    if ((mask & 1) == 0) {
        plw->field_0x646 = 0;
        plw->field_0x648 = 0;
        plw->field_0x64A = 0;
        plw->field_0x64F = 0;
    }
    if ((mask & 0x2000) == 0) {
        plw->field_0x068 = lbl_8079A084;
        plw->field_0x06C = lbl_8079A084;
        plw->field_0x070 = lbl_8079A084;
    }
    fn_8026A2DC(plw);
    fn_8026A2F8(plw);
    fn_8027AC2C(plw, 0xFF, 0xFF);
    plw->field_0x662 = 0;
    plw->field_0x3A3 = 0;
    plw->field_0x3A2 = 0;
    plw->field_0x565 = 0;
    plw->field_0x566 = 0;
    if (Pl_master_ck(plw) == 1U && (mask & 2) == 0) {
        if ((mask & 0x10) != 0) {
            ((PlNetSend2)Pl_net_send)(plw, 6);
        } else if ((mask & 0x40) != 0) {
            ((PlNetSend2)Pl_net_send)(plw, 9);
        } else {
            ((PlNetSend2)Pl_net_send)(plw, 1);
        }
        plw->field_0x012 = 0xF;
    }
}

/* Sets the per-act marker byte and arms the act. */
void pl_act_enter(struct _PLW* plw, s32 a, u16 b, u16 c)
{
    plw->field_0x00E = 1;
    pl_act_enter_raw(plw, a, b, c);
}

/* The "2" marker's act entry, with the +0x256 timer preset. */
void fn_80275ADC(struct _PLW* plw, s32 a, u16 b, u16 c)
{
    plw->field_0x584 = 1;
    plw->field_0x256 = 0x1C2;
    pl_act_enter_raw(plw, a, b, c);
}

/* Writes the player's act kind byte. */
void fn_80275AFC(struct _PLW* plw, s8 value)
{
    plw->kind_0x09 = value;
}

/* Arms the act's three status bits from a packed mask. */
void Pl_act_set_motion(struct _PLW* plw, u16 a, u32 b, u32 c)
{
    switch ((u8)a) {
    case 1:
        fn_80275AFC(plw, 1);
        break;
    case 2:
        fn_80275AFC(plw, 2);
        break;
    case 3:
        fn_80275AFC(plw, 3);
        break;
    default:
        fn_80275AFC(plw, 0);
        break;
    }
    if ((a & 0x8000) != 0) {
        fn_8026FEF0(plw, 1);
    } else {
        pl_act_set_flag(plw, 1);
    }
    if (b == 0) {
        fn_8026FEF0(plw, 2);
    } else {
        pl_act_set_flag(plw, 2);
    }
    if (c == 0) {
        fn_8026FEF0(plw, 4);
        return;
    }
    pl_act_set_flag(plw, 4);
}

/* The act-state dispatcher's motion hand-off. */
void fn_80275C18(struct _PLW* plw, u32 motion, u32 a, u32 b, u32 c)
{
    if ((u16)c == 0) {
        Pl_chr_setX(plw, (u16)motion, a, b);
    } else {
        Pl_chr_set_attr_default(plw, (u16)motion, a, b);
    }
}

/* Picks and arms the entry motion for the player's current act state. */
void fn_80275C34(struct _PLW* plw, u32 kind, u32 a, u32 b, u32 c, u32 d)
{
    plw->field_0x5C4 = 0;
    plw->field_0x310 = 0;
    plw->kind_0x09 = (u8)kind;
    if (kind != 1) {
        if (kind != 3) {
            if (plw->field_0x585 != 0) {
                if (plw->field_0x018 == 1) {
                    if (plw->field_0x002 == 8) {
                        if (fn_80331104() == 1U) {
                            fn_80275C18(plw, 0x3E9, a, b, d);
                        } else {
                            fn_80275C18(plw, 0x3FC, a, b, d);
                        }
                    } else {
                        fn_80275C18(plw, 0x3E9, a, b, d);
                    }
                } else {
                    fn_80275C18(plw, 0x18, a, b, d);
                }
                pl_act_enter(plw, 0, 0x6A, (u16)c);
                return;
            }
            if (plw->field_0x018 == 1) {
                if (plw->field_0x002 == 8) {
                    if (fn_80331104() == 1U) {
                        fn_80275C18(plw, 0x3E9, a, b, d);
                    } else {
                        fn_80275C18(plw, 0x3FC, a, b, d);
                    }
                } else {
                    fn_80275C18(plw, 0x3E9, a, b, d);
                }
                pl_act_enter(plw, 0, 0, (u16)c);
                return;
            }
            if (fn_8027AC18(plw) == 1U) {
                fn_80275C18(plw, 0x190, a, b, d);
                pl_act_enter(plw, 0xA, 0, (u16)c);
                return;
            }
            if (plw->field_0x37A <= 0x96) {
                fn_80275C18(plw, 0x12F, a, b, d);
            } else {
                switch (pl_act_kind_get(plw->area_0x16)) {
                case 1:
                    if (Pl_Skill_ck(plw, 0x7C) == 1U || Pl_Skill_ck(plw, 0x7D) == 1U
                        || Pl_condition_ck(plw, 0x400) == 1U) {
                        fn_80275C18(plw, 1, a, b, d);
                    } else {
                        fn_80275C18(plw, 0x15, a, b, d);
                    }
                    break;
                case 3:
                    if (Pl_Skill_ck(plw, 0x7D) == 1U || Pl_condition_ck(plw, 0x400) == 1U) {
                        fn_80275C18(plw, 1, a, b, d);
                    } else {
                        fn_80275C18(plw, 0x15, a, b, d);
                    }
                    break;
                case 2:
                    if (Pl_Skill_ck(plw, 0x80) == 1U || Pl_Skill_ck(plw, 0x81) == 1U
                        || Pl_condition_ck(plw, 0x800) == 1U) {
                        fn_80275C18(plw, 1, a, b, d);
                    } else {
                        fn_80275C18(plw, 0x14B, a, b, d);
                    }
                    break;
                case 4:
                    if (Pl_Skill_ck(plw, 0x81) == 1U || Pl_condition_ck(plw, 0x800) == 1U) {
                        fn_80275C18(plw, 1, a, b, d);
                    } else {
                        fn_80275C18(plw, 0x14B, a, b, d);
                    }
                    break;
                default:
                    fn_80275C18(plw, 1, a, b, d);
                    break;
                }
            }
            pl_act_enter(plw, 0, 0, (u16)c);
            return;
        }
        if (plw->field_0x018 == 0) {
            if (fn_80276800(plw, 0) == 1) {
                fn_80275C18(plw, 0x79, a, b, d);
            } else if (plw->field_0x37A <= 0x96) {
                fn_80275C18(plw, 0x168, a, b, d);
            } else if (Pl_suimen_ck(plw) == 1U) {
                fn_80275C18(plw, 0x76, a, b, d);
            } else {
                fn_80275C18(plw, 0x64, a, b, d);
            }
        } else if (plw->field_0x002 == 8) {
            if (fn_80331104() == 1U) {
                fn_80275C18(plw, 0x41A, a, b, d);
            } else {
                fn_80275C18(plw, 0x438, a, b, d);
            }
        } else {
            fn_80275C18(plw, 0x41A, a, b, d);
        }
        pl_act_enter(plw, 0, 0x16, (u16)(c | 0x80));
        return;
    }
    if (plw->field_0x585 != 0) {
        fn_80275C18(plw, 0x18, a, b, d);
        pl_act_enter(plw, 0, 0x6A, (u16)c);
        return;
    }
    fn_80275C18(plw, 8, a, b, d);
    if (fn_8027AC18(plw) == 1U) {
        pl_act_enter(plw, 0xA, 0xE, (u16)c);
        return;
    }
    pl_act_enter(plw, 0, 0x1F, (u16)c);
}

/* The three no-argument act-state entries. */
void Pl_act_set_motion_slot(struct _PLW* self, u32 a, u32 b, u32 c)
{
    fn_80275C34(self, a, b, c, 0, 0);
}

void fn_802761C4(struct _PLW* self, u32 a, u32 b, u32 c)
{
    fn_80275C34(self, a, b, c, 12, 1);
}

void fn_802761D0(struct _PLW* self, u32 a, u32 b, u32 c)
{
    fn_80275C34(self, a, b, c, 2, 1);
}

/* The motion-parameter act-state entry. */
void fn_802761DC(struct _PLW* self, u32 a, u32 b, u32 c)
{
    fn_80275C34(self, a, b, c, 1, 0);
}

/* Clears the act's hold latch and re-enters the act state. */
void pl_act_reenter(struct _PLW* self, s32 a, s32 b, s32 c)
{
    self->field_0x442 = 0;
    self->field_0x30A = 0xFF;
    fn_80275C34(self, a, b, c, 0, 0);
}

/* True when the given motion value fits inside the act's stagger budget. */
s32 fn_80276254(struct _PLW* self, s32 value)
{
    return (s16)value <= self->field_0x378;
}

/* True when the given motion value fits inside the act's hold gauge. */
s32 fn_80276270(struct _PLW* plw, s32 value)
{
    return (s16)value <= plw->field_0x370;
}

/* Adds `delta` to the act's hold gauge, clamped to 0..`+0x372`. */
void pl_act_add_hold_gauge(struct _PLW* plw, s16 delta)
{
    if (Pl_master_ck(plw) != 0 && (Pl_motion_input_ck(0) != 1U || delta >= 0) && plw->field_0x00A != 8) {
        s16 value;

        plw->field_0x370 += delta;
        value = plw->field_0x370;
        if (value <= 0) {
            plw->field_0x370 = 0;
        }
        if (plw->field_0x370 >= plw->field_0x372) {
            plw->field_0x370 = plw->field_0x372;
        }
        if (plw->field_0x376 < plw->field_0x370) {
            plw->field_0x376 = plw->field_0x370;
        }
    }
}

/* The act's frame step: runs the hold gauge down and either enters the act's next motion or dumps
 * the player out of it.  Returns 1 when the act ended this frame. */
s32 fn_8027633C(struct _PLW* plw, s16 delta, s8* out)
{
    nw4r::math::VEC3 pos;
    s16 before;

    VEC3_ctor(&pos);
    if (out != NULL) {
        *out = 0;
    }
    if (Pl_master_ck(plw) == 0) {
        return 0;
    }
    before = plw->field_0x370;
    pl_act_add_hold_gauge(plw, delta);
    if (plw->field_0x370 <= 0) {
        if (Pl_cat_skill_ck(plw, 0x2C) == 1U) {
            if (plw->field_0x447 == 0 && before >= 0x40) {
                plw->field_0x447++;
                plw->field_0x370 = 1;
                plw->field_0x376 = 1;
                ((PlJointHolder*)plw->physics_0x13C)->chr_0x04.get_joint_wpos(3, &pos);
                fn_800FC0F0(&pos, 0, 0, plw->area_0x16, (_CP_VECTOR*)&plw->param_0x54, lbl_8079A080);
                if (out != NULL) {
                    *out = 1;
                }
                return 0;
            }
        }
        plw->field_0x376 = 0;
        if (plw->kind_0x09 != 2) {
            fn_80278BE4(plw);
            return 1;
        }
        if (plw->field_0x00A != 6 && Pl_act_ck(plw, 2, 1) == 0) {
            pl_act_enter_raw(plw, 2, 1, 0);
        }
    }
    return 0;
}

/* The same step, suppressed while the shell/ammo latch is latched. */
s32 fn_802764B0(struct _PLW* plw, s16 delta, s8* out)
{
    if (fn_8027E1E4(plw) == 1U) {
        return 0;
    }
    return fn_8027633C(plw, delta, out);
}

/* The same step for a signed gauge delta (a negative one also moves the stagger gauge). */
s32 fn_80276514(struct _PLW* plw, s16 delta, s8* out)
{
    s16 value;

    if (Pl_master_ck(plw) == 0) {
        return 0;
    }
    value = 0;
    if (delta < 0) {
        plw->field_0x376 += delta;
        if (plw->field_0x376 < plw->field_0x370) {
            value = plw->field_0x376 - plw->field_0x370;
        }
    } else {
        value = delta;
    }
    return fn_8027633C(plw, value, out);
}

/* Applies the skill-tier damage bonus and steps the hold gauge. */
void fn_802765B4(struct _PLW* plw, s16 amount)
{
    s16 value = amount;

    if (amount > 0) {
        if (Pl_cat_skill_ck(plw, 3) == 1U) {
            value += amount / 10;
        }
        if (Pl_Skill_ck(plw, 0x98) == 1U) {
            value += value / 4;
        } else if (Pl_Skill_ck(plw, 0x99) == 1U) {
            value -= value / 4;
        }
    }
    pl_act_add_hold_gauge(plw, value);
}

/* Grows the act's gauge ceilings (and, when `a` is 0, the soft cap) by `amount`. */
void fn_80276690(struct _PLW* plw, s32 amount, u8 a)
{
    if (Pl_master_ck(plw) != 0) {
        s16 value;

        plw->field_0x372 += (s16)amount;
        if (plw->field_0x372 <= 1) {
            plw->field_0x372 = 1;
        }
        if (plw->field_0x372 >= 0x96) {
            plw->field_0x372 = 0x96;
        }
        value = plw->field_0x372;
        if (plw->field_0x370 >= value) {
            plw->field_0x370 = value;
        }
        if (plw->field_0x376 >= value) {
            plw->field_0x376 = value;
        }
        if (a == 0) {
            plw->field_0x374 += (s16)amount;
            if (plw->field_0x374 <= 1) {
                plw->field_0x374 = 1;
            }
            if (plw->field_0x374 >= 0x96) {
                plw->field_0x374 = 0x96;
            }
        }
    }
}

/* Adds to the act's stagger budget up to its 150-point ceiling. */
void fn_80276778(struct _PLW* plw, s16 amount)
{
    if (amount > 0) {
        plw->field_0x382 = 0;
    }
    plw->field_0x380 += amount;
    if (plw->field_0x380 >= 0x96) {
        plw->field_0x380 = 0x96;
    }
    fn_802767B4(plw, 0x96);
}

/* Adds to the act's current stagger value, clamped to 0..`+0x380`. */
void fn_802767B4(struct _PLW* plw, s16 amount)
{
    if (amount > 0) {
        plw->field_0x382 = 0;
    }
    plw->field_0x37E += amount;
    if (plw->field_0x37E >= plw->field_0x380) {
        plw->field_0x37E = plw->field_0x380;
        return;
    }
    if (plw->field_0x37E < 0) {
        plw->field_0x37E = 0;
    }
}

/* True while the act's current stagger value is inside `v`. */
u32 fn_80276800(struct _PLW* self, s32 v)
{
    return self->field_0x37E <= v;
}

/* True while the act's hold-gauge re-arm word is set. */
u32 Pl_act_state_ck(struct _PLW* plw)
{
    if (Pl_master_ck(plw) == 0) {
        return 0;
    }
    return plw->field_0x412 > 0;
}

/* Steps the act's stagger budget by `amount`, clamped to 0..`+0x37A`. */
void fn_80276868(struct _PLW* plw, s16 amount)
{
    if (Pl_master_ck(plw) != 0 && (amount >= 0 || Pl_act_state_ck(plw) != 1)) {
        s16 value;

        plw->field_0x378 += amount;
        value = plw->field_0x378;
        if (value <= 0) {
            plw->field_0x378 = 0;
        }
        if (plw->field_0x378 >= plw->field_0x37A) {
            plw->field_0x378 = plw->field_0x37A;
        }
    }
}

/* The same step with the defence-skill and combo modifiers folded in. */
void pl_act_gauge_gate(struct _PLW* plw, s16 amount)
{
    s16 value = amount;

    if (Pl_master_ck(plw) == 0) {
        return;
    }
    if (value < 0) {
        if (Pl_dm_condition_ck(plw, 0x80) == 1U) {
            value = value * 4 - value;
        } else if (Pl_dm_condition_ck(plw, 0x40) == 1U) {
            value = value * 4 - value;
        }
        if (value == -1) {
            if (Pl_Skill_ck(plw, 0xA8) == 1U && (plw->frame_0x020 & 1) != 0) {
                return;
            }
        } else if (Pl_Skill_ck(plw, 0xA8) == 1U) {
            value = (s16)(((u32)value >> 0x1FU) + value >> 1);
        }
        if (Pl_Skill_ck(plw, 0xA9) == 1U && plw->frame_0x020 % 5 == 0) {
            value = value * 2;
        }
    }
    fn_80276868(plw, value);
}

/* The act's guard-break entry: picks the stagger step from the defence skills and applies it. */
void fn_80276A3C(struct _PLW* plw)
{
    s16 step = -1;
    s32 mask = 0;

    if (Pl_dm_condition_ck(plw, 0x80) == 1U) {
        step = -3;
    } else if (Pl_dm_condition_ck(plw, 0x40) == 1U) {
        step = -3;
    }
    if (Pl_Skill_ck(plw, 0xA8) == 1U) {
        mask = 1;
    }
    if (Pl_cat_skill_ck(plw, 0x17) == 1U) {
        s32 had = mask;

        mask = 3;
        if (had == 0) {
            mask = 1;
        }
    }
    if ((u16)mask == 0 || (plw->frame_0x020 & (u16)mask) == 0) {
        if (Pl_Skill_ck(plw, 0xA9) == 1U && plw->frame_0x020 % 5 == 0) {
            step *= 2;
        }
        fn_80276868(plw, step);
    }
}
