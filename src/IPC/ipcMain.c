/*
 * IPC/ipcMain.c - the SDK IPC core (buffer bounds, register access, init).
 *
 * RANGE. `.text` 0x804BB220-0x804BB310 (7 functions / 0xCC B); `.sbss` 0x80795228-0x80795240.
 *   - `.sbss` 0x80795228..0x80795240 is read by `IPCInit` and the buffer-bound accessors only; the client run starts
 *     at 0x804BB310
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`) with `cflags_base` (-O4,p, default alignment 16).
 * NAMES. `IPCReInit` is a GUESS (the unconditional twin of `IPCInit`); the five `.sbss` words are GUESSes from their
 *   use (`__IPCInitialized`, `IPCBufferLo/Hi` the live window, `IPCArenaLo/Hi` the OS arena it was cut from).
 * RESIDUALS. none recorded yet.
 */

#include "types.h"
#include "IPC/ipcMain.h"
#include "OS/OSIpc.h"

/* The IPC register window of the hollywood bus, placed at its hardware address (no relocation in the object). */
volatile u32 IPC_REGS[] : 0xCD000000;

/* `.sbss` is laid out in reverse definition order. */
static u8* IPCArenaHi;
static u8* IPCArenaLo;
static u8* IPCBufferHi;
static u8* IPCBufferLo;
static u8 __IPCInitialized;

/* Reads the OS's IPC buffer window once and keeps it as the live window. */
void IPCInit(void)
{
    if (!__IPCInitialized) {
        IPCArenaHi = __OSGetIPCBufferHi();
        IPCArenaLo = __OSGetIPCBufferLo();
        IPCBufferHi = IPCArenaHi;
        IPCBufferLo = IPCArenaLo;
        __IPCInitialized = TRUE;
    }
}

/* Re-reads the OS's IPC buffer window unconditionally. */
void IPCReInit(void)
{
    __IPCInitialized = FALSE;
    IPCArenaHi = __OSGetIPCBufferHi();
    IPCArenaLo = __OSGetIPCBufferLo();
    IPCBufferHi = IPCArenaHi;
    IPCBufferLo = IPCArenaLo;
    __IPCInitialized = TRUE;
}

/* Reads one IPC register. */
u32 IPCReadReg(u32 reg)
{
    return IPC_REGS[reg];
}

/* Writes one IPC register. */
void IPCWriteReg(u32 reg, u32 value)
{
    IPC_REGS[reg] = value;
}

/* The top of the live IPC buffer window. */
u8* IPCGetBufferHi(void)
{
    return IPCBufferHi;
}

/* The bottom of the live IPC buffer window. */
u8* IPCGetBufferLo(void)
{
    return IPCBufferLo;
}

/* Moves the bottom of the live IPC buffer window. */
void IPCSetBufferLo(u8* lo)
{
    IPCBufferLo = lo;
}
