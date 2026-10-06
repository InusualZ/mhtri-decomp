/*
 * nw4r/fn_8050661C.cpp - a fixed-cell bitmap font for the system messages: nw4r::ut::RomFont's interface over a
 *   caller-supplied code table and glyph sheet (24x24 cells, half-width codes 12 wide).
 * RANGE. .text 0x8050661C-0x80507050 (25 functions; DWCi_Base64Encode/Decode follow to 0x805073C0 and are the DWCi
 *   lane's); .data 0x8062FC48-0x8062FCB8 (the reader member pointer and the vtable; the base64 alphabet after it is
 *   DWCi's); .bss 0x80760CD0-0x80760F00 (the glyph cache table and the synthetic font header); .sdata 0x807941E8
 *   (the encoding); .sbss 0x80795780-0x807957B0 (the code table, the sheet and the sheet metrics).
 * FLAGS. the `nw4r` lib block: GC/3.0a5.2 with `cflags_nw4r`; evidence in docs/nw4r.md.
 * NAMES. GUESS: the class is nw4r::ut::RomFont by its slot-for-slot match with nw4r's (constructor, destructor, Load,
 *   the Font getters, SetAlternateChar's prev/0xFFFF/restore shape), with Load building the font header itself and
 *   one virtual of its own after GetEncoding (ClearGlyphCache, fn_80506C74); FindCodeIndex is fn_80506D1C.  The
 *   statics are named for what Load stores in them.  The only caller is `menu/menu_sysmsg.cpp` (fn_804518E8).
 * RESIDUALS. RomFont's destructor has an empty body by design (nw4r's empty destructor; the delete is the compiler's).
 *   HasGlyph 95.9 % (retail materialises both Shift-JIS range tests and shares `li r3,1` between them).
 *   Load returns an uninitialised local, as retail does (its `return` reads whatever r4 holds after InitReaderFunc).
 *   Data: the claimed `.data`/`.sdata` runs end with DWCi's base64 alphabet and its pointer (a second TU); this
 *   object also emits a weak copy of Font's vtable, which the link folds (`.data` 0xCC against the 0xB8 claim; `.sdata`
 *   0x2 against 0x10).
 * SHAPES. FindCodeIndex returns int and the callers keep the index as that int, narrowing it at each use (retail's
 *   `clrlwi` per use); the code table is read as u16 pairs and walked with a pointer.
 */

#include "nw4r/RomFont.h"

namespace nw4r {
namespace ut {

/* The caller's code table: a (code, type) pair of u16 per glyph, ended by code 0xFFFF; type 1 is half-width. */
static const u16* sCodeTable;
/* untyped: the glyph sheet, a byte image */
static void* sSheetImage;
static s32 sCellWidth;
static s32 sCellHeight;
static s32 sCacheCount;
static s32 sSheetColumns;
static s32 sSheetRows;
static s32 sSheetCells;
static s32 sMarginX;
static s32 sMarginY;
static s32 sSheetPitch;
static s32 sCacheMax;

/* 0 = CP1252 codes, 1 = Shift-JIS codes; 0xFFFF before Load. */
static u16 sEncoding = 0xFFFF;

/* The font header Load fills in. */
static OSFontHeader sFontHeader;
/* Per-glyph cache flags, cleared by Load and ClearGlyphCache. */
static u16 sGlyphCache[256];

static int FindCodeIndex(u16 code);

/* 0x8050661C (0x54): makes an empty font with '?' as the alternate character. */
RomFont::RomFont() : mFontHeader(NULL), mAlternateChar('?') {
    mDefaultWidths.left = 0;
    mDefaultWidths.glyphWidth = 0;
    mDefaultWidths.charWidth = 0;
}

/* 0x80506670 (0x40): destroys the font. */
RomFont::~RomFont() {}

/* 0x805066B0 (0x2CC): attaches a code table and a glyph sheet of `sheetWidth` x `sheetHeight` pixels, building the
 * font header for 24x24 Shift-JIS cells; false when a sheet is already attached. */
/* untyped: byte range - the glyph sheet image */
bool RomFont::Load(const u16* codeTable, void* sheetImage, int sheetWidth, int sheetHeight) {
    int i;
    void* fontData;

    if (mFontHeader != NULL) {
        return false;
    }

    sCodeTable = codeTable;
    sSheetImage = sheetImage;
    sCellWidth = 24;
    sCellHeight = 24;
    sCacheCount = 0;
    sSheetColumns = sheetWidth / 24;
    sSheetRows = sheetHeight / 24;
    sSheetCells = sSheetColumns * sSheetRows;
    sMarginX = 3;
    sMarginY = 3;
    sSheetPitch = sheetWidth / 8;
    sCacheMax = 100;
    sEncoding = 1;

    mFontHeader = &sFontHeader;
    mDefaultWidths.left = 0;
    mDefaultWidths.glyphWidth = GetCellWidth();
    mDefaultWidths.charWidth = GetMaxCharWidth();

    mFontHeader->fontType = 2;
    mFontHeader->firstChar = 0x8140;
    mFontHeader->lastChar = 0x9872;
    mFontHeader->invalChar = ' ';
    mFontHeader->ascent = 24;
    mFontHeader->descent = 0;
    mFontHeader->width = 24;
    mFontHeader->leading = 28;
    mFontHeader->cellWidth = 24;
    mFontHeader->cellHeight = 24;
    mFontHeader->sheetSize = 0x20000;
    mFontHeader->sheetFormat = 0;
    mFontHeader->sheetColumn = 21;
    mFontHeader->sheetRow = 21;
    mFontHeader->sheetWidth = sheetWidth;
    mFontHeader->sheetHeight = sheetHeight;
    mFontHeader->widthTable = 48;
    mFontHeader->sheetHeight = sheetHeight;
    mFontHeader->sheetImage = 0xF00;
    mFontHeader->sheetFullSize = (sheetWidth / 2) * sheetHeight;
    mFontHeader->c[0] = 0;
    mFontHeader->c[1] = 0x55;
    mFontHeader->c[2] = 0xAA;
    mFontHeader->c[3] = 0xFF;

    InitReaderFunc(GetEncoding());

    for (i = 0; i < 256; i++) {
        sGlyphCache[i] = 0;
    }
    return fontData != NULL;
}

/* 0x8050697C (0xC): the font width. */
int RomFont::GetWidth() const {
    return mFontHeader->width;
}

/* 0x80506988 (0x5C): the font height, ascent plus descent. */
int RomFont::GetHeight() const {
    return GetAscent() + GetDescent();
}

/* 0x805069E4 (0xC): the ascent. */
int RomFont::GetAscent() const {
    return mFontHeader->ascent;
}

/* 0x805069F0 (0xC): the descent. */
int RomFont::GetDescent() const {
    return mFontHeader->descent;
}

/* 0x805069FC (0xC): the baseline position, the ascent. */
int RomFont::GetBaselinePos() const {
    return mFontHeader->ascent;
}

/* 0x80506A08 (0xC): the cell height. */
int RomFont::GetCellHeight() const {
    return mFontHeader->cellHeight;
}

/* 0x80506A14 (0xC): the cell width. */
int RomFont::GetCellWidth() const {
    return mFontHeader->cellWidth;
}

/* 0x80506A20 (0xC): the widest character, the font width. */
int RomFont::GetMaxCharWidth() const {
    return mFontHeader->width;
}

/* 0x80506A2C (0x8): the font type, a ROM font. */
int RomFont::GetType() const {
    return 1;
}

/* 0x80506A34 (0x8): the texture format, I4. */
int RomFont::GetTextureFormat() const {
    return 0;
}

/* 0x80506A3C (0xC): the line feed. */
int RomFont::GetLineFeed() const {
    return mFontHeader->leading;
}

/* 0x80506A48 (0x18): the default widths. */
CharWidths RomFont::GetDefaultCharWidths() const {
    return mDefaultWidths;
}

/* 0x80506A60 (0x1C): replaces the default widths. */
void RomFont::SetDefaultCharWidths(const CharWidths& widths) {
    mDefaultWidths = widths;
}

/* 0x80506A7C (0x90): makes `c` the character drawn for codes with no glyph; false when it has none itself. */
bool RomFont::SetAlternateChar(u16 c) {
    u16 prev = mAlternateChar;
    u16 code;

    mAlternateChar = 0xFFFF;
    code = HasGlyph(c) ? c : mAlternateChar;
    if (code != 0xFFFF) {
        mAlternateChar = c;
        return true;
    }
    mAlternateChar = prev;
    return false;
}

/* 0x80506B0C (0xC): replaces the line feed. */
void RomFont::SetLineFeed(int linefeed) {
    mFontHeader->leading = linefeed;
}

/* 0x80506B18 (0x110): the advance width of `c`: 13 for a half-width code, 26 otherwise. */
int RomFont::GetCharWidth(u16 c) const {
    int index = FindCodeIndex(c);
    const u16* table = sCodeTable;
    int type;

    if (static_cast<u16>(index) >= 288) {
        type = 1;
    } else {
        type = table[static_cast<u16>(index) * 2 + 1];
    }
    if (type == 1) {
        return 13;
    } else {
        return 26;
    }
}

/* 0x80506C28 (0x4C): the widths of `c`: no left bearing, glyph and advance both its advance width. */
CharWidths RomFont::GetCharWidths(u16 c) const {
    CharWidths widths;
    int width = GetCharWidth(c);

    widths.glyphWidth = width;
    widths.left = 0;
    widths.charWidth = width;
    return widths;
}

/* 0x80506C74 (0xA8): forgets which glyphs are cached. */
void RomFont::ClearGlyphCache() {
    int i;

    for (i = 0; i < 256; i++) {
        sGlyphCache[i] = 0;
    }
    sCacheCount = 0;
}

/* The table index of `code`, or of '?' when the table has no entry for it. */
/* 0x80506D1C (0xB4) */
static int FindCodeIndex(u16 code) {
    const u16* entry;
    int i;

    for (i = 0, entry = sCodeTable; entry[0] != 0xFFFF; i++, entry += 2) {
        if (entry[0] == code) {
            return i;
        }
    }
    return FindCodeIndex('?');
}

/* 0x80506DD0 (0x184): describes `c`'s glyph: its cell in the sheet (16 cells of 32 pixels a row) and its width. */
void RomFont::GetGlyph(Glyph* glyph, u16 c) const {
    int index = FindCodeIndex(c);
    const u16* table = sCodeTable;
    int type;
    u8 width;

    if (static_cast<u16>(index) >= 288) {
        type = 1;
    } else {
        type = table[static_cast<u16>(index) * 2 + 1];
    }
    width = 24;
    if (type == 1) {
        width = 12;
    }

    glyph->pTexture = sSheetImage;
    glyph->widths.left = 0;
    glyph->widths.glyphWidth = width;
    glyph->widths.charWidth = width;
    glyph->height = mFontHeader->cellHeight;
    glyph->texFormat = 0;
    glyph->texWidth = mFontHeader->sheetWidth;
    glyph->texHeight = mFontHeader->sheetHeight;
    glyph->cellX = (static_cast<u16>(index) % 16) * 32;
    glyph->cellY = (static_cast<u16>(index) / 16) * 32;
}

/* Whether `c` is a printable single-byte Shift-JIS code (ASCII or half-width katakana). */
static inline bool IsHalfWidthSJIS(u16 c) {
    if (c > 0xFF) {
        return false;
    }
    return (c >= 0x20 && c <= 0x7E) || (c >= 0xA1 && c <= 0xDF);
}

/* Whether `c` is a two-byte Shift-JIS code in the table's lead-byte range. */
static inline bool IsFullWidthSJIS(u16 c) {
    u8 hi = c >> 8;
    u8 lo = c;

    return hi >= 0x81 && hi <= 0x98 && lo >= 0x40 && lo <= 0xFC;
}

/* 0x80506F54 (0xC4): whether `c` is a printable code of the current encoding. */
bool RomFont::HasGlyph(u16 c) const {
    switch (sEncoding) {
    case 0:
        return c >= 0x20 && c <= 0xFF;
    case 1: {
        bool result = true;

        if (!IsHalfWidthSJIS(c) && !IsFullWidthSJIS(c)) {
            result = false;
        }
        return result;
    }
    default:
        return false;
    }
}

/* 0x80507018 (0x34): the encoding the codes are in: UTF-16 for the Shift-JIS table, CP1252 otherwise. */
FontEncoding RomFont::GetEncoding() const {
    switch (sEncoding) {
    case 0:
        return FONT_ENCODING_CP1252;
    case 1:
        return FONT_ENCODING_UTF16;
    default:
        return FONT_ENCODING_CP1252;
    }
}

}  // namespace ut
}  // namespace nw4r
