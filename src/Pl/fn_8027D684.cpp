/*
 * Player act/equipment cluster `Pl/fn_8027D684`: the continuation of the Pl_act band after
 * `Pl/pl_act.cpp`, from the act step byte through the player's equipment helpers.  `.text`
 * 0x8027D684-0x802840DC (138 functions, 0x6A58 B) with its own exception tables - extab
 * 0x80012B54-0x80012E7C and extabindex 0x8002FF7C-0x80030438 (101 framed functions, one 8-byte
 * extab record and one 12-byte extabindex record each, which is what pins both ranges).
 *
 * Home is `Pl`: every function's first argument is the player work `_PLW`, the gates are the Pl
 * siblings (`Pl_master_ck`, `Pl_frame_check`, `Pl_Skill_ck`), the equipment helpers take the `_EQUIP`
 * record `include/pl.h` owns, and both bracketing registered units are Pl.  The target object carries
 * extab/extabindex, so the flags are the lib's `cflags_pl` (`-Cpp_exceptions on`).
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/symedit.py range 0x8027D684 0x802840DC`; the runtime dump answers `zz_<addr>_`
 * for every one of them - `python tools/symbols/dumpmap.py lookup`).
 *
 * Residual (work in progress - 32 of the range's 138 functions are reconstructed, 1280 of 27224 B
 * byte-identical; the other 106 keep the target bytes):
 *   * `fn_8027D968` (0x2FC B), `fn_8027E5E4`, `fn_8027F3CC` and `fn_8027D8A0` carry paired-single /
 *     `cror` instructions the frontend cannot emit (playbook, "A paired-single op in a function").
 *   * seven written rows sit above the 80 % bar with a known shape residual: `fn_8027DDC4` 91.2 (the
 *     negated/nested if-chain is retail's), `fn_8027DCA8` 84.2 (`return a || b;` shares the `li r3,1`
 *     tail), `fn_8027DE88` 77.7 and `fn_8027DF38` 67.3 (retail lays the id-scan loop head out *after*
 *     the body and falls through into it; the head-first `for (;;)` costs one `b`), `fn_8027E06C` 71.7,
 *     `fn_8027DFE4` 65.2 (the `case 12` equality if-converts to the branchless bool form) and
 *     `fn_8027DCE0` 65.9 (the comparison tree is identical, retail merges every `return 0` into one
 *     tail block where ours repeats the `li r3,0`).
 *   * the band header `include/unsplit/Pl.h` still declares six symbols this unit now owns
 *     (`fn_8027D7EC`, `fn_8027D8A0`, `fn_8027E1E4`, `fn_8027E220`, `fn_8027EBA8`, `fn_8027EE24`); the
 *     definitions below match those spellings so no consumer breaks, and moving them into an owner
 *     header needs the seven consumer files (which also spell three of them differently:
 *     `src/ef/eft001.cpp` says `void fn_8027D7EC`, `src/Pl/pl_skill.cpp` says
 *     `s32 fn_8027EBA8(_PLW*, u8*)`, `src/sound/fn_800EF7D8.cpp` says `u8 fn_8027EE24(void* equip)`).
 */

#include "types.h"
#include "pl.h"
#include "unsplit/Pl.h"
#include "unsplit/unknown.h"
#include "Pl/pl_master.h"
#include "Pl/pl_skill.h"


extern "C" s32 equip_kind_table_class(u8 kind);
extern "C" u8 fn_8027E290(u8 kind);
extern "C" u8 fn_8027E29C(s32 kind);

/* The 1-based kind's 0x18-byte equipment row (the row stride is the `mulli r0,r0,24` in retail). */
extern "C" void* fn_8027E2A8(u8 kind, u16 index) {
    u8** base = lbl_806AB810[0];
    if ((u8)equip_kind_table_class(kind) != 0) {
        return 0;
    }
    u32 slot = fn_8027E290(kind);
    if ((s32)index >= (s32)lbl_805706C0[slot]) {
        index = 0;
    }
    return base[slot] + index * 0x18;
}

/* The sibling 0x1C-byte row of the second table. */
extern "C" void* fn_8027E354(u8 kind, u16 index) {
    u8** base = lbl_806AB810[1];
    if ((u8)equip_kind_table_class(kind) != 0) {
        return 0;
    }
    u32 slot = fn_8027E290(kind);
    if ((s32)index >= (s32)lbl_805706C0[slot]) {
        index = 0;
    }
    return base[slot] + index * 0x1C;
}

/* The 7-based kind's 0x18-byte row of the fifth table (no kind guard, unlike the 1-based pair). */
extern "C" void* fn_8027EC50(u8 kind, u16 index) {
    u8** base = lbl_806AB810[4];
    return base[fn_8027E29C(kind)] + index * 0x18;
}

/* The sibling 0x24-byte row of the same table. */
extern "C" void* fn_8027ECBC(u8 kind, u16 index) {
    u8** base = lbl_806AB810[4];
    return base[fn_8027E29C(kind)] + index * 0x24;
}

/* In-unit callees referenced before their own definition (this TU is their owner). */
extern "C" void* fn_8027EC50(u8 kind, u16 item_id);
extern "C" void* fn_8027ECBC(u8 kind, u16 item_id);
extern "C" void* fn_8027ED6C(u8 kind, u16 item_id, u32 a, u32 b);
extern "C" void* fn_8027FB84(u8 kind, u16 item_id, u8 deco_count);
extern "C" void* fn_8027FE50(u8 kind, u16 item_id);
extern "C" void* fn_8027E2A8(u8 kind, u16 index);
extern "C" void* fn_8027E354(u8 kind, u16 index);

/* True while the player's health is at or below zero. */
extern "C" s32 fn_8027D684(_PLW* self) {
    return self->field_0x36C <= 0;
}

/* Stores the raw pair the +0x452/+0x458 group holds. */
extern "C" void fn_8027D698(_PLW* self, u8 arg1, s16 arg2) {
    self->field_0x452 = arg1;
    self->field_0x458 = arg2;
}

/* Stores the byte and raises the +0x454 limit to the argument. */
extern "C" void fn_8027D6A4(_PLW* self, u8 arg1, s16 arg2) {
    self->field_0x450 = arg1;
    if (self->field_0x454 < arg2) {
        self->field_0x454 = arg2;
    }
}

/* Stores the byte and raises the +0x456 limit to the argument. */
extern "C" void fn_8027D6C0(_PLW* self, u8 arg1, s16 arg2) {
    self->field_0x451 = arg1;
    if (self->field_0x456 < arg2) {
        self->field_0x456 = arg2;
    }
}

/* Latches the motion-request byte, then sends the motion through the +0x5C8 gate. */
extern "C" void fn_8027D6DC(_PLW* self, s16 value) {
    self->field_0x5C9 = 1;
    if (self->field_0x5C8 != 0) {
        u32 motion = fn_802745DC(self, value);
        fn_802756F0(self, 0xB, (u16)motion, 0);
    }
}

/* Reports the byte at +0x268 as a boolean.  `u32`, not `s32`: the item menu's caller compares the
 * result unsigned (`bl fn_8027D738; cmplwi r3,0x1` at 0x802A008C), which is what its declaration in
 * this unit's header `include/Pl/fn_8027D684.h` carries too. */
extern "C" u32 fn_8027D738(_PLW* self) {
    return self->field_0x268 != 0;
}

/* The +0x655 high-bit gate in front of the +0x268 boolean. */
extern "C" s32 fn_8027D74C(_PLW* self) {
    if ((self->field_0x655 & 0x80) == 0) {
        return 0;
    }
    return fn_8027D738(self);
}

/* Ends the charged action once its timer has run out. */
extern "C" void fn_8027D76C(_PLW* self) {
    if (Pl_master_ck(self) != 0 && (self->field_0x655 & 0x80) != 0) {
        self->act_end_request = 1;
        s16 timer = self->field_0x652;
        if (Pl_item_timer_get(self, self->field_0x650) >= timer) {
            pl_item_add(self, self->field_0x650, -timer);
        }
    }
}

/* The act range 0x54-0x56 of the idle kind. */
extern "C" s32 fn_8027D874(_PLW* self) {
    if (self->field_0x00A == 0 && (u32)(self->act_no - 0x54) <= 2U) {
        return 1;
    }
    return 0;
}

/* Reports the first of the three per-slot gate bytes at 0x806BB7A0 as empty. */
extern "C" s32 fn_8027DC64(void) {
    return lbl_806BB7A0[0].flag_0x00 == 0;
}

/* Reports the second gate byte as empty. */
extern "C" s32 fn_8027DC78(void) {
    return lbl_806BB7A0[1].flag_0x00 == 0;
}

/* Reports the third gate byte as empty. */
extern "C" s32 fn_8027DC90(void) {
    return lbl_806BB7A0[2].flag_0x00 == 0;
}

/* The +0x460 timer, as a boolean. */
extern "C" s32 Pl_timer_0x460_ck(_PLW* self) {
    return self->field_0x460 > 0;
}

/* Advances the 0-100 counter at +0x445 and saturates it. */
extern "C" void fn_8027E048(_PLW* self) {
    if (++self->field_0x445 >= 0x64U) {
        self->field_0x445 = 0x64U;
    }
}


/* The act-number ranges of the held (kind 6) actor. */
extern "C" s32 fn_8027DCA8(_PLW* self) {
    if (self->field_0x00A == 6) {
        return (u32)(self->act_no - 0x34) <= 2U || (u32)(self->act_no - 0x3D) <= 1U;
    }
    return 0;
}

/* The act-number gate the held-state handler asks with the part index. */
extern "C" u32 fn_8027DCE0(_PLW* self, u8 arg1) {
    if (Pl_master_ck(self) == 0) {
        return 0;
    }
    if (self->field_0x00A != 0) {
        return 0;
    }
    switch (self->act_no) {
    default:
        return 0;
    case 34:
    case 35:
        return 1;
    case 36:
        return arg1 != 2;
    case 37:
        return arg1 != 1;
    case 111:
    case 112:
    case 113:
    case 114:
        return 1;
    case 115:
    case 116:
        return arg1 != 1;
    case 117:
    case 118:
        return arg1 != 2;
    case 119:
    case 120:
        return 1;
    }
}

/* The second act-number gate of the same handler family. */
extern "C" u32 fn_8027DDC4(_PLW* self, u8 arg1) {
    if (Pl_master_ck(self) == 0) {
        return 0;
    }
    if (self->field_0x00A == 0) {
        if ((u32)(self->act_no - 0x3F) > 1U) {
            if ((u32)(self->act_no - 0x42) > 1U) {
                if (self->act_no != 0x41 && self->act_no != 0x9A) {
                    return 0;
                }
                if (arg1 == 2) {
                    return 0;
                }
                return 1;
            }
            return arg1 != 1;
        }
        return 1;
    }
    return 0;
}

/* Scans the per-kind item-id table for the first id the slot lookup accepts. */
extern "C" u16 fn_8027DE88(_PLW* self, s32 arg1) {
    u16* table;
    switch (arg1) {
    case 0:
        table = lbl_80792030;
        break;
    case 1:
        table = lbl_80792038;
        break;
    case 2:
        table = lbl_805BFFE0;
        break;
    default:
        return 0xFFFF;
    }
    for (;;) {
        u16 id = *table;
        if (id == 0xFFFF) {
            return 0xFFFF;
        }
        u16 result = fn_80273044(self, id);
        if (result != 0xFFFF) {
            return result;
        }
        table++;
    }
}

/* Scans the same id table for the equipped slot that carries the id. */
extern "C" u16 fn_8027DF38(_PLW* self, s32 arg1) {
    u16* table;
    switch (arg1) {
    case 0:
        table = lbl_80792030;
        break;
    case 1:
        table = lbl_80792038;
        break;
    case 2:
        table = lbl_805BFFE0;
        break;
    default:
        return 0xFFFF;
    }
    for (;;) {
        u16 id = *table;
        if (id == 0xFFFF) {
            return 0xFFFF;
        }
        u16 slot = self->field_0x304;
        if (id == self->slot_id[slot].item_id && self->slot_id[slot].value > 0) {
            return slot;
        }
        table++;
    }
}

/* The act-number class of the held actor, as the handler's return code. */
extern "C" s32 fn_8027DFE4(_PLW* self) {
    switch (self->field_0x00A) {
    case 0:
        switch (self->act_no) {
        case 0xAB:
        case 0xAE:
            return 1;
        case 0xA7:
            return 3;
        }
        return 0;
    case 12:
        if (self->act_no == 0xC) {
            return 2;
        }
        return 0;
    }
    return 0;
}

/* The per-part act-number gate of the held actor. */
extern "C" s32 fn_8027E06C(_PLW* self, s32 arg1) {
    if (self->field_0x00A != 6) {
        return 0;
    }
    switch (arg1) {
    case 0:
        if ((u16)(self->act_no - 0x15) <= 1U) {
            return 1;
        }
        return 0;
    case 1:
        if ((u32)(self->act_no - 0xE) <= 1U || (u32)(self->act_no - 0x24) <= 1U) {
            return 1;
        }
        return 0;
    case 2:
        if ((u32)(self->act_no - 4) <= 3U || (u32)(self->act_no - 0x1E) <= 2U ||
            (u32)(self->act_no - 0x1A) <= 1U || self->act_no == 0x26) {
            return 1;
        }
        return 0;
    }
    return 0;
}

/* ORs the act-flag bits into the byte at +0x567. */
extern "C" void fn_8027E1B8(_PLW* self, u8 flags) {
    self->atk_act_flag |= flags;
}

/* Tests the act-flag byte at +0x567 against a mask. */
s32 Pl_atk_act_flag_ck(_PLW* self, u8 mask) {
    return (self->atk_act_flag & mask) != 0;
}

/* The three-way "is this actor held" test: the +0x352 timer, the system flag or the +0x46F byte. */
extern "C" s32 fn_8027E1E4(_PLW* self) {
    if (self->field_0x352 > 0 || system_w.field_0x2a != 0 || self->field_0x46F != 0) {
        return 1;
    }
    return 0;
}

/* Reports the +0x396 timer, or the live player work through the hold test. */
extern "C" u32 fn_8027E220(_PLW* self, s32 unused) {
    if (self->field_0x396 > 0) {
        return 1;
    }
    if (Pl_master_ck(self) == 1U && fn_8027E1E4(self) == 1U) {
        return 1;
    }
    return 0;
}

/* The equipment kind the data tables are indexed by, one-based. */
extern "C" u8 fn_8027E290(u8 kind) {
    return kind - 1;
}

/* The same one-based kind, for the seven-kind table. */
extern "C" u8 fn_8027E29C(s32 kind) {
    return kind - 7;
}

/* Looks up the 0x18-byte equipment row an `_EQUIP` record names. */
extern "C" void* fn_8027E344(_EQUIP* equip) {
    return fn_8027E2A8(equip->kind, equip->item_id);
}

/* Looks up the 0x1C-byte sibling row the same record names. */
extern "C" void* fn_8027E3F4(_EQUIP* equip) {
    return fn_8027E354(equip->kind, equip->item_id);
}

/* The `_EQUIP` -> data-row lookup the table helper and the sound unit share. */
extern "C" void* fn_8027ECAC(_EQUIP* equip) {
    return fn_8027EC50(equip->kind, equip->item_id);
}

/* The `_EQUIP` -> item lookup the four Pl consumers share. */
extern "C" void* fn_8027ED18(void* equip) {
    _EQUIP* e = (_EQUIP*)equip;
    return fn_8027ECBC(e->kind, e->item_id);
}

/* The three-argument sibling of `fn_8027ED18`. */
extern "C" void* fn_8027EE08(_EQUIP* equip, u32 arg2, u32 arg3) {
    return fn_8027ED6C(equip->kind, equip->item_id, arg2, arg3);
}

/* The `_EQUIP` -> row lookup that also passes the decoration count. */
extern "C" void* fn_8027FC70(_EQUIP* equip) {
    return fn_8027FB84(equip->kind, equip->item_id, equip->deco_count);
}

/* The last `_EQUIP` -> row lookup of the family. */
extern "C" void* fn_8027FF20(_EQUIP* equip) {
    return fn_8027FE50(equip->kind, equip->item_id);
}


/* The equipment kind's row-table class: 0 for kinds 1-6, 1 for 7-11 and 14-15, 2 for 12-13, 0xFF for
 * anything else. */
extern "C" s32 equip_kind_table_class(u8 kind) {
    switch (kind) {
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 14:
    case 15:
        return 1;
    case 12:
    case 13:
        return 2;
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
        return 0;
    }
    return 0xFF;
}

/* The per-kind equipment row count the kind's own table class selects. */
extern "C" s32 fn_8027E918(u8 kind) {
    s32 count = 0;
    if ((u32)(kind - 7) > 8U) {
        if ((u32)(kind - 1) <= 5U) {
            count = lbl_805706C0[fn_8027E290(kind)];
        }
    } else {
        count = lbl_805706D8[fn_8027E29C(kind)];
    }
    return count;
}

/* The address of the eight per-kind equipment table pointers at 0x806AB810. */
u8*** get_eq_data_ptr(void) {
    return lbl_806AB810;
}
