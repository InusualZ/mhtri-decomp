/* The player act state-machine band `Pl/fn_80258FCC.cpp`.
 *
 * `.text` 0x80258FCC-0x8025F088 (74 functions, 0x60BC B), extab 0x8001219C-0x8001239C and
 * extabindex 0x8002F154-0x8002F454; this lib's `cflags_pl` (Wii/1.0).
 *
 * Home is `Pl` (brief section 2 class 3): every function's first argument is the player work
 * `_PLW*`, the gates are the Pl siblings (`Pl_master_ck`, `Pl_act_ck`, `Pl_Skill_ck`,
 * `Pl_frame_check`), and the bracketing registered units are `Pl/fn_80241558.cpp` (ends
 * 0x802430E8) and `Pl/fn_80262940.cpp` (starts 0x80262940).  No `__FILE__` string is reachable
 * from the range and `dumpmap.py lookup 0x80258FCC` answers only `zz_0258fcc_`, so the file keeps
 * the map's `fn_XXXXXXXX` stem (class 4).
 *
 * rule 7 deferred: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/symedit.py range 0x80258FCC 0x8025F088` - every row is `fn_` - and
 * `python tools/splits/tudiscover.py at 0x80258FCC`, which reports 0 must-link anchors, no
 * `__FILE__` anchor and no owned data).
 *
 * The seam is unproven.  The extab/extabindex runs do break on the proposal boundaries - the
 * extabindex run 0x8002F154-0x8002F454 is exactly this run's 64 framed functions and the
 * bracketing proposals' runs abut it - but the extab table is ordered by function address, so
 * that is consistency, not proof.
 *
 * RESIDUAL (this pass).  13 of the 74 functions are written, all above the brief's bar: 11
 * byte-identical, `fn_80259684` 98.32 % (476 B) and `fn_8025A7FC` 94.68 % (312 B).
 *  - `fn_80259684`: the target materialises the `0` argument of `fn_80277DAC` AFTER the two float
 *    arguments (`lfs f1, lbl_80799F00; lfs f2, lbl_80799F04; li r4,0`), ours before them.  Same
 *    instruction count and size; only the materialisation order differs.
 *  - `fn_8025A7FC`: the target schedules the `threshold = 15` materialisation between the
 *    `cmplwi` and the `bne` of the `field_0x128 == 2` test, ours before the load.  Same size.
 *  - The remaining 61 functions are not written, address order: fn_80259860 744 B, fn_80259B48 240 B,
 *    fn_80259C38 304 B, fn_80259D68 112 B, fn_80259DD8 288 B, fn_80259EF8 252 B, fn_80259FF4 312 B,
 *    fn_8025A12C 428 B, fn_8025A2D8 1276 B, fn_8025A934 260 B, fn_8025AA38 576 B, fn_8025AC78 276 B,
 *    fn_8025AD8C 288 B, fn_8025AEAC 316 B, fn_8025AFE8 236 B, fn_8025B0F8 608 B, fn_8025B358 320 B,
 *    fn_8025B498 172 B, fn_8025B544 160 B, fn_8025B5E4 180 B, fn_8025B698 48 B, fn_8025B6C8 168 B,
 *    fn_8025B770 184 B, fn_8025B828 152 B, fn_8025B8C0 500 B, fn_8025BAB4 148 B, fn_8025BB48 180 B,
 *    fn_8025BBFC 332 B, fn_8025BD48 220 B, fn_8025BE24 328 B, fn_8025BF6C 304 B, fn_8025C09C 480 B,
 *    fn_8025C27C 452 B, fn_8025C440 292 B, fn_8025C564 620 B, fn_8025C7D0 204 B, fn_8025C89C 216 B,
 *    fn_8025C974 340 B, fn_8025CAC8 496 B, fn_8025CCB8 432 B, fn_8025CE68 772 B, fn_8025D16C 228 B,
 *    fn_8025D250 372 B, fn_8025D3C4 308 B, fn_8025D4F8 156 B, fn_8025D594 296 B, fn_8025D6BC 404 B,
 *    fn_8025D850 260 B, fn_8025D954 228 B, fn_8025DA38 420 B, fn_8025DBDC 352 B, fn_8025DD3C 252 B,
 *    fn_8025DE38 180 B, fn_8025DEEC 140 B, fn_8025DF78 336 B, fn_8025E0C8 380 B, fn_8025E448 1960 B,
 *    fn_8025EBF0 104 B, fn_8025EC58 152 B, fn_8025ED00 756 B, fn_8025EFF4 148 B
 *    They are the next pass's work.
 *
 * Load-bearing source shapes found here (they cost several variants each):
 *  - a byte field's `x = x - 1` emits a redundant narrowing (`extsb r0,r0`); `x--` does not.
 *  - a `*timer -= 10` on an `s16*` emits `extsh r0,r0`; `*timer -= (s16)10` does not.
 *  - an `s16` parameter that is only compared emits `extsh`+`cmpwi` where retail keeps `cmpwi`;
 *    the parameter is `s32` in that case and the `extsh` belongs to the callee's narrower one.
 *  - a redundant `(u8)` cast on the *second* of two byte stores is what makes MWCC narrow it.
 *  - the `if (cond >= 1) { big }` shape (rather than `if (cond < 1) { small }`) is what puts the
 *    small block at the end of the function where retail has it.
 */

#include "types.h"
#include "nw4r/math.h"
#include "pl.h"
#include "Pl/pl_act.h"
#include "Pl/pl_master.h"
#include "Pl/pl_skill.h"
#include "Pl/fn_8024F200.h"   /* 0x80257E70 - the owner of the three-timer reset this unit calls */
#include "fn_8004CAD8.h"
#include "unsplit/Pl.h"

/* Drives one tick of the player's act state: it ages the shell timer the act belongs to, and
 * picks the next action code from the motion/act predicates when the timer runs out. */
extern "C" void fn_80258FCC(_PLW* self, s16 a2, s16 a3, u8 a4)
{
    s16* timer;

    if (Pl_master_ck(self) == 0) {
        return;
    }
    if (a3 == 0) {
        timer = &self->field_0x418;
    } else {
        timer = &self->field_0x41E;
    }
    if (a4 == 0) {
        if ((s8)self->field_0x444 == 0) {
            if (fn_8026F888(self) == 1) {
                if (*timer > 0) {
                    *timer -= (s16)10;
                    if (*timer < 0) {
                        *timer = 0;
                    }
                    self->field_0x444 = 1;
                }
            }
        } else {
            self->field_0x444--;
        }
    }
    if (*timer <= 0) {
        fn_80245DA0(self, 0);
        if (a3 == 0) {
            self->field_0x3D8 &= ~0x300;
            fn_80275AC4(self, 6, 45, 0);
        } else {
            self->field_0x3D8 &= ~0x300;
            fn_80275AC4(self, 6, 65, 0);
        }
        return;
    }
    if (a4 != 0) {
        return;
    }
    if (fn_8026A644(self, 4) == 1) {
        if (self->field_0x378 >= 150) {
            self->field_0x0A8 = self->field_0x058;
            if (a3 == 0) {
                fn_80275AC4(self, 6, 73, 4);
            } else {
                fn_80275AC4(self, 6, 72, 4);
            }
            return;
        }
    }
    if (fn_8026F908(self, 0) >= 1) {
        fn_8026A3A8(self);
        if (fn_8026A644(self, 0) == 1 && (s8)self->field_0x266 == 0) {
            if (a3 == 0) {
                if (Pl_act_ck(self, 6, 47) != 0) {
                    return;
                }
                fn_80275AC4(self, 6, 47, 0);
            } else {
                if (Pl_act_ck(self, 6, 67) != 0) {
                    return;
                }
                fn_80275AC4(self, 6, 67, 0);
            }
            return;
        }
        if (a3 == 0) {
            if (Pl_act_ck(self, 6, 46) != 0) {
                return;
            }
            if ((s8)self->field_0x266 != 0) {
                return;
            }
            fn_80275AC4(self, 6, 46, 0);
        } else {
            if (Pl_act_ck(self, 6, 66) != 0) {
                return;
            }
            if ((s8)self->field_0x266 != 0) {
                return;
            }
            fn_80275AC4(self, 6, 66, 0);
        }
        return;
    }
    if (fn_8026FE98((_ENEMY_WORK*)self, 0x1C0) == 0) {
        return;
    }
    if (a2 == 1 && self->field_0x28 != 0) {
        return;
    }
    if (a3 == 0) {
        fn_80275AC4(self, 6, 44, 0);
    } else {
        fn_80275AC4(self, 6, 64, 0);
    }
}

/* Advances the act's own three-step state and hands the tick to `fn_80258FCC` once the timer the
 * caller passed has been picked. */
extern "C" void fn_80259310(_PLW* self, s32 a2, s32 a3)
{
    u8 flag;

    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        flag = 0;
        self->field_0x18 = 0;
        fn_8027AC0C(self);
        Pl_chr_setX(self, 51, 4, 0);
        if (a2 == 0) {
            self->field_0x3F6 = 0;
            self->field_0x3F0 = 0;
            fn_80257E70(self);
            fn_80245DA0(self, 0);
            if (a3 == 0) {
                self->field_0x418 = 900;
            } else {
                self->field_0x41E = 450;
            }
            self->field_0x45A = 0;
            self->field_0x45C = 0;
            self->field_0x28 = 4;
        } else {
            fn_80245DA0(self, 1);
            self->field_0x28 = 4;
        }
        fn_80275B04(self, 0, 0, 1);
        fn_8027D4F0(self);
        fn_802770E8(self, (u32)lbl_805C4F5C, 0);
    case 1:
        self->field_0x28 = self->field_0x28 - 1;
        if (self->field_0x28 <= 0) {
            self->act_step_0x05++;
        }
        flag = 1;
        break;
    case 2:
        flag = 0;
        break;
    }
    if (Pl_master_ck(self) == 1) {
        fn_80258FCC(self, 0, a3, flag);
    }
}

/* Two-step act: arms the weapon pose and clears the act flag, then runs the tail predicate. */
extern "C" void fn_8025948C(_PLW* self)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x18 = 0;
        fn_80275B04(self, 0, 0, 1);
        fn_8027D4F0(self);
        fn_8026A224(self, 307, 6, 0);
        fn_8027AC0C(self);
        self->field_0x354 = lbl_80799EB0;
        break;
    case 1:
        if (fn_8026A33C(self) == 1) {
            fn_80276238(self, 0, 4, 0);
        }
        break;
    }
}

/* Two-step act: arms the timed pose, then per tick nudges the actor along the heading and counts the
 * timer down while `fn_80258FCC` runs. */
extern "C" void fn_8025953C(_PLW* self, s32 a2)
{
    VEC3 v;

    fn_80043EA8(&v);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x28 = 20;
        self->field_0x266 = 30;
        fn_8026A224(self, 52, 4, 0);
        fn_8027AC0C(self);
        fn_80277C58(self);
        fn_80275B04(self, 0, 0, 1);
        fn_8027D4F0(self);
        fn_8026FEC0(self, 0x40);
        fn_802770E8(self, (u32)lbl_805C4F5C, 0);
        return;
    case 1:
        v.x = lbl_80799E00;
        v.y = lbl_80799E00;
        v.z = lbl_80799E2C;
        rotVecY(&v, self->field_0x058);
        self->field_0x03C = self->field_0x03C + v.x;
        self->field_0x040 = self->field_0x040 + v.y;
        self->field_0x044 = self->field_0x044 + v.z;
        if (self->field_0x28 > 0) {
            self->field_0x28 = self->field_0x28 - 1;
        }
        fn_80258FCC(self, 1, a2, 0);
        return;
    }
}

/* Two-step act: arms the pose picked by the actor's SE name set, then per tick either steps to the
 * next action or nudges the actor along the heading while `fn_80258FCC` runs. */
extern "C" void fn_80259684(_PLW* self, s32 a2)
{
    VEC3 v;

    fn_80043EA8(&v);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        if (self->se_name_set == 0) {
            Pl_chr_setX(self, 53, 4, 0);
        } else {
            Pl_chr_setX(self, 59, 4, 0);
        }
        fn_8027AC0C(self);
        self->field_0x28 = 20;
        self->field_0x266 = 30;
        fn_80277C58(self);
        fn_80275B04(self, 0, 0, 1);
        fn_8027D4F0(self);
        fn_8026FEC0(self, 0x180);
        fn_802770E8(self, (u32)lbl_805C4F5C, 0);
        return;
    case 1:
        if (Pl_master_ck(self) == 0) {
            return;
        }
        if (fn_80277DAC(self, 0, lbl_80799F00, lbl_80799F04) != 0) {
            self->field_0x0A8 = (u16)self->field_0x058;
            if (a2 == 0) {
                fn_80275AC4(self, 6, 48, 0);
                return;
            }
            fn_80275AC4(self, 6, 68, 0);
            return;
        }
        v.x = lbl_80799E00;
        v.y = lbl_80799E00;
        v.z = lbl_80799F08;
        rotVecY(&v, self->field_0x058);
        self->field_0x03C = self->field_0x03C + v.x;
        self->field_0x040 = self->field_0x040 + v.y;
        self->field_0x044 = self->field_0x044 + v.z;
        if (self->field_0x28 > 0) {
            self->field_0x28 = self->field_0x28 - 1;
        }
        fn_80258FCC(self, 1, a2, 0);
        return;
    }
}

/* One 0x16-byte entry of the `_PLW::field_0x318` scan table `fn_8025E298` walks; `id_0x00` == 0xFF
 * terminates the run. size: 0x16 */
typedef struct PlScanEntry {
    /* +0x00 */ u8 id_0x00;
    /* +0x01 */ u8 pad_0x01[0x1];
    /* +0x02 */ u16 key_0x02;
    /* +0x04 */ u8 pad_0x04[0x2];
    /* +0x06 */ u16 flags_0x06;
    /* +0x08 */ u8 pad_0x08[0x6];
    /* +0x0E */ u16 flags_0x0E;
    /* +0x10 */ u8 pad_0x10[0x6];
} PlScanEntry; /* size: 0x16 */

/* Re-arms the act's 300-frame cooldown. */
extern "C" void fn_8025A7D4(_PLW* self)
{
    self->field_0x3A8 = 300;
}

/* Loads the act's strike budget: `a2` steps remain before the cooldown re-arms. */
extern "C" void fn_8025A7E0(_PLW* self, u8 a2)
{
    self->field_0x3A5 = a2;
    self->field_0x3A4 = (u8)a2;
    self->field_0x3A6 = 0;
    self->field_0x3A7 = 0;
    fn_8025A7D4(self);
}

/* Counts the act's strike window down and re-arms the cooldown once the per-skill strike budget is
 * spent; `field_0x309`'s 0x40 bit is the "struck" status the shell layer reads. */
extern "C" void fn_8025A7FC(_PLW* self)
{
    u8 threshold;

    if (Pl_master_ck(self) == 0) {
        return;
    }
    if ((s8)self->field_0x3A7 > 0) {
        self->field_0x3A7--;
    }
    if (Pl_cat_skill_ck(self, 48) == 1) {
        if (self->field_0x128 == 2) {
            threshold = 6;
        } else {
            threshold = 8;
        }
    } else {
        threshold = 15;
        if (self->field_0x128 == 2) {
            threshold = 12;
        }
    }
    if (self->field_0x3A8 > 0) {
        if (self->field_0x378 <= 0) {
            return;
        }
        if (fn_8026F888(self) != 1) {
            return;
        }
        if ((s8)self->field_0x3A7 != 0) {
            return;
        }
        self->field_0x3A6++;
        self->field_0x3A7 = 1;
        if (self->field_0x3A6 < threshold) {
            return;
        }
        self->field_0x3A6 = 0;
        self->field_0x309 |= 0x40;
        if (self->field_0x3A4 != 0) {
            self->field_0x3A4 = self->field_0x3A4 - 1;
        }
        fn_8025A7D4(self);
    } else {
        self->field_0x3A6 = 0;
        fn_8025A7D4(self);
    }
}

/* Ages the strike counter and saturates it at ten. */
extern "C" void fn_8025B0D4(_PLW* self)
{
    self->field_0x446++;
    if (self->field_0x446 >= 10) {
        self->field_0x446 = 10;
    }
}

/* Tests the actor's action number against the handful of actions the held-item window accepts. */
extern "C" u32 fn_8025E244(_PLW* self)
{
    u16 action;

    if (self->field_0x00A != 0) {
        return 0;
    }
    action = self->act_no;
    if ((u32)(action - 0x31) <= 3 || (u32)(action - 0x61) <= 2 || (u32)(action - 0x2C) <= 1 ||
        (s32)action == 0x7F) {
        return 1;
    }
    return 0;
}

/* Scans the `field_0x318` table for the entry whose id/key pair the caller asks for, honouring the
 * entry's two suppression flags. */
extern "C" u32 fn_8025E298(_PLW* self, u16 a2, u16 a3)
{
    PlScanEntry* entry;

    entry = (PlScanEntry*)self->field_0x318;
    if (entry == NULL || (s8)self->field_0x313 > 0) {
        return 0;
    }
    while (entry->id_0x00 != 0xFF) {
        if ((((entry->flags_0x06 & 0x8000) == 0) || ((entry->flags_0x0E & 0x4000) == 0) ||
             (s8)self->field_0x314 <= 0) &&
            entry->id_0x00 == a2 && entry->key_0x02 == a3) {
            return 1;
        }
        entry++;
    }
    return 0;
}

/* Arms the held-item stance when the actor stands still and the item pair is in the scan table. */
extern "C" u32 fn_8025E32C(_PLW* self)
{
    if (self->field_0x002 == 1) {
        if (self->field_0x308 != 0) {
            return 0;
        }
    } else if (self->field_0x18 == 0 && self->field_0x308 != 0) {
        return 0;
    }
    if ((fn_8025E298(self, 0, 0xB1) == 1 || fn_8025E298(self, 0xC, 0xB) == 1) &&
        fn_8027D8A0(self, 1) == 1) {
        self->field_0x3E5 = 0x10;
        self->field_0x3E4 = 0x10;
        self->field_0x3E7 = 0;
        self->field_0x3E8 = 0;
        return 1;
    }
    if (fn_8025E298(self, 0, 0xA6) == 1 && fn_8027D8A0(self, 0) == 1) {
        self->field_0x3E5 = 0x10;
        self->field_0x3E4 = 0x10;
        self->field_0x3E7 = 0;
        self->field_0x3E8 = 0;
        return 1;
    }
    return 0;
}

/* Toggles the act's boolean latch. */
extern "C" void fn_8025ECF0(_PLW* self)
{
    self->field_0x5E6 = self->field_0x5E6 ^ 1;
}
