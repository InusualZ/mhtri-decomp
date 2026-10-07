/*
 * OS/OSError.c - the OS error reporting: `OSReport`, `OSPanic`, the error-handler table and `__OSUnhandledException`.
 * RANGE. .text 0x804CD620-0x804CDD70 (5 functions); .data 0x8061C570-0x8061C850; .bss 0x8074D2F0-0x8074D340; .sdata
 *    0x80793F90-0x80793F98.  Cut from the OS core band 0x804C1760-0x804D9B4C.  Evidence: .data 0x8061C570 (" in
 *    \"%s\" on line %d") is read by `OSPanic` and `__OSUnhandledException`, and the table of 0x44 bytes
 *    `__OSErrorTable` (.bss 0x8074D2F0) by `OSSetErrorHandler` and `__OSUnhandledException`; the argument-packing
 *    helper at 0x804CDD70 (strlen/strcpy/memset) is not error code: it has a twin at 0x804D71F0 and heads the
 *    exec unit.
 * FLAGS. `cflags_base` per object (configure.py).
 * NAMES. map names throughout; `__OSFpscrEnableBits` (GUESS, the FPSCR exception-enable mask).
 * RESIDUALS. none recorded; the `.data`/`.bss`/`.sdata` tails differ from the claims only by link padding.
 * SHAPES. plain C over the PPC intrinsics; the varargs entries use `va_start`.
 */

#include "types.h"
#include "stdarg.h"

#include "MSL_C/printf.h"
#include "OS/OSContext.h"
#include "OS/OSError.h"
#include "OS/OSInterrupt.h"
#include "OS/OSThread.h"
#include "OS/OSTime.h"
#include "OS/OSVReport.h"
#include "OS/PPCArch.h"
#include "OS/PPCHalt.h"

#define OS_ACTIVE_THREAD_QUEUE (*(OSThreadQueue*)0x800000DC)
#define OS_FPU_CONTEXT (*(OSContext**)0x800000D8)

#define OS_ERROR_MACHINE_CHECK 1
#define OS_ERROR_DSI 2
#define OS_ERROR_ISI 3
#define OS_ERROR_ALIGNMENT 5
#define OS_ERROR_PROGRAM 6
#define OS_ERROR_FPE 16
#define OS_ERROR_MEMORY_PROTECTION 15

#define MSR_FP 0x2000
#define MSR_FE 0x900
#define FPSCR_KEEP_MASK 0x6005F8FF

u32 __OSFpscrEnableBits = 0xF8;
OSErrorHandler __OSErrorTable[17];

/* Prints a formatted message through the console. */
void OSReport(const char* format, ...)
{
    va_list args;

    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

/* Prints a formatted message from an argument list. */
void OSVReport(const char* format, va_list args)
{
    vprintf(format, args);
}

/* Prints the message, the source position and a stack back chain, then halts the processor. */
void OSPanic(const char* file, int line, const char* msg, ...)
{
    va_list args;
    u32 i;
    u32* sp;

    OSDisableInterrupts();
    va_start(args, msg);
    vprintf(msg, args);
    va_end(args);
    OSReport(" in \"%s\" on line %d.\n", file, line);
    OSReport("\nAddress:      Back Chain    LR Save\n");
    for (i = 0, sp = (u32*)OSGetStackPointer(); sp && (u32)sp != 0xFFFFFFFF && i++ < 16; sp = (u32*)*sp) {
        OSReport("0x%08x:   0x%08x    0x%08x\n", sp, sp[0], sp[1]);
    }
    PPCHalt();
}

/* Installs the handler for processor error `error`, switching the FP exception state when it is the FPE handler. */
OSErrorHandler OSSetErrorHandler(u16 error, OSErrorHandler handler)
{
    OSErrorHandler old;
    BOOL enabled;
    u32 msr;
    u32 fpscr;
    OSThread* thread;
    u32 i;

    enabled = OSDisableInterrupts();
    old = __OSErrorTable[error];
    __OSErrorTable[error] = handler;
    if (error == OS_ERROR_FPE) {
        msr = PPCMfmsr();
        PPCMtmsr(msr | MSR_FP);
        fpscr = PPCMffpscr();
        if (handler) {
            for (thread = OS_ACTIVE_THREAD_QUEUE.head; thread; thread = thread->linkActive.next) {
                thread->context.srr1 |= MSR_FE;
                if (!(thread->context.state & 1)) {
                    thread->context.state |= 1;
                    for (i = 0; i < 32; i++) {
                        *(u64*)&thread->context.fpr[i] = 0xFFFFFFFFFFFFFFFF;
                        *(u64*)&thread->context.psf[i] = 0xFFFFFFFFFFFFFFFF;
                    }
                    thread->context.fpscr = 4;
                }
                thread->context.fpscr |= __OSFpscrEnableBits & 0xF8;
                thread->context.fpscr &= FPSCR_KEEP_MASK;
            }
            msr |= MSR_FE;
            fpscr |= __OSFpscrEnableBits & 0xF8;
        } else {
            for (thread = OS_ACTIVE_THREAD_QUEUE.head; thread; thread = thread->linkActive.next) {
                thread->context.srr1 &= ~MSR_FE;
                thread->context.fpscr &= ~0xF8;
                thread->context.fpscr &= FPSCR_KEEP_MASK;
            }
            fpscr &= ~0xF8;
            msr &= ~MSR_FE;
        }
        PPCMtfpscr(fpscr & FPSCR_KEEP_MASK);
        PPCMtmsr(msr);
    }
    OSRestoreInterrupts(enabled);
    return old;
}

/* Runs the installed handler for an unrecoverable exception or reports it and halts. */
void __OSUnhandledException(u8 error, OSContext* context, u32 dsisr, u32 dar)
{
    s64 now = OSGetTime();
    u32 msr;

    if (!(context->srr1 & 2)) {
        OSReport("Non-recoverable Exception %d", error);
    } else {
        if (error == OS_ERROR_PROGRAM && (context->srr1 & 0x100000) && __OSErrorTable[OS_ERROR_FPE]) {
            error = OS_ERROR_FPE;
            msr = PPCMfmsr();
            PPCMtmsr(msr | MSR_FP);
            if (OS_FPU_CONTEXT) {
                OSSaveFPUContext(OS_FPU_CONTEXT);
            }
            PPCMtfpscr(PPCMffpscr() & FPSCR_KEEP_MASK);
            PPCMtmsr(msr);
            if (OS_FPU_CONTEXT == context) {
                OSDisableScheduler();
                __OSErrorTable[OS_ERROR_FPE](OS_ERROR_FPE, context, dsisr, dar);
                context->srr1 &= ~MSR_FP;
                OS_FPU_CONTEXT = NULL;
                context->fpscr &= FPSCR_KEEP_MASK;
                OSEnableScheduler();
                __OSReschedule();
            } else {
                context->srr1 &= ~MSR_FP;
                OS_FPU_CONTEXT = NULL;
            }
            OSLoadContext(context);
        }
        if (__OSErrorTable[error]) {
            OSDisableScheduler();
            __OSErrorTable[error](error, context, dsisr, dar);
            OSEnableScheduler();
            __OSReschedule();
            OSLoadContext(context);
        }
        if (error == 8) {
            OSLoadContext(context);
        }
        OSReport("Unhandled Exception %d", error);
    }
    OSReport("\n");
    OSDumpContext(context);
    OSReport("\nDSISR = 0x%08x                   DAR  = 0x%08x\n", dsisr, dar);
    OSReport("TB = 0x%016llx\n", now);
    switch (error) {
    case OS_ERROR_DSI:
        OSReport("\nInstruction at 0x%x (read from SRR0) attempted to access invalid address 0x%x (read from DAR)\n",
                 context->srr0, dar);
        break;
    case OS_ERROR_ISI:
        OSReport("\nAttempted to fetch instruction from invalid address 0x%x (read from SRR0)\n", context->srr0);
        break;
    case OS_ERROR_ALIGNMENT:
        OSReport("\nInstruction at 0x%x (read from SRR0) attempted to access unaligned address 0x%x (read from DAR)\n",
                 context->srr0, dar);
        break;
    case OS_ERROR_PROGRAM:
        OSReport("\nProgram exception : Possible illegal instruction/operation at or around 0x%x (read from SRR0)\n",
                 context->srr0, dar);
        break;
    case OS_ERROR_MEMORY_PROTECTION:
        OSReport("\n");
        OSReport("AI DMA Address =   0x%04x%04x\n", *(volatile u16*)0xCC005030, *(volatile u16*)0xCC005032);
        OSReport("ARAM DMA Address = 0x%04x%04x\n", *(volatile u16*)0xCC005020, *(volatile u16*)0xCC005022);
        OSReport("DI DMA Address =   0x%08x\n", *(volatile u32*)0xCD006014);
        break;
    }
    OSReport("\nLast interrupt (%d): SRR0 = 0x%08x  TB = 0x%016llx\n", __OSLastInterrupt, __OSLastInterruptSrr0,
             __OSLastInterruptTime);
    PPCHalt();
}
