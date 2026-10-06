/* nw4r/db_console.h - nw4r::db's on-screen console record, as `nw4r/db_assert.cpp` reads it.  The console
 *   functions themselves sit in `WPAD/wpad.cpp`'s range (its header comment: unproven seam). */
#ifndef MHTRI_NW4R_DB_CONSOLE_H
#define MHTRI_NW4R_DB_CONSOLE_H

#include "types.h"

#ifdef __cplusplus
namespace nw4r {
namespace db {
namespace detail {

/* A text console: a ring buffer of `height` lines of `width` characters and the window shown of it.
 * size: 0x2C */
struct ConsoleHead {
    /* +0x00 */ u8* textBuf;
    /* +0x04 */ u16 width;
    /* +0x06 */ u16 height;
    /* +0x08 */ u16 priority;
    /* +0x0A */ u16 attr;
    /* +0x0C */ u16 printTop;
    /* +0x0E */ u16 printXPos;
    /* +0x10 */ u16 ringTop;
    /* +0x12 */ u8 pad_0x12[0x2];
    /* +0x14 */ s32 ringTopLineCnt;
    /* +0x18 */ s32 viewTopLine;
    /* +0x1C */ s16 viewPosX;
    /* +0x1E */ s16 viewPosY;
    /* +0x20 */ u16 viewLines;
    /* +0x22 */ bool isVisible;
    /* +0x23 */ u8 pad_0x23;
    /* +0x24 */ u8 pad_0x24[0x8];
};

}  // namespace detail

typedef detail::ConsoleHead* ConsoleHandle;

}  // namespace db
}  // namespace nw4r
#endif

#endif /* MHTRI_NW4R_DB_CONSOLE_H */
