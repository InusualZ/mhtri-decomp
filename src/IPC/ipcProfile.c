/*
 * IPC/ipcProfile.c - the SDK IPC profiling counters (`IPCiProf*`).
 *
 * RANGE. `.text` 0x804BD5A0-0x804BD780 (4 functions / 0x1C8 B); `.bss` 0x80747440-0x80747540; `.sbss`
 *   0x80795258-0x80795260.
 *   - `IpcFdArray` / `IpcReqPtrArray` (.bss 0x80747440..0x80747540) and `.sbss` 0x80795258..0x80795260 are read only
 *     here
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`) with `cflags_base` (-O4,p, default alignment 16).
 * NAMES. `IPCiProfAck` is a GUESS and `IPCiProfInit` is a GUESS for the map's former placeholders (`IPCiProfAck` retires one un-issued request).
 * RESIDUALS. `IPCiProfQueueReq`: the index and the two walking pointers take r9/r7/r8 where the target has r7/r8/r9 (same
 *   instructions; declaration order, `u32` index, `i = 0` initialiser and statement order were measured).  flipcheck:
 *   `.text` is 4 B short of the claim (the pad before `KPAD/kpad.c`).
 */

#include "types.h"
#include "IPC/ipcProfile.h"

#define IPC_MAX_REQS 32

/* `.bss` and `.sbss` are laid out in reverse definition order. */
static s32 IpcNumUnIssuedReqs;
static s32 IpcNumPendingReqs;
static IPCRequest* IpcReqPtrArray[IPC_MAX_REQS];
static s32 IpcFdArray[IPC_MAX_REQS];

/* Clears the request table and the counters. */
void IPCiProfInit(void)
{
    s32 i;

    IpcNumPendingReqs = 0;
    IpcNumUnIssuedReqs = 0;
    for (i = 0; i < IPC_MAX_REQS; i++) {
        IpcReqPtrArray[i] = NULL;
        IpcFdArray[i] = -1;
    }
}

/* Records a newly queued request in the first free table slot. */
void IPCiProfQueueReq(IPCRequest* req, s32 fd)
{
    s32 i;

    IpcNumPendingReqs++;
    IpcNumUnIssuedReqs++;
    for (i = 0; i < IPC_MAX_REQS; i++) {
        if (IpcReqPtrArray[i] == NULL && IpcFdArray[i] == -1) {
            IpcReqPtrArray[i] = req;
            IpcFdArray[i] = fd;
            return;
        }
    }
}

/* Counts one request as issued to the IOS. */
void IPCiProfAck(void)
{
    IpcNumUnIssuedReqs--;
}

/* Retires the table slot of a request the IOS replied to. */
void IPCiProfReply(IPCRequest* req, s32 fd)
{
    s32 i;

    IpcNumPendingReqs--;
    for (i = 0; i < IPC_MAX_REQS; i++) {
        if (req == IpcReqPtrArray[i] && fd == IpcFdArray[i]) {
            IpcReqPtrArray[i] = NULL;
            IpcFdArray[i] = -1;
            return;
        }
    }
}
