/* homebutton/hkb_resource_accessor.cpp - the keyboard layout engine's abstract resource-accessor base class:
 *   destructor, constructor and the font getter that returns null.
 * RANGE. .text 0x80534D68-0x80534DC0 (3 functions); .data 0x8064A920-0x8064A938.
 *   Edges: the class table .data 0x8064A920 holds 0x80534D68, a pure slot and 0x80534DB8, and is stored by the
 *   constructor 0x80534DA8; it sits between the tables of homebutton/hkb_animation.cpp (0x8064A8F8) and
 *   homebutton/hkb_arc_resource_accessor.cpp (0x8064A938), whose owners are lower and higher (two zigzag seams).
 *   The right edge 0x80534DC0 is the archive name lookup, reached only from the archive accessor's methods.
 * FLAGS. GC/3.0a5.2 + cflags_hkb, the band's flags (evidence: src/homebutton/hbm_kb_event.cpp FLAGS; unmeasured here until bodies exist).
 * NAMES. GUESS: the file name, from the table shape (destructor, pure resource getter, font getter); every
 *   function keeps the map's generated name.
 * RESIDUALS. every body unwritten (stub).
 */
