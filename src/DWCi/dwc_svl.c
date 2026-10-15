/*
 * DWCi/dwc_svl.c - the DWC service locator: begins / ends a session and requests a service token through the auth
 *   interface, mapping the server's error codes into the DWC ranges.
 *
 * RANGE. `.text` 0x8050A990..0x8050AAF0 (4 functions); `.data` 0x80630828..0x80630848 (the unknown-code report);
 *   `.sbss` 0x80795800..0x80795808 (the result pointer). The left edge follows the NAS login (`DWCi/dwc_nasfunc.c`:
 *   its report strings end at 0x80630822 and this object's start on the next 8-byte boundary, and its `.sbss` word
 *   `DWCi_nasLoginState` is padded to 8 bytes); the right edge is the dynamic array (`DWCi/darray.c`;
 *   `DWCi_svlResult` is padded to 8 bytes).
 * FLAGS. `cflags_base` (-func_align 16): the retail functions start 16-byte aligned.
 * NAMES. DWC SDK names (`DWC_SVL*`); GUESS `DWCi_svlResult`. The file name is the DWC SDK's `dwc_svl.c` (no
 *   `__FILE__` string).
 * RESIDUALS. none measured (every row matches).
 */
#include "DWCi/dwc_svl.h"
#include "DWCi/dwc_nasfunc.h"
#include "DWCi/dwc_error.h"
#include "DWCi/dwc_auth_interface.h"
#include "unsplit/Runtime.PPCEABI.H.h"

DWCSvlResult* DWCi_svlResult;

/* 0x8050A990 (0x10): begins a service-locator session. */
s32 DWC_SVLBegin(void) {
    DWCi_svlResult = NULL;
    return 1;
}

/* 0x8050A9A0 (0xC): ends the service-locator session. */
void DWC_SVLEnd(void) {
    DWCi_svlResult = NULL;
}

/* 0x8050A9B0 (0x18): requests a token for the service `svl`; the answer lands in `result`. */
s32 DWC_SVLGetTokenAsync(const char* svl, DWCSvlResult* result) {
    DWCi_svlResult = result;
    return DWCi_npSetup((char*)svl, (DWCiAllocCallback)DWC_Alloc, (DWCiCommandCallback)DWC_Free);
}

/* 0x8050A9D0 (0x11C): steps the token request: 2 while it runs, 3 with a token, 4 on an error (recorded with the
 * server's code mapped into the DWC ranges). */
s32 DWC_SVLProcess(void) {
    DWCSvlResult* result;
    s32 code;

    if (DWCi_AdvanceStatus() != 0) {
        if (DWCi_IsStatusReady() != 0) {
            result = DWCi_svlResult;
            if (result != NULL) {
                *result = *(DWCSvlResult*)DWCi_GetWorkBuffer();
            }
            if (strlen(result->svltoken) == 0) {
                DWCi_SetError(15, -24101);
                return 4;
            }
            return 3;
        }
        code = DWCi_GetStatus();
        if ((u32)(code + 20110) <= 10) {
            code -= 4000;
        } else if ((u32)(code + 20999) <= 888) {
            code -= 4000;
        } else if ((u32)(code + 23999) <= 999) {
            code -= 2000;
        } else {
            DWC_Printf(0x1000000, "[svl] Unknown Error Code:%d\n", code);
        }
        DWCi_SetError(15, code);
        return 4;
    }
    DWCi_authDataTask();
    return 2;
}
