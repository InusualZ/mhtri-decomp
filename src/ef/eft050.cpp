/* ef/eft050.cpp - the `eft050` effect family and the cockpit item-hold band it spawns from,
 * `.text` 0x8033F270..0x803432B4 (46 functions, 16452 B).
 *
 * Home and name, evidence class 2 (a real runtime-dump name).  The map already carries the dump's own
 * definition `eft050_set__FP4_PLWPQ34nw4r4math4VEC3Uc` at 0x80342F34 (`python tools/symbols/dumpmap.py
 * lookup 0x80342F34` answers it, and it is the range's only name the dump knows - every other address
 * answers the dump's `zz_XXXXXXXX_` placeholder).  So the module is `ef` and the file follows the
 * `eft00X.cpp` scheme of the registered neighbours - `ef/eft035.cpp` is the same shape: the dump's
 * `eft035_set`/`eft035_set2` named that TU and its band likewise mixes cockpit-HUD functions with the
 * effect family.  Language: the range defines a mangled name and its callees are mangled names, so
 * the file is `.cpp`; every definition whose map name is plain (`fn_XXXXXXXX`) is `extern "C"` so its
 * emitted name stays the map's stem and objdiff can pair it by name (playbook row 42).
 *
 * Sections.  `.text` 0x8033F270..0x803432B4, extab 0x80016CA4..0x80016DB4 and extabindex
 * 0x8003615C..0x800362F4 (34 eight- and twelve-byte records - exactly the bytes the bracketing units
 * leave unclaimed: `auto_fn_8033F13C_text` ends extab at 0x80016CA4 / extabindex at 0x8003615C and
 * `auto_fn_803432B4_text` starts at 0x80016DB4 / 0x800362F4).  No `.ctors`/`.dtors` word belongs to
 * the range.  The range's own `.data` run 0x805E7400..0x805E7A70 (28 labels, `leak: 0`), its `.sdata`
 * 0x8079308C..0x807930F0 and its `.sdata2` 0x8079B2C0..0x8079B320 are `extern`-declared by their map
 * names and never defined (playbook 29); the jump table at 0x805E918C is this range's
 * (`_8034239cswitchdataD_805e918c` in the dump, i.e. `fn_8034235C`'s switch).
 *
 * rule 7 deferred: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/symedit.py range 0x8033F270 0x803432B4`: 45 of the 46 rows are bare `fn_`
 * stems, and `python tools/symbols/dumpmap.py lookup <addr>` answers `zz_XXXXXXXX_` for every address
 * but 0x80342F34).
 *
 * What it is.  The cockpit item-hold band (0x8033F270..0x8033FDA8) draws the player's item hold: its
 * 2D elements come from the `hud` library (`draw_sprite_ary`, `draw_sprite_idx`, `draw_sprite_anim_ary`,
 * `draw_itemicon_item_id`, `fn_802E0DA8`, `get_lsp_data`, `set_blendmode`, `get_now_areano`), it reads
 * the shared block `lbl_80794880` and the `_PLW` weapon state through `Pl_act_ck`/`Pl_Skill_ck`/
 * `Pl_master_ck`.  The rest is the `eft050` family: `eft050_set` pools the effect record, installs its
 * `release_0x40`/`dispatch_0x34` hooks (`fn_8034305C`, `fn_803430F4`) and its 100-byte work block, and
 * `fn_8033FDA8` is the second spawn entry the hold band calls.  Each spawner stamps `_EFT::type_0x02`,
 * the hooks and `_EFT::area_0x44`, seeds the pooled model handle (`fn_800F8914`) and installs the
 * `state_0x05` machine, whose arms are the model-create pass (`res_eft_model_create`), the alive body,
 * the state advance and the pool release (`fn_800F886C`).
 *
 * Seam (unproven, as the brief says).  This is one maximal unclaimed run.  `python
 * tools/splits/tudiscover.py at 0x8033F270` must-links only `fn_8033F270` and offers weak left
 * (`0x8033C384`/`0x8033C3AC`/`0x8033C448`) and weak right (`0x8033F40C`/`0x8033F560`/`0x8033F638`)
 * boundaries with no anchor, so no seam was cut and the extent settles as the functions match.  The
 * band may hold more than one original TU; the registration records that rather than guessing it.
 *
 * Residuals.  29 of the range's 46 functions are reconstructed (4132 B of the target's 16452 B;
 * `python tools/units/measure.py ef/eft050 --main .` reads 24.48 fuzzy, 14 of them byte-identical and
 * 29 at or above the 80 % bar, mean 97.98 % over the 29 scored).  All but one of the rest is
 * 92-99 % - the residual is register colouring, the owner header's relocation name and the two data
 * passes below, not a code shape:
 *
 *   byte-identical  fn_8033F638, fn_8033FEE8, fn_8033FF24, fn_803401B8, fn_803401C8, fn_803401E4,
 *                   fn_80340594, fn_80342504, fn_8034257C, fn_80342C60, fn_80342C70, fn_80342F20,
 *                   fn_80342F30, fn_803430F4
 *   92.93-99.42     the other 15 (fn_8033FD78 99.42, fn_80342AC0 99.20, fn_8033FF60 99.09,
 *                   fn_8033F270 99.03, fn_8033F6C4 97.96, fn_803401CC 96.67, fn_8033F40C 95.52,
 *                   fn_8033FDA8 95.31, fn_8033F788 95.27, fn_8033FA2C 94.67, `eft050_set` 94.51,
 *                   fn_8034305C 94.18, fn_8033F560 93.98, fn_80342E80 93.63, fn_80340218 92.93)
 *   unwritten       fn_8033FAA4 (0x2D4), fn_80340010 (0x1A8), fn_8034028C (0x19C),
 *                   fn_80340428 (0x16C), fn_803405C8 (0x288), fn_80340850 (0x460),
 *                   fn_80340CB0 (0xCE0), fn_80341990 (0x1DC), fn_80341B6C (0x1DC),
 *                   fn_80341D48 (0x24C), fn_80341F94 (0x3C8), fn_8034235C (0x1A8),
 *                   fn_803425C8 (0x2E0), fn_803428A8 (0x218), fn_80342B70 (0xF0),
 *                   fn_80342CAC (0x1D4), fn_80343130 (0x184)
 *
 * Two residuals are worth a follow-up rather than a per-function note:
 *
 *   * the range's own `.sdata` tables are `extern` here, so MWCC addresses them with `lis`/`addi`;
 *     the target object addresses them with `@sda21` (`li r26, lbl_8079308C@sda21` in fn_8033F40C,
 *     `li r31, lbl_80793098@sda21` in fn_8033F560, `li r5, lbl_807930A0@sda21` in fn_8033FA2C),
 *     which is what a TU that *defined* those small-data objects emits.  Matching it is the data
 *     pass: define the tables and claim `.sdata` 0x8079308C..0x807930F0 (and the `.sdata2` pool
 *     0x8079B2C0..0x8079B320 the two floats in fn_80342B70 read) - one relocation each.
 *   * `include/hud/layout.h` declares `get_lsp_data` inside its `extern "C"` block, so our calls
 *     relocate to the plain name `get_lsp_data` where the target holds the mangling
 *     `get_lsp_data__FUsP10_mh_ivec2_` (rule 9: the map name is a mangling, the declaration belongs
 *     at C++ scope).  That is the owner header's edit, recorded as a `shared-file` request in this
 *     unit's outbox; it costs a relocation-name diff on fn_8033F270/fn_8033F788/fn_8033FA2C.
 *
 * Unit-level residuals, measured: fn_8033F270 misses retail's `extsh` before the narrow `anchor.x`
 * store (one instruction in a 412-byte body); fn_80342E80 keeps `extsh`+`sth` where retail stores
 * the widened sum and narrows only for the compare; fn_80340218's `abs`/`min`/apply chain and
 * fn_8033F560's `extsb`-per-use loop colouring differ by register; `EFT::work_0x38`'s block is
 * typed per family (`Eft050Work`, `Eft051Work`, `Eft050SetWork`) from the offsets each family
 * reads, and the first family's count byte at +0x04 overlaps its handle list above a count of 1
 * (its spawner seeds 1).
 */

#include "types.h"
#include "nw4r/math.h"
#include "ef.h"
#include "ef/eft_res.h"
#include "ef/effect.h"
#include "pl.h"
#include "hud/cockpit_quest.h" /* hud/layout.h + pl.h: the 2D element library and its records */
#include "ef/fn_800CDB2C.h" /* push_g3d_wk */
#include "Runtime.PPCEABI.H/memset.h" /* memset - owner Runtime.PPCEABI.H/memset.c */

/* ---------------------------------------------------------------------------------------------------
 * the records this unit reads
 * ------------------------------------------------------------------------------------------------- */

/* The `.sbss` pointer 0x80794880 (the lobby band's owner header spells it a byte array; this band
 * loads the 4-byte pointer the map records) and the block it points at.  Only the bytes this band
 * reads are named, so the size is an approximation. size: 0x522A (approximation) */
typedef struct CockpitSharedDb {
    /* +0x0000 */ u8 unused_0x0000[0x3E03];
    /* +0x3E03 */ u8 player_slot_0x3E03; /* & 0x7F selects which player's hold is drawn */
    /* +0x3E04 */ u8 unused_0x3E04[0x13A0];
    /* +0x51A4 */ u8 clock_hour_0x51A4;
    /* +0x51A5 */ u8 clock_minute_0x51A5;
    /* +0x51A6 */ u8 unused_0x51A6[0x62];
    /* +0x5208 */ u16 hold_flag_0x5208;
    /* +0x520A */ u8 unused_0x520A[0x1E];
    /* +0x5228 */ u16 busy_flag_0x5228;
} CockpitSharedDb;

extern CockpitSharedDb* lbl_80794880;

/* One slot of the hold list `CockpitItemBlock::items_0x24` points at: the item id the icon is drawn
 * from and the word behind it. size: 0x4 */
typedef struct CockpitItemSlot {
    /* +0x00 */ u16 item_id_0x00;
    /* +0x02 */ u16 unused_0x02;
} CockpitItemSlot;

/* The `.bss` hold block at 0x806BF310, the record `fn_8033F788` walks (the map makes it 0x58 = 88
 * bytes; only the fields this band reads are named). size: 0x28 (approximation) */
typedef struct CockpitItemBlock {
    /* +0x00 */ u8 state_0x00;      /* 1/2 draw the selection frame, `fn_8033F788` tests `+0xFF` */
    /* +0x01 */ u8 unused_0x01[0x07];
    /* +0x08 */ u16 frame_id_0x08;  /* handed to `fn_802E0DA8` as the frame's part */
    /* +0x0A */ u16 sel_id_0x0A;    /* the same, for the two cursor sprites */
    /* +0x0C */ u16 sel_flag_0x0C;  /* set while a selection is drawn */
    /* +0x0E */ u16 cursor_0x0E;    /* the slot the cursor sits on */
    /* +0x10 */ u16 cursor2_0x10;   /* the slot the second cursor marker sits on */
    /* +0x12 */ u8 unused_0x12[0x12];
    /* +0x24 */ const CockpitItemSlot* items_0x24; /* the 50-slot hold list */
} CockpitItemBlock;

/* The two-line panel `fn_8033F270`/`fn_8033F40C`/`fn_8033F560` draw; `fn_8033F638` addresses the one
 * at +0x9C of its record.  The field names come from the sprite id each value is drawn at (the
 * `fn_802152A4(sprite, value, mode, pos)` calls). size: 0x1A */
typedef struct CockpitPanel {
    /* +0x00 */ u8 value_0x00;  /* drawn at sprite 0x201C */
    /* +0x01 */ u8 value_0x01;  /* 0x201D */
    /* +0x02 */ s16 count_0x02; /* + count_delta_0x04 -> 0x201E */
    /* +0x04 */ s16 count_delta_0x04;
    /* +0x06 */ s16 count_0x06; /* + count_delta_0x08 -> 0x201F */
    /* +0x08 */ s16 count_delta_0x08;
    /* +0x0A */ u8 icon_0x0A;   /* index into the icon table lbl_805E7180, 0xFF = none */
    /* +0x0B */ u8 unused_0x0B[0x01];
    /* +0x0C */ s16 gauge_0x0C; /* + gauge_delta_0x0E -> 0x2020 (>= 100 moves the panel) */
    /* +0x0E */ s16 gauge_delta_0x0E;
    /* +0x10 */ s16 timer_0x10; /* + timer_delta_0x12 -> 0x2022 */
    /* +0x12 */ s16 timer_delta_0x12;
    /* +0x14 */ u8 mode_0x14;   /* 0x2044 */
    /* +0x15 */ u8 unused_0x15[0x01];
    /* +0x16 */ s16 value_0x16; /* + value_delta_0x18 -> 0x205F */
    /* +0x18 */ s16 value_delta_0x18;
    /* +0x1A */ u8 label_0x1A;  /* + 0x1C2 is the `LbStr` id drawn at 0x2060 */
} CockpitPanel;

/* The record `fn_8033F638` walks: one panel at +0x9C of it. size: 0xB6 (approximation) */
typedef struct CockpitRecord {
    /* +0x00 */ u8 unused_0x00[0x9C];
    /* +0x9C */ CockpitPanel panel_0x9C;
} CockpitRecord;

/* The `eft050` family's own work block (the 100-byte block `fn_8033FDA8` pools).
 * `models_0x00` is the handle list `fn_800F8A44` releases from and `count_0x04` its used length; both
 * spawners seed 1, so the list holds exactly the one pooled model. size: 0x64 */
typedef struct Eft050Work {
    /* +0x00 */ MHchar* models_0x00[1]; /* the pooled handle list */
    /* +0x04 */ u8 count_0x04;          /* its used length */
    /* +0x05 */ u8 unused_0x05[0x03];
    /* +0x08 */ VEC3 pos_0x08;          /* the spawned position `fn_80041E40` copies in */
    /* +0x14 */ VEC3 spin_0x14;         /* the rotation the alive state advances */
    /* +0x20 */ u8 unused_0x20[0x04];
    /* +0x24 */ f32 scale_0x24;         /* the spawner's first scale argument */
    /* +0x28 */ u8 unused_0x28[0x08];
    /* +0x30 */ f32 scale_0x30;         /* the spawner's second scale argument */
    /* +0x34 */ u8 unused_0x34[0x08];
    /* +0x3C */ u32 param_0x3C;         /* the spawner's 5th argument */
    /* +0x40 */ u8 unused_0x40[0x20];
    /* +0x60 */ u8 flag_0x60; /* the spawner clears it; the alive body reads it */
} Eft050Work;

/* The family `fn_80342C70` drives (its own block: a word count at +0x00, the handles at +0x04, its
 * own timer at +0x0A, which its work block uses as an s16 countdown). size: 0x0C (approximation:
 * only the bytes this band reads are named) */
typedef struct Eft051Work {
    /* +0x00 */ s32 count_0x00;     /* the pooled handle list's used length */
    /* +0x04 */ MHchar* models_0x04[1]; /* the pooled handle list `fn_80342C60` releases */
    /* +0x08 */ u16 unused_0x08;
    /* +0x0A */ s16 timer_0x0A;     /* the countdown `fn_80342E80` runs to 0xB4 */
} Eft051Work;

/* The block `eft050_set` pools for the family `fn_803430F4` drives: a word count at +0x00, the four
 * pooled handles at +0x04, the extra record the fourth `fn_800F8914` hands back at +0x14 and the
 * eight-byte run the spawner zeroes at +0x1C. size: 0x24 (the bytes this spawner writes) */
typedef struct Eft050SetWork {
    /* +0x00 */ s32 count_0x00;
    /* +0x04 */ MHchar* models_0x04[4];
    /* +0x14 */ void* extra_0x14;   /* the record the model create fills; NULL retires the effect */
    /* +0x18 */ void* created_0x18; /* what `res_eft_UV_model_create` returned */
    /* +0x1C */ struct _g3d_work* works_0x1C[2]; /* the create's handle list, pushed back in reverse */
} Eft050SetWork;

/* The item record `fn_80340594` tests (a flag byte that disables the gauge and the item id).
 * size: 0x10 (approximation: only the bytes this band reads are named) */
typedef struct EftHoldItem {
    /* +0x00 */ u8 unused_0x00[0x0A];
    /* +0x0A */ u8 gauge_off_0x0A; /* nonzero: the id is not drawn as a gauge */
    /* +0x0B */ u8 unused_0x0B[0x01];
    /* +0x0C */ u16 item_id_0x0C;
} EftHoldItem;

/* One hold row of `fn_8033F40C`: the three sprite ids a row draws, left to right. size: 0x6 */
typedef struct CockpitRow {
    /* +0x00 */ u16 icons_0x00[3];
} CockpitRow;

/* The 0x14-byte-stride table `fn_803405C8` addresses by its signed index byte. size: 0x14 */
typedef struct Eft050Slot {
    /* +0x00 */ u8 bytes_0x00[0x14];
} Eft050Slot;

/* The record `fn_8034257C` reads its draw mode from and reaches the player work through.
 * size: 0x34 (approximation: only the bytes this band reads are named) */
typedef struct HoldOwner {
    /* +0x00 */ u8 unused_0x00[0x08];
    /* +0x08 */ s8 mode_0x08;   /* 2 selects the skill-dependent draw state */
    /* +0x09 */ u8 unused_0x09[0x27];
    /* +0x30 */ _PLW* plw_0x30;
} HoldOwner;

/* ---------------------------------------------------------------------------------------------------
 * callees no header carries (rule 2 debt: their owner ranges are unregistered).  The `fn_` stems keep
 * C linkage so the call pairs by name.
 * ------------------------------------------------------------------------------------------------- */

#ifdef __cplusplus
extern "C" {
#endif

void fn_802152A4(u16 id, s32 value, u32 mode, const _mh_ivec2_* pos);
VEC3* fn_80041E40(VEC3* dst, const VEC3* src);
void fn_80043EA8(VEC3* out);
void fn_8005050C(f32* out);
void fn_80073F68(VEC3* dst, const VEC3* src);
u32 fn_80050A40(f32 a, f32 b, f32 c, f32 d);
const u8* fn_802D773C(u8 index);
void fn_802D7754(u8 a, u8 b, u16* out);
const u8* lb_entry_id_get(u8 index);
s8* fn_802AA59C(u8 id);
s8* fn_802AA5D8(u8 index);
s8* fn_802AA650(u8 id);
void fn_80222BC4(void* work, u16 id, u32 arg);
void fn_8035A7D8(u16 id, const void* tbl, const void* pos, u16 id2, u32 arg);
void fn_8033F788(CockpitItemBlock* self);
/* this unit's own bodies, forward-declared (address order) */
void fn_8033F270(CockpitPanel* self, const _mh_ivec2_* pos);
void fn_8033F40C(CockpitPanel* self, const _mh_ivec2_* pos);
void fn_8033F560(CockpitPanel* self, const _mh_ivec2_* pos);
void fn_8033F638(CockpitRecord* self);
void fn_8033F6C4(s32 value, _mh_ivec2_* out);
void fn_8033FA2C(void);
s32 fn_8033FD78(void);
void fn_8033FEE8(_EFT* self);
void fn_8033FDA8(u8 type, VEC3* pos, f32 scale, f32 scale2, u32 param, u8 area);
void fn_8033FF24(_EFT* self);
void fn_8033FF60(_EFT* self);
void fn_80340010(_EFT* self);
void fn_803401B8(_EFT* self);
void fn_803401C8(_EFT* self);
Eft050Slot* fn_803401CC(s8 index);
u16 fn_803401E4(const VEC3* a, const VEC3* b);
s32 fn_80340218(s32* cur, u32* target, s32 step);
s32 fn_80340594(EftHoldItem* self);
void fn_80342504(MHchar* model, u32 index);
s32 fn_8034257C(HoldOwner* state, HoldOwner* owner);
void fn_80342AC0(u32 kind, VEC3* pos, f32 scale);
void fn_80342C60(_EFT* self);
void fn_80342C70(_EFT* self);
void fn_80342E80(_EFT* self);
void fn_80342F20(_EFT* self);
void fn_80342F30(_EFT* self);
void fn_803430F4(_EFT* self);
void fn_803401B8(_EFT* self);
void fn_803401C8(_EFT* self);
void fn_8034235C(_EFT* self, MHchar* model);
void fn_80342CAC(_EFT* self);
void fn_8034305C(_EFT* self);
void fn_80343130(_EFT* self);
void eft_instance_release(_EFT* self);
void fn_803432B4(_EFT* self);
void eft_state_advance(_EFT* self);
void eft_instance_release(_EFT* self);

/* This unit's own `.data` run (0x805E7400..0x805E7A70), its `.sdata` tables and its `.sdata2` pool -
 * referenced by name, never defined (playbook 29). */
extern const u16 lbl_805E7410[];
extern const u8 lbl_805E7180[];
extern const u16 lbl_805E7438[];
extern CockpitRow lbl_805E7450[];
extern const u16 lbl_805E7464[];
extern const u16 lbl_805E7498[];
extern const u16 lbl_805E74F0[];
extern const u16 lbl_805E7508[];
extern const u16 lbl_805E7514[];
extern const u16 lbl_805E7528[];
extern const u16 lbl_805E753C[];
extern Eft050Slot lbl_805E7578[];
extern const u16 lbl_805E7760[];
extern const u16 lbl_805E7794[];
extern const u16 lbl_8079308C[];
extern const u16 lbl_80793098[];
extern const u16 lbl_807930A0[];

/* The hold block at 0x806BF310 (`fn_8033FA2C` hands its address to `fn_8033F788`). */
extern CockpitItemBlock lbl_806BF310;

#ifdef __cplusplus
}
#endif

/* C++-scope callees (their map names are manglings, so the front-end has to reproduce them). */
s8* LbStr(u8 kind, u16 id);
void draw_font_idx(u16 id, s8* str, u32 mode, const _mh_ivec2_* pos);
u16 ran_suu(long index);
s32 chk_pointer(void);
u8 Pl_act_ck(_PLW* plw, u8 a, u16 b);
u32 Pl_Skill_ck(_PLW* plw, u16 id);
u8 Pl_master_ck(_PLW* plw);
void eft004_set(u8 type, nw4r::math::VEC3* pos, f32 scale, u32 param, u8 flag);

/* ---------------------------------------------------------------------------------------------------
 * bodies, in address order
 * ------------------------------------------------------------------------------------------------- */

/* Draws one hold slot's panel: the frame, the two counters with their deltas, the gauge bar and the
 * four value/timer pairs, each at its own sprite id. */
extern "C" void fn_8033F270(CockpitPanel* self, const _mh_ivec2_* pos)
{
    _mh_ivec2_ anchor = *pos;
    draw_sprite_ary(lbl_805E7410, pos);
    _SPR_DATA_ frame;
    fn_801E6850(&frame, get_lsp_data(0x2019, 0));
    frame.pos.x += 6;
    draw_sprite(frame, pos);
    if (self->icon_0x0A != 0) {
        u8 icon = lbl_805E7180[self->icon_0x0A];
        if (icon != 0xFF) {
            fn_802E1794((s16)(pos->x + 0x3A), (s16)(pos->y + 0x6E), 0x12, 0xFFFFFFFF, icon);
        }
        if (self->gauge_0x0C + self->gauge_delta_0x0E >= 0x64) {
            anchor.x = (s16)(anchor.x + 6);
        }
        fn_802152A4(0x2020, self->gauge_0x0C + self->gauge_delta_0x0E, 2, &anchor);
    } else {
        draw_font_idx(0x2021, LbStr(0, 0x1B8), 2, pos);
    }
    fn_802152A4(0x201C, self->value_0x00, 2, pos);
    fn_802152A4(0x201D, self->value_0x01, 2, pos);
    fn_802152A4(0x201E, self->count_0x02 + self->count_delta_0x04, 2, pos);
    fn_802152A4(0x201F, self->count_0x06 + self->count_delta_0x08, 2, pos);
    fn_802152A4(0x2022, self->timer_0x10 + self->timer_delta_0x12, 2, pos);
}

/* Draws the hold slot list the selected player carries: one row of three sprites and the entry's name
 * per item, or the "no items" label when the list is empty. */
extern "C" void fn_8033F40C(CockpitPanel* self, const _mh_ivec2_* pos)
{
    draw_sprite_ary(lbl_805E7438, pos);
    u8 sel = lbl_80794880->player_slot_0x3E03 & 0x7F;
    const u8* rows = fn_802D773C(sel);
    const u8* slots = lb_entry_id_get(sel);
    draw_font_idx(0x2043, fn_802AA5D8(sel), 1, pos);
    fn_802152A4(0x2044, self->mode_0x14, 1, pos);
    if (rows[slots[0] + 4] == 0) {
        draw_font_idx(0x2046, LbStr(0, 0x1C1), 1, pos);
    } else {
        for (s8 i = 0; i < rows[slots[0] + 4]; i++) {
            draw_sprite_idx(lbl_805E7450[i].icons_0x00[0], pos);
            draw_sprite_idx(lbl_805E7450[i].icons_0x00[1], pos);
            draw_sprite_idx(lbl_805E7450[i].icons_0x00[2], pos);
            draw_font_idx(lbl_8079308C[i], fn_802AA59C(slots[i + 1]), 1, pos);
        }
    }
}

/* Draws the hold's gauge row, its label and the four clock digits the shared block carries. */
extern "C" void fn_8033F560(CockpitPanel* self, const _mh_ivec2_* pos)
{
    draw_sprite_ary(lbl_805E7464, pos);
    fn_802152A4(0x205F, self->value_0x16 + self->value_delta_0x18, 1, pos);
    draw_font_idx(0x2060, LbStr(0, (u16)(self->label_0x1A + 0x1C2)), 1, pos);
    u16 digits[4];
    fn_802D7754(lbl_80794880->clock_hour_0x51A4, lbl_80794880->clock_minute_0x51A5, digits);
    for (u16 i = 0; i < 4; i++) {
        draw_font_idx(lbl_80793098[i], fn_802AA650((u8)digits[i]), 0, pos);
    }
}

/* Draws the three panels of one hold record: the frame sprite, then each panel at the anchor the
 * preceding `get_lsp_data` wrote. */
extern "C" void fn_8033F638(CockpitRecord* self)
{
    _mh_ivec2_ anchor;
    get_lsp_data(0x1FE1, &anchor);
    draw_sprite_ary(lbl_805E7498, &anchor);
    get_lsp_data(0x200D, &anchor);
    fn_8033F270(&self->panel_0x9C, &anchor);
    get_lsp_data(0x200E, &anchor);
    fn_8033F40C(&self->panel_0x9C, &anchor);
    get_lsp_data(0x200F, &anchor);
    fn_8033F560(&self->panel_0x9C, &anchor);
}

/* Places the two digit sprites of a two-digit value: the ones digit's for `value % 10` and the tens
 * digit's for `value / 10`, then writes the ones digit's anchor, moved right by 4, into `out`. */
extern "C" void fn_8033F6C4(s32 value, _mh_ivec2_* out)
{
    const _SPR_DATA_* ones = get_lsp_data(lbl_805E74F0[value % 10], 0);
    const _SPR_DATA_* tens = get_lsp_data(lbl_805E7508[value / 10], 0);
    out->x = (s16)(ones->pos.x + 4);
    out->y = tens->pos.y;
}

/* Draws the hold list of the block at 0x806BF310: its frame, its 50 item icons with the selected
 * slot's frame around them, and - while a selection is active - the two cursor sprites. */
extern "C" void fn_8033F788(CockpitItemBlock* self)
{
    _mh_ivec2_ anchor;
    get_lsp_data(0x2081, &anchor);
    draw_sprite_anim_ary(lbl_805E7514, 0, &anchor);
    s32 found = 0;
    _SPR_DATA_ frame;
    fn_801E6850(&frame, get_lsp_data(0x1307, 0));
    _mh_ivec2_ frame_pos;
    fn_800526F8(&frame_pos, &frame);
    _mh_ivec2_ cursor = { 0, 0 };
    for (s32 i = 0; i < 0x32; i++) {
        _mh_ivec2_ digit;
        fn_8033F6C4(i, &digit);
        u16 id = (i < 0x14) ? 0x2092 : 0x209B;
        _SPR_DATA_ rec;
        fn_801E6850(&rec, get_lsp_data(id, 0));
        fn_800526F8(&rec, &digit);
        frame.pos.x = (s16)(digit.x + frame_pos.x);
        frame.pos.y = (s16)(digit.y + frame_pos.y);
        draw_sprite(rec, &anchor);
        if (self->cursor_0x0E == i) {
            found = 1;
            fn_800526F8(&cursor, &digit);
        }
        if ((u8)(self->state_0x00 + 0xFF) <= 1) {
            if (chk_pointer() == 0) {
                if (self->cursor2_0x10 == i) {
                    _SPR_DATA_ sel;
                    fn_801E6850(&sel, get_lsp_data(0x1304, 0));
                    sel.pos.x = (s16)(sel.pos.x + digit.x);
                    sel.pos.y = (s16)(sel.pos.y + digit.y);
                    set_blendmode(4, 1, 1);
                    fn_802E0DA8(&sel, self->frame_id_0x08, &anchor);
                    set_blendmode(4, 5, 1);
                }
            }
        }
        u16 item = self->items_0x24[i].item_id_0x00;
        if (item != 0) {
            draw_itemicon_item_id(frame, item, &anchor);
        }
    }
    if (self->sel_flag_0x0C != 0 && found != 0) {
        set_blendmode(4, 1, 1);
        _SPR_DATA_ sel;
        fn_801E6850(&sel, get_lsp_data(0x1305, 0));
        sel.pos.x = (s16)(sel.pos.x + cursor.x);
        sel.pos.y = (s16)(sel.pos.y + cursor.y);
        fn_802E0DA8(&sel, self->sel_id_0x0A, &anchor);
        set_blendmode(4, 5, 1);
        fn_801E6850(&sel, get_lsp_data(0x1306, 0));
        sel.pos.x = (s16)(sel.pos.x + cursor.x);
        sel.pos.y = (s16)(sel.pos.y + cursor.y);
        fn_802E0DA8(&sel, self->sel_id_0x0A, &anchor);
    }
}

/* Draws the hold's own frame and the item-icon panel the shared block's selector points at. */
extern "C" void fn_8033FA2C(void)
{
    _mh_ivec2_ anchor;
    get_lsp_data(0x2066, &anchor);
    draw_sprite_ary(lbl_805E753C, &anchor);
    fn_8033F788(&lbl_806BF310);
    fn_80222BC4(&lbl_806BF310, 0x2090, 0);
    fn_8035A7D8(0x209F, lbl_805E7528, lbl_807930A0, 0x2065, 0);
}

/* Reports the hold's state: 0 while the busy flag is set, -1 while the hold flag is clear, 1
 * otherwise. */
extern "C" s32 fn_8033FD78(void)
{
    CockpitSharedDb* db = lbl_80794880;
    if (db->busy_flag_0x5228 != 0) {
        return 0;
    }
    if (db->hold_flag_0x5208 == 0) {
        return -1;
    }
    return 1;
}

/* Releases the pooled handle list of a work block and clears its used length. */
extern "C" void fn_8033FEE8(_EFT* self)
{
    Eft050Work* work = (Eft050Work*)self->work_0x38;
    fn_800F8A44(work->models_0x00, work->count_0x04);
    work->count_0x04 = 0;
}

/* Spawns the family's effect for one area: rejects a foreign area, pools the record and its work
 * block, seeds the model handle, stamps the type/area and the two hooks, then installs the state
 * machine through `fn_800F9DF4`. */
extern "C" void fn_8033FDA8(u8 type, VEC3* pos, f32 scale, f32 scale2, u32 param, u8 area)
{
    if ((u8)get_now_areano() != (u8)area) {
        return;
    }
    _EFT* eft = (_EFT*)fn_800F8788(0x64);
    if (eft == NULL) {
        return;
    }
    eft->type_0x02 = type;
    eft->release_0x40 = fn_8033FEE8;
    eft->dispatch_0x34 = fn_8033FF24;
    Eft050Work* work = (Eft050Work*)eft->work_0x38;
    work->count_0x04 = 1;
    s32 i = 0;
    MHchar** p = work->models_0x00;
    while (i < work->count_0x04) {
        *p = (MHchar*)fn_800F8914();
        if (*p == NULL) {
            fn_800F886C(eft);
            return;
        }
        p++;
        i++;
    }
    work->scale_0x24 = scale;
    work->param_0x3C = param;
    work->flag_0x60 = 0;
    work->scale_0x30 = scale2;
    fn_80041E40(&eft->pos_0x18, fn_80041E40(&work->pos_0x08, pos));
    eft->rot_0x24.x = 0;
    eft->rot_0x24.y = 0;
    eft->rot_0x24.z = 0;
    eft->field_0x03 = 0x30;
    eft->area_0x44 = area;
    eft->flag_0x01 = 1;
    fn_800F9DF4(eft, 0, 0);
}

/* The state machine of the effect `fn_8033FDA8` spawns: 0 the model-create pass, 1 the alive body, 2
 * the state advance and 3 the pool release. */
extern "C" void fn_8033FF24(_EFT* self)
{
    switch (self->state_0x05) {
    case 0: return fn_8033FF60(self);
    case 1: return fn_80340010(self);
    case 2: return fn_803401B8(self);
    case 3: return fn_803401C8(self);
    }
}

/* State 0: creates the family's model for every pooled handle and hands each to the alive-state body,
 * or retires the effect when the create fails, then advances to state 1. */
extern "C" void fn_8033FF60(_EFT* self)
{
    Eft050Work* work = (Eft050Work*)self->work_0x38;
    self->state_0x05++;
    self->field_0x06 = 0;
    s32 i = 0;
    MHchar** p = work->models_0x00;
    while (i < work->count_0x04) {
        if (res_eft_model_create(*p, 0x44, 0xC) == NULL) {
            fn_803401C8(self);
            return;
        }
        fn_8034235C(self, *p);
        p++;
        i++;
    }
    fn_80340010(self);
}

/* State 2 of the family's state machine: advances the state counter to the next body. */
extern "C" void fn_803401B8(_EFT* self)
{
    self->state_0x05++;
}

/* State 3 of the family's state machine: releases the pooled effect record. */
extern "C" void fn_803401C8(_EFT* self)
{
    fn_800F886C(self);
}

/* Addresses one 0x14-byte slot of the table by its signed index. */
extern "C" Eft050Slot* fn_803401CC(s8 index)
{
    return &lbl_805E7578[index];
}

/* The squared distance of the two points' x/z pairs, narrowed to the u16 the caller wants. */
extern "C" u16 fn_803401E4(const VEC3* a, const VEC3* b)
{
    return (u16)fn_80050A40(a->x, a->z, b->x, b->z);
}

/* Moves `cur` towards `target` by at most `step`, returning the step actually applied. */
extern "C" s32 fn_80340218(s32* cur, u32* target, s32 step)
{
    *target = (u16)*target;
    s32 delta = (u16)(*target - *cur);
    s32 mag = (delta >= 0x8000) ? -(s16)delta : delta;
    if (mag < step) {
        step = mag;
    }
    if (delta >= 0x8000) {
        *cur -= step;
    } else {
        *cur += step;
    }
    return step;
}

/* Whether the record's item id is one of the five ids the hold bar draws as a gauge. */
extern "C" s32 fn_80340594(EftHoldItem* self)
{
    if (self->gauge_off_0x0A == 0) {
        if ((u16)(self->item_id_0x0C + 0xFFC1) <= 4) {
            return 1;
        }
    }
    return 0;
}

/* Shows or hides the model's twelve joints, leaving the one `index` visible. */
extern "C" void fn_80342504(MHchar* model, u32 index)
{
    for (s32 i = 1; i <= 0xC; i++) {
        if (i == index) {
            model->setVisibility(i, TRUE);
        } else {
            model->setVisibility(i, FALSE);
        }
    }
}

/* The hold slot's draw state: 1 normally, 2/3 while the player carries skill 0x95. */
extern "C" s32 fn_8034257C(HoldOwner* state, HoldOwner* owner)
{
    return (state->mode_0x08 == 2) ? (Pl_Skill_ck(owner->plw_0x30, 0x95) == 1) + 2 : 1;
}

/* Spawns five of the family's effects around one position, one per rotation step.  Retail keeps the
 * unfused countdown (`subi` + `cmpwi`, not the peephole's `subic.`), so the pass is scoped off here. */
#pragma peephole off
extern "C" void fn_80342AC0(u32 kind, VEC3* pos, f32 scale)
{
    const u16* ids;
    switch ((u8)kind) {
    case 0:
        ids = lbl_805E7760;
        break;
    case 12:
        ids = lbl_805E7794;
        break;
    default:
        return;
    }
    for (s32 i = 5; i > 0; i--) {
        fn_8033FDA8(1, pos, scale, scale, (u32)ids, kind);
    }
}
#pragma peephole on

/* Releases a second family's pooled handle list, from its word behind the count. */
extern "C" void fn_80342C60(_EFT* self)
{
    Eft051Work* work = (Eft051Work*)self->work_0x38;
    fn_800F8A44(work->models_0x04, work->count_0x00);
}

/* The second family's state machine: 0 the model-create pass, 1 the alive body, 2 the state advance
 * and 3 the pool release. */
extern "C" void fn_80342C70(_EFT* self)
{
    switch (self->state_0x05) {
    case 0: return fn_80342CAC(self);
    case 1: return fn_80342E80(self);
    case 2: return fn_80342F20(self);
    case 3: return fn_80342F30(self);
    }
}

/* State 1 of the second family: retires the effect once its timer runs past 0xB4, otherwise moves
 * every pooled model to its idle state and updates it. */
extern "C" void fn_80342E80(_EFT* self)
{
    Eft051Work* work = (Eft051Work*)self->work_0x38;
    s16 timer = (s16)(work->timer_0x0A + 1);
    work->timer_0x0A = timer;
    if (timer > 0xB4) {
        fn_80342F30(self);
        return;
    }
    for (s32 i = 0; i < work->count_0x00; i++) {
        work->models_0x04[i]->move(0);
        fn_800F93D8(self, (void**)&work->models_0x04[i], 2, 1, NULL);
    }
}

/* State 2 of the second family's state machine: advances to the next body. */
extern "C" void fn_80342F20(_EFT* self)
{
    self->state_0x05++;
}

/* State 3 of the second family's state machine: releases the pooled effect record. */
extern "C" void fn_80342F30(_EFT* self)
{
    fn_800F886C(self);
}

/* Releases the `eft050` family's pools: every model handle one at a time, the two `_g3d_work`
 * handles in reverse order, then the record the create filled. */
extern "C" void fn_8034305C(_EFT* self)
{
    Eft050SetWork* work = (Eft050SetWork*)self->work_0x38;
    for (s32 i = 0; i < work->count_0x00; i++) {
        fn_800F8A44(&work->models_0x04[i], 1);
    }
    for (s32 i = 1; i >= 0; i--) {
        if (work->works_0x1C[i] != NULL) {
            push_g3d_wk(work->works_0x1C[i]);
        }
    }
    fn_800F8A44(&work->extra_0x14, 1);
}

/* Pools the `eft050` record: the effect, its work block, four pooled model handles, the extra record
 * the fourth pool call hands back, and the position/source/area seeds. */
void eft050_set(_PLW* plw, nw4r::math::VEC3* pos, u8 flag)
{
    _EFT* eft = (_EFT*)fn_800F8788(0x2C);
    if (eft == NULL) {
        return;
    }
    eft->type_0x02 = 0;
    eft->dispatch_0x34 = fn_803430F4;
    eft->release_0x40 = fn_8034305C;
    Eft050SetWork* work = (Eft050SetWork*)eft->work_0x38;
    work->count_0x00 = 4;
    s32 i = 0;
    MHchar** p = work->models_0x04;
    while (i < work->count_0x00) {
        *p = (MHchar*)fn_800F8914();
        if (*p == NULL) {
            fn_800F886C(eft);
            return;
        }
        p++;
        i++;
    }
    work->extra_0x14 = fn_800F8914();
    if (work->extra_0x14 == NULL) {
        fn_800F886C(eft);
        return;
    }
    memset(work->works_0x1C, 0, 8);
    eft->field_0x03 = 0x32;
    fn_800F9DF4(eft, 1, 0);
    fn_80041E40(&eft->pos_0x18, pos);
    eft->area_0x44 = flag;
    eft->source_0x30 = plw;
    eft->flag_0x01 = 1;
    eft->timer_0x0C = 0;
    eft->field_0x10 = 0;
}

/* The `eft050` family's own state machine (its three bodies sit in the band above this one). */
extern "C" void fn_803430F4(_EFT* self)
{
    switch (self->state_0x05) {
    case 0: return fn_80343130(self);
    case 1: return fn_803432B4(self);
    case 2: return eft_state_advance(self);
    case 3: return eft_instance_release(self);
    }
}
