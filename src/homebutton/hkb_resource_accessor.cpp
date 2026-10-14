/* homebutton/hkb_resource_accessor.cpp - the keyboard layout engine's abstract resource accessor
 *   (nw4r::lyt::ResourceAccessor): destructor, constructor and the font getter that finds no font.
 * RANGE. .text 0x80534D68-0x80534DC0 (3 functions: the destructor, the constructor, GetFont); .data
 *   0x8064A920-0x8064A938 (the class table).
 *   Edges: the class table .data 0x8064A920 holds 0x80534D68, a pure slot and 0x80534DB8, and is stored by the
 *   constructor 0x80534DA8; it sits between the tables of homebutton/hkb_animation.cpp (0x8064A8F8) and
 *   homebutton/hkb_arc_resource_accessor.cpp (0x8064A938), whose owners are lower and higher (two zigzag seams).
 *   The right edge 0x80534DC0 is the archive name lookup, reached only from the archive accessor's methods.
 * FLAGS. GC/3.0a5.2 + cflags_hkb, the band's flags (evidence: src/homebutton/hbm_kb_event.cpp FLAGS).
 * NAMES. GUESS: nw4r::lyt::ResourceAccessor for the class (the table shape - destructor, a pure resource getter
 *   that homebutton/hkb_material.cpp calls with 'timg', a font getter - and the archive accessor that derives from
 *   it); the rows take the compiler's manglings.
 * RESIDUALS. none once the map rows are renamed (request hkb-a#7); the class table is 0x14 B against the claimed
 *   0x18 B (the trailing zero pad word).
 */
#include "homebutton/hkb_resource_accessor.h"

namespace nw4r {
namespace lyt {

/* 0x80534D68 (0x40): Destroys the accessor. */
ResourceAccessor::~ResourceAccessor() {}

/* 0x80534DA8 (0x10): Constructs the accessor. */
ResourceAccessor::ResourceAccessor() {}

/* 0x80534DB8 (0x8): Returns no font; an accessor that holds fonts overrides this. */
ut::Font* ResourceAccessor::GetFont(const char* name) {
    (void)name;
    return NULL;
}

}  // namespace lyt
}  // namespace nw4r
