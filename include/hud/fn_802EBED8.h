/* The declarations `src/hud/fn_802EBED8.cpp` owns (docs/plan.md 6.5, rules 1-5).
 *
 * This unit is the continuation of the cockpit HUD band above `hud/cockpit_quest.cpp`: the same
 * quest-window / item-slot / act-flag work the sibling bands drive, but no `__FILE__` string of its
 * own survives (checked below), so the file keeps the map's `fn_802EBED8` stem while the symbol
 * itself is named `quest_marker_draw` (brief section 2, class 4), and the module is `hud` (the
 * naming scheme of the band's neighbours `layout.cpp`, `cockpit_quest.cpp` and the registered
 * `hud/fn_80324F7C.c`).
 *
 * Rule-2 debt, recorded rather than guessed.  The callees below whose owner is a registered unit
 * (`ef/eft053.cpp`, `hud/cockpit_quest.cpp`, `hud/layout.cpp`, `menu/menu_item.cpp`) are re-declared
 * here because the owner headers do not yet carry them; the fold is a `shared-file` request in this
 * unit's outbox, not this unit's edit.  The names with a bare `fn_`/`lbl_` stem are the map's own
 * placeholders (rule 7 deferral in the source) and have no registered owner, so they live here until
 * their band is cut.
 */
#ifndef MHTRI_HUD_FN_802EBED8_H
#define MHTRI_HUD_FN_802EBED8_H

#include "types.h"
#include "pl.h"
#include "hud/layout.h"

/* One 5-byte selection record: two bytes, a halfword and a flag byte.  Offsets and width are the
 * loads/stores `fn_802EF400` copies field by field (`lbz`/`lbz`/`lhz`/`lbz`).  size: 0x5 */
typedef struct QuestSel {
    /* +0x0 */ u8 field_0x0;
    /* +0x1 */ u8 field_0x1;
    /* +0x2 */ u16 field_0x2;
    /* +0x4 */ u8 field_0x4;
} QuestSel;

/* The two-byte cursor `fn_802ED6F4` advances: `index` into the caller's bit table (`fn_802ED588`'s
 * mask) and `timer` (0x14 = armed, 0xFF = idle, else a countdown).  size: 0x2 */
typedef struct Cursor2 {
    /* +0x0 */ u8 index;
    /* +0x1 */ s8 timer;
} Cursor2;

/* One 12-byte position record `fn_802EF0A0` copies out of its scratch and hands to `fn_802EEDE4`
 * (three words, `lwz`/`stw`).  size: 0xC */
typedef struct Pos12 {
    /* +0x0 */ u32 x;
    /* +0x4 */ u32 y;
    /* +0x8 */ u32 z;
} Pos12;

/* The `.bss` blink record right after the two 0x194-byte quest work records (0x806BDFF0 = the end of
 * `cockpit_work`'s run).  Only the two bytes `fn_802EF6B0` uses are read here; the tail size is an
 * approximation (the record is at least 0x76 bytes).  size: 0x100 (approximate) */
typedef struct QuestBlink {
    /* +0x00 */ u8 pad_0x00[0x74];
    /* +0x74 */ u8 timer;   /* the 0/10 countdown `fn_802EF6B0` runs */
    /* +0x75 */ u8 flag;    /* the value `fn_8028E4F0` reports while the timer is armed */
    /* +0x76 */ u8 pad_0x76[0x100 - 0x76];
} QuestBlink;

#ifdef __cplusplus
extern "C" {
#endif

/* ---- this unit's own bodies, in address order (a bare `fn_` map name is C linkage) ---- */
u32 fn_802EC6C4(void);
void fn_802EDAF0(_PLW* plw, u8* a, u8* b);
s32 fn_802ED588(_PLW* plw, u8 kind);
void fn_802ED6F4(u32 mask, s32 count, Cursor2* cur);
s32 fn_802ED620(_PLW* plw, u8 kind);
void fn_802ED7E4(_PLW* plw, Cursor2* cur, u8 kind);
void fn_802ED834(_PLW* plw, Cursor2* cur, u8 kind);
s16 fn_802EECA0(_PLW* plw);
s32 fn_802EEC24(_PLW* plw, u16 idx);
void fn_802EE330(_PLW* plw, u16 id, u8 anim, u8 mask, const _mh_ivec2_* pos);
void fn_802EEDE4(void* out);
void fn_802EF0A0(_PLW* plw, void* out);
s32 fn_802EF62C(_PLW* plw);
void fn_802EF6B0(void);
void fn_802EF400(QuestSel* dst, const QuestSel* src);

/* ---- a callee a registered unit owns, re-declared because the owner's header is not included here
 * (same rule-2 debt as the block below, but the name resolves: owner `ef/eft053.cpp`) ---- */
void eft053_shell_pos_project(void* a, void* b); /* 0x80366618 */

/* ---- unsplit callees whose map name is bare (C linkage), rule-2 debt ---- */
u8 fn_803311A0(void);                       /* 0x803311A0 */
void drawshape_copy_vec2(void* a, const f32* b);    /* 0x80053CF8 */
u32 fn_802BE39C(void);                      /* 0x802BE39C */
u8 fn_8028E4F0(void);                       /* 0x8028E4F0 */
u8 get_now_areano(void);                    /* 0x802AFC84 -> get_now_areano__Fv */

/* The quest work block `.bss` 0x806BDCC8 (two `CockpitWork` records, 0x328 B) and the blink record
 * right after it.  Declared and never defined (playbook 29); this unit's minimal view of the first
 * word is the `_PLW*` `fn_802EF62C` takes. */
extern _PLW* cockpit_work[];
extern QuestBlink cockpit_state;

/* The `.data` mask tables `fn_802ED588` walks - 4-byte words terminated by 0.  Declared as sized
 * arrays so MWCC addresses them `lis`/`addi` (a scalar would come out `@sda21`); declared and never
 * defined (playbook 29). */
extern const u32 lbl_805D6338[];
extern const u32 lbl_805D6360[];
extern const u32 lbl_805D638C[];
extern const u32 lbl_805D63B0[];

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_FN_802EBED8_H */
