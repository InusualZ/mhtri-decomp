/*
 * nw4r/fn_80502828.cpp - nw4r::ut's Font interface, the resource font (ResFontBase, ResFont) and the character writer.
 * RANGE. .text 0x80502828-0x80504A3C (43 functions); .data 0x8062FAF0-0x8062FC48 (the reader member-pointer constants
 *   and the Font, ResFontBase and ResFont vtables); .bss 0x80760C98-0x80760CB8 (the fog colour's destructor record
 *   and CharWriter::mLoadingTexture); .sbss 0x80795768-0x80795770 (the fog colour and its guard); .sdata2
 *   0x8079D4F0-0x8079D510.
 * RANGE. Several nw4r source files in link order: ut_Font (0x80502828-0x80502910), ut_ResFontBase (-0x80503010),
 *   ut_ResFont (-0x80503314) and ut_CharWriter (-0x80504A3C); each has its own `.data`/`.sdata2` run, which one
 *   object cannot reproduce (the emission-order seam at 0x8062FBE8, the pad words after each vtable).
 * FLAGS. the `nw4r` lib block: GC/3.0a5.2 with `cflags_nw4r` (`cflags_os` + `-fp_contract off`); evidence in docs/nw4r.md.
 * NAMES. GUESS from the nw4r source, read off the bodies and the vtable slots: Font::InitReaderFunc (fn_80502828),
 *   Font::~Font, detail::ResFontBase's constructor, destructor, SetResourceBuffer, the virtual getters in vtable order,
 *   FindGlyphIndex(map, c) and GetGlyphFromIndex (fn_80502910..fn_80502F08), ResFont's constructor, destructor,
 *   SetResource and Rebuild (fn_80503010..fn_805031E8), CharWriter's constructor, SetColorMapping, destructor, SetupGX,
 *   SetFontSize, GetFontWidth/Height/Ascent, Print, PrintGlyph, UpdateVertexColor (fn_80503314..fn_805045A0) and the
 *   file-local SetupGXWithColorMapping (fn_805046F0).
 * RESIDUALS. Every function matches.  Data: the four retail files' `.data`/`.sdata2`/`.bss` runs are interleaved with
 *   pads one object does not emit; the CharWriter constructor calls the out-of-line SetTextColor that retail links
 *   from 0x80500844 (`WPAD/wpad.cpp`'s range).  SetupGX and SetupGXWithColorMapping register the static fog colour
 *   with our `__dt__Q34nw4r2ut5ColorFv` where retail names `dtor_8005B228` (the weak Color destructor the link keeps
 *   from `font/flfnt.cpp`'s range; request filed to rename it).
 * SHAPES. ResFont's constructor, destructor and SetResource sit under `#pragma dont_inline` (ResFontBase is another
 *   retail file there).  SetupGXForRGBA spells out the default setup instead of calling it (the inline depth orders
 *   the fog copies' stack slots).
 */

#include "nw4r/fn_80502828.h"
#include "EXI/ProbeBarnacle.h"
#include "RVLGX/GXTexture_tail.h"
#include "EXI/GXSetTexCoordGen2.h"

/* ---------------------------------------------------------------------------------------------------------------
 * ut_Font
 * ------------------------------------------------------------------------------------------------------------ */

namespace nw4r {
namespace ut {

/* 0x80502828 (0xA8): selects the character reader for `encoding` (CP1252 for anything unknown). */
void Font::InitReaderFunc(FontEncoding encoding) {
    switch (encoding) {
    case FONT_ENCODING_UTF8:
        mReaderFunc = &CharStrmReader::ReadNextCharUTF8;
        break;
    case FONT_ENCODING_UTF16:
        mReaderFunc = &CharStrmReader::ReadNextCharUTF16;
        break;
    case FONT_ENCODING_SJIS:
        mReaderFunc = &CharStrmReader::ReadNextCharSJIS;
        break;
    case FONT_ENCODING_CP1252:
    default:
        mReaderFunc = &CharStrmReader::ReadNextCharCP1252;
        break;
    }
}

/* ---------------------------------------------------------------------------------------------------------------
 * ut_ResFontBase
 * ------------------------------------------------------------------------------------------------------------ */

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

/* ---------------------------------------------------------------------------------------------------------------
 * ut_ResFont
 * ------------------------------------------------------------------------------------------------------------ */

namespace nw4r {
namespace ut {

/* A file signature or block kind spelled as four characters. */
#define NW4R_MAGIC(a, b, c, d) ((static_cast<u32>(a) << 24) | (static_cast<u32>(b) << 16) | (static_cast<u32>(c) << 8) | (d))

namespace {

/* Turns an offset from `base` stored in `ptr` into a pointer. */
/* untyped: byte range - the resource image the offsets count from */
template <typename T> inline void ResolveOffset(T*& ptr, void* base) {
    ptr = reinterpret_cast<T*>(reinterpret_cast<u8*>(base) + reinterpret_cast<u32>(ptr));
}

}  // namespace

/* ResFontBase lives in its own retail file: nothing of it is inlined here. */
#pragma dont_inline on

/* 0x80503010 (0x3C): makes an empty font. */
ResFont::ResFont() {}

/* 0x8050304C (0x58): destroys the font. */
ResFont::~ResFont() {}

/* 0x805030A4 (0x144): attaches a BRFNT image, resolving its offsets the first time; false when one is attached,
 * the image is not a font or it has no FINF block. */
/* untyped: the BRFNT image, a byte buffer */
bool ResFont::SetResource(void* brfnt) {
    FontInformation* info = NULL;
    BinaryFileHeader* fileHeader = static_cast<BinaryFileHeader*>(brfnt);

    if (mResource != NULL) {
        return false;
    }

    if (fileHeader->signature == NW4R_MAGIC('R', 'F', 'N', 'U')) {
        BinaryBlockHeader* blockHeader =
            reinterpret_cast<BinaryBlockHeader*>(reinterpret_cast<u8*>(fileHeader) + fileHeader->headerSize);
        int blocks;

        for (blocks = 0; blocks < fileHeader->dataBlocks; blocks++) {
            if (blockHeader->kind == NW4R_MAGIC('F', 'I', 'N', 'F')) {
                info = reinterpret_cast<FontInformation*>(blockHeader + 1);
                break;
            }
            blockHeader = reinterpret_cast<BinaryBlockHeader*>(reinterpret_cast<u8*>(blockHeader) + blockHeader->size);
        }
    } else {
        if (fileHeader->version == 0x0104) {
            if (!IsValidBinaryFile(fileHeader, NW4R_MAGIC('R', 'F', 'N', 'T'), 0x0104, 2)) {
                return false;
            }
        } else {
            if (!IsValidBinaryFile(fileHeader, NW4R_MAGIC('R', 'F', 'N', 'T'), 0x0102, 2)) {
                return false;
            }
        }
        info = Rebuild(fileHeader);
    }

    if (info == NULL) {
        return false;
    }
    SetResourceBuffer(brfnt, info);
    InitReaderFunc(GetEncoding());
    return true;
}

#pragma dont_inline reset

/* 0x805031E8 (0x12C): resolves every block's offsets in place, marks the image resolved and returns its FINF
 * block; NULL on an unknown block. */
FontInformation* ResFont::Rebuild(BinaryFileHeader* fileHeader) {
    BinaryBlockHeader* blockHeader =
        reinterpret_cast<BinaryBlockHeader*>(reinterpret_cast<u8*>(fileHeader) + fileHeader->headerSize);
    FontInformation* info = NULL;
    int blocks = 0;

    while (blocks < fileHeader->dataBlocks) {
        switch (blockHeader->kind) {
        case NW4R_MAGIC('F', 'I', 'N', 'F'):
            info = reinterpret_cast<FontInformation*>(blockHeader + 1);
            ResolveOffset(info->pGlyph, fileHeader);
            if (info->pWidth != NULL) {
                ResolveOffset(info->pWidth, fileHeader);
            }
            if (info->pMap != NULL) {
                ResolveOffset(info->pMap, fileHeader);
            }
            break;
        case NW4R_MAGIC('T', 'G', 'L', 'P'):
            ResolveOffset(reinterpret_cast<FontTextureGlyph*>(blockHeader + 1)->sheetImage, fileHeader);
            break;
        case NW4R_MAGIC('C', 'W', 'D', 'H'): {
            FontWidth* width = reinterpret_cast<FontWidth*>(blockHeader + 1);

            if (width->pNext != NULL) {
                ResolveOffset(width->pNext, fileHeader);
            }
        } break;
        case NW4R_MAGIC('C', 'M', 'A', 'P'): {
            FontCodeMap* map = reinterpret_cast<FontCodeMap*>(blockHeader + 1);

            if (map->pNext != NULL) {
                ResolveOffset(map->pNext, fileHeader);
            }
        } break;
        case NW4R_MAGIC('G', 'L', 'G', 'R'):
            break;
        default:
            return NULL;
        }
        blockHeader = reinterpret_cast<BinaryBlockHeader*>(reinterpret_cast<u8*>(blockHeader) + blockHeader->size);
        blocks++;
    }

    fileHeader->signature = NW4R_MAGIC('R', 'F', 'N', 'U');
    return info;
}

}  // namespace ut
}  // namespace nw4r

/* ---------------------------------------------------------------------------------------------------------------
 * ut_CharWriter
 * ------------------------------------------------------------------------------------------------------------ */

namespace nw4r {
namespace ut {

CharWriter::LoadingTexture CharWriter::mLoadingTexture;

/* 0x80503314 (0x220): makes a writer with white text, unit scale, the cursor at the origin and linear filtering. */
CharWriter::CharWriter() : mAlpha(0xFF), mIsWidthFixed(false), mFixedWidth(0.0f), mFont(NULL) {
    mLoadingTexture.Reset();
    ResetColorMapping();
    SetGradationMode(GRADMODE_NONE);
    SetTextColor(Color(0xFFFFFFFF));
    SetScale(1.0f, 1.0f);
    SetCursor(0.0f, 0.0f, 0.0f);
    EnableLinearFilter(true, true);
}

/* 0x80503534 (0x44): sets the colours the glyph intensity is mapped between. */
void CharWriter::SetColorMapping(Color min, Color max) {
    mColorMapping.min = min;
    mColorMapping.max = max;
}

/* 0x80503578 (0x40): destroys the writer. */
CharWriter::~CharWriter() {}

/* The GX state every glyph draw shares: no fog, the identity swap table, one colour channel, one texture
 * coordinate and alpha blending. */
static inline void SetupGXCommon() {
    static const Color fog(0x00000000);

    GXSetFog(0, 0.0f, 0.0f, 0.0f, 0.0f, fog);
    GXSetTevSwapModeTable(0, 0, 1, 2, 3);
    GXSetZTexture(0, 17, 0);
    GXSetNumChans(1);
    GXSetChanCtrl(4, 0, 0, 1, 0, 0, 2);
    GXSetChanCtrl(5, 0, 0, 0, 0, 0, 2);
    GXSetNumTexGens(1);
    GXSetTexCoordGen2(0, 1, 4, 60, 0, 125);
    GXSetNumIndStages(0);
    GXSetBlendMode(1, 4, 5, 15);
}

/* The glyph vertex format: float position, RGBA8 colour and u16 texture coordinates. */
static inline void SetupVertexFormat() {
    GXSetVtxAttrFmt(0, 9, 1, 4, 0);
    GXSetVtxAttrFmt(0, 11, 1, 5, 0);
    GXSetVtxAttrFmt(0, 13, 1, 2, 15);
    GXClearVtxDesc();
    GXSetVtxDesc(9, 1);
    GXSetVtxDesc(11, 1);
    GXSetVtxDesc(13, 1);
}

static void SetupGXWithColorMapping(Color min, Color max);

/* Intensity textures: the vertex colour, with the texture as alpha. */
static inline void SetupGXForI() {
    SetupGXCommon();
    GXSetNumTevStages(1);
    GXSetTevDirect(0);
    GXSetTevSwapMode(0, 0, 0);
    GXSetTevOrder(0, 0, 0, 4);
    GXSetTevColorIn(0, 15, 15, 15, 10);
    GXSetTevAlphaIn(0, 7, 4, 5, 7);
    GXSetTevColorOp(0, 0, 0, 0, 1, 0);
    GXSetTevAlphaOp(0, 0, 0, 0, 1, 0);
    SetupVertexFormat();
}

/* Any other texture: the texture modulated by the vertex colour. */
static inline void SetupGXDefault() {
    SetupGXCommon();
    GXSetNumTevStages(1);
    GXSetTevDirect(0);
    GXSetTevSwapMode(0, 0, 0);
    GXSetTevOrder(0, 0, 0, 4);
    GXSetTevOp(0, 0);
    SetupVertexFormat();
}

/* RGBA textures: the texture modulated by the vertex colour, as the default. */
static inline void SetupGXForRGBA() {
    SetupGXCommon();
    GXSetNumTevStages(1);
    GXSetTevDirect(0);
    GXSetTevSwapMode(0, 0, 0);
    GXSetTevOrder(0, 0, 0, 4);
    GXSetTevOp(0, 0);
    SetupVertexFormat();
}

/* 0x805035B8 (0xA58): sets GX up to draw this writer's glyphs: colour mapping when one is set, else the setup for
 * the font's texture format. */
void CharWriter::SetupGX() {
    mLoadingTexture.Reset();
    if (mColorMapping.min.ToU32() != 0x00000000 || mColorMapping.max.ToU32() != 0xFFFFFFFF) {
        SetupGXWithColorMapping(mColorMapping.min, mColorMapping.max);
    } else if (mFont != NULL) {
        switch (mFont->GetTextureFormat()) {
        case 0:
        case 1:
            SetupGXForI();
            break;
        case 2:
        case 3:
            SetupGXDefault();
            break;
        case 4:
        case 5:
        case 6:
            SetupGXForRGBA();
            break;
        default:
            SetupGXDefault();
            break;
        }
    } else {
        SetupGXDefault();
    }
}

/* 0x80504010 (0xC4): scales the font to draw glyphs `width` by `height` pixels. */
void CharWriter::SetFontSize(f32 width, f32 height) {
    f32 vScale = height / mFont->GetHeight();
    f32 hScale = width / mFont->GetWidth();

    SetScale(hScale, vScale);
}

/* 0x805040D4 (0x60): the scaled font width. */
f32 CharWriter::GetFontWidth() const {
    return mScale.x * mFont->GetWidth();
}

/* 0x80504134 (0x60): the scaled font height. */
f32 CharWriter::GetFontHeight() const {
    return mScale.y * mFont->GetHeight();
}

/* 0x80504194 (0x60): the scaled ascent. */
f32 CharWriter::GetFontAscent() const {
    return mScale.y * mFont->GetAscent();
}

/* 0x805041F4 (0x130): draws `code` at the cursor, advances it and returns the advance. */
f32 CharWriter::Print(u16 code) {
    Glyph glyph;
    f32 width;
    f32 left;

    mFont->GetGlyph(&glyph, code);
    if (mIsWidthFixed) {
        f32 margin = (mFixedWidth - glyph.widths.charWidth * mScale.x) / 2;

        width = mFixedWidth;
        left = margin + glyph.widths.left * mScale.x;
    } else {
        width = glyph.widths.charWidth * mScale.x;
        left = glyph.widths.left * mScale.x;
    }
    PrintGlyph(mCursorPos.x + left, mCursorPos.y, mCursorPos.z, glyph);
    mCursorPos.x += width;
    return width;
}

/* Loads the glyph's sheet into texture `slot` unless it is already there with the same filter. */
inline void CharWriter::LoadTexture(const Glyph& glyph, s32 slot) {
    LoadingTexture loadInfo;

    loadInfo.slot = slot;
    loadInfo.texture = glyph.pTexture;
    loadInfo.filter = mFilter;
    if (loadInfo != mLoadingTexture) {
        GXTexObj texObj;

        GXInitTexObj(&texObj, const_cast<void*>(glyph.pTexture), glyph.texWidth, glyph.texHeight, glyph.texFormat, 0,
                     0, 0);
        GXInitTexObjLOD(&texObj, mFilter.atSmall, mFilter.atLarge, 0.0f, 0.0f, 0.0f, 0, 0, 0);
        GXLoadTexObj(&texObj, slot);
        mLoadingTexture = loadInfo;
    }
}

/* 0x80504324 (0x27C): draws one glyph quad with its top-left corner at (x, y, z). */
void CharWriter::PrintGlyph(f32 x, f32 y, f32 z, const Glyph& glyph) {
    f32 posLeft = x;
    f32 posTop = y;
    f32 posRight = posLeft + glyph.widths.glyphWidth * mScale.x;
    f32 posBottom = posTop + glyph.height * mScale.y;
    f32 posZ = z;
    u16 texLeft = static_cast<u16>((static_cast<u32>(glyph.cellX) << 15) / glyph.texWidth);
    u16 texTop = static_cast<u16>((static_cast<u32>(glyph.cellY) << 15) / glyph.texHeight);
    u16 texRight = static_cast<u16>((static_cast<u32>(glyph.cellX + glyph.widths.glyphWidth) << 15) / glyph.texWidth);
    u16 texBottom = static_cast<u16>((static_cast<u32>(glyph.cellY + glyph.height) << 15) / glyph.texHeight);

    LoadTexture(glyph, 0);
    GXBegin(0x80, 0, 4);

    GXWGFifo.f32 = posLeft;
    GXWGFifo.f32 = posTop;
    GXWGFifo.f32 = posZ;
    GXWGFifo.u32 = *reinterpret_cast<const u32*>(&mVertexColor.lu);
    GXWGFifo.u16 = texLeft;
    GXWGFifo.u16 = texTop;

    GXWGFifo.f32 = posRight;
    GXWGFifo.f32 = posTop;
    GXWGFifo.f32 = posZ;
    GXWGFifo.u32 = *reinterpret_cast<const u32*>(&mVertexColor.ru);
    GXWGFifo.u16 = texRight;
    GXWGFifo.u16 = texTop;

    GXWGFifo.f32 = posRight;
    GXWGFifo.f32 = posBottom;
    GXWGFifo.f32 = posZ;
    GXWGFifo.u32 = *reinterpret_cast<const u32*>(&mVertexColor.rd);
    GXWGFifo.u16 = texRight;
    GXWGFifo.u16 = texBottom;

    GXWGFifo.f32 = posLeft;
    GXWGFifo.f32 = posBottom;
    GXWGFifo.f32 = posZ;
    GXWGFifo.u32 = *reinterpret_cast<const u32*>(&mVertexColor.ld);
    GXWGFifo.u16 = texLeft;
    GXWGFifo.u16 = texBottom;
}

/* 0x805045A0 (0x150): recomputes the corner colours from the text colour, its gradation and the alpha. */
void CharWriter::UpdateVertexColor() {
    mVertexColor.lu = mTextColor.start;
    mVertexColor.ru = (mTextColor.gradationMode != GRADMODE_H) ? mTextColor.start : mTextColor.end;
    mVertexColor.ld = (mTextColor.gradationMode != GRADMODE_V) ? mTextColor.start : mTextColor.end;
    mVertexColor.rd = (mTextColor.gradationMode == GRADMODE_NONE) ? mTextColor.start : mTextColor.end;

    mVertexColor.lu.a = mVertexColor.lu.a * mAlpha / 0xFF;
    mVertexColor.ru.a = mVertexColor.ru.a * mAlpha / 0xFF;
    mVertexColor.ld.a = mVertexColor.ld.a * mAlpha / 0xFF;
    mVertexColor.rd.a = mVertexColor.rd.a * mAlpha / 0xFF;
}

/* Maps the glyph intensity between `min` and `max` in a first TEV stage and modulates by the vertex colour in a
 * second. */
static void SetupGXWithColorMapping(Color min, Color max) {
    SetupGXCommon();
    GXSetNumTevStages(2);
    GXSetTevDirect(0);
    GXSetTevDirect(1);
    GXSetTevSwapMode(0, 0, 0);
    GXSetTevSwapMode(1, 0, 0);
    GXSetTevOrder(0, 0, 0, 0xFF);
    GXSetTevColor(1, min);
    GXSetTevColor(2, max);
    GXSetTevColorIn(0, 2, 4, 8, 15);
    GXSetTevAlphaIn(0, 1, 2, 4, 7);
    GXSetTevColorOp(0, 0, 0, 0, 1, 0);
    GXSetTevAlphaOp(0, 0, 0, 0, 1, 0);
    GXSetTevOrder(1, 0xFF, 0xFF, 4);
    GXSetTevColorIn(1, 15, 0, 10, 15);
    GXSetTevAlphaIn(1, 7, 0, 5, 7);
    GXSetTevColorOp(1, 0, 0, 0, 1, 0);
    GXSetTevAlphaOp(1, 0, 0, 0, 1, 0);
    SetupVertexFormat();
}

}  // namespace ut
}  // namespace nw4r
