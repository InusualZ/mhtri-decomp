/*
 * nw4r/ut_CharWriter.h - nw4r::ut::CharWriter, the glyph writer `nw4r/ut_CharWriter.cpp` defines.
 */
#ifndef NW4R_UT_CHARWRITER_H
#define NW4R_UT_CHARWRITER_H

#include "types.h"
#include "nw4r/math.h"
#include "nw4r/ut_Color.h"
#include "nw4r/ut_Font.h"

#ifdef __cplusplus
namespace nw4r {
namespace ut {

/* Draws glyphs of a Font through GX: colours, scale, cursor and the texture filter. size: 0x4C */
class CharWriter {
public:
    /* The two colours the glyph intensity is mapped between. */
    struct ColorMapping { /* size: 0x8 */
        /* +0x0 */ Color min;
        /* +0x4 */ Color max;
    };

    /* The colours of a glyph quad's four corners. */
    struct VertexColor { /* size: 0x10 */
        /* +0x0 */ Color lu;
        /* +0x4 */ Color ru;
        /* +0x8 */ Color ld;
        /* +0xC */ Color rd;
    };

    /* How the text colour runs from `start` to `end` across a glyph. */
    enum GradationMode {
        GRADMODE_NONE = 0,
        GRADMODE_H = 1,
        GRADMODE_V = 2
    };

    /* The text colour and its gradation. */
    struct TextColor { /* size: 0xC */
        /* +0x0 */ Color start;
        /* +0x4 */ Color end;
        /* +0x8 */ GradationMode gradationMode;
    };

    /* The GX minification/magnification filters. */
    struct TextureFilter { /* size: 0x8 */
        /* +0x0 */ s32 atSmall;
        /* +0x4 */ s32 atLarge;
    };

    /* The texture last loaded into a GX texture slot. */
    struct LoadingTexture { /* size: 0x10 */
        void Reset() {
            slot = 0xFF;
            texture = NULL;
        }

        bool operator!=(const LoadingTexture& rhs) const {
            return slot != rhs.slot || texture != rhs.texture || filter.atSmall != rhs.filter.atSmall ||
                   filter.atLarge != rhs.filter.atLarge;
        }

        /* +0x0 */ s32 slot;
        /* untyped: the texture sheet, a byte image */
        /* +0x4 */ const void* texture;
        /* +0x8 */ TextureFilter filter;
    };

    CharWriter();
    ~CharWriter();

    void SetColorMapping(Color min, Color max);
    void ResetColorMapping() {
        SetColorMapping(Color(0x00000000), Color(0xFFFFFFFF));
    }
    void SetGradationMode(GradationMode mode) {
        mTextColor.gradationMode = mode;
        UpdateVertexColor();
    }
    void SetTextColor(Color color);
    void SetScale(f32 hScale, f32 vScale) {
        mScale.x = hScale;
        mScale.y = vScale;
    }
    void SetCursor(f32 x, f32 y, f32 z) {
        mCursorPos.x = x;
        mCursorPos.y = y;
        mCursorPos.z = z;
    }
    void EnableLinearFilter(bool atSmall, bool atLarge) {
        mFilter.atSmall = atSmall ? 1 : 0;
        mFilter.atLarge = atLarge ? 1 : 0;
    }

    void SetupGX();
    void SetFontSize(f32 width, f32 height);
    const Font* GetFont() const {
        return mFont;
    }
    f32 GetScaleH() const {
        return mScale.x;
    }
    f32 GetScaleV() const {
        return mScale.y;
    }
    void SetCursor(f32 x, f32 y) {
        mCursorPos.x = x;
        mCursorPos.y = y;
    }
    void MoveCursorX(f32 dx) {
        mCursorPos.x += dx;
    }
    void MoveCursorY(f32 dy) {
        mCursorPos.y += dy;
    }

    f32 GetFontWidth() const;
    f32 GetFontHeight() const;
    f32 GetFontAscent() const;
    f32 Print(u16 code);
    void PrintGlyph(f32 x, f32 y, f32 z, const Glyph& glyph);
    void LoadTexture(const Glyph& glyph, s32 slot);
    void UpdateVertexColor();

    /* 0x80760CA8 (static, .bss) */ static LoadingTexture mLoadingTexture;

    /* +0x00 */ ColorMapping mColorMapping;
    /* +0x08 */ VertexColor mVertexColor;
    /* +0x18 */ TextColor mTextColor;
    /* +0x24 */ math::VEC2 mScale;
    /* +0x2C */ math::VEC3 mCursorPos;
    /* +0x38 */ TextureFilter mFilter;
    /* +0x40 */ u8 pad_0x40[0x2];
    /* +0x42 */ u8 mAlpha;
    /* +0x43 */ bool mIsWidthFixed;
    /* +0x44 */ f32 mFixedWidth;
    /* +0x48 */ const Font* mFont;
};

}  // namespace ut
}  // namespace nw4r
#endif

#endif
