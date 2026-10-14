/* homebutton/hkb_qwerty_keys.cpp - the software keyboard's QWERTY key tables: the static initialiser 0x8055FD58 that
 *   fills the key records at .data 0x80655BD8, the P_key_* name strings and the eight 0x258-byte QWERTY key maps
 *   (rows of wide-character key codes) at .data 0x80655F50..0x80657210.
 * RANGE. .text 0x8055FD58-0x8056083C (1 function); .ctors 0x8056F420-0x8056F424; .data 0x80655BD8-0x80657210.
 *   Edges: the key maps are read only through homebutton/hkb_keyboard.cpp's .rodata pointer tables 0x8057A690 and
 *   0x8057A730; in `.data` order they sit between this unit's records and homebutton/hbm_kb_event.cpp's first
 *   object (0x80657210), so they close this unit's claim.
 * FLAGS. GC/3.0a5.2 + cflags_hkb, the band's flags (evidence: src/homebutton/hbm_kb_event.cpp FLAGS; unmeasured here until bodies exist).
 * NAMES. GUESS: the file name, from the P_key_* strings and the key maps; the function keeps the map's generated name.
 * RESIDUALS. the body unwritten (stub).
 */
