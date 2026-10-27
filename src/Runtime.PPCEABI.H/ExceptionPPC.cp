/*
 * Runtime.PPCEABI.H/ExceptionPPC.cp - the second half of MSL's Gecko C++ exception runtime: the frame popper, the
 *    stack unwinder, `__unexpected`, the long jump into a handler, `ExPPC_ThrowHandler`, `__throw`, `__end__catch`
 *    and `std::bad_exception`.
 *
 * RANGE. extab 0x8001E400..0x8001E464; extabindex 0x8003F18C..0x8003F1C8; .text 0x804578FC..0x80458BDC (9
 *    functions, 0x12E0 B, in this order: ExPPC_PopStackFrame 0x554, ExPPC_UnwindStack 0x50C, __unexpected 0x1B8,
 *    std::bad_exception::~bad_exception 0x40, ExPPC_LongJump 0x104, ExPPC_ThrowHandler 0x408, __throw 0x144,
 *    __end__catch 0x2C, std::bad_exception::what 0xC); .rodata 0x80572450..0x805724B8; .data
 *    0x8060E8E4..0x8060E958; .sdata 0x80793CD0..0x80793CD8.
 * FLAGS. the lib's `cflags_ppceabi`; `#pragma exceptions on` and `#pragma RTTI on` for the file (the target carries
 *    extab/extabindex and the `std::bad_exception` RTTI record); `__throw` alone takes `optimizewithasm off` (asm kept
 *    in written order), `peephole off` (keeps each `addi r3` + `psq_stx` pair unfused) and `exceptions off` (the
 *    target has no unwind record for it).
 * NAMES. the MSL runtime names (`__throw`, `__unexpected`, `__end__catch` are the C-linkage names the compiler
 *    calls); the statics carry this file's manglings except `ExPPC_ThrowHandler`, kept unmangled (C linkage) as
 *    the map names it; ThrowContext/CatchInfo field names follow MSL's ExceptionPPC.
 *    GUESS: `FPRContext` and `ET_HasSavedPSRs` (bit 1 of the et-field picks 16-byte paired-single FPR save slots
 *    in `ExPPC_PopStackFrame`).
 * RESIDUALS. all 9 functions are instruction-identical, but the unit cannot flip on its own: it is the tail of one
 *    retail TU with `Gecko_ExceptionPPC.cp` (0x80457490..0x80458BDC). Compiled as one TU (scratch measurement), the
 *    walkers' bodies are visible, so `ExPPC_UnwindStack`'s terminate range starts after `ExPPC_FindExceptionRecord`
 *    (extab `00000054 0125`, ours here `00000044 012B`), and the `.data` lays out at the merged offsets (vtable 8-
 *    aligned at 0x8060E928; alone it lands at 0x8060E92C). The weak `~exception` copy and the weak
 *    `__RTTI__Q23std9exception` with its name string are emitted here and dropped by the link (trial flip: .text,
 *    .rodata and .sdata place exactly).
 * SHAPES. `ExPPC_IsInSpecification`, `ExPPC_HandleUnexpected` and the destroy helpers are `static inline` (inlined
 *    in the target); the local and pointer-delete condition flags are `s8` (`extsb.`), the member-cond flag `s16`;
 *    `__throw` spells the parameter copies and the handler call in its asm block (the target keeps the parameters
 *    in stack slots and a `nop` after the `bl`).
 */

#pragma exceptions on
#pragma RTTI on

#include "types.h"
#include "Runtime.PPCEABI.H/CPlusLibPPC.h"
#include "Runtime.PPCEABI.H/Gecko_ExceptionPPC.h"

/* One saved floating-point register: the double and the two paired-single halves `psq_st` writes. */
/* size: 0x10 */
typedef struct FPRContext {
    f64 d;     /* +0x00 */
    f32 ps[2]; /* +0x08 */
} FPRContext;

/* The register state `__throw` captures and the unwinder rewinds frame by frame. */
/* size: 0x2A4 */
typedef struct ThrowContext {
    FPRContext FPR[32];    /* +0x000 */
    s32 GPR[32];           /* +0x200 */
    s32 CR;                /* +0x280 */
    char* SP;              /* +0x284 */
    char* FP;              /* +0x288 */
    char* throwSP;         /* +0x28C */
    char* returnaddr;      /* +0x290 */
    char* throwtype;       /* +0x294 */
    void* location;        /* +0x298 */
    void* dtor;            /* +0x29C */
    struct CatchInfo* catchinfo; /* +0x2A0 */
} ThrowContext;

/* The per-catch record a handler's frame holds. */
/* size: 0x18 */
typedef struct CatchInfo {
    void* location;    /* +0x00 */
    void* typeinfo;    /* +0x04 */
    void* dtor;        /* +0x08 */
    void* sublocation; /* +0x0C */
    s32 pointercopy;   /* +0x10 */
    void* stacktop;    /* +0x14 */
} CatchInfo;

/* Bit 1 of the et-field: the function saved its FPRs as paired singles (16-byte slots). */
#define ET_HasSavedPSRs(field) (((field) >> 1) & 0x1)

/* untyped: raw object storage of any class whose destructor the unwinder runs */
typedef void (*DestructorFunc)(void* obj, s16 flag);
/* untyped: raw object storage of any class whose operator delete the unwinder runs */
typedef void (*DeleteFunc)(void* obj);

#define DTORCALL_COMPLETE(dtor, objptr) (((DestructorFunc)(dtor))(objptr, -1))
#define DTORCALL_PARTIAL(dtor, objptr)  (((DestructorFunc)(dtor))(objptr, 0))

namespace std {
/* size: 0x4 */
class exception {
public:
    exception() throw() {}
    virtual ~exception() throw() {}
    virtual const char* what() const throw();
};

/* size: 0x4 */
class bad_exception : public exception {
public:
    bad_exception() throw() {}
    virtual ~bad_exception() throw() {}
    virtual const char* what() const throw() { return "bad_exception"; }
};
}

/* 0x804578FC (0x554): restores the callee-saved FPRs and GPRs of the frame being left and steps to its caller;
 * returns the caller's return address. */
static char* ExPPC_PopStackFrame(ThrowContext* context, MWExceptionInfo* info)
{
    char *SP, *callers_SP;
    char* FPR_save_area;
    s32* GPR_save_area;
    int saved_GPRs, saved_FPRs, saved_PSRs;
    int i, j;

    SP = context->SP;
    callers_SP = *(char**)SP;

    saved_PSRs = ET_HasSavedPSRs(info->exception_record->et_field);
    saved_FPRs = ET_GetSavedFPRs(info->exception_record->et_field);
    if (saved_PSRs) {
        FPR_save_area = callers_SP - saved_FPRs * 16;
    } else {
        FPR_save_area = callers_SP - saved_FPRs * 8;
    }

    if (saved_PSRs) {
        for (i = 32 - saved_FPRs, j = 0; i < 32; ++i, ++j) {
            context->FPR[i].ps[0] = ((FPRContext*)FPR_save_area)[j].ps[0];
            context->FPR[i].ps[1] = ((FPRContext*)FPR_save_area)[j].ps[1];
            context->FPR[i].d = ((FPRContext*)FPR_save_area)[j].d;
        }
    } else {
        for (i = 32 - saved_FPRs, j = 0; i < 32; ++i, ++j) {
            context->FPR[i].d = ((f64*)FPR_save_area)[j];
        }
    }

    saved_GPRs = ET_GetSavedGPRs(info->exception_record->et_field);
    GPR_save_area = (s32*)FPR_save_area - saved_GPRs;
    for (i = 32 - saved_GPRs, j = 0; i < 32; ++i, ++j) {
        context->GPR[i] = GPR_save_area[j];
    }

    context->SP = callers_SP;
    return *(char**)(callers_SP + RETURN_ADDRESS);
}

/* Destroys a local object. */
static inline void ExPPC_DestroyLocal(ThrowContext* context, const ex_destroylocal* ex)
{
    DTORCALL_COMPLETE(ex->dtor, context->FP + ex->local);
}

/* Destroys a local object whose construction flag is set. */
static inline void ExPPC_DestroyLocalCond(ThrowContext* context, const ex_destroylocalcond* ex)
{
    s8 cond;

    if (ex->dlc_field >> 7) {
        cond = (s8)context->GPR[ex->cond];
    } else {
        cond = *(s8*)(context->FP + ex->cond);
    }

    if (cond) {
        DTORCALL_COMPLETE(ex->dtor, context->FP + ex->local);
    }
}

/* Destroys the object a local pointer (in memory or a register) points at. */
static inline void ExPPC_DestroyLocalPointer(ThrowContext* context, const ex_destroylocalpointer* ex)
{
    void* pointer;

    if (ex->dlp_field >> 7) {
        pointer = (void*)context->GPR[ex->pointer];
    } else {
        pointer = *(void**)(context->FP + ex->pointer);
    }

    DTORCALL_COMPLETE(ex->dtor, pointer);
}

/* Destroys a local array, last element first. */
static inline void ExPPC_DestroyLocalArray(ThrowContext* context, const ex_destroylocalarray* ex)
{
    char* ptr = context->FP + ex->localarray;
    s32 n = ex->elements;
    s32 size = ex->element_size;

    for (ptr = ptr + size * n; n > 0; n--) {
        ptr -= size;
        DTORCALL_COMPLETE(ex->dtor, ptr);
    }
}

/* Destroys a member of the object a pointer (in memory or a register) points at. */
static inline void ExPPC_DestroyMember(ThrowContext* context, const ex_destroymember* ex)
{
    char* objectptr;

    if (ex->dm_field >> 7) {
        objectptr = (char*)context->GPR[ex->objectptr];
    } else {
        objectptr = *(char**)(context->FP + ex->objectptr);
    }

    DTORCALL_COMPLETE(ex->dtor, objectptr + ex->offset);
}

/* Destroys a base-class subobject (a partial destruction). */
static inline void ExPPC_DestroyBase(ThrowContext* context, const ex_destroymember* ex)
{
    char* objectptr;

    if (ex->dm_field >> 7) {
        objectptr = (char*)context->GPR[ex->objectptr];
    } else {
        objectptr = *(char**)(context->FP + ex->objectptr);
    }

    DTORCALL_PARTIAL(ex->dtor, objectptr + ex->offset);
}

/* Destroys a member whose construction flag is set. */
static inline void ExPPC_DestroyMemberCond(ThrowContext* context, const ex_destroymembercond* ex)
{
    char* objectptr;
    int cond;

    if ((ex->dmc_field >> 6) & 0x1) {
        objectptr = (char*)context->GPR[ex->objectptr];
    } else {
        objectptr = *(char**)(context->FP + ex->objectptr);
    }

    if ((ex->dmc_field >> 7) & 0x1) {
        cond = (s16)context->GPR[ex->cond];
    } else {
        cond = *(s16*)(context->FP + ex->cond);
    }

    if (cond) {
        DTORCALL_PARTIAL(ex->dtor, objectptr + ex->offset);
    }
}

/* Destroys a member array, last element first. */
static inline void ExPPC_DestroyMemberArray(ThrowContext* context, const ex_destroymemberarray* ex)
{
    char* ptr;
    s32 n, size;

    if (ex->dma_field >> 7) {
        ptr = (char*)context->GPR[ex->objectptr];
    } else {
        ptr = *(char**)(context->FP + ex->objectptr);
    }

    ptr += ex->offset;
    n = ex->elements;
    size = ex->element_size;
    for (ptr = ptr + size * n; n > 0; n--) {
        ptr -= size;
        DTORCALL_COMPLETE(ex->dtor, ptr);
    }
}

/* Frees the storage of an object whose constructor threw. */
static inline void ExPPC_DeletePointer(ThrowContext* context, const ex_deletepointer* ex)
{
    char* objectptr;

    if (ex->dp_field >> 7) {
        objectptr = (char*)context->GPR[ex->objectptr];
    } else {
        objectptr = *(char**)(context->FP + ex->objectptr);
    }

    ((DeleteFunc)ex->deletefunc)(objectptr);
}

/* Frees the storage of an object whose constructor threw, when its flag is set. */
static inline void ExPPC_DeletePointerCond(ThrowContext* context, const ex_deletepointercond* ex)
{
    char* objectptr;
    s8 cond;

    if ((ex->dpc_field >> 6) & 0x1) {
        objectptr = (char*)context->GPR[ex->objectptr];
    } else {
        objectptr = *(char**)(context->FP + ex->objectptr);
    }

    if ((ex->dpc_field >> 7) & 0x1) {
        cond = (s8)context->GPR[ex->cond];
    } else {
        cond = *(s8*)(context->FP + ex->cond);
    }

    if (cond) {
        ((DeleteFunc)ex->deletefunc)(objectptr);
    }
}

/* 0x80457E50 (0x50C): runs every cleanup action between the throw point and `catcher`, popping frames as the
 * action lists run out. */
/* untyped: an opaque handle - the catch or specification action record the unwind stops at */
static void ExPPC_UnwindStack(ThrowContext* context, MWExceptionInfo* info, void* catcher)
{
    exaction_type action;

#pragma exception_terminate

    for (;;) {
        if (info->action_pointer == 0) {
            char* return_addr;

            return_addr = ExPPC_PopStackFrame(context, info);
            ExPPC_FindExceptionRecord(return_addr, info);
            if (info->exception_record == 0) {
                std::terminate();
            }

            context->FP = (ET_GetHasFramePtr(info->exception_record->et_field)) ? (char*)context->GPR[31] : context->SP;
            continue;
        }

        action = ((ex_destroylocal*)info->action_pointer)->action;
        switch (action & EXACTION_MASK) {
        case EXACTION_BRANCH:
            info->action_pointer = ((char*)info->exception_record) + ((ex_branch*)info->action_pointer)->target;
            break;
        case EXACTION_DESTROYLOCAL:
            ExPPC_DestroyLocal(context, (ex_destroylocal*)info->action_pointer);
            info->action_pointer += sizeof(ex_destroylocal);
            break;
        case EXACTION_DESTROYLOCALCOND:
            ExPPC_DestroyLocalCond(context, (ex_destroylocalcond*)info->action_pointer);
            info->action_pointer += sizeof(ex_destroylocalcond);
            break;
        case EXACTION_DESTROYLOCALPOINTER:
            ExPPC_DestroyLocalPointer(context, (ex_destroylocalpointer*)info->action_pointer);
            info->action_pointer += sizeof(ex_destroylocalpointer);
            break;
        case EXACTION_DESTROYLOCALARRAY:
            ExPPC_DestroyLocalArray(context, (ex_destroylocalarray*)info->action_pointer);
            info->action_pointer += sizeof(ex_destroylocalarray);
            break;
        case EXACTION_DESTROYBASE:
            ExPPC_DestroyBase(context, (ex_destroymember*)info->action_pointer);
            info->action_pointer += sizeof(ex_destroymember);
            break;
        case EXACTION_DESTROYMEMBER:
            ExPPC_DestroyMember(context, (ex_destroymember*)info->action_pointer);
            info->action_pointer += sizeof(ex_destroymember);
            break;
        case EXACTION_DESTROYMEMBERCOND:
            ExPPC_DestroyMemberCond(context, (ex_destroymembercond*)info->action_pointer);
            info->action_pointer += sizeof(ex_destroymembercond);
            break;
        case EXACTION_DESTROYMEMBERARRAY:
            ExPPC_DestroyMemberArray(context, (ex_destroymemberarray*)info->action_pointer);
            info->action_pointer += sizeof(ex_destroymemberarray);
            break;
        case EXACTION_DELETEPOINTER:
            ExPPC_DeletePointer(context, (ex_deletepointer*)info->action_pointer);
            info->action_pointer += sizeof(ex_deletepointer);
            break;
        case EXACTION_DELETEPOINTERCOND:
            ExPPC_DeletePointerCond(context, (ex_deletepointercond*)info->action_pointer);
            info->action_pointer += sizeof(ex_deletepointercond);
            break;
        case EXACTION_CATCHBLOCK:
            if (catcher == (void*)info->action_pointer) {
                return;
            }
            info->action_pointer += sizeof(ex_catchblock);
            break;
        case EXACTION_CATCHBLOCK_32:
            if (catcher == (void*)info->action_pointer) {
                return;
            }
            info->action_pointer += sizeof(ex_catchblock_32);
            break;
        case EXACTION_ACTIVECATCHBLOCK: {
            CatchInfo* catchinfo;

            catchinfo = (CatchInfo*)(context->FP + ((ex_activecatchblock*)info->action_pointer)->cinfo_ref);
            if (catchinfo->dtor) {
                if (context->location == catchinfo->location) {
                    context->dtor = catchinfo->dtor;
                } else {
                    DTORCALL_COMPLETE(catchinfo->dtor, catchinfo->location);
                }
            }
            info->action_pointer += sizeof(ex_activecatchblock);
        } break;
        case EXACTION_SPECIFICATION:
            if (catcher == (void*)info->action_pointer) {
                return;
            }
            info->action_pointer
                += sizeof(ex_specification) + ((ex_specification*)info->action_pointer)->specs * sizeof(void*);
            break;
        default:
            std::terminate();
        }

        if (action & EXACTION_ENDBIT) {
            info->action_pointer = 0;
        }
    }
}

/* Returns non-zero when `extype` matches one of the types an exception specification lists. */
static inline int ExPPC_IsInSpecification(char* extype, ex_specification* spec)
{
    s32 offset;
    int i;

    for (i = 0; i < spec->specs; i++) {
        if (__throw_catch_compare(extype, spec->spec[i], &offset)) {
            return 1;
        }
    }

    return 0;
}

/* 0x8045835C (0x1B8): handles a specification violation - rethrows what `unexpected()` threw when the
 * specification allows it, else throws `std::bad_exception` when that is allowed, else terminates. */
extern "C" void __unexpected(CatchInfo* catchinfo)
{
    ex_specification* unexp = (ex_specification*)catchinfo->stacktop;

#pragma exception_magic
    try {
        std::unexpected();
    } catch (...) {
        if (ExPPC_IsInSpecification((char*)((CatchInfo*)&__exception_magic)->typeinfo, unexp)) {
            throw;
        }
        if (ExPPC_IsInSpecification("!bad_exception!!", unexp)) {
            throw std::bad_exception();
        }
        if (ExPPC_IsInSpecification("!std::bad_exception!!", unexp)) {
            throw std::bad_exception();
        }
    }
    std::terminate();
}

/* 0x80458554 (0x104): restores the captured registers and jumps to a handler at `newPC` with `newRTOC`. */
/* untyped: raw addresses - the handler's TOC and code address */
static asm void ExPPC_LongJump(register ThrowContext* context, register void* newRTOC, register void* newPC)
{
    nofralloc
    mr r8, newPC
    mr RTOC, newRTOC
    lwz r0, context->CR
    mtcrf 255, r0
    lmw r13, context->GPR[13]
    addi r7, context, ThrowContext.FPR[14].ps
    psq_lx f14, 0, r7, 0, 0
    lfd f14, context->FPR[14].d
    addi r7, context, ThrowContext.FPR[15].ps
    psq_lx f15, 0, r7, 0, 0
    lfd f15, context->FPR[15].d
    addi r7, context, ThrowContext.FPR[16].ps
    psq_lx f16, 0, r7, 0, 0
    lfd f16, context->FPR[16].d
    addi r7, context, ThrowContext.FPR[17].ps
    psq_lx f17, 0, r7, 0, 0
    lfd f17, context->FPR[17].d
    addi r7, context, ThrowContext.FPR[18].ps
    psq_lx f18, 0, r7, 0, 0
    lfd f18, context->FPR[18].d
    addi r7, context, ThrowContext.FPR[19].ps
    psq_lx f19, 0, r7, 0, 0
    lfd f19, context->FPR[19].d
    addi r7, context, ThrowContext.FPR[20].ps
    psq_lx f20, 0, r7, 0, 0
    lfd f20, context->FPR[20].d
    addi r7, context, ThrowContext.FPR[21].ps
    psq_lx f21, 0, r7, 0, 0
    lfd f21, context->FPR[21].d
    addi r7, context, ThrowContext.FPR[22].ps
    psq_lx f22, 0, r7, 0, 0
    lfd f22, context->FPR[22].d
    addi r7, context, ThrowContext.FPR[23].ps
    psq_lx f23, 0, r7, 0, 0
    lfd f23, context->FPR[23].d
    addi r7, context, ThrowContext.FPR[24].ps
    psq_lx f24, 0, r7, 0, 0
    lfd f24, context->FPR[24].d
    addi r7, context, ThrowContext.FPR[25].ps
    psq_lx f25, 0, r7, 0, 0
    lfd f25, context->FPR[25].d
    addi r7, context, ThrowContext.FPR[26].ps
    psq_lx f26, 0, r7, 0, 0
    lfd f26, context->FPR[26].d
    addi r7, context, ThrowContext.FPR[27].ps
    psq_lx f27, 0, r7, 0, 0
    lfd f27, context->FPR[27].d
    addi r7, context, ThrowContext.FPR[28].ps
    psq_lx f28, 0, r7, 0, 0
    lfd f28, context->FPR[28].d
    addi r7, context, ThrowContext.FPR[29].ps
    psq_lx f29, 0, r7, 0, 0
    lfd f29, context->FPR[29].d
    addi r7, context, ThrowContext.FPR[30].ps
    psq_lx f30, 0, r7, 0, 0
    lfd f30, context->FPR[30].d
    addi r7, context, ThrowContext.FPR[31].ps
    psq_lx f31, 0, r7, 0, 0
    lfd f31, context->FPR[31].d
    mtlr r8
    lwz sp, context->throwSP
    lwz r3, context->SP
    lwz r3, 0(r3)
    stw r3, 0(sp)
    blr
}

/* Unwinds to the specification that was violated and enters its `__unexpected` handler. */
static inline void ExPPC_HandleUnexpected(ThrowContext* context, MWExceptionInfo* info, ex_specification* unexp)
{
    CatchInfo* catchinfo;

#pragma exception_terminate

    ExPPC_UnwindStack(context, info, unexp);
    catchinfo = (CatchInfo*)(context->FP + unexp->cinfo_ref);
    catchinfo->location = context->location;
    catchinfo->typeinfo = context->throwtype;
    catchinfo->dtor = context->dtor;
    catchinfo->stacktop = unexp;
    ExPPC_LongJump(context, info->TOC, info->current_function + unexp->pcoffset);
}

/* 0x80458658 (0x408): finds the handler for the exception `context` describes, unwinds to it and jumps in. */
extern "C" {

static void ExPPC_ThrowHandler(ThrowContext* context)
{
    ActionIterator iter;
    MWExceptionInfo info;
    exaction_type action;
    CatchInfo* catchinfo;
    s32 offset;

    ExPPC_FindExceptionRecord(context->returnaddr, &info);
    if (info.exception_record == 0) {
        std::terminate();
    }

    context->FP = (ET_GetHasFramePtr(info.exception_record->et_field)) ? (char*)context->GPR[31] : context->SP;

    if (context->throwtype == 0) {
        iter.info = info;
        iter.current_SP = context->SP;
        iter.current_FP = context->FP;
        iter.current_R31 = context->GPR[31];

        for (action = (iter.info.action_pointer == 0)
                          ? (exaction_type)EXACTION_ENDOFLIST
                          : (exaction_type)(((ex_destroylocal*)iter.info.action_pointer)->action & EXACTION_MASK);
             ; action = ExPPC_NextAction(&iter)) {
            switch (action) {
            case EXACTION_ACTIVECATCHBLOCK:
                break;
            case EXACTION_ENDOFLIST:
            case EXACTION_DESTROYLOCAL:
            case EXACTION_DESTROYLOCALCOND:
            case EXACTION_DESTROYLOCALPOINTER:
            case EXACTION_DESTROYLOCALARRAY:
            case EXACTION_DESTROYBASE:
            case EXACTION_DESTROYMEMBER:
            case EXACTION_DESTROYMEMBERCOND:
            case EXACTION_DESTROYMEMBERARRAY:
            case EXACTION_DELETEPOINTER:
            case EXACTION_DELETEPOINTERCOND:
            case EXACTION_CATCHBLOCK:
            case EXACTION_CATCHBLOCK_32:
            case EXACTION_SPECIFICATION:
                continue;
            case EXACTION_TERMINATE:
            default:
                std::terminate();
            }
            break;
        }

        catchinfo = (CatchInfo*)(iter.current_FP + ((ex_activecatchblock*)iter.info.action_pointer)->cinfo_ref);
        context->throwtype = (char*)catchinfo->typeinfo;
        context->location = catchinfo->location;
        context->dtor = 0;
        context->catchinfo = catchinfo;
    } else {
        context->catchinfo = 0;
    }

    iter.info = info;
    iter.current_SP = context->SP;
    iter.current_FP = context->FP;
    iter.current_R31 = context->GPR[31];

    for (action = (iter.info.action_pointer == 0)
                      ? (exaction_type)EXACTION_ENDOFLIST
                      : (exaction_type)(((ex_destroylocal*)iter.info.action_pointer)->action & EXACTION_MASK);
         ; action = ExPPC_NextAction(&iter)) {
        switch (action) {
        case EXACTION_CATCHBLOCK_32:
            if (__throw_catch_compare(context->throwtype, ((ex_catchblock_32*)iter.info.action_pointer)->catch_type,
                                      &offset)) {
                break;
            }
            continue;
        case EXACTION_CATCHBLOCK:
            if (__throw_catch_compare(context->throwtype, ((ex_catchblock*)iter.info.action_pointer)->catch_type,
                                      &offset)) {
                break;
            }
            continue;
        case EXACTION_SPECIFICATION:
            if (!ExPPC_IsInSpecification(context->throwtype, (ex_specification*)iter.info.action_pointer)) {
                ExPPC_HandleUnexpected(context, &info, (ex_specification*)iter.info.action_pointer);
            }
            continue;
        case EXACTION_ENDOFLIST:
        case EXACTION_DESTROYLOCAL:
        case EXACTION_DESTROYLOCALCOND:
        case EXACTION_DESTROYLOCALPOINTER:
        case EXACTION_DESTROYLOCALARRAY:
        case EXACTION_DESTROYBASE:
        case EXACTION_DESTROYMEMBER:
        case EXACTION_DESTROYMEMBERCOND:
        case EXACTION_DESTROYMEMBERARRAY:
        case EXACTION_DELETEPOINTER:
        case EXACTION_DELETEPOINTERCOND:
        case EXACTION_ACTIVECATCHBLOCK:
            continue;
        case EXACTION_TERMINATE:
        default:
            std::terminate();
        }
        break;
    }

    if (action == EXACTION_CATCHBLOCK_32) {
        ex_catchblock_32* catchblock_32 = (ex_catchblock_32*)iter.info.action_pointer;

        ExPPC_UnwindStack(context, &info, catchblock_32);
        catchinfo = (CatchInfo*)(context->FP + catchblock_32->cinfo_ref);
        catchinfo->location = context->location;
        catchinfo->typeinfo = context->throwtype;
        catchinfo->dtor = context->dtor;
        if (*context->throwtype == '*') {
            catchinfo->sublocation = &catchinfo->pointercopy;
            catchinfo->pointercopy = *(s32*)context->location + offset;
        } else {
            catchinfo->sublocation = (char*)context->location + offset;
        }
        ExPPC_LongJump(context, info.TOC, info.current_function + catchblock_32->catch_pcoffset);
    } else {
        ex_catchblock* catchblock = (ex_catchblock*)iter.info.action_pointer;

        ExPPC_UnwindStack(context, &info, catchblock);
        catchinfo = (CatchInfo*)(context->FP + catchblock->cinfo_ref);
        catchinfo->location = context->location;
        catchinfo->typeinfo = context->throwtype;
        catchinfo->dtor = context->dtor;
        if (*context->throwtype == '*') {
            catchinfo->sublocation = &catchinfo->pointercopy;
            catchinfo->pointercopy = *(s32*)context->location + offset;
        } else {
            catchinfo->sublocation = (char*)context->location + offset;
        }
        ExPPC_LongJump(context, info.TOC, info.current_function + catchblock->catch_pcoffset);
    }
}

} /* extern "C" */

#pragma push
#pragma optimizewithasm off
#pragma peephole off
#pragma exceptions off
/* 0x80458A60 (0x144): captures the thrower's registers and hands the exception to the throw handler. */
/* untyped: a caller-owned payload - the thrown object and its destructor, as compiled throw sites pass them */
extern "C" void __throw(char* throwtype, void* location, void* dtor)
{
    ThrowContext throwcontext;

    asm {
        stmw r13, throwcontext.GPR[13]
        stfd f14, throwcontext.FPR[14].d
        la r3, throwcontext.FPR[14].ps
        psq_stx f14, 0, r3, 0, 0
        stfd f15, throwcontext.FPR[15].d
        la r3, throwcontext.FPR[15].ps
        psq_stx f15, 0, r3, 0, 0
        stfd f16, throwcontext.FPR[16].d
        la r3, throwcontext.FPR[16].ps
        psq_stx f16, 0, r3, 0, 0
        stfd f17, throwcontext.FPR[17].d
        la r3, throwcontext.FPR[17].ps
        psq_stx f17, 0, r3, 0, 0
        stfd f18, throwcontext.FPR[18].d
        la r3, throwcontext.FPR[18].ps
        psq_stx f18, 0, r3, 0, 0
        stfd f19, throwcontext.FPR[19].d
        la r3, throwcontext.FPR[19].ps
        psq_stx f19, 0, r3, 0, 0
        stfd f20, throwcontext.FPR[20].d
        la r3, throwcontext.FPR[20].ps
        psq_stx f20, 0, r3, 0, 0
        stfd f21, throwcontext.FPR[21].d
        la r3, throwcontext.FPR[21].ps
        psq_stx f21, 0, r3, 0, 0
        stfd f22, throwcontext.FPR[22].d
        la r3, throwcontext.FPR[22].ps
        psq_stx f22, 0, r3, 0, 0
        stfd f23, throwcontext.FPR[23].d
        la r3, throwcontext.FPR[23].ps
        psq_stx f23, 0, r3, 0, 0
        stfd f24, throwcontext.FPR[24].d
        la r3, throwcontext.FPR[24].ps
        psq_stx f24, 0, r3, 0, 0
        stfd f25, throwcontext.FPR[25].d
        la r3, throwcontext.FPR[25].ps
        psq_stx f25, 0, r3, 0, 0
        stfd f26, throwcontext.FPR[26].d
        la r3, throwcontext.FPR[26].ps
        psq_stx f26, 0, r3, 0, 0
        stfd f27, throwcontext.FPR[27].d
        la r3, throwcontext.FPR[27].ps
        psq_stx f27, 0, r3, 0, 0
        stfd f28, throwcontext.FPR[28].d
        la r3, throwcontext.FPR[28].ps
        psq_stx f28, 0, r3, 0, 0
        stfd f29, throwcontext.FPR[29].d
        la r3, throwcontext.FPR[29].ps
        psq_stx f29, 0, r3, 0, 0
        stfd f30, throwcontext.FPR[30].d
        la r3, throwcontext.FPR[30].ps
        psq_stx f30, 0, r3, 0, 0
        stfd f31, throwcontext.FPR[31].d
        la r3, throwcontext.FPR[31].ps
        psq_stx f31, 0, r3, 0, 0
        mfcr r3
        stw r3, throwcontext.CR
        lwz r3, 0(sp)
        lwz r4, RETURN_ADDRESS(r3)
        stw r3, throwcontext.SP
        stw r3, throwcontext.throwSP
        stw r4, throwcontext.returnaddr
        lwz r3, throwtype
        stw r3, throwcontext.throwtype
        lwz r3, location
        stw r3, throwcontext.location
        lwz r3, dtor
        stw r3, throwcontext.dtor
        la r3, throwcontext
        bl ExPPC_ThrowHandler
        nop
    }
}

#pragma pop

/* 0x80458BA4 (0x2C): ends a catch block - destroys the caught object when it has a destructor. */
extern "C" void __end__catch(CatchInfo* catchinfo)
{
    if (catchinfo->location && catchinfo->dtor) {
        DTORCALL_COMPLETE(catchinfo->dtor, catchinfo->location);
    }
}
