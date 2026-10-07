/*
 * OS/OSLink.c - the OS module-loader reset: clears the module queue and the string table words in low memory.
 * RANGE. .text 0x804D1440-0x804D1460 (1 function).  Cut from the OS core band 0x804C1760-0x804D9B4C.  Evidence: one
 *    leaf function that stores zero to 0x800030C8..0x800030D0; no data.
 * FLAGS. `cflags_base` per object (configure.py): the target's function starts are all 16-aligned.
 * NAMES. `__OSModuleInit` is the map's name (GUESS: the dump has a placeholder; the body clears the module queue); the low-memory macros are in `OS/OS.h`.
 * RESIDUALS. none: the body matches; the object's .text ends 8 bytes before the claimed end (alignment padding).
 * SHAPES. none beyond what the body shows.
 */

#include "types.h"

#include "OS/OS.h"
#include "OS/OSLink.h"

/* Clears the module queue and the string-table pointer. */
void __OSModuleInit(void)
{
    OS_MODULE_QUEUE.tail = NULL;
    OS_MODULE_QUEUE.head = NULL;
    OS_STRING_TABLE = NULL;
}
