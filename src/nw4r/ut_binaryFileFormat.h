/*
 * nw4r/ut_binaryFileFormat.h - nw4r::ut's binary resource file and block headers and the header check
 *   `nw4r/ut_binaryFileFormat.cpp` defines.
 */
#ifndef NW4R_UT_BINARYFILEFORMAT_H
#define NW4R_UT_BINARYFILEFORMAT_H

#include "types.h"

#ifdef __cplusplus
namespace nw4r {
namespace ut {

/* The common header of an nw4r binary resource file. size: 0x10 */
struct BinaryFileHeader {
    /* +0x0 */ u32 signature;
    /* +0x4 */ u16 byteOrder;
    /* +0x6 */ u16 version;
    /* +0x8 */ u32 fileSize;
    /* +0xC */ u16 headerSize;
    /* +0xE */ u16 dataBlocks;
};

/* The header of one data block inside a binary resource file. size: 0x8 */
struct BinaryBlockHeader {
    /* +0x0 */ u32 kind;
    /* +0x4 */ u32 size;
};

bool IsValidBinaryFile(const BinaryFileHeader* header, u32 signature, u16 version, u16 minBlocks);

}  // namespace ut
}  // namespace nw4r
#endif

#endif
