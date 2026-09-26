/* The types and declarations `src/hud/cockpit_quest.cpp` owns (docs/plan.md 6.5, rules 1-5).
 *
 * The unit is the quest half of the cockpit HUD: it drives the quest bar / target nameplate from the
 * player work record (`_PLW`) and the two cockpit work records `lbl_806BDCC8[2]` (0x194 B each,
 * defined by the band below - `menu/fn_802E4978.cpp`).
 *
 * Playbook 56 (one record, two views).  `lbl_806BDCC8`'s 0x194-byte record is read by *both* bands:
 * `include/menu/fn_802E4978.h` (the band 0x802E4978-0x802E7408) names the fields it reads and the
 * band above (this unit) splices six more into its filler - `blend_0x13E`, `blend_0x140`,
 * `from_0x144`/`to_0x154` (the two 0x10-byte blend endpoints `fn_802E7690` interpolates) and the
 * four `field_0x134`..`field_0x13C` members the sibling already names.  Every member the sibling
 * names keeps its own offset, name and type here; the byte total is unchanged (0x194), so neither
 * view moves.  The two headers are the duplicate rule 1 records as debt (outbox: `shared-file`
 * request naming both files) - the fold is the merge, not this unit's.
 *
 * The outbound callees whose owners are registered are declared in the owner's header and included
 * from the source (`pl.h`, `Pl/pl_skill.h`, `Pl/pl_act.h`, `Pl/pl_master.h`, `hud/layout.h`,
 * `main.h`, `fn_80429B94.h`).  The rest are unsplit (their band has no registered unit), so their
 * declarations live here with the signature the target's call site shows (rule 2's unsplit case,
 * the same debt `include/hud/layout.h` records).
 */
#ifndef MHTRI_HUD_COCKPIT_QUEST_H
#define MHTRI_HUD_COCKPIT_QUEST_H

#include "types.h"
#include "pl.h"
#include "hud/layout.h"

/* The screen projection `fn_802B0230` hands back: two scales and the two offsets
 * `fn_802E8F54` divides by.  size: 0x10 */
typedef struct QuestScreen {
    /* +0x00 */ f32 scale_x;
    /* +0x04 */ f32 scale_y;
    /* +0x08 */ f32 ofs_x;
    /* +0x0C */ f32 ofs_y;
} QuestScreen;

/* One record of the quest-target table `get_move_work_adrs(3)` walks (stride 0xB18, the count is
 * `get_move_work_max(3)`); only the two fields `fn_802E8BA4` reads are named.  size: 0xB18 */
typedef struct QuestTarget {
    /* +0x000 */ u8 pad_0x000[0x1C8];
    /* +0x1C8 */ u32 flags;      /* bit 0: the entry is live */
    /* +0x1CC */ u8 pad_0x1CC[0x1E1 - 0x1CC];
    /* +0x1E1 */ u8 index;       /* the bit `CockpitWork::field_0x0BC` is raised for */
    /* +0x1E2 */ u8 pad_0x1E2[0xB18 - 0x1E2];
} QuestTarget;

/* One 0x10-byte blend endpoint the quest bar interpolates between: three integer components the
 * `.data` tables at 0x805D5C88.. hold, plus a fourth word copied verbatim (the target stores it
 * with a plain `lwz`/`stw`, never through the interpolator).  size: 0x10 */
typedef struct HudBlend {
    /* +0x00 */ s32 x;
    /* +0x04 */ s32 y;
    /* +0x08 */ s32 z;
    /* +0x0C */ u32 tag;   /* copied, not blended (the `lwz`/`stw` pair in `fn_802E7690`) */
} HudBlend;

/* One player's cockpit work record (`lbl_806BDCC8`, two entries, stride 0x194).  This is *this*
 * band's view of the shared record (see the header comment): the members the sibling
 * `menu/fn_802E4978.cpp` names keep their offsets, and the six this band reads sit in the byte run
 * the sibling leaves unnamed.  size: 0x194 */
typedef struct CockpitWork {
    /* +0x000 */ _PLW* plw;          /* the player work record (sibling: `CockpitMove* move`) */
    /* +0x004 */ u8 unused_0x004[0x008 - 0x004];
    /* +0x008 */ u32 field_0x008;
    /* +0x00C */ u8 unused_0x00C[0x019 - 0x00C];
    /* +0x019 */ u8 field_0x019;
    /* +0x01A */ u8 unused_0x01A;
    /* +0x01B */ u8 field_0x01B;
    /* +0x01C */ u8 unused_0x01C;
    /* +0x01D */ u8 field_0x01D;
    /* +0x01E */ u8 unused_0x01E;
    /* +0x01F */ u8 field_0x01F;
    /* +0x020 */ u8 unused_0x020;
    /* +0x021 */ u8 field_0x021;
    /* +0x022 */ u8 unused_0x022;
    /* +0x023 */ u8 field_0x023;
    /* +0x024 */ u8 unused_0x024;
    /* +0x025 */ u8 field_0x025;
    /* +0x026 */ u8 unused_0x026;
    /* +0x027 */ u8 field_0x027;
    /* +0x028 */ u8 unused_0x028;
    /* +0x029 */ u8 field_0x029;
    /* +0x02A */ u8 unused_0x02A;
    /* +0x02B */ u8 field_0x02B;
    /* +0x02C */ u8 unused_0x02C[0x084 - 0x02C];
    /* +0x084 */ s16 field_0x084;     /* the quest window's spot, off `get_lsp_data(0xBE3)` */
    /* +0x086 */ u16 field_0x086;
    /* +0x088 */ f32 field_0x088;
    /* +0x08C */ u8 unused_0x08C[0x0B4 - 0x08C];
    /* +0x0B4 */ f32 field_0x0B4;     /* the scale `fn_802E9130` squares */
    /* +0x0B8 */ u8 unused_0x0B8[0x0BC - 0x0B8];
    /* +0x0BC */ u16 field_0x0BC;     /* the quest-target bitmask `fn_802E8BA4` fills */
    /* +0x0BE */ s16 field_0x0BE;
    /* +0x0C0 */ s16 field_0x0C0;
    /* +0x0C2 */ s16 field_0x0C2;
    /* +0x0C4 */ s16 field_0x0C4;
    /* +0x0C6 */ s16 field_0x0C6;
    /* +0x0C8 */ u8 unused_0x0C8[0x0CD - 0x0C8];
    /* +0x0CD */ u8 field_0x0CD;
    /* +0x0CE */ u8 field_0x0CE;
    /* +0x0CF */ u8 field_0x0CF;
    /* +0x0D0 */ u8 field_0x0D0;
    /* +0x0D1 */ u8 field_0x0D1;
    /* +0x0D2 */ u8 unused_0x0D2[0x134 - 0x0D2];
    /* +0x134 */ u16 field_0x134;     /* the counted-up gauge value (`fn_802E796C` adds 4) */
    /* +0x136 */ u8 field_0x136;      /* the gauge's step latch (0xFF while the act is winding) */
    /* +0x137 */ u8 field_0x137;
    /* +0x138 */ u8 field_0x138;
    /* +0x139 */ u8 field_0x139;
    /* +0x13A */ u8 field_0x13A;      /* the blend clock, counted down from 9 */
    /* +0x13B */ u8 unused_0x13B;
    /* +0x13C */ s16 field_0x13C;     /* the health value the blend clock is restarted on */
    /* +0x13E */ u16 blend_0x13E;     /* the first bar's blend parameter (`fn_802E7408`) */
    /* +0x140 */ u16 blend_0x140;     /* the second bar's blend parameter (`fn_802E7548`) */
    /* +0x142 */ u8 unused_0x142[0x144 - 0x142];
    /* +0x144 */ HudBlend bar_a_0x144; /* the first bar's blended value (`fn_802E7690`'s out) */
    /* +0x154 */ HudBlend bar_b_0x154; /* the second bar's blended value */
    /* +0x164 */ u8 unused_0x164[0x194 - 0x164];
} CockpitWork;

/* The two cockpit work records (`.bss` 0x806BDCC8, 0x328 B = 2 x 0x194).  Defined by the band below,
 * which initialises them (`fn_802E4978` memsets one); declared here, never defined (playbook 29's
 * rule for another unit's storage). */
extern CockpitWork lbl_806BDCC8[2];

#ifdef __cplusplus
extern "C" {
#endif

/* ---- this unit's own bodies, in address order (a bare `fn_` map name is C linkage) ---- */
u16 fn_802E7408(u16 id, u8 kind, _PLW* plw);
u16 fn_802E7548(s32 id, u8 kind, _PLW* plw);
void fn_802E7690(HudBlend* out, const HudBlend* a, const HudBlend* b, u16 t);
void fn_802E7788(CockpitWork* work);
void fn_802E796C(CockpitWork* work, u8 arg1);
void fn_802E7CFC(u16 id, s32 w, s32 h, s32 a3, s32 a4, const void* a5, const _mh_ivec2_* pos, s16 size);
void fn_802E7FA0(CockpitWork* work);
s32 fn_802E884C(CockpitWork* work, QuestTarget* target);
void fn_802E8BA4(CockpitWork* work);
void fn_802E8F54(f32* out, const f32* pos);
f32 fn_802E90C0(CockpitWork* work, const f32* pos);
s32 fn_802E90FC(CockpitWork* work, const f32* pos);
u8 fn_802E9130(CockpitWork* work, u8 value, f32 dist);
void fn_802E9E9C(CockpitWork* work, s32 arg1);

/* ---- unsplit callees whose map name is bare (C linkage) ---- */
u8 fn_802715A0(_PLW* plw, u32 slot);            /* 0x802715A0 Pl/pl_skill.cpp defines it */
s32 fn_8027681C(_PLW* plw);                     /* 0x8027681C */
u8 fn_802B0598(u8 kind);                        /* 0x802B0598 */
f32 fn_800501B4(u16 t);                         /* 0x800501B4 */
u8 fn_8028EF30(u8 index);                       /* 0x8028EF30 */
void fn_802E9070(f32* a, f32* b, u16 t);        /* 0x802E9070 this unit's own body */
f32 fn_802E8FBC(CockpitWork* work, const f32* pos); /* 0x802E8FBC this unit's own body */
const QuestScreen* fn_802B0230(void);           /* 0x802B0230 the screen projection */
s32 fn_802E270C(s32 a, s32 b, f32 t);           /* 0x802E270C */
u8 fn_802EA7DC(CockpitWork* work, QuestTarget* target, s32 flags); /* 0x802EA7DC */
s32 fn_802E8D7C(CockpitWork* work);             /* 0x802E8D7C */
s8 fn_802EA5F4(_PLW* plw);                      /* 0x802EA5F4 */
u16 get_move_work_max(u8 index);                /* 0x800CFA8C */
u8* get_move_work_adrs(u8 index);               /* 0x800CFA90 */
f32 fn_800501E4(u16 t);                         /* 0x800501E4 */
void fn_80500EF4(f32* a, f32* b, f32 t);        /* 0x80500EF4 */
u16 fn_802E73B8(u16 id, _PLW* plw);             /* 0x802E73B8 the band below's last function */

#ifdef __cplusplus
}
#endif

/* ---- unsplit callees whose map name is a mangling: the front-end reproduces it (rule 9) ---- */
void drawshape_set_texture_array(u16 id, const _mh_tex_uv_* uv); /* 0x80053ACC */
void drawshape_set_vertex_array(const _mh_ivec2_* verts);        /* 0x80053860 */
u8 get_option_cfg(u8 index);                    /* 0x803BEC70 -> get_option_cfg__FUc */
u8 get_arena_cfg(u8 a, u8 b);                   /* 0x803BEBF0 -> get_arena_cfg__FUcUc */
void set_blendmode(u8 a, u8 b, u8 c);           /* 0x800E4574 -> set_blendmode__FUcUcUc */
void get_worldworld_pos(nw4r::math::VEC3* out, const nw4r::math::VEC3* pos, u8 area); /* 0x802B0100 */
u8 get_now_areano(void);                        /* 0x802AFC84 -> get_now_areano__Fv */
u8 get_now_mapno(void);                         /* 0x802AFC74 -> get_now_mapno__Fv */

/* The pooled constants and tables this range reads.  Declared, never defined (playbook 29): the
 * unit's object must not rebuild the `.sdata2`/`.data` sections the split already owns. */
extern f32 lbl_8079A8E8;
extern f32 lbl_8079A8F0;
extern f32 lbl_8079A8F8;
extern f32 lbl_8079A8FC;
extern f32 lbl_8079A910;
extern f32 lbl_8079A920;
extern f32 lbl_8079A928;
extern f32 lbl_8079A92C;
extern u16 lbl_805D5C74[2][5];
extern HudBlend lbl_805D5C88;
extern HudBlend lbl_805D5C98;
extern HudBlend lbl_805D5CA4;
extern HudBlend lbl_805D5CB0;
extern HudBlend lbl_805D5CBC;
extern HudBlend lbl_805D5CC8;
extern HudBlend lbl_805D5CD8;
extern HudBlend lbl_805D5CF0;
extern HudBlend lbl_805D5CFC;
extern char lbl_805D5D08[];   /* "cockpit_quest.cpp" (0x12 B) */
extern char lbl_805D5D20[];   /* the `nw4r::db::Panic` message */

#endif /* MHTRI_HUD_COCKPIT_QUEST_H */
