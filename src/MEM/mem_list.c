/*
 * MEM/mem_list.c - the MEM intrusive list: init, append, remove and next.
 * RANGE. .text 0x804C24C0-0x804C25E0 (4 functions).  Cut from the OS core band 0x804C1760-0x804D9B4C.  Evidence: four
 *    functions with no data and no foreign calls, between the allocator (ends 0x804C24C0) and the first AX call
 *    of `__MIXSetPan` (0x804C25E0); `MEMiInitHeapHead` and the expandable heap call them.
 * FLAGS. `cflags_base` per object (configure.py): the target's function starts are all 16-aligned.
 * NAMES. GUESS: `MEMInitList`, `MEMAppendListObject`, `MEMRemoveListObject`, `MEMGetNextListObject` (the dump holds only
 *    junk names; the bodies are the list API's init, append, remove and next); the file name is a GUESS.
 * RESIDUALS. none: every body matches.
 * SHAPES. none beyond what the bodies show.
 */

#include "types.h"

#include "MEM/mem.h"
#include "MEM/mem_list.h"

/* Empty a list and record the node displacement its objects use. */
void MEMInitList(MEMList* list, u16 offset)
{
    list->head = NULL;
    list->tail = NULL;
    list->count = 0;
    list->offset = offset;
}

/* Append an object, linking it after the current tail (or making it head and tail of an empty list). */
/* untyped: opaque handle passed through */
void MEMAppendListObject(MEMList* list, void* object)
{
    if (list->head == NULL) {
        MEMLink* link = (MEMLink*)((u8*)object + list->offset);
        link->next = NULL;
        link->prev = NULL;
        list->head = object;
        list->tail = object;
        list->count++;
    } else {
        MEMLink* link = (MEMLink*)((u8*)object + list->offset);
        link->prev = list->tail;
        link->next = NULL;
        ((MEMLink*)((u8*)list->tail + list->offset))->next = object;
        list->tail = object;
        list->count++;
    }
}

/* Unlink an object from the middle of the list and clear its node. */
/* untyped: opaque handle passed through */
void MEMRemoveListObject(MEMList* list, void* object)
{
    MEMLink* link = (MEMLink*)((u8*)object + list->offset);

    if (link->prev == NULL)
        list->head = link->next;
    else
        ((MEMLink*)((u8*)link->prev + list->offset))->next = link->next;

    if (link->next == NULL)
        list->tail = link->prev;
    else
        ((MEMLink*)((u8*)link->next + list->offset))->prev = link->prev;

    link->prev = NULL;
    link->next = NULL;
    list->count--;
}

/* The object after `object` (NULL yields the head). */
/* untyped: opaque handle passed through */
void* MEMGetNextListObject(MEMList* list, void* object)
{
    if (object == NULL)
        return list->head;
    return ((MEMLink*)((u8*)object + list->offset))->next;
}
