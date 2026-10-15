/*
 * DWCi/dwc_error.cpp - the DWC error state: the last error, its code and the dialog error type.
 *
 * RANGE. `.text` 0x805073C0..0x805074B0 (3 functions); `.data` 0x8062FD00..0x8062FD68 (the error-type jump table);
 *   `.sbss` 0x807957B0..0x807957B8 (error, code). The right edge is the `.data` seam: the target pads the jump
 *   table 0x64 -> 0x68 before DWC_Init's strings, which one object does not emit (dwc_init.cpp follows).
 * FLAGS. `cflags_base` (the retail functions start 16-byte aligned, `gap_*` words between them; the lib's
 *   `cflags_dwc` packs on 4); measurement in docs/network.md.
 * NAMES. Nintendo DWC SDK names read off the bodies (`DWC_GetLastErrorEx`, `DWC_ClearError`); GUESS on
 *   `DWCi_SetError` (the setter twin of `DWC_ClearError`) and on the `.sbss` word names.
 * RESIDUALS. none recorded.
 */
#include "DWCi/dwc_error.h"

extern "C" {

#define DWC_ERROR_FATAL 9

s32 stDwcLastError;
s32 stDwcErrorCode;

/* 0x805073C0 (0xAC): returns the last error, its code and the error type the dialog layer shows. */
s32 DWC_GetLastErrorEx(s32* code, s32* type) {
    if (code != NULL) {
        *code = stDwcErrorCode;
    }
    if (type != NULL) {
        switch ((u32)stDwcLastError) {
        case 2:
        case 3:
        case 4:
        case 5:
        case 8:
        case 14:
        case 16:
        case 18:
        case 21:
            *type = 6;
            break;
        case 6:
            *type = 3;
            break;
        case 7:
            *type = 4;
            break;
        case 13:
            *type = 5;
            break;
        case 22:
            *type = 8;
            break;
        case 10:
        case 11:
        case 12:
        case 23:
            *type = 1;
            break;
        case 15:
        case 17:
        case 19:
        case 20:
        case 24:
            *type = 2;
            break;
        case 1:
        case 9:
            *type = 7;
            break;
        default:
            *type = 0;
            break;
        }
    }
    return stDwcLastError;
}

/* 0x80507470 (0x1C): clears the error state unless it is fatal. */
void DWC_ClearError(void) {
    if (stDwcLastError != DWC_ERROR_FATAL) {
        stDwcLastError = 0;
        stDwcErrorCode = 0;
    }
}

/* 0x80507490 (0x18): records an error and its code unless a fatal error is already pending. */
void DWCi_SetError(s32 error, s32 code) {
    if (stDwcLastError != DWC_ERROR_FATAL) {
        stDwcLastError = error;
        stDwcErrorCode = code;
    }
}

}
