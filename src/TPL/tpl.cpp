/* TPL/tpl.cpp - the texture palette loader (`TPLBind`, `TPLGet`).
 * RANGE. .text 0x804E45B0-0x804E46F0 (2 functions); .data 0x8062AB68-0x8062AB98; .sdata 0x80794148-0x80794150.
 *   Edges: the `TPL.c` __FILE__ string (.sdata 0x80794148) and the "invalid version number for texture palette"
 *   string (.data 0x8062AB68) are read only by TPLBind; 0x804E46F0 starts the USB print helper that reads
 *   "USB: ".
 * FLAGS. the `OS` lib block of configure.py (`cflags_os`, the compiler the former `WPAD/wpad.cpp` block used); unmeasured until bodies exist.
 * NAMES. `TPLBind` and `TPLGet` are the map's names.
 * RESIDUALS. both bodies unwritten (stub).
 */
