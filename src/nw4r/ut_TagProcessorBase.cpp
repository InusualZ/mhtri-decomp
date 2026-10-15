/*
 * nw4r/ut_TagProcessorBase.cpp - nw4r::ut::TagProcessorBase<T>, the text writer's line-feed and tab handler,
 *   instantiated for char and wchar_t.
 * RANGE. .text 0x80501FA8-0x80502678 (8 functions); .data 0x8062FAC8-0x8062FAF0 (the <w> and <c> vtables);
 *   .sdata2 0x8079D4E8-0x8079D4F0 (the int-to-float magic double).
 * RANGE. The edges with the neighbouring nw4r files (math_types .. ut_LockedCache, 0x805012C4-0x80502828) are the
 *   namespace/class runs in link order; each file pools its own `.sdata2` constants (0.0f once per file).
 * FLAGS. the `nw4r` lib block: GC/3.0a5.2 with `cflags_nw4r` (`cflags_os` + `-fp_contract off`); evidence in docs/nw4r.md.
 * NAMES. File from NW4R's ut_TagProcessorBase.cpp; the map carries the template manglings (constructor,
 *   destructor, Process, CalcRect per instantiation).
 * RANGE. The inline ProcessLinefeed/ProcessTab copies this object emits are weak and dropped by the link.
 */

#include "nw4r/ut_TagProcessorBase.h"
#include "nw4r/TextWriterBase.h"

namespace nw4r {
namespace ut {

/* Makes a tag processor. */
template <typename T> TagProcessorBase<T>::TagProcessorBase() {}

/* Destroys a tag processor. */
template <typename T> TagProcessorBase<T>::~TagProcessorBase() {}

/* Acts on a line feed or a tab like Process and returns the rectangle the cursor moved through. */
template <typename T> Operation TagProcessorBase<T>::CalcRect(Rect* rect, u16 code, PrintContext<T>* context) {
    switch (code) {
    case '\n': {
        TextWriterBase<T>& writer = *context->writer;

        rect->right = writer.GetCursorX();
        rect->top = writer.GetCursorY();
        ProcessLinefeed(context);
        rect->left = writer.GetCursorX();
        rect->bottom = writer.GetCursorY() + context->writer->GetFontHeight();
        rect->Normalize();
        return OPERATION_NEXT_LINE;
    }
    case '\t': {
        TextWriterBase<T>& writer = *context->writer;

        rect->left = writer.GetCursorX();
        ProcessTab(context);
        rect->right = writer.GetCursorX();
        rect->top = writer.GetCursorY();
        rect->bottom = rect->top + writer.GetFontHeight();
        rect->Normalize();
        return OPERATION_NO_CHAR_SPACE;
    }
    default:
        return OPERATION_DEFAULT;
    }
}

/* Acts on a line feed or a tab; anything else is left to the writer. */
template <typename T> Operation TagProcessorBase<T>::Process(u16 code, PrintContext<T>* context) {
    switch (code) {
    case '\n':
        ProcessLinefeed(context);
        return OPERATION_NEXT_LINE;
    case '\t':
        ProcessTab(context);
        return OPERATION_NO_CHAR_SPACE;
    default:
        return OPERATION_DEFAULT;
    }
}

template class TagProcessorBase<char>;
template class TagProcessorBase<wchar_t>;

}  // namespace ut
}  // namespace nw4r
