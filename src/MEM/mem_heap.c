/*
 * MEM/mem_heap.c - the MEM library heap core: the heap-list lookup (`FindContainHeap_`), the heap-head initialiser
 *    and finaliser.
 * RANGE. .text 0x804C1760-0x804C1BD0 (3 functions); .bss 0x80748B90-0x80748BB8; .sbss 0x80795298-0x807952A0.  Cut
 *    from the OS core band 0x804C1760-0x804D9B4C.  Evidence: the only readers of the heap list (.bss 0x80748B90
 *    and its OSMutex 0x80748BA0) and of the "list initialised" flag (.sbss 0x80795298) are `MEMiInitHeapHead` and
 *    the finaliser; the expandable-heap functions after 0x804C1BD0 read none of them.
 * FLAGS. `cflags_os` (configure.py), the flags of the unit it was cut from; unchanged by the cut and not measured
 *    with bodies yet.
 * NAMES. `FindContainHeap_` and `MEMiInitHeapHead` are the map's (dump) names; GUESS: `MEMiFinalizeHeap` (the counterpart of
 *    `MEMiInitHeapHead` that `MEMDestroyExpHeap` calls) and the file name.
 * RESIDUALS. no body is written: `FindContainHeap_`, `MEMiInitHeapHead` and `MEMiFinalizeHeap` are unwritten (the
 *    triple-nested search is the hard row).
 * SHAPES. none yet.
 */

#include "types.h"

#include "MEM/mem.h"
#include "MEM/mem_heap.h"

extern u32 lbl_80795298;      /* 'heap list initialised' flag (.sbss) */
extern MEMList lbl_80748B90;  /* global heap list (.bss) */
extern u8 lbl_80748BA0[];     /* global heap-list OSMutex (.bss) */
