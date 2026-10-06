/* `LbLobbyWork`, the record of `lobby_w` (.bss 0x806AAB44, 0x17C B; declared in `lobby/lobby_w.h`), merged from every
 * lobby unit's view (the NPC, page, equipment/menu and sound units'); a byte two views spell differently is a union of
 * both spellings. */
#ifndef MHTRI_LOBBY_LOBBY_WORK_H
#define MHTRI_LOBBY_LOBBY_WORK_H

#include "types.h"

struct LbMenuWork;
struct LbMenuPanel;
struct _se_w;

typedef struct LbLobbyWork {
    /* +0x000 */ union {
        u8 state_0x000;   /* the lobby scene/mode `fn_80211E68` dispatches on (0..0x25) */
        u8 field_0x000;
    };
    /* +0x001 */ u8 field_0x001;
    /* +0x002 */ union {
        u8 field_0x002;   /* the menu-layer state `src/lobby/lb_pane_ui.cpp` sets (0/2) and passes to
                           * `fn_801FB2A8`/`fn_802AEEF8` */
        u8 area_0x002;    /* the scene's area id, compared against `_PLW::area_0x16`
                           * (`src/lobby/lb_npc.cpp`) - the same byte, two consumers */
        s8 field_0x002_s; /* the signed reading the NPC band's act tests use */
    };
    /* +0x003 */ union {
        u8 unused_0x003[3];
        u8 field_0x003;   /* the NPC band resets it to 0xFF */
    };
    /* +0x006 */ u8 field_0x006;
    /* +0x007 */ u8 unused_0x007;
    /* +0x008 */ u8 active_0x008;   /* 1 while a lobby screen (kitchen, trade, board) is open */
    /* +0x009 */ u8 community_flag_0x009;   /* cleared when community command 12 succeeds (GUESS name) */
    /* +0x00A */ u8 unused_0x00A[0x02];
    /* +0x00C */ u32 slots_0x00C[2];
    /* +0x014 */ u8 field_0x014;
    /* +0x015 */ u8 unused_0x015[0x12];
    /* +0x027 */ union {
        u8 flag_0x027;
        u8 field_0x027;
    };
    /* +0x028 */ union {   /* 0x028-0x04F, a multiple of 4 so the s32 inside does not pad the union */
        u8 unused_0x028[0x28];
        struct {
            /* +0x028 */ u8 countdown_0x028;   /* the byte counter `fn_8021261C` decays while it is nonzero */
            /* +0x029 */ u8 pad_0x029[7];
            /* +0x030 */ s32 counter_0x030;    /* incremented once per `fn_8021261C` */
            /* +0x034 */ s16 slide_0x034;      /* the cursor slide `fn_8021261C` ramps inside +-0x3C */
            /* +0x036 */ s16 field_0x036[13];
        };
    };
    /* +0x050 */ u8 unused_0x050[0x2];
    /* +0x052 */ u8 page_0x052;
    /* +0x053 */ u8 unused_0x053;
    /* +0x054 */ u8 kitchen_busy_0x054;   /* set while the kitchen serves a meal */
    /* +0x055 */ u8 unused_0x055[0x21];
    /* +0x076 */ u8 field_0x076;
    /* +0x077 */ u8 field_0x077;
    /* +0x078 */ u8 unused_0x078[0x1];
    /* +0x079 */ s8 field_0x079;
    /* +0x07A */ u8 unused_0x07A[0x2];
    /* +0x07C */ union {   /* 0x07C-0x0AB: the slot run and the NPC/page units' views of it (the union is anchored
                            * at the aligned +0x07C so its u16 members keep their offsets) */
        struct {
            /* +0x07C */ u8 pad_0x07C;
            /* +0x07D */ u8 slots_0x07D[0x2F];
        };
        struct {
            /* +0x07C */ u8 pad_0x07C_b[0x4];
            /* +0x080 */ u8 field_0x080;
            /* +0x081 */ u8 pad_0x081[0x3];
            /* +0x084 */ union {
                u16 count_0x084;            /* the item count `menu_cursor_page_move` takes as its pad argument */
                u16 cmd_mask_0x084[2][5];   /* per-player UI command/availability words, rebuilt from the pad
                                             * record by `fn_80212370` */
            };
            /* +0x098 */ u8 pad_0x098[0x14];
        };
    };
    /* +0x0AC */ union {
        struct LbMenuWork* menu_0xAC;
        struct LbMenuPanel* menu_0x0AC;   /* the page unit's view of the same pointer */
    };
    /* +0x0B0 */ u8 field_0x0B0;
    /* +0x0B1 */ u8 sub_0x0B1;
    /* +0x0B2 */ u8 unused_0x0B2[0x6];
    /* +0x0B8 */ struct _se_w* field_0x0B8;   /* the sound-request handles `src/sound/fn_800D7F54.cpp` reads */
    /* +0x0BC */ struct _se_w* field_0x0BC;
    /* +0x0C0 */ union {
        u8 unused_0x0C0[0x6C];
        u8 talk_0x0C0[0x6C];
    };
    /* +0x12C */ u8 field_0x12C;   /* 1 puts the lobby act layer on hold */
    /* +0x12D */ u8 param_0x12D;
    /* +0x12E */ u8 unused_0x12E[0x1];
    /* +0x12F */ u8 param_0x12F;
    /* +0x130 */ u8 unused_0x130[0x19];
    /* +0x149 */ u8 meal_area_0x149;      /* the online meal's area code (the cook's +0xB6 byte / 4) */
    /* +0x14A */ u8 meal_received_0x14A;  /* nonzero once the host's meal has arrived */
    /* +0x14B */ u8 meal_skill_0x14B[3];  /* the received meal's skill ids */
    /* +0x14E */ s16 meal_value_0x14E[3]; /* their values */
    /* +0x154 */ u16 meal_bonus_0x154[4]; /* the received meal's bonuses */
    /* +0x15C */ u8 field_0x15C;
    /* +0x15D */ u8 field_0x15D;
    /* +0x15E */ u8 field_0x15E;
    /* +0x15F */ u8 field_0x15F;
    /* +0x160 */ u8 field_0x160;
    /* +0x161 */ u8 field_0x161;
    /* +0x162 */ union {
        u8 unused_0x162[0x14];
        struct {
            /* +0x162 */ u8 item_list_selection_0x162;   /* the item list's selected row, stored by the session's case 4 (GUESS) */
            /* +0x163 */ u8 field_0x163;
            /* +0x164 */ u8 pad_0x164[0x0B];
            /* +0x16F */ s8 field_0x16F;
            /* +0x170 */ u8 pad_0x170[0x2];
            /* +0x172 */ union {
                s16 field_0x172;
                u16 busy_0x172;   /* nonzero while a lobby transition owns the screen */
            };
            /* +0x174 */ u8 pad_0x174[0x2];
        };
    };
    /* +0x176 */ u8 field_0x176;
    /* +0x177 */ u8 unused_0x177[0x5];
} LbLobbyWork; /* size: 0x17C */

#endif /* MHTRI_LOBBY_LOBBY_WORK_H */
