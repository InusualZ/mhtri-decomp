/* homebutton/hkb_material.cpp - the software keyboard layout engine's material: TEV stage, swap-table, colour and
 *   channel setup (GXSetTevOrder/ColorIn/AlphaIn/SwapModeTable/KColor, GXSetChanCtrl) and the texture-SRT matrices
 *   (nw4r::math::SinFIdx).
 * RANGE. .text 0x8053072C-0x80533474 (74 functions); .data 0x8064A878-0x8064A8E8; .sdata2 0x8079D5B0-0x8079D5D0;
 *   .sbss2 0x8079D7F0-0x8079D7F8.
 *   Edges: the left edge follows homebutton/hkb_bounding.cpp's static initialiser 0x80530720; the right edge is the
 *   `.sdata2` pool duplicate 4330000000000000 (0x8079D5C8 here, 0x8079D5D8 in homebutton/hkb_texmap.cpp).
 * FLAGS. the `main` lib's `cflags_main` (unmeasured until bodies exist).
 * NAMES. GUESS: the file name, from the GX calls; every function keeps the map's generated name.
 * RESIDUALS. every body unwritten (stub).
 */
