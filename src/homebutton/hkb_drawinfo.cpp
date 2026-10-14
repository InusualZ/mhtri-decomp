/* homebutton/hkb_drawinfo.cpp - the keyboard layout engine's draw information (nw4r::lyt::DrawInfo): the
 *   constructor (identity view matrix, empty view rectangle, unit location scale and alpha, cleared flags) and the
 *   virtual destructor.
 * RANGE. .text 0x805337A0-0x80533854 (2 functions: the constructor, the destructor); .data 0x8064A8E8-0x8064A8F8
 *   (the class table); .sdata2 0x8079D5E8-0x8079D5F0 (0.0, 1.0).
 *   Edges: the class table at .data 0x8064A8E8 holds only the destructor 0x80533814 and is stored by the
 *   constructor 0x805337A0; the next table (0x8064A8F8, homebutton/hkb_animation.cpp) holds higher addresses, a
 *   zigzag `.data` seam.  The float 1.0 is pooled twice (0x8079D5EC here, 0x8079D5F8 read by 0x8053393C), so
 *   0x80533854 starts the next TU.
 * FLAGS. GC/3.0a5.2 + cflags_hkb, the band's flags (evidence: src/homebutton/hbm_kb_event.cpp FLAGS).
 * NAMES. GUESS: nw4r::lyt::DrawInfo for the class and its fields (a layout's draw state: the 3x4 view matrix the
 *   constructor sets to identity, a four-float rectangle, a two-float scale and an alpha defaulting to 1.0, a flag
 *   byte); the constructor and destructor rows take the compiler's manglings.
 * RESIDUALS. none once the map rows are renamed (request hkb-a#7); the class table is 0xC B against the claimed
 *   0x10 B (the trailing zero pad word).
 */
#include "homebutton/hkb_drawinfo.h"

#include "MTX/mtx.h"
#include "Runtime.PPCEABI.H/memset.h"

namespace nw4r {
namespace lyt {

/* 0x805337A0 (0x74): Sets the identity view matrix, the unit scale and alpha, and clears the flags. */
DrawInfo::DrawInfo() {
    mLocationAdjustScale.x = 1.0f;
    mLocationAdjustScale.y = 1.0f;
    mGlobalAlpha = 1.0f;
    memset(&mFlag, 0, sizeof(mFlag));
    PSMTXIdentity(mViewMtx.m);
}

/* 0x80533814 (0x40): Destroys the draw information. */
DrawInfo::~DrawInfo() {}

}  // namespace lyt
}  // namespace nw4r
