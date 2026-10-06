/* lobby/lb_menu_page.cpp - the lobby menu page's frame step and its info-text selector.
 * RANGE. .text 0x80365C84-0x80366618 (2 functions); .data 0x805EDAB4-0x805EDAE0 (`jumptable_805EDAB4`, the frame step's
 *   0..10 switch table, an island inside another TU's `.data` run), extab, extabindex.  One TU: `lb_menu_page_step` is
 *   `lb_menu_info_update`'s only caller.  `ef/eft053.cpp` starts at the right edge.
 * FLAGS. `cflags_lobby` (its `-Cpp_exceptions on` emits the two unwind records) and file-scope `#pragma peephole off`
 *   (retail keeps the unfused `rlwinm`/`clrlwi` + `cmpwi` pairs; measured in docs/lobby.md).
 * NAMES. The unit, `lb_menu_page_step` and `lb_menu_info_update` are GUESSes from the two bodies in the module's `lb_*`
 *   scheme.  Module `lobby`: the frame step reads `lobby_w` +0x0AC (the menu pointer `lobby/fn_801E7530.cpp` uses), the
 *   selector `lobby_world_block`, and every callee is a lobby/hud or Pl icon symbol.
 * RESIDUALS. `lb_menu_info_update`: case 10 / mode 1 loads the icon table base into a fresh r4 one instruction early
 *   and forms `(base + index * 12) + 0xE00`, retail `(base + 0xE00) + index * 12` in r3 (19 of 2452 `.text` bytes).
 *   Retail's own mode-0 twin at +0x27C uses our form, so the association is the allocator's; the `&entries[i]`,
 *   `entries + i`, index-local and `table`-local spellings and a scoped `#pragma scheduling off` do not move it.
 * SHAPES. The `rec`-first declaration in case 10 / mode 1 puts `flags` in retail's r26.
 */

#include "types.h"

/* The types this unit's own bodies define (rules 1/3/4/5): the shared views of `lobby_w` and
 * `lobby_world_block` in `lobby/*.h` stop short of the offsets below, so each is this unit's
 * view with its own name.  `LbIconRec`/`LbWorldBlock`/`lobby_world_block`/`equip_record_copy` come from the
 * owner's header (rule 2). */
#include "lobby/lb_pane_ui.h"

/* Retail keeps the unfused `rlwinm`/`clrlwi` + `cmpwi` pairs the peephole pass folds into record forms. */
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
    /* +0x200 */ s32 data_0x200;        /* `lb_panel_yes_no_draw`'s second argument for the 6265 panel */
    /* +0x204 */ u8 unused_0x204[0x8];
    /* +0x20C */ LbIconRec icons_0x20C[4]; /* 12-byte icon records `equip_record_copy` copies */
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

/* Foreign callees whose owners' headers do not declare them, as plain prototypes (rule 2 debt). */
extern "C" {
void fn_801E66A8(s32 a, s32 b);                  /* 0x801E66A8 - resets the band's work group */
void fn_801E677C(void* work, u8 kind, u8 flag);  /* 0x801E677C - initialises one sub-work record */
void fn_801E68B4(void* work, u8 flag);           /* 0x801E68B4 - ditto, one argument */
s16 fn_801EF73C(void* work);                     /* 0x801EF73C - the icon index of the sub-work (s16) */
s32 fn_801F0834(void* work);                     /* 0x801F0834 - non-zero while the sub-work is live */
s32 fn_802142D8(LbIconRec* icon, s32 flag, void* table_a, void* table_b);
s32 lb_panel_msg_draw(s32 page, s16 id);               /* the 0x1877 panel's primary text */
s32 fn_80214FB8(s32 page, s32 id, void* data);   /* the same panel's row text plus its data */
s32 lb_panel_line_draw(s32 page, s16 id, s32 a, s32 b); /* its secondary text */
s32 lb_panel_yes_no_draw(s32 page, s32 data);             /* its value */
s32 fn_802179D4(LbIconRec* icon);                /* non-zero while the record is already held */
s32 fn_80217F4C(LbIconRec* icon, u8 kind, u16 value); /* fills a record from a kind and a value */
s32 fn_8021A5FC(void);                           /* 0x8021A5FC - the banner/message step */
s32 equip_kind_table_class(u8 kind);                        /* the equipment kind's row-table class (Pl) */
s32 fn_8027F1B8(LbIconRec* icon);                /* the record's stack count (Pl) */
s32 fn_8027F21C(LbIconRec* icon);                /* the record's "held" test (Pl) */
s32 menu_money_draw(s32 id);                         /* 0x802DF6E4 - the HUD/2D element release */
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
            lb_panel_yes_no_draw(6265, self->data_0x200);
            break;
        case 3:
            msg_a = 77;
            lb_panel_yes_no_draw(6265, self->data_0x200);
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
            lb_panel_yes_no_draw(6265, self->data_0x200);
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
            equip_record_copy(&icon, &lobby_world_block->entries_0x0E00[fn_801EF73C(&self->work_0x3A4)]);
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
            lb_panel_yes_no_draw(6265, self->data_0x200);
            break;
        case 3:
            msg_a = 77;
            equip_record_copy(&icon, &self->icons_0x20C[self->index_0x128]);
            if ((u8)equip_kind_table_class(icon.kind_0x00) == 1 && fn_802179D4(&icon) == 0) {
                msg_b = 78;
            }
            lb_panel_yes_no_draw(6265, self->data_0x200);
            break;
        case 4:
            msg_a = 79;
            lb_panel_yes_no_draw(6265, self->data_0x200);
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
            lb_panel_yes_no_draw(6265, self->data_0x200);
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
            equip_record_copy(&icon, &lobby_world_block->entries_0x0E00[fn_801EF73C(&self->work_0x3A4)]);
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
            lb_panel_yes_no_draw(6265, self->data_0x200);
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
            equip_record_copy(&icon, &lobby_world_block->entries_0x0E00[fn_801EF73C(&self->work_0x3A4)]);
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
            lb_panel_yes_no_draw(6265, self->data_0x200);
            break;
        case 3:
            page = 6264;
            msg_a = 118;
            break;
        }
        break;
    }

    if (msg_a != 0xFFFF) {
        lb_panel_msg_draw(page, (s16)msg_a);
    }
    if (msg_b != 0xFFFF) {
        lb_panel_line_draw(page, (s16)msg_b, arg_c, arg_d);
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
                lb_panel_msg_draw(6263, 90);
            } else {
                lb_panel_msg_draw(6263, 89);
            }
            break;
        case 1:
            lb_panel_msg_draw(6263, 91);
            break;
        }
        break;
    case 5:
        if (self->mode_0x01 != 2 && self->mode_0x01 != 3) {
            fn_8021A5FC();
            menu_money_draw(6360);
        }
        fn_803642B8(self);
        break;
    case 10:
        fn_8021A5FC();
        menu_money_draw(6360);
        fn_803659E8(self);
        break;
    case 6:
        fn_8021A5FC();
        menu_money_draw(6360);
        fn_80364BD8(self);
        break;
    case 7:
        fn_8021A5FC();
        menu_money_draw(6360);
        fn_80364EE8(self);
        break;
    case 8:
        fn_8021A5FC();
        menu_money_draw(6360);
        fn_803653A0(self);
        break;
    case 9:
        fn_8033C1AC();
        break;
    }

    lb_menu_info_update(self);
}
