/*
 * nw4r/fn_80502828.h - declarations of the symbols owned by `nw4r/fn_80502828.cpp` that other units call or read:
 *   nw4r::ut's Font interface, the resource font (ResFontBase, ResFont) and the character writer.
 */
#ifndef NW4R_FN_80502828_H
#define NW4R_FN_80502828_H

#include "types.h"
#include "nw4r/fn_805012C4.h"
#include "gx.h"

#ifdef __cplusplus
extern "C" {
#endif

/* --------------------------------------------------------------------------------------------- */
/* Callees (their bodies live in their own units)                                                 */
/* --------------------------------------------------------------------------------------------- */
/* untyped: opaque band object, typed by the callers' views */
u32 fn_805041F4(void* self, int arg);

#ifdef __cplusplus
}
#endif

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

    /* +0x00 */ /* the vtable */
    /* +0x04 */ CharStrmReader::ReadFunc mReaderFunc;
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

/* An RGBA colour (the SDK's GXColor with nw4r's constructors). size: 0x4 */
struct Color : public _GXColor {
    Color() {
        *reinterpret_cast<u32*>(this) = 0xFFFFFFFF;
    }

    Color(u32 rgba) {
        *reinterpret_cast<u32*>(this) = rgba;
    }

    ~Color() {}

    u32 ToU32() const {
        return *reinterpret_cast<const u32*>(this);
    }
};

/* Draws glyphs of a Font through GX: colours, scale, cursor and the texture filter. size: 0x4C */
class CharWriter {
public:
    /* The two colours the glyph intensity is mapped between. */
    struct ColorMapping { /* size: 0x8 */
        /* +0x0 */ Color min;
        /* +0x4 */ Color max;
    };

    /* The colours of a glyph quad's four corners. */
    struct VertexColor { /* size: 0x10 */
        /* +0x0 */ Color lu;
        /* +0x4 */ Color ru;
        /* +0x8 */ Color ld;
        /* +0xC */ Color rd;
    };

    /* How the text colour runs from `start` to `end` across a glyph. */
    enum GradationMode {
        GRADMODE_NONE = 0,
        GRADMODE_H = 1,
        GRADMODE_V = 2
    };

    /* The text colour and its gradation. */
    struct TextColor { /* size: 0xC */
        /* +0x0 */ Color start;
        /* +0x4 */ Color end;
        /* +0x8 */ GradationMode gradationMode;
    };

    /* The GX minification/magnification filters. */
    struct TextureFilter { /* size: 0x8 */
        /* +0x0 */ s32 atSmall;
        /* +0x4 */ s32 atLarge;
    };

    /* The texture last loaded into a GX texture slot. */
    struct LoadingTexture { /* size: 0x10 */
        void Reset() {
            slot = 0xFF;
            texture = NULL;
        }

        bool operator!=(const LoadingTexture& rhs) const {
            return slot != rhs.slot || texture != rhs.texture || filter.atSmall != rhs.filter.atSmall ||
                   filter.atLarge != rhs.filter.atLarge;
        }

        /* +0x0 */ s32 slot;
        /* untyped: the texture sheet, a byte image */
        /* +0x4 */ const void* texture;
        /* +0x8 */ TextureFilter filter;
    };

    CharWriter();
    ~CharWriter();

    void SetColorMapping(Color min, Color max);
    void ResetColorMapping() {
        SetColorMapping(Color(0x00000000), Color(0xFFFFFFFF));
    }
    void SetGradationMode(GradationMode mode) {
        mTextColor.gradationMode = mode;
        UpdateVertexColor();
    }
    void SetTextColor(Color color) {
        mTextColor.start = color;
        UpdateVertexColor();
    }
    void SetScale(f32 hScale, f32 vScale) {
        mScale.x = hScale;
        mScale.y = vScale;
    }
    void SetCursor(f32 x, f32 y, f32 z) {
        mCursorPos.x = x;
        mCursorPos.y = y;
        mCursorPos.z = z;
    }
    void EnableLinearFilter(bool atSmall, bool atLarge) {
        mFilter.atSmall = atSmall ? 1 : 0;
        mFilter.atLarge = atLarge ? 1 : 0;
    }

    void SetupGX();
    void SetFontSize(f32 width, f32 height);
    f32 GetFontWidth() const;
    f32 GetFontHeight() const;
    f32 GetFontAscent() const;
    f32 Print(u16 code);
    void PrintGlyph(f32 x, f32 y, f32 z, const Glyph& glyph);
    void LoadTexture(const Glyph& glyph, s32 slot);
    void UpdateVertexColor();

    /* 0x80760CA8 (static, .bss) */ static LoadingTexture mLoadingTexture;

    /* +0x00 */ ColorMapping mColorMapping;
    /* +0x08 */ VertexColor mVertexColor;
    /* +0x18 */ TextColor mTextColor;
    /* +0x24 */ math::VEC2 mScale;
    /* +0x2C */ math::VEC3 mCursorPos;
    /* +0x38 */ TextureFilter mFilter;
    /* +0x40 */ u8 pad_0x40[0x2];
    /* +0x42 */ u8 mAlpha;
    /* +0x43 */ bool mIsWidthFixed;
    /* +0x44 */ f32 mFixedWidth;
    /* +0x48 */ const Font* mFont;
};

/* A resource font loaded from a BRFNT image. size: 0x1C */
class ResFont : public detail::ResFontBase {
public:
    ResFont();
    virtual ~ResFont();

    /* untyped: the BRFNT image, a byte buffer */
    bool SetResource(void* brfnt);
    static FontInformation* Rebuild(BinaryFileHeader* fileHeader);
};

}  // namespace ut
}  // namespace nw4r
#endif

#endif
