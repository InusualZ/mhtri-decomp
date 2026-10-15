/*
 * nw4r/ut_ResFontBase.cpp - nw4r::ut::detail::ResFontBase, the resource font: the FINF/TGLP/CWDH/CMAP lookups.
 * RANGE. .text 0x80502910-0x80503010 (25 functions); .data 0x8062FB80-0x8062FBE8 (the inline Font constructor's
 *   reader member-pointer constant and the ResFontBase vtable).
 * RANGE. The `.data` emission-order seam at 0x8062FBE8 (the ResFontBase vtable, then ut_ResFont's vtable) closes the
 *   file; its `.text` ends with GetGlyphFromIndex, before ResFont's constructor.
 * FLAGS. the `nw4r` lib block: GC/3.0a5.2 with `cflags_nw4r` (`cflags_os` + `-fp_contract off`); evidence in docs/nw4r.md.
 * NAMES. File and members from NW4R's ut_ResFontBase.cpp, read off the bodies and the vtable slots: the constructor,
 *   destructor, SetResourceBuffer, the virtual getters in vtable order, FindGlyphIndex(map, c) and GetGlyphFromIndex.
 * RESIDUALS. The weak Font::~Font and Font vtable this object emits are `nw4r/ut_Font.cpp`'s; the link drops them
 *   (a trial flip keeps the DOL hash), while flipcheck counts them because the map names them.
 */

#include "nw4r/ut_ResFontBase.h"

namespace nw4r {
namespace ut {

namespace detail {

/* The glyph index a code has no glyph for. */
static const u16 GLYPH_INDEX_NOT_FOUND = 0xFFFF;

/* 0x80502910 (0x54): makes an empty resource font. */
ResFontBase::ResFontBase() : mResource(NULL), mFontInfo(NULL), mLastCharCode(0), mLastGlyphIndex(GLYPH_INDEX_NOT_FOUND) {}

/* 0x80502964 (0x40): destroys a resource font. */
ResFontBase::~ResFontBase() {}

/* 0x805029A4 (0xC): attaches the resource buffer and its FINF block. */
/* untyped: the font resource, a byte image */
void ResFontBase::SetResourceBuffer(void* buffer, FontInformation* info) {
    mResource = buffer;
    mFontInfo = info;
}

/* 0x805029B0 (0xC): the font width. */
int ResFontBase::GetWidth() const {
    return mFontInfo->width;
}

/* 0x805029BC (0xC): the font height. */
int ResFontBase::GetHeight() const {
    return mFontInfo->height;
}

/* 0x805029C8 (0xC): the ascent. */
int ResFontBase::GetAscent() const {
    return mFontInfo->ascent;
}

/* 0x805029D4 (0x14): the descent, height minus ascent. */
int ResFontBase::GetDescent() const {
    return mFontInfo->height - mFontInfo->ascent;
}

/* 0x805029E8 (0x14): the baseline position inside a cell. */
int ResFontBase::GetBaselinePos() const {
    return mFontInfo->pGlyph->baselinePos;
}

/* 0x805029FC (0x10): the glyph cell height. */
int ResFontBase::GetCellHeight() const {
    return mFontInfo->pGlyph->cellHeight;
}

/* 0x80502A0C (0x10): the glyph cell width. */
int ResFontBase::GetCellWidth() const {
    return mFontInfo->pGlyph->cellWidth;
}

/* 0x80502A1C (0x10): the widest glyph. */
int ResFontBase::GetMaxCharWidth() const {
    return mFontInfo->pGlyph->maxCharWidth;
}

/* 0x80502A2C (0x8): the font type, a resource font. */
int ResFontBase::GetType() const {
    return 2;
}

/* 0x80502A34 (0x10): the texture format of the glyph sheets. */
int ResFontBase::GetTextureFormat() const {
    return mFontInfo->pGlyph->sheetFormat;
}

/* 0x80502A44 (0x10): the line feed. */
int ResFontBase::GetLineFeed() const {
    return mFontInfo->linefeed;
}

/* 0x80502A54 (0x18): the widths used for codes with no width entry. */
CharWidths ResFontBase::GetDefaultCharWidths() const {
    return mFontInfo->defaultWidth;
}

/* 0x80502A6C (0x20): replaces the default widths. */
void ResFontBase::SetDefaultCharWidths(const CharWidths& widths) {
    mFontInfo->defaultWidth = widths;
}

/* The glyph index of `c` through the one-entry cache. */
inline u16 ResFontBase::FindGlyphIndex(u16 c) const {
    const FontCodeMap* map;

    if (c == mLastCharCode) {
        return mLastGlyphIndex;
    }
    mLastCharCode = c;
    for (map = mFontInfo->pMap; map != NULL; map = map->pNext) {
        if (map->ccodeBegin <= c && c <= map->ccodeEnd) {
            mLastGlyphIndex = FindGlyphIndex(map, c);
            return mLastGlyphIndex;
        }
    }
    mLastGlyphIndex = GLYPH_INDEX_NOT_FOUND;
    return mLastGlyphIndex;
}

/* The glyph index of `c`, or the alternate character's when it has none. */
inline u16 ResFontBase::GetGlyphIndex(u16 c) const {
    u16 index = FindGlyphIndex(c);

    return (index != GLYPH_INDEX_NOT_FOUND) ? index : mFontInfo->alterCharIndex;
}

/* The widths of glyph `index`, or the default widths. */
inline const CharWidths& ResFontBase::GetCharWidthsFromIndex(u16 index) const {
    const FontWidth* width;

    for (width = mFontInfo->pWidth; width != NULL; width = width->pNext) {
        if (width->indexBegin <= index && index <= width->indexEnd) {
            return width->widthTable[index - width->indexBegin];
        }
    }
    return mFontInfo->defaultWidth;
}

/* 0x80502A8C (0xB8): makes `c` the character drawn for codes with no glyph; false when it has none itself. */
bool ResFontBase::SetAlternateChar(u16 c) {
    u16 index = FindGlyphIndex(c);

    if (index != GLYPH_INDEX_NOT_FOUND) {
        mFontInfo->alterCharIndex = index;
        return true;
    }
    return false;
}

/* 0x80502B44 (0xC): replaces the line feed. */
void ResFontBase::SetLineFeed(int linefeed) {
    mFontInfo->linefeed = linefeed;
}

/* 0x80502B50 (0x4C): the advance width of `c`. */
int ResFontBase::GetCharWidth(u16 c) const {
    return GetCharWidths(c).charWidth;
}

/* 0x80502B9C (0x110): the widths of `c`'s glyph. */
CharWidths ResFontBase::GetCharWidths(u16 c) const {
    return GetCharWidthsFromIndex(GetGlyphIndex(c));
}

/* 0x80502CAC (0xC4): describes `c`'s glyph. */
void ResFontBase::GetGlyph(Glyph* glyph, u16 c) const {
    GetGlyphFromIndex(glyph, GetGlyphIndex(c));
}

/* 0x80502D70 (0xB8): whether the font has a glyph for `c`. */
bool ResFontBase::HasGlyph(u16 c) const {
    return FindGlyphIndex(c) != GLYPH_INDEX_NOT_FOUND;
}

/* 0x80502E28 (0xC): the encoding the character codes are in. */
FontEncoding ResFontBase::GetEncoding() const {
    return static_cast<FontEncoding>(mFontInfo->encoding);
}

/* 0x80502E34 (0xD4): the glyph index `map` gives `c`: direct offset, table or binary-searched scan list. */
u16 ResFontBase::FindGlyphIndex(const FontCodeMap* map, u16 c) const {
    u16 index = GLYPH_INDEX_NOT_FOUND;

    switch (map->mappingMethod) {
    case 0:
        index = map->mapInfo[0] + (c - map->ccodeBegin);
        break;
    case 1:
        index = map->mapInfo[c - map->ccodeBegin];
        break;
    case 2: {
        const CMapInfoScan* scan = reinterpret_cast<const CMapInfoScan*>(map->mapInfo);
        const CMapScanEntry* first = &scan->entries[0];
        const CMapScanEntry* last = &scan->entries[scan->num - 1];

        while (first <= last) {
            const CMapScanEntry* mid = first + (last - first) / 2;

            if (mid->ccode < c) {
                first = mid + 1;
            } else if (c < mid->ccode) {
                last = mid - 1;
            } else {
                return mid->index;
            }
        }
    } break;
    }
    return index;
}

/* 0x80502F08 (0x108): describes glyph `index`: its sheet, cell position and widths. */
void ResFontBase::GetGlyphFromIndex(Glyph* glyph, u16 index) const {
    const FontTextureGlyph& tg = *mFontInfo->pGlyph;
    u32 cellsInSheet = tg.sheetRow * tg.sheetLine;
    u32 sheetNo = index / cellsInSheet;
    u32 cellNo = index % cellsInSheet;
    u32 cellUnitX = cellNo % tg.sheetRow;
    u32 cellUnitY = cellNo / tg.sheetRow;
    u32 cellPixelX = cellUnitX * (tg.cellWidth + 1);
    u32 cellPixelY = cellUnitY * (tg.cellHeight + 1);

    glyph->pTexture = tg.sheetImage + sheetNo * tg.sheetSize;
    glyph->widths = GetCharWidthsFromIndex(index);
    glyph->height = tg.cellHeight;
    glyph->texFormat = tg.sheetFormat;
    glyph->texWidth = tg.sheetWidth;
    glyph->texHeight = tg.sheetHeight;
    glyph->cellX = cellPixelX + 1;
    glyph->cellY = cellPixelY + 1;
}

}  // namespace detail

}  // namespace ut
}  // namespace nw4r
