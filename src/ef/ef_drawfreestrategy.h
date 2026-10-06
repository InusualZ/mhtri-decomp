/* ef/ef_drawfreestrategy.h - nw4r::ef `DrawFreeStrategy`, the strategy class `ef/ef_drawfreestrategy.cpp` defines. */
#ifndef MHTRI_EF_EF_DRAWFREESTRATEGY_H
#define MHTRI_EF_EF_DRAWFREESTRATEGY_H

#include "ef/ef_drawstrategyimpl.h"

#ifdef __cplusplus
namespace nw4r {
namespace ef {

/* Draws each particle as a quad (and optionally a second, crossing quad) under its own rotation. */
class DrawFreeStrategy : public DrawStrategyImpl {
public:
    DrawFreeStrategy();
    virtual ~DrawFreeStrategy() {}
    virtual void Draw(const EfDrawInfo& info, EfDrawParticleManager* pm);
}; /* size: 0xE0 */

}  // namespace ef
}  // namespace nw4r
#endif

#endif /* MHTRI_EF_EF_DRAWFREESTRATEGY_H */
