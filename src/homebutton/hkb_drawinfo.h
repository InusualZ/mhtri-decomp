/*
 * homebutton/hkb_drawinfo.h - the keyboard layout engine's draw information (nw4r::lyt::DrawInfo): the view matrix
 *   and rectangle, the location scale, the global alpha and the drawing flags a layout is drawn with.
 *   The layout is read from the constructor in homebutton/hkb_drawinfo.cpp.
 */
#ifndef HOMEBUTTON_HKB_DRAWINFO_H
#define HOMEBUTTON_HKB_DRAWINFO_H

#include "types.h"
#include "nw4r/math.h"
#include "nw4r/fn_805012C4.h"

namespace nw4r {
namespace lyt {

/* The state a layout is drawn with. size: 0x54 */
class DrawInfo {
public:
    DrawInfo();
    virtual ~DrawInfo();

    /* The drawing switches, one bit each. */
    struct Flag { /* size: 0x1 */
        u8 mulViewDraw : 1;               /* +0x0 */
        u8 receiveAlpha : 1;              /* +0x0 */
        u8 locationAdjust : 1;            /* +0x0 */
        u8 invisiblePaneCalculateMtx : 1; /* +0x0 */
    };

    /* +0x04 */ math::MTX34 mViewMtx;
    /* +0x34 */ ut::Rect mViewRect;
    /* +0x44 */ math::VEC2 mLocationAdjustScale;
    /* +0x4C */ f32 mGlobalAlpha;
    /* +0x50 */ Flag mFlag;
};

}  // namespace lyt
}  // namespace nw4r

#endif /* HOMEBUTTON_HKB_DRAWINFO_H */
