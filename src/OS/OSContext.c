/*
 * OS/OSContext.c - the OS context: FPU save/load, context save/load/clear, fiber switch, `OSDumpContext` and
 *    `__OSContextInit`.
 * RANGE. .text 0x804CCD40-0x804CD620 (15 functions); .data 0x8061C390-0x8061C570.  Cut from the OS core band
 *    0x804C1760-0x804D9B4C.  Evidence: .data 0x8061C390..0x8061C570 (the register-dump format strings and the
 *    "FPU-unavailable handler installed" string) is read only by `OSDumpContext` and `__OSContextInit`.
 * FLAGS. `cflags_os` (configure.py), the flags of the unit it was cut from; unchanged by the cut and not measured
 *    with bodies yet.
 * NAMES. map names throughout.
 * RESIDUALS. every body is unwritten.
 * SHAPES. none yet.
 */
