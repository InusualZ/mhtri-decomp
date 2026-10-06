/* ef/ef_drawstripestrategy.h - symbols `ef/ef_drawstripestrategy.cpp` owns: the draw-time particle copies and
 * ahead-vector builders (0x800B8788-0x800B882C) that `ef/ef_drawbillboardstrategy.cpp`'s dispatch hands out.  The
 * unit's other declarations are in `ef/fn_800AEE48.h`. */
#ifndef MHTRI_EF_EF_DRAWSTRIPESTRATEGY_H
#define MHTRI_EF_EF_DRAWSTRIPESTRATEGY_H

#include "types.h"
#include "nw4r/math.h"
#include "ef/ef_drawstrategy.h"

struct EfParticleState; /* ef/fn_800AEE48.h */

/* One tube ring: its centre, its X and Z axes and the texture coordinate along the stripe (the stripe and smooth
 * stripe tubes; the copy moves the vectors as words and the scalar as a float). */
typedef struct EfStripeParam {
    /* +0x00 */ VEC3 center;
    /* +0x0C */ VEC3 side;
    /* +0x18 */ VEC3 up;
    /* +0x24 */ f32 tex_t;
} EfStripeParam; /* size: 0x28 */

#ifdef __cplusplus
extern "C" {
#endif

/* A node whose +0xAC vector the ahead builder reads. size: 0xB8 */
typedef struct EfAheadItem {
    u8 pad_0x00[0xAC]; /* +0x00 */
    Vec field_0xAC;    /* +0xAC */
} EfAheadItem;

/* 0x800B8788 - builds the particle's +0xB0 transform into a local and copies it into `dst`. */
void ef_ahead_manager_axis_y(nw4r::math::VEC3* dst, struct EfParticleState* particle);
/* 0x800B87C8 - copies the particle's +0x98 block into `dst`. */
void ef_ahead_emitter_axis_y(nw4r::math::VEC3* dst, struct EfParticleState* particle);
/* 0x800B87D0 - subtracts the two ahead vectors; falls back to the particle's reference block when the result is degenerate. */
void ef_ahead_from_emitter(Vec* a, struct EfParticleState* particle, EfAheadItem* item);
/* 0x800B882C - builds the ahead vector; `arg` is the effect's ahead-context object, handed through to `ef_particle_get_move_dir`. */
/* untyped: opaque handle passed through to ef_particle_get_move_dir */
void ef_ahead_move_dir(Vec* a, struct EfParticleState* particle, void* arg);

/* The helpers the free and line strategies' draws call. */
/* 0x800B7DB0 - the view matrix of a draw, pushed along the view direction by the draw's depth offset. */
MTX34* ef_draw_info_view_mtx(const EfDrawInfo* info, MTX34* result);
/* untyped: caller-owned payload - the particle record whose rotation is read */
void ef_particle_get_rotate(const void* src, Vec3* out);
/* 0x800B5B34 (0xC): the next node of the manager's particle list (its link sits at the manager's per-list offset). */
/* untyped: opaque handle - a list node reached through a runtime byte offset */
void* ef_pm_list_next(void* self, void* node);
/* 0x800B5B40 (0x8): the head of the manager's particle list. */
u32 ef_pm_list_head(struct EfParticleState* self);
/* 0x800B7F58 - transforms `v` by `m` into `dst` and returns `dst`. */
VEC3* ef_vec3_transform(VEC3* dst, const MTX34* m, const VEC3* v);
/* 0x800B612C (0xC): the Z unit vector. */
const VEC3* ef_get_unit_z_vec(void);
/* 0x800B6138 (0xC): the X unit vector. */
const VEC3* ef_get_unit_x_vec(void);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#include "ef/ef_drawstrategyimpl.h"

extern "C" {
/* 0x800B5B54 (0x10): the number of particles the manager holds. */
s32 ef_stripe_draw_count(nw4r::ef::DrawStrategyImpl* self, EfDrawParticleManager* pm);
}

namespace nw4r {
namespace ef {

/* Draws the particles of a manager as one straight-segment stripe. */
class DrawStripeStrategy : public DrawStrategyImpl {
public:
    /* The ahead context plus the emitter's X axis and the view's Z axis in manager space. */
    struct AheadContext : public DrawStrategyImpl::AheadContext {
        AheadContext(const MTX34* view_mtx, EfDrawParticleManager* pm);

        /* +0xBC */ VEC3 emitter_axis_x;
        /* +0xC8 */ VEC3 view_axis_z;
        /* +0xD4 */ f32* trig_table;    /* the tube's (sin, cos) pair per side, built by the tube draw */
    }; /* size: 0xD8 */

    DrawStripeStrategy();
    virtual ~DrawStripeStrategy() {}
    virtual void Draw(const EfDrawInfo& info, EfDrawParticleManager* pm);
    virtual CalcAheadFunc GetCalcAheadFunc(EfDrawParticleManager* pm);
}; /* size: 0xE0 */

}  // namespace ef
}  // namespace nw4r
#endif

#endif /* MHTRI_EF_EF_DRAWSTRIPESTRATEGY_H */
