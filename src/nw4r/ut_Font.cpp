/*
 * nw4r/ut_Font.cpp - nw4r::ut::Font, the font interface: the reader selection and the destructor that keys its vtable.
 * RANGE. .text 0x80502828-0x80502910 (2 functions); .data 0x8062FAF0-0x8062FB80 (the four reader member-pointer
 *   constants and the Font vtable).
 * RANGE. The edges with ut_LockedCache before and ut_ResFontBase after are the class runs in link order; the `.data`
 *   emission order (constants, then the vtable) closes at 0x8062FB80, where ut_ResFontBase's constant starts.
 * FLAGS. the `nw4r` lib block: GC/3.0a5.2 with `cflags_nw4r` (`cflags_os` + `-fp_contract off`); evidence in docs/nw4r.md.
 * NAMES. File and members from NW4R's ut_Font.cpp/ut_Font.h: Font::InitReaderFunc and Font::~Font.
 * RESIDUALS. none known.
 * SHAPES. Font::~Font is defined out of line here (`NW4R_UT_FONT_EMIT_DTOR`) and inline for every other file: the
 *   target carries the destructor and the vtable in this file while ResFontBase's and RomFont's destructors inline the
 *   empty body, and a file with only InitReaderFunc emits neither. The other files' weak copies are dropped by the link.
 */

#define NW4R_UT_FONT_EMIT_DTOR
#include "nw4r/ut_Font.h"

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

/* 0x805028D0 (0x40): destroys a font. */
Font::~Font() {}

}  // namespace ut
}  // namespace nw4r
