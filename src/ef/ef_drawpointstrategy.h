/* ef/ef_drawpointstrategy.h - nw4r::ef `DrawPointStrategy`, the strategy class `ef/ef_drawpointstrategy.cpp`
 * defines. */
#ifndef MHTRI_EF_EF_DRAWPOINTSTRATEGY_H
#define MHTRI_EF_EF_DRAWPOINTSTRATEGY_H

#include "ef/ef_drawstrategyimpl.h"

#ifdef __cplusplus
namespace nw4r {
namespace ef {

/* Draws each particle as a GX point sized by its scale. */
class DrawPointStrategy : public DrawStrategyImpl {
public:
    DrawPointStrategy();
    virtual ~DrawPointStrategy() {}
    virtual void Draw(const EfDrawInfo& info, EfDrawParticleManager* pm);
}; /* size: 0xE0 */

}  // namespace ef
}  // namespace nw4r
#endif

#endif /* MHTRI_EF_EF_DRAWPOINTSTRATEGY_H */
