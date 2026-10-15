/*
 * NET/netmisc.c - the RevoEX NET helpers linked after the NCD library: the `SOStartup` error-code mapping
 *   (`NETGetStartupErrorCode` and its two helpers), the REX version string, the wireless MAC wrapper and the
 *   NET memcpy/memset.
 * RANGE. .text 0x8051CDD0-0x8051D710 (7 functions); .data 0x806310D0-0x80631128 ("Unknown SOStartup Error: %d\n" and
 *   the "REX-PPC" version string), .sdata 0x80794430-0x80794438 (`netRexVersion`).  No .bss, .rodata or .sdata2.
 * RANGE. Left edge: the NET `.data` and `.sdata` start 8-aligned after the NCD tail (0x806310CA, 0x80794428 + 4),
 *   so a second object starts there; in `.text` the edge lies in 0x8051CDD0-0x8051D048.  It is drawn at 0x8051CDD0:
 *   `NETiConfigListToErrorBase` touches no NCD state (no data, no IPC), and its only caller is
 *   `NETGetStartupErrorCode`; every function at and above it is NET's, while every NCD function below it uses the
 *   NCD statics or ioctl.  The two edges link byte-identically (same compiler and flags), so the `.text` choice is
 *   unfalsifiable from the image.  Right edge: `NWC24/nwc24_msg.c` at 0x8051D710.
 * FLAGS. the NWC24 lib block's GC/3.0a5.2 + `cflags_nwc24`, like `NCD/ncdsystem.c`; the file sets
 *   `#pragma auto_inline off` (retail calls `NETiConfigListToErrorBase` from the function after it).
 * NAMES. Module NET from the RevoEX version string ("<< REX-PPC ... (RevoEX-2.4) ... >>") and the `NET*` API names
 *   (`NETMemCpy` is the runtime dump's name).  The file name is a GUESS (no `__FILE__` string in the range).
 *   GUESS: `NETiConfigListToErrorBase`, `NETGetStartupErrorCode`, `NETiStartupErrorToCode`, `NETGetRexVersion`,
 *   `netRexVersion`, `NCDGetWirelessMacAddress` (a NET-object wrapper over `NCDiGetWirelessMacAddress`; the SDK
 *   spelling is probably `NETGetWirelessMacAddress`).
 * SHAPES. `NETMemCpy`/`NETMemSet` are `asm` functions: retail replicates the fill byte with in-place `rlwimi r4,r4`,
 *   runs counted `mtctr` loops whose shape no C spelling of ours reproduces (unrolled at -O4,p, `mtctr` hoisted above
 *   the zero test under `optimize_for_size`) and saves r31 in a leaf without a link-register frame.
 * SHAPES. Nothing in the image calls `NETGetRexVersion`: it is `__declspec(export)` so the link keeps it (playbook 36).
 * RESIDUALS. none in `.text`.
 */

#include "NET/netmisc.h"
#include "NCD/ncdsystem.h"
#include "unsplit/OS.h"

#pragma auto_inline off

s32 NETiConfigListToErrorBase(u32 wired, u32 wireless, u32 wirelessOther);
s32 NETiStartupErrorToCode(s32 result, s32 base);

/* The index of the lowest set bit of `mask`, or -1. */
static inline s32 NETiLowestBit(u32 mask) {
    s32 i;
    u32 bit;
    for (i = 0, bit = 1; i < 32; i++, bit <<= 1) {
        if (mask & bit) {
            return i;
        }
    }
    return -1;
}

/* 0x8051CDD0 (0x278): the error-code base for the enabled profile masks (99 when none or several kinds). */
s32 NETiConfigListToErrorBase(u32 wired, u32 wireless, u32 wirelessOther) {
    s32 base = 99;
    if (wired != 0) {
        if (wireless == 0 && wirelessOther == 0) {
            base = NETiLowestBit(wired) + 20;
        }
    } else if (wireless != 0) {
        if (wirelessOther == 0) {
            base = NETiLowestBit(wireless) + 30;
        }
    } else if (wirelessOther != 0) {
        base = NETiLowestBit(wirelessOther) + 40;
    }
    return base;
}

/* 0x8051D048 (0x80): the user-facing network error code for a failed `SOStartup` result. */
s32 NETGetStartupErrorCode(s32 result) {
    s32 base = 99;
    u32 wirelessOther;
    u32 wireless;
    u32 wired;

    if (NCDiGetEnabledConfigList(&wired, &wireless, &wirelessOther) >= 0) {
        base = NETiConfigListToErrorBase(wired, wireless, wirelessOther);
    }
    if (base < 0) {
        result = (s32)0x80000000;
        base = 99;
    }
    return NETiStartupErrorToCode(result, base) - base;
}

/* 0x8051D0C8 (0x178): maps an `SOStartup` result onto its error code (offset by the profile `base`). */
s32 NETiStartupErrorToCode(s32 result, s32 base) {
    if (result >= 0) {
        return 0;
    }
    switch (result) {
    case -45:
        return -50200;
    case -28:
        return -50300;
    case -62:
        return -50400;
    case -111:
        return -52700;
    case -121:
        if (base >= 20 && base < 30) {
            return -51400;
        }
        return -51000;
    case -112:
    case -76:
    case -48:
    case -39:
        if (base >= 20 && base < 30) {
            return -51400;
        }
        return -51300;
    case -102:
    case -101:
    case -100:
        return -52000;
    case (s32)0x80000000:
        return -50100;
    default:
        OSReport("Unknown SOStartup Error: %d\n", result);
        return -50100;
    }
}

char* netRexVersion = "<< REX-PPC 2.4.255.0 (RevoEX-2.4) REL 090609111526 >>";

/* 0x8051D240 (0x8): the RevoEX version string. */
__declspec(export) char* NETGetRexVersion(void) {
    return netRexVersion;
}

/* 0x8051D248 (0x4): copies the wireless MAC address (6 bytes) into `address`. */
void NCDGetWirelessMacAddress(u8* address) {
    NCDiGetWirelessMacAddress(address);
}

/* 0x8051D24C (0x3C0): copies `size` bytes (overlap-safe), 32-byte blocks through `dcbz` when the run is long. */
/* untyped: byte range */
asm void* NETMemCpy(void* dst, const void* src, u32 size) {
    nofralloc
    stwu r1,-16(r1)
    cmplw r3,r4
    mr r0,r3
    stw r31,12(r1)
    bne cpy_1
    b cpy_27
cpy_1:
    ble cpy_5
    add r6,r4,r5
    cmplw r3,r6
    bge cpy_5
    clrlwi. r4,r5,30
    add r7,r3,r5
    srwi r5,r5,2
    beq cpy_3
    mtctr r4
cpy_2:
    lbzu r3,-1(r6)
    stbu r3,-1(r7)
    bdnz cpy_2
cpy_3:
    cmpwi r5,0
    beq cpy_26
    mtctr r5
cpy_4:
    lwzu r3,-4(r6)
    stwu r3,-4(r7)
    bdnz cpy_4
    b cpy_26
cpy_5:
    addi r6,r4,-32
    cmplw r3,r6
    ble cpy_6
    cmplw r3,r4
    blt cpy_22
cpy_6:
    cmplwi r5,64
    blt cpy_22
    clrlwi. r10,r3,27
    beq cpy_11
    subfic r10,r10,32
    mr r8,r4
    srwi. r6,r10,2
    mr r9,r3
    clrlwi r7,r10,30
    beq cpy_8
    mtctr r6
cpy_7:
    lwz r6,0(r8)
    addi r8,r8,4
    stw r6,0(r9)
    addi r9,r9,4
    bdnz cpy_7
cpy_8:
    cmpwi r7,0
    beq cpy_10
    mtctr r7
cpy_9:
    lbz r6,0(r8)
    addi r8,r8,1
    stb r6,0(r9)
    addi r9,r9,1
    bdnz cpy_9
cpy_10:
    add r3,r3,r10
    add r4,r4,r10
    subf r5,r10,r5
cpy_11:
    clrlwi r8,r4,30
    mr r7,r4
    cmpwi r8,2
    mr r6,r3
    clrrwi r31,r5,5
    srwi r9,r5,5
    beq cpy_17
    bge cpy_12
    cmpwi r8,0
    beq cpy_13
    bge cpy_15
    b cpy_21
cpy_12:
    cmpwi r8,4
    bge cpy_21
    b cpy_19
cpy_13:
    mtctr r9
cpy_14:
    dcbz 0,r6
    lwz r12,0(r7)
    lwz r11,4(r7)
    stw r12,0(r6)
    stw r11,4(r6)
    lwz r12,8(r7)
    lwz r11,12(r7)
    stw r12,8(r6)
    stw r11,12(r6)
    lwz r12,16(r7)
    lwz r11,20(r7)
    stw r12,16(r6)
    stw r11,20(r6)
    lwz r12,24(r7)
    lwz r11,28(r7)
    addi r7,r7,32
    stw r12,24(r6)
    stw r11,28(r6)
    addi r6,r6,32
    bdnz cpy_14
    b cpy_21
cpy_15:
    lwz r10,-1(r4)
    mtctr r9
    addi r7,r4,3
    slwi r10,r10,8
cpy_16:
    dcbz 0,r6
    lwz r12,0(r7)
    lwz r11,4(r7)
    rlwimi r10,r12,8,24,31
    slwi r8,r12,8
    stw r10,0(r6)
    rlwimi r8,r11,8,24,31
    slwi r9,r11,8
    lwz r12,8(r7)
    stw r8,4(r6)
    rlwimi r9,r12,8,24,31
    slwi r8,r12,8
    lwz r11,12(r7)
    stw r9,8(r6)
    rlwimi r8,r11,8,24,31
    slwi r10,r11,8
    lwz r12,16(r7)
    stw r8,12(r6)
    rlwimi r10,r12,8,24,31
    slwi r8,r12,8
    lwz r11,20(r7)
    stw r10,16(r6)
    rlwimi r8,r11,8,24,31
    slwi r9,r11,8
    lwz r12,24(r7)
    stw r8,20(r6)
    rlwimi r9,r12,8,24,31
    slwi r8,r12,8
    lwz r11,28(r7)
    addi r7,r7,32
    stw r9,24(r6)
    rlwimi r8,r11,8,24,31
    slwi r10,r11,8
    stw r8,28(r6)
    addi r6,r6,32
    bdnz cpy_16
    b cpy_21
cpy_17:
    lwz r10,-2(r4)
    mtctr r9
    addi r7,r4,2
    slwi r10,r10,16
cpy_18:
    dcbz 0,r6
    lwz r12,0(r7)
    lwz r11,4(r7)
    rlwimi r10,r12,16,16,31
    slwi r8,r12,16
    stw r10,0(r6)
    rlwimi r8,r11,16,16,31
    slwi r9,r11,16
    lwz r12,8(r7)
    stw r8,4(r6)
    rlwimi r9,r12,16,16,31
    slwi r8,r12,16
    lwz r11,12(r7)
    stw r9,8(r6)
    rlwimi r8,r11,16,16,31
    slwi r10,r11,16
    lwz r12,16(r7)
    stw r8,12(r6)
    rlwimi r10,r12,16,16,31
    slwi r8,r12,16
    lwz r11,20(r7)
    stw r10,16(r6)
    rlwimi r8,r11,16,16,31
    slwi r9,r11,16
    lwz r12,24(r7)
    stw r8,20(r6)
    rlwimi r9,r12,16,16,31
    slwi r8,r12,16
    lwz r11,28(r7)
    addi r7,r7,32
    stw r9,24(r6)
    rlwimi r8,r11,16,16,31
    slwi r10,r11,16
    stw r8,28(r6)
    addi r6,r6,32
    bdnz cpy_18
    b cpy_21
cpy_19:
    lwz r10,-3(r4)
    mtctr r9
    addi r7,r4,1
    slwi r10,r10,24
cpy_20:
    dcbz 0,r6
    lwz r12,0(r7)
    lwz r11,4(r7)
    rlwimi r10,r12,24,8,31
    slwi r8,r12,24
    stw r10,0(r6)
    rlwimi r8,r11,24,8,31
    slwi r9,r11,24
    lwz r12,8(r7)
    stw r8,4(r6)
    rlwimi r9,r12,24,8,31
    slwi r8,r12,24
    lwz r11,12(r7)
    stw r9,8(r6)
    rlwimi r8,r11,24,8,31
    slwi r10,r11,24
    lwz r12,16(r7)
    stw r8,12(r6)
    rlwimi r10,r12,24,8,31
    slwi r8,r12,24
    lwz r11,20(r7)
    stw r10,16(r6)
    rlwimi r8,r11,24,8,31
    slwi r9,r11,24
    lwz r12,24(r7)
    stw r8,20(r6)
    rlwimi r9,r12,24,8,31
    slwi r8,r12,24
    lwz r11,28(r7)
    addi r7,r7,32
    stw r9,24(r6)
    rlwimi r8,r11,24,8,31
    slwi r10,r11,24
    stw r8,28(r6)
    addi r6,r6,32
    bdnz cpy_20
cpy_21:
    add r3,r3,r31
    add r4,r4,r31
    subf r5,r31,r5
cpy_22:
    srwi. r7,r5,2
    clrlwi r6,r5,30
    beq cpy_24
    mtctr r7
cpy_23:
    lwz r5,0(r4)
    addi r4,r4,4
    stw r5,0(r3)
    addi r3,r3,4
    bdnz cpy_23
cpy_24:
    cmpwi r6,0
    beq cpy_26
    mtctr r6
cpy_25:
    lbz r5,0(r4)
    addi r4,r4,1
    stb r5,0(r3)
    addi r3,r3,1
    bdnz cpy_25
cpy_26:
    mr r3,r0
cpy_27:
    lwz r31,12(r1)
    addi r1,r1,16
    blr
}

/* 0x8051D60C (0x104): fills `size` bytes with the byte `value`, 32-byte blocks through `dcbz` when the run is long. */
/* untyped: byte range */
asm void* NETMemSet(void* dst, s32 value, u32 size) {
    nofralloc
    cmpwi r5,0
    mr r9,r3
    beqlr
    cmplwi r5,64
    rlwimi r4,r4,8,16,23
    rlwimi r4,r4,16,0,15
    blt set_10
    clrlwi. r8,r3,27
    beq set_5
    subfic r8,r8,32
    mr r7,r3
    srwi. r6,r8,2
    clrlwi r0,r8,30
    beq set_2
    mtctr r6
set_1:
    stw r4,0(r7)
    addi r7,r7,4
    bdnz set_1
set_2:
    cmpwi r0,0
    beq set_4
    mtctr r0
set_3:
    stb r4,0(r7)
    addi r7,r7,1
    bdnz set_3
set_4:
    add r3,r3,r8
    subf r5,r8,r5
set_5:
    cmpwi r4,0
    mr r6,r3
    clrrwi r7,r5,5
    srwi r0,r5,5
    bne set_7
    mtctr r0
set_6:
    dcbz 0,r6
    addi r6,r6,32
    bdnz set_6
    b set_9
set_7:
    mtctr r0
set_8:
    dcbz 0,r6
    stw r4,0(r6)
    stw r4,4(r6)
    stw r4,8(r6)
    stw r4,12(r6)
    stw r4,16(r6)
    stw r4,20(r6)
    stw r4,24(r6)
    stw r4,28(r6)
    addi r6,r6,32
    bdnz set_8
set_9:
    add r3,r3,r7
    subf r5,r7,r5
set_10:
    srwi. r6,r5,2
    clrlwi r0,r5,30
    beq set_12
    mtctr r6
set_11:
    stw r4,0(r3)
    addi r3,r3,4
    bdnz set_11
set_12:
    cmpwi r0,0
    beq set_14
    mtctr r0
set_13:
    stb r4,0(r3)
    addi r3,r3,1
    bdnz set_13
set_14:
    mr r3,r9
    blr
}
