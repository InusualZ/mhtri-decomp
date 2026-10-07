/*
 * OS/OSContext.h - the processor context record and the entry points of `OS/OSContext.c` that other units call.
 */
#ifndef OS_OSCONTEXT_H
#define OS_OSCONTEXT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct OSContext {
    /* +0x000 */ u32 gpr[32];
    /* +0x080 */ u32 cr;
    /* +0x084 */ u32 lr;
    /* +0x088 */ u32 ctr;
    /* +0x08C */ u32 xer;
    /* +0x090 */ f64 fpr[32];
    /* +0x190 */ u32 fpscr_pad;
    /* +0x194 */ u32 fpscr;
    /* +0x198 */ u32 srr0;
    /* +0x19C */ u32 srr1;
    /* +0x1A0 */ u16 mode;
    /* +0x1A2 */ u16 state;
    /* +0x1A4 */ u32 gqr[8];
    /* +0x1C4 */ u32 psf_pad;
    /* +0x1C8 */ f64 psf[32];
} OSContext; /* size: 0x2C8 */

/* 0x804CD0A0 - restores a context and resumes it; does not return. */
void OSLoadContext(OSContext* context);
/* 0x804CD1F0 - zeroes a context's state. */
void OSClearContext(OSContext* context);
/* 0x804CD2E0 - prints a context's registers through `OSReport`. */
void OSDumpContext(OSContext* context);
/* 0x804CCFB0 - makes a context the running thread's current one. */
void OSSetCurrentContext(OSContext* context);

#ifdef __cplusplus
}
#endif

#endif
