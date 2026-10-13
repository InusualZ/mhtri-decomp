/*
 * menu/menu_infomation.cpp - the equipment-information screen: the status pages, the equipment detail putters per
 *   weapon kind, the panel rows and the column arrangement (`Set_equip_column_arrangement`).  C++; the named rows are
 *   the free functions their manglings spell, every bare stem is `extern "C"`.
 * RANGE. .text 0x80308FB4-0x8031A6C0 (149 functions); extab, extabindex, .data 0x805DCC50-0x805DCE98, .sdata
 *   0x80792BB8-0x80792C20, .sdata2 0x8079AE30-0x8079AE50.  The whole run is one TU: `.data` 0x805DCCDC is the bare
 *   source name "menu_infomation.cpp" (one copy in the DOL; `fn_80312F84` builds it with its assert message at
 *   0x80313020) and its referrers span 0x8030A328-0x8031A244.  The extab/extabindex runs tile with
 *   `ef/fn_8030681C.cpp` below and `menu/fn_8031A6C0.cpp` above.
 * FLAGS. `cflags_menu` (configure.py); its `-opt nopeephole` is what `equip_list_page_count`'s target wants.
 * NAMES. Module `menu` and the file name from the `__FILE__` string (the retail misspelling is the original).  Of the
 *   map's rows five are named (the four `Put_*` putters, `Set_equip_column_arrangement`); the rest are named for their
 *   bodies.  GUESSes, from the argument list `put_equip_category_panel` calls them with: `equip_variant_resolve`/
 *   `equip_variant_resolve_ex`, `put_equip_panel_row_ex`/`put_equip_panel_row_base`, `put_equip_panel_kind4`/
 *   `put_equip_panel_kind5`.
 *   GUESS: `equip_detail_melee_draw`, `equip_detail_bowgun_draw`, `equip_detail_kind6_draw`,
 *   GUESS: `equip_detail_slots_draw`
 *   GUESS (from its reader `fn_80313E24`): `equip_panel_row_layout`
 * RESIDUALS. 59 rows unwritten (objdiff scores them zero): 0x80309098-0x8030A1D0, 0x8030A1DC-0x8030A30C,
 *   0x8030A328-0x8030B790, 0x8030B868-0x8030BACC, 0x8030BAF4-0x8030CA50, 0x8030CA68-0x8030D338,
 *   0x8030D798-0x8030D808, 0x8030D980-0x8030E784, 0x8030E79C-0x8030F374, 0x8030F38C-0x8030F7B0,
 *   0x8030F87C-0x8030FAC0, 0x8030FDA0-0x8030FEF4, 0x8030FF0C-0x80310548, 0x8031077C-0x80310E40,
 *   0x80310F30-0x80311540, 0x8031165C-0x80311840, 0x80311920-0x80311A88, 0x80311AE8-0x80312DF0,
 *   0x80312E34-0x80312F18, 0x80312F84-0x80313E24.  25 more rows in 0x8031468C-0x80319F18 are empty definitions kept
 *   so the map's rows pair by name; each is a body still to write: 0x8031468C, 0x80314A64, 0x80314F90, 0x80315274,
 *   0x80315A60, 0x80315C00, 0x80315D0C, 0x80315E34, 0x80316058, 0x80316780, 0x80316A14, 0x80316DDC, 0x803170B4,
 *   0x803173C4, 0x80317788, 0x80317B84, 0x80317F94, 0x803182D8, 0x803188AC, 0x80318A10, 0x80318E08, 0x80318FB0,
 *   0x80319204, 0x8031949C, 0x80319F18.
 *  - Blocked by a name: `fn_80314F90` calls `fn_8027F5C4`/`fn_8027F0F4` (`Pl`, unnamed and undeclared in an owner
 *    header), so its body would add generated tokens.
 *  - Blocked by a header: `Put_equip_dtl_basis_sword_colorX` passes `_EQUIP_INDEX::shown[0]` to `fn_8027FF88`, which
 *    `Pl/fn_8027D684.h` declares as taking a `u32`; `fn_803153F8` clears `_PLW` +0x61A/+0x62A through `EquipColumnPanel`
 *    because `Pl/plw.h` names them `unk61A`/`unk62A`.
 *  - Blocked by a declaration: `fn_8030C314` (`put_equip_category_panel`) calls `get_rare_color__FUc`, which
 *    `hud/layout.h` declares inside its `extern "C"` block, so our object would emit the unmangled name; the fix is
 *    moving it to the C++ block (and the owner's `ai/fn_802D44F4.h`).
 *  - `put_equip_info_upper_page`/`put_equip_info_piece_page` read `lbl_80792BF4` and `lobby_world_block`.
 *   The partial rows include:
 *  - `fn_80311540`: retail masks the third argument (`rlwinm r0,r6,0,31,28` + `clrlwi`) before `li r5,0`; `c &= ~6`
 *    gives `andi.` after it, `c & ~6` in the call the late `rlwinm`;
 *  - `fn_80311560`: the `mr r31,r8` save is scheduled two instructions earlier than retail;
 *  - `fn_80313E24`: register colouring only - retail keeps the five arguments in r16-r20 and the two assert-string
 *    bases in r30/r31, MWCC one window higher;
 *  - `fn_80319178`: the count and the index swap r7/r8;
 *  - `put_equip_panel_row_zero`: `flag &= 7` on a `u8` gives `clrlwi r0,r0,29`, retail `(u8)(x & 0x80000007)`;
 *  - `put_lsp_sprite_at_anchor`: one argument-register move taken in the other order;
 *  - `fn_80312DF0`: the two anchor words take each other's stack slots (`stw r0,0xc(r1)`, retail `0x8(r1)`);
 *  - `fn_80314604`: retail tests `bowgun == 1` (`cmplwi r31,0x1` + `bne`), MWCC folds it to `cmpwi r31,0x0` + `beq`;
 *  - `fn_8031A58C`: retail keeps an `extsh r3,r3` on `menu_page_count`'s result that ours drops;
 *  - `Put_equip_dtl_basis_sword_colorX`: see the `fn_8027FF88` header blocker above.
 *   flipcheck: `.sdata` (0x68) and `.sdata2` (0x20) are claimed and our object emits neither; `.data` 0xF2 of 0x248 (the
 *   jump tables, the assert strings, `equip_panel_row_layout`) in a different order; `.text`, extab and extabindex are
 *   short of the target's.
 * SHAPES. `fn_8030FB78`'s `d.x = d.x - pos.x` stores through the `extsh` retail has; `d.x -= pos.x` drops it
 *   (playbook 38).  `fn_8030D808`/`fn_803106A8` copy the anchor offset as one word and use `-=`.
 *  - The resolve-then-dispatch entries return the resolver's `u8` (`if (r == 1) return r; return dispatch(...)`):
 *    retail's `bne`+`b` pair; the `fn_8030F87C` and `put_equip_panel_row_ex` families stay `void` (they measure worse).
 *  - Locals declared in the order that gives retail's stack slots (`fn_803142C8`, `fn_80314340`, `fn_803144A0`,
 *    `fn_8031994C`); `fn_803159CC` assigns one `ret` in every arm, which keeps retail's branches.
 *  - `equip_panel_row_layout` is defined after its reader: defined above it, MWCC addresses it and the assert strings
 *    from one `.data` base register, which retail does not.
 *  - The `Panic` calls carry retail's line through `#line` (6824, 6956, 10920) and string literals.
 *  - Records: `MenuSlot` (`menu/menu_item.h`) is the 0x330-byte working record; `StatusScreenWork`
 *    (`menu/menu_infomation.h`) is the head's partial view of it, shared with `ef/fn_8030681C.cpp`; its +0x190 `equip` is
 *    the `_PLW` (`Pl/plw.h`) the putters read (`equipA[6]`, `equipB`/`equipC`/`equipD`).  `_EQUIP_INDEX` (the 0x18-byte
 *    shown/worn view the resolvers fill), `EquipColumnPanel` (a view of `_PLW` +0x61A), `EquipSlotInfo` and
 *    `EquipListWork` are this unit's own; `fn_8027ECAC`'s record is `PlSkillRec` (`Pl/fn_8027D684.h`).
 */

#include "types.h"
#include "pl.h"
#include "hud/layout.h"
#include "menu/menu_item.h"
#include "menu/menu_message.h"
#include "menu/menu_infomation.h"
#include "Pl/pl_skill.h"
#include "Pl/fn_8027D684.h"
#include "ef/fn_800CDB2C.h"
#include "ai/fn_802D44F4.h"
#include "unsplit/menu.h"
#include "userdata_item.h"
#include "nw4r/db_assert.h"

extern "C" int sprintf(s8*, const char*, ...);   /* 0x8045DECC, the OS Runtime's */

/* The resolved equipment view the detail putters draw from, filled by `equip_variant_resolve`/
 * `equip_variant_resolve_ex` and the six-piece resolver at 0x80315730: the three pieces shown (for a bowgun the
 * frame/barrel/stock triple, kinds 0xB/0xC/0xD) and the three the hunter wears beside them (`_PLW`'s
 * +0x1D0/+0x1E8/+0x1F4 records).  size: 0x18 (the six words the resolvers clear) */
struct _EQUIP_INDEX {
    /* +0x00 */ _EQUIP* shown[3];
    /* +0x0C */ _EQUIP* worn[3];
};

/* The per-hunter panel `fn_803153F8` clears: two parallel 8-entry tables, the first `u16` and the
 * second `u8`, starting at +0x61A.  size: 0x632 (approximate: max touched offset + 1). */
struct EquipColumnPanel {
    /* +0x000 */ u8  pad_0x000[0x61A];
    /* +0x61A */ u16 field_0x61A[8];
    /* +0x62A */ u8  field_0x62A[8];
};

/* The slot-descriptor `fn_80319178` walks (a 3-way equipment slot selection).  size: 0x0C
 * (approximate: max touched offset + 1). */
struct EquipSlotInfo {
    /* +0x00 */ u8  pad_0x00;
    /* +0x01 */ u8  idx;              /* the active slot index, 0..2 */
    /* +0x02 */ u8  pad_0x02[2];
    /* +0x04 */ u16 flags;            /* two bytes the caller reads as the low/high half */
    /* +0x06 */ u16 slots[3];         /* the three slot ids */
};


/* This unit's own C++-linkage exports (address order).  They are declared as their real signatures so
 * the C++ front-end reproduces the map's mangling (rule 9). */
void Put_equip_dtl_basis_bowgun_gan1(_PLW*, _EQUIP_INDEX*, u16, u8, _mh_ivec2_*);
void Put_equip_dtl_basis_bowgun_gan2(_PLW*, _EQUIP_INDEX*, u16, u8, _mh_ivec2_*);
void Put_equip_dtl_basis_bowgun_gan_lv(_PLW*, _EQUIP_INDEX*, u16, u8, _mh_ivec2_*);
u8   Set_equip_column_arrangement(_PLW*, _EQUIP_INDEX*, _EQUIP*);

#ifdef __cplusplus
extern "C" {
#endif

/* This unit's own forward declarations (address order).  They carry C linkage here so the definitions
 * below emit the map's bare `fn_XXXXXXXX` stems (rule 9: an `fn_` stem is not a mangling). */
void equip_info_update(StatusScreenWork*);
void put_equip_info_upper_page(StatusScreenWork*);
void put_equip_info_piece_page(StatusScreenWork*);
s32  fn_8030A1D0(_PLW*, s32);
s32  fn_8030A1DC(_PLW*, s32, s32, s32);
s32  equip_page_count_step(_PLW*, void*, u32, s8);
s32  fn_8030A328(void*, void*, s32, s32, u16, s8);
void equip_detail_page_refresh(_PLW*, _EQUIP*, s8 page, s8 last, u16 flags);
void fn_8030BACC(StatusScreenWork*);
s32  equip_detail_melee_draw(_PLW*, _EQUIP*, u32, u32);
s32  fn_8030CA68(_PLW*, _EQUIP*, _EQUIP*, u16, u8);
u8   fn_8030D6C0(_PLW*, _EQUIP*, _EQUIP*, u16, u8);
void equip_detail_bowgun_draw(_PLW*, _EQUIP*, u16, u8);
u8   fn_8030D808(_PLW*, _EQUIP_INDEX*, u16, u8);
u8 put_equip_row_variant(_PLW*, _EQUIP*, _EQUIP*, _EQUIP*, u16, u8);
void equip_detail_slots_draw(_PLW*, _EQUIP*, u16, u8);
void fn_8030F7B0(_PLW*, _EQUIP*, _EQUIP*, u16, u8);
u8 put_equip_row_variant_ex(_PLW*, _EQUIP*, _EQUIP*, _EQUIP*, u16, u8);
u8 fn_8030FD30(_PLW*, _EQUIP*, _EQUIP*, _EQUIP*, _EQUIP*, _EQUIP*, _EQUIP*, u16, u8);
u8 fn_8030FC58(_PLW*, _EQUIP*, _EQUIP*, u16, u8);
u8 fn_80310560(_PLW*, _EQUIP*, _EQUIP*, u16, u8);
u8 put_equip_row_variant2(_PLW*, _EQUIP*, _EQUIP*, _EQUIP*, u16, u8);
u8 fn_80310638(_PLW*, _EQUIP*, _EQUIP*, _EQUIP*, _EQUIP*, _EQUIP*, _EQUIP*, u16, u8);
u8 fn_80310E58(_PLW*, _EQUIP*, _EQUIP*, u16, u8);
u8 fn_80310EC0(_PLW*, _EQUIP*, _EQUIP*, _EQUIP*, u16, u8);
u8 fn_80310F30(_PLW*, _EQUIP_INDEX*, u16, u8);
void fn_80311560(_PLW*, _EQUIP*, _EQUIP*, u16, u8, u8);
void put_equip_panel_row_ex(_PLW*, _EQUIP_INDEX*, u16, u8, u8);
void put_equip_panel_row_variant(_PLW*, _EQUIP*, _EQUIP*, _EQUIP*, u16, u8, u8);
void put_equip_panel_row_zero(_PLW*, _EQUIP*, u16, u8);
void put_equip_panel_row(_PLW*, _EQUIP*, _EQUIP*, _EQUIP*, u16, u8);
void put_lsp_sprite_at_anchor(u32*, u8);
void put_lsp_sprite_runs(u32*);
void put_equip_panel_row_base(_PLW*, _EQUIP_INDEX* view, u16 a, u8 b);
void fn_8030F87C(_PLW*, _EQUIP_INDEX*, u16, u8);
void put_lsp_sprite_offset(u16);
u8 fn_8030FDA0(_PLW*, _EQUIP_INDEX*, u16, u8);
u8 fn_803106A8(_PLW*, _EQUIP_INDEX*, u16, u8);
u8   fn_8031077C(_PLW*, _EQUIP_INDEX*, u8, _mh_ivec2_*);
void fn_8030E79C(_PLW*, _EQUIP*, _EQUIP*, u16, u8);
void fn_8030E784(_PLW*, _EQUIP*, u16, u8);
void fn_8030F38C(_PLW*, _EQUIP*, _EQUIP*, u16, u8);
void equip_detail_kind6_draw(_PLW*, _EQUIP*, u16, u8);
void fn_8030FB78(u16, u8);
void fn_8030FB60(u16);
void fn_8030FB6C(u16);
void fn_8030FC40(_PLW*, _EQUIP*, u16, u8);
void fn_8030FF0C(_PLW*, _EQUIP*, _EQUIP*, u16, u8);
void fn_8030FEF4(_PLW*, _EQUIP*, u16, u8);
void fn_80310548(_PLW*, _EQUIP*, u16, u8);
void fn_80310E40(_PLW*, _EQUIP*, u16, u8);
void fn_80311540(_PLW*, _EQUIP*, u16, u8, u8);
void fn_80312DF0(u32*);
void fn_80313E24(_PLW*, _EQUIP_INDEX*, u8, u8, const _mh_ivec2_*);
void fn_803142C8(u8, u8, const _mh_ivec2_*, u8, u8);
void fn_80314340(u8 kind, u8 idx, const _mh_ivec2_* pos, u8 state, u8 mode);
void fn_803144A0(u8 row, u8 col, u8 icon, const _mh_ivec2_* pos, u8 value, u8 state, u32 selected, u8 mode);
u8   fn_80314604(_PLW*, s8, u32);
void fn_8031468C(void);
u32  fn_803149B8(u16, u32*, u8, u32*);
u32  fn_80314A64(u16, u32, u32, u32*);
u8   fn_80314F18(_EQUIP*, _EQUIP*);
s32  fn_80314F90(_EQUIP*, _EQUIP*, u16*);
void fn_80315274(_PLW*, _EQUIP_INDEX*, u8);
void fn_803153F8(EquipColumnPanel*, u8);
u8   equip_variant_resolve(_PLW*, _EQUIP*, _EQUIP*, u8*, _EQUIP_INDEX*);
u8   equip_variant_resolve_ex(_PLW*, _EQUIP*, _EQUIP*, _EQUIP*, u8, _EQUIP_INDEX*);
u8   fn_80315730(_PLW*, _EQUIP*, _EQUIP*, _EQUIP*, _EQUIP*, _EQUIP*, _EQUIP*, u8*, _EQUIP_INDEX*);
u8   fn_803159CC(u8);
u8   fn_80315A60(_PLW*, _EQUIP_INDEX*, u8);
void fn_80315C00(void);
void fn_80315D0C(void);
void fn_80315E34(void);
void fn_80316058(void);
u8   fn_8031622C(_PLW*, _EQUIP_INDEX*, u8, u8*, s16*);
u8   fn_803164D0(_PLW*, _EQUIP_INDEX*, u8, u8*, s16*);
void fn_80316780(void);
u32  fn_80316978(u32, _EQUIP**, u8, u8*);
void fn_80316A14(void);
void fn_80316DDC(void);
void fn_803170B4(void);
u8   fn_8031729C(u32, _EQUIP_INDEX*, u8, u8*);
void fn_803173C4(void);
void fn_80317788(void);
void fn_80317B84(void);
void fn_80317F94(void);
u8   fn_80318204(u8, u8*, u8*);
void fn_803182D8(void);
u8   fn_80318790(u32, u8**, u8);
void fn_803188AC(void);
u8   fn_80318A10(_PLW*, _EQUIP_INDEX*, u8, u8, u8*);
u8   fn_80318E08(_PLW*, _EQUIP_INDEX*, u8, u8);
void fn_80318FB0(void);
u8   fn_80319178(EquipSlotInfo*, u16*, s8*);
void fn_80319204(void);
void fn_8031949C(void);
s32  fn_8031994C(_PLW*, _EQUIP_INDEX*, u8, u16*, u8*);
void fn_80319F18(void);
void fn_8031A410(_PLW*, _EQUIP_INDEX*, _EQUIP*);
void fn_8031A428(_PLW*, _EQUIP*, _EQUIP*, _EQUIP*, _EQUIP*);
void fn_8031A58C(MenuSlot*);
void fn_8031A638(MenuSlot*);

/* Callees other units own.  Most are declared in their owner's header and included above
 * (`fn_8027FF88`, `fn_8027F11C`, `fn_8027ECAC`, `fn_8027FFFC` in `Pl/fn_8027D684.h`; `Pl_Skill_slot_item_get`
 * and `pl_item_add` in `Pl/pl_skill.h`; `fn_8029FFFC` and `get_menu_lsp_tbl` in `menu/menu_item.h`;
 * `GameMode_ck` in `ef/fn_800CDB2C.h`; `put_lsp_anchor_offset` and `get_str_tbl` in `unsplit/menu.h`).
 * The two below cannot: `fn_8031AE38`/`fn_8031BFEC` are owned by `menu/fn_8031A6C0.cpp`, whose header
 * declares neither and `unsplit/menu.h` may not.  Declared here as this unit's view. */
void fn_8031AE38(MenuSlot* self);
void fn_8031BFEC(void* cursor, s32 kind, MenuSlot* owner);

extern u32 lbl_805DCD48[];          /* .data: the per-colour packed word table */
extern const char lbl_80792BE4[3];  /* .sdata "%d" */
extern const char lbl_80792BE8[3];  /* .sdata "%s" */
extern const char lbl_80792BEC[5];  /* .sdata "%d%s" */

#ifdef __cplusplus
}
#endif

/* ---------------------------------------------------------------------------------------------------
 * 0x80308FB4..0x8030CA68 - the screen half's head: the five bodies the old `ef/fn_8030681C.cpp`
 * registration held (it ran into this range at 0x80308FB4, a seam the `__FILE__` string proves false)
 * --------------------------------------------------------------------------------------------------- */

/* The equip page count with the default column arrangement. */
s32 fn_8030A1D0(_PLW* equip, s32 mode)
{
    return fn_8030A1DC(equip, mode, 0, 0);
}

/* The same, for a caller that carries a page id and a signed page delta. */
s32 equip_page_count_step(_PLW* a, void* b, u32 page, s8 delta)
{
    return fn_8030A328(a, b, 0, 0, page, delta);
}

/* The equip detail page with the row and the item list's own selector. */
s32 equip_detail_melee_draw(_PLW* a, _EQUIP* b, u32 row, u32 sel)
{
    return fn_8030CA68(a, b, 0, row, sel);
}

/* Resets the screen's page counters and the four row flags for the equip-detail mode (mode 8). */
void fn_8030BACC(StatusScreenWork* self)
{
    self->field_0x14 = 8;
    self->field_0x1C = 0;
    self->field_0x1A0 = 0;
    self->field_0x1A1 = 0;
    self->field_0x1A2 = 0;
    self->field_0x1A3 = 0;
    self->field_0x19E = 0;
}

/* The number of equip rows the screen shows: every row whose slot id and flag are both set, rounded up
 * to the six-per-page step (`(n + 5) / 6`) and never below one page. */
s32 equip_list_page_count(EquipListWork* self)
{
    u32 i;
    s32 count = 0;

    for (i = 0; i < 8; i++) {
        if (self->slot_id[i] != 0 && self->row_flag[i] != 0) {
            count++;
        }
    }

    {
        s8 pages = (s8)((count + 5) / 6);
        if (pages == 0) {
            pages = 1;
        }
        return pages;
    }
}

/* 0x8030D338 - the sword's colour panel: classifies the slot (`fn_8027FF88`), draws the `get_menu_lsp_tbl(0x8E|0x8F)`
 * sprite run recoloured from `lbl_805DCD48` and, for the colour forms, up to two label rows. */
void Put_equip_dtl_basis_sword_colorX(_PLW* plw, _EQUIP_INDEX* idx, u16 a, u16 b,
                                      _mh_ivec2_* pos, u8 c) {    u8 n = fn_8027FF88((u32)idx->shown[0]);
    if (n == 0) {
        return;
    }
    int sel = n - 1;
    u32 color = lbl_805DCD48[(u8)sel];
    u16* tbl;
    _mh_ivec2_ vec;
    u32 srcA;
    u32 srcB;
    s8 buf[8];
    _SPR_DATA_ spr;
    if ((u8)sel < 3) {
        tbl = (u16*)get_menu_lsp_tbl(0x8E);
        if (a == 0) {
            srcA = *(u32*)pos;
            put_lsp_anchor_offset(0x80E, &vec, &srcA);
        } else if (pos == NULL) {
            get_lsp_data(0, &vec);
        } else {
            return;
        }
    } else {
        tbl = (u16*)get_menu_lsp_tbl(0x8F);
        if (b == 0) {
            srcB = *(u32*)pos;
            put_lsp_anchor_offset(0x819, &vec, &srcB);
        } else if (pos == NULL) {
            get_lsp_data(0, &vec);
        } else {
            return;
        }
    }
    while (*tbl != 0xFFFF) {
        spr_data_copy(&spr, get_lsp_data(*tbl, NULL));
        spr.color = color;
        draw_sprite(spr, &vec);
        tbl++;
    }
    draw_sprite_ary(((u16**)get_menu_lsp_tbl(0x95))[(u8)sel], &vec);
    switch ((u8)sel) {
    case 1:
        sprintf(buf, lbl_80792BE8, get_str_tbl(0x32)[2]);
        draw_font_idx(0x811, buf, 4, &vec);
        sprintf(buf, lbl_80792BE4, 1);
        draw_font_idx(0x812, buf, 4, &vec);
        break;
    case 2:
        sprintf(buf, lbl_80792BE8, get_str_tbl(0x32)[2]);
        draw_font_idx(0x814, buf, 4, &vec);
        sprintf(buf, lbl_80792BEC, 0xF, get_str_tbl(0x32)[3]);
        draw_font_idx(0x815, buf, 4, &vec);
        break;
    case 3:
        sprintf(buf, lbl_80792BE8, get_str_tbl(0x32)[2]);
        draw_font_idx(0x81C, buf, 4, &vec);
        sprintf(buf, lbl_80792BE4, 1);
        draw_font_idx(0x81D, buf, 4, &vec);
        break;
    case 4:
        sprintf(buf, lbl_80792BE8, get_str_tbl(0x32)[2]);
        draw_font_idx(0x81F, buf, 4, &vec);
        sprintf(buf, lbl_80792BEC, 0xF, get_str_tbl(0x32)[3]);
        draw_font_idx(0x820, buf, 4, &vec);
        sprintf(buf, lbl_80792BE8, get_str_tbl(0x32)[2]);
        draw_font_idx(0x81C, buf, 4, &vec);
        sprintf(buf, lbl_80792BE4, 1);
        draw_font_idx(0x81D, buf, 4, &vec);
        break;
    }
}

/* ---- the sword/bowgun "basis" dispatch helpers, 0x8030D6A8..0x8030D980 ---- */

/* 0x8030D6A8 - the sword-detail colour entry: force the sub-index to 0 and tail into the shared
 * `fn_8030D6C0`. */
void equip_detail_bowgun_draw(_PLW* s0, _EQUIP* s1, u16 a, u8 b) {
    fn_8030D6C0(s0, s1, NULL, a, b);
}

/* 0x8030F374 - the same zero-sub-index entry for the `fn_8030F38C` band. */
void equip_detail_kind6_draw(_PLW* s0, _EQUIP* s1, u16 a, u8 b) {
    fn_8030F38C(s0, s1, 0, a, b);
}

/* 0x8030E784 - the `fn_8030E79C` band's zero-sub-index entry. */
void fn_8030E784(_PLW* s0, _EQUIP* s1, u16 a, u8 b) {
    fn_8030E79C(s0, s1, 0, a, b);
}

/* 0x8030FC40 - the `fn_8030FC58` band's zero-sub-index entry. */
void fn_8030FC40(_PLW* s0, _EQUIP* s1, u16 a, u8 b) {
    fn_8030FC58(s0, s1, 0, a, b);
}

/* 0x8030FEF4 - the `fn_8030FF0C` band's zero-sub-index entry. */
void fn_8030FEF4(_PLW* s0, _EQUIP* s1, u16 a, u8 b) {
    fn_8030FF0C(s0, s1, 0, a, b);
}

/* 0x80310548 - the `fn_80310560` band's zero-sub-index entry. */
void fn_80310548(_PLW* s0, _EQUIP* s1, u16 a, u8 b) {
    fn_80310560(s0, s1, 0, a, b);
}

/* 0x80310E40 - the `fn_80310E58` band's zero-sub-index entry. */
void fn_80310E40(_PLW* s0, _EQUIP* s1, u16 a, u8 b) {
    fn_80310E58(s0, s1, 0, a, b);
}

/* 0x80311540 - the `fn_80311560` band's zero-sub-index entry (its third argument's top two bits
 * are cleared on the way through). */
void fn_80311540(_PLW* s0, _EQUIP* s1, u16 a, u8 c, u8 d) {
    c &= ~6;
    fn_80311560(s0, s1, NULL, a, c, d);
}

/* 0x8030FB60 / 0x8030FB6C - the two fixed-kind entries into `fn_8030FB78`. */
void fn_8030FB60(u16 a) {
    fn_8030FB78(a, 6);
}

void fn_8030FB6C(u16 a) {
    fn_8030FB78(a, 7);
}

/* 0x80312DF0 - draw the sprite run `get_menu_lsp_tbl(0xCD)` names at the row `put_lsp_anchor_offset`
 * positions from the caller's value. */
void fn_80312DF0(u32* p) {
    u32 v = *p;
    _mh_ivec2_ pos;
    put_lsp_anchor_offset(0x9A5, &pos, &v);
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0xCD), &pos);
}

/* 0x8030D6C0 - the shared sword/small-blade branch: resolve the caller's variant into a stack
 * record with `equip_variant_resolve` and hand off to the dispatcher (unless it reports "done"). */
u8 fn_8030D6C0(_PLW* s0, _EQUIP* s1, _EQUIP* s2, u16 a, u8 b) {
    _EQUIP_INDEX out;
    u8 spb;
    spb = b;
    u8 r = equip_variant_resolve(s0, s1, s2, &spb, &out);
    if (r == 1) {
        return r;
    }
    return fn_8030D808(s0, &out, a, spb);
}

/* 0x8030FB78 - the bowgun/ammo row: derive the offset from the 0x8A7 anchor (or zero), then draw
 * the `get_str_tbl(0x2E)[kind]` label at entry 0x8A8. */
void fn_8030FB78(u16 a, u8 b) {
    _mh_ivec2_ pos;
    _mh_ivec2_ d;
    if (a != 0) {
        get_lsp_data(0x8A7, &pos);
        get_lsp_data(a, &d);
        d.x -= pos.x;
        d.y -= pos.y;
    } else {
        d.x = 0;
        d.y = 0;
    }
    u32 tmp = *(u32*)&d;
    put_lsp_anchor_offset(0x8A7, &pos, &tmp);
    draw_font_idx(0x8A8, (s8*)((u8**)get_str_tbl(0x2E))[b], 0, &pos);
}

/* 0x8030D808 - the weapon detail's shared body: positions the panel from the 0x84E anchor (or the caller's offset) and
 * dispatches on `fn_80315A60`'s row: one label, the caller's value, or the three bowgun panels and a text row. */
u8 fn_8030D808(_PLW* s0, _EQUIP_INDEX* s1, u16 a, u8 b) {
    _mh_ivec2_ pos;
    _mh_ivec2_ off;
    if (a != 0) {
        get_lsp_data(0x84E, &pos);
        get_lsp_data(a, &off);
        off.x -= pos.x;
        off.y -= pos.y;
    } else {
        off.x = 0;
        off.y = 0;
    }
    u32 src = *(u32*)&off;
    put_lsp_anchor_offset(0x84E, &pos, &src);
    u8 r = fn_80315A60(s0, s1, b);
    switch (r) {
    case 1:
        return r;
    case 2:
        draw_font_idx(0x8D2, (s8*)((u8**)get_str_tbl(0x2E))[1], 0, &pos);
        return r;
    }
    fn_80315274(s0, s1, b);
    Put_equip_dtl_basis_bowgun_gan1(s0, s1, 0, b, &off);
    Put_equip_dtl_basis_bowgun_gan2(s0, s1, 0, b, &off);
    Put_equip_dtl_basis_bowgun_gan_lv(s0, s1, 0, b, &off);
    fn_803153F8((EquipColumnPanel*)s0, b);
    return 0;
}

/* 0x8030D728 - the second small-weapon branch entry (resolves through `equip_variant_resolve_ex`). */
u8 put_equip_row_variant(_PLW* s0, _EQUIP* p1, _EQUIP* p2, _EQUIP* p3, u16 a, u8 b) {
    _EQUIP_INDEX out;
    u8 r = equip_variant_resolve_ex(s0, p1, p2, p3, b, &out);
    if (r == 1) {
        return r;
    }
    return fn_8030D808(s0, &out, a, b);
}

/* ---- the same resolve-then-dispatch shape, one family per weapon sub-band ----
 * Each `fn_8030F7B0`-style entry resolves the caller's variant into a stack record with
 * `equip_variant_resolve` and hands it to its band's dispatcher; `put_equip_row_variant_ex`-style resolves through
 * `equip_variant_resolve_ex` (six registers plus a slot); `fn_8030FD30`-style through `fn_80315730`, whose
 * ninth (stack) argument is the record. */

/* 0x8030F7B0 - the 0x8030F87C band's `equip_variant_resolve` entry. */
void fn_8030F7B0(_PLW* s0, _EQUIP* s1, _EQUIP* s2, u16 a, u8 b) {
    _EQUIP_INDEX out;
    u8 spb;
    spb = b;
    if (equip_variant_resolve(s0, s1, s2, &spb, &out) == 1) {
        return;
    }
    fn_8030F87C(s0, &out, a, spb);
}

/* 0x8030F814 - the `fn_8030F87C` band's variant whose variant word is forced to 0. */
void equip_detail_slots_draw(_PLW* s0, _EQUIP* s1, u16 a, u8 b) {
    _EQUIP_INDEX out;
    u8 spb;
    spb = b;
    if (equip_variant_resolve(s0, s1, 0, &spb, &out) == 1) {
        return;
    }
    fn_8030F87C(s0, &out, a, spb);
}

/* 0x8030FC58 - the 0x8030FDA0 band's `equip_variant_resolve` entry. */
u8 fn_8030FC58(_PLW* s0, _EQUIP* s1, _EQUIP* s2, u16 a, u8 b) {
    _EQUIP_INDEX out;
    u8 spb;
    spb = b;
    u8 r = equip_variant_resolve(s0, s1, s2, &spb, &out);
    if (r == 1) {
        return r;
    }
    return fn_8030FDA0(s0, &out, a, spb);
}

/* 0x8030FCC0 - the 0x8030FDA0 band's `equip_variant_resolve_ex` entry. */
u8 put_equip_row_variant_ex(_PLW* s0, _EQUIP* p1, _EQUIP* p2, _EQUIP* p3, u16 a, u8 b) {
    _EQUIP_INDEX out;
    u8 r = equip_variant_resolve_ex(s0, p1, p2, p3, b, &out);
    if (r == 1) {
        return r;
    }
    return fn_8030FDA0(s0, &out, a, b);
}

/* 0x8030FD30 - the 0x8030FDA0 band's `fn_80315730` entry. */
u8 fn_8030FD30(_PLW* s0, _EQUIP* p1, _EQUIP* p2, _EQUIP* p3, _EQUIP* p4, _EQUIP* p5, _EQUIP* p6, u16 a, u8 b) {
    _EQUIP_INDEX out;
    u8 r = fn_80315730(s0, p1, p2, p3, p4, p5, p6, &b, &out);
    if (r == 1) {
        return r;
    }
    return fn_8030FDA0(s0, &out, a, b);
}

/* 0x80310560 - the 0x803106A8 band's `equip_variant_resolve` entry. */
u8 fn_80310560(_PLW* s0, _EQUIP* s1, _EQUIP* s2, u16 a, u8 b) {
    _EQUIP_INDEX out;
    u8 spb;
    spb = b;
    u8 r = equip_variant_resolve(s0, s1, s2, &spb, &out);
    if (r == 1) {
        return r;
    }
    return fn_803106A8(s0, &out, a, spb);
}

/* 0x803105C8 - the 0x803106A8 band's `equip_variant_resolve_ex` entry. */
u8 put_equip_row_variant2(_PLW* s0, _EQUIP* p1, _EQUIP* p2, _EQUIP* p3, u16 a, u8 b) {
    _EQUIP_INDEX out;
    u8 r = equip_variant_resolve_ex(s0, p1, p2, p3, b, &out);
    if (r == 1) {
        return r;
    }
    return fn_803106A8(s0, &out, a, b);
}

/* 0x80310638 - the 0x803106A8 band's `fn_80315730` entry. */
u8 fn_80310638(_PLW* s0, _EQUIP* p1, _EQUIP* p2, _EQUIP* p3, _EQUIP* p4, _EQUIP* p5, _EQUIP* p6, u16 a, u8 b) {
    _EQUIP_INDEX out;
    u8 r = fn_80315730(s0, p1, p2, p3, p4, p5, p6, &b, &out);
    if (r == 1) {
        return r;
    }
    return fn_803106A8(s0, &out, a, b);
}

/* 0x80310E58 - the 0x80310F30 band's `equip_variant_resolve` entry. */
u8 fn_80310E58(_PLW* s0, _EQUIP* s1, _EQUIP* s2, u16 a, u8 b) {
    _EQUIP_INDEX out;
    u8 spb;
    spb = b;
    u8 r = equip_variant_resolve(s0, s1, s2, &spb, &out);
    if (r == 1) {
        return r;
    }
    return fn_80310F30(s0, &out, a, spb);
}

/* 0x80310EC0 - the 0x80310F30 band's `equip_variant_resolve_ex` entry. */
u8 fn_80310EC0(_PLW* s0, _EQUIP* p1, _EQUIP* p2, _EQUIP* p3, u16 a, u8 b) {
    _EQUIP_INDEX out;
    u8 r = equip_variant_resolve_ex(s0, p1, p2, p3, b, &out);
    if (r == 1) {
        return r;
    }
    return fn_80310F30(s0, &out, a, b);
}

/* 0x80311560 - the 0x803116D8 band's `equip_variant_resolve` entry (its resolved byte is masked before it
 * is handed on). */
void fn_80311560(_PLW* s0, _EQUIP* s1, _EQUIP* s2, u16 a, u8 b, u8 c) {
    _EQUIP_INDEX out;
    u8 spb;
    spb = b;
    if (equip_variant_resolve(s0, s1, s2, &spb, &out) == 1) {
        return;
    }
    spb = (u8)(spb & ~6);
    put_equip_panel_row_ex(s0, &out, a, spb, c);
}

/* 0x803106A8 - the 0x8031077C band's body: position the panel from the 0x8F0 anchor (or the
 * caller's offset) and hand the resulting anchor to `fn_8031077C`. */
u8 fn_803106A8(_PLW* s0, _EQUIP_INDEX* s1, u16 a, u8 b) {
    _mh_ivec2_ pos;
    _mh_ivec2_ off;
    if (a != 0) {
        get_lsp_data(0x8F0, &pos);
        get_lsp_data(a, &off);
        off.x -= pos.x;
        off.y -= pos.y;
    } else {
        off.x = 0;
        off.y = 0;
    }
    u32 src = *(u32*)&off;
    put_lsp_anchor_offset(0x8F0, &pos, &src);
    u32 copy = *(u32*)&pos;
    return fn_8031077C(s0, s1, b, (_mh_ivec2_*)&copy);
}

/* ---------------------------------------------------------------------------------------------------
 * 0x80313E24..0x8031A638 - the second half: the equipment columns, the slot blocks and the panel's
 * own cursor (13 bodies, the rest are placeholders - see the header's residual list)
 * --------------------------------------------------------------------------------------------------- */

extern u8 equip_panel_row_layout[22][3];

/* 0x80313E24 (0x4A4): Draws the equipment panel of one kind: for each of its eleven rows the row frame, every
 * column cell of each of the three shown pieces (resolved one by one), then the totals cell and the row label. */
void fn_80313E24(_PLW* plw, _EQUIP_INDEX* view, u8 flags, u8 kind, const _mh_ivec2_* pos) {
    _EQUIP_INDEX resolved;
    u8 count;
    u8 piece_flags;
    u8 ok;
    u8 row;
    u8 piece;

    count = 0;
    for (row = 0; row < 11; row++) {
        u32 frame_pos = *(const u32*)pos;
        fn_803142C8(kind, row, (const _mh_ivec2_*)&frame_pos, 0, flags);
        u8* cols = equip_panel_row_layout[(u8)(row + kind * 11)];
        for (piece = 0; piece < 3; piece++) {
            piece_flags = flags & ~6;
            switch (piece) {
            case 0:
                ok = equip_variant_resolve(plw, view->shown[0], NULL, &piece_flags, &resolved);
                break;
            case 1:
                ok = equip_variant_resolve(plw, view->shown[1], NULL, &piece_flags, &resolved);
                break;
            case 2:
                ok = equip_variant_resolve(plw, view->shown[2], NULL, &piece_flags, &resolved);
                break;
            }
            if (ok == 1) {
#line 6824
                nw4r::db::Panic("menu_infomation.cpp", __LINE__, "NW4R:Failed assertion 0");
            } else {
                u8 col = cols[0];
                if (col != 0xFF) {
                    u32 better = (fn_80318E08(plw, &resolved, col, piece_flags) & 1) != 0;
                    u8 state = fn_80318A10(plw, &resolved, col, piece_flags, &count);
                    u32 at = *(const u32*)pos;
                    fn_803144A0(piece, row, 0, (const _mh_ivec2_*)&at, count, state, better, piece_flags);
                }
                col = cols[1];
                if (col != 0xFF) {
                    u32 better = (fn_80318E08(plw, &resolved, col, piece_flags) & 1) != 0;
                    u8 state = fn_80318A10(plw, &resolved, col, piece_flags, &count);
                    u32 at = *(const u32*)pos;
                    fn_803144A0(piece, row, 1, (const _mh_ivec2_*)&at, count, state, better, piece_flags);
                }
                col = cols[2];
                if (col != 0xFF) {
                    u32 better = (fn_80318E08(plw, &resolved, col, piece_flags) & 1) != 0;
                    u8 state = fn_80318A10(plw, &resolved, col, piece_flags, &count);
                    u32 at = *(const u32*)pos;
                    fn_803144A0(piece, row, 2, (const _mh_ivec2_*)&at, count, state, better, piece_flags);
                }
            }
        }
        u8 total_state = 0;
        u8 total = 0;
        u8 col = cols[0];
        if (col != 0xFF) {
            u32 better = (fn_80318E08(plw, view, col, flags) & 1) != 0;
            total_state = fn_80318A10(plw, view, col, flags, &count);
            u32 at = *(const u32*)pos;
            fn_803144A0(piece, row, 0, (const _mh_ivec2_*)&at, count, total_state, better, flags);
            total = count;
        }
        col = cols[1];
        if (col != 0xFF) {
            u32 better = (fn_80318E08(plw, view, col, flags) & 1) != 0;
            u8 state = fn_80318A10(plw, view, col, flags, &count);
            u32 at = *(const u32*)pos;
            fn_803144A0(piece, row, 1, (const _mh_ivec2_*)&at, count, state, better, flags);
            total_state |= state;
            total += count;
        }
        col = cols[2];
        if (col != 0xFF) {
            u32 better = (fn_80318E08(plw, view, col, flags) & 1) != 0;
            u8 state = fn_80318A10(plw, view, col, flags, &count);
            u32 at = *(const u32*)pos;
            fn_803144A0(piece, row, 2, (const _mh_ivec2_*)&at, count, state, better, flags);
            total_state |= state;
            total += count;
        }
        if (total == 0) {
            total_state |= 0x20;
        }
        u32 label_pos = *(const u32*)pos;
        fn_80314340(kind, row, (const _mh_ivec2_*)&label_pos, total_state, flags);
    }
}

/* 0x803142C8 - draw the base/selected run for one slot: resolve the `0x9AA` anchor against the
 * caller's position, then the `0xD0`/`0xD2` sprite lists at it. */
void fn_803142C8(u8 kind, u8 idx, const _mh_ivec2_* pos, u8 state, u8 mode) {
    u32 out1;
    u32 out2;
    u32 in = *(const u32*)pos;   /* the 4-byte `_mh_ivec2_` block, copied whole as the target does */
    u32 tmp;
    put_lsp_anchor_offset(0x9AA, &out1, &in);
    u16* tbl = (u16*)get_menu_lsp_tbl(0xD0);
    tmp = out1;
    put_lsp_anchor_offset(tbl[idx], &out2, &tmp);
    u16* tbl2 = (u16*)get_menu_lsp_tbl(0xD2);
    draw_sprite_ary(tbl2, (const _mh_ivec2_*)&out2);
}

/* 0x80314340 (0x160): Draws the slot's label from string table 0x3B or 0x3C at the `0xD0` row of the 0x9AA anchor,
 * green while the mode's bit 2 and a 0x18 state bit are set and grey for state bit 0x20. */
void fn_80314340(u8 kind, u8 idx, const _mh_ivec2_* pos, u8 state, u8 mode) {
    u32 base;
    u32 at;
    u32 in = *(const u32*)pos;
    u32 tmp;
    _SPR_DATA_ spr;
    u8** strs;

    put_lsp_anchor_offset(0x9AA, &base, &in);
    u16* tbl = (u16*)get_menu_lsp_tbl(0xD0);
    tmp = base;
    put_lsp_anchor_offset(tbl[idx], &at, &tmp);
    switch (kind) {
    case 0:
        strs = get_str_tbl(0x3B);
        break;
    case 1:
        strs = get_str_tbl(0x3C);
        break;
    default:
#line 6956
        nw4r::db::Panic("menu_infomation.cpp", __LINE__, "NW4R:Failed assertion 0");
        return;
    }
    spr_data_copy(&spr, get_lsp_data(0x9E3, NULL));
    if ((mode & 4) && (state & 0x18)) {
        spr.color = 0x50CD46FF;
    }
    if (state & 0x20) {
        spr.color = 0x878787FF;
    }
    draw_font(spr, (s8*)strs[idx], 4, (const _mh_ivec2_*)&at);
}

/* 0x803144A0 (0x164): Draws one slot cell: the `0xD3` icon at the `0xD6`/`0xD5` row and column anchors (tinted when
 * selected) and, for a non-zero value, the `0xD4` number sprite with the value printed in it. */
void fn_803144A0(u8 row, u8 col, u8 icon, const _mh_ivec2_* pos, u8 value, u8 state, u32 selected, u8 mode) {
    u32 base;
    u32 at;
    u32 in;
    u32 tmp;
    s8 buf[16];
    _SPR_DATA_ spr;

    u16* rows = (u16*)get_menu_lsp_tbl(0xD6);
    in = *(const u32*)pos;
    put_lsp_anchor_offset(rows[row], &base, &in);
    u16* cols = (u16*)get_menu_lsp_tbl(0xD5);
    tmp = base;
    put_lsp_anchor_offset(cols[col], &at, &tmp);
    u16* icons = (u16*)get_menu_lsp_tbl(0xD3);
    spr_data_copy(&spr, get_lsp_data(icons[icon], NULL));
    if (selected == 1) {
        spr.color = 0xAA825A7D;
    }
    draw_sprite(spr, (const _mh_ivec2_*)&at);
    if (value != 0) {
        spr_data_copy(&spr, get_lsp_data(((u16*)get_menu_lsp_tbl(0xD4))[icon], NULL));
        if ((mode & 4) && (state & 0x18)) {
            spr.color = 0x50CD46FF;
        }
        sprintf(buf, lbl_80792BE4, value);
        draw_font(spr, buf, 5, (const _mh_ivec2_*)&at);
    }
}

/* 0x80314604 - the next/previous slot step: the bowgun category (`Pl_Skill_slot_item_get(plw,0xB) == 0x17`)
 * clamps one higher than the rest. */
u8 fn_80314604(_PLW* plw, s8 slot, u32 confirm) {
    u8 bowgun = 0;
    if (confirm == 1 && Pl_Skill_slot_item_get(plw, 0xB) == 0x17) {
        bowgun = 1;
    }
    if (bowgun == 1) {
        slot++;
        if (slot > 6) {
            slot = 6;
        }
    } else {
        if (slot > 5) {
            slot = 5;
        }
    }
    return (u8)slot;
}

/* 0x8031468C */
void fn_8031468C(void) {}

/* 0x803149B8 - route a slot selection to the two value-pair draws; kinds 1/2/5/6 use pair A and
 * 3/4/7/8 use pair B, anything else yields 0. */
u32 fn_803149B8(u16 a, u32* b, u8 sel, u32* out) {
    u32 v0 = b[0];
    u32 v1 = b[1];
    u32 v2 = b[2];
    u8 kind = fn_803159CC(sel);
    switch (kind) {
    case 1:
    case 2:
    case 5:
    case 6:
        return fn_80314A64(a, v0, 0, out);
    case 3:
    case 4:
    case 7:
    case 8:
        return fn_80314A64(a, v1, v2, out);
    }
    return 0;
}

/* 0x80314A64 */
u32 fn_80314A64(u16 a, u32 b, u32 c, u32* out) { return 0; }

/* 0x80314F18 - sum the equipment "category weight" of two pieces: the second only counts when the
 * first is a weapon (kind 0xC) and the second is its matching form (kind 0xD). */
u8 fn_80314F18(_EQUIP* a, _EQUIP* b) {
    u8 total = fn_8027F11C(a);
    if (a->kind == 0xC && b != NULL && b->kind == 0xD) {
        total = (u8)(total + fn_8027F11C(b));
    }
    return total;
}

/* 0x80314F90 */
s32 fn_80314F90(_EQUIP* equip, _EQUIP* sub, u16* ids) { return 0; }

/* 0x80315274 */
void fn_80315274(_PLW* plw, _EQUIP_INDEX* view, u8 flags) {}

/* 0x803153F8 - clear the eight column entries of a hunter's panel. */
void fn_803153F8(struct EquipColumnPanel* self, u8 unused) {
    self->field_0x61A[0] = 0;
    self->field_0x62A[0] = 0;
    self->field_0x61A[1] = 0;
    self->field_0x62A[1] = 0;
    self->field_0x61A[2] = 0;
    self->field_0x62A[2] = 0;
    self->field_0x61A[3] = 0;
    self->field_0x62A[3] = 0;
    self->field_0x61A[4] = 0;
    self->field_0x62A[4] = 0;
    self->field_0x61A[5] = 0;
    self->field_0x62A[5] = 0;
    self->field_0x61A[6] = 0;
    self->field_0x62A[6] = 0;
    self->field_0x61A[7] = 0;
    self->field_0x62A[7] = 0;
}

/* 0x80315440 (0x17C): Resolves one piece into the view: the shown piece is the piece itself (flag bit 0), the
 * caller's second piece or the worn record of its kind, and the worn column is the second piece or that record. */
u8 equip_variant_resolve(_PLW* plw, _EQUIP* equip, _EQUIP* other, u8* flags, _EQUIP_INDEX* view) {
    u8 ret = 0;
    _EQUIP* worn;

    view->shown[0] = NULL;
    view->shown[1] = NULL;
    view->shown[2] = NULL;
    view->worn[0] = NULL;
    view->worn[1] = NULL;
    view->worn[2] = NULL;
    if (plw == NULL) {
        return 1;
    }
    if (equip == NULL) {
        return 1;
    }
    *flags |= 0x20;
    switch (equip->kind) {
    case 11:
        worn = &plw->equipB;
        break;
    case 12:
        if (plw->equipB.kind == 0xB) {
            worn = &plw->equipC;
        } else {
            worn = &plw->equipB;
        }
        break;
    case 13:
        if (plw->equipB.kind == 0xB) {
            worn = &plw->equipD;
        } else {
            worn = &plw->equipB;
        }
        break;
    case 1:
        worn = &plw->equipA[0];
        break;
    case 2:
        worn = &plw->equipA[1];
        break;
    case 3:
        worn = &plw->equipA[2];
        break;
    case 4:
        worn = &plw->equipA[3];
        break;
    case 5:
        worn = &plw->equipA[4];
        break;
    case 6:
        worn = &plw->equipA[5];
        break;
    case 7:
    case 8:
    case 9:
    case 10:
    case 14:
    case 15:
        worn = &plw->equipB;
        break;
    default:
        ret = 1;
        break;
    }
    if (*flags & 1) {
        view->shown[0] = equip;
        if (worn->item_id == 0 && other == NULL) {
            *flags &= ~8;
        }
    } else if (other != NULL) {
        view->shown[0] = other;
    } else {
        view->shown[0] = worn;
    }
    if (other != NULL) {
        view->worn[0] = other;
        if (other->item_id != 0) {
            *flags |= 0x10;
        } else {
            *flags &= ~8;
        }
    } else {
        view->worn[0] = worn;
    }
    return ret;
}

/* 0x803155BC (0x174): Resolves a bowgun triple into the view: with flag bit 0 the caller's frame/barrel/stock
 * (a lone part completed from the worn set), otherwise the worn set; the worn column is always the worn set. */
u8 equip_variant_resolve_ex(_PLW* plw, _EQUIP* frame, _EQUIP* barrel, _EQUIP* stock, u8 flags,
                            _EQUIP_INDEX* view) {
    u8 ret = 0;

    view->shown[0] = NULL;
    view->shown[1] = NULL;
    view->shown[2] = NULL;
    view->worn[0] = NULL;
    view->worn[1] = NULL;
    view->worn[2] = NULL;
    if (plw == NULL) {
        ret = 1;
    } else if (flags & 1) {
        if (frame == NULL) {
            ret = 1;
        } else if (barrel == NULL && stock == NULL) {
            switch (frame->kind) {
            case 11:
                view->shown[0] = frame;
                view->shown[1] = &plw->equipC;
                view->shown[2] = &plw->equipD;
                break;
            case 12:
                view->shown[0] = &plw->equipB;
                view->shown[1] = frame;
                view->shown[2] = &plw->equipD;
                break;
            case 13:
                view->shown[0] = &plw->equipB;
                view->shown[1] = &plw->equipC;
                view->shown[2] = frame;
                break;
            default:
                ret = 1;
                break;
            }
        } else {
            if (frame->kind == 0xB) {
                view->shown[0] = frame;
            } else {
                ret = 1;
            }
            if (barrel != NULL) {
                if (barrel->kind == 0xC) {
                    view->shown[1] = barrel;
                } else {
                    ret = 1;
                }
            } else {
                ret = 1;
            }
            if (stock != NULL) {
                if (stock->kind == 0xD) {
                    view->shown[2] = stock;
                } else {
                    ret = 1;
                }
            } else {
                ret = 1;
            }
        }
    } else {
        view->shown[0] = &plw->equipB;
        view->shown[1] = &plw->equipC;
        view->shown[2] = &plw->equipD;
    }
    view->worn[0] = &plw->equipB;
    view->worn[1] = &plw->equipC;
    view->worn[2] = &plw->equipD;
    return ret;
}

/* 0x80315730 (0x29C): Resolves a bowgun triple against a second triple: the shown column as the 0x803155BC
 * resolver picks it (the second triple when flag bit 0 is clear and it is complete), the worn column from the
 * second triple when it is a whole frame/barrel/stock set, else the worn set; flag bit 3 is cleared when the worn
 * column is not a whole, filled set. */
u8 fn_80315730(_PLW* plw, _EQUIP* frame, _EQUIP* barrel, _EQUIP* stock, _EQUIP* other_frame,
               _EQUIP* other_barrel, _EQUIP* other_stock, u8* flags, _EQUIP_INDEX* view) {
    u8 ret = 0;

    view->shown[0] = NULL;
    view->shown[1] = NULL;
    view->shown[2] = NULL;
    view->worn[0] = NULL;
    view->worn[1] = NULL;
    view->worn[2] = NULL;
    if (plw == NULL) {
        ret = 1;
    } else if (*flags & 1) {
        if (frame == NULL) {
            ret = 1;
        } else if (barrel == NULL && stock == NULL) {
            switch (frame->kind) {
            case 11:
                view->shown[0] = frame;
                view->shown[1] = &plw->equipC;
                view->shown[2] = &plw->equipD;
                break;
            case 12:
                view->shown[0] = &plw->equipB;
                view->shown[1] = frame;
                view->shown[2] = &plw->equipD;
                break;
            case 13:
                view->shown[0] = &plw->equipB;
                view->shown[1] = &plw->equipC;
                view->shown[2] = frame;
                break;
            default:
                ret = 1;
                break;
            }
        } else {
            if (frame->kind == 0xB) {
                view->shown[0] = frame;
            } else {
                ret = 1;
            }
            if (barrel != NULL) {
                if (barrel->kind == 0xC) {
                    view->shown[1] = barrel;
                } else {
                    ret = 1;
                }
            } else {
                ret = 1;
            }
            if (stock != NULL) {
                if (stock->kind == 0xD) {
                    view->shown[2] = stock;
                } else {
                    ret = 1;
                }
            } else {
                ret = 1;
            }
        }
    } else if (other_frame != NULL && other_barrel != NULL && other_stock != NULL) {
        view->shown[0] = other_frame;
        view->shown[1] = other_barrel;
        view->shown[2] = other_stock;
    } else {
        view->shown[0] = &plw->equipB;
        view->shown[1] = &plw->equipC;
        view->shown[2] = &plw->equipD;
    }
    if (other_frame != NULL && other_barrel != NULL && other_stock != NULL) {
        if (other_frame->kind == 0xB && other_barrel->kind == 0xC && other_stock->kind == 0xD) {
            view->worn[0] = other_frame;
            view->worn[1] = other_barrel;
            view->worn[2] = other_stock;
        } else {
            *flags &= ~8;
            view->worn[0] = &plw->equipB;
            view->worn[1] = &plw->equipC;
            view->worn[2] = &plw->equipD;
        }
    } else if (plw->equipB.kind == 0xB && plw->equipC.kind == 0xC && plw->equipD.kind == 0xD) {
        view->worn[0] = &plw->equipB;
        view->worn[1] = &plw->equipC;
        view->worn[2] = &plw->equipD;
    } else {
        *flags &= ~8;
        view->worn[0] = &plw->equipB;
        view->worn[1] = &plw->equipC;
        view->worn[2] = &plw->equipD;
    }
    if (view->worn[0]->item_id == 0 || view->worn[1]->item_id == 0 || view->worn[2]->item_id == 0) {
        *flags &= ~8;
    }
    return ret;
}

/* 0x803159CC - unpack the three-bit equipment class from a flags byte: bit 0 selects the high/low
 * group, bit 5 the sub-group within it, bit 1 the last choice. */
u8 fn_803159CC(u8 flags) {
    u8 ret;
    if (flags & 1) {
        if (flags & 0x20) {
            if (flags & 2) {
                ret = 1;
            } else {
                ret = 2;
            }
        } else {
            if (flags & 2) {
                ret = 3;
            } else {
                ret = 4;
            }
        }
    } else {
        if (flags & 0x20) {
            if (flags & 2) {
                ret = 5;
            } else {
                ret = 6;
            }
        } else {
            if (flags & 2) {
                ret = 7;
            } else {
                ret = 8;
            }
        }
    }
    return ret;
}

/* 0x80315A60 */
u8 fn_80315A60(_PLW* plw, _EQUIP_INDEX* view, u8 flags) { return 0; }

/* 0x80315C00 */
void fn_80315C00(void) {}

/* 0x80315D0C */
void fn_80315D0C(void) {}

/* 0x80315E34 */
void fn_80315E34(void) {}

/* 0x80316058 */
void fn_80316058(void) {}

/* 0x8031622C (0x2A4): Reports the element A of the shown piece's weapon record: its type (biased by 6) and its
 * value times ten; a hidden (negative) value counts only with skill 0xC9, and bits 1/2 compare it with the worn one. */
u8 fn_8031622C(_PLW* plw, _EQUIP_INDEX* view, u8 flags, u8* out_type, s16* out_value) {
    _EQUIP* worn = view->worn[0];
    _EQUIP* shown = view->shown[0];
    u8 ret = 0;
    s32 hidden = 0;
    u8 type = 0;
    s16 value = 0;
    u8 worn_type = 0;
    s16 worn_value = 0;
    PlSkillRec* rec;
    u32 skill;

    *out_type = 0;
    *out_value = 0;
    skill = 0;
    if ((flags & 2) && (flags & 4) && Pl_Skill_ck(plw, 0xC9) == 1) {
        skill = 1;
    }
    u8 kind = fn_803159CC(flags);
    switch (kind) {
    case 1:
    case 2:
        if ((u8)(worn->kind + 0xF5) <= 2) {
            worn_type = 0;
            worn_value = 0;
        } else {
            rec = fn_8027ECAC(worn);
            if (rec != NULL && rec->element_a_type != 0) {
                if (rec->element_a_value > 0) {
                    worn_type = rec->element_a_type + 6;
                    worn_value = rec->element_a_value;
                } else if (skill == 1) {
                    worn_type = rec->element_a_type + 6;
                    worn_value = rec->element_a_value - rec->element_a_value * 2;
                }
            }
        }
        rec = fn_8027ECAC(shown);
        if (rec != NULL && rec->element_a_type != 0) {
            if (rec->element_a_value > 0) {
                type = rec->element_a_type + 6;
                value = rec->element_a_value;
                ret |= 1;
            } else if (skill == 1) {
                type = rec->element_a_type + 6;
                value = rec->element_a_value - rec->element_a_value * 2;
                hidden = 1;
                ret |= 1;
            }
        }
        break;
    case 5:
    case 6:
        rec = fn_8027ECAC(shown);
        if (rec != NULL && rec->element_a_type != 0) {
            if (rec->element_a_value > 0) {
                type = rec->element_a_type + 6;
                value = rec->element_a_value;
                ret |= 1;
            } else if (skill == 1) {
                type = rec->element_a_type + 6;
                value = rec->element_a_value - rec->element_a_value * 2;
                hidden = 1;
                ret |= 1;
            }
        }
        break;
    }
    worn_value *= 10;
    value *= 10;
    if (ret & 1) {
        if ((flags & 8) && (flags & 1) && worn_type != 0 && worn_type == type && worn_value != value) {
            ret |= (worn_value < value) ? 2 : 4;
        }
        if (flags & 4) {
            switch (hidden) {
            case 1:
                ret |= 8;
                break;
            case 2:
                ret |= 0x10;
                break;
            }
        }
        *out_type = type;
        *out_value = value;
    }
    return ret;
}

/* 0x803164D0 (0x2B0): Reports the element B of the shown piece's weapon record: its type (biased by 11) and its
 * value times ten; a hidden (negative) value counts only with skill 0xC9, and bits 1/2 compare it with the worn one. */
u8 fn_803164D0(_PLW* plw, _EQUIP_INDEX* view, u8 flags, u8* out_type, s16* out_value) {
    _EQUIP* worn = view->worn[0];
    _EQUIP* shown = view->shown[0];
    u8 ret = 0;
    s32 hidden = 0;
    u8 type = 0;
    s16 value = 0;
    u8 worn_type = 0;
    s16 worn_value = 0;
    PlSkillRec* rec;
    u32 skill;

    *out_type = 0;
    *out_value = 0;
    skill = 0;
    if ((flags & 2) && (flags & 4) && Pl_Skill_ck(plw, 0xC9) == 1) {
        skill = 1;
    }
    u8 kind = fn_803159CC(flags);
    switch (kind) {
    case 1:
    case 2:
        if ((u8)(worn->kind + 0xF5) <= 2) {
            worn_type = 0;
            worn_value = 0;
        } else {
            rec = fn_8027ECAC(worn);
            if (rec != NULL && rec->element_b_type != 0) {
                if (rec->element_b_value > 0) {
                    worn_type = rec->element_b_type + 11;
                    worn_value = rec->element_b_value;
                } else if (skill == 1) {
                    worn_type = rec->element_b_type + 11;
                    worn_value = rec->element_b_value - rec->element_b_value * 2;
                }
            }
        }
        rec = fn_8027ECAC(shown);
        if (rec != NULL && rec->element_b_type != 0) {
            if (rec->element_b_value > 0) {
                type = rec->element_b_type + 11;
                value = rec->element_b_value;
                ret |= 1;
            } else if (skill == 1) {
                type = rec->element_b_type + 11;
                value = rec->element_b_value - rec->element_b_value * 2;
                hidden = 1;
                ret |= 1;
            }
        }
        break;
    case 5:
    case 6:
        rec = fn_8027ECAC(shown);
        if (rec != NULL && rec->element_b_type != 0) {
            if (rec->element_b_value > 0) {
                type = rec->element_b_type + 11;
                value = rec->element_b_value;
                ret |= 1;
            } else if (skill == 1) {
                type = rec->element_b_type + 11;
                value = rec->element_b_value - rec->element_b_value * 2;
                hidden = 1;
                ret |= 1;
            }
        }
        break;
    default:
        return 0;
    }
    worn_value *= 10;
    value *= 10;
    if (ret & 1) {
        if ((flags & 8) && (flags & 1) && worn_type != 0 && worn_type == type && worn_value != value) {
            ret |= (worn_value < value) ? 2 : 4;
        }
        if (flags & 4) {
            switch (hidden) {
            case 1:
                ret |= 8;
                break;
            case 2:
                ret |= 0x10;
                break;
            }
        }
        *out_type = type;
        *out_value = value;
    }
    return ret;
}

/* 0x80316780 */
void fn_80316780(void) {}

/* 0x80316978 - read the class byte at +0x0C of the `fn_8027ECAC` sub-record for the classes that
 * carry it (1/2/5/6); returns bit 0 set when the byte was written. */
u32 fn_80316978(u32 unused, struct _EQUIP** slot, u8 sel, u8* out) {
    u32 ret = 0;
    u8 v = 0;
    u8 kind = fn_803159CC(sel);
    if ((u32)(kind - 1) <= 1 || (u32)(kind - 5) <= 1) {
        PlSkillRec* q = fn_8027ECAC(slot[0]);
        if (q != 0 && q->slot_count != 0) {
            v = q->slot_count;
            ret |= 1;
        }
    }
    *out = v;
    return ret;
}

/* 0x80316A14 */
void fn_80316A14(void) {}

/* 0x80316DDC */
void fn_80316DDC(void) {}

/* 0x803170B4 */
void fn_803170B4(void) {}

/* 0x8031729C (0x128): For the bowgun classes (3/4/7/8) with a whole frame/barrel/stock view, stores the
 * `Get_pl_type` of the frame and copies of the barrel and stock; returns bit 0 set when it stored one. */
u8 fn_8031729C(u32 unused, _EQUIP_INDEX* view, u8 sel, u8* out) {
    _EQUIP* frame = view->shown[0];
    _EQUIP* barrel = view->shown[1];
    _EQUIP* stock = view->shown[2];
    u8 ret = 0;
    u8 type = 0;

    switch (fn_803159CC(sel)) {
    case 3:
    case 4:
        if (frame->kind == 0xB && barrel->kind == 0xC && stock->kind == 0xD) {
            _EQUIP parts[2];
            equip_record_copy(&parts[0], barrel);
            equip_record_copy(&parts[1], stock);
            type = Get_pl_type(frame, parts);
            ret |= 1;
        }
        break;
    case 7:
    case 8:
        if (frame->kind == 0xB && barrel->kind == 0xC && stock->kind == 0xD) {
            _EQUIP parts[2];
            equip_record_copy(&parts[0], barrel);
            equip_record_copy(&parts[1], stock);
            type = Get_pl_type(frame, parts);
            ret |= 1;
        }
        break;
    }
    if (ret & 1) {
        *out = type;
    }
    return ret;
}

/* 0x803173C4 */
void fn_803173C4(void) {}

/* 0x80317788 */
void fn_80317788(void) {}

/* 0x80317B84 */
void fn_80317B84(void) {}

/* 0x80317F94 */
void fn_80317F94(void) {}

/* 0x80318204 - map a packed equipment kind to its two display indices; 0 and 1 are not
 * representable and yield 0. */
u8 fn_80318204(u8 kind, u8* out1, u8* out2) {
    *out1 = 0;
    *out2 = 0;
    switch (kind) {
    case 0:
        *out1 = 0;
        break;
    case 1:
        *out1 = 1;
        *out2 = 1;
        break;
    case 2:
        *out1 = 2;
        *out2 = 1;
        break;
    case 3:
        *out1 = 1;
        *out2 = 2;
        break;
    case 4:
        *out1 = 2;
        *out2 = 2;
        break;
    case 5:
        *out1 = 1;
        *out2 = 3;
        break;
    case 6:
        *out1 = 2;
        *out2 = 3;
        break;
    default:
        return 0;
    }
    return 1;
}

/* 0x803182D8 */
void fn_803182D8(void) {}

/* 0x80318790 - true when the slot's class carries a filled "option" record: kinds 1/2/5/6 test
 * one item, 3/4/7/8 test the primary/secondary/tertiary items in turn. */
u8 fn_80318790(u32 unused, u8** b, u8 sel) {
    u8* p = b[0];
    u8* q = b[1];
    u8* r = b[2];
    u8 ret = 0;
    u8 kind = fn_803159CC(sel);
    switch (kind) {
    case 1:
    case 2:
    case 5:
    case 6:
        if ((u8)(p[0] + 0xF5) <= 2) {
            if (fn_8027FFFC((struct _EQUIP*)p) == 1) {
                ret |= 1;
            }
        }
        break;
    case 3:
    case 4:
    case 7:
    case 8:
        if (p[0] == 0xB) {
            if (fn_8027FFFC((struct _EQUIP*)p) == 1) {
                ret |= 1;
            }
        }
        if (q[0] == 0xC) {
            if (fn_8027FFFC((struct _EQUIP*)q) == 1) {
                ret |= 1;
            }
        }
        if (r[0] == 0xD) {
            if (fn_8027FFFC((struct _EQUIP*)r) == 1) {
                ret |= 1;
            }
        }
        break;
    default:
        break;
    }
    return ret;
}

/* 0x803188AC */
void fn_803188AC(void) {}

/* 0x80318A10 */
u8 fn_80318A10(_PLW* plw, _EQUIP_INDEX* view, u8 col, u8 flags, u8* count) { return 0; }

/* 0x80318E08 */
u8 fn_80318E08(_PLW* plw, _EQUIP_INDEX* view, u8 col, u8 flags) { return 0; }

/* 0x80318FB0 */
void fn_80318FB0(void) {}

/* 0x80319178 - pack the active and the next non-empty slot id (and their two colour bytes) into
 * the caller's arrays; returns how many were written. */
u8 fn_80319178(struct EquipSlotInfo* info, u16* ids, s8* colors) {
    u8 n = 0;
    u8 idx = info->idx;
    if (idx < 3) {
        if (info->slots[idx] != 0) {
            ids[0] = info->slots[idx];
            colors[0] = (s8)((u8)info->flags - 0xA);
            n = 1;
        }
    }
    idx++;
    if (idx < 3) {
        u16 v = info->slots[idx];
        if (v != 0) {
            ids[n] = v;
            colors[n] = (s8)(((info->flags >> 8) & 0xFF) - 0xA);
            n++;
        }
    }
    return n;
}

/* 0x80319204 */
void fn_80319204(void) {}

/* 0x8031949C */
void fn_8031949C(void) {}

/* 0x8031994C (0x5CC): Lists the shown piece's five slot ids into `ids` (bit 0 of `marks` per filled slot); for the
 * classes compared against the worn set, bit 3 marks an id the worn set does not carry.  Returns the slot count. */
s32 fn_8031994C(_PLW* plw, _EQUIP_INDEX* view, u8 flags, u16* ids, u8* marks) {
    _EQUIP* shown_frame = view->shown[0];
    _EQUIP* shown_barrel = view->shown[1];
    _EQUIP* shown_stock = view->shown[2];
    _EQUIP* worn_frame = view->worn[0];
    _EQUIP* worn_barrel = view->worn[1];
    _EQUIP* worn_stock = view->worn[2];
    s32 ret = 0;
    u16 worn[5];
    u16 shown[5];
    u32 i;
    u32 j;
    u8 used;
    u32 found;

    for (i = 0; i < 5; i++) {
        ids[i] = 0;
        marks[i] = 0;
        shown[i] = 0;
        worn[i] = 0;
    }
    switch (fn_803159CC(flags)) {
    case 1:
    case 2:
        fn_80314F90(worn_frame, NULL, worn);
        ret = fn_80314F90(shown_frame, NULL, shown);
        used = 0;
        for (i = 0; i < 5; i++) {
            if (shown[i] != 0) {
                ids[i] = shown[i];
                marks[i] |= 1;
                if ((flags & 8) && (flags & 1)) {
                    found = 0;
                    for (j = 0; j < 5; j++) {
                        if (shown[i] == worn[j] && !(used & (1 << j))) {
                            used |= 1 << j;
                            found = 1;
                        }
                    }
                    if (found == 0) {
                        marks[i] |= 8;
                    }
                }
            }
        }
        break;
    case 3:
    case 4:
        if (shown_frame->kind == 0xB && shown_barrel->kind == 0xC && shown_stock->kind == 0xD) {
            if (worn_frame->kind == 0xB && worn_barrel->kind == 0xC && worn_stock->kind == 0xD) {
                fn_80314F90(worn_barrel, worn_stock, worn);
            } else {
                fn_80314F90(worn_frame, NULL, worn);
            }
            ret = fn_80314F90(shown_barrel, shown_stock, shown);
            used = 0;
            for (i = 0; i < 5; i++) {
                if (shown[i] != 0) {
                    ids[i] = shown[i];
                    marks[i] |= 1;
                    if ((flags & 8) && (flags & 1)) {
                        found = 0;
                        for (j = 0; j < 5; j++) {
                            if (shown[i] == worn[j] && !(used & (1 << j))) {
                                used |= 1 << j;
                                found = 1;
                            }
                        }
                        if (found == 0) {
                            marks[i] |= 8;
                        }
                    }
                }
            }
        }
        break;
    case 5:
    case 6:
        ret = fn_80314F90(shown_frame, NULL, shown);
        for (i = 0; i < 5; i++) {
            if (shown[i] != 0) {
                ids[i] = shown[i];
                marks[i] |= 1;
            }
        }
        break;
    case 7:
    case 8:
        if (shown_frame->kind == 0xB && shown_barrel->kind == 0xC && shown_stock->kind == 0xD) {
            ret = fn_80314F90(shown_barrel, shown_stock, shown);
            for (i = 0; i < 5; i++) {
                if (shown[i] != 0) {
                    ids[i] = shown[i];
                    marks[i] |= 1;
                }
            }
        }
        break;
    }
    return ret;
}

/* 0x80319F18 */
void fn_80319F18(void) {}

/* 0x8031A244 (0x1CC): Copies the hunter's nine worn pieces into the column array, then overwrites the column the
 * view's first shown piece belongs to (the bowgun's barrel and stock too); returns 0 for an unknown kind. */
u8 Set_equip_column_arrangement(_PLW* plw, _EQUIP_INDEX* index, _EQUIP* equip) {
    equip_record_copy(&equip[0], &plw->equipA[0]);
    equip_record_copy(&equip[1], &plw->equipA[1]);
    equip_record_copy(&equip[2], &plw->equipA[2]);
    equip_record_copy(&equip[3], &plw->equipA[3]);
    equip_record_copy(&equip[4], &plw->equipA[4]);
    equip_record_copy(&equip[5], &plw->equipA[5]);
    equip_record_copy(&equip[6], &plw->equipB);
    equip_record_copy(&equip[7], &plw->equipC);
    equip_record_copy(&equip[8], &plw->equipD);
    if (index == NULL) {
        return 1;
    }
    _EQUIP* frame = index->shown[0];
    _EQUIP* barrel = index->shown[1];
    _EQUIP* stock = index->shown[2];
    switch (frame->kind) {
    case 7:
    case 8:
    case 9:
    case 10:
    case 14:
    case 15:
        equip_record_copy(&equip[6], frame);
        break;
    case 11:
        equip_record_copy(&equip[6], frame);
        if (barrel != NULL) {
            if (barrel->kind == 0xC) {
                equip_record_copy(&equip[7], barrel);
            }
            if (stock != NULL && stock->kind == 0xD) {
                equip_record_copy(&equip[8], stock);
            }
        }
        break;
    case 12:
        equip_record_copy(&equip[7], frame);
        break;
    case 13:
        equip_record_copy(&equip[8], frame);
        break;
    case 1:
        equip_record_copy(&equip[0], frame);
        break;
    case 2:
        equip_record_copy(&equip[1], frame);
        break;
    case 3:
        equip_record_copy(&equip[2], frame);
        break;
    case 4:
        equip_record_copy(&equip[3], frame);
        break;
    case 5:
        equip_record_copy(&equip[4], frame);
        break;
    case 6:
        equip_record_copy(&equip[5], frame);
        break;
    default:
#line 10920
        nw4r::db::Panic("menu_infomation.cpp", __LINE__, "NW4R:Failed assertion 0");
        return 0;
    }
    return 1;
}

/* 0x8031A410 (0x18): Unpacks the view's three shown pieces and tail-calls the bowgun column update. */
void fn_8031A410(_PLW* plw, _EQUIP_INDEX* view, _EQUIP* columns) {
    fn_8031A428(plw, view->shown[0], view->shown[1], view->shown[2], columns);
}

/* 0x8031A428 (0x164): Puts a ranged frame into the bowgun columns: a frame whose options are off takes the frame
 * column and clears the barrel/stock columns to the frame's id; otherwise the parts follow the worn frame. */
void fn_8031A428(_PLW* plw, _EQUIP* frame, _EQUIP* barrel, _EQUIP* stock, _EQUIP* columns) {
    if ((u32)(frame->kind - 1) <= 5) {
        return;
    }
    if (frame->kind == 0xB && Gunner_opt_ok_ck(frame) == 0) {
        equip_record_copy(&columns[6], frame);
        columns[7].item_id = frame->item_id;
        columns[7].deco_count = 0;
        columns[7].deco_level = 0;
        columns[7].skill_id[0] = 0;
        columns[7].skill_id[1] = 0;
        columns[7].skill_id[2] = 0;
        columns[8].item_id = frame->item_id;
        columns[8].deco_count = 0;
        columns[8].deco_level = 0;
        columns[8].skill_id[0] = 0;
        columns[8].skill_id[1] = 0;
        columns[8].skill_id[2] = 0;
    } else if (plw->equipB.kind == 0xB) {
        if ((u8)(frame->kind + 0xF5) > 2) {
            columns[7].kind = 0;
            columns[7].item_id = 0;
            columns[8].kind = 0;
            columns[8].item_id = 0;
        } else if (Gunner_opt_ok_ck(&plw->equipB) == 0) {
            equip_record_copy(&columns[6], frame);
            columns[7].kind = 0;
            columns[7].item_id = 0;
            columns[8].kind = 0;
            columns[8].item_id = 0;
        }
    } else if (barrel == NULL && stock == NULL && (u8)(frame->kind + 0xF5) <= 2) {
        equip_record_copy(&columns[6], frame);
        columns[7].kind = 0;
        columns[7].item_id = 0;
        columns[8].kind = 0;
        columns[8].item_id = 0;
    }
}

/* 0x8031A58C - resets a menu slot into its "equipment panel" state: the row height from its +0x10 flag, the panel's
 * own fields, and the embedded +0x1EC record handed to its constructor. */
void fn_8031A58C(MenuSlot* self) {
    s16 size = 0x18;
    if (self->field_0x010 != 0) {
        size = 0x20;
    }
    self->field_0x014 = 1;
    self->field_0x23A = 0;
    self->field_0x1B0 = 0;
    self->field_0x19E = 0;
    self->entry_count_b = 8;
    self->field_0x1A3 = 8;
    self->field_0x1A0 = 0;
    s16 h = menu_page_count(size, 8);
    self->field_0x1A1 = (s8)h;
    self->field_0x1A2 = 0;
    fn_8029FFFC(self, 0);
    fn_8031AE38(self);
    fn_8031BFEC(&self->field_0x1EC[0], 0, self);
}

/* 0x8031A638 - when a menu slot is a ready bowgun/gunner panel, refresh both of its option rows. */
void fn_8031A638(MenuSlot* self) {
    s8 st = self->field_0x014;
    if ((u32)(st - 1) <= 1) {
        if (self->field_0x001 == 3) {
            if (GameMode_ck() != 2) {
                if (self->field_0x1F0 != 0) {
                    pl_item_add(self->worker, self->field_0x1F0, 1);
                }
                if (self->field_0x1F2 != 0) {
                    pl_item_add(self->worker, self->field_0x1F2, 1);
                }
            }
        }
    }
}

/* ---------------------------------------------------------------------------------------------------
 * The unwritten bodies, in address order.  Each notes the shape its target's codegen proves where
 * that shape is not obvious from the call list.
 * --------------------------------------------------------------------------------------------------- */

/* 0x80308FB4 - the equip-information screen's per-frame dispatch on its page mode: builds the two sub-screens for
 * modes 0 and 1, or refreshes the detail page for modes 2..10 with the mode as the page delta. */
void equip_info_update(StatusScreenWork* self) {
    u16* table;
    _mh_ivec2_ pos;

    s8 mode = self->field_0x1AE;

    switch (mode) {
    case 0:
        put_equip_info_upper_page(self);
        break;
    case 1:
        put_equip_info_piece_page(self);
        get_lsp_data(1482, &pos);
        draw_sprite_idx(1497, &pos);
        get_lsp_data(1322, &pos);
        table = (u16*)get_menu_lsp_tbl(0);
        PutPageArrow(table, self->field_0x1AE, self->field_0x1AF, self->field_0x08, &pos, 0);
        break;
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
        equip_page_count_step(self->equip, 0, 0, (u8)mode - 2);
        equip_detail_page_refresh(self->equip, NULL, self->field_0x1AE, self->field_0x1AF,
                    self->field_0x08);
        break;
    }
}

/* 0x8030FAC0 - draw the sprite `id` at the 0x915 frame, offset from the 0x914 anchor (or from zero
 * when the id is the anchor itself). */
void put_lsp_sprite_offset(u16 id) {
    _mh_ivec2_ pos;
    _mh_ivec2_ d;

    if (id != 0x914) {
        get_lsp_data(0x914, &pos);
        get_lsp_data(id, &d);
        d.x -= pos.x;
        d.y -= pos.y;
    } else {
        d.x = 0;
        d.y = 0;
    }
    u32 tmp = *(u32*)&d;
    put_lsp_anchor_offset(0x914, &pos, &tmp);
    draw_sprite_idx(0x915, &pos);
}

/* 0x80311A88 - draw the sprite row `id` at the 0x917 anchor's offset when the id is set, or the
 * caller's packed position otherwise. */
void put_lsp_sprite_at_anchor(u32* p, u8 id) {
    u32 v;
    _mh_ivec2_ pos;

    if (id == 0) {
        v = *p;
        put_lsp_anchor_offset(0x917, &pos, &v);
    } else {
        get_lsp_data(id, &pos);
    }
    draw_sprite_idx(0x918, &pos);
}

/* 0x80312F18 - draw the 0xC7 and 0x89 sprite runs at the 0x955 and 0x966 anchors, each offset by the
 * caller's position. */
void put_lsp_sprite_runs(u32* p) {
    _mh_ivec2_ pos;
    _mh_ivec2_ pos2;
    u32 v = *p;
    u32 v2;

    put_lsp_anchor_offset(0x955, &pos, &v);
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0xC7), &pos);
    v2 = *(u32*)&pos;
    put_lsp_anchor_offset(0x966, &pos2, &v2);
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0x89), &pos2);
}

/* 0x803115E0 - the `put_equip_panel_row_ex` panel's six-register resolve entry. */
void put_equip_panel_row_variant(_PLW* s0, _EQUIP* s1, _EQUIP* s2, _EQUIP* s3, u16 a, u8 b, u8 c) {
    _EQUIP_INDEX out;

    if (equip_variant_resolve_ex(s0, s1, s2, s3, b, &out) == 1) {
        return;
    }
    put_equip_panel_row_ex(s0, &out, a, b, c);
}

/* 0x80311840 - the `put_equip_panel_row_base` panel's entry whose resolve word is forced to zero and whose flag
 * byte keeps only its low three bits. */
void put_equip_panel_row_zero(_PLW* s0, _EQUIP* s1, u16 a, u8 b) {
    _EQUIP_INDEX out;
    u8 flag = b;

    if (equip_variant_resolve(s0, s1, 0, &flag, &out) == 1) {
        return;
    }
    flag &= 7;
    put_equip_panel_row_base(s0, &out, a, flag);
}

/* 0x803118B4 - the `put_equip_panel_row_base` panel's six-register resolve entry. */
void put_equip_panel_row(_PLW* s0, _EQUIP* s1, _EQUIP* s2, _EQUIP* s3, u16 a, u8 b) {
    _EQUIP_INDEX out;

    if (equip_variant_resolve_ex(s0, s1, s2, s3, b, &out) == 1) {
        return;
    }
    put_equip_panel_row_base(s0, &out, a, b);
}

/* The equipment panel's column layout: for each of the two panel kinds, eleven rows of three column ids
 * (0xFF = no column) that `fn_80313E24` draws.  size: 0x42 */
u8 equip_panel_row_layout[22][3] = {
    { 0, 1, 2 },
    { 3, 4, 5 },
    { 6, 7, 8 },
    { 9, 10, 11 },
    { 12, 13, 14 },
    { 39, 40, 41 },
    { 20, 21, 0xFF },
    { 22, 23, 0xFF },
    { 24, 25, 0xFF },
    { 26, 27, 0xFF },
    { 34, 35, 0xFF },
    { 30, 31, 0xFF },
    { 32, 33, 0xFF },
    { 36, 0xFF, 0xFF },
    { 15, 0xFF, 0xFF },
    { 16, 0xFF, 0xFF },
    { 18, 0xFF, 0xFF },
    { 17, 0xFF, 0xFF },
    { 19, 0xFF, 0xFF },
    { 38, 0xFF, 0xFF },
    { 28, 0xFF, 0xFF },
    { 29, 0xFF, 0xFF },
};
