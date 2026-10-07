/*
 * MTX/mtx.c - the MTX library 3x4 matrix routines: identity, copy, concat, inverse, rotation, translation, scale and
 *    quaternion conversion.
 * RANGE. .text 0x804C5C10-0x804C68A0 (19 functions); .sdata 0x80793F00-0x80793F08; .sdata2 0x8079D278-0x8079D298.
 *    Cut from the OS core band 0x804C1760-0x804D9B4C.  Evidence: the dump names `PSMTXIdentity`..`PSMTXQuat`; the
 *    range owns .sdata 0x80793F00 (read by `PSMTXConcat`/`PSMTXConcatArray`) and .sdata2 0x8079D278..0x8079D298
 *    (read by the rotation/translate/scale/quaternion bodies) and its vector helpers call into `PSVEC*` at
 *    0x804C6B60; `PSMTXMultVec` (0x804C68A0) is the registered `MTX/mtxvec.c`.
 * FLAGS. `cflags_os` (configure.py), the flags of the unit it was cut from; unchanged by the cut and not measured
 *    with bodies yet.
 * NAMES. all `PSMTX*` rows are the map's names; the remaining `fn_` rows keep their stems.
 * RESIDUALS. paired-single bodies (`psq_st`/`psq_l`) are not reachable from this front end; every body is unwritten.
 * SHAPES. none yet.
 */
