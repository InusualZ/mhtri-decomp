/*
 * nw4r/ut_CharStrmReader.cpp - nw4r::ut::CharStrmReader's UTF-8, UTF-16, CP1252 and Shift-JIS decoders.
 * RANGE. .text 0x80501E98-0x80501FA8 (4 functions); no data.
 * RANGE. The edges with the neighbouring nw4r files (math_types .. ut_LockedCache, 0x805012C4-0x80502828) are the
 *   namespace/class runs in link order; each file pools its own `.sdata2` constants (0.0f once per file).
 * FLAGS. the `nw4r` lib block: GC/3.0a5.2 with `cflags_nw4r` (`cflags_os` + `-fp_contract off`); evidence in docs/nw4r.md.
 * NAMES. File from NW4R's ut_CharStrmReader.cpp; the map carries the member manglings.
 * SHAPES. GetChar/StepStrm pun mCharStrm through a reference so each access reloads it.
 */

#include "nw4r/ut_CharStrmReader.h"

namespace nw4r {
namespace ut {

/* Whether `c` opens a two-byte Shift-JIS character. */
static inline bool IsSJISLeadByte(u8 c) {
    return (c >= 0x81 && c < 0xA0) || c >= 0xE0;
}

/* 0x80501E98 (0x78): decodes one UTF-8 character (up to three bytes). */
u16 CharStrmReader::ReadNextCharUTF8() {
    u16 code;

    if (!(GetChar<u8>(0) & 0x80)) {
        code = GetChar<u8>(0);
        StepStrm<u8>(1);
    } else if ((GetChar<u8>(0) & 0xE0) == 0xC0) {
        code = ((GetChar<u8>(0) & 0x1F) << 6) | (GetChar<u8>(1) & 0x3F);
        StepStrm<u8>(2);
    } else {
        code = ((GetChar<u8>(0) & 0x1F) << 12) | ((GetChar<u8>(1) & 0x3F) << 6) | (GetChar<u8>(2) & 0x3F);
        StepStrm<u8>(3);
    }
    return code;
}

/* 0x80501F10 (0x1C): reads one UTF-16 code unit. */
u16 CharStrmReader::ReadNextCharUTF16() {
    u16 code = GetChar<u16>(0);

    StepStrm<u16>(1);
    return code;
}

/* 0x80501F2C (0x1C): reads one CP1252 byte. */
u16 CharStrmReader::ReadNextCharCP1252() {
    u16 code = GetChar<u8>(0);

    StepStrm<u8>(1);
    return code;
}

/* 0x80501F48 (0x60): decodes one Shift-JIS character (one or two bytes). */
u16 CharStrmReader::ReadNextCharSJIS() {
    u16 code;

    if (IsSJISLeadByte(GetChar<u8>(0))) {
        code = (GetChar<u8>(0) << 8) | GetChar<u8>(1);
        StepStrm<u8>(2);
    } else {
        code = GetChar<u8>(0);
        StepStrm<u8>(1);
    }
    return code;
}

}  // namespace ut
}  // namespace nw4r
