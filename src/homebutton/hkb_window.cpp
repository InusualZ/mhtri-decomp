/* homebutton/hkb_window.cpp - the software keyboard layout engine's window pane: a pane subclass whose class table
 *   (.data 0x8064A774, 33 slots) adds six slots to the 27 of the pane table, and whose frame draws (0x8052E98C,
 *   0x8052EF6C, 0x8052F6B0) read the .data table 0x8064A738 and call the quad helpers of homebutton/hkb_keyboard.cpp.
 * RANGE. .text 0x8052E0CC-0x80530680 (21 functions); .ctors 0x8056F3EC-0x8056F3F0; .data 0x8064A738-0x8064A800;
 *   .sbss 0x80795A38-0x80795A40; .sdata2 0x8079D598-0x8079D5B0.
 *   Edges: the left edge follows homebutton/hkb_textbox.cpp's static initialiser 0x8052E0C0; the right edge is this
 *   unit's static initialiser 0x80530674 (.ctors word 0x8056F3EC).
 * FLAGS. GC/3.0a5.2 + cflags_hkb, the band's flags (evidence: src/homebutton/hbm_kb_event.cpp FLAGS; unmeasured here until bodies exist).
 * NAMES. GUESS: the file name, from the six added slots and the frame draws; every function keeps the map's
 *   generated name.
 * RESIDUALS. every body unwritten (stub).
 */
