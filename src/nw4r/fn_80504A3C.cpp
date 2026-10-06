/*
 * nw4r/fn_80504A3C.cpp - nw4r::ut's text writer, TextWriterBase<T>, instantiated for char and wchar_t.
 * RANGE. .text 0x80504A3C-0x8050661C (11 functions); .ctors 0x8056F3DC-0x8056F3E0; .bss 0x80760CB8-0x80760CD0 (the
 *   default tag processors' destructor records); .sbss 0x80795770-0x80795780 (the two default tag processors and their
 *   guards); .sdata2 0x8079D510-0x8079D528.
 * RANGE. The retail link keeps only what the game reaches: GetLineHeight for char, the Font::GetCharStrmReader copy,
 *   and the wchar_t constructor, destructor, GetLineHeight, PrintMutable, CalcLineRectImpl, CalcStringRectImpl,
 *   PrintImpl and AdjustCursor; this object also emits the char functions the link drops.
 * FLAGS. the `nw4r` lib block: GC/3.0a5.2 with `cflags_nw4r`; evidence in docs/nw4r.md.
 * NAMES. GUESS from the nw4r source, read off the bodies: TextWriterBase<T>::GetLineHeight (fn_80504A3C <c>,
 *   fn_80504BA4 <w>), Font::GetCharStrmReader (fn_80504AB8), the wchar_t constructor/destructor (fn_80504AF0,
 *   fn_80504B4C), PrintMutable (fn_80504C20), CalcLineRectImpl (fn_80504C28), CalcStringRectImpl (fn_805052B0),
 *   PrintImpl (fn_805053D0), AdjustCursor (fn_80505E5C), the static initialiser (fn_80506598) and the default tag
 *   processors mDefaultTagProcessor<c>/<w>.
 * RESIDUALS. TextWriterBase's constructor and destructor have empty bodies by design (member-list construction and
 *   nw4r's empty destructor).
 *   CalcLineRectImpl 99.65 %: retail keeps the unused prMaxRect copy as one stack Rect at 0x38, this
 *   compiler splits it into four slots and the frame grows 0x70.  The static initialiser is named
 *   `__sinit_\fn_80504A3C_cpp`, which the map cannot carry yet.  Font::GetCharStrmReader is a strong function here
 *   (retail links a weak copy at 0x80504AB8, after GetLineHeight<c>); the char instantiation's other functions are
 *   emitted and dropped by the link.
 * SHAPES. GetCharStrmReader is defined under `#pragma auto_inline off` (retail calls it); PrintImpl declares useLimit,
 *   charSpace, prStrPos, prLineStrPos in that order (the saved-register order) and its width-limit test reads the
 *   local xOrigin.
 */

#include "nw4r/TextWriterBase.h"

namespace nw4r {
namespace ut {

/* The largest float: the "no width limit" value. */
static const f32 NW4R_FLT_MAX = 3.4028235e+38f;

template <typename T> TagProcessorBase<T> TextWriterBase<T>::mDefaultTagProcessor;

/* Min and max in nw4r's spelling (the first operand wins a tie). */
template <typename V> inline V Min(V a, V b) {
    return (a > b) ? b : a;
}

template <typename V> inline V Max(V a, V b) {
    return (a < b) ? b : a;
}

/* Makes a writer with no width limit, no extra spacing, tabs of four characters and the default tag processor. */
template <typename T>
TextWriterBase<T>::TextWriterBase()
    : mWidthLimit(NW4R_FLT_MAX), mCharSpace(0.0f), mLineSpace(0.0f), mTabWidth(4), mDrawFlag(0),
      mTagProcessor(&mDefaultTagProcessor) {}

/* Destroys the writer. */
template <typename T> TextWriterBase<T>::~TextWriterBase() {}

/* The scaled line feed plus the line spacing. */
template <typename T> f32 TextWriterBase<T>::GetLineHeight() const {
    const Font* font = GetFont();
    int linefeed = font != NULL ? font->GetLineFeed() : 0;

    return mLineSpace + GetScaleV() * linefeed;
}

/* Draws `length` characters of `str` and leaves the cursor after them. */
template <typename T> f32 TextWriterBase<T>::PrintMutable(const T* str, int length) {
    return PrintImpl(str, length, true);
}

/* Measures one line of `*str` (up to `length` characters) into `rect` and advances `*str` past it; true when the
 * line broke at the width limit. */
template <typename T> bool TextWriterBase<T>::CalcLineRectImpl(Rect* rect, const T** str, int length) {
    const T* const strBegin = *str;
    const T* const strEnd = strBegin + length;
    const bool useLimit = mWidthLimit < NW4R_FLT_MAX;
    PrintContext<T> context = {this, strBegin, 0.0f, 0.0f, 0};
    f32 x = 0.0f;
    bool charSpace = false;
    bool overLimit = false;
    const T* prStrPos;
    Rect prMaxRect;
    CharStrmReader reader = GetFont()->GetCharStrmReader();
    u16 code;

    rect->left = 0.0f;
    rect->right = 0.0f;
    rect->top = Min(0.0f, GetLineHeight());
    rect->bottom = Max(0.0f, GetLineHeight());
    prMaxRect = *rect;

    reader.Set(strBegin);
    prStrPos = NULL;
    code = reader.Next();
    while (static_cast<const T*>(reader.GetCurrentPos()) <= strEnd) {
        if (code < ' ') {
            Operation operation;
            Rect ctrlRect(x, 0.0f, 0.0f, 0.0f);

            context.str = static_cast<const T*>(reader.GetCurrentPos());
            context.flags = !charSpace;
            SetCursorX(x);

            if (useLimit && code != '\n' && prStrPos != NULL) {
                PrintContext<T> context2 = context;
                TextWriterBase<T> myCopy(*this);
                Rect rect2;

                context2.writer = &myCopy;
                operation = mTagProcessor->CalcRect(&rect2, code, &context2);
                if (rect2.GetWidth() > 0.0f && myCopy.GetCursorX() - context.xOrigin > mWidthLimit) {
                    overLimit = true;
                    code = '\n';
                    reader.Set(prStrPos);
                    continue;
                }
            }

            operation = mTagProcessor->CalcRect(&ctrlRect, code, &context);
            reader.Set(context.str);

            rect->left = Min(rect->left, ctrlRect.left);
            rect->top = Min(rect->top, ctrlRect.top);
            rect->right = Max(rect->right, ctrlRect.right);
            rect->bottom = Max(rect->bottom, ctrlRect.bottom);
            x = GetCursorX();

            if (operation == OPERATION_END_DRAW) {
                *str += length;
                return false;
            } else if (operation == OPERATION_NO_CHAR_SPACE) {
                charSpace = false;
            } else if (operation == OPERATION_CHAR_SPACE) {
                charSpace = true;
            } else if (operation == OPERATION_NEXT_LINE) {
                break;
            }
        } else {
            f32 dx = 0.0f;

            if (charSpace) {
                dx += GetCharSpace();
            }
            if (IsWidthFixed()) {
                dx += GetFixedWidth();
            } else {
                dx += GetFont()->GetCharWidth(code) * GetScaleH();
            }

            if (useLimit && prStrPos != NULL && x + dx > mWidthLimit) {
                overLimit = true;
                code = '\n';
                reader.Set(prStrPos);
                continue;
            }

            x += dx;
            rect->left = Min(rect->left, x);
            rect->right = Max(rect->right, x);
            charSpace = true;
        }

        if (useLimit) {
            prStrPos = static_cast<const T*>(reader.GetCurrentPos());
        }
        code = reader.Next();
    }

    *str = static_cast<const T*>(reader.GetCurrentPos());
    return overLimit;
}

/* Measures the whole of `str` (up to `length` characters), line by line, into `rect`. */
template <typename T> void TextWriterBase<T>::CalcStringRectImpl(Rect* rect, const T* str, int length) {
    const T* const end = str + length;
    int remain = length;
    const T* pos = str;

    rect->left = 0.0f;
    rect->right = 0.0f;
    rect->top = 0.0f;
    rect->bottom = 0.0f;
    SetCursor(0.0f, 0.0f);

    do {
        Rect lineRect;

        CalcLineRectImpl(&lineRect, &pos, remain);
        remain = end - pos;

        rect->left = Min(rect->left, lineRect.left);
        rect->top = Min(rect->top, lineRect.top);
        rect->right = Max(rect->right, lineRect.right);
        rect->bottom = Max(rect->bottom, lineRect.bottom);
    } while (remain > 0);
}

/* Draws `length` characters of `str` from the cursor, handling control characters, the width limit and the
 * alignment flags; returns the widest line. */
template <typename T> f32 TextWriterBase<T>::PrintImpl(const T* str, int length, bool moveCursor) {
    f32 xOrigin = GetCursorX();
    f32 yOrigin = GetCursorY();
    const f32 orgCursorY = yOrigin;
    const bool useLimit = mWidthLimit < NW4R_FLT_MAX;
    bool charSpace = false;
    const T* prStrPos = str;
    const T* prLineStrPos = str;
    f32 cursorYAdj;
    f32 textWidth = AdjustCursor(&xOrigin, &yOrigin, str, length);

    cursorYAdj = orgCursorY - GetCursorY();
    PrintContext<T> context = {this, str, xOrigin, yOrigin, 0};
    CharStrmReader reader = GetFont()->GetCharStrmReader();
    u16 code;

    reader.Set(str);
    code = reader.Next();
    while (static_cast<const T*>(reader.GetCurrentPos()) - str <= length) {
        if (code < ' ') {
            Operation operation;

            context.str = static_cast<const T*>(reader.GetCurrentPos());
            context.flags = !charSpace;

            if (useLimit && code != '\n' && prStrPos != prLineStrPos) {
                PrintContext<T> context2 = context;
                TextWriterBase<T> myCopy(*this);
                Rect rect;

                context2.writer = &myCopy;
                operation = mTagProcessor->CalcRect(&rect, code, &context2);
                if (rect.GetWidth() > 0.0f && myCopy.GetCursorX() - context.xOrigin > mWidthLimit) {
                    reader.Set(prStrPos);
                    code = '\n';
                    continue;
                }
            }

            operation = mTagProcessor->Process(code, &context);
            if (operation == OPERATION_NEXT_LINE) {
                if (IsDrawFlagSet(0x3, 0x1)) {
                    const int remain = length - (context.str - str);
                    const f32 width = CalcLineWidth(context.str, remain);
                    const f32 offset = (textWidth - width) / 2;

                    SetCursorX(context.xOrigin + offset);
                } else if (IsDrawFlagSet(0x3, 0x2)) {
                    const int remain = length - (context.str - str);
                    const f32 width = CalcLineWidth(context.str, remain);
                    const f32 offset = textWidth - width;

                    SetCursorX(context.xOrigin + offset);
                } else {
                    const f32 width = GetCursorX() - context.xOrigin;

                    textWidth = Max(textWidth, width);
                    SetCursorX(context.xOrigin);
                }

                if (useLimit) {
                    prLineStrPos = static_cast<const T*>(reader.GetCurrentPos());
                }
                charSpace = false;
            } else if (operation == OPERATION_NO_CHAR_SPACE) {
                charSpace = false;
            } else if (operation == OPERATION_CHAR_SPACE) {
                charSpace = true;
            } else if (operation == OPERATION_END_DRAW) {
                break;
            }
            reader.Set(context.str);
        } else {
            const f32 baseY = GetCursorY();

            if (useLimit && prStrPos != prLineStrPos) {
                const f32 baseX = GetCursorX();
                const f32 space = charSpace ? GetCharSpace() : 0.0f;
                const f32 width = IsWidthFixed() ? GetFixedWidth() : GetFont()->GetCharWidth(code) * GetScaleH();

                if (width + (space + (baseX - xOrigin)) > mWidthLimit) {
                    reader.Set(prStrPos);
                    code = '\n';
                    continue;
                }
            }

            if (charSpace) {
                MoveCursorX(GetCharSpace());
            }
            charSpace = true;
            {
                const f32 adj = -GetFont()->GetBaselinePos() * GetScaleV();

                MoveCursorY(adj);
                CharWriter::Print(code);
                SetCursorY(baseY);
            }
        }

        if (useLimit) {
            prStrPos = static_cast<const T*>(reader.GetCurrentPos());
        }
        code = reader.Next();
    }

    {
        const f32 width = GetCursorX() - context.xOrigin;

        textWidth = Max(textWidth, width);
    }

    if (IsDrawFlagSet(0x300, 0x100) || IsDrawFlagSet(0x300, 0x200)) {
        SetCursorY(orgCursorY);
    } else if (moveCursor) {
        if (IsDrawFlagSet(0x300, 0x000)) {
            SetCursorY(GetCursorY() - GetFontAscent());
        }
    } else {
        MoveCursorY(cursorYAdj);
    }
    return textWidth;
}

/* Moves the origin and the cursor for the alignment flags before `str` is drawn; returns the text width. */
template <typename T> f32 TextWriterBase<T>::AdjustCursor(f32* xOrigin, f32* yOrigin, const T* str, int length) {
    f32 textWidth = 0.0f;
    f32 textHeight = 0.0f;

    if (!IsDrawFlagSet(0x333, 0x300) && !IsDrawFlagSet(0x333, 0x000)) {
        Rect textRect;

        CalcStringRect(&textRect, str, length);
        textWidth = textRect.left + textRect.right;
        textHeight = textRect.top + textRect.bottom;
    }

    if (IsDrawFlagSet(0x30, 0x10)) {
        *xOrigin -= textWidth / 2;
    } else if (IsDrawFlagSet(0x30, 0x20)) {
        *xOrigin -= textWidth;
    }

    if (IsDrawFlagSet(0x300, 0x100)) {
        *yOrigin -= textHeight / 2;
    } else if (IsDrawFlagSet(0x300, 0x200)) {
        *yOrigin -= textHeight;
    }

    if (IsDrawFlagSet(0x3, 0x1)) {
        const f32 width = CalcLineWidth(str, length);
        const f32 offset = (textWidth - width) / 2;

        SetCursorX(*xOrigin + offset);
    } else if (IsDrawFlagSet(0x3, 0x2)) {
        const f32 width = CalcLineWidth(str, length);
        const f32 offset = textWidth - width;

        SetCursorX(*xOrigin + offset);
    } else {
        SetCursorX(*xOrigin);
    }

    if (IsDrawFlagSet(0x300, 0x300)) {
        SetCursorY(*yOrigin);
    } else {
        SetCursorY(*yOrigin + GetFontAscent());
    }
    return textWidth;
}

/* A reader set up for this font's encoding. */
#pragma auto_inline off
CharStrmReader Font::GetCharStrmReader() const {
    return CharStrmReader(mReaderFunc);
}
#pragma auto_inline reset

template class TextWriterBase<char>;
template class TextWriterBase<wchar_t>;

}  // namespace ut
}  // namespace nw4r
