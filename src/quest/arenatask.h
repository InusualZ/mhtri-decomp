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
 * The `extern`s below came from the band header `unsplit/arena.h` when this unit claimed its
 * own `.bss 0x806E3E10..0x806E40C0`, `.sbss 0x80794D3C..0x80794D50`, `.sdata 0x80793B28..0x80793B88`,
 * `.sdata2 0x8079C960..0x8079C998` and `.data 0x80604D30..0x806073F0` runs in
 * `config/RMHE08/splits.txt` (rule 12: the unit that uses the bytes claims and matches them; rule 2:
 * the declaration lives with the owner, and this is the owner's header).  The unit defines the `.bss` run
 * and two `.sbss` pointers at the foot of `src/quest/arenatask.cpp`, after every body (see its header).  The
 * band's record types stay in the band header, which this header includes.
 *
 * C++-only: the two vector arrays are `nw4r::math::VEC3` (the band's TUs are C++).
 */
#ifndef MHTRI_QUEST_ARENATASK_H
#define MHTRI_QUEST_ARENATASK_H

#include "types.h"
#include "nw4r/math.h"      /* nw4r::math::VEC3, the two `.bss` vector arrays */
#include "gx.h"             /* _GXColor, the ambient/direct-light colour record */
#include "unsplit/arena.h"  /* the band's record types (rule 1: one definition, included) */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804463C4 - the arena mode's task body: first run loads `04/arena.brres` and the arena state, then
 * it drives the match, and on the match's end it hands over to `GameModeExec`/`VsGameModeExec`. */
struct TaskSlot;
void arena_task(struct TaskSlot* task);

/* 0x80446EE8 - resets the arena work for the next round and builds its local or network setup;
 * 0x804473B8 - the per-frame match step: returns 1 when the round ends, -1 on a network drop, 2 to leave. */
void arena_result_next(ArenaWork* work);
s32 arena_game_task(ArenaWork* work);

/* 0x80445B80 - fills the arena quest-info rows; `mode` picks the clear-time column (1 = two-player);
 * 0x804459E4 - loads the arena's `.brres` texture pack and binds its textures. */
void arena_quest_info_build(u8 mode);
void arena_resource_load(void);

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
 * the dump's `que_info`) and the arena LSP data `arena_lsp_data_get` returns (`.sbss` 0x80794D40). */
extern ArenaQuestInfoList* que_info;
extern u8* arena_lsp_data_adrs;
/* 0x804459DC - the arena's loaded LSP data block (`hud/cockpit.cpp`'s `get_lsp_data` reads it in the arena). */
extern "C" u8* arena_lsp_data_get(void);

/* The arena texture pack's resource record (`.sdata` 0x80793B28: the archive size and its name) and the
 * table of texture names `arena_resource_load` looks up in it (`.data` 0x80607310, 32 slots, the last
 * one NULL). size: 0x8 */
typedef struct ArenaResourceInfo {
    /* +0x00 */ u32 size_0x00;
    /* +0x04 */ const char* name_0x04;
} ArenaResourceInfo; /* size: 0x8 */
extern ArenaResourceInfo arena_resource_info;
extern const char* arena_texture_names[32];

/* The 0x200-byte arena user-data record buffer the two `arena_eqdata_from_*` fillers build their
 * record in and `arena_userdata_apply` consumes (`.bss` 0x806E3E10, defined at the foot of `arenatask.cpp`).  Its two
 * 0x100-byte records are selected by the player work's own `chunk_ofs << 8`; the GUESS in the name is
 * that stride, which is what `arena_eqdata_from_vsuser` and `Pl/fn_80288CEC.cpp`'s `fn_8028F1E8`
 * both index it with.  `Pl/fn_80288CEC.cpp` is the one foreign reader (rule 2: it includes this
 * header for it). */
extern u8 arena_user_data_buf[];

/* Pooled float constants the band's setup functions address as globals (playbook 29: declare, never
 * define - a definition would make MWCC emit a second copy in `.sdata2`).  Each is named for the
 * place `arena_camera_light_vec_init` writes it into. */
extern const f32 arena_zero_f;            /* .sdata2 0x8079C960, 0.0f */
extern const f32 arena_50f;               /* .sdata2 0x8079C964, 50.0f */
extern const f32 arena_camera_near_z;    /* .sdata2 0x8079C968, 1.0f */
extern const f32 arena_camera_far_z;     /* .sdata2 0x8079C96C, 10000.0f */
extern const f32 arena_85f;               /* .sdata2 0x8079C974, 85.0f */
extern const f32 arena_440f;              /* .sdata2 0x8079C978, 440.0f */
extern const f32 arena_75f;               /* .sdata2 0x8079C97C, 75.0f */
extern const f32 arena_n50f;              /* .sdata2 0x8079C980, -50.0f */
extern const f32 arena_70f;               /* .sdata2 0x8079C984, 70.0f */
extern const f32 arena_20f;               /* .sdata2 0x8079C988, 20.0f */
extern const f32 arena_n30f;              /* .sdata2 0x8079C98C, -30.0f */
extern const f32 arena_n20f;              /* .sdata2 0x8079C990, -20.0f */
extern const f32 arena_100f;              /* .sdata2 0x8079C994, 100.0f */

/* The `.data`/`.sdata2` colour pair `arena_light_init` hands to the light unit: the four direct-light
 * colours and the single ambient colour (the two `lbl_` rows these replaced were renamed in the
 * band's naming pass).  They are
 * spelled as the 4-byte words the target itself copies (`lwz`/`stw`, not the four `lbz`/`stb` a
 * struct-of-bytes member copy emits), and the caller reinterprets each word as the `_GXColor` the two
 * setters take. */
extern u32 arena_light_colors[4];         /* .data 0x806073E0 */
extern const u32 arena_ambient_color;     /* .sdata2 0x8079C970 */

/* The ten-float spawn-offset table `arena_player_init` indexes with a three-float stride (`.data`
 * 0x806073B8): its three 12-byte rows are slot kind 0/1/2's `(x, y, z)`.  Declared extern like the
 * pool above - the range is this unit's own, so a definition would rebuild the run. */
extern f32 arena_player_offset_table[10];

/* The arena stage configuration table the band's three task bodies index with `stage_0x0C * 0x3B0`
 * (`.data` 0x80604D30, ten 0x3B0-byte records).  It is the first symbol of this unit's `.data`
 * claim on the low side, and the definition is a TRANSCRIPTION - see its own comment in
 * `src/quest/arenatask.cpp` for why the record internals are not reconstructed yet. */
extern u32 arena_stage_config[2360];

/* 0x80446B8C - installs the arena scene's ambient light and the four direct lights (its body is in
 * `src/quest/arenatask.cpp`). */
void arena_light_init(void);

/* 0x80445C84 - packs each of a player's six `_EQUIP` records into its `+0x04` word.  The parameter is
 * the move-work record `get_move_work_adrs(2)` hands back, so the shape is forward-declared here. */
struct _PLW;
void arena_equip_color_set(struct _PLW* plw);

/* 0x80445EB8 / 0x804461B4 - build the arena user-data record the two Vs modes read.  Both take a
 * player's move-work record and both are in `src/quest/arenatask.cpp`. */
void arena_eqdata_from_userdata(struct _PLW* plw);
void arena_eqdata_from_vsuser(struct _PLW* plw);

/* 0x80446990 - places every player's move work at its arena spawn offset, gives it the motion its
 * equip slot's kind selects, and marks the slot the player index agrees with (its body is in
 * `src/quest/arenatask.cpp`). */
void arena_player_init(ArenaWork* work);

#ifdef __cplusplus
}

/* ---- the mangled half (rule 9) ---- */

/* 0x80445DBC - converts one 0xEC-byte acdata equip record into the arena's own `_arena_eq_data`
 * view: its eight 0xC-byte head records, the `6` slot count and the two trailing blobs (0x60 +
 * 0x20).  The record's own tag is `_arena_eq_data` - the map row's spelling
 * (`dl_acdata_to_ar_eqdata__FP14_arena_eq_dataUc`) is the evidence for it, and the tag is what MWCC
 * mangles the parameter type from. */
void dl_acdata_to_ar_eqdata(_arena_eq_data* data, u8 index);
#endif

#endif /* MHTRI_QUEST_ARENATASK_H */
