/* homebutton/hkb_textbox.cpp - the software keyboard layout engine's text-box pane: wide-string text drawn through
 *   nw4r::ut's CharWriter/ResFont (SetupGX, SetFontSize, SetColorMapping, GetCharStrmReader).
 * RANGE. .text 0x8052C880-0x8052E0CC (22 functions); .ctors 0x8056F3E8-0x8056F3EC; .data 0x8064A6B0-0x8064A738;
 *   .sdata 0x807944B0-0x807944B8; .sbss 0x80795A30-0x80795A38; .sdata2 0x8079D580-0x8079D598.
 *   Edges: the left edge follows homebutton/hkb_layout.cpp's static initialiser 0x8052C874; the right edge is this
 *   unit's static initialiser 0x8052E0C0 (.ctors word 0x8056F3E8).
 * FLAGS. the `main` lib's `cflags_main` (unmeasured until bodies exist).
 * NAMES. GUESS: the file name, from the font calls; every function keeps the map's generated name.
 * RESIDUALS. every body unwritten (stub).
 */
