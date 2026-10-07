/*
 * OS/OSAlloc.c - the OS heap allocator: free-list insert, allocate/free, heap creation and the current-heap setters.
 * RANGE. .text 0x804CBD10-0x804CC030 (6 functions); .sdata 0x80793F80-0x80793F88; .sbss 0x80795318-0x80795328.  Cut
 *    from the OS core band 0x804C1760-0x804D9B4C.  Evidence: .sbss 0x80795318..0x80795328 and .sdata 0x80793F80
 *    are read only by this range; `OSFreeToHeap` calls `DLInsert` (0x804CBD10), the first function.
 * FLAGS. `cflags_os` (configure.py), the flags of the unit it was cut from; unchanged by the cut and not measured
 *    with bodies yet.
 * NAMES. `OSFreeToHeap` is the map's name; `glx_SetLoadCallback` (0x804CBF40, a 16-byte setter of the .sdata word)
 *    keeps the map's stem.
 * RESIDUALS. every body is unwritten.
 * SHAPES. none yet.
 */
