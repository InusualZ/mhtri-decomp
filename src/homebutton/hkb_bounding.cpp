/* homebutton/hkb_bounding.cpp - the software keyboard layout engine's bounding pane: a pane subclass whose class table
 *   (.data 0x8064A800, 27 slots) overrides only its destructor and two slots of the pane table.
 * RANGE. .text 0x80530680-0x8053072C (5 functions); .ctors 0x8056F3F0-0x8056F3F4; .data 0x8064A800-0x8064A878;
 *   .sbss 0x80795A40-0x80795A48.
 *   Edges: the left edge follows homebutton/hkb_window.cpp's static initialiser 0x80530674; the right edge is this
 *   unit's static initialiser 0x80530720 (.ctors word 0x8056F3F0).
 * FLAGS. GC/3.0a5.2 + cflags_hkb, the band's flags (evidence: src/homebutton/hbm_kb_event.cpp FLAGS; unmeasured here until bodies exist).
 * NAMES. GUESS: the file name, from a pane subclass that adds no slot; every function keeps the map's generated name.
 * RESIDUALS. every body unwritten (stub).
 */
