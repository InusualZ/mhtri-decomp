/* homebutton/hkb_texmap.cpp - the keyboard layout engine's texture-map record: GX texture/TLUT object setup from a
 *   TPL entry (GXInitTexObj/GXInitTexObjCI/GXInitTlutObj, TPLGet).
 * RANGE. .text 0x80533474-0x805337A0 (4 functions); .sdata2 0x8079D5D0-0x8079D5E8.
 *   Edges: the left edge is the `.sdata2` pool duplicate 4330000000000000 (0x8079D5C8 read by
 *   homebutton/hkb_material.cpp, 0x8079D5D8 read by 0x80533474).  The right edge: the pool word 0x8079D5E4 is the
 *   zero pad after the float at 0x8079D5E0 (a TU's `.sdata2` contribution ends 8-aligned), and the next float 0.0
 *   at 0x8079D5E8 belongs to homebutton/hkb_drawinfo.cpp.
 * FLAGS. GC/3.0a5.2 + cflags_hkb, the band's flags (evidence: src/homebutton/hbm_kb_event.cpp FLAGS; unmeasured here until bodies exist).
 * NAMES. GUESS: the file name, from the GX texture and TPL calls; every function keeps the map's generated name.
 * RESIDUALS. every body unwritten (stub).
 */
