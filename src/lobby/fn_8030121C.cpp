/* The lobby UI band, `.text` 0x8030121C..0x803066F0 (21 716 B, 52 map symbols).
 *
 * Registered once, at its final home (docs/plan.md 12) from proposal `8030121C_fn_8030121C.cpp`.
 * Module `lobby` (class 3): the range's own predicates read the lobby pointer block `lbl_80794880`
 * (`.sbss`), and its callees are the lobby/menu UI API - `LbStr__FUcUs` (18 call sites),
 * `get_lsp_data__FUsP10_mh_ivec2_` (58), `draw_sprite_ary__FPCUsPC10_mh_ivec2_` (16),
 * `draw_font_idx__FUsPScUlPC10_mh_ivec2_` (14), `put_menu_cursor__FPUsUsPC10_mh_ivec2_` (5),
 * `GetMenuFontColor__Fbbbb` (4), `ItemName__FUs`.  No `__FILE__` string covers the range and the
 * runtime dump answers `zz_` for every in-range address, so the file keeps the map's stem - brief
 * section 2, class 4.
 *
 * rule 7 deferred: the symbol map has only fn_XXXXXXXX for this range (checked with `python
 * tools/symbols/symedit.py range 0x8030121C 0x803066F0`), so every definition below keeps the map's
 * own name.
 *
 * Seam: the proposal's edges are the discovery `--max-bytes` cap, not a proven TU boundary; the
 * supports are the `.sdata2` pool-run boundary at 0x8079AD98..0x8079ADC8 (a run no other unit
 * references) and the extab/extabindex runs, which are exactly the gaps between the bracketing
 * registered units (extab 0x80015964..0x80015AAC, extabindex 0x8003447C..0x80034668).
 *
 * Flags: the `lobby` lib's `cflags_lobby` (`-O3`, `-inline noauto`), plus `#pragma exceptions on`
 * because `cflags_lobby` turns exceptions off and the lobby target objects carry the unwind records.
 */
#include "types.h"
#include "ef.h"
#include "ef/eft_res.h"
#include "ef/effect.h"
#include "ef/fn_800CDB2C.h"
#include "hud/layout.h"
#include "ai/fn_802D0F34.h"
#include "Runtime.PPCEABI.H/memset.h"

/* The effect-pool release the unsplit band declares (`include/unsplit/unknown.h`); its owner is an
 * unclaimed address, so the rule-2 named gap applies. */
namespace nw4r {
namespace ef {
struct Effect;
}
}  // namespace nw4r
void push_eft_effect_heap_num(nw4r::ef::Effect** effects, long count);
/* `include/main.h` cannot be included here: its `_GXChannelID` enum clashes with the one `ef.h` pulls
 * in.  `ck_WideMode` is a C++ free function (`ck_WideMode__Fv`), so the declaration is at C++ scope. */
int ck_WideMode(void);

/* `sprintf` is a libc intrinsic; `-nosyspath` means it has to be declared at the use site (the lobby
 * units do the same).  `draw_font_idx` (`draw_font_idx__FUsPScUlPC10_mh_ivec2_`) and `put_frame_dialog`
 * are unclaimed band addresses - rule 2's named unsplit gap - so they are declared here. */
int sprintf(char* dst, const char* fmt, ...);
void draw_font_idx(u16, s8*, u32, const _mh_ivec2_*);
/* `LbStr__FUcUs` - owned by `src/lobby/fn_8020C588.cpp`, whose header cannot be included here (it
 * declares `fn_800D0708` as `s32`, which clashes with `ef/fn_800CDB2C.h`'s `u32`). */
void* LbStr(u8 kind, u16 idx);

/* The effect-41/42 spawners' pooled work block (`_EFT::work_0x38`) as this band views it.  The count
 * and the two parallel arrays the machines walk: the `effect` slots at +0x04 (stride 0x8, handed to
 * `push_eft_effect_heap_num` / `fn_800F8A44` as a block) and the four g3d part handles at +0x0C
 * (stride 0x10).  size: 0x20 (approximate: the pooled block is larger). */
typedef struct Eft8030EffectSlot {
    /* +0x000 */ nw4r::ef::Effect* effect;
    /* +0x004 */ u8 pad_0x004[0x4];
} Eft8030EffectSlot; /* size: 0x8 */

typedef struct Eft8030ModelSlot {
    /* +0x000 */ void* parts_0x000[4];
} Eft8030ModelSlot; /* size: 0x10 */

typedef struct Eft8030Work {
    /* +0x000 */ s32 count;
    /* +0x004 */ Eft8030EffectSlot slots_0x004[1];
    /* +0x00C */ Eft8030ModelSlot models_0x00C[1];
    /* +0x01C */ u8* source_0x01C;
} Eft8030Work; /* size: 0x20 (approximate: the pooled block is larger) */

#pragma exceptions on
#pragma peephole off

extern "C" {

/* ------------------------------------------------------------------------------------------------
 * The range's own entry points, declared here because the callers precede the definitions (rule 2:
 * this TU owns them).  Signatures are the call sites' view; they are refined as the bodies land.
 * ------------------------------------------------------------------------------------------------ */
void fn_803032EC(void* self);
void* fn_80304550(void* a, u8 b, u32 c, u32 d);
void fn_8030487C(u8* self);
void fn_803051E8(u8* self);
void fn_80305910(u8* self);
void fn_80305920(void* self);
void fn_80306614(_EFT* self);
void fn_803066B4(_EFT* self);

/* Unclaimed band addresses (rule 2's named unsplit gap - no registered unit owns them yet). */
/* `include/stage/fn_802B2AA0.h` cannot be included here: it pulls `sound/mhchar.h`, whose
 * `_GXChannelID` enum clashes with the one `pl.h` (via `hud/layout.h`) defines. */
u8* fn_802B04A0(u8 kind);
s32 fn_80222BC4(u8*, s32, u8);
s32 fn_8035A7D8(s32, const void*, const void*, s32, s32);

/* C-linkage object/data the range reads that no registered unit has claimed (rule 2's unsplit home;
 * these sit in the range's own unclaimed `.data`/`.sbss` runs). */
extern void* lb_tr_flag_data;             /* .sbss 0x80794AF4 - the `_tr_flag` table base */
extern void* lbl_80794880;                /* .sbss - the lobby pointer block */
extern const u16 lbl_805DB120[];          /* .data 0x805DB120 */
extern const u16 lbl_805DB1B0[];          /* .data 0x805DB1B0 */
extern const u16 lbl_805DB1D8[];          /* .data 0x805DB1D8 */
extern const u16 lbl_805DB1F0[];          /* .data 0x805DB1F0 */
extern const u16 lbl_805DB2A0[];          /* .data 0x805DB2A0 */
extern const u16 lbl_805DB088[];          /* .data 0x805DB088 */
extern const u16 lbl_805DB018[];          /* .data 0x805DB018 */
extern const u16 lbl_805DB024[];          /* .data 0x805DB024 */
extern const u16 lbl_805DAFE8[];          /* .data 0x805DAFE8 */
extern u16 lbl_80792A68;                 /* .sdata 0x80792A68 */
extern u16 lbl_80792A70;                 /* .sdata 0x80792A70 */
extern char lbl_80792A78;                /* .sdata 0x80792A78 - a 7-byte string */
extern u8 lbl_806BF310[];                 /* .bss 0x806BF310, 0x58 B */

/* `src/lobby/fn_802FA9A0.cpp` owns these addresses (the band below); declared in its header once the
 * bodies land, so this file reaches them through that header. */
void fn_802FF7F4(void* self);
void fn_802FFE70(void* self);

/* ------------------------------------------------------------------------------------------------
 * bodies, in address order
 * ------------------------------------------------------------------------------------------------ */

/* 0x8030170C - the band's reset entry: clears the state byte and the +0x58 byte, then runs the two
 * `fn_8030xxxx` teardown steps on `self`.  r3 is `self` throughout (the callee keeps it). */
void fn_8030170C(void* self) {
    u8* p = (u8*)self;
    p[0x01] = 0;
    p[0x58] = 0;
    fn_802FF7F4(self);
    fn_802FFE70(self);
}

/* 0x80302228 - draws the selection-window frame after fetching the `0x23B6` sprite's position; the
 * frame sprite is drawn only in the +0x58 == 1 state. */
void fn_80302228(u8* self) {
    _mh_ivec2_ pos;

    get_lsp_data(0x23B6, &pos);
    if (self[0x58] == 1) {
        draw_sprite_ary(&lbl_80792A68, &pos);
    }
}

/* 0x80302274 - draws the page's tab strip: sprite `0x23B6`/`0x23D7` depending on `sel`, with the
 * `lbl_805DB088` step table and, in the three idle states, the `lbl_80792A70` overlay. */
void fn_80302274(u8* self, u32 sel) {
    _mh_ivec2_ pos;
    u32 index;
    u16 id;

    if ((u8)sel == 0) {
        index = 0;
        id = 0x23B6;
    } else {
        index = 1;
        id = 0x23D7;
    }
    get_lsp_data(id, &pos);
    draw_sprite_anim_ary(lbl_805DB088, index, &pos);
    if (self[0x01] == 3 || self[0x01] == 5 || self[0x01] == 0) {
        draw_sprite_anim_ary(&lbl_80792A70, index, &pos);
    }
}

/* 0x80302754 - draws the page arrow (sprite `0x23D8`, animation 0). */
void fn_80302754(void) {
    _mh_ivec2_ pos;

    get_lsp_data(0x23D8, &pos);
    draw_sprite_anim_ary(lbl_805DB120, 0, &pos);
}

/* 0x80302844 - draws the same page arrow's second phase (sprite `0x23CD`, animation 1). */
void fn_80302844(void) {
    _mh_ivec2_ pos;

    get_lsp_data(0x23CD, &pos);
    draw_sprite_anim_ary(lbl_805DB120, 1, &pos);
}

/* 0x8030326C - a digit's screen anchor: index `index % 10` selects one of the ten anchors in
 * `lbl_805DB1B0`, and the result is offset by (+4, +2). */
void fn_8030326C(s32 index, _mh_ivec2_* out) {
    _mh_ivec2_ pos;

    get_lsp_data(lbl_805DB1B0[index % 10], &pos);
    out->x = pos.x + 4;
    out->y = pos.y + 2;
}

/* 0x80303580 - the page's static furniture pass: sprite `0x2469` then the `lbl_806BF310` overlay,
 * closing with the `0x2497` string banner. */
void fn_80303580(void) {
    _mh_ivec2_ pos;

    get_lsp_data(0x2469, &pos);
    draw_sprite_ary(lbl_805DB1F0, &pos);
    fn_803032EC(lbl_806BF310);
    fn_80222BC4(lbl_806BF310, 0x2468, 0);
    fn_8035A7D8(0x2497, lbl_805DB1D8, &lbl_80792A78, 0x2467, 0);
}

/* 0x803035F8 - finds the `_tr_flag` record whose leading id matches `id`, or returns the table base
 * when the 0xFFFF terminator is reached.  Records are 0xC bytes. */
void* fn_803035F8(u32 id) {
    u8* base = (u8*)lb_tr_flag_data;
    u8* p = base;
    u16 want = (u16)id;
    u16 cur;

    while ((cur = *(u16*)p) != 0xFFFF) {
        if (cur == want) {
            return p;
        }
        p += 0xC;
    }
    return base;
}

/* 0x80304508 - thunk to fn_80304550 with the argument narrowed to a byte. */
void* fn_80304508(void* a, u32 b, u32 c, u32 d) {
    return fn_80304550(a, (u8)b, c, d);
}

/* 0x80304510 - fn_80304550 with the argument narrowed, then stores `value` at the returned effect's
 * work block +0x1C. */
void fn_80304510(void* a, u32 b, u32 c, u32 d, u32 value) {
    _EFT* rec = (_EFT*)fn_80304550(a, (u8)b, c, d);
    if (rec != NULL) {
        Eft8030Work* w = (Eft8030Work*)rec->work_0x38;
        w->source_0x01C = (u8*)value;
    }
}

/* 0x80304804 - retires the effect's pooled handles and clears its count. */
void fn_80304804(_EFT* self) {
    Eft8030Work* w = (Eft8030Work*)self->work_0x38;
    push_eft_effect_heap_num(&w->slots_0x004[0].effect, w->count);
    w->count = 0;
}

/* 0x80304840 - the effect machine's state dispatcher (state at +0x05). */
void fn_80304840(u8* self) {
    switch (self[0x05]) {
    case 0:
        fn_8030487C(self);
        break;
    case 1:
        fn_803051E8(self);
        break;
    case 2:
        fn_80305910(self);
        break;
    case 3:
        fn_80305920(self);
        break;
    }
}

/* 0x80305910 - state-2 step: advance the state byte. */
void fn_80305910(u8* self) {
    self[0x05]++;
}

/* 0x80305920 - state-3 step: retire the effect. */
void fn_80305920(void* self) {
    fn_800F886C(self);
}

/* 0x80306510 - state step: advance the state byte. */
void fn_80306510(u8* self) {
    self[0x05]++;
}

/* 0x80306520 - retire the effect. */
void fn_80306520(void* self) {
    fn_800F886C(self);
}

/* 0x80301A20 - draws the page header row (`0x231B`/`0x2316` by wide mode) and, while the +0x00
 * value is <= 1, the `0x22FF` overlay. */
void fn_80301A20(u8* self) {
    _mh_ivec2_ pos;

    if (ck_WideMode()) {
        get_lsp_data(0x231B, &pos);
        draw_sprite_ary(lbl_805DB024, &pos);
    } else {
        get_lsp_data(0x2316, &pos);
        draw_sprite_ary(lbl_805DB018, &pos);
    }
    if (self[0x00] <= 1) {
        get_lsp_data(0x22FF, &pos);
        draw_sprite_ary(lbl_805DAFE8, &pos);
    }
}

/* 0x80306524 - builds the effect-41 spawner: pools a 0x20-byte effect record, installs the
 * `fn_803066B4` dispatcher / `fn_80306614` releaser, fills its one model slot and arms it. */
void fn_80306524(void) {
    s32 area = get_now_areano();
    u8* res;
    _EFT* rec;
    Eft8030Work* w;
    Eft8030EffectSlot* slots;
    s32 i;

    res = fn_802B04A0(area);
    if (res == NULL) {
        return;
    }
    rec = (_EFT*)fn_800F8788(0x20);
    if (rec == NULL) {
        return;
    }
    rec->dispatch_0x34 = fn_803066B4;
    rec->release_0x40 = fn_80306614;
    w = (Eft8030Work*)rec->work_0x38;
    w->source_0x01C = res;
    w->count = 1;
    slots = w->slots_0x004;
    for (i = 0; i < w->count; i++) {
        slots[i].effect = (nw4r::ef::Effect*)fn_800F8914();
        if (slots[i].effect == NULL) {
            fn_800F886C(rec);
            return;
        }
    }
    memset(w->models_0x00C, 0, 0x10);
    rec->field_0x03 = 0x29;
    fn_800F9DF4(rec, 8, 0);
    rec->area_0x44 = area;
}

/* 0x80306614 - releases the effect-41 spawner's models and pooled handles. */
void fn_80306614(_EFT* self) {
    Eft8030Work* w = (Eft8030Work*)self->work_0x38;
    Eft8030EffectSlot* slots = w->slots_0x004;
    Eft8030ModelSlot* models = w->models_0x00C;
    s32 i;

    for (i = 0; i < w->count; i++) {
        s32 j;
        for (j = 3; j >= 0; j--) {
            if (models[i].parts_0x000[j] != NULL) {
                push_g3d_wk((_g3d_work*)models[i].parts_0x000[j]);
            }
        }
        if (slots[i].effect != NULL) {
            fn_800F8A44(&slots[i], 1);
        }
    }
}

/* 0x80304088 - draws the hunt-report row: the `0x22ED` frame, the `0x22EE`/`0x22EF` labels and the
 * count read out of the lobby block (`lbl_80794880` +0x4834) as a negative. */
void fn_80304088(void) {
    u8* lobby = (u8*)lbl_80794880;
    s8 count = -lobby[0x4834];
    _mh_ivec2_ pos;
    char buf[0x80];

    get_lsp_data(0x22ED, &pos);
    draw_sprite_ary(lbl_805DB2A0, &pos);
    draw_font_idx(0x22EE, (s8*)LbStr(0, 0x195), 1, &pos);
    if (count <= 1) {
        sprintf(buf, (char*)LbStr(0, 0x198));
    } else if (count & 1) {
        sprintf(buf, (char*)LbStr(0, 0x197), count / 2);
    } else {
        sprintf(buf, (char*)LbStr(0, 0x196), count / 2);
    }
    draw_font_idx(0x22EF, (s8*)buf, 1, &pos);
}

} /* extern "C" */
