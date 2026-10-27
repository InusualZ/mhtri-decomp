/*
 * lobby/lb_scroll_work.h - the lobby equipment scroll's work record (`LbScrollWork`, 0x80 B): the list kind and
 *   mode `lb_scroll_init` sets, the cursor, the side tables `lb_scroll_rows_build` fills, the owner player and the
 *   two staged three-record sides (`lobby/fn_80219260.cpp`'s `lb_equip_compare_set`/`lb_equip_base_set` fill them,
 *   its detail drawer reads them).  The functions that use it are `lobby/fn_80212810.cpp`'s scroll helpers and
 *   `lobby/fn_80219260.cpp`'s equipment page.
 */
#ifndef MHTRI_LOBBY_LB_SCROLL_WORK_H
#define MHTRI_LOBBY_LB_SCROLL_WORK_H

#include "types.h"
#include "Pl/plw.h"          /* _EQUIP */
#include "lobby/lb_list.h"   /* LbItemCount */

struct _PLW;

typedef struct LbScrollWork {
    /* +0x00 */ u8 kind_0x00;          /* the equipment kind the list shows (1-15) */
    /* +0x01 */ u8 flag_0x01;          /* the gunner part (11-13) the kind-11 panel opens on */
    /* +0x02 */ u8 mode_0x02;
    /* +0x03 */ u8 paged_0x03;         /* nonzero: the detail panel shows the bowgun's three parts */
    /* +0x04 */ u8 left_mode_0x04;
    /* +0x05 */ u8 right_mode_0x05;
    /* +0x06 */ u16 moved_0x06;         /* the buttons that last moved the page (for the page arrows) */
    /* +0x08 */ s16 cursor_0x08;       /* reset to 0 once it reaches count_0x0A */
    /* +0x0A */ s16 count_0x0A;
    /* +0x0C */ s16 value_0x0C;        /* the need list's row count (read as an `s8`) */
    /* +0x0E */ u16 value_0x0E;        /* the item id the smith/need panels show */
    /* +0x10 */ u8 flags_0x10;         /* bit 1: the compare is locked; bits 0-1: a compared set is staged */
    /* +0x11 */ u8 left_group_0x11;
    /* +0x12 */ u8 right_group_0x12;
    /* +0x13 */ u8 left_rows_0x13[10];
    /* +0x1D */ u8 right_rows_0x1D[10];
    /* +0x27 */ u8 unused_0x27;
    /* +0x28 */ u16 kakou_state_0x28;  /* the upgrade state `lb_kakou_source_draw` explains */
    /* +0x2A */ u8 unused_0x2A[2];
    /* +0x2C */ _EQUIP* kakou_src_0x2C; /* the upgrade source records */
    /* +0x30 */ const LbItemCount* need_items_0x30; /* the item-need list the kind-15 panel draws */
    /* +0x34 */ struct _PLW* owner_0x34; /* the player whose equipment the list stages */
    /* +0x38 */ _EQUIP cmp_0x38[3];    /* the compared side: a piece, or a bowgun frame, barrel and stock */
    /* +0x5C */ _EQUIP base_0x5C[3];   /* the base side, laid out the same way */
} LbScrollWork; /* size: 0x80 */

#endif /* MHTRI_LOBBY_LB_SCROLL_WORK_H */
