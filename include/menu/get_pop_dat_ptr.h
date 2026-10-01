/*
 * The owner header for `src/menu/get_pop_dat_ptr.cpp` (`.text` 0x803BE30C-0x803C4BA0).
 *
 * RULE 2 HOME.  The four symbols the band headers used to carry - `get_option_cfg`
 * (`include/unsplit/lobby.h`), `get_arena_cfg` and `get_cfg` (`include/unsplit/menu.h`) and
 * `event_demo_ck` (`include/unsplit/Pl.h`) - are defined by this unit, so their declarations live
 * here and those three headers include this file instead of copying them.
 *
 * Linkage follows the map name, not the band header's old spelling: `get_option_cfg__FUc`,
 * `get_arena_cfg__FUcUc` and `event_demo_ck__Fv` are C++ free functions (so they are declared at C++
 * scope and the front-end reproduces the mangling, rule 9), while `get_cfg` keeps the map's plain
 * `fn_803BECA0`-style name and is `extern "C"`.
 */
#ifndef MHTRI_MENU_GET_POP_DAT_PTR_H
#define MHTRI_MENU_GET_POP_DAT_PTR_H

#include "types.h"

/* 0x803BEC70 - one option out of `option_w`, with index 20 inverted. */
u8 get_option_cfg(u8 index);
/* 0x803BEBF0 - one option out of the arena profile of the VS user work. */
u8 get_arena_cfg(u8 index, u8 value);
/* `event_demo_ck` (0x803C4814) is `lobby/lb_server_sel_trans.cpp`'s since the phase 4 recut; its header is included for the
 * consumers of this one. */
#include "lobby/lb_server_sel_trans.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x803BECA0 - `get_option_cfg`/`get_arena_cfg` picked by `system_w`'s VS/arena mode byte. */
u8 get_cfg(u8 index, u8 value);

/* 0x803BE8B8 - applies the player's arena profile: the SE/BGM volumes and the screen brightness the
 * Vs user block of `player` stores (nothing when the Vs mode byte is clear or the block is absent).
 * 0x803BEA94 - stores one clamped arena profile byte (`index`, `value`) into it and pushes a changed
 * volume through.  Added with `quest/arenatask.cpp` (rule 2: this range owns both addresses). */
void arena_cfg_apply(u8 player);
void arena_cfg_set(u8 player, u8 index, u8 value);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_GET_POP_DAT_PTR_H */
