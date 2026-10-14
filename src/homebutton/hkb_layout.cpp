/* homebutton/hkb_layout.cpp - the software keyboard layout engine's layout class: it walks a layout resource's blocks
 *   (the 'pan1'/'pic1'/'txt1'/'grp1' magics compared at 0x8052B7A0..0x8052C148) and builds the pane tree.
 * RANGE. .text 0x8052B004-0x8052C880 (34 functions); .ctors 0x8056F3E4-0x8056F3E8; .data 0x8064A5E8-0x8064A6B0;
 *   .sbss 0x80795A28-0x80795A30; .sdata2 0x8079D570-0x8079D580.
 *   Edges: the left edge follows homebutton/fn_8052A040.cpp's static initialiser 0x8052AFF8 (.ctors word 0x8056F3E0);
 *   the right edge is this unit's static initialiser 0x8052C874 (.ctors word 0x8056F3E4).
 * FLAGS. the `main` lib's `cflags_main` (unmeasured until bodies exist).
 * NAMES. GUESS: the file name, from the block magics; every function keeps the map's generated name.
 * RESIDUALS. every body unwritten (stub).
 */
