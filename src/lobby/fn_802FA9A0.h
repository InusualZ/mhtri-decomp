/* Declarations of `lobby/fn_802FA9A0.cpp`'s symbols and its own views of the `lobby_w`/`lobby_world_block` blocks it
 * reads (one view per unit, like `lobby/fn_8021E1EC.h`). */
#ifndef MHTRI_LOBBY_FN_802FA9A0_H
#define MHTRI_LOBBY_FN_802FA9A0_H

#include "types.h"

/* One goods row of the trader's buying tables (`lb_barter_goods_a`/`_b`, ended by item 0): the item, its base price
 * and whether the trader can ask for it.  size: 0x6 */
typedef struct LbBarterGoods {
    /* +0x00 */ u16 item_0x00;
    /* +0x02 */ u16 price_0x02;
    /* +0x04 */ u8 flag_0x04;   /* nonzero: one of the goods the trader may want */
    /* +0x05 */ u8 pad_0x05;
} LbBarterGoods;

/* One offer row of the trader's per-kind tables (`lb_barter_src_a`/`_b`): the item, the count it comes in, its price,
 * the unlock condition and progress flag that open it, and whether it can be taken once only.  size: 0xC */
typedef struct LbBarterSrc {
    /* +0x00 */ u16 item_0x00;
    /* +0x02 */ u16 count_0x02;
    /* +0x04 */ u16 price_0x04;
    /* +0x06 */ u16 cond_0x06;   /* the `lb_unlock_cond_ck` id */
    /* +0x08 */ u16 flag_0x08;   /* the progress flag that must be set, 0xFFFF none */
    /* +0x0A */ u8 once_0x0A;    /* 1: a progress entry, taken once */
    /* +0x0B */ u8 pad_0x0B;
} LbBarterSrc;

/* One row the trade screen shows or stocks: the item, its price, the count, the price rate and the once flag.
 * size: 0xC */
typedef struct LbBarterEntry {
    /* +0x00 */ u16 item_0x00;
    /* +0x02 */ u16 price_0x02;
    /* +0x04 */ s32 count_0x04;
    /* +0x08 */ u8 rate_0x08;
    /* +0x09 */ u8 flag_0x09;
    /* +0x0A */ u8 pad_0x0A[2];
} LbBarterEntry;

/* A reference to one open offer row: its index in its kind's table and the kind.  size: 0x4 */
typedef struct LbBarterRef {
    /* +0x00 */ u16 index_0x00;   /* 0xFFFF ends the list */
    /* +0x02 */ u8 kind_0x02;
    /* +0x03 */ u8 pad_0x03;
} LbBarterRef;

/* One cart row: the item, how many, the unit price and the once flag.  size: 0x8 */
typedef struct LbBarterCartRow {
    /* +0x00 */ u16 item_0x00;
    /* +0x02 */ u16 count_0x02;
    /* +0x04 */ u16 price_0x04;   /* 0: an empty row */
    /* +0x06 */ u8 flag_0x06;
    /* +0x07 */ u8 pad_0x07;
} LbBarterCartRow;

/* One row of the item hand-over request the settle builds.  size: 0x4 */
typedef struct LbBarterGetRow {
    /* +0x00 */ u16 item_0x00;
    /* +0x02 */ s16 count_0x02;
} LbBarterGetRow;

/* The trade screen's work block (lobby screen 11, `lobby_w.menu_0x0AC`; `lb_barter_open` clears its 0x2000 bytes).
 * size: 0x2000 */
typedef struct LbBarterWork {
    /* +0x000 */ u8 type_0x000;            /* 2: the trader buys at rolled rates */
    /* +0x001 */ u8 step_0x001;            /* the screen step `lb_barter_step` switches on */
    /* +0x002 */ u8 repeat_0x002;          /* the confirm key repeats while set */
    /* +0x003 */ u8 ship_bits_0x003;       /* the trade kinds of the visit (the whale-event work's bits) */
    /* +0x004 */ u16 item_0x004;           /* the item the trader was given, marked in the hold strip */
    /* +0x006 */ u8 field_0x006;
    /* +0x007 */ u8 got_index_0x007;       /* the once-only row the hand-over shows */
    /* +0x008 */ u8 kind_0x008;            /* the kind `lb_barter_refs_kind_build` listed */
    /* +0x009 */ u8 menu_0x009[5];         /* the top menu's rows */
    /* +0x00E */ u8 pad_0x00E[2];
    /* +0x010 */ s16 mode_0x010;           /* 0 buying (price times count), 1 bartering */
    /* +0x012 */ u8 pad_0x012[0x30 - 0x12];
    /* +0x030 */ s16 tab_0x030;            /* 0 stock, 1 offers, 2 settle */
    /* +0x032 */ s16 cursor_0x032;
    /* +0x034 */ s16 rows_0x034;           /* the page's rows plus the back row */
    /* +0x036 */ s16 menu_count_0x036;
    /* +0x038 */ s16 cart_row_0x038;
    /* +0x03A */ s16 cart_col_0x03A;
    /* +0x03C */ s16 page_0x03C;
    /* +0x03E */ s16 pages_0x03E;
    /* +0x040 */ s16 pages_a_0x040;        /* the pages of the first offer table */
    /* +0x042 */ s16 cursor_b_0x042;
    /* +0x044 */ u16 moved_0x044;
    /* +0x046 */ s16 count_0x046;          /* the hold strip's items, then the goods handed over */
    /* +0x048 */ u8 pad_0x048[4];
    /* +0x04C */ s16 shown_0x04C;
    /* +0x04E */ u8 pad_0x04E[2];
    /* +0x050 */ u32 dialog_0x050;         /* the confirmation dialogs' state word */
    /* +0x054 */ s32 frames_0x054;
    /* +0x058 */ u8 ready_0x058;           /* the trade could be settled */
    /* +0x059 */ u8 pad_0x059[3];
    /* +0x05C */ s32 arg_0x05C;
    /* +0x060 */ s32 arg_0x060;
    /* +0x064 */ LbBarterSrc* srcs_0x064[3];   /* each kind's offer table */
    /* +0x070 */ u32 blink_0x070;          /* the cart blink's frame, 0 off */
    /* +0x074 */ u8 rates_0x074[0xC0];     /* each goods row's rolled rate class */
    /* +0x134 */ u16 wanted_0x134[4];      /* the goods the trader wants */
    /* +0x13C */ u16 got_0x13C[5];         /* the once-only rows taken */
    /* +0x146 */ LbBarterRef refs_0x146[0xC0];
    /* +0x446 */ u8 pad_0x446[2];
    /* +0x448 */ LbBarterEntry page_0x448[5];
    /* +0x484 */ LbBarterEntry stock_0x484[0x40];
    /* +0x784 */ LbBarterCartRow get_0x784[10];   /* what the player takes */
    /* +0x7D4 */ LbBarterCartRow pay_0x7D4[10];   /* what the player gives */
    /* +0x824 */ LbBarterGetRow req_0x824[11];    /* the hand-over request (0x29 bytes cleared) */
    /* +0x850 */ u16 hold_items_0x850[10];
    /* +0x864 */ u8 unused_0x864[0x2000 - 0x864];
} LbBarterWork;

/* One absence range of the whale-event work, in days.  size: 0x2 */
typedef struct LbKujiraRange {
    /* +0x00 */ u8 min_0x00;
    /* +0x01 */ u8 max_0x01;
} LbKujiraRange;

/* The lobby work block (`lobby_w`, .bss 0x806AAB44, 0x17C B in the map) as THIS range sees it: only the fields
 * below are touched - the rest of the block is filler because the range never reaches into it (rule 5's padding
 * exception).  The offsets come from the range's own code.  size: 0x17C */
typedef struct LbLobbyView {
    /* +0x000 */ u8 field_0x000;   /* the lobby screen (`lb_barter_open` sets 11) */
    /* +0x001 */ u8 unused_0x001[0x2];
    /* +0x003 */ u8 field_0x003;   /* 0 while the lobby act layer is idle */
    /* +0x004 */ u8 unused_0x004[0x4];
    /* +0x008 */ u8 field_0x008;   /* `lb_barter_open` sets it to 1 */
    /* +0x009 */ u8 unused_0x009[0x70];
    /* +0x079 */ s8 field_0x079;   /* `lb_pig_dress_refresh` sets it to 2 around the dress refresh */
    /* +0x07A */ u8 unused_0x07A[0x32];
    /* +0x0AC */ LbBarterWork* menu_0x0AC;  /* the open screen's work block (the trade screen's here) */
    /* +0x0B0 */ u8 unused_0x0B0[0xAF];
    /* +0x15F */ u8 kujira_0x15F;  /* == 1 while the whale event is up (`LbCheckKujiraEvent`) */
    /* +0x160 */ u8 unused_0x160[0x1];
    /* +0x161 */ u8 field_0x161;   /* the second event gate `lb_event_gate2_ck` reports */
    /* +0x162 */ u8 unused_0x162[0x1A];
} LbLobbyView; /* size: 0x17C */

/* The whale-event work at +0x4832 of the block `lobby_world_block` points at: the trader's visit.  size: 0x0C (the
 * extent this range reads; the parent block's filler covers the gap to its next field) */
typedef struct LbKujiraWork {
    /* +0x00 */ u8 flag_0x00;   /* the trade kinds of the current visit, mirrored from +0x01 */
    /* +0x01 */ u8 bits_0x01;   /* one bit per unlocked trade kind (`lb_kujira_bit_set`) */
    /* +0x02 */ s8 count_0x02;  /* days left: negative while the trader stays, positive while it is away */
    /* +0x03 */ u8 field_0x03;  /* cleared when a stay starts */
    /* +0x04 */ u16 seed_0x04;  /* the trade seed `lb_kujira_seed_roll` draws */
    /* +0x06 */ u16 item_0x06;  /* the item the trader was given (0xFB/0xED shorten the absence) */
    /* +0x08 */ u8 unused_0x08[0x4];
} LbKujiraWork; /* size: 0x0C */

/* The block the `.sbss` pointer `lobby_world_block` (0x80794880) points at, as this range sees it: the
 * mode byte at +0x3E01 (`lb_world_mode_get`), the per-kind state bytes at +0x4654 (`lb_world_upgrade_get`) and the
 * whale-event work at +0x4832.  Everything between them is filler.  size: 0x6010 (the neighbour view's extent -
 * `lobby/fn_8021E1EC.h`) */
typedef struct LbBlockView {
    /* +0x0000 */ u8 unused_0x0000[0x3E01];
    /* +0x3E01 */ u8 mode_0x3E01;   /* the scene byte the area gate compares against 4 */
    /* +0x3E02 */ u8 village_rank_0x3E02; /* raised to 1..3 by the progress entries 0x1A/0x21/0x26 */
    /* +0x3E03 */ u8 unused_0x3E03[0x3F3A - 0x3E03];
    /* +0x3F3A */ u8 level_0x3F3A;   /* raised to 1/2 by the progress entries 0x00/0x03 */
    /* +0x3F3B */ u8 unused_0x3F3B[0x3F46 - 0x3F3B];
    /* +0x3F46 */ u8 level_0x3F46;   /* raised to 1/2 by the progress entries 0x01/0x04 */
    /* +0x3F47 */ u8 unused_0x3F47[0x3F52 - 0x3F47];
    /* +0x3F52 */ u8 level_0x3F52;   /* raised to 1/2 by the progress entries 0x02/0x05 */
    /* +0x3F53 */ u8 unused_0x3F53[0x4654 - 0x3F53];
    /* +0x4654 */ union {
        u8 states_0x4654[0x1E];      /* the trade ship's save record (`quest/quest_types.h`'s `LbShipBlock`) */
        struct {
            /* +0x4654 */ u8 grade_0x4654[4];   /* the grade of each goods table (0 locked) */
            /* +0x4658 */ u8 orders_0x4658[0x18];
            /* +0x4670 */ u8 ships_0x4670;      /* how many ships sail */
            /* +0x4671 */ u8 pad_0x4671;
        }; /* size: 0x1E */
    };
    /* +0x4672 */ u8 unused_0x4672[0x4832 - 0x4672];
    /* +0x4832 */ LbKujiraWork work_0x4832;
    /* +0x483E */ u8 unused_0x483E[0x516C - 0x483E];
    /* +0x516C */ u8 kitchen_pairs_0x516C[0xF0];  /* the kitchen pairs tried offline (`quest/quest_types.h`) */
    /* +0x525C */ u8 kitchen_pairs_0x525C[0x14];  /* the pairs tried online */
    /* +0x5270 */ u8 unused_0x5270[0x6010 - 0x5270];
} LbBlockView; /* size: 0x6010 */

/* The caller's record, only ever handled by address here (`struct _ENEMY_WORK*` parameters). */
struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif

extern LbLobbyView lobby_w;      /* .bss 0x806AAB44 */
extern LbBlockView* lobby_world_block; /* .sbss 0x80794880 */

/* The two id tables the predicates walk, in this unit's `.data` claim: 28 entries (ids 0..27) and 12 (ids 28..39),
 * each pointing at a `u16` array terminated by 0xFFFF. */
extern u16* lb_unlock_cond_tbl0[28];
extern u16* lb_unlock_cond_tbl1[12];

/* The trade screen's data: the price rate of each rate class (percent), the two rate-class weight tables the mood
 * picks between, the mood weights, the whale event's absence ranges, the trader's two goods tables and the two
 * per-kind offer tables. */
extern u8 lb_barter_rate_pct[];
extern u8 lb_barter_mood_weights[];
extern u8* lb_barter_rate_weight_tbls[];
extern LbKujiraRange lb_kujira_stay_ranges[];
extern LbBarterGoods lb_barter_goods_b[];
extern LbBarterGoods lb_barter_goods_a[];
extern LbBarterSrc* lb_barter_src_a[3];
extern LbBarterSrc* lb_barter_src_b[3];

/* `lb_world_mode_get` (0x802FB5E4) is defined by this range and read by `lb_event_state_ck` below. */
u8 lb_world_mode_get(void);

/* The whale-event work helpers, called by `lb_progress_unlock` and `lb_kujira_work_reset` ahead of their
 * definitions. */
void lb_kujira_work_reset(void);
/* 0x802FF234 - whether the whale-event countdown has gone negative. */
u32 kujira_event_over_ck(void);
void lb_kujira_bit_set(u32 raw);
void lb_kujira_seed_roll(LbKujiraWork* work);

/* The trade screen (lobby screen 11): `lobby/fn_80219260.cpp` opens it, `lobby/fn_8030121C.cpp` steps and draws it. */
void lb_barter_open(s32 arg_a, s32 arg_b);
void lb_barter_close(void);
s16 lb_barter_kind_count(void);
u32 lb_barter_src_ok_ck(LbBarterSrc* src);
void lb_barter_refs_build(LbBarterWork* work, u8* mask);
u8 lb_barter_rate_get(LbBarterWork* work, u16 index);
u16 lb_barter_price_scale(u16 price, u8 rate);
u16 lb_barter_row_price_get(LbBarterWork* work, u16 price, u16 index);
void lb_barter_page_fill(LbBarterWork* work, s16 page);
void lb_barter_entry_copy(LbBarterEntry* dst, LbBarterEntry* src);
void lb_barter_stock_add(LbBarterWork* work, u16 item, s32 delta);
s32 lb_barter_stock_get(LbBarterWork* work, u16 item);
s32 lb_barter_wanted_ck(LbBarterWork* work, u16 item);
void lb_barter_stock_build(LbBarterWork* work);
void lb_barter_reset(LbBarterWork* work);
u32 lb_barter_settle_ok_ck(LbBarterWork* work);
s16 lb_barter_cart_slot_find(LbBarterWork* work, LbBarterCartRow* cart, u16 item, u8 once);
u16 lb_barter_cart_total(LbBarterWork* work, LbBarterCartRow* cart);
u32 lb_barter_entry_flags(LbBarterWork* work, LbBarterCartRow* cart, s16 index);
void lb_barter_cart_add(LbBarterWork* work, LbBarterCartRow* cart, LbBarterEntry* entry, s16 count);
s16 lb_barter_settle(LbBarterWork* work);
void lb_barter_restart(LbBarterWork* work, u8* mask);
s32 lb_barter_step(LbBarterWork* work, u8* mask);
void lb_barter_menu_init(LbBarterWork* work);
void lb_barter_hold_open(LbBarterWork* work);
void lb_barter_refs_kind_build(LbBarterWork* work, u8 kind);
void lb_barter_page_fill_kind(LbBarterWork* work, s16 page);

/* The screen-filter effect's (type 0x24) four state steps `enemy/em_sub_state_prog.cpp`'s `fn_802FA964` dispatches
 * to; the record is the effect instance (`_EFT`), the parameter keeps the caller's `_ENEMY_WORK` spelling. */
void eft036_model_create(struct _ENEMY_WORK* work);
void eft036_move(struct _ENEMY_WORK* work);
void eft036_end(struct _ENEMY_WORK* work);
void eft036_release(struct _ENEMY_WORK* work);

/* 0x802FB97C */
u32 lb_event_state_ck(void);
/* 0x802FB9F8 - whether the lobby work block's second event gate (+0x161) is up. */
u32 lb_event_gate2_ck(void);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
/* 0x802FB9DC - whether the whale event is up; the `ef` band calls it.  C++ linkage: the map spells it mangled
 * (rule 9). */
u32 LbCheckKujiraEvent(void);
#endif

#endif /* MHTRI_LOBBY_FN_802FA9A0_H */
