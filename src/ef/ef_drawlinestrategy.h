/* ef/ef_drawlinestrategy.h - nw4r::ef `DrawLineStrategy`, the strategy class `ef/ef_drawlinestrategy.cpp` defines. */
#ifndef MHTRI_EF_EF_DRAWLINESTRATEGY_H
#define MHTRI_EF_EF_DRAWLINESTRATEGY_H

#include "ef/ef_drawstrategyimpl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800BF5B0 - the smaller of two floats, by address (the line and point widths clamp through it). */
f32* ef_min_float(f32* a, f32* b);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
namespace nw4r {
namespace ef {

/* Draws each particle as a line segment along its direction. */
class DrawLineStrategy : public DrawStrategyImpl {
public:
    DrawLineStrategy();
    virtual ~DrawLineStrategy() {}
    virtual void Draw(const EfDrawInfo& info, EfDrawParticleManager* pm);
}; /* size: 0xE0 */

}  // namespace ef
}  // namespace nw4r
#endif

#endif /* MHTRI_EF_EF_DRAWLINESTRATEGY_H */
