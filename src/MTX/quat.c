/*
 * MTX/quat.c - the MTX library quaternion routines `QUATMtx` and `QUATSlerp`.
 * RANGE. .text 0x804C6D70-0x804C70E0 (2 functions); .rodata 0x80573A00-0x80573A10; .sdata2 0x8079D2C0-0x8079D2D0.
 *    Cut from the OS core band 0x804C1760-0x804D9B4C.  Evidence: the map names `QUATMtx` and `QUATSlerp`; they
 *    read the 16-byte .rodata table at 0x80573A00 and the four floats at .sdata2 0x8079D2C0..0x8079D2D0, and no
 *    NAND code does; `NANDPrivateCreate` (0x804C70E0) is the first NAND function.
 * FLAGS. `cflags_os` (configure.py), the flags of the unit it was cut from; unchanged by the cut and not measured
 *    with bodies yet.
 * NAMES. `QUATMtx` and `QUATSlerp` are the map's names; the file name is a GUESS from them.
 * RESIDUALS. every body is unwritten (paired-single).
 * SHAPES. none yet.
 */
