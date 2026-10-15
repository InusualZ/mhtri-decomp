/*
 * DWCi/dwc_nasfunc.c - the DWC NAS login front end: the auth task's teardown and NAND-result steps, and the NAS login
 *   start / step functions the game's GameSpy interface drives.
 *
 * RANGE. `.text` 0x8050A710..0x8050A990 (4 functions); `.data` 0x806307F0..0x80630828 (the two report strings);
 *   `.sdata` 0x80794330..0x80794338 (the empty names); `.sbss` 0x807957F8..0x80795800 (the login state). The left edge
 *   follows `DWCi/dwc_auth_interface.c` (its last string ends 0x806307EA, this object's `.data` starts on the next
 *   8-byte boundary); the right edge is the service locator (`DWCi/dwc_svl.c`; the report strings end 0x80630822
 *   and `DWCi_nasLoginState` is padded to 8 bytes). The GameSpy code the DWC library links behind it is its own
 *   files: `DWCi/darray.c`, `DWCi/hashtable.c`, `DWCi/gsPlatform.c`, `DWCi/gsAvailable.c`, `DWCi/gt2Auth.c` ..
 *   `DWCi/gt2Message.c`.
 * FLAGS. `cflags_base`: the retail functions start 16-byte aligned (`gap_*` words between them).
 * NAMES. DWC SDK names (`DWC_NASLoginAsync`, `DWC_NASLoginProcess`); `DWCi_Auth_EndProcess` is the function's own
 *   report string; GUESS `DWCi_Auth_CheckNandResult`, GUESS `DWCi_nasLoginState`. The file name is the DWC SDK's
 *   `dwc_nasfunc.c` (no `__FILE__` string); the two `DWCi_Auth_*` steps share the object (no seam in its data).
 * RESIDUALS. none measured (every row matches).
 */
#include "DWCi/dwc_nasfunc.h"
#include "DWCi/dwc_error.h"
#include "DWCi/dwc_auth_interface.h"

s32 DWCi_nasLoginState;

/* 0x8050A710 (0x58): the NHTTP teardown callback: reports, tells the host the session is over and parks the
 * state machine in its terminal state. */
void DWCi_Auth_EndProcess(void) {
    DWC_Printf(0x1000000, "DWCi_Auth_EndProcess()\n");
    DWCi_runtime->commandCallback(0, (u32)DWCi_runtime, 0);
    DWCi_runtime = NULL;
    DWCi_state = 25;
}

/* 0x8050A770 (0x128): moves the auth state machine on after a NAND call completed. */
void DWCi_Auth_CheckNandResult(s32 success, s32 retry, s32 noExist, s32 access) {
    s32 result;

    DWCi_runtime->resultFlag = 0;
    if (retry != 27 && (s32)DWCi_runtime->resultValue == -3 && DWCi_runtime->retryCount < 5) {
        DWCi_runtime->retryCount++;
        DWCi_state = retry;
        return;
    }
    DWCi_runtime->retryCount = 0;
    result = DWCi_runtime->resultValue;
    if (result == 0) {
        DWCi_state = success;
    } else if (noExist != 27 && result == -12) {
        DWCi_state = noExist;
    } else if (access != 27 && result == -1) {
        DWCi_state = access;
    } else {
        DWC_Printf(0x1000000, " NAND access failed.[%d]\n", result);
        *(volatile s32*)DWCi_stateBlock = ((s32)DWCi_runtime->resultValue == -4 ? -1 : 0) - 29000;
        DWCi_Auth_EndProcess();
    }
}


/* ---- dwc_nas ----------------------------------------------------------------------------------------- */

/* 0x8050A8A0 (0x2C): starts the NAS login (the auth interface with empty names and the DWC allocator). */
s32 DWC_NASLoginAsync(void) {
    DWCi_nasLoginState = 4;
    return DWCi_initRuntime((char*)L"", "", 0, (DWCiAllocCallback)DWC_Alloc, (DWCiCommandCallback)DWC_Free);
}

/* 0x8050A8D0 (0xB4): steps the NAS login: 2 while it runs, 3 on success, 4 on an error (recorded), 5 cancelled. */
s32 DWC_NASLoginProcess(void) {
    switch (DWCi_nasLoginState) {
    case 1:
        return 3;
    case 2:
        return 5;
    case 3:
        return 4;
    case 4:
        DWCi_authDataTask();
        if (DWCi_AdvanceStatus() != 0) {
            if (DWCi_IsStatusReady() != 0) {
                DWCi_nasLoginState = 1;
                return 3;
            }
            DWCi_nasLoginState = 3;
            DWCi_SetError(2, DWCi_GetStatus());
            return 4;
        }
        return 2;
    }
    return 4;
}
