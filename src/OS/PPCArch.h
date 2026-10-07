/*
 * OS/PPCArch.h - declarations of the symbols owned by `OS/PPCArch.c` that other units call; `PPCHalt` and `PPCMtdec`
 * live in their leaf headers `OS/PPCHalt.h` and `OS/PPCMtdec.h`.
 */
#ifndef OS_PPCARCH_H
#define OS_PPCARCH_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804770E0 / 0x804770F0 - reads / writes the machine state register. */
u32 PPCMfmsr(void);
void PPCMtmsr(u32 value);

/* 0x80477100 / 0x80477110 - reads / writes HID0 (`__setHID0` is the raw write). */
u32 PPCMfhid0(void);
void __setHID0(u32 value);

/* 0x80477120 / 0x80477130 - reads / writes the L2 cache control register. */
u32 PPCMfl2cr(void);
void PPCMtl2cr(u32 value);

/* 0x80477150 - issues a `sync`. */
void PPCSync(void);

/* 0x80477180..0x804771D0 - write the performance-monitor control and counter registers. */
void PPCMtmmcr0(u32 value);
void PPCMtmmcr1(u32 value);
void PPCMtpmc1(u32 value);
void PPCMtpmc2(u32 value);
void PPCMtpmc3(u32 value);
void PPCMtpmc4(u32 value);

/* 0x804771E0 / 0x80477200 - reads / writes the low word of the FPSCR. */
u32 PPCMffpscr(void);
void PPCMtfpscr(u32 value);

/* 0x80477230 - reads HID2. */
u32 PPCMfhid2(void);
void PPCMthid2(u32 value);

/* 0x80477250 - writes the gather-pipe address register. */
void PPCMtwpar(u32 value);

/* 0x80477260 / 0x80477290 - turns speculative execution off / puts the FPU into non-IEEE mode. */
void PPCDisableSpeculation(void);
void PPCSetFpNonIEEEMode(void);

/* 0x804772A0 - writes HID4, forcing the errata-required bit on. */
void PPCMthid4(u32 value);

#ifdef __cplusplus
}
#endif

#endif
