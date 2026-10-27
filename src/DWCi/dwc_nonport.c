/*
 * DWCi/dwc_nonport.c - the DWC library's platform layer ahead of the auth interface: the fast memory copy and the
 * console friend code.
 *
 * RANGE. `.text` 0x80507C40..0x80508750 (2 functions: `DWCi_Np_CPUCopyFast`, `DWCi_GetConsoleFriendCode`);
 *   `.data` 0x8062FF90..0x80630020 (the copy's jump table, the two friend-code report lines); `.sbss`
 *   0x807957D0..0x807957E0 (the friend code and its one-shot flag). The right edge is pinned by
 *   `DWCi/dwc_auth_interface.c`'s string-pool base: its functions address their `.data` off `DWCi_authDataPath`
 *   (0x80630020), which MWCC only does when that object starts the TU's `.data`; `dwc_report.cpp` (Matching) ends
 *   the left edge.
 * FLAGS. `cflags_base`, as its DWCi siblings (16-byte function alignment).
 * NAMES. `DWCi_Np_CPUCopyFast` is the map's; the file name is a GUESS from its `DWCi_Np_` (non-port) prefix (no
 *   `__FILE__` string); `DWCi_GetConsoleFriendCode` and the data names are GUESSes from the bodies.
 * SHAPES. `DWCi_Np_CPUCopyFast` is a goto-free dispatch on the copy width (rule 8): the retail jumps share the
 *   `memcpy` tail and the three copy loops (93.82 with gotos, 91.11 without). The `.sbss` objects are defined
 *   last first.
 * RESIDUALS. `DWCi_Np_CPUCopyFast`: the shared tails above (`.text` 68 B over); retail's forward jumps from the
 *   alignment code into the copy loops cross each other, which nested `if (0) { case ... }` blocks cannot express
 *   (playbook 241).
 *   `DWCi_GetConsoleFriendCode` names `DWCi_consoleFriendCode` where retail names `lbl_807957DC` (the low word).
 */

#include "types.h"
#include "Runtime.PPCEABI.H/memcpy.h"    /* memcpy: the runtime unit's header (rule 2) */
#include "NWC24/nwc24_msg.h"             /* NWC24iGetUserId: the NWC24 band's own header (rule 2) */
#include "VF/vf.h"                       /* VFipf2*: its owner's header (rule 2) */
#include "DWCi/dwc_init.h"               /* DWC_AllocEx, DWC_Free */
#include "DWCi/dwc_report.h"             /* DWC_Printf */
#include "DWCi/dwc_nonport.h"

/* 0x80507C40 (0x9E4): copies `len` bytes, by the widest unit both pointers can be aligned to (8, 4 or 2
 * bytes, after copying the bytes in front of the boundary), falling back to `memcpy` for odd distances. */
void DWCi_Np_CPUCopyFast(u8* dst, const u8* src, u32 len) {
    u32 diff;
    s32 count;
    s32 n;
    s32 width;
    if (len > 32) {
        diff = (u32)dst ^ (u32)src;
        if (diff & 1) {
            width = 1;
        } else {
            diff &= 15;
            if (diff & 2) {
                n = 2 - ((u32)dst & 1);
                if (n != 0) {
                    *dst++ = *src++;
                    len -= 1;
                }
                width = 2;
            } else if (diff & 4) {
                n = 4 - ((u32)dst & 3);
                switch (n) {
                case 3: *dst++ = *src++;
                case 2: *dst++ = *src++;
                case 1: *dst++ = *src++;
                    len -= n;
                }
                width = 4;
            } else {
                n = 8 - ((u32)dst & 7);
                switch (n) {
                case 7: *dst++ = *src++;
                case 6: *dst++ = *src++;
                case 5: *dst++ = *src++;
                case 4: *dst++ = *src++;
                case 3: *dst++ = *src++;
                case 2: *dst++ = *src++;
                case 1: *dst++ = *src++;
                    len -= n;
                }
                width = 8;
            }
        }
    } else if ((((u32)dst & 7) == 0) && (((u32)src & 7) == 0)) {
        width = 8;
    } else if ((((u32)dst & 3) == 0) && (((u32)src & 3) == 0)) {
        width = 4;
    } else if ((((u32)dst & 1) == 0) && (((u32)src & 1) == 0)) {
        width = 2;
    } else {
        width = 1;
    }
    if (width == 1) {
        memcpy(dst, src, len);
    } else if (width == 2) {
        count = (s32)(len >> 1);
        if (count != 0) {
            u16* d = (u16*)dst;
            const u16* s = (const u16*)src;
            s32 i;
            i = count >> 4;
            while (i-- > 0) {
                *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++;
                *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++;
                *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++;
                *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++;
            }
            switch (count & 15) {
            case 15: *d++ = *s++;
            case 14: *d++ = *s++;
            case 13: *d++ = *s++;
            case 12: *d++ = *s++;
            case 11: *d++ = *s++;
            case 10: *d++ = *s++;
            case 9: *d++ = *s++;
            case 8: *d++ = *s++;
            case 7: *d++ = *s++;
            case 6: *d++ = *s++;
            case 5: *d++ = *s++;
            case 4: *d++ = *s++;
            case 3: *d++ = *s++;
            case 2: *d++ = *s++;
            case 1: *d++ = *s++;
            }
            dst = (u8*)d;
            src = (const u8*)s;
        }
        if ((len & 1) == 1) {
            *dst = *src;
        }
    } else if (width == 4) {
        count = (s32)(len >> 2);
        if (count != 0) {
            u32* d = (u32*)dst;
            const u32* s = (const u32*)src;
            s32 i;
            i = count >> 3;
            while (i-- > 0) {
                *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++;
                *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++;
            }
            switch (count & 7) {
            case 7: *d++ = *s++;
            case 6: *d++ = *s++;
            case 5: *d++ = *s++;
            case 4: *d++ = *s++;
            case 3: *d++ = *s++;
            case 2: *d++ = *s++;
            case 1: *d++ = *s++;
            }
            dst = (u8*)d;
            src = (const u8*)s;
        }
        switch (len & 3) {
        case 3: *dst++ = *src++;
        case 2: *dst++ = *src++;
        case 1: *dst++ = *src++;
        }
    } else {
        count = (s32)(len >> 3);
        if (count != 0) {
            u64* d = (u64*)dst;
            const u64* s = (const u64*)src;
            s32 i;
            i = count >> 3;
            while (i-- > 0) {
                *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++;
                *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++;
            }
            switch (count & 7) {
            case 7: *d++ = *s++;
            case 6: *d++ = *s++;
            case 5: *d++ = *s++;
            case 4: *d++ = *s++;
            case 3: *d++ = *s++;
            case 2: *d++ = *s++;
            case 1: *d++ = *s++;
            }
            dst = (u8*)d;
            src = (const u8*)s;
        }
        switch (len & 7) {
        case 7: *dst++ = *src++;
        case 6: *dst++ = *src++;
        case 5: *dst++ = *src++;
        case 4: *dst++ = *src++;
        case 3: *dst++ = *src++;
        case 2: *dst++ = *src++;
        case 1: *dst++ = *src++;
        }
    }
}

/* 0x8062FFD0 (0x24) / 0x8062FFF4 (0x25): the two lines the friend-code reporter prints, the value
 * as `%016lld` (the 64-bit pair `lbl_807957D8`/`0xDC` prints as two varargs registers). */
char DWCi_reportFriendCodeFormat[] = " get console friend code = %016lld\n";
char DWCi_reportFriendCodeFailedFormat[] = " failed to get console friend code.\n";

/* The unit's `.sbss` run, defined in reverse: MWCC emits `.sbss` objects last first. */
u64 DWCi_consoleFriendCode;
s32 DWCi_friendCodeReady;

/* 0x80508630 (0x118): the console's friend code, fetched once.  The first call brings the file
 * system up, takes the two blocks `DWC_AllocEx` hands it, reads the id into the unit's own
 * `.sbss` value and reports it; every later call answers out of that value. */
u64 DWCi_GetConsoleFriendCode(void) {
    s32 vfInitialized;
    u8* first;
    u8* second;

    if (DWCi_friendCodeReady == 0) {
        vfInitialized = VFipf2IsInitialized();
        first = DWC_AllocEx(3, 0x4000, 0x20);
        second = DWC_AllocEx(3, 0x8000, 0x20);
        DWCi_friendCodeReady = 1;
        if (vfInitialized != 1) {
        VFipf2Init(second, 0x8000);
        }
        if (NWC24iGetUserId((u32*)&DWCi_consoleFriendCode) == 0) {
        DWC_Printf(0x8000000, DWCi_reportFriendCodeFormat, DWCi_consoleFriendCode);
        } else {
        DWC_Printf(0x8000000, DWCi_reportFriendCodeFailedFormat);
        DWCi_consoleFriendCode = 0;
        }
        if (vfInitialized != 1) {
        VFipf2Shutdown();
        }
        DWC_Free(3, first, 0);
        DWC_Free(3, second, 0);
    }
    return DWCi_consoleFriendCode;
}
