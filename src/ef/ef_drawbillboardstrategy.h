/* ef/ef_drawbillboardstrategy.h - nw4r::ef `DrawBillboardStrategy` and `DrawDirectionalStrategy`, the two strategy
 * classes `ef/ef_drawbillboardstrategy.cpp`'s range defines (the directional one is its second TU). */
#ifndef MHTRI_EF_EF_DRAWBILLBOARDSTRATEGY_H
#define MHTRI_EF_EF_DRAWBILLBOARDSTRATEGY_H

#include "ef/ef_drawstrategyimpl.h"

#ifdef __cplusplus
namespace nw4r {
namespace ef {

/* Draws each particle as a camera-facing quad, a stripe or a tube, by the draw setting's type option. */
class DrawBillboardStrategy : public DrawStrategyImpl {
public:
    DrawBillboardStrategy();
    virtual ~DrawBillboardStrategy() {}
    virtual void Draw(const EfDrawInfo& info, EfDrawParticleManager* pm);
    virtual CalcAheadFunc GetCalcAheadFunc(EfDrawParticleManager* pm);
}; /* size: 0xE0 */

/* Draws each particle as a quad oriented along its direction of travel. */
class DrawDirectionalStrategy : public DrawStrategyImpl {
public:
    DrawDirectionalStrategy();
    virtual ~DrawDirectionalStrategy() {}
    virtual void Draw(const EfDrawInfo& info, EfDrawParticleManager* pm);
    virtual CalcAheadFunc GetCalcAheadFunc(EfDrawParticleManager* pm);
}; /* size: 0xE0 */

}  // namespace ef
}  // namespace nw4r
#endif

#endif /* MHTRI_EF_EF_DRAWBILLBOARDSTRATEGY_H */
