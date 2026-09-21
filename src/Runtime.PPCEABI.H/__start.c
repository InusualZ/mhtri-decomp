/*
 * SDK startup: __start and the statics it runs on the way to main.
 *
 * .init 0x800062C0-0x800065C0 - __check_pad3 (0x28), __set_debug_bba (0xC), __get_debug_bba (0x8),
 * __start (0x16C), __init_registers (0x90), __init_data (0xA8), in that order, separated by 4-12 B of
 * padding: every function is 16-byte aligned, which only `-O4,p`'s implied -func_align 16 produces (-O3
 * packs them to 8 B and moves every symbol after the first), so the layout needs the cflags_runtime the
 * registration already uses. The range holds nothing else.
 *
 * Attribution: the map marks the five statics `scope:local` and their only caller is __start
 * (0x8000639C/0x80006454/0x80006458, __init_registers, __init_data), and the GCN-era SDK source
 * (pikmin2 `Dolphin/__start.c`) is this file function for function under `#pragma section code_type
 * ".init"`. __start is `scope:weak` (the SDK declares it `__declspec(weak)`) and is the DOL's entry point
 * (header entry 0x80006310). The next range, 0x800065C0-0x80006624 (__init_hardware, __flush_cache), is a
 * separate SDK file (Runtime.PPCEABI.H/__ppc_eabi_init.cpp).
 * Registered NonMatching under the Runtime.PPCEABI.H lib (Wii/1.3, cflags_runtime).
 *
 * Load-bearing source shapes (do not "clean up" without re-measuring):
 *   - `#pragma section code_type ".init"` for the file, and `asm`/`nofralloc` blocks for __start and
 *     __init_registers carrying the GCN asm text instruction for instruction (hint bits included: the two
 *     `beq+` sit at 0x800063C4/0x800063D0). The asm block resolves its `bl`/`lis` labels from the C
 *     declarations, so the functions defined below __start and the linker-script addresses (_stack_addr,
 *     _SDA2_BASE_, _SDA_BASE_) have to be declared before it or the compiler reports "undefined label".
 *   - __init_data must NOT carry a `#pragma scheduling off`: with it MWCC emits the epilogue as
 *     `lwz r31,28; lwz r30,24; lwz r29,20; lwz r0,36; mtlr r0`, where retail loads the LR first. The
 *     pragma is the whole difference - the optimizer level is not (O3, O3,p, O4 and
 *     -opt peephole[,schedule],level=3/4 all give retail's order).
 *   - the Wii port of the parse-args path stores the aligned argv base twice, to ARENAHI_ADDR (0x80000034)
 *     and to a second low-memory latch at 0x80003110 with no symbol anywhere in the repo
 *     (ARENAHI_MIRROR_ADDR below is a placeholder), and reads the device-code halfword at 0x800030E6
 *     rather than the GCN's 0x80000000 - DVD_DEVICECODE_ADDR is redefined to the Wii value.
 *   - __rom_copy_info's two leading words are `rom` then `addr` (the reverse of the GCN struct): retail's
 *     memcpy takes +4 as r3/dst and +0 as r4/src (0x8000653C/0x80006540/0x80006550).
 *   - Debug_BBA is the retail file's own `static u8`, and the retail object references it with
 *     R_PPC_EMB_SDA21 instead of defining it: its byte lives in the `.sbss` scaffold at 0x807953C8, which no
 *     unit can claim here - a `.sbss` split has to be 16-byte aligned (dtk refuses `auto_10_807953C9_sbss
 *     .sbss 10:0x807953C9`) and that block also holds PowerCallback/ResetCallback. dtk therefore names the
 *     scaffold symbol `Debug_BBA_807953C8`, and objdiff matches a relocation by name.
 *
 * Residual: the name on the two Debug_BBA SDA21 relocations, and nothing else. All six functions are
 * instruction- and hint-bit-identical (.rela.init identical, 30 relocations) and .init's first 760 bytes
 * match byte for byte - the target's 8 trailing bytes (0x800065B8) are the DOL's link-time padding before
 * __init_hardware, not object content. The mismatch leaves __set_debug_bba at 98.33 % and __get_debug_bba
 * at 97.5 %; a probe compiled with `extern u8 Debug_BBA_807953C8;` measures 100 % on both, so the missing
 * piece is dtk's scaffold name, not the source.
 */

#pragma section code_type ".init"

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;

/* Low-memory OS addresses (plain constants in the retail object: `lis`+`addi`, no relocation). */
#define EXCEPTIONMASK_ADDR      0x80000044
#define BOOTINFO2_ADDR          0x800000F4
#define ARENAHI_ADDR            0x80000034
#define DEBUGFLAG_ADDR          0x800030E8
#define DVD_DEVICECODE_ADDR     0x800030E6
#define PAD3_ADDR               0x800030E4
#define ARENAHI_MIRROR_ADDR     0x80003110

#define OS_BI2_DEBUGFLAG_OFFSET 0xC

#define TRUE 1

/* Linker-generated tables. `rom`/`addr` order is this (Wii) build's, see the header comment. */
typedef struct {
    u32 rom;
    u32 addr;
    u32 size;
} __rom_copy_info;

typedef struct {
    u32 addr;
    u32 size;
} __bss_init_info;

extern void OSResetSystem(int reset, u32 resetCode, int force);
extern void __init_hardware(void);
extern void DBInit(void);
extern void OSInit(void);
extern void InitMetroTRK(void);
extern void InitMetroTRK_BBA(void);
extern void __init_user(void);
extern int main(int argc, char** argv);
extern void exit(int status);
extern void __flush_cache(void* addr, u32 size);
extern void* memcpy(void* dst, const void* src, u32 size);
extern void* memset(void* dst, int val, u32 size);

extern u8 Debug_BBA;

/* Linker-script addresses loaded by __init_registers. */
extern u8 _stack_addr[];
extern u8 _SDA2_BASE_[];
extern u8 _SDA_BASE_[];

extern __rom_copy_info _rom_copy_info[];
extern __bss_init_info _bss_init_info[];

/* Referenced from the __start asm block before their own definitions; the assembler needs the C
 * declaration to resolve the label. */
static void __init_registers(void);
static void __init_data(void);

/* Resets the machine if pad 3 is held down (the low-memory pad-3 reset latch). */
static void __check_pad3(void)
{
    if ((*(u16*)PAD3_ADDR & 0x0EEF) == 0x0EEF) {
        OSResetSystem(0, 0, 0);
    }
    return;
}

/* Records that the BBA (network adapter) debug path was requested. */
static void __set_debug_bba(void) { Debug_BBA = 1; }

/* Reports whether the BBA debug path was requested. */
static u8 __get_debug_bba(void) { return Debug_BBA; }

// clang-format off

__declspec(weak) asm void __start(void)
{
	nofralloc
	bl __init_registers
	bl __init_hardware
	li r0, -1
	stwu r1, -8(r1)
	stw r0, 4(r1)
	stw r0, 0(r1)
	bl __init_data
	li r0, 0
	lis r6, EXCEPTIONMASK_ADDR@ha
	addi r6, r6, EXCEPTIONMASK_ADDR@l
	stw r0, 0(r6)
	lis r6, BOOTINFO2_ADDR@ha
	addi r6, r6, BOOTINFO2_ADDR@l
	lwz r6, 0(r6)

_check_TRK:
	cmplwi r6, 0
	beq _load_lomem_debug_flag
	lwz r7, OS_BI2_DEBUGFLAG_OFFSET(r6)
	b _check_debug_flag

_load_lomem_debug_flag:
	lis r5, ARENAHI_ADDR@ha
	addi r5, r5, ARENAHI_ADDR@l
	lwz r5, 0(r5)
	cmplwi r5, 0
	beq _goto_main
	lis r7, DEBUGFLAG_ADDR@ha
	addi r7, r7, DEBUGFLAG_ADDR@l
	lwz r7, 0(r7)

_check_debug_flag:
	li r5, 0
	cmplwi r7, 2
	beq _goto_inittrk
	cmplwi r7, 3
	li r5, 1
	beq _goto_inittrk
	cmplwi r7, 4
	bne _goto_main
	li r5, 2
	bl __set_debug_bba
	b _goto_main

_goto_inittrk:
	lis r6, InitMetroTRK@ha
	addi r6, r6, InitMetroTRK@l
	mtlr r6
	blrl

_goto_main:
	lis r6, BOOTINFO2_ADDR@ha
	addi r6, r6, BOOTINFO2_ADDR@l
	lwz r5, 0(r6)
	cmplwi r5, 0
	beq+ _no_args
	lwz r6, 8(r5)
	cmplwi r6, 0
	beq+ _no_args
	add r6, r5, r6
	lwz r14, 0(r6)
	cmplwi r14, 0
	beq _no_args
	addi r15, r6, 4
	mtctr r14

_loop:
	addi r6, r6, 4
	lwz r7, 0(r6)
	add r7, r7, r5
	stw r7, 0(r6)
	bdnz _loop
	lis r5, ARENAHI_ADDR@ha
	addi r5, r5, ARENAHI_ADDR@l
	rlwinm r7, r15, 0, 0, 0x1a
	stw r7, 0(r5)
	lis r5, ARENAHI_MIRROR_ADDR@ha
	addi r5, r5, ARENAHI_MIRROR_ADDR@l
	rlwinm r7, r15, 0, 0, 0x1a
	stw r7, 0(r5)
	b _end_of_parseargs

_no_args:
	li r14, 0
	li r15, 0

_end_of_parseargs:
	bl DBInit
	bl OSInit
	lis r4, DVD_DEVICECODE_ADDR@ha
	addi r4, r4, DVD_DEVICECODE_ADDR@l
	lhz r3, 0(r4)
	andi. r5, r3, 0x8000
	beq _check_pad3
	andi. r3, r3, 0x7fff
	cmplwi r3, 1
	bne _skip_crc

_check_pad3:
	bl __check_pad3

_skip_crc:
	bl __get_debug_bba
	cmplwi r3, 1
	bne _goto_skip_init_bba
	bl InitMetroTRK_BBA

_goto_skip_init_bba:
	bl __init_user
	mr r3, r14
	mr r4, r15
	bl main
	b exit
}

asm static void __init_registers(void)
{
	nofralloc
	li r0, 0
	li r3, 0
	li r4, 0
	li r5, 0
	li r6, 0
	li r7, 0
	li r8, 0
	li r9, 0
	li r10, 0
	li r11, 0
	li r12, 0
	li r14, 0
	li r15, 0
	li r16, 0
	li r17, 0
	li r18, 0
	li r19, 0
	li r20, 0
	li r21, 0
	li r22, 0
	li r23, 0
	li r24, 0
	li r25, 0
	li r26, 0
	li r27, 0
	li r28, 0
	li r29, 0
	li r30, 0
	li r31, 0
	lis r1, _stack_addr@h
	ori r1, r1, _stack_addr@l
	lis r2, _SDA2_BASE_@h
	ori r2, r2, _SDA2_BASE_@l
	lis r13, _SDA_BASE_@h
	ori r13, r13, _SDA_BASE_@l
	blr
}

// clang-format on

/* Copies one rom-copy range into place and flushes it out of the cache. */
inline static void __copy_rom_section(void* dst, const void* src, u32 size)
{
	if (size && (dst != src)) {
		memcpy(dst, src, size);
		__flush_cache(dst, size);
	}
}

/* Zeroes one bss range. */
inline static void __init_bss_section(void* dst, u32 size)
{
	if (size) {
		memset(dst, 0, size);
	}
}

/* Replays the linker-generated rom-copy and bss-init tables. */
static void __init_data(void)
{
	__rom_copy_info* dci;
	__bss_init_info* bii;

	dci = _rom_copy_info;
	while (TRUE) {
		if (dci->size == 0)
			break;
		__copy_rom_section((void*)dci->addr, (void*)dci->rom, dci->size);
		dci++;
	}

	bii = _bss_init_info;
	while (TRUE) {
		if (bii->size == 0)
			break;
		__init_bss_section((void*)bii->addr, bii->size);
		bii++;
	}
}
