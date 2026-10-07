/*
 * ef/ef_draworder.h - the draw-order list classes: the abstract `DrawOrderBase` (its table and its implicit
 *   constructor come out of `ef/ef_effectsystem.cpp`, which builds the instance) and `DrawOrder`, whose three
 *   virtuals are `ef/ef_draworder.cpp`'s (`fn_800A3B20`, `fn_800A3D7C`, `fn_800A39A4`; its table is emitted there).
 */
#ifndef MHTRI_EF_EF_DRAWORDER_H
#define MHTRI_EF_EF_DRAWORDER_H

#include "types.h"

struct ParticleManager;
struct EfDrawInfo;

namespace nw4r {
namespace ef {

struct Effect;

/* The draw-order interface: puts a particle manager on the effect's draw list, takes it off, and draws the list. */
class DrawOrderBase {
public:
    /* +0x00: the vtable pointer */
    virtual void Add(Effect* effect, ParticleManager* pm) = 0;
    virtual void Remove(Effect* effect, ParticleManager* pm) = 0;
    virtual void Draw(Effect* effect, const EfDrawInfo* info) = 0;
}; /* size: 0x4 */

/* The ordered draw list (by each manager's draw order). */
class DrawOrder : public DrawOrderBase {
public:
    virtual void Add(Effect* effect, ParticleManager* pm);
    virtual void Remove(Effect* effect, ParticleManager* pm);
    virtual void Draw(Effect* effect, const EfDrawInfo* info);
}; /* size: 0x4 */

} // namespace ef
} // namespace nw4r

#endif /* MHTRI_EF_EF_DRAWORDER_H */
