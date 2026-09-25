/* Not-yet-reconstructed `enemy`-band symbols: addresses whose bracketing registered units both name the `enemy` module, so the declaration belongs here until the owning unit is registered.
 *
 * Declarations moved here from the consumer units' `src/` files (docs/plan.md 6.5 rule 2:
 * an extern lives with the TU that owns the symbol).  The signature set is what the
 * consumers used; where only the parameter spelling differed the wider form is kept.
 */
#ifndef MHTRI_UNSPLIT_ENEMY_H
#define MHTRI_UNSPLIT_ENEMY_H

#include "types.h"
#include "nw4r/math.h"

struct _ENEMY_WORK;
struct EnemyData;

#ifdef __cplusplus
extern "C" {
#endif

struct EnemyData* fn_80140C00(u8 group, u8 index);
u8* fn_8014260C(u8 id);

void CancelFade(struct _ENEMY_WORK *self);
u32 em_frame_check__FP11_ENEMY_WORKUsff(struct _ENEMY_WORK *self, u16 a, f32 b, f32 c);
u32 em_sleep_ck__FP11_ENEMY_WORKUc(struct _ENEMY_WORK* enemy, u8 kind);
u32 fn_8012EC60(void);
u32 fn_8012ECF0(void);
void fn_8012F5B8(struct _ENEMY_WORK* self, s32 a, s32 b, s32 c);
void fn_8012F62C(struct _ENEMY_WORK *self, u32 a, u32 b, u32 c);
void fn_8012F8C8();
f32 fn_8012F8E4(struct _ENEMY_WORK *self);
f32 fn_8012F8EC(struct _ENEMY_WORK *self);
f32 fn_8012F8F4(struct _ENEMY_WORK *self);
u32 fn_8012F93C(struct _ENEMY_WORK *self);
void fn_8012FC60(struct _ENEMY_WORK* work);
void fn_8012FCC4(struct _ENEMY_WORK* work, s32 arg1, f32 arg2);
void fn_8012FCE4(struct _ENEMY_WORK* work);
void fn_8012FF38(struct _ENEMY_WORK* work);
u32 fn_80130008();
u8 fn_8013023C(struct _ENEMY_WORK* work);
f32 fn_80130248(struct _ENEMY_WORK* self);
f32 fn_801302E4(struct _ENEMY_WORK* work);
void fn_801303EC();
void fn_801303FC();
void fn_80130438(struct _ENEMY_WORK* work);
void fn_80130478(struct _ENEMY_WORK *self, u32 a);
void fn_801305C4(struct _ENEMY_WORK *self);
u32 fn_80130778(s32 kind);
void fn_80130858(struct _ENEMY_WORK* enemy, s16 value);
void fn_80130A10(struct _ENEMY_WORK* enemy, s32 value);
void fn_80130CDC();
u32 fn_80130DF8(void);
struct _ENEMY_WORK* fn_80131034(struct _ENEMY_WORK* self, u8 kind, u8 distance_check);
void fn_80131150(struct _ENEMY_WORK* work);
void fn_80131D9C(struct _ENEMY_WORK* work);
void fn_80131DB4(struct _ENEMY_WORK* work);
void fn_80131DF4(struct _ENEMY_WORK* work);
void fn_80131E0C(struct _ENEMY_WORK* work);
void fn_80131E74(struct _ENEMY_WORK* work);
void fn_80131EC0();
void fn_801320A4(struct _ENEMY_WORK* work);
u32 fn_80132184(void);
u32 fn_801322CC(struct _ENEMY_WORK* enemy, s32 value);
void fn_801324E0(struct _ENEMY_WORK* work);
void fn_801333E0(struct _ENEMY_WORK* work);
void fn_80133B5C(struct _ENEMY_WORK* work);
void fn_80133BB4(struct _ENEMY_WORK* enemy);
void fn_80133BC0(struct _ENEMY_WORK* work);
void fn_80133C30(struct _ENEMY_WORK* work);
void fn_80133C3C(struct _ENEMY_WORK *self);
u32 fn_80133C50(struct _ENEMY_WORK *self, u32 a);
void fn_80133CC8(struct _ENEMY_WORK *self, u32 a, u32 b);
u16 fn_80133DB0();
void fn_80133E3C(struct _ENEMY_WORK *self, s32 a, f32 b, f32 c);
void fn_80133F4C(struct _ENEMY_WORK *self, f32 a, f32 b);
u32 fn_80134114(struct _ENEMY_WORK* self, s32 a, s32 b);
void fn_80134964(struct _ENEMY_WORK* self, void* tbl, s32 a, s32 b, s32 c);
u32 fn_80134B0C(struct _ENEMY_WORK *self, void *tbl);
void fn_80134DF4(struct _ENEMY_WORK *self);
void fn_80134E28(struct _ENEMY_WORK *self);
void fn_80134E8C(struct _ENEMY_WORK *self);
void fn_80134F18(struct _ENEMY_WORK *self);
void fn_80134F70(struct _ENEMY_WORK* self, void* tbl);
void fn_80135000(struct _ENEMY_WORK* self, u32 a, void* tbl);
void fn_801353E4(struct _ENEMY_WORK *self);
void fn_801353F8(struct _ENEMY_WORK *self);
void fn_80135418(struct _ENEMY_WORK *self);
void fn_801354F4(struct _ENEMY_WORK *self, void *p);
void fn_80135584(struct _ENEMY_WORK* self, void* p);
void fn_801355C8();
u32 fn_80135600();
f32 fn_80135644(struct _ENEMY_WORK *self, void *tbl);
f32 fn_801356A8(struct _ENEMY_WORK *self, f32 a, f32 b, f32 c);
s32 fn_80135748();
u32 fn_80135BC4(struct _ENEMY_WORK* work, s32 arg1);
void fn_801363F8(struct _ENEMY_WORK* work);
void fn_80136D14(struct _ENEMY_WORK* work);
void fn_80136D4C();
void fn_80136E38(struct _ENEMY_WORK* work, s32 arg1);
void fn_801373D0(struct _ENEMY_WORK* work);
s32 fn_80137C9C(struct _ENEMY_WORK* work, void* arg1);
void fn_80137DD0(struct _ENEMY_WORK* work);
s32 fn_80137EE0(struct _ENEMY_WORK* work, s32 arg1);
void fn_80138024(struct _ENEMY_WORK* work, s32 a, s32 b);
void fn_8013BDE4(u8 **in, u32 id, s16 *out);
u32 fn_801406E0(u32 id, u32 cmd);
s16 fn_80140778(u8 *stream, u32 id, u32 mode);
s32 fn_801408B4(struct _ENEMY_WORK* work);
void fn_801409C8(struct _ENEMY_WORK *self, u8 *in, u32 id, u32 sub, s32 value);
void fn_80140AF8(struct _ENEMY_WORK *self, u32 a, u32 b);
void fn_80140B10(struct _ENEMY_WORK *self, u32 a, u32 b);
void fn_80143190(void);
void fn_80144584(s32 kind);
void fn_801481FC(struct _ENEMY_WORK* self);
void fn_801493A8();
void fn_80149788();
void fn_801498C8();
void fn_80149A08();
void fn_80149AFC();
void fn_80149C54();
void fn_80149C58();
f32 get_em_chg_scale__FP11_ENEMY_WORK(struct _ENEMY_WORK* work);
f32 get_em_scale__FP11_ENEMY_WORK(struct _ENEMY_WORK* work);
void get_joint_wpos_em__FP11_ENEMY_WORKUlPQ34nw4r4math4VEC3(struct _ENEMY_WORK* work, u32 joint, Vec3* out);

/* The enemy action band 0x80127F48.. and the handler band 0x80170A54..0x80170EF4, owned by the
 * not-yet-registered proposals `proposal/8016xxxx`/`proposal/8017xxxx`.  Added by the
 * `enemy/fn_80170FA8.cpp` registration: its dispatcher tail-calls the 0x80170xxx handlers and the
 * state machines call `fn_80127F48`/`fn_80128A14`.  The band brackets as `enemy` on both sides
 * (fn_8014A1BC .. fn_80170FA8), so rule 2 sends the declarations here.  Signatures: `self` only for
 * the handlers that take one argument, and `fn_80170EF4` takes the action's extra selector in r4.
 *
 * `fn_80128A14`'s last two are u8 in the consumer's view, but the owner's body narrows them itself
 * (`clrlwi r4,r4,24`/`clrlwi r5,r5,24`), so the definition takes u32 and the declaration here is
 * widened to match the owner (enemy/fn_801251D0.cpp).  Three landed units used to declare them three
 * ways in their own files (`u8` in
 * fn_80149D6C, `u32` in fn_8014A1BC, `s32` in fn_80177890) - the same rule-2 debt.  Every
 * call site passes a constant, so the spelling is codegen-neutral. */
void fn_80127F48(struct _ENEMY_WORK* self);
void fn_80128A14(struct _ENEMY_WORK* self, u32 a, u32 b);
void fn_80170A54(struct _ENEMY_WORK* self);
void fn_80170AD0(struct _ENEMY_WORK* self);
void fn_80170B4C(struct _ENEMY_WORK* self);
void fn_80170C68(struct _ENEMY_WORK* self);
void fn_80170D04(struct _ENEMY_WORK* self);
void fn_80170D74(struct _ENEMY_WORK* self);
void fn_80170DF0(struct _ENEMY_WORK* self);
void fn_80170E78(struct _ENEMY_WORK* self);
void fn_80170EF4(struct _ENEMY_WORK* self, u32 a);
/* Declarations moved here from `enemy/fn_80176C58.cpp` (docs/plan.md 6.5 rule 2). */
void fn_8012F5C4(struct _ENEMY_WORK* self, u32 a, u32 b, u32 c, u32 d);
u32 fn_8012EC3C(struct _ENEMY_WORK* self);
void fn_80130F74(struct _ENEMY_WORK* self);
void fn_801376B4(struct _ENEMY_WORK* self);
void fn_80135C5C(struct _ENEMY_WORK* self, u32 a, u32 b);
void fn_80147E2C(void* self);
/* Declarations moved here from `enemy/fn_801679B0.cpp` (docs/plan.md 6.5 rule 2): enemy-band
 * symbols no registered unit owns.  Guarded for C++ because the C consumers carry their own
 * ABI-equivalent spellings of `fn_80134004` (`f32,u16` in `enemy/fn_80149D6C.c`, `u32,f32` in
 * `enemy/fn_8014A1BC.c`), which MWCC's C front-end treats as a conflicting redeclaration. */
#ifdef __cplusplus
void fn_80131D84(struct _ENEMY_WORK* self);
void fn_80134004(struct _ENEMY_WORK* self, u32 a, f32 b);
void fn_80167404(struct _ENEMY_WORK* self);
void fn_80167968(struct _ENEMY_WORK* self);
void fn_801321C4(struct _ENEMY_WORK* self);
void fn_801321D0(struct _ENEMY_WORK* self);
#endif
void fn_8013221C(struct _ENEMY_WORK* self, f32 a, u32 b, u32 c);
void fn_80132224(struct _ENEMY_WORK* self);
void fn_80132264(struct _ENEMY_WORK* self);
void fn_80141B88(u16 a, s32 b, s32 c, u8 d, u8 e, s32 f, s32 g, s32 h, s32 i, s32 j, s32 k);
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
/* The C++ spellings of the mangled callees this band calls, so a call site never spells the
 * mangling (docs/plan.md 6.5 rule 9); each mangles back to its map name.  `get_joint_wpos_em`
 * itself is C++ in the target (`get_joint_wpos_em__FP11_ENEMY_WORKUlPQ34nw4r4math4VEC3`), so the
 * declaration moves here from the extern "C" block (relocaudit). */
u32 em_frame_check(struct _ENEMY_WORK* self, u16 a, f32 b, f32 c);
void get_joint_wpos_em(struct _ENEMY_WORK* enemy, u32 joint, Vec3* out);
#endif

#endif /* MHTRI_UNSPLIT_ENEMY_H */
