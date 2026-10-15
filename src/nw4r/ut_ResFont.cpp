/*
 * nw4r/ut_ResFont.cpp - nw4r::ut::ResFont, the BRFNT-backed font: attaching an image and resolving its offsets.
 * RANGE. .text 0x80503010-0x80503314 (4 functions); .data 0x8062FBE8-0x8062FC48 (the ResFont vtable).
 * RANGE. The `.data` emission-order seam at 0x8062FBE8 opens the file; its `.text` ends with Rebuild, before
 *   CharWriter's constructor.
 * FLAGS. the `nw4r` lib block: GC/3.0a5.2 with `cflags_nw4r` (`cflags_os` + `-fp_contract off`); evidence in docs/nw4r.md.
 * NAMES. File and members from NW4R's ut_ResFont.cpp: the constructor, destructor, SetResource and Rebuild.
 * RESIDUALS. none known.
 * SHAPES. The constructor, destructor and SetResource sit under `#pragma dont_inline` (ResFontBase is another retail
 *   file, so nothing of it inlines here).
 */

#include "nw4r/ut_ResFont.h"

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
