/*
 * MEM/mem_heap.c - the MEM library heap core: the heap-list lookup (`FindContainHeap_`), the heap-head initialiser
 *    and finaliser.
 * RANGE. .text 0x804C1760-0x804C1BD0 (3 functions); .bss 0x80748B90-0x80748BB8; .sbss 0x80795298-0x807952A0.  Cut
 *    from the OS core band 0x804C1760-0x804D9B4C.  Evidence: the only readers of the heap list (.bss 0x80748B90
 *    and its OSMutex 0x80748BA0) and of the "list initialised" flag (.sbss 0x80795298) are `MEMiInitHeapHead` and
 *    the finaliser; the expandable-heap functions after 0x804C1BD0 read none of them.
 * FLAGS. `cflags_base` per object (configure.py): the target's function starts are all 16-aligned.
 * NAMES. GUESS: `MEMiRootHeapList`, `MEMiRootHeapMutex`, `MEMiRootHeapListReady` (the heap-list data).`FindContainHeap_` and `MEMiInitHeapHead` are the map's (dump) names; GUESS: `MEMiFinalizeHeap` (the counterpart of
 *    `MEMiInitHeapHead` that `MEMDestroyExpHeap` calls) and the file name.
 * RESIDUALS. `MEMiInitHeapHead` and `MEMiFinalizeHeap`: the target holds three inlined search levels plus the call; ours
 *    holds one level plus the call, whatever the inline depth (2 or 3) or the pragma position; the saves differ with it.
 *    Relocation difference: each of the two lacks the target's `_savegpr_27`/`_restgpr_27` pair and two
 *    `MEMGetNextListObject` calls (the inlined search levels).
 *    .sbss: the target holds 8 B at 0x80795298 (flag + a word nothing here reads), ours 4 B.
 *    `FindContainHeap_` needs `inline_depth(2)` (three loops) and the `inline` keyword to be inlined into its callers.
 * SHAPES. none yet.
 */

#include "types.h"

#include "MEM/mem.h"
#include "MEM/mem_heap.h"
#include "MEM/mem_list.h"
#include "OS/OSMutex.h"

static MEMList MEMiRootHeapList;      /* .bss 0x80748B90: list of the top-level heaps */
static OSMutex MEMiRootHeapMutex;     /* .bss 0x80748BA0: guards the heap list */
static u32 MEMiRootHeapListReady;     /* .sbss 0x80795298 */

#pragma inline_depth(2)

/* Returns the innermost heap of `list` whose range holds `block`, or NULL. */
/* untyped: byte range - the address searched for */
inline MEMiHeapHead* FindContainHeap_(MEMList* list, const void* block)
{
    MEMiHeapHead* container = NULL;
    MEMiHeapHead* heap = NULL;

    while ((heap = (MEMiHeapHead*)MEMGetNextListObject(list, heap)) != NULL) {
        if ((u32)heap->start <= (u32)block && (u32)block < (u32)heap->end) {
            container = FindContainHeap_(&heap->children, block);
            if (container != NULL)
                return container;
            return heap;
        }
    }
    return NULL;
}

/* Initialises a heap head and links it into the heap list of the heap that contains it. */
/* untyped: byte range - the heap's start and end addresses */
void MEMiInitHeapHead(MEMiHeapHead* heap, u32 signature, void* start, void* end, u16 attribute)
{
    MEMList* list;
    MEMiHeapHead* parent;

    heap->signature = signature;
    heap->start = start;
    heap->end = end;
    heap->attribute.value = 0;
    heap->attribute.fields.optFlag = attribute;
    MEMInitList(&heap->children, 4);

    if (MEMiRootHeapListReady == 0) {
        MEMInitList(&MEMiRootHeapList, 4);
        OSInitMutex(&MEMiRootHeapMutex);
        MEMiRootHeapListReady = 1;
    }

    OSInitMutex(heap->mutex);
    OSLockMutex(&MEMiRootHeapMutex);
    list = &MEMiRootHeapList;
    parent = FindContainHeap_(&MEMiRootHeapList, heap);
    if (parent != NULL)
        list = &parent->children;
    MEMAppendListObject(list, heap);
    OSUnlockMutex(&MEMiRootHeapMutex);
}

/* Unlinks a heap head from its parent's list and clears its signature. */
void MEMiFinalizeHeap(MEMiHeapHead* heap)
{
    MEMList* list;
    MEMiHeapHead* parent;

    OSLockMutex(&MEMiRootHeapMutex);
    list = &MEMiRootHeapList;
    parent = FindContainHeap_(&MEMiRootHeapList, heap);
    if (parent != NULL)
        list = &parent->children;
    MEMRemoveListObject(list, heap);
    OSUnlockMutex(&MEMiRootHeapMutex);
    heap->signature = 0;
}
