/*
 * nw4r/ut_Color.h - nw4r::ut::Color, the SDK's GXColor with nw4r's constructors; header-only, its weak destructor is
 *   emitted by every object that needs it.  The members keep nw4r's call structure (the word constructors assign
 *   through operator=(u32), which writes through ToU32ref; the four-channel constructor calls Set): a unit that calls
 *   them out of line (`#pragma dont_inline on`) gets retail's weak copies (g3d/g3d_anmclr.cpp, g3d/g3d_resanm.cpp).
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
        *this = 0xFFFFFFFF;
    }

    Color(u32 rgba) {
        *this = rgba;
    }

    Color(int red, int green, int blue, int alpha) {
        Set(red, green, blue, alpha);
    }

    ~Color() {}

    /* Sets the four channels. */
    void Set(int red, int green, int blue, int alpha) {
        r = red;
        g = green;
        b = blue;
        a = alpha;
    }

    /* Sets the four channels from one RGBA word. */
    Color& operator=(u32 rgba) {
        ToU32ref() = rgba;
        return *this;
    }

    /* Copies the four channels of an SDK colour. */
    Color& operator=(const GXColor& color) {
        return *this = *reinterpret_cast<const u32*>(&color);
    }

    /* The colour with `rgba`'s bits set. */
    Color operator|(u32 rgba) const {
        return Color(ToU32() | rgba);
    }

    /* The colour masked by `rgba`. */
    Color operator&(u32 rgba) const {
        return Color(ToU32() & rgba);
    }

    u32& ToU32ref() {
        return *reinterpret_cast<u32*>(this);
    }

    const u32& ToU32ref() const {
        return *reinterpret_cast<const u32*>(this);
    }

    u32 ToU32() const {
        return ToU32ref();
    }

    operator u32() const {
        return ToU32ref();
    }
};

}  // namespace ut
}  // namespace nw4r
#endif

#endif
