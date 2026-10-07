/*
 * OS/OSCache.c - the OS cache control: data/instruction cache and locked-cache ranges, the DMA error handler and
 *    `__OSCacheInit`.
 * RANGE. .text 0x804CC5F0-0x804CCD40 (21 functions); .data 0x8061C158-0x8061C390.  Cut from the OS core band
 *    0x804C1760-0x804D9B4C.  Evidence: .data 0x8061C158..0x8061C390 holds the "L2 INVALIDATE" and DMA error
 *    strings read only by `DMAErrorHandler` and `__OSCacheInit`; the range is the DC/IC/LC accessors around them.
 * FLAGS. `cflags_os` (configure.py), the flags of the unit it was cut from; unchanged by the cut and not measured
 *    with bodies yet.
 * NAMES. map names throughout.
 * RESIDUALS. every body is unwritten.
 * SHAPES. none yet.
 */
