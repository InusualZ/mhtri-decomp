/* homebutton/hkb_bounding.cpp - the software keyboard layout engine's bounding pane (nw4r::lyt::Bounding): a pane
 *   that marks an area and draws nothing; its class table overrides the destructor, GetRuntimeTypeInfo and DrawSelf.
 * RANGE. .text 0x80530680-0x8053072C (5 functions); .ctors 0x8056F3F0-0x8056F3F4; .data 0x8064A800-0x8064A878;
 *   .sbss 0x80795A40-0x80795A48.
 *   Edges: the left edge follows homebutton/hkb_window.cpp's static initialiser 0x80530674; the right edge is this
 *   unit's static initialiser 0x80530720 (.ctors word 0x8056F3F0).
 * FLAGS. GC/3.0a5.2 + cflags_hkb, the band's flags (evidence: src/homebutton/hbm_kb_event.cpp FLAGS).
 * NAMES. GUESS: nw4r::lyt::Bounding, from the shape of NintendoWare's layout library (the runtime dump names the
 *   neighbouring classes nw4hbm::lyt::Pane / Window / TextBox, the HOME Button copy of the library; the band spells
 *   the namespace nw4r::lyt after homebutton/hkb_material.cpp). File name GUESS lyt_bounding.cpp's role.
 * NAMES. The map rows keep their generated names until the hkb-b#1 rename batch lands (the source spells the
 *   manglings); the batch also names the pane base's rows (fn_8052A13C, fn_8052A274 and the inherited slots).
 * RESIDUALS. none.
 */
#include "homebutton/hkb_bounding.h"

namespace nw4r {
namespace lyt {

const ut::detail::RuntimeTypeInfo Bounding::typeInfo(&Pane::typeInfo);

/* 0x80530680 (0x3C): Builds the pane from its `bnd1` block. */
Bounding::Bounding(const res::Bounding* pBlock, const ResBlockSet& resBlockSet) : Pane(pBlock) {}

/* 0x805306BC (0x58): Destroys the pane. */
Bounding::~Bounding() {}

/* 0x80530714 (0x4): Draws nothing: a bounding pane only marks an area. */
void Bounding::DrawSelf(const DrawInfo& drawInfo) {}

/* 0x80530718 (0x8): Returns the class's run-time type record. */
const ut::detail::RuntimeTypeInfo* Bounding::GetRuntimeTypeInfo() const {
    return &typeInfo;
}

}  // namespace lyt
}  // namespace nw4r
