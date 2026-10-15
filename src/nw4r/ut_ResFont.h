/*
 * nw4r/ut_ResFont.h - nw4r::ut::ResFont, the BRFNT-backed font `nw4r/ut_ResFont.cpp` defines.
 */
#ifndef NW4R_UT_RESFONT_H
#define NW4R_UT_RESFONT_H

#include "types.h"
#include "nw4r/ut_ResFontBase.h"
#include "nw4r/ut_binaryFileFormat.h"

#ifdef __cplusplus
namespace nw4r {
namespace ut {

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
