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
 * Attribution: MSL's `Runtime.PPCEABI.H/Gecko_ExceptionPPC.cp`, the Wii revision: `fragmentinfo` is the
 * 32-slot table (`.bss:0x806F4B48`, size 0x180 = 32 * sizeof(ProcessInfo)) the `mtctr` registration loop
 * walks, where pikmin2's and prime's GCN copies have one slot. The out-of-line throw machinery (`__throw` at
 * 0x80458A60, `__unexpected`, `ExPPC_ThrowHandler`) is registered as the adjacent `ExceptionPPC.cp`
 * (`.text` 0x804578FC-0x80458BDC), whose target object calls this file's walkers; the two are one retail TU
 * (`ExceptionPPC.cp`'s header has the measured evidence), and the shared record types live in
 * `Gecko_ExceptionPPC.h` until they are folded.
 * The struct/macro vocabulary and the function bodies are the ones the same revision carries in
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
 *      included) with `.text` unchanged, re-checked with `objcopy --only-section` + `cmp` (extab 16 B,
 *      extabindex 24 B, both identical). It cannot be a lib-wide flag: the same lib's
 *      `__init_cpp_exceptions.cpp` *gains* extab/extabindex under it while its target object has none, so it
 *      belongs in the object's own `extra_cflags`.
 *
 * Residual / open items (measured against the target object; the percentages are per-symbol objdiff):
 *   - `__register_fragment` 93.68 % is the unit's only remaining code difference: the two independent preheader
 *     instructions `addi r5,r5,fragmentinfo@l` (the base) and `li r6,0` (the index) come out in the opposite
 *     order (76/76 B, 19/19 ins, identical relocations). It is a `-O4,p` scheduler tie-break, not a source
 *     shape: ~1000 shapes (every declaration / for-init / increment / condition / body combination, the
 *     `f = fragmentinfo + i` and `f[i]` derived-pointer forms, `while` / `do` / `for(;;)`, `register`,
 *     `u32`/`long` indices) reproduce the same two-row swap, and the `-opt` axis (`no<pass>` for every
 *     documented pass, `-O3`, `-schedule off`, `-opt display`) plus the internal `#pragma opt_*` set (85 names,
 *     on and off) only permutes the same three instructions. Three things do emit retail's exact order and
 *     none is usable: `-proc 7450`/`744X`/`e600` (and the pre-3.0 GC compilers) fix this function but drop the
 *     walkers (to 94.47 / 97.04 / 99.44 % under 7450),
 *     `-inline deferred` fixes all five but reverses `.text` (it emits in reverse source order, so
 *     `__register_fragment` lands last), and `#pragma opt_repositioncode on` / `opt_rotateloops on` change the
 *     loop itself. `-opt nopeephole` scores 99.37 % here, but it is the same two-row swap with `li r0,32` and
 *     `li r6,0` transposed - the number moves, the code does not - so it is a metric artifact and is
 *     deliberately **not** landed (a scoped pragma would also be a per-function flag, playbook 33).
 *   - `fragmentinfo` is defined here and its `.bss 0x806F4B48-0x806F4CC8` (0x180) is claimed in `splits.txt`,
 *     which pairs the unit's `.bss` with the target's. The `.data` jump table `0x8060E8A0-0x8060E8E4` is
 *     claimed too, which is what pairs our jump table with `jumptable_8060E8A0`; this object
 *     loads no `.sdata2`/`.sdata` constant (no such section, no pool relocation).
 *   - `ExPPC_FindExceptionFragment` is `static`; `ExPPC_FindExceptionRecord` and `ExPPC_NextAction` are
 *     declared in `Gecko_ExceptionPPC.h` for `ExceptionPPC.cp` (codegen-neutral: the object is byte-identical
 *     apart from the two symbols' binding). The map's `fn_80457504`, `fn_8045759C` and `fn_8045774C` were renamed to
 *     this file's mangled names and all three measure 100 %. `fragmentinfo` used to be spelled
 *     `fragmentinfo_806F4B48` in the target object (the map marked it `scope:local`, and dtk spells a local
 *     symbol `<name>_<address>`); declaring it `scope:global` makes both sides say `fragmentinfo`. Neither
 *     rename moved the score - both were latent pairing traps, not the residual.
 *   - `ExPPC_NextAction` calls `std::terminate` (`terminate__3stdFv`, 0x8045670C, owned by `CPlusLibPPC.cpp`
 *     and declared in `CPlusLibPPC.h`) when the unwind cannot continue.
 */

/* The target object carries its own extab/extabindex (0x10 + 0x18) and the lib's flags say
 * `-Cpp_exceptions off`, so the unit turns them back on for itself - the source-level spelling of the flag the
 * measurement used. It changes nothing else in the object. */
#pragma exceptions on

#include "types.h"
#include "Runtime.PPCEABI.H/CPlusLibPPC.h"
#include "Runtime.PPCEABI.H/Gecko_ExceptionPPC.h"

#define MAXFRAGMENTS   32

/* The 32-slot fragment table (`.bss:0x806F4B48`, 0x180) the registration functions fill and the walkers search. */
ProcessInfo fragmentinfo[MAXFRAGMENTS];

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
void ExPPC_FindExceptionRecord(char* returnaddr, MWExceptionInfo* info)
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
exaction_type ExPPC_NextAction(ActionIterator* iter)
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
                std::terminate();
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
                std::terminate();
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
