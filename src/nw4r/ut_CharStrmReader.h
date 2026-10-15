/*
 * nw4r/ut_CharStrmReader.h - nw4r::ut::CharStrmReader, whose decoders `nw4r/ut_CharStrmReader.cpp` defines.
 */
#ifndef NW4R_UT_CHARSTRMREADER_H
#define NW4R_UT_CHARSTRMREADER_H

#include "types.h"

#ifdef __cplusplus
namespace nw4r {
namespace ut {

/* Reads characters out of an encoded string one code point at a time. size: 0x8 */
class CharStrmReader {
public:
    typedef u16 (CharStrmReader::*ReadFunc)();

    CharStrmReader(ReadFunc func) : mCharStrm(NULL), mReadFunc(func) {}

    /* untyped: byte range - the encoded string, read as u8 or u16 by the encoding */
    void Set(const void* stream) {
        mCharStrm = stream;
    }
    /* untyped: byte range - the encoded string, read as u8 or u16 by the encoding */
    const void* GetCurrentPos() const {
        return mCharStrm;
    }
    u16 Next() {
        return (this->*mReadFunc)();
    }

    u16 ReadNextCharUTF8();
    u16 ReadNextCharUTF16();
    u16 ReadNextCharCP1252();
    u16 ReadNextCharSJIS();

    template <typename T> T GetChar(int offset) const {
        return reinterpret_cast<const T* const&>(mCharStrm)[offset];
    }

    template <typename T> void StepStrm(int count) {
        reinterpret_cast<const T*&>(mCharStrm) += count;
    }

    /* untyped: byte range - the encoded string, read as u8 or u16 by the encoding */
    /* +0x0 */ const void* mCharStrm;
    /* +0x4 */ ReadFunc mReadFunc;
};

}  // namespace ut
}  // namespace nw4r
#endif

#endif
