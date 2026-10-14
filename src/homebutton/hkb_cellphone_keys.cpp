/* homebutton/hkb_cellphone_keys.cpp - the software keyboard's cellphone/kana key tables: the static initialiser
 *   0x8055F728 that fills the key records at .data 0x80652530, the B_*key name strings and the 0x270-byte key-map
 *   objects, each pointed to by a 0x2C-byte .rodata descriptor.
 * RANGE. .text 0x8055F728-0x8055FB70 (1 function); .ctors 0x8056F418-0x8056F41C; .rodata 0x8057BDB0-0x8057C228;
 *   .data 0x80652530-0x80655630.
 *   Edges: the .rodata descriptors run 0x8057BDB0..0x8057C228 in 0x2C steps; the last, 0x8057C1FC, points at the
 *   last 0x270-byte key map .data 0x806553C0 and is read by homebutton/hkb_cellphone.cpp (0x8053E968) through the
 *   same descriptor table (.rodata 0x8057AFA8) that names 0x8057C14C..0x8057C1D0, so both runs end with this unit's
 *   claim, before homebutton/hbm_kb_event.cpp's .rodata 0x8057C228 and homebutton/hkb_sign_keys.cpp's .data 0x80655630.
 * FLAGS. GC/3.0a5.2 + cflags_hkb, the band's flags (evidence: src/homebutton/hbm_kb_event.cpp FLAGS; unmeasured here until bodies exist).
 * NAMES. GUESS: the file name, from the B_CPkey_* strings; the function keeps the map's generated name.
 * RESIDUALS. the body unwritten (stub).
 */
