/*
 * The pop-data / option / demo system file, `.text` 0x803BE30C-0x803C4BA0 (110 functions, 26772 B).
 *
 * What it is.  The head of the range loads the per-map/area monster-population files
 * (`m%03d_%06d_c_pop.dat`, `06/..._f_pop.dat`, `06/..._r_pop.dat`) into the move-work buffer
 * `get_move_work_adrs(0)`; the middle is the option table `option_w` (.bss 0x80659090) and its
 * arena-config twin; the tail is the demo/event work `demo_work` (.bss 0x806D2B20) with its
 * accessors.  Its callers are spread across `Pl`, `menu`, `hud`, `lobby`, `ef`, `enemy` and
 * `stage`, and every one of the range's own `__F` manglings is C++ (the discovery probe's
 * `mangled-defined` set: `ck_option_cfg__FUc`, `get_option_cfg__FUc`, `get_arena_cfg__FUcUc`,
 * `set_option_cfg__FUcUc`, `set_def_option__Fv`, `set_option_mode__Fv`, `get_demo_no__Fv`,
 * `event_demo_ck__Fv`, `get_pop_dat_ptr__Fv`), so the unit is C++.  The range's other symbols keep
 * the map's plain (C-linkage) spelling - `fn_803BECA0`, `fn_803BEE04`, `fn_803C482C`, ... - and are
 * defined inside `extern "C"` so the front-end reproduces the name.
 *
 * Module and name (evidence classes 3 and 4).  No `__FILE__` string reaches the range - its only
 * `.data`/`.sdata` references are its own tables (the pop file-name strings at 0x805F84E0/0x805F84F8/
 * 0x805F8510, the option maximum table at 0x805F8528, the kana/keyboard tables at 0x805F87B0.. and
 * 0x807936A8..), and the shared runtime dump answers `zz_` for 101 of the range's 110 addresses (the
 * nine real names it does carry are the ones the map already has).  Module `menu` is class 3: the
 * range's callee surface is the menu 2D library's (`PutPageArrow`, `get_menu_lsp_tbl`, `get_lsp_data`,
 * `draw_sprite_*`, `GetMenuFontColor`, `LbStr`, `chk_pointer`, `PlayMode_ck`), it shares 48-66 of those
 * callees with `menu/menu_result.cpp` / `menu/menu_item.cpp` / `menu/menu_item_page.cpp`, and the
 * immediately preceding registered band is `menu/multi_result.cpp` (0x8039D278..0x803A3A50).  The file
 * name keeps the range's first symbol, which is the runtime dump's own real name (`get_pop_dat_ptr`).
 *
 * Seam.  UNPROVEN, and the brief's range is kept: this is the discovery `--max-bytes` cap, not a
 * boundary.  `tudiscover at 0x803BE30C` reports only weak cuts - the best right-hand cut is 0x803BF294
 * (share 0.107-0.116, "fn_803BF0E0/fn_803BF198 called only from this range"), the best left-hand one
 * is this range's own start (share 0.037), and `tudiscover at 0x803BE318` ranks a cut at 0x803BE318
 * above it - so nothing here is decisive.  The data does agree with the range: the `.bss` pair
 * 0x806D2AF8/0x806D2B20, the `.data` runs 0x805F84E0..0x805F8510 and 0x805F87B0..0x805F8948, the
 * `.sdata` run 0x807936A8..0x807938B0 and the whole `.sdata2` run 0x8079C630..0x8079C688 are
 * referenced only from inside it (the two exceptions are the single `.sbss` words 0x80794C60/0x80794C64,
 * which two earlier functions at 0x803BD3C0/0x803BA1B4 also read - the one hint that the TU may start
 * earlier than 0x803BE30C).  A seam re-draw is the follow-up if the bodies say so.
 *
 * Flags.  `cflags_menu` (its siblings' group): the target objects keep the unfused narrow-load pairs
 * (`infer.py` on `auto_fn_803BE318_text.o`: "0 record forms, 1 kept `clrlwi` before a narrowing
 * store" -> peephole off) and carry `extab`/`extabindex` (78 framed functions, 0x80019164..0x800193D4
 * and 0x8003987C..0x80039C24), which `-Cpp_exceptions on` supplies.
 *
 * Sections.  Only `.text` is claimed.  The unit's own extab 0x80019164..0x800193D4, extabindex
 * 0x8003987C..0x80039C24, `.data` 0x805F84E0..0x805F8510 + 0x805F87B0..0x805F8948, `.sdata`
 * 0x807936A8..0x807938B0, `.sdata2` 0x8079C630..0x8079C688, `.bss` 0x806D2AF8..0x806D2B48 and `.sbss`
 * 0x80794C60..0x80794C88 are NOT claimed yet - the pass that writes the bodies that emit them claims
 * them with them (docs/plan.md 8.4; the `menu/menu_result.cpp` precedent).  The pooled constants the
 * written bodies load are therefore spelled as literals, not as the map's pool symbols.
 *
 * Residuals (this pass).
 *   * 83 of the 110 functions are unwritten and measure 0 %.  The ones that need a heavily shared
 *     `lbl_` global (`lbl_806BF530` - 26 referrers outside the range - `lobby_world_block` - 90 - and the
 *     `fn_802B0668` / `fn_80217934` / `fn_800F886C` / `fn_800F8A44` callee set, each declared across
 *     30+ files) are deliberately left out: rule 7 makes naming them this batch's job, and that is a
 *     cross-unit rename batch, not this unit's registration.  They are the first follow-up.
 *   * `set_option_mode` / `set_option_cfg` / `set_def_option` / `set_option_from` (488 B) are not
 *     written: their bodies apply the result through `set_SE_volume`/`set_BGM_volume`
 *     (0x800F29BC/0x800F2A08, owned by `sound/fn_800EF7D8.cpp`, which has no owner header yet - the
 *     declarations sit in its own source at 397-398) and `set_now_brightness` (0x8003FC50, owned by
 *     `main/main.cpp`, declared nowhere).  Giving those two owners their headers is a cross-unit
 *     change with a re-measure of both units; it is the second follow-up.
 *   * `not_in_demo` (0x803C482C, `fn_803C482C`) is not written: `include/lobby/lb_companion_ui.h`
 *     declares the same symbol as `s32 fn_803C482C(void* psw)`, so naming it here needs that header's
 *     call-site signature settled first.
 *   * The pop-file-name strings (0x805F84E0/0x805F84F8/0x805F8510) and the option default table
 *     (0x805F8548) are declared in `include/unsplit/menu.h` and never defined, the same shape
 *     `menu/menu_note.cpp` records for its own `.data` literals.
 *   * `option_w`'s element type is unsigned here (`lbzx`, no `extsb`, in `ck_option_cfg`) while the
 *     target's `set_option_cfg` converts its value to a signed byte before the store; one of the two
 *     views is wrong and the first divergence in those rows says which.
 */

#include "types.h"
#include "nw4r/math.h"       /* nw4r::math::VEC3, the 3-float record VEC3_ctor takes */
#include "mh3_pad/vec3.h"    /* VEC3_ctor (owner src/mh3_pad.cpp) */
#include "fn_8004CAD8.h"     /* get_vsUser_work / _vs_user_data (owner src/fn_8004CAD8.cpp) */
#include "hud/layout.h"     /* _mh_ivec2_, which the band header's prototypes name */
#include "Runtime.PPCEABI.H/memset.h" /* memset (owner: the Runtime.PPCEABI.H lib) */
#include "unsplit/menu.h"    /* option_w, this band's own tables, its unowned callees */
#include "unsplit/unknown.h" /* SystemWork / system_w */

/* The pop-data pointer accessor (map name `get_pop_dat_ptr__Fv`). */
PopData** get_pop_dat_ptr(void)
{
    return pop_dat_ptrs;
}

/* Reads one option, inverting index 20 (the one the option screen stores negated). */
u8 get_option_cfg(u8 index)
{
    if (index == 20) {
        return option_w[index] ^ 1;
    }
    return option_w[index];
}

/* Reads one option clamped to its table maximum, inverting index 20. */
u8 ck_option_cfg(u8 index)
{
    u8 value = option_w[index];

    if (value > option_value_max[index]) {
        value = option_value_max[index];
    }
    if (index == 20) {
        value ^= 1;
    }
    return value;
}

/* Reads one option out of the arena profile of the VS user work. */
u8 get_arena_cfg(u8 index, u8 value)
{
    _vs_user_data* user;

    if (system_w.vs_mode_0x8b0 == 0) {
        return 0;
    }
    user = get_vsUser_work(index);
    if (user == 0) {
        return 0;
    }
    if (value == 20) {
        return user->cfg_0x00[value] ^ 1;
    }
    return user->cfg_0x00[value];
}

/* Non-zero while the demo work is playing the demo the game asks about. */
u32 event_demo_ck(void)
{
    return demo_work.state_0x00 == 4;
}

/* The demo id the demo work is playing, 0xFF when none. */
u8 get_demo_no(void)
{
    return demo_work.demo_no_0x02;
}

/* This unit's own symbols that keep the map's plain (C-linkage) spelling. */
extern "C" {

/* The arena twin of `ck_option_cfg`, on the VS user work's profile.  Defined by the pass that writes
 * the arena half of the option code. */
u8 ck_arena_cfg(u8 index, u8 value);
/* Redraws the pop entry list `entry_reset` clears. */
void entry_list_update(PopEntry* entry);
/* The demo work's per-frame pass. */
void demo_work_process(DemoWork* work);

/* Dispatches to the normal or the arena option check, by the system's VS/arena mode byte. */
u8 ck_cfg(u8 index, u8 value)
{
    if (system_w.vs_mode_0x8b0 == 0) {
        return ck_option_cfg(value);
    }
    return ck_arena_cfg(index, value);
}

/* Dispatches to the normal or the arena option read, by the system's VS/arena mode byte. */
u8 get_cfg(u8 index, u8 value)
{
    if (system_w.vs_mode_0x8b0 == 0) {
        return get_option_cfg(value);
    }
    return get_arena_cfg(index, value);
}

/* Reads option 21 out of the option table or the arena profile. */
u8 get_option_21(void)
{
    if (system_w.vs_mode_0x8b0 == 0) {
        return option_w[21];
    }
    return get_arena_cfg(0, 21);
}

/* Maps an option id to the (value, flag) byte pair `option_value_tbl` holds for it. */
void get_option_pair(u8 index, u8* value, u8* flag)
{
    u8 entry = 255;

    switch (index) {
    case 1:
        entry = 0;
        break;
    case 5:
        entry = 1;
        break;
    case 6:
        entry = 2;
        break;
    case 7:
        entry = 3;
        break;
    }

    if (entry != 255) {
        *value = option_value_tbl[entry * 2];
        *flag = option_value_tbl[entry * 2 + 1];
    } else {
        *value = 0;
        *flag = 0;
    }
}

/* A 4-byte `blr`: the option module's empty hook.  GUESS - nothing in the DOL calls it. */
void option_nop(void)
{
}

/* Initialises the 3-float record at `out` and hands the same pointer back. */
VEC3* vec3_construct(VEC3* out)
{
    VEC3_ctor(out);
    return out;
}

/* Clears the demo work's state and its +0x04 word. */
void demo_state_set(DemoWork* work)
{
    work->state_0x00 = 3;
    work->field_0x04 = 0;
}

/* The word the demo work holds at +0x04. */
u32 get_demo_data(void)
{
    return demo_work.field_0x04;
}

/* Non-zero while any of the demo work's three entry flags is set. */
u32 demo_flag_ck(void)
{
    if (demo_work.field_0x0C != 0) {
        return 1;
    }
    if (demo_work.entry_0x14.flag_0x00 != 0) {
        return 1;
    }
    if (demo_work.entry_0x14.flag_0x08 != 0) {
        return 1;
    }
    return 0;
}

/* Clears the demo work. */
void demo_work_clear(void)
{
    memset(&demo_work, 0, 36);
}

/* Marks the demo work as running. */
void demo_set_running(void)
{
    demo_work.field_0x08 = 1;
}

/* Initialises the demo work for a fresh play-through. */
void demo_work_init(void)
{
    demo_work.field_0x0D = 0;
    demo_work.field_0x0C = 0;
    demo_work.state_0x00 = 1;
    demo_work.field_0x04 = 0;
    demo_work.demo_no_0x02 = 255;
}

/* Saves the two demo words into the work block's saved pair. */
void demo_save_words(void)
{
    demo_saved_words[0] = demo_init_word_0;
    demo_saved_words[1] = demo_init_word_1;
}

/* Advances the entry's one-byte counter at +0x05. */
void counter_inc(PopEntry* entry)
{
    entry->count_0x005++;
}

/* Zeroes the entry's four state fields. */
void entry_state_reset(PopEntry* entry)
{
    entry->field_0x001 = 0;
    entry->field_0x0EC = 0;
    entry->field_0x14C = 0;
    entry->field_0x0E8 = 0;
}

/* Clears the entry's +0x02 byte and redraws the entry list. */
void entry_reset(PopEntry* entry)
{
    entry->field_0x002 = 0;
    entry_list_update(entry);
}

/* True for the five map ids 394..398. */
u32 map_id_ck(u16 map)
{
    return (u16)(map - 394) <= 4;
}

} /* extern "C" */
