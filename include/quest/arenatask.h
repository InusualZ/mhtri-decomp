/*
 * quest/arenatask.h - the arena task band's published interface: the two entry points and the band's
 * own data, which `src/quest/arenatask.cpp` (`.text` 0x804459E4..0x80448404) owns.
 *
 * `arena_task` is the task `mh3_pad.cpp`'s `ArenaSelExec` (0x80041A34) hands to `Tsk_Change`;
 * `arena_other_player_eq_set` is what `lobby/lb_companion_ui.cpp`'s act 25 calls with the request's two
 * bytes.  Both were declared locally by those two consumers while the map's rows were still
 * `fn_804463C4`/`fn_80445D58`; the naming pass that registered the band renamed the rows and moved the
 * declarations here.
 *
 * The `extern`s below came from the band header `include/unsplit/arena.h` when this unit claimed its
 * own `.bss 0x806E4010..0x806E40C0`, `.sbss 0x80794D3C..0x80794D50`, `.sdata 0x80793B28..0x80793B88`,
 * `.sdata2 0x8079C960..0x8079C998` and `.data 0x80607210..0x806073F0` runs in
 * `config/RMHE08/splits.txt` (rule 12: the unit that uses the bytes claims and matches them; rule 2:
 * the declaration lives with the owner, and this is the owner's header).  The band's record types stay
 * in the band header, which this header includes.
 *
 * C++-only: the two vector arrays are `nw4r::math::VEC3` (the band's TUs are C++).
 */
#ifndef MHTRI_QUEST_ARENATASK_H
#define MHTRI_QUEST_ARENATASK_H

#include "types.h"
#include "nw4r/math.h"      /* nw4r::math::VEC3, the two `.bss` vector arrays */
#include "unsplit/arena.h"  /* the band's record types (rule 1: one definition, included) */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804463C4 - the arena mode's task body: first run loads `04/arena.brres` and the arena state, then
 * it drives the match, and on the match's end it hands over to `GameModeExec`/`VsGameModeExec`. */
void arena_task(void);

/* 0x80445D58 - records the equip index a *remote* player picked (`player` is the player number the
 * caller sends, `equip` the index); the local player's own choice never comes through here. */
void arena_other_player_eq_set(u8 player, u8 equip);

/* The arena task's own state block (`.bss` 0x806E4010, the dump's `arena_work`); `Pl/fn_80288CEC.cpp`
 * reads its `+0x04` mode byte through this symbol too, so the owner publishes it. */
extern ArenaWork arena_work;

/* The arena draw-callback slots the task registers sub-transparency for (`.bss` 0x806E4068, four
 * words, the dump's `arena_draw_func`); `menu/menu_result.cpp`'s dispatcher calls one of them. */
extern u32 arena_draw_func[4];

/* The arena camera pair and light quad the scene setup writes and the camera/light units read
 * (`.bss` 0x806E4078 / 0x806E4090).  Names are GUESSES from `arena_camera_light_vec_init`'s bodies. */
extern nw4r::math::VEC3 arena_camera_vec[2];   /* eye + look-at */
extern nw4r::math::VEC3 arena_light_vec[4];    /* four direct-light positions */

/* The arena quest-info row list `arena_quest_info_build` fills (`.sbss` 0x80794D3C, ten 0x18-byte rows,
 * the dump's `que_info`) and the arena LSP data the 8-byte accessor `fn_804459DC` returns
 * (`.sbss` 0x80794D40 - the accessor itself is the band's candidate first function, outside the
 * registered range). */
extern ArenaQuestInfoList* que_info;
extern u8* arena_lsp_data_adrs;

/* Pooled float constants the band's setup functions address as globals (playbook 29: declare, never
 * define - a definition would make MWCC emit a second copy in `.sdata2`).  Each is named for the
 * place `arena_camera_light_vec_init` writes it into. */
extern const f32 arena_zero_f;            /* .sdata2 0x8079C960, 0.0f */
extern const f32 arena_50f;               /* .sdata2 0x8079C964, 50.0f */
extern const f32 arena_85f;               /* .sdata2 0x8079C974, 85.0f */
extern const f32 arena_440f;              /* .sdata2 0x8079C978, 440.0f */
extern const f32 arena_75f;               /* .sdata2 0x8079C97C, 75.0f */
extern const f32 arena_n50f;              /* .sdata2 0x8079C980, -50.0f */
extern const f32 arena_70f;               /* .sdata2 0x8079C984, 70.0f */
extern const f32 arena_20f;               /* .sdata2 0x8079C988, 20.0f */
extern const f32 arena_n30f;              /* .sdata2 0x8079C98C, -30.0f */
extern const f32 arena_n20f;              /* .sdata2 0x8079C990, -20.0f */
extern const f32 arena_100f;              /* .sdata2 0x8079C994, 100.0f */

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_QUEST_ARENATASK_H */
