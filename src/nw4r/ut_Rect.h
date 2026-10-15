/*
 * nw4r/ut_Rect.h - nw4r::ut::Rect, the axis-aligned rectangle the tag processor and the text writer fill in (all of
 *   its members are inline; NW4R ships it as a header only).
 */
#ifndef NW4R_UT_RECT_H
#define NW4R_UT_RECT_H

#include "types.h"

#ifdef __cplusplus
namespace nw4r {
namespace ut {

/* An axis-aligned rectangle. size: 0x10 */
struct Rect {
    Rect() : left(0.0f), top(0.0f), right(0.0f), bottom(0.0f) {}
    Rect(f32 l, f32 t, f32 r, f32 b) : left(l), top(t), right(r), bottom(b) {}

    f32 GetWidth() const {
        return right - left;
    }

    /* Orders the edges so left <= right and top <= bottom. */
    void Normalize() {
        f32 l = left;
        f32 t = top;
        f32 r = right;
        f32 b = bottom;

        left = FSelect(r - l, l, r);
        right = FSelect(r - l, r, l);
        top = FSelect(b - t, t, b);
        bottom = FSelect(b - t, b, t);
    }

    /* `ifPos` when `cond` >= 0, else `ifNeg`, through `fsel`. */
    static f32 FSelect(register f32 cond, register f32 ifPos, register f32 ifNeg) {
        register f32 ret;
        asm { fsel ret, cond, ifPos, ifNeg }
        return ret;
    }

    /* +0x0 */ f32 left;
    /* +0x4 */ f32 top;
    /* +0x8 */ f32 right;
    /* +0xC */ f32 bottom;
};

}  // namespace ut
}  // namespace nw4r
#endif

#endif
