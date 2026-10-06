/*
 * nw4r/TextWriterBase.h - nw4r::ut's text writer, whose members `nw4r/fn_80504A3C.cpp` defines
 *   (TextWriterBase<T>, instantiated for char and wchar_t) and the print context its tag processor receives.
 */
#ifndef MHTRI_NW4R_TEXTWRITERBASE_H
#define MHTRI_NW4R_TEXTWRITERBASE_H

#include "types.h"
#include "nw4r/fn_80502828.h"

#ifdef __cplusplus
namespace nw4r {
namespace ut {

template <typename T> class TextWriterBase;

/* What a tag processor is told while a string prints. size: 0x14 */
template <typename T> struct PrintContext {
    /* +0x00 */ TextWriterBase<T>* writer;
    /* +0x04 */ const T* str;
    /* +0x08 */ f32 xOrigin;
    /* +0x0C */ f32 yOrigin;
    /* +0x10 */ u32 flags;
};

/* Lays out and draws strings with a CharWriter: wrapping width, character and line spacing, tabs and a tag
 * processor for control characters. size: 0x64 */
template <typename T> class TextWriterBase : public CharWriter {
public:
    TextWriterBase();
    ~TextWriterBase();

    f32 GetLineHeight() const;
    f32 PrintMutable(const T* str, int length);
    bool CalcLineRectImpl(Rect* rect, const T** str, int length);
    void CalcStringRectImpl(Rect* rect, const T* str, int length);
    f32 PrintImpl(const T* str, int length, bool moveCursor);
    f32 AdjustCursor(f32* xOrigin, f32* yOrigin, const T* str, int length);

    f32 CalcLineWidth(const T* str, int length) {
        Rect rect;
        TextWriterBase<T> myCopy(*this);

        myCopy.SetCursor(0.0f, 0.0f);
        myCopy.CalcLineRectImpl(&rect, &str, length);
        return rect.GetWidth();
    }
    void CalcStringRect(Rect* rect, const T* str, int length) const {
        TextWriterBase<T> myCopy(*this);

        myCopy.CalcStringRectImpl(rect, str, length);
    }
    f32 GetCharSpace() const {
        return mCharSpace;
    }
    bool IsDrawFlagSet(u32 mask, u32 flag) const {
        return (mDrawFlag & mask) == flag;
    }

    int GetTabWidth() const {
        return mTabWidth;
    }
    bool IsWidthFixed() const {
        return mIsWidthFixed;
    }
    f32 GetFixedWidth() const {
        return mFixedWidth;
    }
    f32 GetCursorX() const {
        return mCursorPos.x;
    }
    f32 GetCursorY() const {
        return mCursorPos.y;
    }
    void SetCursorX(f32 x) {
        mCursorPos.x = x;
    }
    void SetCursorY(f32 y) {
        mCursorPos.y = y;
    }

    /* 0x80795770 / 0x80795774 (static, .sbss) */ static TagProcessorBase<T> mDefaultTagProcessor;

    /* +0x4C */ f32 mWidthLimit;
    /* +0x50 */ f32 mCharSpace;
    /* +0x54 */ f32 mLineSpace;
    /* +0x58 */ int mTabWidth;
    /* +0x5C */ u32 mDrawFlag;
    /* +0x60 */ TagProcessorBase<T>* mTagProcessor;
};

}  // namespace ut
}  // namespace nw4r
#endif

#endif
