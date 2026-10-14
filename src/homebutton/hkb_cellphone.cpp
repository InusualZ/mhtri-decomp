/* homebutton/hkb_cellphone.cpp - the software keyboard's cellphone-keypad screen: the B_CPkey_*, W_ChngTag_* and
 *   B_SGNkey_* button tables, the predictive-input mode buttons and the keypad classes that walk them, ending in the
 *   static initialiser that writes the W_* name pointers into the tables .data 0x8064E0B8 and 0x8064E280.
 * RANGE. .text 0x8053E808-0x805425E4 (79 functions); .ctors 0x8056F3F8-0x8056F3FC; .rodata 0x8057AB98-0x8057B098;
 *   .data 0x8064DE88-0x8064E760; .sdata 0x80794568-0x807945C8; .sdata2 0x8079D620-0x8079D628.
 *   Edges: the left edge follows the seven 8-byte slots 0x8053E7D0..0x8053E800 of homebutton/hkb_keyboard.cpp's class
 *   table .data 0x8064DC30; the right edge is the static initialiser 0x8054254C (.ctors word 0x8056F3F8) and its
 *   thunks to 0x805425E4.
 *   One TU: the zigzag `.data` seam 0x8064E650 (tables 0x8064E51C -> 0x8064E650, .text interval
 *   0x8053F65C..0x8053F7BC) is contradicted by three private objects read on both sides of it - .rodata 0x8057AFA8
 *   (0x8053E808, 0x8053E8B8, 0x8053FCA8), .rodata 0x8057AB98 (0x8053E968, 0x8053F7BC, 0x805404D8) and the .sdata2
 *   byte 0x8079D620 (0x8053F22C, 0x80540684).
 * FLAGS. the `main` lib's `cflags_main` (unmeasured until bodies exist).
 * NAMES. GUESS: the file name, from the B_CPkey_* tables; every function keeps the map's generated name.
 * RESIDUALS. every body unwritten (stub).
 */
