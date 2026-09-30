/* The player per-frame control cluster `Pl/fn_8025F088.cpp`
 * (`.text` 0x8025F088-0x80262940, 17 functions, extab 0x8001239C-0x8001241C,
 * extabindex 0x8002F454-0x8002F514).
 *
 * Its own entry points, then the callees the unit calls whose owner's header does not declare them.
 * Declaring them here rather than in each owner's header follows the `enemy/fn_80165FC8.h` precedent
 * ("their owners' headers do not declare these, or declare a different signature; the shapes here are
 * the call sites'"); moving each to its owner's header is the rule-2 debt the unit header records.
 */
#ifndef MHTRI_PL_FN_8025F088_H
#define MHTRI_PL_FN_8025F088_H

#include "types.h"
#include "nw4r/math.h"
/* The owner's header for `pl_model_state_set` (0x80267270, `Pl/fn_80262940.cpp`'s range).  This header used
 * to declare it `u32` while the owner defines it `void` - the `(10505) illegal overloading` this
 * include clears (docs/plan.md 6.5 rule 2). */
#include "Pl/fn_80262940.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* ---- this unit's own entry points (unmangled `fn_*` stems, so C linkage - playbook 42/48) ---- */
void fn_8025F088(struct _PLW* self);
void fn_8025F478(struct _PLW* self);
s32 fn_8025F588(struct _PLW* self);
void fn_8025FA00(struct _PLW* self);
void fn_8025FF0C(struct _PLW* self, u8 mode);
s32 fn_80260198(struct _PLW* self);
void fn_80260248(struct _PLW* self, s32 mode, s16 delta);
void fn_802602A0(struct _PLW* self);
s32 fn_8026077C(u8 a, u8 b);
void fn_802607C4(struct _PLW* self);
void fn_8026099C(struct _PLW* self);
s16 fn_80260A18(struct _PLW* self);
s32 fn_80260A58(struct _PLW* self);
void fn_80260B38(struct _PLW* self);
void fn_80261770(struct _PLW* self);
void fn_802621B0(struct _PLW* self);
s32 fn_80262688(struct _PLW* self);

/* ---- Pl-band callees with no registered owner (`include/unsplit/Pl.h`'s home) ---- */
/* `fn_802756F0` is declared by that band header itself (`include/unsplit/Pl.h`, `void`, `u8`/`u16`/`u32`,
 * the callee's own prologue's widths) - the `u32`/`s32`/`u16` copy that stood here clashed with it
 * ((10197) illegal function overloading) as soon as MAIN's landing put the declaration there (rule 2). */
u32 fn_8024676C(void);
u32 fn_8025DE38(struct _PLW* self);
u32 fn_8025E0C8(struct _PLW* self);
u32 fn_8025E298(struct _PLW* self, s32 a, s32 b);
u32 fn_8025E448(struct _PLW* self);
u32 fn_8025EC58(struct _PLW* self);
u32 fn_8025ED00(struct _PLW* self);
u32 fn_80276800(struct _PLW* self, s32 v);
/* `fn_80278BE4` is declared by its owner's header, `include/Pl/pl_act.h` (`void`; `Pl/pl_act.cpp`
 * defines it `extern "C" void`), which this unit includes - the `u32` copy that stood here clashed
 * with it ((10505) illegal overloading) once MAIN's landing registered the declaration (rule 2). */
u32 fn_80278C7C(struct _PLW* self);
u32 fn_80278CD0(struct _PLW* self);
u32 fn_802872E4(struct _PLW* self);
s32 fn_802919FC(struct _PLW* self, void* a, void* b, f32* out, s32 slot);
u32 fn_802950D8(struct _PLW* self, u8 mode, u16 flags);
u32 fn_8027DCA8(struct _PLW* self);
u32 fn_8029EFDC(void* p);

/* ---- callees another registered unit owns whose header does not declare them ---- */
u32 fn_8012A624(void* out);
u32 fn_80131934(u8 index);
u32 fn_80224AC4(void* physics);
/* `Pl_item_timer_get` and `fn_80273044` are declared by their owner, `include/Pl/pl_skill.h` (`extern "C"
 * int`/`u16`, `struct _PLW*`, `u16`), which this unit includes - the `s32`/`u32` copies that stood here
 * clashed with it ((10197) illegal function overloading) once MAIN's landing registered the owner's
 * declarations (rule 2). */
u32 fn_80262940(struct _PLW* self);
u32 fn_802642D0(struct _PLW* self);
u32 fn_802657F8(struct _PLW* self);
/* `pl_model_state_set` (0x80267270) stood here as `u32 (struct _PLW*, u32, s32, u16)`; its owner
 * `Pl/fn_80262940.cpp` defines it `void`, so `include/Pl/fn_80262940.h`, included above, declares it
 * and this header no longer does (rule 2).  `fn_80262940`/`fn_802642D0`/`fn_802657F8` stay: that
 * owner's header does not declare them yet. */
u32 fn_8026F7B4(void);
u32 fn_8026FE44(struct _PLW* self);
u32 fn_8027035C(struct _PLW* self);
u32 fn_80270728(struct _PLW* self);
u32 fn_80270CA4(struct _PLW* self);
u32 fn_80277EC0(struct _PLW* self);
u32 fn_80278578(struct _PLW* self, s32 v);
u32 fn_80278D1C(struct _PLW* self);
u32 fn_80279C20(struct _PLW* self);
u32 fn_8027AF34(struct _PLW* self);
/* `fn_80272E30` is declared by its owner, `include/Pl/pl_skill.h` (`extern "C" s16`,
 * `Pl/pl_skill.cpp:1233`), which this unit includes - the `u32` copy that stood here clashed with it
 * ((10505) illegal function overloading) the moment the owner registered its declaration (rule 2). */
s32 fn_80273228(struct _PLW* self, u16 item_id, s16 value);
/* `fn_8027D4F0` is declared by its owner, `include/Pl/pl_act.h` (`extern "C" void`), which this
 * unit includes - the `u32` copy that stood here clashed with it the moment the owner registered
 * its declaration ((10505) illegal overloading); the call site discards the result (rule 2). */
u32 pl_act_kind_get(struct _PLW* self);

/* ---- library callees (the owners' headers do not declare these) ---- */
 /* 0x80041E40 - copies a 0xC-byte record and
    * returns `dst` (the owner's body); normalised with the declaration fold-in, 2026-09-27 */
u32 fn_8004EB18(u16 item_id);
u32 fn_80050A40(f32 a, f32 b, f32 c, f32 d);
u32 fn_800524C0(f32 a, void* b, void* c, void* d, void* e);
u32 fn_800E0914(void* p);
u32 fn_803C482C(void);
u32 fn_800F16D4(s32 value);
u32 fn_8033112C(void* p);
u32 fn_80335CE8(struct _PLW* self, s32 a, u16 b);
u32 fn_803BA9B0(void* p);

/* ---- mangled callees at C++ scope, so the front-end reproduces the map's spelling (rule 9) ---- */
#ifdef __cplusplus
}
void* get_move_work_adrs(u8 kind);
u8* GetItemData(u16 item_id); /* -> GetItemData__FUs */
u32 get_move_work_max(u8 kind);
u32 Pl_suimen_ck(struct _PLW* self); /* -> Pl_suimen_ck__FP4_PLW */
extern "C" {
#endif

/* This unit's reads of the Pl band's `.sdata2` pools (playbook 29: declared, never defined) are
 * declared by their owners' headers and included where they are used (rule 2): the words inside
 * `Pl/pl_frame_data.cpp`'s run (0x80799E00-0x80799F98) by `Pl/pl_frame_data.h`, and the words of
 * the run that follows (0x80799F98-0x80799FDC) by `Pl/pl_act_data.h`, both included by
 * `Pl/fn_8025F088.cpp`.  Nothing from either run is declared here any more. */

/* The player's per-chunk move-work table (`.bss` 0x806AB848) is owned by `Pl/bss_pool.cpp`, whose
 * header `Pl/bss_pool.h` declares it; nothing from that run is declared here any more (rule 2). */

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_FN_8025F088_H */
