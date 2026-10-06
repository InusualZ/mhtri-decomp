/*
 * Symbols whose module is UNDECIDED (docs/plan.md 6.5 rule 2).
 *
 * The registered unit bands interleave across modules, so an address whose nearest registered unit below
 * and nearest above name different modules (or, as here, sit in a section with no registered range at all)
 * has no sound `<module>.h` to move to.  Those sites are documented here rather than guessed into a wrong
 * module - a wrong module is worse than a documented unknown.
 *
 * `system_w` (.bss 0x806585E0) used to be declared here; `src/mh3_pad.cpp` now owns its `.bss` and the type and
 * declaration live in `mh3_pad/system_w.h`, which this header still includes for its consumers.
 */
#ifndef MHTRI_UNSPLIT_UNKNOWN_H
#define MHTRI_UNSPLIT_UNKNOWN_H

#include "types.h"
#include "nw4r/math.h"
#include "gx.h"

/* Owner headers (rule 2): `camera/fn_802B5C58.cpp` owns 0x802B5C58-0x802BEAAC, which covers the
 * 0x802B8DF8/0x802BE638 sites declared here until that range registered. */
#include "camera/camera.h"
#include "ef/fn_8030681C.h"

struct MHchar;
struct _CP_VECTOR;
struct _PLW;
struct _ENEMY_WORK;

#ifdef __cplusplus
/* Declared, never included: these declarations only need the type by pointer, and
 * including `pl.h`/`ef.h` here would pull their full records into units that carry their own view. */
namespace nw4r { namespace ef { struct Effect; } }
#endif

#ifdef __cplusplus
extern "C" {
#endif
/* Declarations whose module band is ambiguous (the registered units bracketing the address name
 * different modules).  Added with `Pl/fn_80262940.cpp` (proposal 80262940).  `fn_80223E54` was
 * originally here too; MAIN later registered the lobby range `lobby/fn_8021E1EC.cpp`
 * (0x8021E1EC-0x80224AC4) over it, so its declaration moved to that owner's header (rule 2). */
s32 fn_8035B700(s32 a, s32 b, u16 c); /* 0x8035B700 - bracket: hud below, Network above */

#include "mh3_pad/system_w.h" /* `SystemWork`/`system_w`, owned by src/mh3_pad.cpp (rule 1/2) */

/* The unowned C helpers `enemy/fn_8019ED34.cpp` calls: addresses whose bracketing registered units
 * name different modules (0x802B/0x803), so no `<module>.h` is sound - the rule 2 named gap.  The
 * signatures are the call sites' (r3 the work record; `eft_em_spawn_param`'s fifth argument is the s32
 * `0`/0xF4A0/0xB61 the target materialises; `quest_flag_200000_ck` returns the r3 word compared against 1). */
/* `fn_802BE638` (0x802BE638) is in `camera/fn_802B5C58.cpp`'s range 0x802B5C58-0x802BEAAC, so the
 * owner's header declares it and this one includes it (rule 2). */
void eft_em_spawn_param(struct _ENEMY_WORK* self, u32 a, u32 b, void* v, f32 s, s32 d);
/* `enemy_data_find`/`enemy_data_grp` (0x803438E4 / 0x803439D4) are declared in their owner's
 * header, `ef/eft_slot.h`, since `ef/eft_slot.cpp` registered the band that defines them -
 * a declaration here of a symbol a registered unit owns is rule 2's finding.  Their consumers
 * include that header. */

/* 0x802D884C / 0x802DE578 / 0x8042CB9C - helpers `Pl/fn_802489D4.cpp`
 * (0x802489D4-0x8024F200) calls whose address bands have no registered range at all, so no
 * `<module>.h` is sound for them (this file's own note, above).  Their map names are bare
 * `fn_XXXXXXXX`, so they keep C linkage like everything else in this block.  `fn_802B8DF8`
 * (0x802B8DF8) left this group when `camera/fn_802B5C58.cpp` registered that range - its owner's
 * header carries it now (rule 2). */
void fn_802D884C(u16* a, s16* b);
void fn_802DE578(struct _PLW* self, void* work);
s32 fn_80331104(void);
/* Added with `Pl/fn_80273B14.cpp`: 0x80335CE8 (hud below, enemy above) names different modules, so its
 * home is this file.  0x8029F73C moved to `menu/menu_item.h` (rule 2): `menu/menu_item.cpp`
 * registered the range 0x8029F3C8..0x802A6624, which owns that address. */

/* 0x8033A920 and 0x80463EE0 - called by `enemy/fn_802F5138.cpp`'s action band (an idle-mode
 * retire request and a release request).  Neither address has a registered unit in either
 * direction, and their nearest registered neighbours name different modules
 * (`hud/fn_80324F7C.c` below 0x8033A920, `enemy/fn_8035E034.cpp` above it;
 * `Runtime.PPCEABI.H/Gecko_ExceptionPPC.cp` below 0x80463EE0, `AX/AXFXReverbHi.c` above it), so
 * the module is undecided and the declaration belongs in this band, not in a guessed one.  The
 * 0x802FA9A0 / 0x802FAB98 / 0x802FAFB4 / 0x802FAFC4 group left this block when main landed
 * `lobby/fn_802FA9A0.cpp` (0x802FA9A0..0x8030121C, which owns all four) - they are declared in
 * `lobby/fn_802FA9A0.h` now (rule 2). */
void fn_8033A920(u32 arg);
/* 0x80463EE0 - a float-returning two-argument function: `ef/fn_80114E34.cpp` carries the
 * signature its own call sites set (`f32 fn_80463EE0(s16, f32)`), which is the one declared
 * here; the action band tail-calls it with its own parameters. */
f32 fn_80463EE0(s16 a, f32 b);
/* The `.sdata2` / `.data` pool entries the 0x805482CC-0x8054E894 game-UI band loads.  The band's
 * target object carries no data section at all, so every constant it uses is another translation
 * unit's pool entry and is declared here `extern` and used as a load operand - never defined
 * (playbook 29/58; a definition would make the object emit its own copy and drift the DOL).  The
 * address band names no module (the nearest registered ranges are `DWCi/fn_805113B0.c` below and
 * `homebutton/fn_80555374.cpp` above), so this file is their home (rule 2's named gap).
 * Added with the `fn_805482CC.cpp` registration. */
extern f32 lbl_8079D628; /* 640.0f - the row bound fn_80548394 writes into a text record */
extern f64 lbl_8079D630; /* the u32->double magic 4503599627370496.0 */
extern f32 lbl_8079D638; /* 1.0f */
extern f32 lbl_8079D63C; /* 0.5f */
extern f32 lbl_8079D698; /* 15.0f */

/* `.sbss` 0x80794868 - the frame counter: `main.cpp`'s frame end increments it after the retrace wait, the lobby item
 * list and the stream player copy it as a tick stamp, and the network layer callback stamps peer joins with it.  No
 * registered range covers it (the bracketing `.sbss` claims are `main.cpp` below and `fn_80047398.cpp` above). */
extern u32 frame_counter;

/* The helpers the 0x805482CC-0x8054E894 band tail-calls into its unregistered neighbours (the bands
 * below at 0x8054F788 / 0x805526xx and above at 0x80553xxx / 0x8055Bxxx, none of which is registered
 * yet, so rule 2's owner has no header to name).  Signatures are the call sites': r3 is the address
 * the caller hands over, and `fn_8055A3E0` takes the sub-object plus the mode word the caller
 * materialises. */
void fn_8054F788(void* record);
void fn_80553670(void* record);
void fn_80526D00(void* record);
/* 0x8055A3E0 / 0x8055BEF0 / 0x8055C1D4 / 0x8055C2CC are inside the registered band
 * `homebutton/fn_80555374.cpp`, so `include/homebutton/fn_80555374.h` declares them (rule 2). */

/* Merged 2026-09-24: a second lane formalized into this shared header.  Declarations it
 * needed that the first did not; a symbol both named keeps the first (verified) signature. */
#ifdef __cplusplus
}
#endif

/* The target objects reference these by their C++ manglings (`get_now_areano__Fv`,
 * `get_stg_eft_col__FUcUc`, `cpSetRotMatrix__FP10_CP_VECTORPQ34nw4r4math5MTX34`, ...), so they are
 * declared with C++ linkage.  The one remaining C consumer (`sound/fn_800DCFEC.c`) keeps the plain C
 * declarations below until its own language is resolved (that unit is a follow-up). */
#ifdef __cplusplus
u8 get_now_areano();
u8 get_now_mapno();
void* res_eft_model_create(struct MHchar* chr, u16 id, u32 arg);
void* res_eft_model_create_light(struct MHchar* chr, u16 id, u32 a, long b);
u32 get_stg_eft_col(u8 area, u8 which);
u8 eftGetKeyAlpha(u8* key, long frame);
void eftGetKeyRGB(u8* keys, long frame, u8* r, u8* g, u8* b);
u16 Get_motion_no(struct _PLW* plw);
void cpSetRotMatrix(struct _CP_VECTOR* rot, Mtx34* mtx);
#else
u8 get_now_areano();
u8 get_now_mapno();
void* res_eft_model_create(struct MHchar* chr, u16 id, u32 arg);
void* res_eft_model_create_light(struct MHchar* chr, u16 id, u32 a, long b);
u32 get_stg_eft_col(u8 area, u8 which);
u8 eftGetKeyAlpha(u8* key, long frame);
void eftGetKeyRGB(u8* keys, long frame, u8* r, u8* g, u8* b);
u16 Get_motion_no(struct _PLW* plw);
void cpSetRotMatrix(struct _CP_VECTOR* rot, Mtx34* mtx);
#endif

#ifdef __cplusplus /* C++-only: outside the extern "C" block, so C++ linkage is kept */
void SetRootMtxTrans(nw4r::ef::Effect* effect, nw4r::math::VEC3* pos);
u32 effect_move(nw4r::ef::Effect* effect);
void change_paramscale_eff(nw4r::ef::Effect* effect, f32 scale);
void change_paramscale_eff_vec3(nw4r::ef::Effect* effect, nw4r::math::VEC3* vec);
void push_eft_effect_heap_num(nw4r::ef::Effect** effects, long count);
void change_color_eff(nw4r::ef::Effect* effect, nw4r::math::VEC3* pos, _GXColor color);
nw4r::ef::Effect* res_eft_create(u16 id, u16 param, u32 idx);
#endif

#endif /* MHTRI_UNSPLIT_UNKNOWN_H */
