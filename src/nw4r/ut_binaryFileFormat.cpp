/*
 * nw4r/ut_binaryFileFormat.cpp - nw4r::ut's binary resource file header check.
 * RANGE. .text 0x80501E24-0x80501E98 (1 function); no data.
 * RANGE. The edges with the neighbouring nw4r files (math_types .. ut_LockedCache, 0x805012C4-0x80502828) are the
 *   namespace/class runs in link order; each file pools its own `.sdata2` constants (0.0f once per file).
 * FLAGS. the `nw4r` lib block: GC/3.0a5.2 with `cflags_nw4r` (`cflags_os` + `-fp_contract off`); evidence in docs/nw4r.md.
 * NAMES. File from NW4R's ut_binaryFileFormat.cpp; IsValidBinaryFile carries its mangling in the map.
 */

#include "nw4r/ut_binaryFileFormat.h"

namespace nw4r {
namespace ut {

/* 0x80501E24 (0x74): checks a resource file's signature, byte order, version, size and block count. */
bool IsValidBinaryFile(const BinaryFileHeader* header, u32 signature, u16 version, u16 minBlocks) {
    if (header->signature != signature) {
        return false;
    }
    if (header->byteOrder != 0xFEFF) {
        return false;
    }
    if (header->version != version) {
        return false;
    }
    if (header->fileSize < sizeof(BinaryFileHeader) + sizeof(BinaryBlockHeader) * minBlocks) {
        return false;
    }
    if (header->dataBlocks < minBlocks) {
        return false;
    }
    return true;
}

}  // namespace ut
}  // namespace nw4r
