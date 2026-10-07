/*
 * OS/OSUtf.c - the OS Unicode conversion: UTF-8/UTF-16 to UTF-32 and UTF-32 to Shift-JIS.
 * RANGE. .text 0x804D5420-0x804D5670 (4 functions); .data 0x8061D6D8-0x80629518.  Cut from the OS core band
 *    0x804C1760-0x804D9B4C.  Evidence: the 0xBE40-byte conversion tables (.data 0x8061D6D8..0x80629518) are read
 *    only by `fn_804D55B0` and `OSUTF32toSJIS`.
 * FLAGS. `cflags_os` (configure.py), the flags of the unit it was cut from; unchanged by the cut and not measured
 *    with bodies yet.
 * NAMES. `OSUTF8to32`, `OSUTF16to32` and `OSUTF32toSJIS` are the map's names.
 * RESIDUALS. every body is unwritten.
 * SHAPES. none yet.
 */
