/* OS/OSFont.h - the RVL SDK's ROM font header (`OSFontHeader`). */
#ifndef MHTRI_OS_OSFONT_H
#define MHTRI_OS_OSFONT_H

#include "types.h"

/* size: 0x30 */
typedef struct OSFontHeader {
    /* +0x00 */ u16 fontType;
    /* +0x02 */ u16 firstChar;
    /* +0x04 */ u16 lastChar;
    /* +0x06 */ u16 invalChar;
    /* +0x08 */ u16 ascent;
    /* +0x0A */ u16 descent;
    /* +0x0C */ u16 width;
    /* +0x0E */ u16 leading;
    /* +0x10 */ u16 cellWidth;
    /* +0x12 */ u16 cellHeight;
    /* +0x14 */ u32 sheetSize;
    /* +0x18 */ u16 sheetFormat;
    /* +0x1A */ u16 sheetColumn;
    /* +0x1C */ u16 sheetRow;
    /* +0x1E */ u16 sheetWidth;
    /* +0x20 */ u16 sheetHeight;
    /* +0x22 */ u16 widthTable;
    /* +0x24 */ u32 sheetImage;
    /* +0x28 */ u32 sheetFullSize;
    /* +0x2C */ u8 c[4];
} OSFontHeader;

#endif /* MHTRI_OS_OSFONT_H */
