/*
 * OS/PPCArch.c - the processor-architecture accessors: SPR/MSR/FPSCR reads and writes, the halt loop and the HID4 setter.
 * RANGE. .text 0x804770E0-0x804772F0 (23 functions); .data 0x80612CE0-0x80612D18.  Evidence: every symbol in the range
 *    is a single special-purpose-register accessor or a caller of one; `PPCMthid4` is the only reader of the 0x38-byte
 *    string at 0x80612CE0 that it hands to `OSReport`; `AXFXGetHooks` (AX) ends at 0x804770D4 below and an unrelated
 *    544-byte function starts at 0x804772F0 above.
 * FLAGS. the library flags (configure.py); `#pragma function_align 16` restores the 16-byte function starts the
 *    -O4,p library flags would otherwise leave at 4.
 * NAMES. map names; GUESS: `PPCMtmmcr0`, `PPCMtmmcr1`, `PPCMtpmc1`, `PPCMtpmc2`, `PPCMtpmc3`, `PPCMtpmc4`, `PPCMtwpar` (the dump calls these `__setMMCR0`,
 *    `__setPMC1` .. `__setWPAR`; the bodies are the SPR writes).
 * RESIDUALS. none known beyond the report rows.
 * SHAPES. the accessors are asm functions with `nofralloc` (playbook 104); `PPCMthid4` is C with an asm SPR write.
 */

#pragma function_align 16

#include "types.h"

#include "OS/OSError.h"
#include "OS/PPCArch.h"
#include "OS/PPCHalt.h"
#include "OS/PPCMtdec.h"

asm u32 PPCMfmsr(void)
{
    nofralloc
    mfmsr r3
    blr
}

asm void PPCMtmsr(u32 value)
{
    nofralloc
    mtmsr r3
    blr
}

asm u32 PPCMfhid0(void)
{
    nofralloc
    mfspr r3, HID0
    blr
}

asm void __setHID0(u32 value)
{
    nofralloc
    mtspr HID0, r3
    blr
}

asm u32 PPCMfl2cr(void)
{
    nofralloc
    mfspr r3, L2CR
    blr
}

asm void PPCMtl2cr(u32 value)
{
    nofralloc
    mtspr L2CR, r3
    blr
}

asm void PPCMtdec(u32 value)
{
    nofralloc
    mtspr DEC, r3
    blr
}

asm void PPCSync(void)
{
    nofralloc
    sc
    blr
}

asm void PPCHalt(void)
{
    nofralloc
    sync
loop:
    nop
    li r3, 0
    nop
    b loop
}

asm void PPCMtmmcr0(u32 value)
{
    nofralloc
    mtspr MMCR0, r3
    blr
}

asm void PPCMtmmcr1(u32 value)
{
    nofralloc
    mtspr MMCR1, r3
    blr
}

asm void PPCMtpmc1(u32 value)
{
    nofralloc
    mtspr PMC1, r3
    blr
}

asm void PPCMtpmc2(u32 value)
{
    nofralloc
    mtspr PMC2, r3
    blr
}

asm void PPCMtpmc3(u32 value)
{
    nofralloc
    mtspr PMC3, r3
    blr
}

asm void PPCMtpmc4(u32 value)
{
    nofralloc
    mtspr PMC4, r3
    blr
}

asm u32 PPCMffpscr(void)
{
    nofralloc
    stwu r1, -32(r1)
    stfd f31, 24(r1)
    mffs f31
    stfd f31, 8(r1)
    lfd f31, 24(r1)
    lwz r3, 12(r1)
    addi r1, r1, 32
    blr
}

asm void PPCMtfpscr(u32 value)
{
    nofralloc
    stwu r1, -32(r1)
    stfd f31, 24(r1)
    li r4, 0
    stw r4, 8(r1)
    stw r3, 12(r1)
    lfd f31, 8(r1)
    mtfsf 255, f31
    lfd f31, 24(r1)
    addi r1, r1, 32
    blr
}

asm u32 PPCMfhid2(void)
{
    nofralloc
    mfspr r3, 920
    blr
}

asm void PPCMthid2(u32 value)
{
    nofralloc
    mtspr 920, r3
    blr
}

asm void PPCMtwpar(u32 value)
{
    nofralloc
    mtspr WPAR, r3
    blr
}

asm void PPCDisableSpeculation(void)
{
    nofralloc
    stwu r1, -16(r1)
    mflr r0
    stw r0, 20(r1)
    bl PPCMfhid0
    ori r3, r3, 512
    bl __setHID0
    lwz r0, 20(r1)
    mtlr r0
    addi r1, r1, 16
    blr
}

asm void PPCSetFpNonIEEEMode(void)
{
    nofralloc
    mtfsb1 29
    blr
}

static char s_hid4ErrataMessage[] = "H4A should not be cleared because of Broadway errata.\n";

/* Writes HID4; when the caller clears bit 31 it reports the Broadway erratum and forces the bit back on. */
asm void PPCMthid4(u32 value)
{
    nofralloc
    stwu r1, -16(r1)
    mflr r0
    stw r0, 20(r1)
    clrrwi. r0, r3, 31
    stw r31, 12(r1)
    mr r31, r3
    beq skip
    mtspr 1011, r3
    b done
skip:
    lis r3, s_hid4ErrataMessage@ha
    addi r3, r3, s_hid4ErrataMessage@l
    crclr 4*cr1+eq
    bl OSReport
    oris r31, r31, 0x8000
    mtspr 1011, r31
done:
    lwz r0, 20(r1)
    lwz r31, 12(r1)
    mtlr r0
    addi r1, r1, 16
    blr
}
