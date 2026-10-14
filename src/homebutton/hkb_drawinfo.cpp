/* homebutton/hkb_drawinfo.cpp - the keyboard layout engine's draw-info class: a constructor that loads an identity
 *   matrix and the 0.0/1.0 defaults, and a virtual destructor.
 * RANGE. .text 0x805337A0-0x80533854 (2 functions); .data 0x8064A8E8-0x8064A8F8; .sdata2 0x8079D5E8-0x8079D5F0.
 *   Edges: the class table at .data 0x8064A8E8 holds only the destructor 0x80533814 and is stored by the
 *   constructor 0x805337A0; the next table (0x8064A8F8, homebutton/hkb_animation.cpp) holds higher addresses, a
 *   zigzag `.data` seam.  The float 1.0 is pooled twice (0x8079D5EC here, 0x8079D5F8 read by 0x8053393C), so
 *   0x80533854 starts the next TU.
 * FLAGS. GC/3.0a5.2 + cflags_hkb, the band's flags (evidence: src/homebutton/hbm_kb_event.cpp FLAGS; unmeasured here until bodies exist).
 * NAMES. GUESS: the file name, from the class shape; both functions keep the map's generated names.
 * RESIDUALS. every body unwritten (stub).
 */
