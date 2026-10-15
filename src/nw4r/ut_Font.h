/*
 * nw4r/ut_Font.h - nw4r::ut::Font, the font interface `nw4r/ut_Font.cpp` defines (its reader selection and the
 *   destructor that keys the vtable), with the glyph metrics every font hands the writer.
 */
#ifndef NW4R_UT_FONT_H
#define NW4R_UT_FONT_H

#include "types.h"
#include "nw4r/ut_CharStrmReader.h"

#ifdef __cplusplus
namespace nw4r {
namespace ut {

/* The text encoding a font maps its character codes from. */
enum FontEncoding {
    FONT_ENCODING_UTF8 = 0,
    FONT_ENCODING_UTF16 = 1,
    FONT_ENCODING_SJIS = 2,
    FONT_ENCODING_CP1252 = 3
};

/* A glyph's horizontal metrics. size: 0x3 */
struct CharWidths {
    /* +0x0 */ s8 left;
    /* +0x1 */ u8 glyphWidth;
    /* +0x2 */ s8 charWidth;
};

/* One glyph as the writer draws it: its texture sheet, metrics and cell. size: 0x14 */
struct Glyph {
    /* untyped: the texture sheet, a byte image in `texFormat` */
    /* +0x00 */ const void* pTexture;
    /* +0x04 */ CharWidths widths;
    /* +0x07 */ u8 height;
    /* +0x08 */ s32 texFormat;
    /* +0x0C */ u16 texWidth;
    /* +0x0E */ u16 texHeight;
    /* +0x10 */ u16 cellX;
    /* +0x12 */ u16 cellY;
};

/* A font: the metrics and glyph interface the writer draws through, and the reader for its encoding.
 * size: 0x10 */
class Font {
public:
    Font() : mReaderFunc(&CharStrmReader::ReadNextCharCP1252) {}
    virtual ~Font();

    virtual int GetWidth() const = 0;
    virtual int GetHeight() const = 0;
    virtual int GetAscent() const = 0;
    virtual int GetDescent() const = 0;
    virtual int GetBaselinePos() const = 0;
    virtual int GetCellHeight() const = 0;
    virtual int GetCellWidth() const = 0;
    virtual int GetMaxCharWidth() const = 0;
    virtual int GetType() const = 0;
    virtual int GetTextureFormat() const = 0;
    virtual int GetLineFeed() const = 0;
    virtual CharWidths GetDefaultCharWidths() const = 0;
    virtual void SetDefaultCharWidths(const CharWidths& widths) = 0;
    virtual bool SetAlternateChar(u16 c) = 0;
    virtual void SetLineFeed(int linefeed) = 0;
    virtual int GetCharWidth(u16 c) const = 0;
    virtual CharWidths GetCharWidths(u16 c) const = 0;
    virtual void GetGlyph(Glyph* glyph, u16 c) const = 0;
    virtual bool HasGlyph(u16 c) const = 0;
    virtual FontEncoding GetEncoding() const = 0;

    void InitReaderFunc(FontEncoding encoding);

    CharStrmReader GetCharStrmReader() const;

    /* +0x00 */ /* the vtable */
    /* +0x04 */ CharStrmReader::ReadFunc mReaderFunc;
};

#ifndef NW4R_UT_FONT_EMIT_DTOR
/* The empty destructor, inline wherever a derived font is built or destroyed; `nw4r/ut_Font.cpp` defines it out of
 * line instead, which emits it with the vtable there. */
inline Font::~Font() {}
#endif

}  // namespace ut
}  // namespace nw4r
#endif

#endif
