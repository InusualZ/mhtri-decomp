/* VI/vi3in1.cpp - the video encoder register programming of `vi3in1.c`: CGMS, closed caption, trap filter and RGB mode.
 * RANGE. .text 0x804E9AE0-0x804EB22C (15 functions); .data 0x8062B920-0x8062BF30; .bss 0x8075B280-0x8075B2A0;
 *   .sdata 0x80794188-0x80794198; .sbss 0x80795690-0x807956A8.
 *   Edges: the `vi3in1.c` __FILE__ string sits inside the one 0x610-byte .data object at 0x8062B920 that the
 *   functions 0x804E9D00..0x804EAD80 read; its flag bytes (.sdata 0x8079418C-0x80794197) are
 *   read only here; the left edge is the i2c unit and the right edge 0x804EB22C is not 16-aligned (the next TU
 *   starts a 4-aligned function).  `__VISetCGMS`, `__VISetClosedCaption`, `__VISetTrapFilter`, `VISetTrapFilter` and
 *   `__VISetRGBModeImm` are the map's names for entry points the vi.c setup function calls.
 * FLAGS. the `OS` lib block of configure.py (`cflags_os`, the compiler the former `WPAD/wpad.cpp` block used); unmeasured until bodies exist.
 * NAMES. the map's names above; the other functions are generated.
 * RESIDUALS. every body unwritten (stub).
 */
