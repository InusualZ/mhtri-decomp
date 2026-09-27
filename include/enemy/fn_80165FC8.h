/* The enemy state/action unit `enemy/fn_80165FC8.cpp` (0x80165FC8..0x801679B0, 21 functions).
 *
 * This header holds what the unit needs from outside its own range: the two records its bodies read
 * (the `enemy_data_find` entry and the `ResUserDataAc` accessor the range's three non-state methods
 * belong to), and the declarations of the flat `fn_XXXXXXXX` symbols the range calls.  The flat names
 * are C-linkage and never spelled as a mangling (docs/plan.md 6.5 rule 9); the genuinely mangled
 * callees are declared at C++ scope with their real signatures.
 *
 * The record layouts are the union of the offsets this unit reads and the offsets the records' other
 * readers name (`enemy/fn_8013BE60.c`'s `_ENEMY_DATA`, `enemy/fn_80170600.cpp`'s `ENEMY_ENTRY`; the
 * two are the same record).  `ResUserDataAc` is the `g3d_resuser_ac.h` accessor `enemy/fn_80138074.c`
 * reconstructs (its vtable is `lbl_805A6D28`, stored by that unit's `fn_8015DAA8` constructor); the
 * three methods of this range that belong to it (`fn_801661BC`/`fn_801661FC`/`fn_801662D4`) are
 * emitted here because their addresses fall in this range.
 */
#ifndef MHTRI_ENEMY_FN_80165FC8_H
#define MHTRI_ENEMY_FN_80165FC8_H

#include "types.h"
#include "nw4r/math.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif

/* The record `enemy_data_find` returns lives in `include/enemy/ENEMY_DATA.h` (rule 1: one home
 * for the type, now that a second unit reads it). */
#include "enemy/ENEMY_DATA.h"

/* The user-data accessor record `enemy/fn_80138074.c` reconstructs: the vtable at +0 (emitted by the
 * other TU, `lbl_805A6D28`), the enemy work at +4, a flag word at +8.  size: 0x0C */
typedef struct ResUserDataAc {
    /* +0x00 */ void* vtable;
    /* +0x04 */ struct _ENEMY_WORK* work;
    /* +0x08 */ u32 flags;
} ResUserDataAc;

/* ---- the range's own flat entry points (called through their map names) ---- */
u32 fn_80165FC8(struct _ENEMY_WORK* self, u32 arg1);
void fn_801661BC(ResUserDataAc* self);
void fn_801661FC(ResUserDataAc* self, nw4r::math::MTX34* mtx, void* cursor, s32 arg3);
ResUserDataAc* fn_801662D4(ResUserDataAc* self, s32 flags);
void fn_80166330(void);
u32 fn_801663E4(struct _ENEMY_WORK* self);
void fn_80166DF8(struct _ENEMY_WORK* self, u32 kind);
void fn_801671A8(void);
void fn_801671AC(struct _ENEMY_WORK* self);
void fn_8016730C(struct _ENEMY_WORK* self);
void fn_80167388(struct _ENEMY_WORK* self);
void fn_80167404(struct _ENEMY_WORK* self);
void fn_80167458(struct _ENEMY_WORK* self);
void fn_801674D4(struct _ENEMY_WORK* self);
void fn_80167550(struct _ENEMY_WORK* self);
void fn_801675CC(struct _ENEMY_WORK* self);
void fn_80167648(struct _ENEMY_WORK* self);
void fn_801676C4(struct _ENEMY_WORK* self);
void fn_80167770(struct _ENEMY_WORK* self);
void fn_801678A0(struct _ENEMY_WORK* self);
void fn_80167968(struct _ENEMY_WORK* self);

/* ---- enemy-band callees owned by other registered units ---- */
u32 fn_80127308(struct _ENEMY_WORK* self, const void* src, nw4r::math::VEC3* dst, u32 a);
void fn_80126324(struct _ENEMY_WORK* self, u32 a, u32 b, f32 c);
void em_action_finish(struct _ENEMY_WORK* self);
void fn_80128A8C(struct _ENEMY_WORK* self, u8 a, u8 b);
u32 fn_80129A1C(struct _ENEMY_WORK* self, u16 a, u32 b, s16* timer);
u32 fn_80129A70(struct _ENEMY_WORK* self, u16 a);
u32 fn_80129D3C(struct _ENEMY_WORK* self);
u8 fn_80129DB8(struct _ENEMY_WORK* self);
/* 0x8012A014 - owned by `include/enemy/fn_801251D0.h`; the spelling here is the owner's (its body
 * forwards r7/r8 to `fn_80129F5C` as the pair of table pointers it walks, `cmpwi r4,0`).  The older
 * `u32 d, const void* e` copy was the second declaration that stopped this header and the owner's
 * being included together (same class as `fn_80128A8C`). */
u32 fn_8012A014(struct _ENEMY_WORK* self, u32 a, u32 b, u16 c, void* d, void* e);
u32 fn_8012A204(struct _ENEMY_WORK* self);
void fn_8012B380(struct _ENEMY_WORK* self, u32 a, u32 b, u32 c);
void fn_8012CEB4(struct _ENEMY_WORK* self, s16 timer, u8 index);
u32 fn_8012ECF0(struct _ENEMY_WORK* self);
u8 fn_8015D934(struct _ENEMY_WORK* self);
void fn_8012E664(struct _ENEMY_WORK* self);
void fn_8012E694(struct _ENEMY_WORK* self);
void em_mot_set(struct _ENEMY_WORK* self, u32 a, u32 b, u32 c);
void em_mot_set_ck(struct _ENEMY_WORK* self, u32 a, u32 b, u32 c);
u32 em_mot_end_ck(struct _ENEMY_WORK* self);
void fn_8012FCC4(struct _ENEMY_WORK* self, u32 a, f32 b);
void fn_80130248(struct _ENEMY_WORK* self);
void em_move_mode_set(struct _ENEMY_WORK* self, u32 a);
void fn_801305C4(struct _ENEMY_WORK* self);
void fn_8013072C(struct _ENEMY_WORK* self, u32 a, u32 b);
void fn_80131D84(void);
void fn_80131DF4(struct _ENEMY_WORK* self);
void fn_80131E74(struct _ENEMY_WORK* self);
u32 fn_801337FC(struct _ENEMY_WORK* self);
void fn_80133BC0(struct _ENEMY_WORK* self);
void fn_801353F8(struct _ENEMY_WORK* self);
void fn_801355C8(struct _ENEMY_WORK* self, void* p);
void fn_80135764(nw4r::math::VEC3* a, nw4r::math::VEC3* b, u32 c, u16 d, f32 e);
void fn_8013A654(ResUserDataAc* self, u32 a);
void fn_8013AAC4(struct _ENEMY_WORK* self);
void fn_8013918C(void* self, s16 a);
void fn_8016C674(struct _ENEMY_WORK* self, u8* a, u8* b);
u32 fn_80176AA8(void* p);

/* ---- library callees (their owners' headers do not declare these, or declare a different
 * signature; the shapes here are the call sites' - each is a leaf this unit never re-enters) ---- */
void fn_8005D1AC(void* out, s32 a);
void fn_800504D4(void* out);
void fn_800532DC(void* dst, const void* src);
int fn_8006FDCC(const void* p);
void fn_8005D0CC(void* obj, const void* sub);
void fn_80051490(nw4r::math::VEC3* dst, const nw4r::math::VEC3* src);
void fn_80051574(nw4r::math::MTX34* dst, const nw4r::math::MTX34* src);
void* fn_80097EB0(void* sub, s32 a);
u8 fn_802B0668(u8 kind);
u32 fn_802B0998(u32 kind);
/* `enemy_data_find`/`enemy_data_grp` (0x803438E4 / 0x803439D4): declared in their owner's header,
 * `include/ef/eft_slot.h`, since `ef/eft_slot.cpp` registered the band that defines them (rule 2). */
void rotMatrixX(u32 angle, nw4r::math::MTX34* m);
void rotMatrixZ(u32 angle, nw4r::math::MTX34* m);

/* ---- the unit's read-only pool constants and the seat table (referenced, not defined here) ---- */
extern f32 lbl_80797330;
extern f32 lbl_80797338;
extern f32 lbl_807974EC;
extern f32 lbl_807974F0;
extern f32 lbl_807974F4;
extern f32 lbl_807974F8;
extern f32 lbl_807974FC;
extern f32 lbl_80797500;
extern f32 lbl_80797504;
extern f32 lbl_80797508;
extern f32 lbl_8079750C;
extern f32 lbl_80797510;
extern f32 lbl_80797514;
extern u8 lbl_805A5DB4[];
extern nw4r::math::VEC3 lbl_806A7868[];
extern nw4r::math::VEC3 lbl_806A7880[];

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
/* The genuinely mangled callees, at C++ scope so the front-end emits the map's spelling
 * (rule 9: the call site never writes the mangling). */
void* get_move_work_adrs(u8 kind);
u32 get_move_work_max(u8 kind);
s32 ran_suu(s32 n);
u32 em_frame_check(struct _ENEMY_WORK* self, u16 a, f32 b, f32 c);
s32 em_die_ck(struct _ENEMY_WORK* self);
u32 em_act_ck(struct _ENEMY_WORK* self, u8 a, u8 b);
#endif

#endif /* MHTRI_ENEMY_FN_80165FC8_H */
