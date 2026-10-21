/* ef/ef_drawbillboardstrategy.h - nw4r::ef `DrawBillboardStrategy` and `DrawDirectionalStrategy`, the two strategy
 * classes `ef/ef_drawbillboardstrategy.cpp` and `ef/ef_drawdirectionalstrategy.cpp` define, and the walker
 * records the two share. */
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

#ifdef __cplusplus
extern "C" {
#endif

/* The ahead-context argument block the walkers read: the particle handle and two cached positions. */
typedef struct EfAheadArgs {
    void* particle;    /* +0x00 */
    u8 pad_0x04[0x94]; /* +0x04 */
    VEC3 prev_pos;     /* +0x98  the previous resolved position */
    Vec pos;           /* +0xA4  the fallback position */
} EfAheadArgs; /* size: 0xB0 (lower bound, the record continues past what this unit reads) */

/* The particle/emitter record each walker advances: its world position sits at +0xAC. */
typedef struct EfWalkerObj {
    u8 pad_0x00[0xAC]; /* +0x00 */
    Vec world_pos;     /* +0xAC */
} EfWalkerObj; /* size: 0xB8 (lower bound, the record continues past what this unit reads) */

/* The billboard TU's ahead-vector walkers; the directional builder returns them too. */
void ef_billboard_ahead_younger(Vec* out, EfAheadArgs* args, EfWalkerObj* em);
void ef_billboard_ahead_neighbours(Vec* out, EfAheadArgs* args, EfWalkerObj* em);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EF_DRAWBILLBOARDSTRATEGY_H */
