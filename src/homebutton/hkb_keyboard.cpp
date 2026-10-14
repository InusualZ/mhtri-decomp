/* homebutton/hkb_keyboard.cpp - the software keyboard's layout helpers and key tables: name compares, quad/line
 *   drawing for the layout panes, the JP/US/EU key-change and keypad tables (N_KeyChange_*, P_Gkey_*) and the
 *   keyboard classes that walk them, ending in the static initialiser that writes the pointer table at .data
 *   0x8064B180.
 * RANGE. .text 0x805352DC-0x8053E808 (145 functions); .ctors 0x8056F3F4-0x8056F3F8; .rodata 0x80579450-0x8057AB98;
 *   .data 0x8064A950-0x8064DE88; .bss 0x807901C0-0x807901F0; .sdata 0x807944C0-0x80794568;
 *   .sbss 0x80795A48-0x80795A50; .sdata2 0x8079D610-0x8079D620.
 *   Edges: the left edge follows the archive resource accessor's last method (homebutton/hkb_arc_resource_accessor.cpp
 *   ends at 0x805352DC); the float 0.0 is pooled again at 0x8079D610 (read by 0x80535454), so the `.sdata2` pool of
 *   homebutton/hkb_animation.cpp ends before it.  The right edge is the static initialiser 0x8053E5F8 (.ctors word
 *   0x8056F3F4) and its thunks to 0x8053E808.  The jump tables .data 0x8064D9D4..0x8064DB18 are one run: the class
 *   tables 0x8064DB54..0x8064DBD8 after them hold 0x8053DF20, 0x8053DD64, 0x8053D8D4 and 0x8053D64C in reverse
 *   order, so no seam sits at 0x8053D8A0.
 * FLAGS. GC/3.0a5.2 + cflags_hkb, the band's flags (evidence: src/homebutton/hbm_kb_event.cpp FLAGS; unmeasured here until bodies exist).
 * NAMES. GUESS: the file name, from the key tables; every function keeps the map's generated name.
 * RESIDUALS. every body unwritten (stub).  0x805352DC..0x80535E3C (layout helpers called from the pane, text box and
 *   window units) may be a TU of its own: no data object, pool duplicate or class table separates it from the key
 *   tables, so it stays here until a body measures it.
 */
