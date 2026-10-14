/* homebutton/hkb_sign_keys.cpp - the software keyboard's sign/symbol key tables: the static initialiser 0x8055FB70
 *   that fills the key records at .data 0x80655630 with the B_SGNkey_* names.
 * RANGE. .text 0x8055FB70-0x8055FD58 (1 function); .ctors 0x8056F41C-0x8056F420; .data 0x80655630-0x80655BD8;
 *   .sdata 0x80794658-0x80794668 (the wide string at 0x80794658 is pointed to by the key record .data 0x80655A68).
 *   Edges: the left edge follows homebutton/hkb_cellphone_keys.cpp's static initialiser 0x8055F728; the right edge is
 *   this unit's static initialiser (.ctors word 0x8056F41C).
 * FLAGS. GC/3.0a5.2 + cflags_hkb, the band's flags (evidence: src/homebutton/hbm_kb_event.cpp FLAGS; unmeasured here until bodies exist).
 * NAMES. GUESS: the file name, from the B_SGNkey_* strings; the function keeps the map's generated name.
 * RESIDUALS. the body unwritten (stub).
 */
