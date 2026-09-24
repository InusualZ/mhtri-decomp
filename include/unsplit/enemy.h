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

#ifdef __cplusplus
extern "C" {
#endif

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
void fn_80134964();
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

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_UNSPLIT_ENEMY_H */
