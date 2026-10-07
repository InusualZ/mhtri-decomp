/*
 * OS/OSIpc.c - the IPC buffer range the OS hands to the IOS interface: the two accessors and the initialiser.
 * RANGE. .text 0x804D5670-0x804D56B0 (3 functions); .sdata 0x80793FC0-0x80793FC8; .sbss 0x807953A0-0x807953A8.  Cut from
 *    the OS core band 0x804C1760-0x804D9B4C.  Evidence: the two static words `IpcBufferLo` (.sdata, initialised to -1) and
 *    `IpcBufferHi` (.sbss) are read and written only by these three functions.
 * FLAGS. `cflags_base` per object (configure.py): the target's function starts are all 16-aligned.
 * NAMES. the three functions and `IpcBufferLo`/`IpcBufferHi` are the map's names.
 * RESIDUALS. the .sdata and .sbss objects end 4 bytes short of the claimed 8 (the trailing word is alignment padding).
 * SHAPES. none beyond what the bodies show.
 */

#include "types.h"

#include "OS/OS.h"
#include "OS/OSIpc.h"

static void* IpcBufferLo = (void*)0xFFFFFFFF;
static void* IpcBufferHi;

/* Returns the end of the IPC buffer range. */
/* untyped: raw address of the IPC buffer range end */
void* __OSGetIPCBufferHi(void)
{
    return IpcBufferHi;
}

/* Returns the start of the IPC buffer range. */
/* untyped: raw address of the IPC buffer range start */
void* __OSGetIPCBufferLo(void)
{
    return IpcBufferLo;
}

/* Latches the IPC buffer range the boot code left in low memory. */
void __OSInitIPCBuffer(void)
{
    IpcBufferLo = OS_IPC_BUFFER_LO;
    IpcBufferHi = OS_IPC_BUFFER_HI;
}
