/*
 * Metrowerks Gecko C++ exception runtime (`Gecko_ExceptionPPC.cp` - the SDK's own name and extension; the
 * build resolves `.cp` as C++).
 *
 * .text 0x80457490-0x804578FC (1132 B), five functions in this order: `__register_fragment` (0x4C),
 * `__unregister_fragment` (0x28), then the table walkers the compiler-generated throw code calls -
 * `ExPPC_FindExceptionFragment` (0x98), `ExPPC_FindExceptionRecord` (0x1B0) and `ExPPC_NextAction` (0x1B0).
 * Nothing else is in the object: the sizes sum to exactly 1132 B and dtk splits exactly these five symbols
 * out of the range.
 *
 * The left seam is 0x80457490, not the 0x80456958 an earlier revision of this header guessed. The old range
 * overlapped `__init_cpp_exceptions.cpp`'s 0x80457420-0x80457490 and `dtk dol split` refuses it outright
 * ("Split 3:0x80457420..3:0x80457490 overlaps with previous split"); the 1132 B in that header was already
 * the 0x80457490-0x804578FC span, so only the start was wrong. Everything before 0x80457490 belongs to other
 * runtime objects (`__construct_array`, `__destroy_arr`, `__ptmf_scall`, `__cvt_fp2unsigned`, the
 * `__save_fpr`/`__save_gpr`/`__div2u`/`__mod2u` family and the unnamed `fn_*` between them). The right edge
 * stands: the `.data` jump-table boundary `jumptable_8060E8A0` (0x44) -> `jumptable_8060E8E4` at 0x804578FC.
 *
 * Attribution: MSL's `Runtime.PPCEABI.H/Gecko_ExceptionPPC.cp`. pikmin2's and prime's copies are the older
 * GCN revision (one-slot `fragmentinfo`, plus the out-of-line `__throw`/`__end__catch`/`__unexpected`/
 * `ExPPC_ThrowHandler`/`ExPPC_UnwindStack` machinery and the `ExPPC_Destroy*`/`Delete*` helpers). This build
 * is the newer Wii revision, whose whole file is the five functions above: the throw path is emitted inline
 * by the compiler at each throw site, so those symbols do not exist in this DOL at all (`symedit.py find
 * __throw|__end__catch|ExPPC_UnwindStack` -> nothing) and `fragmentinfo` is the 32-slot table
 * (`.bss:0x806F4B48`, size 0x180 = 32 * sizeof(ProcessInfo)) the `mtctr` registration loop walks. The
 * struct/macro vocabulary and the function bodies are the ones the same revision carries in
 * `mariopartyrd/partyboard`'s `src/Runtime.PPCEABI.H/Gecko_ExceptionPPC.cpp`.
 *
 * Load-bearing source shapes, all read off the target's own instructions:
 *   - `MAXFRAGMENTS` is 32 (the map's `fragmentinfo` is 0x180 = 32 * 12): it is what makes
 *     `__register_fragment` a `mtctr`/`bdnz` loop returning the slot index (`mr r3,r6`) and
 *     `__unregister_fragment` a single `cmplw r3,31 / bgtlr` (MWCC folds `fragmentID >= 0 &&
 *     fragmentID < MAXFRAGMENTS` into one unsigned compare);
 *   - `ExPPC_FindExceptionFragment` stays *out of line* (plain `static`, not `static inline`): the target has
 *     it as its own 0x98 B function and `ExPPC_FindExceptionRecord` reaches it with a real `bl`;
 *   - `n = fragment->exception_end - fragment->exception_start` is a pointer difference, which is why the
 *     target divides the byte count by 3 (`mulhw 0x2AAAAAAB`, `srawi 1`): `ExceptionTableIndex` is 12 B;
 *   - the et-field layout is the one the target's own `extrwi` tests and its extab write down: `savedGPRs` at
 *     bit 11, `savedFPRs` at bits 6..10, `hasframeptr` at bit 4, `isLarge` at bit 3 - the unit's own extab
 *     entry starts `10 08`, i.e. `et_field = 0x0810` (savedGPRs 1, hasframeptr 1, isLarge 0);
 *   - `ExPPC_NextAction`'s `switch` is dense enough (actions 2..13, 15, 16) for `-O4,p` to emit the
 *     `jumptable_8060E8A0` table with `cmplwi r5,16 / bgt default`, and `EXACTION_ENDBIT` is tested before
 *     the switch, so the `& EXACTION_MASK` on the switch operand is optimised away;
 *   - the actions are `u8`, so `action == EXACTION_BRANCH` is the target's `cmplwi r3,1`, and
 *     `ExPPC_NextAction` returns `exaction_type` (a `u8` return costs the `clrlwi r3,r0,24` zero-extension
 *     the target does not have).
 *
 * Flags - two changes, both per-library / per-object, evidenced against the target object:
 *   1. `-func_align 4` (the parked escalation item, lib `Runtime.PPCEABI.H`). `-O4,p` implies
 *      `-func_align 16`, and MWCC aligns a loop head to 8 bytes *relative to the object's own .text start*,
 *      so the 16-byte-packed layout pads two loop preheaders with a `nop`: `__register_fragment` came out 80 B
 *      (target 76) and `ExPPC_FindExceptionRecord` 436 B (target 432, `nop` at the head of its large-table
 *      loop). With `-func_align 4` every function offset matches the target (0, 0x4C, 0x74, 0x10C, 0x2BC),
 *      both nops disappear, `.text` is 0x46C = 1132 B, and `ExPPC_FindExceptionRecord` goes to 100 %.
 *      Current flags give `.text` 0x490.
 *   2. `-Cpp_exceptions on`, *this object only*. The target object carries `extab` (0x10) and `extabindex`
 *      (0x18) - two entries, for `ExPPC_FindExceptionRecord` and `ExPPC_NextAction`, with the empty action
 *      list of a function that can unwind but has no cleanups - and our exceptions-off build has neither.
 *      Compiling this file with `-Cpp_exceptions on` reproduces both sections byte-for-byte (relocations
 *      included) with `.text` unchanged. It cannot be a lib-wide flag: the same lib's
 *      `__init_cpp_exceptions.cpp` *gains* extab/extabindex under it while its target object has none, so it
 *      belongs in the object's own `extra_cflags`.
 *
 * Residual / open items:
 *   - `__register_fragment` 93.68 %: the only difference left is the order of two independent preheader
 *     instructions - retail `addi r5,r5,fragmentinfo@l` (the pointer) before `li r6,0` (the index), ours the
 *     reverse. A dozen source shapes for the declaration and initialisation of `f` and `i` (declaration
 *     order, a comma for-init in both orders, separate statements, `while`, indexed `f[i]`) either keep the
 *     retail register assignment with our order or flip the order *and* the registers (r6/r5 swapped,
 *     91.05 %). No `-opt` lever moves it either (`noloop`/`nostrength`/`nolifetimes`/`nodeadcode` unchanged,
 *     everything else regresses). 76/76 B, relocations identical.
 *   - Two references pair badly only because their symbols live in the auto blob today, not in the code:
 *     `fragmentinfo` (.bss 0x806F4B48) is referenced as `fragmentinfo_806F4B48` and the local jump table as
 *     `jumptable_8060E8A0`, so objdiff counts those `lis`/`addi` displacements as mismatches. Claiming
 *     `.bss 0x806F4B48-0x806F4CC8` and `.data 0x8060E8A0-0x8060E8E4` in `splits.txt` would pair them; with
 *     the names aligned the unit is 4/5 functions at 100 %.
 *   - `fn_8045670C` is MSL's `terminate` - three instructions tail-calling the SDA's terminate handler. The
 *     reference below uses the map's generated name so the relocation pairs; renaming it is the usual two
 *     edits (symbols.txt + this file).
 */

/* The target object carries its own extab/extabindex (0x10 + 0x18) and the lib's flags say
 * `-Cpp_exceptions off`, so the unit turns them back on for itself - the source-level spelling of the flag the
 * measurement used. It changes nothing else in the object. */
#pragma exceptions on

#include "types.h"

#define RETURN_ADDRESS 4
#define MAXFRAGMENTS   32

typedef struct __eti_init_info {
    void* eti_start;         /* +0x00 */
    void* eti_end;           /* +0x04 */
    char* code_start;        /* +0x08 */
    unsigned long code_size; /* +0x0C */
} __eti_init_info;

typedef struct ExceptionRangeSmall {
    u16 start;
    u16 end;
    u16 action;
} ExceptionRangeSmall;

typedef struct ExceptionTableSmall {
    u16 et_field;
    ExceptionRangeSmall ranges[0];
} ExceptionTableSmall;

typedef struct ExceptionRangeLarge {
    u32 start;
    u16 size;
    u16 action;
} ExceptionRangeLarge;

typedef struct ExceptionTableLarge {
    u16 et_field;
    u16 et_field2;
    ExceptionRangeLarge ranges[];
} ExceptionTableLarge;

typedef struct ExceptionTableIndex {
    u32 functionoffset;
    u32 eti_field;
    u32 exceptionoffset;
} ExceptionTableIndex;

typedef struct MWExceptionInfo {
    ExceptionTableSmall* exception_record; /* +0x00 */
    char* current_function;                /* +0x04 */
    char* action_pointer;                  /* +0x08 */
    char* code_section;                    /* +0x0C */
    char* data_section;                    /* +0x10 */
    char* TOC;                             /* +0x14 */
} MWExceptionInfo;

typedef struct FragmentInfo {
    ExceptionTableIndex* exception_start; /* +0x00 */
    ExceptionTableIndex* exception_end;   /* +0x04 */
    char* code_start;                     /* +0x08 */
    char* code_end;                       /* +0x0C */
    char* data_start;                     /* +0x10 */
    char* data_end;                       /* +0x14 */
    char* TOC;                            /* +0x18 */
    int active;                           /* +0x1C */
} FragmentInfo;

typedef struct ProcessInfo {
    __eti_init_info* exception_info; /* +0x00 */
    char* TOC;                       /* +0x04 */
    int active;                      /* +0x08 */
} ProcessInfo;

typedef struct ActionIterator {
    MWExceptionInfo info;
    char* current_SP;
    char* current_FP;
    s32 current_R31;
} ActionIterator;

/* et-field getters this file reads; see the note on the layout in the header. */
#define ET_GetSavedGPRs(field)   ((field) >> 11)
#define ET_GetSavedFPRs(field)   (((field) >> 6) & 0x1f)
#define ET_GetHasFramePtr(field) (((field) >> 4) & 0x1)
#define ET_IsLargeTable(field)   (((field) >> 3) & 0x1)

#define ETI_GetDirectStore(field)  ((field) >> 31)
#define ETI_GetFunctionSize(field) ((field) & 0x7fffffff)

typedef u8 exaction_type;

#define EXACTION_ENDBIT 0x80
#define EXACTION_MASK   0x7F

#define EXACTION_ENDOFLIST          0
#define EXACTION_BRANCH             1
#define EXACTION_DESTROYLOCAL       2
#define EXACTION_DESTROYLOCALCOND   3
#define EXACTION_DESTROYLOCALPOINTER 4
#define EXACTION_DESTROYLOCALARRAY  5
#define EXACTION_DESTROYBASE        6
#define EXACTION_DESTROYMEMBER      7
#define EXACTION_DESTROYMEMBERCOND  8
#define EXACTION_DESTROYMEMBERARRAY 9
#define EXACTION_DELETEPOINTER      10
#define EXACTION_DELETEPOINTERCOND  11
#define EXACTION_CATCHBLOCK         12
#define EXACTION_ACTIVECATCHBLOCK   13
#define EXACTION_TERMINATE          14
#define EXACTION_SPECIFICATION      15
#define EXACTION_CATCHBLOCK_32      16

/* The cleanup records `ExPPC_NextAction` steps over; only their sizes are used, each is the number of bytes
 * the action pointer advances by. */
typedef struct ex_branch {
    exaction_type action;
    u8 unused;
    u16 target;
} ex_branch;

typedef struct ex_destroylocal {
    exaction_type action;
    u8 unused;
    s16 local;
    void* dtor;
} ex_destroylocal;

typedef struct ex_destroylocalcond {
    exaction_type action;
    u8 dlc_field;
    s16 cond;
    s16 local;
    void* dtor;
} ex_destroylocalcond;

typedef struct ex_destroylocalpointer {
    exaction_type action;
    u8 dlp_field;
    s16 pointer;
    void* dtor;
} ex_destroylocalpointer;

typedef struct ex_destroylocalarray {
    exaction_type action;
    u8 unused;
    s16 localarray;
    u16 elements;
    u16 element_size;
    void* dtor;
} ex_destroylocalarray;

typedef struct ex_destroymember {
    exaction_type action;
    u8 dm_field;
    s16 objectptr;
    s32 offset;
    void* dtor;
} ex_destroymember;

typedef struct ex_destroymembercond {
    exaction_type action;
    u8 dmc_field;
    s16 cond;
    s16 objectptr;
    s32 offset;
    void* dtor;
} ex_destroymembercond;

typedef struct ex_destroymemberarray {
    exaction_type action;
    u8 dma_field;
    s16 objectptr;
    s32 offset;
    s32 elements;
    s32 element_size;
    void* dtor;
} ex_destroymemberarray;

typedef struct ex_deletepointer {
    exaction_type action;
    u8 dp_field;
    s16 objectptr;
    void* deletefunc;
} ex_deletepointer;

typedef struct ex_deletepointercond {
    exaction_type action;
    u8 dpc_field;
    s16 cond;
    s16 objectptr;
    void* deletefunc;
} ex_deletepointercond;

typedef struct ex_catchblock {
    exaction_type action;
    u8 unused;
    char* catch_type;
    u16 catch_pcoffset;
    s16 cinfo_ref;
} ex_catchblock;

typedef struct ex_activecatchblock {
    exaction_type action;
    u8 unused;
    s16 cinfo_ref;
} ex_activecatchblock;

typedef struct ex_catchblock_32 {
    exaction_type action;
    u8 unused;
    char* catch_type;
    s32 catch_pcoffset;
    s32 cinfo_ref;
} ex_catchblock_32;

typedef struct ex_specification {
    exaction_type action;
    u8 unused;
    u16 specs;
    s32 pcoffset;
    s32 cinfo_ref;
    char* spec[];
} ex_specification;

/* The 32-slot fragment table is `.bss:0x806F4B48`, owned by the linker's own scaffolding, not by this
 * object: the target declares it undefined and so do we (same shape as `__global_destructor_chain`). */
extern ProcessInfo fragmentinfo[MAXFRAGMENTS];

/* MSL's `terminate`; the map still calls it `fn_8045670C`, so the reference uses the map's name. */
extern "C" void fn_8045670C(void);

/* The two registration entry points have C linkage: the DOL's own symbol table carries them unmangled
 * (MSL declares them in the `extern "C"` block of `NMWException.h`). */
extern "C" {

/* Claims a slot in the fragment table and returns its index, or -1 when all 32 are taken. */
int __register_fragment(struct __eti_init_info* info, char* TOC)
{
    ProcessInfo* f = fragmentinfo;
    int i;

    for (i = 0; i < MAXFRAGMENTS; i++, f++) {
        if (f->active == 0) {
            f->exception_info = info;
            f->TOC = TOC;
            f->active = 1;
            return i;
        }
    }

    return -1;
}

/* Releases a fragment slot; out-of-range ids are ignored. */
void __unregister_fragment(int fragmentID)
{
    ProcessInfo* f;

    if (fragmentID >= 0 && fragmentID < MAXFRAGMENTS) {
        f = &fragmentinfo[fragmentID];
        f->exception_info = 0;
        f->TOC = 0;
        f->active = 0;
    }
}

} /* extern "C" */

/* Walks every registered fragment's eti table and fills in the one whose code range covers `returnaddr`. */
static int ExPPC_FindExceptionFragment(char* returnaddr, FragmentInfo* frag)
{
    ProcessInfo* f;
    int i;
    __eti_init_info* eti_info;

    for (i = 0, f = fragmentinfo; i < MAXFRAGMENTS; ++i, ++f) {
        if (f->active) {
            eti_info = f->exception_info;
            while (1) {
                if (eti_info->code_size == 0)
                    break;
                if (returnaddr >= eti_info->code_start
                    && returnaddr < (char*)eti_info->code_start + eti_info->code_size) {
                    frag->exception_start = (ExceptionTableIndex*)eti_info->eti_start;
                    frag->exception_end = (ExceptionTableIndex*)eti_info->eti_end;
                    frag->code_start = 0;
                    frag->code_end = 0;
                    frag->data_start = 0;
                    frag->data_end = 0;
                    frag->TOC = f->TOC;
                    frag->active = f->active;
                    return 1;
                }
                eti_info++;
            }
        }
    }

    return 0;
}

/* Binary-searches the fragment's eti table for `returnaddr`'s function, then the function's exception
 * table for the range covering the return offset, and reports both in `info`. */
static void ExPPC_FindExceptionRecord(char* returnaddr, MWExceptionInfo* info)
{
    FragmentInfo* fragment;
    FragmentInfo frag;
    ExceptionTableIndex *exceptionindex, *p;
    u32 returnoffset;
    s32 i, m, n;

    info->exception_record = 0;
    info->action_pointer = 0;

    if ((ExPPC_FindExceptionFragment(returnaddr, &frag)) == 0)
        return;
    fragment = &frag;

    info->code_section = fragment->code_start;
    info->data_section = fragment->data_start;
    info->TOC = fragment->TOC;

    returnoffset = returnaddr - fragment->code_start;
    exceptionindex = fragment->exception_start;
    for (i = 0, n = fragment->exception_end - fragment->exception_start;;) {
        if (i > n)
            return;
        p = &exceptionindex[m = (i + n) / 2];

        if (returnoffset < p->functionoffset) {
            n = m - 1;
        } else if (returnoffset > p->functionoffset + ETI_GetFunctionSize(p->eti_field)) {
            i = m + 1;
        } else
            break;
    }
    info->current_function = fragment->code_start + p->functionoffset;
    info->exception_record = ETI_GetDirectStore(p->eti_field) ? (ExceptionTableSmall*)(&p->exceptionoffset)
                                                             : (ExceptionTableSmall*)(fragment->data_start + p->exceptionoffset);

    returnoffset -= p->functionoffset;

    if (ET_IsLargeTable(info->exception_record->et_field)) {
        ExceptionTableLarge* etl = (ExceptionTableLarge*)info->exception_record;
        ExceptionRangeLarge* erl;

        for (erl = etl->ranges; erl->start != 0; erl++) {
            u32 range_end = erl->start + (erl->size * 4);

            if (erl->start <= returnoffset && range_end >= returnoffset) {
                info->action_pointer = (char*)etl + erl->action;
                break;
            }
        }
    } else {
        ExceptionTableSmall* ets = (ExceptionTableSmall*)info->exception_record;
        ExceptionRangeSmall* ers;

        for (ers = ets->ranges; ers->start != 0; ers++) {
            if (ers->start <= returnoffset && ers->end >= returnoffset) {
                info->action_pointer = (char*)ets + ers->action;
                break;
            }
        }
    }
}

/* Reloads the caller's r31 out of the save area the callee's prologue built (the last saved GPR slot). */
static inline s32 ExPPC_PopR31(char* SP, MWExceptionInfo* info)
{
    f64* FPR_save_area;
    s32* GPR_save_area;
    int saved_GPRs, saved_FPRs;

    saved_FPRs = ET_GetSavedFPRs(info->exception_record->et_field);
    FPR_save_area = (f64*)(SP - saved_FPRs * 8);
    saved_GPRs = ET_GetSavedGPRs(info->exception_record->et_field);
    GPR_save_area = (s32*)FPR_save_area;

    return GPR_save_area[-1];
}

/* Advances the action iterator to the next cleanup record, unwinding a stack frame whenever the current
 * function's action list runs out, and returns the next action to perform. */
static exaction_type ExPPC_NextAction(ActionIterator* iter)
{
    exaction_type action;

    for (;;) {
        if (iter->info.action_pointer == 0
            || ((action = ((ex_destroylocal*)iter->info.action_pointer)->action) & EXACTION_ENDBIT) != 0) {
            char *return_addr, *callers_SP;

            callers_SP = *(char**)iter->current_SP;

            if (ET_GetSavedGPRs(iter->info.exception_record->et_field)) {
                iter->current_R31 = ExPPC_PopR31(callers_SP, &iter->info);
            }

            return_addr = *(char**)(callers_SP + RETURN_ADDRESS);

            ExPPC_FindExceptionRecord(return_addr, &iter->info);

            if (iter->info.exception_record == 0) {
                fn_8045670C();
            }

            iter->current_SP = callers_SP;
            iter->current_FP = (ET_GetHasFramePtr(iter->info.exception_record->et_field)) ? (char*)iter->current_R31
                                                                                         : iter->current_SP;

            if (iter->info.action_pointer == 0)
                continue;
        } else {
            switch (action) {
            case EXACTION_DESTROYLOCAL:
                iter->info.action_pointer += sizeof(ex_destroylocal);
                break;
            case EXACTION_DESTROYLOCALCOND:
                iter->info.action_pointer += sizeof(ex_destroylocalcond);
                break;
            case EXACTION_DESTROYLOCALPOINTER:
                iter->info.action_pointer += sizeof(ex_destroylocalpointer);
                break;
            case EXACTION_DESTROYLOCALARRAY:
                iter->info.action_pointer += sizeof(ex_destroylocalarray);
                break;
            case EXACTION_DESTROYBASE:
            case EXACTION_DESTROYMEMBER:
                iter->info.action_pointer += sizeof(ex_destroymember);
                break;
            case EXACTION_DESTROYMEMBERCOND:
                iter->info.action_pointer += sizeof(ex_destroymembercond);
                break;
            case EXACTION_DESTROYMEMBERARRAY:
                iter->info.action_pointer += sizeof(ex_destroymemberarray);
                break;
            case EXACTION_DELETEPOINTER:
                iter->info.action_pointer += sizeof(ex_deletepointer);
                break;
            case EXACTION_DELETEPOINTERCOND:
                iter->info.action_pointer += sizeof(ex_deletepointercond);
                break;
            case EXACTION_CATCHBLOCK:
                iter->info.action_pointer += sizeof(ex_catchblock);
                break;
            case EXACTION_CATCHBLOCK_32:
                iter->info.action_pointer += sizeof(ex_catchblock_32);
                break;
            case EXACTION_ACTIVECATCHBLOCK:
                iter->info.action_pointer += sizeof(ex_activecatchblock);
                break;
            case EXACTION_SPECIFICATION:
                iter->info.action_pointer
                    += sizeof(ex_specification) + ((ex_specification*)iter->info.action_pointer)->specs * sizeof(void*);
                break;
            default:
                fn_8045670C();
            }
        }

        action = ((ex_destroylocal*)iter->info.action_pointer)->action & EXACTION_MASK;

        if (action == EXACTION_BRANCH) {
            iter->info.action_pointer = ((char*)iter->info.exception_record) + ((ex_branch*)iter->info.action_pointer)->target;
            action = ((ex_destroylocal*)iter->info.action_pointer)->action & EXACTION_MASK;
        }
        return action;
    }
}
