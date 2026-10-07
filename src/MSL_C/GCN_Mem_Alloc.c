/*
 * MSL_C/GCN_Mem_Alloc.c - the system allocation hook of the GameCube/Wii MSL port (`__sys_free`: heap-arena
 *    initialisation and `OSFreeToHeap`).
 *
 * RANGE. .text 0x80458BDC..0x80458C94 (1 functions in the map, 0xB8 B); .rodata 0x805724B8..0x80572528.
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi` (the group of the unit it was cut from; unmeasured until
 *    bodies exist).
 * NAMES. file name is the string's own (`GCN_Mem_Alloc.c`).
 * EVIDENCE. its only `.rodata` string reads `GCN_Mem_Alloc.c : InitDefaultHeap. No Heap Available`, the original
 *    `__FILE__`-style name, and only `__sys_free` reads it.
 * RESIDUALS. no body written; the allocator proper starts at the next unit.
 * SHAPES. none yet: no body is written, so nothing is measured; the map rows of the range are the work list
 *    (`ledger.py unit MSL_C/GCN_Mem_Alloc.c`).
 */
