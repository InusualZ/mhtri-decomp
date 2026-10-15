/*
 * nw4r/ut_TagProcessorBase.h - nw4r::ut::TagProcessorBase<T>, whose members `nw4r/ut_TagProcessorBase.cpp` defines
 *   and instantiates for char and wchar_t.
 */
#ifndef NW4R_UT_TAGPROCESSORBASE_H
#define NW4R_UT_TAGPROCESSORBASE_H

#include "types.h"
#include "nw4r/ut_Rect.h"

#ifdef __cplusplus
namespace nw4r {
namespace ut {

template <typename T> struct PrintContext;
template <typename T> class TextWriterBase;

/* What a tag processor asks the writer to do after a control character. */
enum Operation {
    OPERATION_DEFAULT = 0,
    OPERATION_NO_CHAR_SPACE = 1,
    OPERATION_CHAR_SPACE = 2,
    OPERATION_NEXT_LINE = 3,
    OPERATION_END_DRAW = 4
};

/* Handles a text writer's control characters (line feed and tab). size: 0x4 */
template <typename T> class TagProcessorBase {
public:
    TagProcessorBase();
    virtual ~TagProcessorBase();
    virtual Operation Process(u16 code, PrintContext<T>* context);
    virtual Operation CalcRect(Rect* rect, u16 code, PrintContext<T>* context);

    /* Moves the cursor to the start of the next line. */
    void ProcessLinefeed(PrintContext<T>* context) {
        TextWriterBase<T>& writer = *context->writer;
        f32 x = context->xOrigin;
        f32 y = writer.GetCursorY() + writer.GetLineHeight();

        writer.SetCursorX(x);
        writer.SetCursorY(y);
    }

    /* Moves the cursor to the next tab stop (tab width in character widths). */
    void ProcessTab(PrintContext<T>* context) {
        TextWriterBase<T>& writer = *context->writer;
        int tabWidth = writer.GetTabWidth();

        if (tabWidth > 0) {
            f32 charWidth = writer.IsWidthFixed() ? writer.GetFixedWidth() : writer.GetFontWidth();
            f32 dx = writer.GetCursorX() - context->xOrigin;
            f32 tabPixel = tabWidth * charWidth;
            int numTab = static_cast<int>(dx / tabPixel) + 1;
            f32 x = context->xOrigin + tabPixel * numTab;

            writer.SetCursorX(x);
        }
    }

    /* +0x0 */ /* the vtable */
};

}  // namespace ut
}  // namespace nw4r
#endif

#endif
