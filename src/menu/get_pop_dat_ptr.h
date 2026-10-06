/*
 * menu/get_pop_dat_ptr.h - the declarations `menu/get_pop_dat_ptr.cpp` owns (rule 2; `unsplit/lobby.h`, `unsplit/menu.h`
 *   and `unsplit/Pl.h` include it).  `get_option_cfg__FUc`, `get_arena_cfg__FUcUc` and `event_demo_ck__Fv` are C++ free
 *   functions at C++ scope (rule 9); `get_cfg` keeps the map's plain name inside `extern "C"`.
 */
#ifndef MHTRI_MENU_GET_POP_DAT_PTR_H
#define MHTRI_MENU_GET_POP_DAT_PTR_H

#include "types.h"

/* 0x803BEC70 - one option out of `option_w`, with index 20 inverted. */
u8 get_option_cfg(u8 index);
/* 0x803BEA28 - one option out of `option_w`, clamped to its table maximum, with index 20 inverted
 * (`ck_option_cfg__FUc`). */
u8 ck_option_cfg(u8 index);
/* 0x803BEBF0 - one option out of the arena profile of the VS user work. */
u8 get_arena_cfg(u8 index, u8 value);
/* `event_demo_ck` (0x803C4814) is `lobby/lb_server_sel_trans.cpp`'s; its header is included for the consumers of this
 * one. */
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

/* The note trade's goods table (`.sdata` 0x807936B0): its first word points at the 0-terminated `NoteGoods`
 * records; the reader takes it as an object of unknown extent (a `lis`/`lwz` load, not a small-data one) (GUESS name). */
struct NoteGoods;
extern struct NoteGoods* note_goods_tbl[];

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_GET_POP_DAT_PTR_H */
