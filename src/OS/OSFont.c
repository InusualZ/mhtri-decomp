/*
 * OS/OSFont.c - the OS font: the code tables, the font encoding and the glyph decoders.
 * RANGE. .text 0x804CFF80-0x804D0C70 (8 functions); .data 0x8061C8C0-0x8061D3D0; .sdata 0x80793FB0-0x80793FB8; .sbss
 *    0x80795348-0x80795358; .sdata2 0x8079D318-0x8079D320.  Cut from the OS core band 0x804C1760-0x804D9B4C.
 *    Evidence: .data 0x8061C8C0..0x8061D3D0 (the 0xB10-byte code tables, read by `fn_804CFF80`), the font state
 *    (.sdata 0x80793FB0, .sbss 0x80795348..0x80795358) and .sdata2 0x8079D318/0x8079D31C are read only by these
 *    functions.
 * FLAGS. `cflags_os` (configure.py), the flags of the unit it was cut from; unchanged by the cut and not measured
 *    with bodies yet.
 * NAMES. `OSSetFontEncode` is the map's name.
 * RESIDUALS. every body is unwritten.
 * SHAPES. none yet.
 */
