/* homebutton/hkb_textbox.cpp - the software keyboard layout engine's text-box pane (nw4r::lyt::TextBox): a wide
 *   string drawn through nw4r::ut's text writer with the box's font, colours, spacing and placement, line by line
 *   inside the box width.
 * RANGE. .text 0x8052C880-0x8052E0CC (22 functions); .ctors 0x8056F3E8-0x8056F3EC; .data 0x8064A6B0-0x8064A738;
 *   .sdata 0x807944B0-0x807944B8; .sbss 0x80795A30-0x80795A38; .sdata2 0x8079D580-0x8079D598.
 *   Edges: the left edge follows homebutton/hkb_layout.cpp's static initialiser 0x8052C874; the right edge is this
 *   unit's static initialiser 0x8052E0C0 (.ctors word 0x8056F3E8).
 * FLAGS. GC/3.0a5.2 + cflags_hkb, the band's flags (evidence: src/homebutton/hbm_kb_event.cpp FLAGS).
 * NAMES. GUESS: nw4r::lyt::TextBox and its members, from the runtime dump (GetColor in the anonymous namespace of
 *   lyt_textBox.cpp at 0x8052C880, nw4hbm::lyt::TextBox::SetVtxColor at 0x8052CDF4; the HOME Button copy of
 *   NintendoWare's layout library, spelt nw4r::lyt in this band after homebutton/hkb_material.cpp).
 *   CalcLineRectImpl / CalcStringRectImpl are GUESS names for the file's line-measuring helpers; SetWriterTextColor
 *   is the GUESS name of the out-of-line copy of CharWriter::SetTextColor(Color, Color) (0x8052D400).
 * SHAPES. DrawSelf takes the text width through ut::Rect::GetWidth (the inline call keeps retail's second
 *   textRect.left load).
 * SHAPES. the constructor reaches the material through CONVERT_OFFS_TO_PTR (homebutton/hkb_material.h): the inline
 *   ConvertOffsToPtr copied the material list into a second register (`mr r5,r8`).
 * SHAPES. GetTextDrawRect reads the line and character spacing into locals before the writer stores, and copies
 *   GetVtxPos's result out of a reference-bound temporary (retail's slot order: temporary above the copy).
 * SHAPES. CalcLineRectImpl / CalcStringRectImpl are plain statics: retail emits them after their first user
 *   (DrawSelf, GetTextDrawRect), so they are defined there. SetFont(NULL) in the destructor keeps retail's dead
 *   GetWidth/GetHeight code.
 * RESIDUALS. AllocStringBuffer: retail recomputes `minLen + 1` for NewArray's loop count after the allocation; ours keeps
 *   it in r31 (an unsigned `minLen + 1U` argument recovers the recomputed size; an int, u16 or u32 local does not).
 * RESIDUALS. DrawSelf: float register colouring (retail keeps hMag in f30 and a later 0.0 in f31; ours swaps them).
 * RESIDUALS. CharWriter::SetTextColor(Color, Color) is a file static here (src/nw4r/fn_80502828.h has no such
 *   member: request hkb-b#4); weak copies the target lacks (Color, Font, CharWriter record dtors).
 */
#include "homebutton/hkb_textbox.h"

#include "MSL_C/strstr.h"
#include "MTX/PSMTXCopy.h"
#include "RVLGX/GXTev.h"
#include "RVLGX/GXTransform.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"

namespace nw4r {
namespace lyt {

namespace {

/* Returns a TEV colour clamped to 0..255 per channel. */
ut::Color GetColor(const GXColorS10& src) {
    GXColor dst;
    dst.r = (u8)(src.r < 0 ? 0 : src.r > 255 ? 255 : src.r);
    dst.g = (u8)(src.g < 0 ? 0 : src.g > 255 ? 255 : src.g);
    dst.b = (u8)(src.b < 0 ? 0 : src.b > 255 ? 255 : src.b);
    dst.a = (u8)(src.a < 0 ? 0 : src.a > 255 ? 255 : src.a);
    return ut::Color(*reinterpret_cast<u32*>(&dst));
}

/* Sets the writer's top and bottom text colours and rebuilds its vertex colours. */
inline void SetWriterTextColor(ut::TextWriterBase<wchar_t>* pWriter, ut::Color start, ut::Color end) {
    pWriter->mTextColor.start = start;
    pWriter->mTextColor.end = end;
    pWriter->UpdateVertexColor();
}

int CalcLineRectImpl(ut::Rect* pRect, ut::TextWriterBase<wchar_t>* pTextWriter, const wchar_t* str, int length,
                     f32 maxWidth, bool* pbOver);
void CalcStringRectImpl(ut::Rect* pRect, ut::TextWriterBase<wchar_t>* pTextWriter, const wchar_t* str,
                        int length, f32 maxWidth);

/* Measures a line on a copy of the writer, so the writer's cursor is left alone; returns its character count. */
inline int CalcLineStrNum(f32* pWidth, ut::TextWriterBase<wchar_t>* pTextWriter, const wchar_t* str, int length,
                          f32 maxWidth, bool* pbOver) {
    ut::Rect rect;
    ut::TextWriterBase<wchar_t> myCopy(*pTextWriter);

    myCopy.SetCursor(0.0f, 0.0f);
    int ret = CalcLineRectImpl(&rect, &myCopy, str, length, maxWidth, pbOver);
    *pWidth = rect.GetWidth();
    return ret;
}

/* Measures the string on a copy of the writer. */
inline void CalcStringRect(ut::Rect* pRect, const ut::TextWriterBase<wchar_t>& writer, const wchar_t* str,
                           int length, f32 maxWidth) {
    ut::TextWriterBase<wchar_t> myCopy(writer);

    CalcStringRectImpl(pRect, &myCopy, str, length, maxWidth);
}

}  // namespace

const ut::detail::RuntimeTypeInfo TextBox::typeInfo(&Pane::typeInfo);

/* 0x8052C934 (0x2EC): Builds the text box from its `txt1` block: the string buffer and string, the colours, the
 * font (looked up, or loaded from a `font` resource) and the material. */
TextBox::TextBox(const res::TextBox* pBlock, const ResBlockSet& resBlockSet) : Pane(pBlock), mFontSize(0.0f, 0.0f) {
    u16 allocStrLen = (u16)(pBlock->textBufBytes / sizeof(wchar_t));
    if (allocStrLen > 0) {
        allocStrLen -= 1;
    }

    mTextBuf = NULL;
    mTextBufBytes = 0;
    mTextLen = 0;
    mpFont = NULL;
    mFontSize = Size(0.0f, 0.0f);
    SetTextPositionH(1);
    SetTextPositionV(1);
    mLineSpace = 0.0f;
    mCharSpace = 0.0f;
    mpTagProcessor = NULL;
    memset(&mBits, 0, sizeof(mBits));

    if (allocStrLen > 0) {
        AllocStringBuffer(allocStrLen);
    }

    if (pBlock->textStrBytes >= sizeof(wchar_t) && mTextBuf) {
        const wchar_t* pBlockText = detail::ConvertOffsToPtr<wchar_t>(pBlock, pBlock->textStrOffset);
        u16 resStrLen = (u16)(pBlock->textStrBytes / sizeof(wchar_t) - 1);
        SetString(pBlockText, 0, resStrLen);
    }

    for (int i = 0; i < 2; ++i) {
        *reinterpret_cast<u32*>(&mTextColors[i]) = pBlock->textCols[i];
    }
    mFontSize = pBlock->fontSize;
    mTextPosition = pBlock->textPosition;
    mBits.textAlignment = pBlock->textAlignment;
    mCharSpace = pBlock->charSpace;
    mLineSpace = pBlock->lineSpace;

    const res::Font* fonts = detail::ConvertOffsToPtr<res::Font>(resBlockSet.pFontList, 0xC);
    const char* fontName = detail::ConvertOffsToPtr<char>(fonts, fonts[pBlock->fontIdx].nameStrOffset);

    if (ut::Font* pFont = resBlockSet.pResAccessor->GetFont(fontName)) {
        mpFont = pFont;
    } else if (void* fontRes = resBlockSet.pResAccessor->GetResource('font', fontName, NULL)) {
        if (ut::ResFont* pResFont = Layout::NewObj<ut::ResFont>()) {
            pResFont->SetResource(fontRes);
            mpFont = pResFont;
            mBits.bAllocFont = true;
        }
    }

    const u32* matOffsTbl = CONVERT_OFFS_TO_PTR(u32, resBlockSet.pMaterialList, 0xC);
    const res::Material* pResMaterial =
        CONVERT_OFFS_TO_PTR(res::Material, resBlockSet.pMaterialList, matOffsTbl[pBlock->materialIdx]);
    mpMaterial = Layout::NewObj<Material>(pResMaterial, resBlockSet);
}

/* 0x8052CC20 (0x1A0): Releases an owned font, the material and the string buffer. */
TextBox::~TextBox() {
    SetFont(NULL);

    if (mpMaterial && !mpMaterial->mbUserAllocated) {
        Layout::DeleteObj(mpMaterial);
        mpMaterial = NULL;
    }

    FreeStringBuffer();
}

/* 0x8052CDC0 (0x8): Returns the colour of corner `idx`: the top text colour for the top corners, else the bottom's. */
ut::Color TextBox::GetVtxColor(u32 idx) const {
    return GetTextColor(idx / 2);
}

/* 0x8052CDF4 (0x48): Sets the text colour of the corners' row. */
void TextBox::SetVtxColor(u32 idx, ut::Color value) {
    SetTextColor(idx / 2, value);
}

/* 0x8052CE68 (0x18): Returns one channel of a corner's text colour. */
u8 TextBox::GetVtxColorElement(u32 idx) const {
    return reinterpret_cast<const u8*>(&mTextColors[idx / 4 / 2])[idx % 4];
}

/* 0x8052CE80 (0x18): Sets one channel of a corner's text colour. */
void TextBox::SetVtxColorElement(u32 idx, u8 value) {
    reinterpret_cast<u8*>(&mTextColors[idx / 4 / 2])[idx % 4] = value;
}

/* 0x8052CE98 (0x568): Draws the string line by line: each line measured inside the box width and placed by the
 * box's alignment, in the text colours modulated by the pane alpha and the material's colour mapping. */
void TextBox::DrawSelf(const DrawInfo& drawInfo) {
    if (!mTextBuf || !mpFont || !mpMaterial) {
        return;
    }

    LoadMtx(drawInfo);

    ut::TextWriterBase<wchar_t> writer;
    ut::Rect textRect = GetTextDrawRect(&writer);

    ut::Color topCol = detail::MultipleAlpha(mTextColors[0], mGlbAlpha);
    ut::Color btmCol = detail::MultipleAlpha(mTextColors[1], mGlbAlpha);
    writer.SetGradationMode(topCol.ToU32() != btmCol.ToU32() ? ut::CharWriter::GRADMODE_V
                                                            : ut::CharWriter::GRADMODE_NONE);
    SetWriterTextColor(&writer, topCol, btmCol);

    GXColorS10 tevCol0 = mpMaterial->mTevCols[0];
    ut::Color minCol = GetColor(tevCol0);
    GXColorS10 tevCol1 = mpMaterial->mTevCols[1];
    ut::Color maxCol = GetColor(tevCol1);
    writer.SetColorMapping(minCol, maxCol);

    writer.SetupGX();
    GXSetAlphaCompare(7, 0, 0, 7, 0);

    f32 hMag;
    switch (mBits.textAlignment) {
    default:
    case 0:
        hMag = GetTextMagH();
        break;
    case 1:
        hMag = 0.0f;
        break;
    case 2:
        hMag = 0.5f;
        break;
    case 3:
        hMag = 1.0f;
        break;
    }

    const wchar_t* strPos = mTextBuf;
    writer.SetCursor(textRect.left, -textRect.top);
    const f32 textWidth = textRect.GetWidth();
    int remain = mTextLen;

    while (remain > 0) {
        bool bOver;
        f32 lineWidth;
        int lineStrNum = CalcLineStrNum(&lineWidth, &writer, strPos, remain, mSize.width, &bOver);

        writer.SetCursorX(textRect.left + (textWidth - lineWidth) * hMag);
        writer.PrintMutable(strPos, lineStrNum);
        if (bOver) {
            writer.PrintMutable(L"\n", 1);
        }

        strPos += lineStrNum;
        remain -= lineStrNum;
    }
}

namespace {

/* 0x8052D444 (0x3E0): Measures the line starting at `str` that fits in `maxWidth`; returns its character count
 * and sets `*pbOver` when the line was cut at the width. */
int CalcLineRectImpl(ut::Rect* pRect, ut::TextWriterBase<wchar_t>* pTextWriter, const wchar_t* str,
                     int length, f32 maxWidth, bool* pbOver) {
    ut::PrintContext<wchar_t> context = {pTextWriter, str, 0.0f, 0.0f, 0};
    const ut::Font* font = pTextWriter->GetFont();
    f32 x = 0.0f;
    bool bCharSpace = false;
    ut::CharStrmReader reader = font->GetCharStrmReader();
    const void* prStrPos = reader.GetCurrentPos(); /* untyped: the reader's position */

    pRect->left = 0.0f;
    pRect->right = 0.0f;
    pRect->top = ut::Min(0.0f, pTextWriter->GetLineHeight());
    pRect->bottom = ut::Max(0.0f, pTextWriter->GetLineHeight());
    *pbOver = false;

    reader.Set(str);
    ut::Rect prMaxRect = *pRect;
    u16 code = reader.Next();

    while (static_cast<const wchar_t*>(reader.GetCurrentPos()) - str <= length) {
        if (code < ' ') {
            ut::Rect rect(x, 0.0f, 0.0f, 0.0f);

            context.str = static_cast<const wchar_t*>(reader.GetCurrentPos());
            context.flags = !bCharSpace;
            pTextWriter->SetCursorX(x);

            ut::Operation operation = pTextWriter->mTagProcessor->CalcRect(&rect, code, &context);
            reader.Set(context.str);

            pRect->left = ut::Min(pRect->left, rect.left);
            pRect->top = ut::Min(pRect->top, rect.top);
            pRect->right = ut::Max(pRect->right, rect.right);
            pRect->bottom = ut::Max(pRect->bottom, rect.bottom);
            x = pTextWriter->GetCursorX();

            if (pRect->GetWidth() > maxWidth) {
                *pbOver = true;
                break;
            }

            if (operation == ut::OPERATION_END_DRAW) {
                return length;
            } else if (operation == ut::OPERATION_NO_CHAR_SPACE) {
                bCharSpace = false;
            } else if (operation == ut::OPERATION_CHAR_SPACE) {
                bCharSpace = true;
            } else if (operation == ut::OPERATION_NEXT_LINE) {
                break;
            }
        } else {
            if (bCharSpace) {
                x += pTextWriter->GetCharSpace();
            }
            bCharSpace = true;

            if (pTextWriter->IsWidthFixed()) {
                x += pTextWriter->GetFixedWidth();
            } else {
                x += pTextWriter->GetFont()->GetCharWidth(code) * pTextWriter->GetScaleH();
            }

            pRect->left = ut::Min(pRect->left, x);
            pRect->right = ut::Max(pRect->right, x);

            if (pRect->GetWidth() > maxWidth) {
                *pbOver = true;
                break;
            }
        }

        prStrPos = reader.GetCurrentPos();
        code = reader.Next();
        prMaxRect = *pRect;
    }

    if (*pbOver && prStrPos) {
        *pRect = prMaxRect;
        return static_cast<const wchar_t*>(prStrPos) - str;
    }
    return static_cast<const wchar_t*>(reader.GetCurrentPos()) - str;
}

}  // namespace

/* 0x8052D824 (0xC4): Makes room for `minLen` characters plus the terminator, replacing a smaller buffer. */
void TextBox::AllocStringBuffer(u16 minLen) {
    if (minLen == 0) {
        return;
    }

    const u32 textBufBytes = (minLen + 1) * sizeof(wchar_t);
    if (textBufBytes <= mTextBufBytes) {
        return;
    }

    FreeStringBuffer();
    mTextBuf = Layout::NewArray<wchar_t>(minLen + 1U);
    if (mTextBuf) {
        mTextBufBytes = (u16)textBufBytes;
    }
}

/* 0x8052D8E8 (0x4C): Releases the string buffer. */
void TextBox::FreeStringBuffer() {
    if (mTextBuf) {
        Layout::DeleteArray(mTextBuf, mTextBufBytes / sizeof(wchar_t));
        mTextBuf = NULL;
        mTextBufBytes = 0;
    }
}

/* 0x8052D934 (0xDC): Copies a terminated string into the buffer at `dstIdx`; returns the characters copied. */
u16 TextBox::SetString(const wchar_t* str, u16 dstIdx) {
    const u32 strLen = wcslen(reinterpret_cast<const u16*>(str));

    if (!mTextBuf) {
        return 0;
    }

    const u16 bufLen = GetStringBufferLength();
    if (dstIdx >= bufLen) {
        return 0;
    }

    const u32 cpLen = ut::Min<u32>(strLen, bufLen - dstIdx);

    memcpy(mTextBuf + dstIdx, str, cpLen * sizeof(wchar_t));
    mTextLen = (u16)(dstIdx + cpLen);
    mTextBuf[mTextLen] = 0;
    return (u16)cpLen;
}

/* 0x8052DA10 (0xC4): Copies `strLen` characters into the buffer at `dstIdx`; returns the characters copied. */
u16 TextBox::SetString(const wchar_t* str, u16 dstIdx, u16 strLen) {
    if (!mTextBuf) {
        return 0;
    }

    const u16 bufLen = GetStringBufferLength();
    if (dstIdx >= bufLen) {
        return 0;
    }

    const u32 cpLen = ut::Min<u32>(strLen, bufLen - dstIdx);

    memcpy(mTextBuf + dstIdx, str, cpLen * sizeof(wchar_t));
    mTextLen = (u16)(dstIdx + cpLen);
    mTextBuf[mTextLen] = 0;
    return (u16)cpLen;
}

/* 0x8052DAD4 (0x8): Returns the box's font. */
const ut::Font* TextBox::GetFont() const {
    return mpFont;
}

/* 0x8052DADC (0x84): Loads the pane matrix with its y axis flipped, since text is laid out downwards. */
void TextBox::LoadMtx(const DrawInfo& drawInfo) {
    math::MTX34 mtx;

    if (drawInfo.mFlag.mulViewDraw) {
        PSMTXConcat(&drawInfo.mViewMtx, &mGlbMtx, &mtx);
    } else {
        PSMTXCopy(&mGlbMtx, &mtx);
    }

    mtx.m[0][1] = -mtx.m[0][1];
    mtx.m[1][1] = -mtx.m[1][1];
    mtx.m[2][1] = -mtx.m[2][1];

    GXLoadPosMtxImm(mtx.m, 0);
    GXSetCurrentMtx(0);
}

/* 0x8052DB60 (0x384): Returns where the string is drawn: its measured size placed in the box by the text
 * position. */
const ut::Rect TextBox::GetTextDrawRect(ut::TextWriterBase<wchar_t>* pWriter) const {
    ut::Rect textRect;

    pWriter->SetCursor(0.0f, 0.0f);
    pWriter->mFont = mpFont;
    pWriter->SetFontSize(mFontSize.width, mFontSize.height);
    const f32 lineSpace = mLineSpace;
    const f32 charSpace = mCharSpace;
    pWriter->mLineSpace = lineSpace;
    pWriter->mCharSpace = charSpace;
    if (mpTagProcessor) {
        pWriter->mTagProcessor = mpTagProcessor;
    }

    CalcStringRect(&textRect, *pWriter, mTextBuf, mTextLen, mSize.width);

    const Size textSize(textRect.right - textRect.left, textRect.bottom - textRect.top);
    math::VEC2 ltPos = static_cast<const math::VEC2&>(GetVtxPos());

    ltPos.x += (mSize.width - textSize.width) * GetTextMagH();
    ltPos.y -= (mSize.height - textSize.height) * GetTextMagV();

    textRect.left = ltPos.x;
    textRect.top = ltPos.y;
    textRect.right = ltPos.x + textSize.width;
    textRect.bottom = ltPos.y - textSize.height;

    return textRect;
}

namespace {

/* 0x8052DEE4 (0x1D4): Measures the whole string as lines no wider than `maxWidth`. */
void CalcStringRectImpl(ut::Rect* pRect, ut::TextWriterBase<wchar_t>* pTextWriter, const wchar_t* str,
                        int length, f32 maxWidth) {
    pRect->left = 0.0f;
    pRect->right = 0.0f;
    pRect->top = 0.0f;
    pRect->bottom = 0.0f;
    pTextWriter->SetCursor(0.0f, 0.0f);

    int remain = length;
    const wchar_t* pos = str;

    do {
        ut::Rect rect;
        bool bOver;
        int read = CalcLineRectImpl(&rect, pTextWriter, pos, remain, maxWidth, &bOver);

        pos += read;
        remain -= read;

        pRect->left = ut::Min(pRect->left, rect.left);
        pRect->top = ut::Min(pRect->top, rect.top);
        pRect->right = ut::Max(pRect->right, rect.right);
        pRect->bottom = ut::Max(pRect->bottom, rect.bottom);

        if (bOver) {
            CalcLineRectImpl(&rect, pTextWriter, L"\n", 1, maxWidth, &bOver);

            pRect->left = ut::Min(pRect->left, rect.left);
            pRect->top = ut::Min(pRect->top, rect.top);
            pRect->right = ut::Max(pRect->right, rect.right);
            pRect->bottom = ut::Max(pRect->bottom, rect.bottom);
        }
    } while (remain > 0);
}

}  // namespace

/* 0x8052E0B8 (0x8): Returns the class's run-time type record. */
const ut::detail::RuntimeTypeInfo* TextBox::GetRuntimeTypeInfo() const {
    return &typeInfo;
}

}  // namespace lyt
}  // namespace nw4r
