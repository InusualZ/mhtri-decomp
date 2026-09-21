/*
 * SDK startup: __init_hardware and __flush_cache, the EABI initialisation that __start calls.
 *
 * .init 0x800065C0-0x80006624 - __init_hardware (0x24, 36 B) then a 12-byte gap (16-byte alignment of the
 * second function's input section) then __flush_cache (0x34, 52 B), in that order and nothing else in the
 * range.
 *
 * Attribution (batch 1, step 1): a different retail file than __start.c, on two counts - both functions
 * are `scope:global` in the map (the __start.c statics are `scope:local`), and the SDK keeps them in
 * `Dolphin/__ppc_eabi_init.cpp` (`__declspec(section ".init") asm void __init_hardware(void)` and
 * `__declspec(section ".init") asm void __flush_cache(unsigned long, int)`), which pikmin2 and prime both
 * carry. `__init_hardware` is called from __start at 0x80006314 and __flush_cache from __init_data at
 * 0x80006564 and from the MetroTRK flush path; nothing else in the DOL references either.
 * Registered NonMatching under the Runtime.PPCEABI.H lib (Wii/1.3, cflags_runtime).
 *
 * Both functions are `asm` bodies (the SDK shape), so the source *is* the instruction sequence and no
 * flag or source shape can move it. The bodies are pikmin2's file instruction for instruction (three init
 * calls, plain `sync`, `addic. r4, r4, -8`); prime's older two-call variant is the one that differs. Two
 * things are load-bearing rather than decoration: the three `__OS*Init` symbols must be declared before
 * the block - MWCC's inline assembler resolves `bl` through the C symbol table and reports an undeclared
 * target as `(10159) undefined label` - and the file is C++ (`.cpp`, `-lang=c++`), so the `extern "C"`
 * block is what keeps both global names unmangled like the map's symbols.
 *
 * Load-bearing source shapes:
 *   - `__declspec(section ".init")` on each definition: compiled as plain `.text` objdiff pairs nothing.
 *   - MWCC aligns `__flush_cache` to 16 bytes on its own, which reproduces the target's 12-byte gap after
 *     `__init_hardware` (the split object spells it `gap_00_800065E4_init`); both `.init` sections are
 *     100 B and the relocation pairs are identical.
 * Residual: none - both functions are byte-identical to the target (objdiff 100.0 %, code and relocations).
 *
 * Flags live in configure.py (`Runtime.PPCEABI.H` lib, Wii/1.3, cflags_runtime); asm bodies make them
 * irrelevant here.
 */

extern "C" {

void __OSPSInit(void);
void __OSFPRInit(void);
void __OSCacheInit(void);

__declspec(section ".init") asm void __init_hardware(void)
{
    nofralloc
    mfmsr r0
    ori r0, r0, 0x2000
    mtmsr r0
    mflr r31
    bl __OSPSInit
    bl __OSFPRInit
    bl __OSCacheInit
    mtlr r31
    blr
}

__declspec(section ".init") asm void __flush_cache(unsigned long address, int size)
{
    nofralloc
    lis r5, 0xFFFFFFF1@h
    ori r5, r5, 0xFFFFFFF1@l
    and r5, r5, r3
    subf r3, r5, r3
    add r4, r4, r3
loop:
    dcbst 0, r5
    sync
    icbi 0, r5
    addic r5, r5, 8
    addic. r4, r4, -8
    bge loop
    isync
    blr
}

} // extern "C"
