/*
 * nw4r/ut_Color.h - nw4r::ut::Color, the SDK's GXColor with nw4r's constructors; header-only, its weak destructor is
 *   emitted by every object that needs it.
 */
#ifndef NW4R_UT_COLOR_H
#define NW4R_UT_COLOR_H

#include "types.h"
#include "gx.h"

#ifdef __cplusplus
namespace nw4r {
namespace ut {

/* An RGBA colour (the SDK's GXColor with nw4r's constructors). size: 0x4 */
struct Color : public _GXColor {
    Color() {
        *reinterpret_cast<u32*>(this) = 0xFFFFFFFF;
    }

    Color(u32 rgba) {
        *reinterpret_cast<u32*>(this) = rgba;
    }

    Color(u8 red, u8 green, u8 blue, u8 alpha) {
        r = red;
        g = green;
        b = blue;
        a = alpha;
    }

    ~Color() {}

    u32 ToU32() const {
        return *reinterpret_cast<const u32*>(this);
    }
};

}  // namespace ut
}  // namespace nw4r
#endif

#endif
