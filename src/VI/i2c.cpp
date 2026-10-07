/* VI/i2c.cpp - the bit-banged I2C writer the video encoder is programmed through (`WaitMicroTime`, `sendSlaveAddr`, `__VISendI2CData`).
 * RANGE. .text 0x804E91C0-0x804E9AE0 (3 functions); .sdata 0x80794180-0x80794188; .sbss 0x80795688-0x80795690.
 *   Edges: the 8-byte .sdata object 0x80794180 and the .sbss word 0x80795688 are read only here; the callers are the
 *   vi3in1 functions (0x804E9AE0..) and none of vi.c.
 * FLAGS. the `OS` lib block of configure.py (`cflags_os`, the compiler the former `WPAD/wpad.cpp` block used); unmeasured until bodies exist.
 * NAMES. all three are the map's names.
 * RESIDUALS. every body unwritten (stub).
 */
