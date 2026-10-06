#ifndef MHTRI_EF_FN_800CDB2C_H
#define MHTRI_EF_FN_800CDB2C_H

#include "types.h"

/* ef/fn_800CDB2C.h - the declarations of `ef/system_core.cpp`'s symbols its consumers call (docs/plan.md 6.5
 * rule 2), plus `push_g3d_wk` (`nw_resource.cpp`'s); where a consumer's codegen needs its own view of a signature,
 * the declaration carries that view.
 */
#ifdef __cplusplus
extern "C" {
#endif

/* 0x800CF384 - `system_w` +0x27, the local player's index (a zero-extended byte load); `Pl/pl_master.cpp`,
 * `sound/fn_800D7F54.cpp`, `hud/move_work_update.cpp` and `main.cpp` read it. */
u32 my_player_no(void);
/* 0x800CF394 - the setter paired with it, declared `s8`: every consumer (`light/light.cpp`'s fn_802BEDE8/fn_802BEE3C)
 * narrows its argument with `extsb`, which only a signed parameter emits; the map name is unmangled (C linkage). */
void my_player_no_set(s8 value);
u8  GameMode_ck(void);
u32 move_work_state_ck(void);
/* 0x800CF2C4 - the play-mode/move-state dispatcher: it reads `GameMode_ck()`, switches on `PlayMode_ck()` and
 * hands the caller's mode byte on (`Pl/pl_act_step.cpp`'s act-175 arm calls it). */
void ef_move_state_dispatch(u8 mode);

s32 fn_800CED10(char* path, u32 dma, u32 size);
s32 fn_800CEE2C(const char* path, void* info);
/* The task-table entry `ef/fn_80059550.cpp` reads (its own return view is `_GXTexObj*`). */
u8* fn_800D0568(s32 index);

/* 0x800D0708 - `system_w`'s +0x7D3 byte, which the consumers compare against 1; the name comes from the call
 * surface.  Unmangled, so it sits at C scope: its consumers declare it `extern "C"` too, and a C++-scope
 * declaration clashes with theirs (`(10505) illegal overloading`). */
u32 game_ready_ck(void);

/* 0x800CF3C4 - the narrowed player-count read; `menu/menu_message.cpp`'s `menu_list_mode_get`/`menu_list_fill`
 * read it as a signed byte (`extsb`, then `cmpwi`). */
s8 player_count_get(void);

/* 0x800CF3D4 - stores the session's player count (`system_w`'s +0x28, which `player_count_get` reads back);
 * 0x800CF0F0 - the return-to-title reset: stops the sound, clears the system work and every task and re-enters
 * the title (`quest/arenatask.cpp` calls both). */
void player_count_set(u8 count);
void game_reset_to_title(void);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
/* 0x800CEEBC - the random ring, a C++ free function (`ran_suu__Fl`, rule 9): r3 is the ring index, and it
 * returns the narrowed u16 it has just stored (`clrlwi r3,r0,16`), as the owner defines it. */
#ifndef EF_FN_800CDB2C_NO_RAN_SUU
/* A TU that holds a section with its own width of `ran_suu`'s result (`lobby/lb_npc.cpp`) defines the macro and declares it itself. */
u16 ran_suu(s32 index);
#endif

/* 0x800CEC64 - reads the file `path` into `dest` (`size` bytes) through the loader; non-zero when the read
 * succeeded (`load_file__FPcUll`, a C++ free function). */
s32 load_file(char* path, u32 dest, s32 size);

/* 0x800CF218 - the play-mode byte `PlayMode_ck__Fv`; the consumers use `u8` (values < 7). */
u8 PlayMode_ck(void);

/* 0x800D3058 - the g3d work-handle release `push_g3d_wk__FP9_g3d_work` (`nw_resource.cpp`'s), a C++ free function
 * (rule 9); `ef/eft035.cpp`'s release paths hand it the pooled `_g3d_work*` handles. */
struct _g3d_work;
void push_g3d_wk(struct _g3d_work* work);
#endif

#ifdef __cplusplus
/* The mode and loader entry points `quest/arenatask.cpp`'s task drives (their map names are the manglings of these
 * signatures, rule 9): 0x800CEA34 - polls the load of `name` (`out` receives its progress; 1 while pending),
 * 0x800CEF9C - the full game reset, 0x800CF238/0x800CF254 - the game/play mode setters, 0x800CFB2C - builds the
 * move work of `kind`, 0x800CFB0C - sets the move-work record count of slot `index`, 0x800D0F14 - the loading
 * display. */
u32 file_loading_ck(char* name, s32* out);
void all_reset(void);
void GameMode_set(u8 mode);
void PlayMode_set(u8 mode);
s32 create_move_work(s32 kind);
void set_move_work_max(u8 index, s32 value);
void loading_disp_set(u8 kind, u8 arg);
#endif

/* 0x800D2914 - switches the display power-management off (`pmic_disp_off__Fv`, C++ linkage). */
void pmic_disp_off(void);
#endif /* MHTRI_EF_FN_800CDB2C_H */
