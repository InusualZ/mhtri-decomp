/*
 * Runtime.PPCEABI.H/ExceptionPPC.cp - the C++ throw machinery that follows the Gecko fragment walkers: unwind and
 *    destroy helpers, `__unexpected`, `ExPPC_ThrowHandler`, `__throw`, and the `bad_exception` name record.
 *
 * RANGE. extab 0x8001E400..0x8001E464; extabindex 0x8003F18C..0x8003F1C8; .text 0x804578FC..0x80458BDC (9
 *    functions in the map, 0x12E0 B); .rodata 0x80572450..0x805724B8; .data 0x8060E8E4..0x8060E958; .sdata
 *    0x80793CD0..0x80793CD8.
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi` (the group of the unit it was cut from; unmeasured until
 *    bodies exist).
 * NAMES. file name GUESS: the symbols (`__unexpected`, `ExPPC_ThrowHandler`, `__throw`) are the Metrowerks
 *    `ExceptionPPC` runtime, the older sibling of `Gecko_ExceptionPPC.cp`; the `.cp` extension follows that unit.
 * EVIDENCE. all five extab/extabindex records of the old `MSL_C/alloc.cpp` span lie in it; the jump table `.data`
 *    0x8060E8E4 is read by its second function (the first Gecko unit owns the table before it, which its matched
 *    object proves); the `.rodata` strings `std::bad_exception` / `!bad_exception!!` are read by `__unexpected`
 *    and the 12-byte function that ends it; the span ends where `__sys_free`'s `GCN_Mem_Alloc.c` message string
 *    is read.
 * RESIDUALS. unit is C++ with its own extab, so it needs `#pragma exceptions on` like the Gecko unit (unmeasured);
 *    the Gecko header's claim that no `__throw` exists in this DOL is stale (the map names it at 0x80458A60).
 * SHAPES. none yet: no body is written, so nothing is measured; the map rows of the range are the work list
 *    (`ledger.py unit Runtime.PPCEABI.H/ExceptionPPC.cp`).
 */
