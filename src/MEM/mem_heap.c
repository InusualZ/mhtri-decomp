/*
 * MEM/mem_heap.c - the MEM library heap core: the heap-list lookup (`FindContainHeap_`), the heap-head initialiser
 *    and finaliser.
 * RANGE. .text 0x804C1760-0x804C1BD0 (3 functions); .bss 0x80748B90-0x80748BB8; .sbss 0x80795298-0x807952A0.  Cut
 *    from the OS core band 0x804C1760-0x804D9B4C.  Evidence: the only readers of the heap list (.bss 0x80748B90
 *    and its OSMutex 0x80748BA0) and of the "list initialised" flag (.sbss 0x80795298) are `MEMiInitHeapHead` and
 *    the finaliser; the expandable-heap functions after 0x804C1BD0 read none of them.
 * FLAGS. `cflags_base` per object (configure.py); `#pragma ipa file` in this TU: file-level IPA inlines the plain
 *    global `FindContainHeap_` three levels deep into each caller (plus the call) and still emits it first, global.
 * NAMES. GUESS: `MEMiRootHeapList`, `MEMiRootHeapMutex`, `MEMiRootHeapListReady` (the heap-list data; the RVL SDK
 *    statics are believed to be sRootList/sRootMutex/sRootListInitialized). `FindContainHeap_` and `MEMiInitHeapHead`
 *    are the map's (dump) names; GUESS: `MEMiFinalizeHeap` (the counterpart of `MEMiInitHeapHead` that
 *    `MEMDestroyExpHeap` calls), `FindListContainHeap_` (RVL SDK helper name; fully inlined, no symbol) and the file name.
 * RESIDUALS. none in .text (3/3 functions byte-identical, relocations identical). .sbss: the claim's 0x8079529C word
 *    and .text's 0x804C1BCC word are alignment fill, not data.
 * SHAPES. The list choice is the helper `FindListContainHeap_` passed straight into `MEMAppendListObject`/
 *    `MEMRemoveListObject`; spelled inline in each caller it rotates the three inlined loop registers (r29/r30/r31).
 *    `FindContainHeap_` declares its child result inside the range test.
 */

#pragma ipa file

#include "types.h"

#include "MEM/mem.h"
#include "MEM/mem_heap.h"
#include "MEM/mem_list.h"
#include "OS/OSMutex.h"

static MEMList MEMiRootHeapList;      /* .bss 0x80748B90: list of the top-level heaps */
static OSMutex MEMiRootHeapMutex;     /* .bss 0x80748BA0: guards the heap list */
static u32 MEMiRootHeapListReady;     /* .sbss 0x80795298 */

/* Returns the innermost heap of `list` whose range holds `block`, or NULL. */
/* untyped: byte range - the address searched for */
MEMiHeapHead* FindContainHeap_(MEMList* list, const void* block)
{
    MEMiHeapHead* heap = NULL;

    while ((heap = (MEMiHeapHead*)MEMGetNextListObject(list, heap)) != NULL) {
        if ((u32)heap->start <= (u32)block && (u32)block < (u32)heap->end) {
            MEMiHeapHead* child = FindContainHeap_(&heap->children, block);
            if (child != NULL)
                return child;
            return heap;
        }
    }
    return NULL;
}

/* Returns the list a heap head belongs in: its container's child list, or the root list. */
static MEMList* FindListContainHeap_(MEMiHeapHead* heap)
{
    MEMList* list = &MEMiRootHeapList;
    MEMiHeapHead* parent = FindContainHeap_(&MEMiRootHeapList, heap);
    if (parent != NULL)
        list = &parent->children;
    return list;
}

/* Initialises a heap head and links it into the heap list of the heap that contains it. */
/* untyped: byte range - the heap's start and end addresses */
void MEMiInitHeapHead(MEMiHeapHead* heap, u32 signature, void* start, void* end, u16 attribute)
{
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
    MEMAppendListObject(FindListContainHeap_(heap), heap);
    OSUnlockMutex(&MEMiRootHeapMutex);
}

/* Unlinks a heap head from its parent's list and clears its signature. */
void MEMiFinalizeHeap(MEMiHeapHead* heap)
{
    OSLockMutex(&MEMiRootHeapMutex);
    MEMRemoveListObject(FindListContainHeap_(heap), heap);
    OSUnlockMutex(&MEMiRootHeapMutex);
    heap->signature = 0;
}
