/*
 * homebutton/hkb_textbox.h - the keyboard layout engine's text-box pane (nw4r::lyt::TextBox), which
 *   homebutton/hkb_textbox.cpp defines: a wide string drawn with a font through nw4r::ut's text writer.
 */
#ifndef HOMEBUTTON_HKB_TEXTBOX_H
#define HOMEBUTTON_HKB_TEXTBOX_H

#include "homebutton/hkb_layout.h"
#include "nw4r/TextWriterBase.h"

namespace nw4r {
namespace lyt {

namespace res {

/* One entry of the `fnl1` block: the font name's offset. size: 0x8 */
struct Font {
    /* +0x0 */ u32 nameStrOffset;
    /* +0x4 */ u8 type;
    /* +0x5 */ u8 pad_0x5[3];
};

/* The `txt1` block: a pane, the string buffer sizes, the font and material, the text colours and spacing.
 * size: 0x74 */
struct TextBox : public Pane {
    /* +0x4C */ u16 textBufBytes;
    /* +0x4E */ u16 textStrBytes;
    /* +0x50 */ u16 materialIdx;
    /* +0x52 */ u16 fontIdx;
    /* +0x54 */ u8 textPosition;
    /* +0x55 */ u8 textAlignment;
    /* +0x56 */ u8 pad_0x56[2];
    /* +0x58 */ u32 textStrOffset;
    /* +0x5C */ u32 textCols[2];
    /* +0x64 */ Size fontSize;
    /* +0x6C */ f32 charSpace;
    /* +0x70 */ f32 lineSpace;
};

}  // namespace res

/* A pane that draws a wide string with a font, its text colours and its placement. size: 0x104 */
class TextBox : public Pane {
public:
    /* The flag byte: whether the box owns its font, and the line alignment. size: 0x1 */
    struct Bits {
        u8 bAllocFont : 1;    /* +0x0 */
        u8 textAlignment : 2; /* +0x0 */
    };

    TextBox(const res::TextBox* pBlock, const ResBlockSet& resBlockSet);
    virtual ~TextBox();
    virtual const ut::detail::RuntimeTypeInfo* GetRuntimeTypeInfo() const;
    virtual void DrawSelf(const DrawInfo& drawInfo);
    virtual ut::Color GetVtxColor(u32 idx) const;
    virtual void SetVtxColor(u32 idx, ut::Color value);
    virtual u8 GetVtxColorElement(u32 idx) const;
    virtual void SetVtxColorElement(u32 idx, u8 value);
    virtual void LoadMtx(const DrawInfo& drawInfo);
    virtual void AllocStringBuffer(u16 minLen);
    virtual void FreeStringBuffer();
    virtual u16 SetString(const wchar_t* str, u16 dstIdx);
    virtual u16 SetString(const wchar_t* str, u16 dstIdx, u16 strLen);

    const ut::Font* GetFont() const;

    /* Sets the box's font, releasing an owned one, and takes the font's cell size as the font size. */
    void SetFont(const ut::Font* pFont) {
        if (mBits.bAllocFont) {
            Layout::DeleteObj(const_cast<ut::Font*>(mpFont));
            mBits.bAllocFont = false;
        }
        mpFont = pFont;
        if (mpFont) {
            SetFontSize(Size(mpFont->GetWidth(), mpFont->GetHeight()));
        } else {
            SetFontSize(Size(0.0f, 0.0f));
        }
    }
    /* Sets the size the font is drawn at. */
    void SetFontSize(const Size& fontSize) { mFontSize = fontSize; }

    /* Returns how many characters the buffer holds besides the terminator. */
    u16 GetStringBufferLength() const {
        if (mTextBufBytes == 0) {
            return 0;
        }
        return (u16)(mTextBufBytes / sizeof(wchar_t) - 1);
    }
    const ut::Rect GetTextDrawRect(ut::TextWriterBase<wchar_t>* pWriter) const;

    /* Returns text colour `type` (0 = top, 1 = bottom). */
    const ut::Color GetTextColor(u32 type) const { return mTextColors[type]; }
    /* Sets text colour `type` (0 = top, 1 = bottom). */
    void SetTextColor(u32 type, ut::Color value) { mTextColors[type] = value; }

    /* Returns the text's horizontal placement (0 left, 1 centre, 2 right). */
    u8 GetTextPositionH() const { return (u8)(mTextPosition % 3); }
    /* Returns the text's vertical placement (0 top, 1 centre, 2 bottom). */
    u8 GetTextPositionV() const { return (u8)(mTextPosition / 3); }
    /* Sets the text's horizontal placement. */
    void SetTextPositionH(u8 pos) { mTextPosition = (u8)(GetTextPositionV() * 3 + pos); }
    /* Sets the text's vertical placement. */
    void SetTextPositionV(u8 pos) { mTextPosition = (u8)(pos * 3 + GetTextPositionH()); }

    /* Returns how far the text sits from the left of its box, as a fraction of the free width. */
    f32 GetTextMagH() const {
        f32 hMag = 0.0f;
        switch (GetTextPositionH()) {
        default:
        case 0:
            hMag = 0.0f;
            break;
        case 1:
            hMag = 0.5f;
            break;
        case 2:
            hMag = 1.0f;
            break;
        }
        return hMag;
    }

    /* Returns how far the text sits from the top of its box, as a fraction of the free height. */
    f32 GetTextMagV() const {
        f32 vMag = 0.0f;
        switch (GetTextPositionV()) {
        default:
        case 0:
            vMag = 0.0f;
            break;
        case 1:
            vMag = 0.5f;
            break;
        case 2:
            vMag = 1.0f;
            break;
        }
        return vMag;
    }

    /* The class's run-time type record (0x80795A30); its parent is Pane's. */
    static const ut::detail::RuntimeTypeInfo typeInfo;

    /* +0xD8 */ wchar_t* mTextBuf;
    /* +0xDC */ ut::Color mTextColors[2];
    /* +0xE4 */ const ut::Font* mpFont;
    /* +0xE8 */ Size mFontSize;
    /* +0xF0 */ f32 mLineSpace;
    /* +0xF4 */ f32 mCharSpace;
    /* +0xF8 */ ut::TagProcessorBase<wchar_t>* mpTagProcessor;
    /* +0xFC */ u16 mTextBufBytes;
    /* +0xFE */ u16 mTextLen;
    /* +0x100 */ u8 mTextPosition;
    /* +0x101 */ Bits mBits;
    /* +0x102 */ u8 pad_0x102[2];
};

}  // namespace lyt
}  // namespace nw4r

#endif
