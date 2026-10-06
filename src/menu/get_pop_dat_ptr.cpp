/*
 * menu/get_pop_dat_ptr.cpp - the pop-data / option system file: the per-map monster-population file loads
 *   (`m%03d_%06d_c_pop.dat`, `06/..._f_pop.dat`, `06/..._r_pop.dat`) into `get_move_work_adrs(0)`, the option table
 *   `option_w` and its arena-config twin, and the demo state words.  C++ (the range defines `get_pop_dat_ptr__Fv`,
 *   `ck_option_cfg__FUc`, `get_option_cfg__FUc`, `get_arena_cfg__FUcUc`, `set_option_cfg__FUcUc`, `set_def_option__Fv`,
 *   ...); the plain-spelled rows are defined inside `extern "C"`.
 * RANGE. .text 0x803BE30C-0x803C3A5C (87 functions); extab, extabindex, .ctors 0x8056F3B8, .data 0x805F84E0-0x805F8920,
 *   .bss 0x806D2AF8-0x806D2B20 (`pop_dat_ptrs`), .sdata 0x807936A8-0x80793738, .sbss 0x80794C60-0x80794C90, .sdata2
 *   0x8079C630-0x8079C680.  The edges are unproven: `tudiscover` gives only weak cuts (0x803BE318, 0x803BF294), and the
 *   `.sbss` words 0x80794C60/0x80794C64 are also read at 0x803BD3C0/0x803BA1B4, a hint that the TU starts earlier.
 * FLAGS. `cflags_menu` (configure.py); `infer.py` on the target: 0 record forms and a `clrlwi` kept before a narrowing
 *   store (the peephole off), and extab records (`-Cpp_exceptions on`).
 * NAMES. Module `menu`: no `__FILE__` string reaches the range, and its callee surface is the menu 2D library's
 *   (`PutPageArrow`, `get_menu_lsp_tbl`, `get_lsp_data`, `draw_sprite_*`, `GetMenuFontColor`, `LbStr`), shared with
 *   `menu/menu_result.cpp`, `menu/menu_item.cpp` and `menu/menu_item_page.cpp`.  The file keeps the first symbol, the
 *   runtime dump's own name.
 *   GUESS (from each body and its callers): note_goods_tbl
 * RESIDUALS. 72 rows unwritten (objdiff scores them zero): `fn_803BE318`, 0x803BE4D4-0x803BEA28,
 *   0x803BEA94-0x803BEBF0, `fn_803BECC8`, 0x803BED3C-0x803BEE04, 0x803BEEB8-0x803BEFC0, 0x803BEFD4-0x803C0C4C,
 *   0x803C0C58-0x803C0F24, 0x803C0F3C-0x803C3A30, `fn_803C3A40`.  `set_option_cfg`/`set_def_option`/`set_option_mode`
 *   apply their result through `set_SE_volume__FUc`/`set_BGM_volume__FUc` (`sound/snd_bank_loader.cpp`) and
 *   `set_now_brightness__Ff` (`main.cpp`), which no header declares.  The 4 partial rows:
 *  - `get_arena_cfg`: indexed-access fold, retail `add r3,r3,r0; lbz r0,0xd0(r3)` twice where ours emits `lbzx`
 *    (120 B against 128);
 *  - `get_option_pair`: retail reads the value table through `@sda21` (`lbl_807936B8`), ours through `lis`/`addi`
 *    (`option_value_tbl` is not declared small);
 *  - `demo_state_set`: our store lands at +0x4 where retail stores at +0x8;
 *  - `demo_save_words`: our second load is scheduled before the `li`, retail's after it.
 *  - `option_w`'s element type is unsigned here (`lbzx`, no `extsb`, in `ck_option_cfg`) while the target's
 *    `set_option_cfg` converts its value to a signed byte before the store: one of the two views is wrong.
 *   flipcheck: `.ctors` (0x4), `.data` (0x440), `.sdata` (0x90), `.sbss` (0x30) and `.sdata2` (0x50) claimed but not
 *   emitted; short `.text` 0x294 of 0x5750, extab 0x10 of 0x208, extabindex 0x18 of 0x30C; the bytes of all three
 *   differ; `demo_init_word_0`/`demo_init_word_1`, `demo_saved_words`, `option_value_max` and `option_value_tbl` are
 *   defined by no link input.
 */

#include "types.h"
#include "nw4r/math.h"       /* nw4r::math::VEC3, the 3-float record VEC3_ctor takes */
#include "mh3_pad/vec3.h"    /* VEC3_ctor (owner src/mh3_pad.cpp) */
#include "fn_8004CAD8.h"     /* get_vsUser_work / _vs_user_data (owner src/fn_8004CAD8.cpp) */
#include "hud/layout.h"     /* _mh_ivec2_, which the band header's prototypes name */
#include "Runtime.PPCEABI.H/memset.h" /* memset (owner: the Runtime.PPCEABI.H lib) */
#include "unsplit/menu.h"    /* option_w, this band's own tables, its callees */
#include "unsplit/unknown.h" /* SystemWork / system_w */
#include "lobby/lb_server_sel_trans.h" /* demo_work_process (owner's header, rule 2) */

/* The ten pop-data pointers (.bss 0x806D2AF8) the accessor below hands out; this unit's whole `.bss` claim. */
PopData* pop_dat_ptrs[10];

/* Hands out the ten pop-data pointers. */
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

/* This unit's own symbols that keep the map's plain (C-linkage) spelling. */
extern "C" {

/* The arena twin of `ck_option_cfg`, on the VS user work's profile.  Defined by the pass that writes
 * the arena half of the option code. */
u8 ck_arena_cfg(u8 index, u8 value);
/* Redraws the pop entry list `entry_reset` clears. */
void entry_list_update(PopEntry* entry);

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

} /* extern "C" */
