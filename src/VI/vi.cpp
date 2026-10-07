/* VI/vi.cpp - the video interface library: retrace handling, mode configuration, framebuffer flush and the TV getters.
 * RANGE. .text 0x804E6710-0x804E91C0 (28 functions); .data 0x8062B3C8-0x8062B920; .bss 0x8075B110-0x8075B280;
 *   .sdata 0x80794160-0x80794180; .sbss 0x807955D8-0x80795688.
 *   Edges: the `vi.c` __FILE__ string (.sdata 0x80794178) and the "<< RVL_SDK - VI ... (0x4302_145) >>" build
 *   string (.data 0x8062B3C8) are read by the function 0x804E7F60 (it calls setFbbRegs and setVerticalRegs); the TU's state (.bss 0x8075B110/0x8075B188/
 *   0x8075B200/0x8075B258, .sbss 0x80795608..0x80795684) is read only from 0x804E6710..0x804E91B0.  The right edge
 *   0x804E91C0 starts `WaitMicroTime` (i2c).  The one data object read from both sides of this range is the setup
 *   function 0x804E68B0 reading the vi3in1 global at .sbss 0x80795690 (an extern).
 * FLAGS. the `OS` lib block of configure.py (`cflags_os`, the compiler the former `WPAD/wpad.cpp` block used); unmeasured until bodies exist.
 * NAMES. the map's names (`VIWaitForRetrace`, `VIFlush`, `VISetNextFrameBuffer`, `VISetBlack`, `VIGetNextField`,
 *   `VIGetCurrentLine`, `VIGetTvFormat`, `VIGetDTVStatus`, `__VIDisplayPositionToXY`, `VIEnableDimming`,
 *   `VIResetDimmingCount`, `setFbbRegs`, `setVerticalRegs`); the rest are generated.
 * RESIDUALS. every body unwritten (stub).
 */
