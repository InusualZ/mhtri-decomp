/*
 * RVLGX/GXAttr.c - the SDK GX vertex-attribute setup (descriptors, formats, arrays, texcoord generation).
 *
 * RANGE. `.text` 0x804B4E40-0x804B5DB0; `.data` 0x8061A7D0-0x8061A9C0; `.sdata` 0x80793E50-0x80793E60 (13 functions /
 *   0xF24 B).
 *   - six `.data` jump tables 0x8061A7D0..0x8061A9C0 are read by `GXSetVtxDesc`, `GXGetVtxDesc`, the two attribute-
 *     format setters, `GXGetVtxAttrFmt` and `GXSetTexCoordGen2`; `.sdata` 0x80793E50..0x80793E60 by
 *     `__GXCalculateVLim`
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`, `cflags_os`), the block of the unit this range was cut
 *   from; unmeasured until bodies exist.
 * RESIDUALS. the right edge (0x804B5DB0, `GXSetMisc`) lies in a gap with no data evidence (`GXSetNumTexGens` is the
 *   last attribute function; naming order decides). `.data` (0x1F0) and `.sdata` (0x10) are claimed and not
 *   emitted (their readers are unwritten). The pool census reports the GX units as one fold candidate
 *   through a single `.sdata2` word, `__GXData` (0x8079D0F0), an SDK global defined in `RVLGX/GXInit.c` and
 *   not a pooled literal; the value `1.0f` sits at four pool addresses (0x8079D104, 0x8079D140, 0x8079D1A4,
 *   0x8079D1F4) read by four different files. `GXInvalidateVtxCache` written (byte-identical). The other 12
 *   functions are unwritten, the largest `GXSetVtxDesc` at 0x804B4E40 (0x264 B); `python
 *   tools/units/sweepcomments.py --unit RVLGX/GXAttr.c` lists them.
 */

#pragma function_align 16

#include "types.h"
#include "gx.h"

/* Tells the hardware the vertex cache's contents are stale (a write-gather-pipe command). */
void GXInvalidateVtxCache(void)
{
    GXWGFifo.u8 = 0x48;
}
