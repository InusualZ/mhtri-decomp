/* ef/ef_drawsmoothstripestrategy.h - nw4r::ef `DrawSmoothStripeStrategy`, the strategy class
 * `ef/ef_drawsmoothstripestrategy.cpp` defines. */
#ifndef MHTRI_EF_EF_DRAWSMOOTHSTRIPESTRATEGY_H
#define MHTRI_EF_EF_DRAWSMOOTHSTRIPESTRATEGY_H

#include "ef/ef_drawstrategyimpl.h"

#ifdef __cplusplus
namespace nw4r {
namespace ef {

/* Draws the particles of a manager as one curve-interpolated stripe or tube. */
class DrawSmoothStripeStrategy : public DrawStrategyImpl {
public:
    /* The ahead context plus the emitter's X axis and the view's Z axis in manager space (the stripe strategy's
     * layout; this constructor leaves the trigonometric table unset). */
    struct AheadContext : public DrawStrategyImpl::AheadContext {
        AheadContext(const MTX34* view_mtx, EfDrawParticleManager* pm);

        /* +0xBC */ VEC3 emitter_axis_x;
        /* +0xC8 */ VEC3 view_axis_z;
        /* +0xD4 */ f32* trig_table;    /* the tube's (cos, sin) pair per side, built by the tube draw */
    }; /* size: 0xD8 */

    DrawSmoothStripeStrategy();
    virtual ~DrawSmoothStripeStrategy() {}
    virtual void Draw(const EfDrawInfo& info, EfDrawParticleManager* pm);
    virtual CalcAheadFunc GetCalcAheadFunc(EfDrawParticleManager* pm);
}; /* size: 0xE0 */

}  // namespace ef
}  // namespace nw4r
#endif

#endif /* MHTRI_EF_EF_DRAWSMOOTHSTRIPESTRATEGY_H */
