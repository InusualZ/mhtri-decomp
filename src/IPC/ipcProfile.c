/*
 * IPC/ipcProfile.c - the SDK IPC profiling counters (`IPCiProf*`).
 *
 * RANGE. `.text` 0x804BD5A0-0x804BD780 (4 functions / 0x1DC B + 4 B pad); `.bss` 0x80747440-0x80747540; `.sbss`
 *   0x80795258-0x80795260.
 *   - `IpcFdArray` / `IpcReqPtrArray` (.bss 0x80747440..0x80747540) and `.sbss` 0x80795258..0x80795260 are read only
 *     here
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`) with `cflags_base` (-O4,p, default alignment 16).
 * NAMES. `IPCiProfAck` is a GUESS and `IPCiProfInit` is a GUESS for the map's former placeholders (`IPCiProfAck` retires one un-issued request).
 * RESIDUALS. none in `.text` (4/4 rows byte-identical, relocations 30/30, `.bss`/`.sbss` layouts identical); the
 *   4 B between the last function and the claim's end is link padding before `KPAD/kpad.c` (align 16).
 * SHAPES. `IPCiProfQueueReq`: the two tables are walked through named pointers declared after the index (an indexed
 *   loop leaves the walks as frontend webs numbered above `i`, so `i` colours last and takes r9 where retail has r7;
 *   as locals below `i` they colour after it and take r8/r9), and the `for` step bumps the pointers before the index
 *   (retail's loop tail order).  `.bss`: `IpcFdArray` carries a zero initialiser so it is emitted ahead of the
 *   tentative `IpcReqPtrArray` (playbook idea 158); the tentative objects follow their first code reference, which
 *   put `IpcReqPtrArray` first.
 */

#include "types.h"
#include "IPC/ipcProfile.h"

#define IPC_MAX_REQS 32

/* The tentative objects land in first-reference order; the zero-initialised table is emitted ahead of them. */
static s32 IpcNumUnIssuedReqs;
static s32 IpcNumPendingReqs;
static IPCRequest* IpcReqPtrArray[IPC_MAX_REQS];
static s32 IpcFdArray[IPC_MAX_REQS] = {0};

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
    IPCRequest** reqp;
    s32* fdp;

    IpcNumPendingReqs++;
    IpcNumUnIssuedReqs++;
    reqp = IpcReqPtrArray;
    fdp = IpcFdArray;
    for (i = 0; i < IPC_MAX_REQS; reqp++, fdp++, i++) {
        if (*reqp == NULL && *fdp == -1) {
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
