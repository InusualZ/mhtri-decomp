/* lobby/lb_menu_page.cpp - the lobby menu page's per-frame handler and its info/message selector.
 *
 * `.text` 0x80365C84..0x80366618, two functions (0x788 and 0x20C bytes) plus their extab records
 * 0x800177C4..0x800177D4 and extabindex entries 0x8003720C..0x80037224 - each run is exactly the gap
 * between the split objects that bracket this unit (`auto_fn_803659E8_text`'s record ends at
 * 0x800177C4/0x8003720C, `auto_fn_80366618_text`'s begins at 0x800177D4/0x80037224), and the two
 * functions are one TU: `lb_menu_page_step` calls `lb_menu_info_update` and nothing else in the DOL does.
 *
 * WHAT IT IS.  `lb_menu_page_step` is the page's frame step: it resets the 2D state
 * (`set_zmode(0,0,0)`, `set_blendmode(4,5,1)`), switches on the page's `state_0x00` 1..10 and runs
 * one per-page draw/update arm (`fn_803642B8`/`fn_803645C4`/`fn_80364BD8`/`fn_80364EE8`/
 * `fn_803653A0`/`fn_803659E8`/`fn_8033C1AC`, the neighbouring functions of this band), then calls
 * `lb_menu_info_update`.  That one turns the page's state into the two text ids the info panel shows:
 * it picks a page id (6263/6264/6265 = the map's 0x1877/0x1878/0x1879) and a primary/secondary id,
 * and pushes them with `fn_80214EF0(page, id)` and `fn_802150DC(page, id, a, b)`; the arms that need
 * an icon record copy one out of the shared lobby block (`lobby_world_block + 0xE00`, stride 0xC) or use
 * the page's own record tables (+0x264 stride 0x18, +0x328 stride 0x10, +0x20C icons, +0x23C words,
 * +0x24C u16s).
 *
 * MODULE AND NAME (brief section 2, evidence order).  1. No `__FILE__` string: a relocation sweep of
 * both split objects finds no `.c`/`.cpp` literal at all - the range's only data references are the
 * `.sbss` pointer `lobby_world_block`, the `.data` jump table below and calls.  (The `menu_note.cpp`
 * string the proposal's discovery note cites at 0x805E91F8 is NOT this unit's: it is referenced by
 * `src/menu/menu_note.cpp`'s object and by nothing else, and neither object here names it.)
 * 2. `dumpmap.py lookup 0x80365C84` / `0x8036640C` answer `zz_0365c84_` / `zz_036640c_` placeholders,
 * and `symedit.py range 0x80365C84 0x80366618` showed two bare stems before this lane named them.
 * 3. Module `lobby` and the unit's name come from the code: the frame step reads the lobby work block
 * `lobby_w` (.bss 0x806AAB44) at +0x0AC - the same menu pointer `lobby/fn_801E7530.cpp` uses - and
 * the selector reads the lobby page block `lobby_world_block` (.sbss 0x80794880, the 4-byte pointer); all
 * 20 callee sites are lobby/hud symbols (`set_zmode`/`set_blendmode`, the 0x1877/0x1878/0x1879 panel
 * family `fn_80214EF0`/`fn_80214FB8`/`fn_802150DC`/`fn_80215170`, `fn_801E66A8`/`fn_801E677C`/
 * `fn_801E68B4`, `fn_801EF73C`/`fn_801F0834`, `fn_802142D8`/`fn_802179D4`/`fn_80217F4C`,
 * `fn_8033C1AC`) plus the Pl/HUD icon queries `equip_kind_table_class`/`fn_8027F1B8`/`fn_8027F21C` and the
 * 12-byte icon copy `fn_8004A20C`.
 *
 * NAMES: GUESS, derived from the two bodies (marked per the brief, for a later naming pass).  The pair
 * is the lobby menu page's frame step and its info-text selector, so the unit is `lb_menu_page.cpp`
 * and its symbols are `lb_menu_page_step` / `lb_menu_info_update` - the module's own `lb_*` scheme
 * (`lb_companion_ui`, `lb_npc`, `lb_act_dispatch`) with a verb phrase per body.  The map rows at
 * 0x80365C84 / 0x8036640C were renamed with the source, one edit each, through
 * `symedit.py rename`; nothing else referenced either (its own check plus a `grep` over
 * `build/RMHE08/obj/`: the only other citations are the jump table below and a call from the
 * unregistered `fn_80363A5C`, which has no source yet).
 *
 * Naming note: the `fn_XXXXXXXX` names this file still carries are OTHER units' symbols - the
 * lobby/hud/Pl callees in the linkage block below and the six 0x8036xxxx siblings - whose map rows
 * this lane does not own.  The two symbols this unit defines are named.
 *
 * LANGUAGE AND FLAGS.  C++: the range reaches genuinely mangled callees (`set_zmode__FbUcb`,
 * `set_blendmode__FUcUcUc`) through their real signatures (rule 9), and every plain `fn_` definition
 * is `extern "C"` so it keeps the map's name (playbook 42).  Lib `lobby` (`cflags_lobby`: -O3,
 * -inline noauto), whose `-Cpp_exceptions on` (flags-audit 2026-09-28) emits the unwind records: the
 * target objects carry one extab record each (8 B) and one extabindex entry each (12 B).
 *
 * DATA.  `.data` 0x805EDAB4..0x805EDAE0 (0x2C B) is this unit's own jump table: MWCC emits it for
 * `lb_menu_page_step`'s dense 0..10 switch, the map records it as `jumptable_805EDAB4` (`scope:local`,
 * 11 entries x 4 B), and it is the only data either body emits.  It is a slice of the unclaimed
 * `.data` run 0x805E9248.., so the claim is one island inside an `auto_*_data` chunk - the bytes on
 * both sides belong to other TUs (the neighbouring tables resolve to `fn_80360A58`/`fn_80363A5C`/
 * `fn_803667EC` jump tables).  No `.sdata2` is emitted or claimed.
 *
 * SEAM: UNPROVEN at the trailing edge (brief section 8.3).  The leading edge is solid (the previous
 * split object is a different band and nothing here is cited from it); the trailing edge at
 * 0x80366618 is attribute.py's `--max-bytes` cut - the next functions (0x80366618, 0x803667EC) draw
 * with `cpSetRotMatrixZXY`/`mulVecMatAddTrans` and lead into `eft053` at 0x803669A0, and no evidence
 * class settles where this unit ends.
 *
 * FLAGS, MEASURED.  The lib's `-Cpp_exceptions on` (flags-audit 2026-09-28) is required: without it
 * the object emits no
 * extab/extabindex at all (`datagap.py --mode both` reports `extab 16B (ours 0B)`,
 * `extabindex 24B (ours 0B)`, `matched_data` 44 of 84).  `#pragma peephole off` is required too:
 * retail keeps the unfused `rlwinm` + `cmpwi` and `clrlwi` + `cmpwi` pairs this unit is full of,
 * and the pass folds them into `rlwinm.`/`clrlwi.` (A/B over the unit: 96.484505 -> 99.55954).
 *
 * STATUS AND RESIDUAL.  `lb_menu_page_step` is byte-identical (524 B, 100.00 %).  `lb_menu_info_update`
 * measures
 * 99.439835 %: with the peephole pass off the two functions' instruction streams have the same
 * length (482 instructions each), and the whole residual is one register/association choice in
 * case 10 / mode 1.  Retail loads the icon record's base pointer into `r3` - the register the
 * argument pointer `self` has just died in - and forms the record as `(base + 0xE00) + index * 12`;
 * ours loads the same `.sbss` pointer into a fresh `r4` one instruction earlier and forms
 * `(base + index * 12) + 0xE00`:
 *
 *   retail                                          ours
 *   330 lha   r0,962(r3)    ; self->icon_index_0x3C2 330 lwz  r4,0(0)
 *   334 mulli r0,r0,12                               334 lha  r0,962(r3)
 *   338 lwz   r3,0(0)       ; lobby_world_block          338 mulli r0,r0,12
 *   33c addi  r3,r3,3584                             33c add  r3,r4,r0
 *   340 add   r25,r3,r0                              340 addi r25,r3,3584
 *   344 lbz   r3,0(r25)                              344 lbz  r3,0(r25)
 *
 * Both forms occur in retail itself (the case 10 / mode 0 twin at 0x27c uses ours), so the
 * association is the allocator's, not the source's, and no source spelling tried moved it: the
 * plain `&entries[i]` and `entries + i` forms, an index local, a `rec`-first declaration (which
 * *did* fix the other half of the pair - `flags` moved from r25 to retail's r26 and the unit went
 * 99.49429 -> 99.55954), a `table` local, and `#pragma scheduling off` scoped over the function
 * (97.22023, rejected).  Nine measured variants in all (seven source shapes for this pair plus the two flag A/Bs); this is the
 * best of them.
 *
 * UNIT SCORE.  fuzzy 99.55954 %, `.text` 2452 B with 524 B byte-identical, and `.data`/`extab`/
 * `extabindex` 84 of 84 B - `datagap.py --unit lobby/lb_menu_page --mode both` reports no gap in
 * either direction, and the unit is `NonMatching` (the 0.44 % residual above is what stands
 * between it and a flip).
 */

#include "types.h"

/* The types this unit's own bodies define (rules 1/3/4/5): the shared views of `lobby_w` and
 * `lobby_world_block` in `include/lobby/*.h` stop short of the offsets below, so each is this unit's
 * view with its own name.  `LbIconRec`/`LbWorldBlock`/`lobby_world_block`/`fn_8004A20C` come from the
 * owner's header (rule 2). */
#include "lobby/fn_801F3294.h"

/* The target objects carry one 8-byte extab record and one 12-byte extabindex entry per function
 * (both runs are claimed in splits.txt); `cflags_lobby`'s `-Cpp_exceptions off` would emit none. */
#pragma peephole off

typedef struct LbMenuPage LbMenuPage;

/* The lobby work block (`lobby_w`, .bss 0x806AAB44, 0x17C B) as this range reads it: only the menu
 * pointer. size: 0xB0 (the extent this unit reads) */
typedef struct LbMenuOwnerWork {
    /* +0x000 */ u8 unused_0x000[0xAC];
    /* +0x0AC */ LbMenuPage* menu_0xAC; /* the lobby menu page object `lb_menu_page_step` drives */
} LbMenuOwnerWork; /* size: 0xB0 (the extent this unit reads) */

/* One 0x20-byte sub-work record of the page (`fn_801E66A8`/`fn_801E677C`/`fn_801E68B4` initialise the
 * three at +0x0A0/+0x0C0/+0x0E0); only the first one's kind word is read here. size: 0x20 */
typedef struct LbPageSubWork {
    /* +0x00 */ s16 kind_0x00;
    /* +0x02 */ u8 unused_0x02[0x1E];
} LbPageSubWork; /* size: 0x20 */

/* One 0x18-byte record of the page's record table at +0x264. size: 0x18 */
typedef struct LbPageRecord {
    /* +0x00 */ u8 kind_0x00;      /* the row kind `fn_80217F4C` is handed (12/13 mean "use kind 11") */
    /* +0x01 */ u8 unused_0x01;
    /* +0x02 */ u16 flags_0x02;    /* bits 1/2/3 select the text ids (0x2/0x4/0x8) */
    /* +0x04 */ u16 value_0x04;    /* `fn_80217F4C`'s value for the row */
    /* +0x06 */ u8 unused_0x06[0x2];
    /* +0x08 */ void* data_0x08;   /* `fn_80214FB8`'s third argument */
    /* +0x0C */ u8 unused_0x0C[0xC];
} LbPageRecord; /* size: 0x18 */

/* One 0x10-byte entry of the page's entry table at +0x328. size: 0x10 */
typedef struct LbPageEntry {
    /* +0x00 */ void* data_0x00;   /* `fn_80214FB8`'s third argument */
    /* +0x04 */ u8 unused_0x04[0x6];
    /* +0x0A */ u16 flags_0x0A;    /* bits 1/2/5 select the text ids (0x2/0x4/0x20) */
    /* +0x0C */ u8 unused_0x0C[0x4];
} LbPageEntry; /* size: 0x10 */

/* The lobby menu page object (`lobby_w.menu_0xAC`): the state machine both functions drive, plus the
 * record/entry tables and the three sub-works its arms read.  Every offset below is one the
 * disassembly reads; the gaps carry theirs.  The counts are the observed bounds (the arrays are
 * indexed by s16 page fields, so they are the extent the tables occupy, not proven bounds).
 * size: 0x3C4+ */
typedef struct LbMenuPage {
    /* +0x000 */ u8 state_0x00;         /* the page state both dispatchers switch on (1..10) */
    /* +0x001 */ u8 mode_0x01;          /* the per-state sub-mode */
    /* +0x002 */ u8 unused_0x002[0x3];
    /* +0x005 */ u8 open_0x05;          /* non-zero while the page is open */
    /* +0x006 */ u8 unused_0x006[0x6];
    /* +0x00C */ s16 list_mode_0x0C;    /* the row/list index into `records_0x264` and `entries_0x328` */
    /* +0x00E */ u8 unused_0x00E[0x8];
    /* +0x016 */ u16 record_index_0x16; /* the row index `fn_80214FB8`'s argument comes from */
    /* +0x018 */ s16 has_entries_0x18;  /* non-zero while the entry table has a live row (guess) */
    /* +0x01A */ u8 unused_0x01A[0x2];
    /* +0x01C */ s16 sub_mode_0x1C;     /* picks between the two text ids of the 0x78/0x79 pair */
    /* +0x01E */ u8 unused_0x01E[0x80];
    /* +0x09E */ u8 kind_0x9E;          /* must be 1 or 2 for the 0x78/0x79 pair to be shown */
    /* +0x09F */ u8 unused_0x09F;
    /* +0x0A0 */ LbPageSubWork work_0xA0;
    /* +0x0C0 */ LbPageSubWork work_0xC0;
    /* +0x0E0 */ LbPageSubWork work_0xE0;
    /* +0x100 */ u8 unused_0x100[0x28];
    /* +0x128 */ s16 index_0x128;       /* the icon/word/u16 index of the three tables below */
    /* +0x12A */ u8 unused_0x12A[0xD6];
    /* +0x200 */ s32 data_0x200;        /* `fn_80215170`'s second argument for the 6265 panel */
    /* +0x204 */ u8 unused_0x204[0x8];
    /* +0x20C */ LbIconRec icons_0x20C[4]; /* 12-byte icon records `fn_8004A20C` copies */
    /* +0x23C */ u32 values_0x23C[4];      /* the words `fn_80214FB8` is handed */
    /* +0x24C */ u16 flags_0x24C[12];      /* the per-index flag run both text selectors read */
    /* +0x264 */ LbPageRecord records_0x264[8];
    /* +0x324 */ u8 unused_0x324[0x4];
    /* +0x328 */ LbPageEntry entries_0x328[7];
    /* +0x398 */ u8 unused_0x398[0xC];
    /* +0x3A4 */ u8 work_0x3A4[0x1E];  /* the sub-work `fn_801F0834`/`fn_801EF73C` run on */
    /* +0x3C2 */ s16 icon_index_0x3C2; /* the shared icon table's index (guess) */
    /* +0x3C4 */ u8 tail_0x3C4[];
} LbMenuPage; /* size: 0x3C4+ */

/* Foreign callees whose owners do not publish them: bare prototypes inside the linkage block, the
 * convention the neighbouring lobby units use (each is filed as a shared-file request, rule 2). */
extern "C" {
void fn_801E66A8(s32 a, s32 b);                  /* 0x801E66A8 - resets the band's work group */
void fn_801E677C(void* work, u8 kind, u8 flag);  /* 0x801E677C - initialises one sub-work record */
void fn_801E68B4(void* work, u8 flag);           /* 0x801E68B4 - ditto, one argument */
s16 fn_801EF73C(void* work);                     /* 0x801EF73C - the icon index of the sub-work (s16) */
s32 fn_801F0834(void* work);                     /* 0x801F0834 - non-zero while the sub-work is live */
s32 fn_802142D8(LbIconRec* icon, s32 flag, void* table_a, void* table_b);
s32 fn_80214EF0(s32 page, s16 id);               /* the 0x1877 panel's primary text */
s32 fn_80214FB8(s32 page, s32 id, void* data);   /* the same panel's row text plus its data */
s32 fn_802150DC(s32 page, s16 id, s32 a, s32 b); /* its secondary text */
s32 fn_80215170(s32 page, s32 data);             /* its value */
s32 fn_802179D4(LbIconRec* icon);                /* non-zero while the record is already held */
s32 fn_80217F4C(LbIconRec* icon, u8 kind, u16 value); /* fills a record from a kind and a value */
s32 fn_8021A5FC(void);                           /* 0x8021A5FC - the banner/message step */
s32 equip_kind_table_class(u8 kind);                        /* the equipment kind's row-table class (Pl) */
s32 fn_8027F1B8(LbIconRec* icon);                /* the record's stack count (Pl) */
s32 fn_8027F21C(LbIconRec* icon);                /* the record's "held" test (Pl) */
s32 fn_802DF6E4(s32 id);                         /* 0x802DF6E4 - the HUD/2D element release */
s32 fn_8033C1AC(void);                           /* 0x8033C1AC - the companion-page step */
void fn_803642B8(LbMenuPage* self);              /* 0x803642B8 - the band's per-state draw arms */
void fn_803645C4(LbMenuPage* self);
void fn_80364BD8(LbMenuPage* self);
void fn_80364EE8(LbMenuPage* self);
void fn_803653A0(LbMenuPage* self);
void fn_803659E8(LbMenuPage* self);
}

/* `set_zmode__FbUcb`'s owner (`sound/fn_800E3CBC.cpp`) publishes no header; the mangling is
 * reproduced by this real signature (rule 9).  `set_blendmode` is declared by the owner's header
 * included above. */
void set_zmode(bool first, u8 mode, bool second);

/* Sets the page's two text ids from its state and sub-mode and pushes them to the info panel. */
extern "C" void lb_menu_info_update(LbMenuPage* self) {
    LbIconRec icon;
    u16 msg_a = 0xFFFF;
    u16 msg_b = 0xFFFF;
    s32 page = 6263;
    s32 arg_c = 2;
    s32 arg_d = 1;
    s32 icon_flag = 0;

    switch (self->state_0x00) {
    case 5:
        switch (self->mode_0x01) {
        case 0:
            if (self->open_0x05 != 0) {
                msg_a = 84;
            } else {
                msg_a = 92;
                {
                u16 flags = self->records_0x264[self->list_mode_0x0C].flags_0x02;

                if ((flags & 2) != 0) {
                    msg_b = 81;
                } else if ((flags & 8) != 0) {
                    if ((flags & 4) != 0) {
                        msg_b = 93;
                    } else {
                        msg_b = 94;
                    }
                } else if ((flags & 4) != 0) {
                    msg_b = 82;
                    arg_d = 2;
                }
                }
            }
            break;
        case 1:
            fn_80214FB8(6263, 95, self->records_0x264[self->record_index_0x16].data_0x08);
            fn_80215170(6265, self->data_0x200);
            break;
        case 3:
            msg_a = 77;
            fn_80215170(6265, self->data_0x200);
            {
                LbPageRecord* rec = &self->records_0x264[self->list_mode_0x0C];

                if ((u32)(rec->kind_0x00 - 12) <= 1) {
                    fn_80217F4C(&icon, 11, 1);
                } else {
                    fn_80217F4C(&icon, rec->kind_0x00, rec->value_0x04);
                }
            }
            if ((u8)equip_kind_table_class(icon.kind_0x00) == 1 && fn_802179D4(&icon) == 0) {
                msg_b = 78;
            }
            break;
        case 4:
            msg_a = 79;
            fn_80215170(6265, self->data_0x200);
            break;
        case 7:
            msg_a = 80;
            page = 6264;
            break;
        case 5:
            page = 6263;
            if ((u32)(self->kind_0x9E - 1) <= 1) {
                if (self->sub_mode_0x1C == 0) {
                    msg_a = 120;
                } else {
                    msg_a = 121;
                }
            }
            break;
        }
        break;
    case 10:
        switch (self->mode_0x01) {
        case 0:
            if (fn_801F0834(&self->work_0x3A4) != 0) {
                break;
            }
            page = 6264;
            if (self->work_0xA0.kind_0x00 == 0) {
                msg_a = 96;
                icon_flag = 0;
            } else {
                msg_a = 97;
                icon_flag = 1;
            }
            fn_8004A20C(&icon, &lobby_world_block->entries_0x0E00[fn_801EF73C(&self->work_0x3A4)]);
            if (icon.kind_0x00 == 0) {
                break;
            }
            if ((u16)fn_802142D8(&icon, icon_flag, self->icons_0x20C, self->values_0x23C) == 0) {
                break;
            }
            msg_b = 98;
            arg_d = 2;
            break;
        case 1:
            if (self->open_0x05 != 0) {
                msg_a = 84;
            } else {
                msg_a = 99;
                {
                LbIconRec* rec;
                u16 flags = self->flags_0x24C[self->index_0x128];

                if ((flags & 8) != 0) {
                    if ((flags & 4) != 0) {
                        msg_b = 100;
                    } else {
                        msg_b = 101;
                    }
                } else if ((flags & 4) != 0) {
                    msg_b = 82;
                    arg_d = 2;
                } else {
                    rec = &lobby_world_block->entries_0x0E00[self->icon_index_0x3C2];

                    if ((u8)equip_kind_table_class(rec->kind_0x00) == 1 && rec->kind_0x00 != 11) {
                        if ((flags & 0x100) != 0) {
                            msg_b = 123;
                        } else if (fn_8027F1B8(rec) > 0) {
                            msg_b = 122;
                            arg_c = 5;
                        }
                    }
                }
                }
            }
            break;
        case 2:
            fn_80214FB8(6263, 102, (void*)self->values_0x23C[self->index_0x128]);
            fn_80215170(6265, self->data_0x200);
            break;
        case 3:
            msg_a = 77;
            fn_8004A20C(&icon, &self->icons_0x20C[self->index_0x128]);
            if ((u8)equip_kind_table_class(icon.kind_0x00) == 1 && fn_802179D4(&icon) == 0) {
                msg_b = 78;
            }
            fn_80215170(6265, self->data_0x200);
            break;
        case 4:
            msg_a = 79;
            fn_80215170(6265, self->data_0x200);
            break;
        }
        break;
    case 6:
        switch (self->mode_0x01) {
        case 0:
            msg_a = 103;
            {
                u16 flags = self->records_0x264[self->list_mode_0x0C].flags_0x02;

                if ((flags & 2) != 0) {
                    msg_b = 48;
                    arg_d = 2;
                } else if ((flags & 8) != 0) {
                    if ((flags & 4) != 0) {
                        msg_b = 93;
                    } else {
                        msg_b = 94;
                    }
                } else if ((flags & 4) != 0) {
                    msg_b = 82;
                    arg_d = 2;
                }
            }
            break;
        case 1:
            fn_80214FB8(6263, 95, self->records_0x264[self->record_index_0x16].data_0x08);
            fn_80215170(6265, self->data_0x200);
            return;
        case 5:
            page = 6263;
            if ((u32)(self->kind_0x9E - 1) <= 1) {
                if (self->sub_mode_0x1C == 0) {
                    msg_a = 120;
                } else {
                    msg_a = 121;
                }
            }
            break;
        }
        break;
    case 7:
        switch (self->mode_0x01) {
        case 0:
            if (fn_801F0834(&self->work_0x3A4) != 0) {
                break;
            }
            page = 6264;
            msg_a = 104;
            fn_8004A20C(&icon, &lobby_world_block->entries_0x0E00[fn_801EF73C(&self->work_0x3A4)]);
            if (icon.kind_0x00 == 0) {
                break;
            }
            if (fn_8027F21C(&icon) != 0) {
                break;
            }
            msg_b = 105;
            arg_d = 2;
            break;
        case 1:
            if (self->open_0x05 != 0) {
                msg_a = 84;
                break;
            }
            msg_a = 106;
            if (self->has_entries_0x18 == 0) {
                break;
            }
            {
                u16 flags = self->entries_0x328[self->list_mode_0x0C].flags_0x0A;

                if ((flags & 0x20) != 0) {
                    msg_b = 107;
                } else if ((flags & 4) != 0) {
                    msg_b = 82;
                    arg_d = 2;
                }
            }
            break;
        case 2:
            fn_80214FB8(6263, 108, self->entries_0x328[self->list_mode_0x0C].data_0x00);
            fn_80215170(6265, self->data_0x200);
            break;
        }
        break;
    case 8:
        switch (self->mode_0x01) {
        case 0:
            if (fn_801F0834(&self->work_0x3A4) != 0) {
                break;
            }
            page = 6264;
            msg_a = 111;
            fn_8004A20C(&icon, &lobby_world_block->entries_0x0E00[fn_801EF73C(&self->work_0x3A4)]);
            if (icon.kind_0x00 == 0) {
                break;
            }
            if (fn_8027F1B8(&icon) != 0) {
                break;
            }
            msg_b = 112;
            arg_d = 2;
            break;
        case 1:
            if (self->open_0x05 != 0) {
                msg_a = 84;
                break;
            }
            msg_a = 113;
            {
                u16 flags = self->entries_0x328[self->list_mode_0x0C].flags_0x0A;

                if ((flags & 2) != 0) {
                    msg_b = 114;
                    arg_d = 2;
                } else if ((flags & 4) != 0) {
                    msg_b = 82;
                    arg_d = 2;
                }
            }
            break;
        case 2:
            fn_80214FB8(6263, 115, self->entries_0x328[self->list_mode_0x0C].data_0x00);
            fn_80215170(6265, self->data_0x200);
            break;
        case 3:
            page = 6264;
            msg_a = 118;
            break;
        }
        break;
    }

    if (msg_a != 0xFFFF) {
        fn_80214EF0(page, (s16)msg_a);
    }
    if (msg_b != 0xFFFF) {
        fn_802150DC(page, (s16)msg_b, arg_c, arg_d);
    }
}

/* The page's frame step: reset the 2D state, run the state's own arm, then refresh the info text. */
extern "C" void lb_menu_page_step(void) {
    LbMenuPage* self = ((LbMenuOwnerWork*)&lobby_w)->menu_0xAC;

    set_zmode(0, 0, 0);
    set_blendmode(4, 5, 1);

    switch (self->state_0x00) {
    case 1:
        fn_801E66A8(1, 1);
        fn_801E68B4(&self->work_0xA0, 0);
        break;
    case 2:
        fn_801E66A8(1, 1);
        if (self->work_0xA0.kind_0x00 == 2) {
            fn_801E677C(&self->work_0xC0, 4, 0);
        } else {
            fn_801E677C(&self->work_0xC0, (u8)(self->work_0xA0.kind_0x00 + 2), 0);
        }
        break;
    case 3:
    case 4:
        fn_801E66A8(1, 1);
        if (self->state_0x00 == 4) {
            fn_801E677C(&self->work_0xE0, (u8)self->work_0xA0.kind_0x00, 1);
            fn_803645C4(self);
        } else {
            fn_801E677C(&self->work_0xE0, (u8)self->work_0xA0.kind_0x00, 0);
        }
        switch (self->work_0xA0.kind_0x00) {
        case 0:
            if (self->state_0x00 == 4) {
                fn_80214EF0(6263, 90);
            } else {
                fn_80214EF0(6263, 89);
            }
            break;
        case 1:
            fn_80214EF0(6263, 91);
            break;
        }
        break;
    case 5:
        if (self->mode_0x01 != 2 && self->mode_0x01 != 3) {
            fn_8021A5FC();
            fn_802DF6E4(6360);
        }
        fn_803642B8(self);
        break;
    case 10:
        fn_8021A5FC();
        fn_802DF6E4(6360);
        fn_803659E8(self);
        break;
    case 6:
        fn_8021A5FC();
        fn_802DF6E4(6360);
        fn_80364BD8(self);
        break;
    case 7:
        fn_8021A5FC();
        fn_802DF6E4(6360);
        fn_80364EE8(self);
        break;
    case 8:
        fn_8021A5FC();
        fn_802DF6E4(6360);
        fn_803653A0(self);
        break;
    case 9:
        fn_8033C1AC();
        break;
    }

    lb_menu_info_update(self);
}
