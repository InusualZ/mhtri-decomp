/* unsplit/ef.h - the ef band header: declarations the ef consumers use for symbols the band's units define.  Every
 * address declared here now has a registered owner (the rule-2 residual of this file); the signatures are what the
 * consumers used, the wider form kept where only parameter spellings differed. */
#ifndef MHTRI_UNSPLIT_EF_H
#define MHTRI_UNSPLIT_EF_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

f32 fn_800C9DCC(f32 arg0);
/* 0x800A7750 - `ef/ef_emitter.cpp`'s per-emitter creation entry, called by the creation queue: the emitter form,
 * the effect handle, the setting record, the manager, a life and an optional VEC3 position. */
void fn_800A7750(void* form, void* eh, const void* setting, void* manager, u16 life, const void* pos);
/* 0x800AB740 / 0x800AB658 - `ef/ef_particlemanager.cpp`'s teardown entry and ramp helper (caller:
 * `ef/ef_particle.cpp`). */
void* fn_800AB740(void* table, void* self);
f32 fn_800AB658(void* self, f32 v);


/* `ef/fn_801173AC.cpp`'s eft024 kind-1 state-1 handler. */
void fn_801173AC(void* self);
/* `VEC3_ctor` (0x80043EA8) is `src/mh3_pad.cpp`'s and is declared in `mh3_pad.h`, which the
 * consumers of this band include (docs/plan.md 6.5 rule 2: a band header declares no owned symbol). */

/* 0x800CFA90 / 0x800CFAD0 - `ef/system_core.cpp`'s move-work record table and its record count; `Pl/fn_80273B14.cpp`
 * walks the records by their +0x008 slot byte. */
#ifdef __cplusplus
extern "C++" { /* the map spells both `__FUc`: C++ free functions, not C names */
#endif
void* get_move_work_adrs(u8 kind);
u32 get_move_work_max(u8 kind);
#ifdef __cplusplus
}
#endif
struct Vec;

/* 0x800C68E8 - `ef/ef_drawstrategyimpl.cpp`'s per-particle draw helper the free/line/point/smooth strategies call;
 * `fn_800C5F74` and `fn_800C6064` are declared in `ef/ef_drawstrategyimpl.h`. */
void fn_800C68E8(void* self, void* particle, void* ed, void* em, u32 first, u32 rebindColor);

/* `ef/ef_drawstripestrategy.cpp`'s helpers the free-strategy draw calls (caller: `ef/ef_drawfreestrategy.cpp`). */
void fn_800B7DB0(void* a, MTX34* out);
void* fn_800B4B04(void* self, s16 flag);   /* the owner defines it; this is the ABI */
void fn_800B54B4(const void* src, Vec3* out);

/* The effect record the band's callers pass. */
struct _EFT;

/* `eft_net_send` (0x803386C4) is `hud/pl_frame_sync.cpp`'s, declared in `hud/eft_net_send.h`. */

#ifdef __cplusplus
}
#endif

/* The target objects reference this by its C++ mangling
 * (`vec_to_mh_vec3__FPQ34nw4r4math4VEC3P3Vec`), so it is declared with C++ linkage (relocaudit). */
#ifdef __cplusplus
void vec_to_mh_vec3(Vec3* dst, struct Vec* src);   /* converts the engine vector to nw4r's */
#endif

#endif /* MHTRI_UNSPLIT_EF_H */
