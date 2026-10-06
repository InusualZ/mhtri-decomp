/* nw4r/RomFont.h - the system-message bitmap font `nw4r/fn_8050661C.cpp` defines (nw4r::ut::RomFont's interface over
 *   a caller-supplied code table and glyph sheet). */
#ifndef MHTRI_NW4R_ROMFONT_H
#define MHTRI_NW4R_ROMFONT_H

#include "types.h"
#include "nw4r/fn_80502828.h"
#include "OS/OSFont.h"

#ifdef __cplusplus
namespace nw4r {
namespace ut {

/* A font of fixed 24x24 cells read from a code table and a glyph sheet. size: 0x1C */
class RomFont : public Font {
public:
    RomFont();
    virtual ~RomFont();

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
    virtual void ClearGlyphCache();

    /* untyped: byte range - the glyph sheet image */
    bool Load(const u16* codeTable, void* sheetImage, int sheetWidth, int sheetHeight);

    /* +0x10 */ OSFontHeader* mFontHeader;
    /* +0x14 */ CharWidths mDefaultWidths;
    /* +0x17 */ u8 pad_0x17;
    /* +0x18 */ u16 mAlternateChar;
    /* +0x1A */ u8 pad_0x1A[0x2];
};

}  // namespace ut
}  // namespace nw4r
#endif

#endif /* MHTRI_NW4R_ROMFONT_H */
