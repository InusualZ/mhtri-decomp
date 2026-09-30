/* menu/menu_infomation.cpp - the equipment-information screen: the whole `menu_infomation.cpp` TU,
 * `.text` 0x80308FB4..0x8031A6C0 (0x1170C B, 149 symbols), extab 0x80015B54..0x80015F3C (125
 * records) and extabindex 0x80034764..0x80034D40 (125 x 12 B).
 *
 * NAME AND EXTENT (class 1 evidence, `.pi/notes/seam-round.md`).  `.data` 0x805DCCDC is the bare
 * source name `"menu_infomation.cpp"` (the retail misspelling is the original) and 0x805DCCF0 its
 * `"NW4R:Failed assertion "` message; `fn_80312F84` (this unit) builds the pair at 0x80313020.  That
 * static has exactly ONE copy in the DOL, and the functions whose relocations name it span
 * 0x8030A328 .. `Set_equip_column_arrangement` (0x8031A244) - a TU-local static has one emitter, so
 * that whole run is one original source file.  The seams that used to cut it (0x8030D338: 2 referrers
 * above / 9 below; 0x80313E24: 3 / 8) were FALSE; the left edge 0x80308FB4 (0 above / 11 below) is
 * the cut this file took, and 0x8031A6C0 is the right one (11 / 0, and the extab/extabindex runs tile
 * there).  Three registrations were redrawn onto it: `ef/fn_8030681C.cpp`'s tail, this file's old
 * 0x8030D338..0x80313E24 fragment, and the never-landed `menu/fn_80313E24.cpp`.
 *
 * MODULE is `menu` (the `menu_*` file family this band's data pool carries: `menu_item.cpp`,
 * `menu_note.cpp`, `menu_placeinfo.cpp`), extension `.cpp` (the range's mangled callees).
 * `fn_8027FF88`'s caller and the `Set_equip_column_arrangement` mangling are the two C++ signals.
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for most of this range (`symedit.py range`
 * 0x80308FB4 0x8031A6C0: of 149 rows five are named - the four `Put_*` putters and
 * `Set_equip_column_arrangement` - and `dumpmap.py lookup` answers `zz_<addr>_` for the rest).  The
 * named rows are written as the C++ free functions their manglings spell; every bare stem is
 * `extern "C"` so the emitted name stays the map's.
 *
 * SECTIONS.  extab/extabindex are this unit's own runs; the split at 0x80308FB4 is the first
 * extabindex record whose function is `equip_info_update` (0x80034674 + 12*20 = 0x80034764, its extab
 * pointer 0x80015B54), and both runs tile exactly with the neighbours `ef/fn_8030681C.cpp`
 * (0x80015AB4 / 0x80034674) and `menu/fn_8031A6C0.cpp` (0x80015F3C / 0x80034D40).  No `.data`/`.sdata`
 * run is claimed: every table and constant the bodies read is shared with the neighbouring units of
 * this band, and our object emits no data section at all (`datagap.py`: ours-extra empty in every
 * section; every gap is target-extra, i.e. an unwritten body).
 *
 * STATUS (official metric, this worktree's `build/RMHE08/obj/menu/menu_infomation.o` vs ours):
 * **8.986390 % fuzzy, 2092 / 71436 `.text` bytes matched, 27 byte-identical rows**; 91 of the 149
 * rows are written.  They came from the three registrations this seam redrew (28 rows of the old
 * 0x8030D338 fragment, 5 screen wrappers of the old `ef/fn_8030681C.cpp` head, the 50 rows the
 * never-landed `menu/fn_80313E24.cpp` brought) plus the five bodies this pass added.
 *
 * THIS PASS (2026-09-28).  Seven bodies written and measured (7.71 % -> 8.99 %), plus the names of 22
 * of this unit's own `fn_XXXXXXXX` rows (rule 7), which the new call sites needed anyway:
 *   `put_lsp_sprite_offset` (0x8030FAC0, 160 B), `put_lsp_sprite_runs` (0x80312F18, 108 B),
 *       `put_equip_panel_row_variant` (0x803115E0, 124 B) and `put_equip_panel_row`
 *       (0x803118B4, 108 B) 100.00.
 *   `put_lsp_sprite_at_anchor` (0x80311A88, 96 B) 99.79  one sprite row, at the caller's packed
 *       position when the id is 0.
 *   `put_equip_panel_row_zero` (0x80311840, 116 B) 94.48 and `equip_info_update` (0x80308FB4, 228 B)
 *       85.75.
 *   `fn_8030FB78` (0x8030FB78, 200 B) 96.00 -> 100.00: an existing row, fixed by playbook 38 - the
 *       `d.x = d.x - pos.x` pair stores through an `extsh` MWCC does not emit for `d.x -= pos.x`.
 *
 * RESIDUALS, in address order:
 *   `fn_80311540` 46.25  its tail-call argument is masked with `rlwinm r0,r6,0,31,28` + `clrlwi` (a
 *       bitfield/`char` conversion whose source shape is not recovered).
 *   `fn_803159CC` 69.19  the target branches on `(flags & 2)` in three arms where MWCC folds all
 *       four to one select.
 *   `equip_info_update` 85.75: a register-colouring residual (the same instruction multiset, the
 *       allocator's web order differs) on the `PutPageArrow` argument setup - `mt.py diff` shows the
 *       two `extsb` pairs in r4/r5 where the target has them in r5/r6.
 *   `put_equip_panel_row_zero` 94.48: `flag &= 7` on a `u8` gives `clrlwi r0,r0,29` where the target
 *       has `rlwinm r0,r0,0,31,28` + `clrlwi r6,r0,24`, i.e. `(u8)(x & 0x80000007)` - the mask's
 *       spelling is not recovered.
 *   `put_lsp_sprite_at_anchor` 99.79: one argument-register move (`mr r3,r0` + `clrlwi r3,r3,16` taken
 *       in the other order).
 *   the unwritten rows, largest first: `fn_80311C3C` (0xC64), `fn_80312F84` (0xBB8) - the Panic that
 *       builds this unit's `__FILE__` pair at 0x80313020 -, `put_equip_info_upper_page`,
 *       `fn_8030A9B4`, `put_equip_info_piece_page`, `fn_8031994C`/`fn_8031949C` and the bowgun panels
 *       (`fn_8030E79C`/`fn_8031077C`/`fn_80310F30`/`put_equip_panel_row_ex`).
 *   the 37 tail placeholders (`fn_80315C00`, `fn_8031A428`, ...) are empty definitions kept so the
 *       map's rows pair by name; each is a body still to write, not a reconstruction.
 *
 * BLOCKED BY A DECLARATION, NOT BY THE CODE (each measured, then held out of this commit).  Five more
 * bodies were written and measured in this pass and are not in it, because each newly references a
 * symbol whose correct spelling would mean editing a file another live lane owns this wave (`hud/`):
 *   `fn_8030BFB4`/`fn_8030C0D4`/`fn_8030C1F4` (0x8030BFB4.., 288 B each) **measured 100.00 %** - the
 *       equip panel's per-kind dispatcher, its three variants differing only in the case-0 builder.
 *       They call `fn_802E14CC__FUsP6_EQUIPPC10_mh_ivec2_` and `fn_8030A05C` (below) calls
 *       `fn_802E1134__FUsUcUlPC10_mh_ivec2_`; both addresses are in `hud/layout.cpp`'s registered
 *       range and both are declared in `include/hud/layout.h` **inside its `extern "C"` block**, so
 *       our object emits the unmangled name and `undefrefs.py` refuses the unit.  The fixing pass is
 *       mechanical: rename the two symbols (rule 7), move those two declarations into that header's
 *       C++-scope block, sweep `src/hud/layout.cpp`, and land these four bodies unchanged.
 *   `fn_8030A05C` (0x8030A05C, 372 B) **measured 94.01 %** - the three-row piece preview, the same
 *       `fn_802E1134` blocker.
 *   `fn_8030C314` (520 B) **measured 87.50 %** as `put_equip_category_panel` - the per-category panel
 *       dispatch.  Its tail calls `get_rare_color` (0x802DB254, owned by `ai/fn_802D44F4.cpp`), which
 *       `include/hud/layout.h` likewise declares `extern "C"`: our object emits `get_rare_color`
 *       where the map has `get_rare_color__FUc`.  Declaring it at C++ scope in the *owner's* header
 *       (`include/ai/fn_802D44F4.h`, rule 2's home) clashes with that declaration while this unit
 *       includes `hud/layout.h`, so the fix is the same one-line move.  Its `EquipWork` view stays
 *       below, because `equip_detail_page_refresh` takes it too.
 *   `PutPageArrow` (0x802DAF48) and `GetEquipName` (0x8027E9F0) were the same class of blocker and are
 *       fixed in this commit: each is now declared at C++ scope in its owner's header
 *       (`include/ai/fn_802D44F4.h` and `include/Pl/fn_8027D684.h`), so our object emits the map's
 *       mangled spelling.  `undefrefs.py` listed both before that fix and lists neither now.
 *   `get_lsp_data` (0x802E0550) is the one unresolved reference `undefrefs.py` still reports, and it
 *       is **pre-existing** (`git show main:src/menu/menu_infomation.cpp` spells it 9 times): the same
 *       `extern "C"` declaration, the same one-line fix, the same reason it is not in this commit.
 *   `put_equip_info_upper_page`/`put_equip_info_piece_page` additionally read `lbl_80792BF4` and
 *       `lbl_80794880`, unowned `.data`/`.sdata` of which this range is the only referrer: rule 12
 *       says the unit claims those runs and emits them, its own measured step (a `.data` claim can
 *       drop the target's `R_PPC_NONE` pool relocations) - deferred, not forgotten.
 *
 * RULE 7 NAMES.  The 22 renames are the names of this unit's own rows, derived from what each body
 * does for its caller (the calling screen, the anchor id it draws at, the record it resolves).  The
 * unwritten ones are explicit **GUESS**es: `fn_80315440`/`fn_803155BC` become
 * `equip_variant_resolve`/`equip_variant_resolve_ex`, `fn_803116D8`/`fn_80311920` become
 * `put_equip_panel_row_ex`/`put_equip_panel_row_base`, and `fn_803128A0`/`fn_80313C04` become
 * `put_equip_panel_kind4`/`put_equip_panel_kind5` - each from the only evidence there is, the
 * argument list `put_equip_category_panel` calls it with.  Refine them when the bodies are written.
 *
 * MOVED BODIES.  The head's five wrappers and the tail's 13 bodies are the same source, with the
 * signature reconciliations one TU needs: `equip_variant_resolve`/`equip_variant_resolve_ex`/`fn_80315730`/`fn_80315A60`/
 * `fn_80315274`/`fn_803153F8` now take the argument list the old 0x8030D338 half already called them
 * with (they were `void f(void)` placeholders in a separate TU), and `Set_equip_column_arrangement`
 * was written with `void*` parameters, which mangles to a name objdiff cannot pair; it now spells the
 * map's `Set_equip_column_arrangement__FP4_PLWP12_EQUIP_INDEXP6_EQUIP` (rule 9).  `fn_8030B790`
 * measures 81.48 -> 94.63 across the move because the merged unit compiles it with `cflags_menu`
 * (`-opt nopeephole`), which that body's target codegen wants.
 *
 * TYPES.  `MenuSlot` (include/menu/menu_item.h) is the 0x330-byte menu working record; the
 * `+0x19E`/`+0x1A0..+0x1A3`/`+0x1B0`/`+0x1EC`/`+0x1F0` fields this unit's tail reads were named on it
 * with `menu/fn_80313E24.cpp`, which is where `menu_item.h`'s own `fn_8031A638(MenuSlot*)` caller puts
 * them too.  `StatusScreenWork` (include/menu/menu_infomation.h) is the head's partial view of the
 * same record - it lives in this unit's header because `ef/fn_8030681C.cpp`'s two below-the-seam
 * bodies read it as well.  The three tail records (`EquipColumnPanel`, `EquipSlotInfo`,
 * `EquipSubInfo`) and `EquipListWork` are this unit's own.  `EquipWork` (this unit's own, defined
 * below the declaration block) is the record `StatusScreenWork::equip` points at - the view the
 * category panel proves, with the six piece records at 0x140 and the bowgun/ammo triple at
 * 0x1D0/0x1E8/0x1F4.
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

extern "C" int sprintf(s8*, const char*, ...);   /* 0x8045DECC, the OS Runtime's */

/* The equipment slot index the detail putters walk (the `Set_equip_column_arrangement` this unit
 * also holds takes it).  Only +0x00/+0x04 are touched by this range.  size: 0x08 (approximate: max
 * touched offset + 1) */
struct _EQUIP_INDEX {
    /* +0x00 */ u32 equip_0x00;   /* the packed equipment word `fn_8027FF88` classifies */
    /* +0x04 */ u32 unused_0x04;
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

/* The sub-record `fn_8027ECAC` returns and `fn_80316978` reads (+0x0C is its class byte).
 * size: 0x10 (approximate: max touched offset + 1). */
struct EquipSubInfo {
    /* +0x00 */ u8 pad_0x00[0xC];
    /* +0x0C */ u8 field_0x0C;
};

/* The equip list record `fn_8030B790` counts over: one 16-bit slot id per row and a matching per-row
 * flag byte, 0x5F2 into the record.  Only those two arrays are reached, so the size is their extent.
 * size: 0x60C (the extent `fn_8030B790` proves) */
typedef struct EquipListWork {
    u8  pad_0x000[0x5F2];          /* +0x000 */
    u16 slot_id[8];                /* +0x5F2  one id per row (stride 2) */
    u8  row_flag[8];               /* +0x602  the per-row visible flag */
    u8  pad_0x60A[2];              /* +0x60A */
} EquipListWork;

/* The equipment work record the status screens walk (the record `StatusScreenWork::equip` points
 * at).  Only the view these bodies prove is named: the seven 0x0C-byte piece records at 0x140..0x188
 * and the bowgun/ammo triple at 0x1D0/0x1E8/0x1F4.  size: 0x200 (approximate: max touched offset +
 * 1, the record's own extent is larger - `put_equip_info_piece_page` reads +0x612). */
typedef struct EquipWork {
    u8     pad_0x000[0x140];        /* +0x000 */
    _EQUIP piece[6];                /* +0x140  one piece record per equip row, stride 0x0C */
    u8     pad_0x188[0x1D0 - 0x188];/* +0x188 */
    _EQUIP bowgun;                  /* +0x1D0  the ranged-weapon record */
    u8     pad_0x1DC[0x1E8 - 0x1DC];/* +0x1DC */
    _EQUIP field_0x1E8;             /* +0x1E8  the ammo/coating record */
    _EQUIP field_0x1F4;             /* +0x1F4  the second ammo record */
} EquipWork;


/* This unit's own C++-linkage exports (address order).  They are declared as their real signatures so
 * the C++ front-end reproduces the map's mangling (rule 9). */
void Put_equip_dtl_basis_bowgun_gan1(_PLW*, _EQUIP_INDEX*, u16, u8, _mh_ivec2_*);
void Put_equip_dtl_basis_bowgun_gan2(_PLW*, _EQUIP_INDEX*, u16, u8, _mh_ivec2_*);
void Put_equip_dtl_basis_bowgun_gan_lv(_PLW*, _EQUIP_INDEX*, u16, u8, _mh_ivec2_*);
void Set_equip_column_arrangement(_PLW*, _EQUIP_INDEX*, _EQUIP*);

#ifdef __cplusplus
extern "C" {
#endif

/* This unit's own forward declarations (address order).  They carry C linkage here so the definitions
 * below emit the map's bare `fn_XXXXXXXX` stems (rule 9: an `fn_` stem is not a mangling). */
void equip_info_update(StatusScreenWork*);
void put_equip_info_upper_page(StatusScreenWork*);
void put_equip_info_piece_page(StatusScreenWork*);
s32  fn_8030A1D0(void*, s32);
s32  fn_8030A1DC(void*, s32, s32, s32);
s32  equip_page_count_step(void*, void*, u32, s32);
s32  fn_8030A328(void*, void*, s32, s32, u16, s8);
void equip_detail_page_refresh(EquipWork*, _EQUIP*, s8 page, s8 last, u16 flags);
s32  fn_8030B790(EquipListWork*);
void fn_8030BACC(StatusScreenWork*);
s32  fn_8030CA50(void*, void*, u32, u32);
s32  fn_8030CA68(void*, void*, s32, u16, u8);
void fn_8030D6C0(void*, void*, void*, u16, u8);
void fn_8030D6A8(void*, void*, u16, u8);
void fn_8030D808(void*, void*, u16, u8);
void put_equip_row_variant(void*, void*, void*, void*, u16, u8);
void fn_8030F814(void*, void*, u16, u8);
void fn_8030F7B0(void*, void*, void*, u16, u8);
void put_equip_row_variant_ex(void*, void*, void*, void*, u16, u8);
void fn_8030FD30(void*, void*, void*, void*, void*, void*, void*, u16);
void fn_8030FC58(void*, void*, void*, u16, u8);
void fn_80310560(void*, void*, void*, u16, u8);
void put_equip_row_variant2(void*, void*, void*, void*, u16, u8);
void fn_80310638(void*, void*, void*, void*, void*, void*, void*, u16);
void fn_80310E58(void*, void*, void*, u16, u8);
void fn_80310EC0(void*, void*, void*, void*, u16, u8);
void fn_80310F30(void*, void*, u16, u8);
void fn_80311560(void*, void*, void*, u16, u8, u8);
void put_equip_panel_row_ex(void*, void*, u16, u8, u8);
void put_equip_panel_row_variant(EquipWork*, _EQUIP*, _EQUIP*, _EQUIP*, u16, u8, u8);
void put_equip_panel_row_zero(EquipWork*, _EQUIP*, u16, u8);
void put_equip_panel_row(EquipWork*, _EQUIP*, _EQUIP*, _EQUIP*, u16, u8);
void put_lsp_sprite_at_anchor(u32*, u8);
void put_lsp_sprite_runs(u32*);
void put_equip_panel_row_base(EquipWork*, u8* rec, u16 a, u8 b);
void fn_8030F87C(void*, void*, u16, u8);
void put_lsp_sprite_offset(u16);
void fn_8030FDA0(void*, void*, u16, u8);
void fn_803106A8(void*, void*, u16, u8);
void fn_8031077C(void*, void*, u8, void*);
void fn_8030E79C(void*, void*, void*, u16, u8);
void fn_8030E784(void*, void*, u16, u8);
void fn_8030F38C(void*, void*, void*, u16, u8);
void fn_8030F374(void*, void*, u16, u8);
void fn_8030FB78(u16, u8);
void fn_8030FB60(u16);
void fn_8030FB6C(u16);
void fn_8030FC40(void*, void*, u16, u8);
void fn_8030FF0C(void*, void*, void*, u16, u8);
void fn_8030FEF4(void*, void*, u16, u8);
void fn_80310548(void*, void*, u16, u8);
void fn_80310E40(void*, void*, u16, u8);
void fn_80311540(void*, void*, u16, u32, u8);
void fn_80312DF0(u32*);
void fn_80313E24(void);
void fn_803142C8(u32, u8, const _mh_ivec2_*);
void fn_80314340(void);
void fn_803144A0(void);
u8   fn_80314604(_PLW*, s8, u32);
void fn_8031468C(void);
u32  fn_803149B8(u16, u32*, u8, u32*);
u32  fn_80314A64(u16, u32, u32, u32*);
u8   fn_80314F18(void*, void*);
void fn_80314F90(void);
void fn_80315274(void*, void*, u8);
void fn_803153F8(EquipColumnPanel*, u8);
u8   equip_variant_resolve(void*, void*, void*, void*, void*);
u8   equip_variant_resolve_ex(void*, void*, void*, void*, u8, void*);
u8   fn_80315730(void*, void*, void*, void*, void*, void*, void*, void*, void*);
u8   fn_803159CC(u8);
u8   fn_80315A60(void*, void*, u8);
void fn_80315C00(void);
void fn_80315D0C(void);
void fn_80315E34(void);
void fn_80316058(void);
void fn_8031622C(void);
void fn_803164D0(void);
void fn_80316780(void);
u32  fn_80316978(u32, _EQUIP**, u8, u8*);
void fn_80316A14(void);
void fn_80316DDC(void);
void fn_803170B4(void);
void fn_8031729C(void);
void fn_803173C4(void);
void fn_80317788(void);
void fn_80317B84(void);
void fn_80317F94(void);
u8   fn_80318204(u8, u8*, u8*);
void fn_803182D8(void);
u32  fn_80318790(u32, u8**, u8);
void fn_803188AC(void);
void fn_80318A10(void);
void fn_80318E08(void);
void fn_80318FB0(void);
u8   fn_80319178(EquipSlotInfo*, u16*, s8*);
void fn_80319204(void);
void fn_8031949C(void);
void fn_8031994C(void);
void fn_80319F18(void);
void fn_8031A410(void*, u32*, void*);
void fn_8031A428(void*, void*, u32, u32, void*);
void fn_8031A58C(MenuSlot*);
void fn_8031A638(MenuSlot*);

/* Callees other units own.  Most are declared in their owner's header and included above
 * (`fn_8027FF88`, `fn_8027F11C`, `fn_8027ECAC`, `fn_8027FFFC` in `Pl/fn_8027D684.h`; `Pl_Skill_slot_item_get`
 * and `fn_80272E30` in `Pl/pl_skill.h`; `fn_8029FFFC` and `get_menu_lsp_tbl` in `menu/menu_item.h`;
 * `fn_800CF208` in `ef/fn_800CDB2C.h`; `put_lsp_anchor_offset` and `get_str_tbl` in `include/unsplit/menu.h`).
 * The two below cannot: `fn_8031AE38`/`fn_8031BFEC` are owned by `menu/fn_8031A6C0.cpp`, whose header
 * declares neither and `include/unsplit/menu.h` (their old home) may not.  Declared here as this unit's
 * view, the way `lobby/fn_801EC9F8.cpp` declares its own. */
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
s32 fn_8030A1D0(void* equip, s32 mode)
{
    return fn_8030A1DC(equip, mode, 0, 0);
}

/* The same, for a caller that carries a page id and a signed page delta. */
s32 equip_page_count_step(void* a, void* b, u32 page, s32 delta)
{
    return fn_8030A328(a, b, 0, 0, page, delta);
}

/* The equip detail page with the row and the item list's own selector. */
s32 fn_8030CA50(void* a, void* b, u32 row, u32 sel)
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
s32 fn_8030B790(EquipListWork* self)
{
    s32 count = 0;
    u32 i;

    for (i = 0; i < 8; i++) {
        if (self->slot_id[i] != 0 && self->row_flag[i] != 0) {
            count++;
        }
    }

    {
        s8 pages = (s8)((count + 5) / 6);
        if (pages != 0) {
            return pages;
        }
    }
    return 1;
}

/* 0x8030D338 - the sword's colour panel.  Classify the slot (`fn_8027FF88`), then draw the sprite
 * run `get_menu_lsp_tbl(0x8E|0x8F)` names (each frame recoloured from `lbl_805DCD48`) and, for the
 * colour forms, up to two label rows from the string table. */
void Put_equip_dtl_basis_sword_colorX(_PLW* plw, _EQUIP_INDEX* idx, u16 a, u16 b,
                                      _mh_ivec2_* pos, u8 c) {    u8 n = fn_8027FF88(idx->equip_0x00);
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
        fn_801E6850(&spr, get_lsp_data(*tbl, NULL));
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
void fn_8030D6A8(void* s0, void* s1, u16 a, u8 b) {
    fn_8030D6C0(s0, s1, 0, a, b);
}

/* 0x8030F374 - the same zero-sub-index entry for the `fn_8030F38C` band. */
void fn_8030F374(void* s0, void* s1, u16 a, u8 b) {
    fn_8030F38C(s0, s1, 0, a, b);
}

/* 0x8030E784 - the `fn_8030E79C` band's zero-sub-index entry. */
void fn_8030E784(void* s0, void* s1, u16 a, u8 b) {
    fn_8030E79C(s0, s1, 0, a, b);
}

/* 0x8030FC40 - the `fn_8030FC58` band's zero-sub-index entry. */
void fn_8030FC40(void* s0, void* s1, u16 a, u8 b) {
    fn_8030FC58(s0, s1, 0, a, b);
}

/* 0x8030FEF4 - the `fn_8030FF0C` band's zero-sub-index entry. */
void fn_8030FEF4(void* s0, void* s1, u16 a, u8 b) {
    fn_8030FF0C(s0, s1, 0, a, b);
}

/* 0x80310548 - the `fn_80310560` band's zero-sub-index entry. */
void fn_80310548(void* s0, void* s1, u16 a, u8 b) {
    fn_80310560(s0, s1, 0, a, b);
}

/* 0x80310E40 - the `fn_80310E58` band's zero-sub-index entry. */
void fn_80310E40(void* s0, void* s1, u16 a, u8 b) {
    fn_80310E58(s0, s1, 0, a, b);
}

/* 0x80311540 - the `fn_80311560` band's zero-sub-index entry (its third argument's top two bits
 * are cleared on the way through). */
void fn_80311540(void* s0, void* s1, u16 a, u32 c, u8 d) {
    fn_80311560(s0, s1, 0, a, (u8)(c & 0x9FFFFFFF), d);
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
void fn_8030D6C0(void* s0, void* s1, void* s2, u16 a, u8 b) {
    u8 out[24];
    u8 spb;
    spb = b;
    if (equip_variant_resolve(s0, s1, s2, &spb, out) == 1) {
        return;
    }
    fn_8030D808(s0, out, a, spb);
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

/* 0x8030D808 - the weapon detail's shared body: position the panel from the 0x84E anchor (or the
 * caller's offset), then dispatch on `fn_80315A60`'s row - one label, the caller's value, or the
 * three bowgun panels plus the trailing text row. */
void fn_8030D808(void* s0, void* s1, u16 a, u8 b) {
    _mh_ivec2_ pos;
    _mh_ivec2_ off;
    _mh_ivec2_ src;
    if (a != 0) {
        get_lsp_data(0x84E, &pos);
        get_lsp_data(a, &off);
        off.x = off.x - pos.x;
        off.y = off.y - pos.y;
    } else {
        off.x = 0;
        off.y = 0;
    }
    src = off;
    put_lsp_anchor_offset(0x84E, &pos, &src);
    u8 r = fn_80315A60(s0, s1, b);
    switch (r) {
    case 1:
        return;
    case 2:
        draw_font_idx(0x8D2, (s8*)((u8**)get_str_tbl(0x2E))[1], 0, &pos);
        return;
    default:
        fn_80315274(s0, s1, b);
        Put_equip_dtl_basis_bowgun_gan1((_PLW*)s0, (_EQUIP_INDEX*)s1, 0, b, &off);
        Put_equip_dtl_basis_bowgun_gan2((_PLW*)s0, (_EQUIP_INDEX*)s1, 0, b, &off);
        Put_equip_dtl_basis_bowgun_gan_lv((_PLW*)s0, (_EQUIP_INDEX*)s1, 0, b, &off);
        fn_803153F8((EquipColumnPanel*)s0, b);
        return;
    }
}

/* 0x8030D728 - the second small-weapon branch entry (resolves through `equip_variant_resolve_ex`). */
void put_equip_row_variant(void* s0, void* p1, void* p2, void* p3, u16 a, u8 b) {
    u8 out[24];
    if (equip_variant_resolve_ex(s0, p1, p2, p3, b, out) == 1) {
        return;
    }
    fn_8030D808(s0, out, a, b);
}

/* ---- the same resolve-then-dispatch shape, one family per weapon sub-band ----
 * Each `fn_8030F7B0`-style entry resolves the caller's variant into a stack record with
 * `equip_variant_resolve` and hands it to its band's dispatcher; `put_equip_row_variant_ex`-style resolves through
 * `equip_variant_resolve_ex` (six registers plus a slot); `fn_8030FD30`-style through `fn_80315730`, whose
 * ninth (stack) argument is the record. */

/* 0x8030F7B0 - the 0x8030F87C band's `equip_variant_resolve` entry. */
void fn_8030F7B0(void* s0, void* s1, void* s2, u16 a, u8 b) {
    u8 out[24];
    u8 spb;
    spb = b;
    if (equip_variant_resolve(s0, s1, s2, &spb, out) == 1) {
        return;
    }
    fn_8030F87C(s0, out, a, spb);
}

/* 0x8030F814 - the `fn_8030F87C` band's variant whose variant word is forced to 0. */
void fn_8030F814(void* s0, void* s1, u16 a, u8 b) {
    u8 out[24];
    u8 spb;
    spb = b;
    if (equip_variant_resolve(s0, s1, 0, &spb, out) == 1) {
        return;
    }
    fn_8030F87C(s0, out, a, spb);
}

/* 0x8030FC58 - the 0x8030FDA0 band's `equip_variant_resolve` entry. */
void fn_8030FC58(void* s0, void* s1, void* s2, u16 a, u8 b) {
    u8 out[24];
    u8 spb;
    spb = b;
    if (equip_variant_resolve(s0, s1, s2, &spb, out) == 1) {
        return;
    }
    fn_8030FDA0(s0, out, a, spb);
}

/* 0x8030FCC0 - the 0x8030FDA0 band's `equip_variant_resolve_ex` entry. */
void put_equip_row_variant_ex(void* s0, void* p1, void* p2, void* p3, u16 a, u8 b) {
    u8 out[24];
    if (equip_variant_resolve_ex(s0, p1, p2, p3, b, out) == 1) {
        return;
    }
    fn_8030FDA0(s0, out, a, b);
}

/* 0x8030FD30 - the 0x8030FDA0 band's `fn_80315730` entry. */
void fn_8030FD30(void* s0, void* p1, void* p2, void* p3, void* p4, void* p5, void* p6, u16 a) {
    u8 out[59];
    u8 tmp;
    if (fn_80315730(s0, p1, p2, p3, p4, p5, p6, &tmp, out) == 1) {
        return;
    }
    fn_8030FDA0(s0, out, a, tmp);
}

/* 0x80310560 - the 0x803106A8 band's `equip_variant_resolve` entry. */
void fn_80310560(void* s0, void* s1, void* s2, u16 a, u8 b) {
    u8 out[24];
    u8 spb;
    spb = b;
    if (equip_variant_resolve(s0, s1, s2, &spb, out) == 1) {
        return;
    }
    fn_803106A8(s0, out, a, spb);
}

/* 0x803105C8 - the 0x803106A8 band's `equip_variant_resolve_ex` entry. */
void put_equip_row_variant2(void* s0, void* p1, void* p2, void* p3, u16 a, u8 b) {
    u8 out[24];
    if (equip_variant_resolve_ex(s0, p1, p2, p3, b, out) == 1) {
        return;
    }
    fn_803106A8(s0, out, a, b);
}

/* 0x80310638 - the 0x803106A8 band's `fn_80315730` entry. */
void fn_80310638(void* s0, void* p1, void* p2, void* p3, void* p4, void* p5, void* p6, u16 a) {
    u8 out[59];
    u8 tmp;
    if (fn_80315730(s0, p1, p2, p3, p4, p5, p6, &tmp, out) == 1) {
        return;
    }
    fn_803106A8(s0, out, a, tmp);
}

/* 0x80310E58 - the 0x80310F30 band's `equip_variant_resolve` entry. */
void fn_80310E58(void* s0, void* s1, void* s2, u16 a, u8 b) {
    u8 out[24];
    u8 spb;
    spb = b;
    if (equip_variant_resolve(s0, s1, s2, &spb, out) == 1) {
        return;
    }
    fn_80310F30(s0, out, a, spb);
}

/* 0x80310EC0 - the 0x80310F30 band's `equip_variant_resolve_ex` entry. */
void fn_80310EC0(void* s0, void* p1, void* p2, void* p3, u16 a, u8 b) {
    u8 out[24];
    if (equip_variant_resolve_ex(s0, p1, p2, p3, b, out) == 1) {
        return;
    }
    fn_80310F30(s0, out, a, b);
}

/* 0x80311560 - the 0x803116D8 band's `equip_variant_resolve` entry (its resolved byte is masked before it
 * is handed on). */
void fn_80311560(void* s0, void* s1, void* s2, u16 a, u8 b, u8 c) {
    u8 out[36];
    u8 spb;
    spb = b;
    if (equip_variant_resolve(s0, s1, s2, &spb, out) == 1) {
        return;
    }
    spb = (u8)(spb & 0x9FFFFFFF);
    put_equip_panel_row_ex(s0, out, a, spb, c);
}

/* 0x803106A8 - the 0x8031077C band's body: position the panel from the 0x8F0 anchor (or the
 * caller's offset) and hand the resulting anchor to `fn_8031077C`. */
void fn_803106A8(void* s0, void* s1, u16 a, u8 b) {
    _mh_ivec2_ pos;
    _mh_ivec2_ off;
    _mh_ivec2_ src;
    _mh_ivec2_ copy;
    if (a != 0) {
        get_lsp_data(0x8F0, &pos);
        get_lsp_data(a, &off);
        off.x = off.x - pos.x;
        off.y = off.y - pos.y;
    } else {
        off.x = 0;
        off.y = 0;
    }
    src = off;
    put_lsp_anchor_offset(0x8F0, &pos, &src);
    copy = pos;
    fn_8031077C(s0, s1, b, &copy);
}

/* ---------------------------------------------------------------------------------------------------
 * 0x80313E24..0x8031A638 - the second half: the equipment columns, the slot blocks and the panel's
 * own cursor (13 bodies, the rest are placeholders - see the header's residual list)
 * --------------------------------------------------------------------------------------------------- */

/* 0x80313E24 */
void fn_80313E24(void) {}

/* 0x803142C8 - draw the base/selected run for one slot: resolve the `0x9AA` anchor against the
 * caller's position, then the `0xD0`/`0xD2` sprite lists at it. */
void fn_803142C8(u32 unused, u8 idx, const _mh_ivec2_* pos) {
    u32 in = *(const u32*)pos;   /* the 4-byte `_mh_ivec2_` block, copied whole as the target does */
    u32 out1;
    u32 tmp;
    u32 out2;
    put_lsp_anchor_offset(0x9AA, &out1, &in);
    u16* tbl = (u16*)get_menu_lsp_tbl(0xD0);
    tmp = out1;
    put_lsp_anchor_offset(tbl[idx], &out2, &tmp);
    u16* tbl2 = (u16*)get_menu_lsp_tbl(0xD2);
    draw_sprite_ary(tbl2, (const _mh_ivec2_*)&out2);
}

/* 0x80314340 */
void fn_80314340(void) {}

/* 0x803144A0 */
void fn_803144A0(void) {}

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
u8 fn_80314F18(void* a, void* b) {
    u8 total = fn_8027F11C(a);
    if (*(u8*)a == 0xC && b != 0 && *(u8*)b == 0xD) {
        total = (u8)(total + fn_8027F11C(b));
    }
    return total;
}

/* 0x80314F90 */
void fn_80314F90(void) {}

/* 0x80315274 */
void fn_80315274(void* a, void* b, u8 c) {}

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

/* 0x80315440 */
u8 equip_variant_resolve(void* a, void* b, void* c, void* d, void* e) { return 0; }

/* 0x803155BC */
u8 equip_variant_resolve_ex(void* a, void* b, void* c, void* d, u8 e, void* f) { return 0; }

/* 0x80315730 */
u8 fn_80315730(void* a, void* b, void* c, void* d, void* e, void* f, void* g, void* h, void* i) { return 0; }

/* 0x803159CC - unpack the three-bit equipment class from a flags byte: bit 0 selects the high/low
 * group, bit 5 the sub-group within it, bit 1 the last choice. */
u8 fn_803159CC(u8 flags) {
    if (flags & 1) {
        if (flags & 0x20) {
            if (flags & 2) {
                return 1;
            }
            return 2;
        } else {
            if (flags & 2) {
                return 3;
            }
            return 4;
        }
    } else {
        if (flags & 0x20) {
            if (flags & 2) {
                return 5;
            }
            return 6;
        } else {
            return (flags & 2) ? 7 : 8;
        }
    }
}

/* 0x80315A60 */
u8 fn_80315A60(void* a, void* b, u8 c) { return 0; }

/* 0x80315C00 */
void fn_80315C00(void) {}

/* 0x80315D0C */
void fn_80315D0C(void) {}

/* 0x80315E34 */
void fn_80315E34(void) {}

/* 0x80316058 */
void fn_80316058(void) {}

/* 0x8031622C */
void fn_8031622C(void) {}

/* 0x803164D0 */
void fn_803164D0(void) {}

/* 0x80316780 */
void fn_80316780(void) {}

/* 0x80316978 - read the class byte at +0x0C of the `fn_8027ECAC` sub-record for the classes that
 * carry it (1/2/5/6); returns bit 0 set when the byte was written. */
u32 fn_80316978(u32 unused, struct _EQUIP** slot, u8 sel, u8* out) {
    u32 ret = 0;
    u8 v = 0;
    u8 kind = fn_803159CC(sel);
    if ((u32)(kind - 1) <= 1 || (u32)(kind - 5) <= 1) {
        struct EquipSubInfo* q = (struct EquipSubInfo*)fn_8027ECAC(slot[0]);
        if (q != 0 && q->field_0x0C != 0) {
            v = q->field_0x0C;
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

/* 0x8031729C */
void fn_8031729C(void) {}

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
u32 fn_80318790(u32 unused, u8** b, u8 sel) {
    u8* p = b[0];
    u8* q = b[1];
    u8* r = b[2];
    u32 ret = 0;
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
void fn_80318A10(void) {}

/* 0x80318E08 */
void fn_80318E08(void) {}

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
            n = n + 1;
        }
    }
    return n;
}

/* 0x80319204 */
void fn_80319204(void) {}

/* 0x8031949C */
void fn_8031949C(void) {}

/* 0x8031994C */
void fn_8031994C(void) {}

/* 0x80319F18 */
void fn_80319F18(void) {}

void Set_equip_column_arrangement(_PLW* plw, _EQUIP_INDEX* index, _EQUIP* equip) {}

/* 0x8031A410 - unpack a 3-word slot record and tail-call the panel updater. */
void fn_8031A428(void* self, void* slot, u32 a, u32 b, void* work);
void fn_8031A410(void* self, u32* slot, void* arg) {
    fn_8031A428(self, (void*)slot[0], slot[1], slot[2], arg);
}

/* 0x8031A428 */
void fn_8031A428(void* self, void* slot, u32 a, u32 b, void* work) {}

/* 0x8031A58C - reset a menu slot into its "equipment panel" state: pick the row height from the
 * slot's +0x10 flag, seed the panel's own fields and hand the embedded +0x1EC record to its
 * constructor. */
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
            if (fn_800CF208() != 2) {
                if (self->field_0x1F0 != 0) {
                    fn_80272E30(self->worker, self->field_0x1F0, 1);
                }
                if (self->field_0x1F2 != 0) {
                    fn_80272E30(self->worker, self->field_0x1F2, 1);
                }
            }
        }
    }
}

/* ---------------------------------------------------------------------------------------------------
 * The unwritten bodies, in address order.  Each notes the shape its target's codegen proves where
 * that shape is not obvious from the call list.
 * --------------------------------------------------------------------------------------------------- */

/* 0x80308FB4 - the equip-information screen's per-frame dispatch on its page mode: build the two
 * sub-screens for modes 0 and 1, or refresh the detail page for the modes 2..10 with the mode's own
 * value as the page delta. */
void equip_info_update(StatusScreenWork* self) {
    u16* table;
    _mh_ivec2_ pos;

    switch (self->field_0x1AE) {
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
        equip_page_count_step(self->equip, 0, 0, self->field_0x1AE - 2);
        equip_detail_page_refresh((EquipWork*)self->equip, NULL, self->field_0x1AE, self->field_0x1AF,
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
void put_equip_panel_row_variant(EquipWork* s0, _EQUIP* s1, _EQUIP* s2, _EQUIP* s3, u16 a, u8 b, u8 c) {
    u8 out[24];

    if (equip_variant_resolve_ex(s0, s1, s2, s3, b, out) == 1) {
        return;
    }
    put_equip_panel_row_ex(s0, out, a, b, c);
}

/* 0x80311840 - the `put_equip_panel_row_base` panel's entry whose resolve word is forced to zero and whose flag
 * byte keeps only its low three bits. */
void put_equip_panel_row_zero(EquipWork* s0, _EQUIP* s1, u16 a, u8 b) {
    u8 out[24];
    u8 flag = b;

    if (equip_variant_resolve(s0, s1, 0, &flag, out) == 1) {
        return;
    }
    flag &= 7;
    put_equip_panel_row_base(s0, out, a, flag);
}

/* 0x803118B4 - the `put_equip_panel_row_base` panel's six-register resolve entry. */
void put_equip_panel_row(EquipWork* s0, _EQUIP* s1, _EQUIP* s2, _EQUIP* s3, u16 a, u8 b) {
    u8 out[24];

    if (equip_variant_resolve_ex(s0, s1, s2, s3, b, out) == 1) {
        return;
    }
    put_equip_panel_row_base(s0, out, a, b);
}
