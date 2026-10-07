/*
 * OS/OSContext.c - the OS context: FPU save/load, context save/load/clear, fiber switch, `OSDumpContext` and
 *    `__OSContextInit`.
 * RANGE. .text 0x804CCD40-0x804CD620 (15 functions); .data 0x8061C390-0x8061C570.  Cut from the OS core band
 *    0x804C1760-0x804D9B4C.  Evidence: .data 0x8061C390..0x8061C570 (the register-dump format strings and the
 *    "FPU-unavailable handler installed" string) is read only by `OSDumpContext` and `__OSContextInit`.
 * FLAGS. `cflags_base` per object (configure.py): the target's function starts are all 16-aligned.
 * NAMES. map names (GUESS: `OSSaveFPUContext`, `OSLoadContext`, `OSSwitchFPUContext`, `__OSContextInit`, which the dump has as
 *    placeholders; their bodies are the FPU save, the context restore, the FPU-unavailable handler and the handler installer)
 *    except GUESS: `OSGetCurrentContext` (reads the current-context word of the low-memory arena at
 *    0x800000D4) and `OSInitContext` (zeroes a context and stores the entry point and stack pointer it is given).
 * RESIDUALS. `OSDumpContext` (98.9 %): the saved registers are permuted (context r29, string base r27 in ours; r30, r28 in the
 *    target) and the string pool base is `...data.0` where the target names it `lbl_8061C390`; no declaration order changed it.
 * SHAPES. the register save/restore bodies are asm functions with `nofralloc` (playbook 104) and use numbered SPRs where
 *    this compiler's name differs; `OSDumpContext` and `__OSContextInit` are C.
 */

#include "types.h"

#include "DB/DBPrintf.h"
#include "OS/OS.h"
#include "OS/OSContext.h"
#include "OS/OSError.h"
#include "OS/OSInterrupt.h"
#include "OS/OSSwitchFiberEx.h"

/* The low-memory words the context code shares: the running context and the FPU's current owner. */
#define OS_CURRENT_CONTEXT (*(OSContext**)0x800000D4)
#define OS_FPU_CONTEXT (*(OSContext**)0x800000D8)

#define OS_DUMP_STACK_DEPTH 16

/* Loads the floating-point registers and FPSCR from `context`. */
asm void __OSLoadFPUContext(u32 unused, OSContext* context)
{
    nofralloc
    lhz r5, 418(r4)
    clrlwi. r5, r5, 31
    beq L120
    lfd f0, 400(r4)
    mtfsf 255, f0
    mfspr r5, 920
    rlwinm. r5, r5, 3, 31, 31
    beq La0
    psq_l f0, 456(r4), 0, 0
    psq_l f1, 464(r4), 0, 0
    psq_l f2, 472(r4), 0, 0
    psq_l f3, 480(r4), 0, 0
    psq_l f4, 488(r4), 0, 0
    psq_l f5, 496(r4), 0, 0
    psq_l f6, 504(r4), 0, 0
    psq_l f7, 512(r4), 0, 0
    psq_l f8, 520(r4), 0, 0
    psq_l f9, 528(r4), 0, 0
    psq_l f10, 536(r4), 0, 0
    psq_l f11, 544(r4), 0, 0
    psq_l f12, 552(r4), 0, 0
    psq_l f13, 560(r4), 0, 0
    psq_l f14, 568(r4), 0, 0
    psq_l f15, 576(r4), 0, 0
    psq_l f16, 584(r4), 0, 0
    psq_l f17, 592(r4), 0, 0
    psq_l f18, 600(r4), 0, 0
    psq_l f19, 608(r4), 0, 0
    psq_l f20, 616(r4), 0, 0
    psq_l f21, 624(r4), 0, 0
    psq_l f22, 632(r4), 0, 0
    psq_l f23, 640(r4), 0, 0
    psq_l f24, 648(r4), 0, 0
    psq_l f25, 656(r4), 0, 0
    psq_l f26, 664(r4), 0, 0
    psq_l f27, 672(r4), 0, 0
    psq_l f28, 680(r4), 0, 0
    psq_l f29, 688(r4), 0, 0
    psq_l f30, 696(r4), 0, 0
    psq_l f31, 704(r4), 0, 0
La0:
    lfd f0, 144(r4)
    lfd f1, 152(r4)
    lfd f2, 160(r4)
    lfd f3, 168(r4)
    lfd f4, 176(r4)
    lfd f5, 184(r4)
    lfd f6, 192(r4)
    lfd f7, 200(r4)
    lfd f8, 208(r4)
    lfd f9, 216(r4)
    lfd f10, 224(r4)
    lfd f11, 232(r4)
    lfd f12, 240(r4)
    lfd f13, 248(r4)
    lfd f14, 256(r4)
    lfd f15, 264(r4)
    lfd f16, 272(r4)
    lfd f17, 280(r4)
    lfd f18, 288(r4)
    lfd f19, 296(r4)
    lfd f20, 304(r4)
    lfd f21, 312(r4)
    lfd f22, 320(r4)
    lfd f23, 328(r4)
    lfd f24, 336(r4)
    lfd f25, 344(r4)
    lfd f26, 352(r4)
    lfd f27, 360(r4)
    lfd f28, 368(r4)
    lfd f29, 376(r4)
    lfd f30, 384(r4)
    lfd f31, 392(r4)
L120:
    blr
}

/* Saves the floating-point registers and FPSCR into `context`. */
asm void __OSSaveFPUContext(u32 unused0, u32 unused1, OSContext* context)
{
    nofralloc
    lhz r3, 418(r5)
    ori r3, r3, 1
    sth r3, 418(r5)
    stfd f0, 144(r5)
    stfd f1, 152(r5)
    stfd f2, 160(r5)
    stfd f3, 168(r5)
    stfd f4, 176(r5)
    stfd f5, 184(r5)
    stfd f6, 192(r5)
    stfd f7, 200(r5)
    stfd f8, 208(r5)
    stfd f9, 216(r5)
    stfd f10, 224(r5)
    stfd f11, 232(r5)
    stfd f12, 240(r5)
    stfd f13, 248(r5)
    stfd f14, 256(r5)
    stfd f15, 264(r5)
    stfd f16, 272(r5)
    stfd f17, 280(r5)
    stfd f18, 288(r5)
    stfd f19, 296(r5)
    stfd f20, 304(r5)
    stfd f21, 312(r5)
    stfd f22, 320(r5)
    stfd f23, 328(r5)
    stfd f24, 336(r5)
    stfd f25, 344(r5)
    stfd f26, 352(r5)
    stfd f27, 360(r5)
    stfd f28, 368(r5)
    stfd f29, 376(r5)
    stfd f30, 384(r5)
    stfd f31, 392(r5)
    mffs f0
    stfd f0, 400(r5)
    lfd f0, 144(r5)
    mfspr r3, 920
    rlwinm. r3, r3, 3, 31, 31
    beq L254
    psq_st f0, 456(r5), 0, 0
    psq_st f1, 464(r5), 0, 0
    psq_st f2, 472(r5), 0, 0
    psq_st f3, 480(r5), 0, 0
    psq_st f4, 488(r5), 0, 0
    psq_st f5, 496(r5), 0, 0
    psq_st f6, 504(r5), 0, 0
    psq_st f7, 512(r5), 0, 0
    psq_st f8, 520(r5), 0, 0
    psq_st f9, 528(r5), 0, 0
    psq_st f10, 536(r5), 0, 0
    psq_st f11, 544(r5), 0, 0
    psq_st f12, 552(r5), 0, 0
    psq_st f13, 560(r5), 0, 0
    psq_st f14, 568(r5), 0, 0
    psq_st f15, 576(r5), 0, 0
    psq_st f16, 584(r5), 0, 0
    psq_st f17, 592(r5), 0, 0
    psq_st f18, 600(r5), 0, 0
    psq_st f19, 608(r5), 0, 0
    psq_st f20, 616(r5), 0, 0
    psq_st f21, 624(r5), 0, 0
    psq_st f22, 632(r5), 0, 0
    psq_st f23, 640(r5), 0, 0
    psq_st f24, 648(r5), 0, 0
    psq_st f25, 656(r5), 0, 0
    psq_st f26, 664(r5), 0, 0
    psq_st f27, 672(r5), 0, 0
    psq_st f28, 680(r5), 0, 0
    psq_st f29, 688(r5), 0, 0
    psq_st f30, 696(r5), 0, 0
    psq_st f31, 704(r5), 0, 0
L254:
    blr
}

/* Saves the floating-point registers into `context`. */
asm void OSSaveFPUContext(OSContext* context)
{
    nofralloc
    addi r5, r3, 0
    b __OSSaveFPUContext
}

/* Makes `context` the running context and sets the MSR FP bit to match its FP ownership. */
asm void OSSetCurrentContext(OSContext* context)
{
    nofralloc
    lis r4, -32768
    stw r3, 212(r4)
    clrlwi r5, r3, 2
    stw r5, 192(r4)
    lwz r5, 216(r4)
    cmpw r5, r3
    bne L2a8
    lwz r6, 412(r3)
    ori r6, r6, 8192
    stw r6, 412(r3)
    mfmsr r6
    ori r6, r6, 2
    mtmsr r6
    blr
L2a8:
    lwz r6, 412(r3)
    rlwinm r6, r6, 0, 19, 17
    stw r6, 412(r3)
    mfmsr r6
    rlwinm r6, r6, 0, 19, 17
    ori r6, r6, 2
    mtmsr r6
    isync
    blr
}

/* Returns the running context. */
asm OSContext* OSGetCurrentContext(void)
{
    nofralloc
    lis r3, -32768
    lwz r3, 212(r3)
    blr
}

/* Saves the callee-saved registers into `context`; returns 0 now and 1 when it is resumed. */
asm BOOL OSSaveContext(OSContext* context)
{
    nofralloc
    stmw r13, 52(r3)
    mfspr r0, GQR1
    stw r0, 424(r3)
    mfspr r0, GQR2
    stw r0, 428(r3)
    mfspr r0, GQR3
    stw r0, 432(r3)
    mfspr r0, GQR4
    stw r0, 436(r3)
    mfspr r0, GQR5
    stw r0, 440(r3)
    mfspr r0, GQR6
    stw r0, 444(r3)
    mfspr r0, GQR7
    stw r0, 448(r3)
    mfcr r0
    stw r0, 128(r3)
    mflr r0
    stw r0, 132(r3)
    stw r0, 408(r3)
    mfmsr r0
    stw r0, 412(r3)
    mfctr r0
    stw r0, 136(r3)
    mfxer r0
    stw r0, 140(r3)
    stw r1, 4(r3)
    stw r2, 8(r3)
    li r0, 1
    stw r0, 12(r3)
    li r3, 0
    blr
}

/* Restores `context` and resumes it. */
asm void OSLoadContext(OSContext* context)
{
    nofralloc
    lis r4, OSDisableInterrupts@ha
    lwz r6, 408(r3)
    addi r5, r4, OSDisableInterrupts@l
    cmplw r6, r5
    ble L388
    lis r4, OSDisableInterrupts+0xc@ha
    addi r0, r4, OSDisableInterrupts+0xc@l
    cmplw r6, r0
    bge L388
    stw r5, 408(r3)
L388:
    lwz r0, 0(r3)
    lwz r1, 4(r3)
    lwz r2, 8(r3)
    lhz r4, 418(r3)
    rlwinm. r5, r4, 0, 30, 30
    beq L3b0
    rlwinm r4, r4, 0, 31, 29
    sth r4, 418(r3)
    lmw r5, 20(r3)
    b L3b4
L3b0:
    lmw r13, 52(r3)
L3b4:
    lwz r4, 424(r3)
    mtspr GQR1, r4
    lwz r4, 428(r3)
    mtspr GQR2, r4
    lwz r4, 432(r3)
    mtspr GQR3, r4
    lwz r4, 436(r3)
    mtspr GQR4, r4
    lwz r4, 440(r3)
    mtspr GQR5, r4
    lwz r4, 444(r3)
    mtspr GQR6, r4
    lwz r4, 448(r3)
    mtspr GQR7, r4
    lwz r4, 128(r3)
    mtcr r4
    lwz r4, 132(r3)
    mtlr r4
    lwz r4, 136(r3)
    mtctr r4
    lwz r4, 140(r3)
    mtxer r4
    mfmsr r4
    rlwinm r4, r4, 0, 17, 15
    rlwinm r4, r4, 0, 31, 29
    mtmsr r4
    lwz r4, 408(r3)
    mtspr SRR0, r4
    lwz r4, 412(r3)
    mtspr SRR1, r4
    lwz r4, 16(r3)
    lwz r3, 12(r3)
    rfi
}

/* Returns the caller's stack pointer. */
asm u32 OSGetStackPointer(void)
{
    nofralloc
    mr r3, r1
    blr
}

/* Runs `func` on the stack `stack` and returns to the old stack. */
/* untyped: opaque handle passed through - the new stack */
asm void OSSwitchFiber(void (*func)(void), void* stack)
{
    nofralloc
    mflr r0
    mr r5, r1
    stwu r5, -8(r4)
    mr r1, r4
    stw r0, 4(r5)
    mtlr r3
    blrl
    lwz r5, 0(r1)
    lwz r0, 4(r5)
    mtlr r0
    mr r1, r5
    blr
}

/* Runs `func` on the stack `stack`, with four arguments, and returns to the old stack. */
asm void OSSwitchFiberEx(u32 arg0, u32 arg1, u32 arg2, u32 arg3, void (*func)(void), u8* stack)
{
    nofralloc
    mflr r0
    mr r9, r1
    stwu r9, -8(r8)
    mr r1, r8
    stw r0, 4(r9)
    mtlr r7
    blrl
    lwz r5, 0(r1)
    lwz r0, 4(r5)
    mtlr r0
    mr r1, r5
    blr
}

/* Marks `context` empty and releases the FPU ownership when it held it. */
void OSClearContext(OSContext* context)
{
    context->mode = 0;
    context->state = 0;
    if (context == OS_FPU_CONTEXT) {
        OS_FPU_CONTEXT = NULL;
    }
}

/* Zeroes `context` and sets its entry point and stack pointer. */
asm void OSInitContext(OSContext* context, u32 pc, u32 sp)
{
    nofralloc
    stw r4, 408(r3)
    stw r5, 4(r3)
    li r11, 0
    ori r11, r11, 36914
    stw r11, 412(r3)
    li r0, 0
    stw r0, 128(r3)
    stw r0, 140(r3)
    stw r2, 8(r3)
    stw r13, 52(r3)
    stw r0, 12(r3)
    stw r0, 16(r3)
    stw r0, 20(r3)
    stw r0, 24(r3)
    stw r0, 28(r3)
    stw r0, 32(r3)
    stw r0, 36(r3)
    stw r0, 40(r3)
    stw r0, 44(r3)
    stw r0, 48(r3)
    stw r0, 56(r3)
    stw r0, 60(r3)
    stw r0, 64(r3)
    stw r0, 68(r3)
    stw r0, 72(r3)
    stw r0, 76(r3)
    stw r0, 80(r3)
    stw r0, 84(r3)
    stw r0, 88(r3)
    stw r0, 92(r3)
    stw r0, 96(r3)
    stw r0, 100(r3)
    stw r0, 104(r3)
    stw r0, 108(r3)
    stw r0, 112(r3)
    stw r0, 116(r3)
    stw r0, 120(r3)
    stw r0, 124(r3)
    stw r0, 420(r3)
    stw r0, 424(r3)
    stw r0, 428(r3)
    stw r0, 432(r3)
    stw r0, 436(r3)
    stw r0, 440(r3)
    stw r0, 444(r3)
    stw r0, 448(r3)
    b OSClearContext
}

/* Prints the integer, special, graphics-quantisation, floating-point and paired-single registers of `context`, then walks its stack. */
void OSDumpContext(OSContext* context)
{
    u32 i;
    u32* p;
    BOOL enabled;
    OSContext* current;
    OSContext fpContext;

    OSReport("------------------------- Context 0x%08x -------------------------\n", context);
    for (i = 0; i < 16; i++) {
        OSReport("r%-2d  = 0x%08x (%14d)  r%-2d  = 0x%08x (%14d)\n", i, context->gpr[i], context->gpr[i], i + 16,
                 context->gpr[i + 16], context->gpr[i + 16]);
    }
    OSReport("LR   = 0x%08x                   CR   = 0x%08x\n", context->lr, context->cr);
    OSReport("SRR0 = 0x%08x                   SRR1 = 0x%08x\n", context->srr0, context->srr1);
    OSReport("\nGQRs----------\n");
    for (i = 0; i < 4; i++) {
        OSReport("gqr%d = 0x%08x \t gqr%d = 0x%08x\n", i, context->gqr[i], i + 4, context->gqr[i + 4]);
    }
    if (context->state & 1) {
        enabled = OSDisableInterrupts();
        current = OS_CURRENT_CONTEXT;
        OSClearContext(&fpContext);
        OSSetCurrentContext(&fpContext);
        OSReport("\n\nFPRs----------\n");
        for (i = 0; i < 32; i += 2) {
            OSReport("fr%d \t= %d \t fr%d \t= %d\n", i, (u32)context->fpr[i], i + 1, (u32)context->fpr[i + 1]);
        }
        OSReport("\n\nPSFs----------\n");
        for (i = 0; i < 32; i += 2) {
            OSReport("ps%d \t= 0x%x \t ps%d \t= 0x%x\n", i, (u32)context->psf[i], i + 1, (u32)context->psf[i + 1]);
        }
        OSClearContext(&fpContext);
        OSSetCurrentContext(current);
        OSRestoreInterrupts(enabled);
    }
    OSReport("\nAddress:      Back Chain    LR Save\n");
    for (i = 0, p = (u32*)context->gpr[1]; p != NULL && (u32)p != 0xFFFFFFFF && i++ < OS_DUMP_STACK_DEPTH; p = (u32*)*p) {
        OSReport("0x%08x:   0x%08x    0x%08x\n", p, p[0], p[1]);
    }
}

/* Handles the FPU-unavailable exception: gives the FPU to the faulting context and resumes it. */
static asm void OSSwitchFPUContext(u8 exception, OSContext* context)
{
    nofralloc
    mfmsr r5
    ori r5, r5, 8192
    mtmsr r5
    isync
    lwz r5, 412(r4)
    ori r5, r5, 8192
    mtspr SRR1, r5
    lis r3, -32768
    lwz r5, 216(r3)
    stw r4, 216(r3)
    cmpw r5, r4
    beq L_840
    cmpwi r5, 0
    beq L_83c
    bl __OSSaveFPUContext
L_83c:
    bl __OSLoadFPUContext
L_840:
    lwz r3, 128(r4)
    mtcr r3
    lwz r3, 132(r4)
    mtlr r3
    lwz r3, 408(r4)
    mtspr SRR0, r3
    lwz r3, 136(r4)
    mtctr r3
    lwz r3, 140(r4)
    mtxer r3
    lhz r3, 418(r4)
    rlwinm r3, r3, 0, 31, 29
    sth r3, 418(r4)
    lwz r5, 20(r4)
    lwz r3, 12(r4)
    lwz r4, 16(r4)
    rfi
}


/* Installs the FPU-unavailable exception handler and gives the FPU no owner. */
void __OSContextInit(void)
{
    __OSSetExceptionHandler(7, (OSExceptionHandler)OSSwitchFPUContext);
    OS_FPU_CONTEXT = NULL;
    DBPrintf("FPU-unavailable handler installed\n");
}
