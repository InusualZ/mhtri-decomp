/*
 * nw4r/ut_ResFontBase.h - nw4r::ut::detail::ResFontBase, the resource font `nw4r/ut_ResFontBase.cpp` defines, and
 *   the BRFNT blocks (FINF, TGLP, CWDH, CMAP) it reads.
 */
#ifndef NW4R_UT_RESFONTBASE_H
#define NW4R_UT_RESFONTBASE_H

#include "types.h"
#include "nw4r/ut_Font.h"

#ifdef __cplusplus
namespace nw4r {
namespace ut {

/* The glyph sheets of a resource font (the FINF's TGLP block). size: 0x18 */
struct FontTextureGlyph {
    /* +0x00 */ u8 cellWidth;
    /* +0x01 */ u8 cellHeight;
    /* +0x02 */ s8 baselinePos;
    /* +0x03 */ u8 maxCharWidth;
    /* +0x04 */ u32 sheetSize;
    /* +0x08 */ u16 sheetNum;
    /* +0x0A */ u16 sheetFormat;
    /* +0x0C */ u16 sheetRow;
    /* +0x0E */ u16 sheetLine;
    /* +0x10 */ u16 sheetWidth;
    /* +0x12 */ u16 sheetHeight;
    /* +0x14 */ u8* sheetImage;
};

/* One run of glyph widths (a CWDH block). size: 0x8 plus the table */
struct FontWidth {
    /* +0x0 */ u16 indexBegin;
    /* +0x2 */ u16 indexEnd;
    /* +0x4 */ FontWidth* pNext;
    /* +0x8 */ CharWidths widthTable[1];
};

/* One run of character codes mapped to glyph indices (a CMAP block). size: 0xC plus the mapping */
struct FontCodeMap {
    /* +0x0 */ u16 ccodeBegin;
    /* +0x2 */ u16 ccodeEnd;
    /* +0x4 */ u16 mappingMethod;
    /* +0x6 */ u16 reserved;
    /* +0x8 */ FontCodeMap* pNext;
    /* +0xC */ u16 mapInfo[1];
};

/* One entry of a scanned CMAP: a code and its glyph. size: 0x4 */
struct CMapScanEntry {
    /* +0x0 */ u16 ccode;
    /* +0x2 */ u16 index;
};

/* A scanned CMAP's payload: the entry count and the sorted entries. size: 0x2 plus the entries */
struct CMapInfoScan {
    /* +0x0 */ u16 num;
    /* +0x2 */ CMapScanEntry entries[1];
};

/* A resource font's FINF block. size: 0x18 */
struct FontInformation {
    /* +0x00 */ u8 fontType;
    /* +0x01 */ s8 linefeed;
    /* +0x02 */ u16 alterCharIndex;
    /* +0x04 */ CharWidths defaultWidth;
    /* +0x07 */ u8 encoding;
    /* +0x08 */ FontTextureGlyph* pGlyph;
    /* +0x0C */ FontWidth* pWidth;
    /* +0x10 */ FontCodeMap* pMap;
    /* +0x14 */ u8 height;
    /* +0x15 */ u8 width;
    /* +0x16 */ u8 ascent;
    /* +0x17 */ u8 pad_0x17;
};

namespace detail {

/* A font drawn from an NW4R font resource (FINF/TGLP/CWDH/CMAP blocks), with a one-entry glyph-index cache.
 * size: 0x1C */
class ResFontBase : public Font {
public:
    ResFontBase();
    virtual ~ResFontBase();

    virtual int GetWidth() const;
    virtual int GetHeight() const;
    virtual int GetAscent() const;
    virtual int GetDescent() const;
    virtual int GetBaselinePos() const;
    virtual int GetCellHeight() const;
    virtual int GetCellWidth() const;
    virtual int GetMaxCharWidth() const;
    virtual int GetType() const;
    virtual int GetTextureFormat() const;
    virtual int GetLineFeed() const;
    virtual CharWidths GetDefaultCharWidths() const;
    virtual void SetDefaultCharWidths(const CharWidths& widths);
    virtual bool SetAlternateChar(u16 c);
    virtual void SetLineFeed(int linefeed);
    virtual int GetCharWidth(u16 c) const;
    virtual CharWidths GetCharWidths(u16 c) const;
    virtual void GetGlyph(Glyph* glyph, u16 c) const;
    virtual bool HasGlyph(u16 c) const;
    virtual FontEncoding GetEncoding() const;

    /* untyped: the font resource, a byte image */
    void SetResourceBuffer(void* buffer, FontInformation* info);
    u16 FindGlyphIndex(u16 c) const;
    u16 FindGlyphIndex(const FontCodeMap* map, u16 c) const;
    u16 GetGlyphIndex(u16 c) const;
    const CharWidths& GetCharWidthsFromIndex(u16 index) const;
    void GetGlyphFromIndex(Glyph* glyph, u16 index) const;

    /* untyped: the font resource, a byte image */
    /* +0x10 */ void* mResource;
    /* +0x14 */ FontInformation* mFontInfo;
    /* +0x18 */ mutable u16 mLastCharCode;
    /* +0x1A */ mutable u16 mLastGlyphIndex;
};

}  // namespace detail

}  // namespace ut
}  // namespace nw4r
#endif

#endif
