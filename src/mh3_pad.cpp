/*
 * Naming note: the symbol map has only fn_XXXXXXXX for the unnamed functions in this range
 * (checked with `python tools/symbols/dumpmap.py lookup <addr>`: every unnamed entry in this range is a
 * bare `zz_<addr>_` placeholder in the runtime dump as well, so there is no real name to recover).  The
 * functions the map does name (`Tsk_Change`, `GameModeExec`, `cnvt_eur_fname`, `initKPAD`, ...) use
 * their real C++ signatures.
 *
 * `mh3_pad.cpp` - the game-root pad / mode file.  `.text` 0x800408A8-0x80046C80 (125 functions); phase 4 cut the pad-connect tail (0x80046C80-0x80047398) into `pad_connect.cpp`.
 *
 * Name evidence (docs/plan.md 12, evidence class 1): the unit's own `fn_80041AA4` (the WPAD init
 * routine) reaches an OSPanic whose file argument is the bare source name string
 * `"mh3_pad.cpp"` at .data:0x80580EF0 (`build/RMHE08/asm/auto_07_8057C820_data.s` line 5016); the
 * neighbouring `"MEM2 heap allocation error.\n"` at 0x80580EFC is the panic message.  No other
 * `.cpp`/`.c` string is referenced anywhere in the range.  Module: the address band and the sibling
 * units are the game-root system files (`main.cpp`, `sys_mem.cpp`, `fn_80040598.cpp`,
 * `nw_resource.cpp`) in configure.py's `main` lib, so the file is top-level `src/mh3_pad.cpp`.
 *
 * Seam: the left edge 0x800408A8 is settled (`fn_80040598.cpp`'s extab group ends exactly there and
 * this unit's first extab record starts at 0x80006820).  The right edge 0x80046C80 is where the reconciled
 * candidate puts `pad_connect.cpp` (the proposal's old edge 0x80047398 was a `--max-bytes` cap, not a TU boundary).
 *
 * Sections: .text 0x800408A8-0x80046C80, and the .ctors word 0x8056F2C4-0x8056F2C8 (dtk assigned it to
 * this unit on the split; the first C++ static constructor of the file).
 *
 * `.bss` 0x806585B8-0x806694D0 is this unit's own (claimed, defined at the foot of the file, every symbol offset and
 * size equal to the target): `system_w`, `Screen_w`, `option_w`, `lb_param_w`, the `Psw` pad records and their twin, the
 * task table, the WPAD sampling buffers and the pointer/mutex state.  Evidence: the static constructor
 * `fn_80046B94` (`.ctors` word 0x8056F2C4) constructs `Psw` and `Psw_prev` as 4 x 0x350 B arrays, and the window is bounded by
 * `main.cpp`'s `.bss` below and `pad_connect.cpp`'s `game_mutex` above.  The two pad arrays' constructor is not reconstructed (plain
 * storage here); the names marked GUESS at their definitions (`Psw_prev`, `pad_chan_state`, `pad_btn_table_*`, `kpad_work`,
 * `pointer_*`) are derived from the functions that touch them.  `lbl_807419C8` (the RcRecord table) is not
 * in this window: it sits in the DVD library's `.bss`.
 *
 * This session reconstructed 67 of the 145 functions to the 80 % bar: the RSO sub-overlay loader state
 * machine (fn_800408A8..fn_80040FDC), the task-slot table (fn_80041640..fn_80041944), the vector
 * helpers (copyVec3..fn_80041E9C), the RcRecord accessors (fn_80042B34..fn_80042F60), the pad
 * callbacks and the Psw pad-record accessors, the KPAD/WPAD setup and the small flag/word helpers.
 *
 * Residuals (measured against the target object, symbol by symbol):
 *   - fn_80040AC4 / fn_80040B98 / fn_80040ED0 (79.2 / 79.4 / 79.1 %): the loader state functions keep
 *     the pool array and the name table in two registers (r30/r31) across both loads; ours re-materialises
 *     the base each time, so ~10 of the ~47 instructions differ by register only.  Same byte sizes.
 *   - fn_8004192C (79.2 %): the target sign-extends the s16 slot (`extsh r0,r3`) before the `slwi r3,5`;
 *     ours shifts directly - a compiler shape difference, same instruction count.
 *   - fn_80041644 (55.5 %): the retail object narrows each argument with `clrlwi r0,rN,24` in front of
 *     its `stb`; MWCC emits no mask for `(u8)rN` with a `u32`/`u8` parameter (measured with this flag
 *     set), so 4 of the 10 instructions are missing.
 *   - fn_80044A30 (75.0 %): register colouring only (the target keeps the zero in r5 and the base in r4).
 *   - the remaining 72 symbols have no body yet (0 %, unpaired): they are the WPAD-init / mode-exec /
 *     GX-draw / timer groups and the Psw ring-buffer helpers.  Several need fields of `system_w` whose
 *     shared-header spellings are still `unkNN` (rule 7 keeps the `unk` half enforced), so they are
 *     deferred until the header is renamed - see the unit report's residual backlog.
 *
 * config_requests: the `unsplit/unknown.h` field additions (field_0x20/0x21, field_0x33[4]/
 * field_0x37[4], field_0x7dc[4], field_0x868 - all previously `pad_*`), the `RSOModule` hoist out of
 * `src/RSO/runtime.c`, and the unsplit externs this unit needs.  See the outbox.
 */

#include "types.h"
#include "gx.h"
#include "nw4r/math.h"
#include "unsplit/unknown.h" /* SystemWork / system_w (rule 1/2) */
#include "mh3_pad/Psw.h"        /* PlayerPad / Psw (rule 1/2) */
#include "mh3_pad/Screen_w.h"   /* ScreenWork / Screen_w (rule 1/2) */
#include "mh3_pad/lb_param_w.h" /* LbParamWork / lb_param_w (rule 1/2) */
#include "mh3_pad/option_w.h"   /* option_w (rule 2) */
#include "OS/mem.h"            /* MEMAllocator (rule 1) */
#include "RSO/runtime.h"     /* RSOModule + RSOStaticLocateObject (rule 2) */
#include "fn_80040598.h"     /* the game-root RSO loaders (rule 2) */
#include "ef/fn_800CDB2C.h"  /* GameMode_ck / fn_800CEE2C (rule 2) */
#include "mh3_pad/task.h"    /* `TaskSlot` (rule 1) */
#include "quest/arenatask.h" /* arena_task: the arena-select task ArenaSelExec installs (rule 2) */
#include "Runtime.PPCEABI.H/memset.h"
#include "g3d/fn_80075DCC.h" /* fn_8007A510 (rule 2) */

/* ------------------------------------------------------------------ *
 * Local types
 * ------------------------------------------------------------------ */

/* `TaskSlot` (the 0x20-byte task slot `task_slot_table` indexes by `slot << 5`) lives in `mh3_pad/task.h`. */

/* `PlayerPad` (the 0x350-byte per-player pad record at `Psw`) lives in `mh3_pad/Psw.h`. */

/* The 0x18-byte per-channel state record of `pad_chan_state` (`fn_80041AA4` clears one per channel next to
 * the `Psw` records); only the motor byte at +0x14 is read so far. */
typedef struct PadChanState {
    /* +0x00 */ u8 pad_0x00[0x14];
    /* +0x14 */ u8 motor_level_0x14;  /* copied into `system_w.field_0x7dc` (the per-channel motor-on state) */
    /* +0x15 */ u8 pad_0x15[3];
} PadChanState; /* size: 0x18 */

/* The 0x1E-byte record table at `lbl_807419C8` (fn_80042B34 and the fn_80042EE8..fn_80042F60
 * accessors read single bytes out of it). */
typedef struct RcRecord {
    /* +0x00 */ u16 field_0x00;
    /* +0x02 */ u8 field_0x02;
    /* +0x03 */ u8 field_0x03;
    /* +0x04 */ u8 field_0x04;
    /* +0x05 */ u8 field_0x05;
    /* +0x06 */ u8 field_0x06;
    /* +0x07 */ u8 field_0x07;
    /* +0x08 */ u8 pad_0x08[0x2];
    /* +0x0A */ u8 field_0x0a;
    /* +0x0B */ u8 pad_0x0b[0x13];
} RcRecord; /* size: 0x1E */

/* ------------------------------------------------------------------ *
 * Extern declarations (all unsplit addresses - rule 2's named gap; see config_requests)
 * ------------------------------------------------------------------ */

extern "C" {
/* --- RSO load state (.sbss 0x807947B4..0x807947F0) --- */
extern u32 lbl_807947B4; /* load base */
extern u32 lbl_807947B8;
extern u32 lbl_807947BC;
extern u32 lbl_807947C0;
extern u8 lbl_807947C8[]; /* [0] state, [1] sub-state */
extern u32 lbl_807947D0;
extern RSOModule* lbl_807947D4;
extern RSOModule* lbl_807947D8;
extern RSOModule* lbl_807947DC;
extern RSOModule* lbl_807947E0;
extern RSOModule* lbl_807947E4;
extern u8 lbl_807947F0; /* KPAD initialised flag */

/* --- .sdata --- */
extern u8 lbl_80790E28[];   /* per-slot RSO pool index */
extern char lbl_80790E30[]; /* "mh3.sel" */

/* --- .data string / table pool --- */
extern char* lbl_8057C9D8[]; /* RSO path pointers */
extern u8 lbl_8057CA28[];
extern char lbl_8057CA40[];
extern char lbl_8057CA50[];
extern char lbl_805803EC[];

/* --- .bss --- */
extern u32 rso_slot_load_base[5];
extern u32 rso_slot_unused_words[5];
extern TaskSlot task_slot_table[16];
extern MEMAllocator wpad_mem2_allocator; /* MEM2 allocator record */

/* --- .sbss table --- */
extern RcRecord lbl_807419C8[];

/* --- .sdata2 --- */
extern f32 lbl_80795AE8; /* 0.0f */
extern f32 lbl_80795AE0;
extern f32 lbl_80795AE4;

/* --- helpers owned by other units (rule 2 headers included above) --- */
extern void DCFlushRange(void* addr, u32 size);
extern void fn_804DAF94(void* module);
extern void fn_804C08A0(void);
extern void fn_804C0D00(void);
extern void fn_804ED5A0(void);
extern void fn_804ED5D0(void);
extern void fn_80043728(void);
extern void WPADSetExtensionCallback(int chan, void* callback);
extern void WPADSetSamplingCallback(int chan, void* callback);
extern void WPADControlMotor(int chan, u32 cmd, u32 time);
extern void* MEMCreateExpHeapEx(void* base, u32 size, u16 attr);
extern void* MEMAllocFromAllocator(void* allocator, u32 size);
extern void MEMFreeToAllocator(void* allocator, void* block);
extern char* strrchr(const char* s, int c);
extern void fn_80523490(void);

/* task-entry bodies referenced by address from the mode dispatchers (.text, other units) */
extern s32 task_func;
extern void fn_8028DDCC(void);
extern void fn_8028E528(void);
extern void fn_8028BF1C(void);
extern void fn_803A13B4(void);
extern void fn_8021F3A8(void);
extern PadChanState pad_chan_state[4];
}

/* ------------------------------------------------------------------ *
 * 0x800408A8 - 0x80040FDC  (RSO sub-overlay loader state machine)
 * ------------------------------------------------------------------ */

/* Boot: clear the sub-overlay state, load `mh3.sel` into the pool top, publish it and run its
 * prolog. */
extern "C" void fn_800408A8(void)
{
    u32 i;

    for (i = 0; i < 5; i++) {
        lbl_807947C8[i] = 0xFF;
    }
    lbl_807947D0 = (u32)CntSdRsoTerminate(lbl_80790E30, (void*)0x80810000);
    lbl_807947B8 = lbl_807947B4;
    lbl_807947BC = lbl_807947B4 + 0x170000;
    lbl_807947D4 = (RSOModule*)fn_80040598(lbl_8057CA40, (void*)lbl_807947B4, 0xFF);
    fn_804DA7E4(lbl_807947D4);
    lbl_807947D4->prolog(lbl_807947D4);
}

extern "C" void fn_80040928(void)
{
    lbl_807947D0 = (u32)CntSdRsoTerminate(lbl_80790E30, (void*)0x80810000);
}

/* Release every loaded sub-overlay and reset the loader state. */
extern "C" void fn_80040954(void)
{
    s32 loadedD8;
    s32 loadedDC;
    s32 loadedE0;
    s32 loadedE4;

    loadedD8 = 0;
    loadedDC = 0;
    loadedE0 = 0;
    loadedE4 = 0;
    lbl_807947C0 = lbl_807947BC;
    rso_slot_load_base[0] = 0;
    rso_slot_unused_words[0] = 0;
    rso_slot_load_base[1] = 0;
    rso_slot_unused_words[1] = 0;
    rso_slot_load_base[2] = 0;
    rso_slot_unused_words[2] = 0;
    rso_slot_load_base[3] = 0;
    rso_slot_unused_words[3] = 0;
    rso_slot_load_base[4] = 0;
    rso_slot_unused_words[4] = 0;
    if (lbl_807947D8 != NULL) {
        lbl_807947D8->epilog(lbl_807947D8);
        loadedD8 = 1;
    }
    if (lbl_807947DC != NULL) {
        lbl_807947DC->epilog(lbl_807947DC);
        loadedDC = 1;
    }
    if (lbl_807947E0 != NULL) {
        lbl_807947E0->epilog(lbl_807947E0);
        loadedE0 = 1;
    }
    if (lbl_807947E4 != NULL) {
        lbl_807947E4->epilog(lbl_807947E4);
        loadedE4 = 1;
    }
    if (loadedD8 != 0) {
        fn_804DAF94(lbl_807947D8);
        lbl_807947D8 = NULL;
    }
    if (loadedDC != 0) {
        fn_804DAF94(lbl_807947DC);
        lbl_807947DC = NULL;
    }
    if (loadedE0 != 0) {
        fn_804DAF94(lbl_807947E0);
        lbl_807947E0 = NULL;
    }
    if (loadedE4 != 0) {
        fn_804DAF94(lbl_807947E4);
        lbl_807947E4 = NULL;
    }
    lbl_807947C8[0] = 0xFF;
    lbl_807947C8[1] = 0xFF;
}

/* Load the two-overlay mode (state 2). */
extern "C" void fn_80040AC4(void)
{
    u32* pools;
    char** names;
    u8* idx;

    if ((s8)lbl_807947C8[0] != 2 && (s8)lbl_807947C8[0] != 4) {
        fn_80040954();
        pools = rso_slot_load_base;
        names = lbl_8057C9D8;
        idx = lbl_80790E28;
        pools[idx[0]] = lbl_807947C0;
        lbl_807947D8 = (RSOModule*)fn_80040598(names[0], (void*)pools[idx[0]], idx[0]);
        pools[idx[1]] = lbl_807947C0;
        lbl_807947DC = (RSOModule*)fn_80040598(names[1], (void*)pools[idx[1]], idx[1]);
        fn_804DA7E4(lbl_807947D8);
        fn_804DA7E4(lbl_807947DC);
        lbl_807947D8->prolog(lbl_807947D8);
        lbl_807947DC->prolog(lbl_807947DC);
        lbl_807947C8[0] = 2;
    }
}

/* Load the three-overlay mode (state 1). */
extern "C" void fn_80040B98(void)
{
    if ((s8)lbl_807947C8[0] != 1) {
        fn_80040954();
        rso_slot_load_base[lbl_80790E28[0]] = lbl_807947C0;
        lbl_807947D8 = (RSOModule*)fn_80040598(lbl_8057C9D8[0], (void*)rso_slot_load_base[lbl_80790E28[0]],
                                                lbl_80790E28[0]);
        rso_slot_load_base[lbl_80790E28[2]] = lbl_807947C0;
        lbl_807947DC = (RSOModule*)fn_80040598(lbl_8057C9D8[2], (void*)rso_slot_load_base[lbl_80790E28[2]],
                                                lbl_80790E28[2]);
        rso_slot_load_base[lbl_80790E28[3]] = lbl_807947C0;
        lbl_807947E0 = (RSOModule*)fn_80040598(lbl_8057C9D8[3], (void*)rso_slot_load_base[lbl_80790E28[3]],
                                                lbl_80790E28[3]);
        fn_804DA7E4(lbl_807947D8);
        fn_804DA7E4(lbl_807947DC);
        fn_804DA7E4(lbl_807947E0);
        lbl_807947C8[0] = 1;
        lbl_807947D8->prolog(lbl_807947D8);
        lbl_807947DC->prolog(lbl_807947DC);
        lbl_807947E0->prolog(lbl_807947E0);
        lbl_807947C8[0] = 1;
    }
}

/* Load the four-overlay mode (state 3). */
extern "C" void fn_80040CA8(void)
{
    if ((s8)lbl_807947C8[0] != 3) {
        fn_80040954();
        rso_slot_load_base[lbl_80790E28[0]] = lbl_807947C0;
        lbl_807947D8 = (RSOModule*)fn_80040598(lbl_8057C9D8[0], (void*)rso_slot_load_base[lbl_80790E28[0]],
                                                lbl_80790E28[0]);
        rso_slot_load_base[lbl_80790E28[1]] = lbl_807947C0;
        lbl_807947DC = (RSOModule*)fn_80040598(lbl_8057C9D8[1], (void*)rso_slot_load_base[lbl_80790E28[1]],
                                                lbl_80790E28[1]);
        rso_slot_load_base[lbl_80790E28[3]] = lbl_807947C0;
        lbl_807947E0 = (RSOModule*)fn_80040598(lbl_8057C9D8[4], (void*)rso_slot_load_base[lbl_80790E28[3]],
                                                lbl_80790E28[3]);
        rso_slot_load_base[3] = lbl_807947C0;
        lbl_807947E4 = (RSOModule*)fn_80040598(lbl_8057C9D8[2], (void*)lbl_807947C0, 3);
        fn_804DA7E4(lbl_807947D8);
        fn_804DA7E4(lbl_807947DC);
        fn_804DA7E4(lbl_807947E0);
        fn_804DA7E4(lbl_807947E4);
        lbl_807947C8[0] = 3;
        lbl_807947D8->prolog(lbl_807947D8);
        lbl_807947DC->prolog(lbl_807947DC);
        lbl_807947E0->prolog(lbl_807947E0);
        lbl_807947E4->prolog(lbl_807947E4);
        lbl_807947C8[0] = 3;
    }
}

/* Reload the language overlay (state byte 1) selected by `which`. */
extern "C" void fn_80040DE8(u8 which)
{
    u8 sel;

    sel = lbl_8057CA28[which];
    if ((s8)lbl_807947C8[1] == -1 || (s8)lbl_807947C8[1] != (s8)sel) {
        if (lbl_807947E4 != NULL) {
            lbl_807947E4->epilog(lbl_807947E4);
            fn_804DAF94(lbl_807947E4);
            lbl_807947E4 = NULL;
        }
        DCFlushRange((void*)lbl_807947C0, (u32)(0x80C4CE00 - lbl_807947C0));
        rso_slot_load_base[lbl_80790E28[4]] = lbl_807947C0;
        lbl_807947E4 = (RSOModule*)fn_80040598(lbl_8057C9D8[sel], (void*)rso_slot_load_base[lbl_80790E28[4]],
                                                lbl_80790E28[4]);
        fn_804DA7E4(lbl_807947E4);
        lbl_807947E4->prolog(lbl_807947E4);
        lbl_807947C8[1] = sel;
    }
}

/* Load the fifth-overlay mode (state 4). */
extern "C" void fn_80040ED0(void)
{
    if ((s8)lbl_807947C8[0] != 4) {
        fn_80040954();
        rso_slot_load_base[lbl_80790E28[0]] = lbl_807947C0;
        lbl_807947D8 = (RSOModule*)fn_80040598(lbl_8057C9D8[0], (void*)rso_slot_load_base[lbl_80790E28[0]],
                                                lbl_80790E28[0]);
        rso_slot_load_base[lbl_80790E28[1]] = lbl_807947C0;
        lbl_807947DC = (RSOModule*)fn_80040598(lbl_8057C9D8[1], (void*)rso_slot_load_base[lbl_80790E28[1]],
                                                lbl_80790E28[1]);
        rso_slot_load_base[lbl_80790E28[5]] = lbl_807947C0;
        lbl_807947E0 = (RSOModule*)fn_80040598(lbl_8057C9D8[0x13], (void*)rso_slot_load_base[lbl_80790E28[5]],
                                                lbl_80790E28[5]);
        fn_804DA7E4(lbl_807947D8);
        fn_804DA7E4(lbl_807947DC);
        fn_804DA7E4(lbl_807947E0);
        lbl_807947D8->prolog(lbl_807947D8);
        lbl_807947DC->prolog(lbl_807947DC);
        lbl_807947E0->prolog(lbl_807947E0);
        lbl_807947C8[0] = 4;
    }
}

/* Reset the two pool tops. */
extern "C" void fn_80040FDC(void)
{
    lbl_807947C0 = lbl_807947B8;
    lbl_807947BC = lbl_807947B8;
}

/* ------------------------------------------------------------------ *
 * 0x80041404 - 0x800418B0  (font helpers and the task-slot table)
 * ------------------------------------------------------------------ */

extern "C" char* fn_80041404(char* s, int c)
{
    return strrchr(s, c);
}

extern "C" void fn_80041408(void)
{
    fn_80523490();
}

extern "C" void fn_80041640(void)
{
}

/* GX immediate-mode colour (writes the 0xCC008000 write-gather pipe). */
extern "C" void fn_80041644(u32 r, u32 g, u32 b, u32 a)
{
    *(volatile u8*)0xCC008000 = (u8)r;
    *(volatile u8*)0xCC008000 = (u8)g;
    *(volatile u8*)0xCC008000 = (u8)b;
    *(volatile u8*)0xCC008000 = (u8)a;
}

extern "C" void fn_8004166C(f32 x, f32 y, f32 z)
{
    *(volatile f32*)0xCC008000 = x;
    *(volatile f32*)0xCC008000 = y;
    *(volatile f32*)0xCC008000 = z;
}

extern "C" void fn_80041680(void)
{
    memset(task_slot_table, 0, 0x200);
}

/* Start a task slot running `func` (state 0xC). */
extern "C" void fn_800417F0(void* func, s16 slot)
{
    TaskSlot* task;

    task = &task_slot_table[slot];
    memset(task, 0, 0x20);
    task->state = 0xC;
    task->func = (void (*)(TaskSlot*))func;
}

extern "C" void fn_80041850(s16* timer)
{
    *timer = 0;
}

extern "C" void fn_8004185C(s16 slot)
{
    task_slot_table[slot].state = 0x10;
}

extern "C" void fn_80041878(s16 slot)
{
    task_slot_table[slot].state = 1;
}

extern "C" void fn_80041894(s16 slot)
{
    task_slot_table[slot].state = 2;
}

extern "C" void fn_800418B0(s16 slot)
{
    task_slot_table[slot].state = 0;
}

/* Start a task slot with a fresh body (state 8).  `Tsk_Change__FPvs`: (void*, short). */
void Tsk_Change(void* func, s16 slot)
{
    TaskSlot* task;

    task = &task_slot_table[slot];
    memset(task, 0, 0x20);
    task->state = 8;
    task->func = (void (*)(TaskSlot*))func;
}

extern "C" TaskSlot* fn_8004192C(s16 slot)
{
    return &task_slot_table[slot];
}

/* The arena-select task entry: load the four-overlay mode and hand slot 4 to `arena_task`. */
extern "C" void fn_80041944(void)
{
    fn_80040CA8();
    if (task_func != 0) {
        Tsk_Change((void*)task_func, 4);
    }
}

extern "C" void fn_80041A0C(void)
{
    Tsk_Change((void*)&fn_8028DDCC, 4);
}

extern "C" void fn_80041A30(void)
{
}

void ArenaSelExec(void)
{
    fn_80040CA8();
    Tsk_Change((void*)&arena_task, 4);
}

extern "C" void fn_80041A64(void* block)
{
    MEMAllocFromAllocator(&wpad_mem2_allocator, (u32)block);
}

extern "C" s32 fn_80041A74(void* block)
{
    MEMFreeToAllocator(&wpad_mem2_allocator, block);
    return 1;
}

void VsGameModeExec(void)
{
    Tsk_Change((void*)&fn_8028E528, 4);
}

void TestModeExec(void)
{
}

/* ------------------------------------------------------------------ *
 * 0x80041E40 - 0x80041E9C  (vector helpers)
 * ------------------------------------------------------------------ */

/* The record these three work on is `nw4r::math::VEC3` (0xC, x/y/z at +0/+4/+8) - see
 * `mh3_pad/vec3.h` for the evidence (`setVec3`'s body is byte-identical to the map's
 * `setVector3__FPQ34nw4r4math4VEC3fff`).  The parameters were `void*` until the type-fix pass. */
extern "C" void fn_80041E70(VEC3* dst, const VEC3* src)
{
    dst->x = src->x;
    dst->y = src->y;
    dst->z = src->z;
}

extern "C" VEC3* copyVec3(VEC3* dst, const VEC3* src)
{
    fn_80041E70(dst, src);
    return dst;
}

/* The return is the first argument, as the callers use it (`mr r4,r3` after the `bl`); the body
 * leaves r3 alone, so `return out;` costs no instruction. */
extern "C" VEC3* setVec3(VEC3* out, f32 x, f32 y, f32 z)
{
    out->x = x;
    out->y = y;
    out->z = z;
    return out;
}

extern "C" void* fn_80041E9C(void* base, u32 size)
{
    return MEMCreateExpHeapEx(base, size, 0);
}

/* ------------------------------------------------------------------ *
 * 0x80042B34 - 0x80042F60  (RcRecord accessors)
 * ------------------------------------------------------------------ */

extern "C" u16 fn_80042B34(s32 idx)
{
    return lbl_807419C8[idx].field_0x00;
}

extern "C" u8 fn_80042B48(s32 idx)
{
    return lbl_807419C8[idx].field_0x0a;
}

extern "C" u8 fn_80042EE8(s32 idx)
{
    return lbl_807419C8[idx].field_0x05;
}

extern "C" u8 fn_80042F00(s32 idx)
{
    return lbl_807419C8[idx].field_0x04;
}

extern "C" u8 fn_80042F18(s32 idx)
{
    return lbl_807419C8[idx].field_0x03;
}

extern "C" u8 fn_80042F30(s32 idx)
{
    return lbl_807419C8[idx].field_0x02;
}

extern "C" u8 fn_80042F48(s32 idx)
{
    return lbl_807419C8[idx].field_0x07;
}

extern "C" u8 fn_80042F60(s32 idx)
{
    return lbl_807419C8[idx].field_0x06;
}

/* ------------------------------------------------------------------ *
 * 0x800438A4 - 0x80045330  (pad callbacks and vector copies)
 * ------------------------------------------------------------------ */

extern "C" void fn_800438A4(void)
{
}

extern "C" void fn_800438A8(s32 arg0)
{
    if (arg0 == 0) {
        fn_804ED5A0();
    }
}

extern "C" void fn_800438B8(s32 arg0)
{
    if (arg0 == 0) {
        fn_804ED5D0();
    }
}

/* The retail body is a bare `blr`; the parameter is the record the call sites pass. */
extern "C" void VEC3_ctor(VEC3* out)
{
    (void)out;
}

extern "C" void fn_80045164(f32* dst, f32* src)
{
    dst[0] = src[0];
    dst[1] = src[1];
}

extern "C" void fn_80045330(f32* dst, f32* src)
{
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
}

/* ------------------------------------------------------------------ *
 * 0x800438C8 - 0x800441F8  (Psw pad-record accessors)
 * ------------------------------------------------------------------ */

u16 get_rcSwitch_on(s32 chan)
{
    PlayerPad* pad;

    pad = &Psw[chan];
    switch (pad->mode) {
    case 0:
        return pad->field_0xce;
    case 1:
        return pad->field_0x100;
    case 2:
        return pad->field_0xda;
    default:
        return 0;
    }
}

u16 get_rcSwitch_rept(s32 chan)
{
    PlayerPad* pad;

    pad = &Psw[chan];
    switch (pad->mode) {
    case 0:
        return pad->field_0xd2;
    case 1:
        return pad->field_0x11e;
    case 2:
        return pad->field_0xf8;
    default:
        return 0;
    }
}

s32 get_ControlType(s32 chan)
{
    PlayerPad* pad;

    pad = &Psw[chan];
    if (pad->field_0x30 != 0) {
        return 0xFF;
    }
    switch (pad->mode) {
    case 0:
        return 0;
    case 1:
        return 1;
    case 2:
        return 2;
    default:
        return 0xFF;
    }
}

void set_menu_key_ad_flag(u8 chan, bool flag)
{
    PlayerPad* pad;

    pad = &Psw[chan];
    if (flag) {
        pad->field_0x335 = 1;
    } else {
        pad->field_0x335 = 0;
    }
}

/* ------------------------------------------------------------------ *
 * 0x80044C8C - 0x80044CF4  (KPAD / WPAD setup)
 * ------------------------------------------------------------------ */

void initKPAD(void)
{
    if (lbl_807947F0 == 0) {
        fn_804C08A0();
        lbl_807947F0 = 1;
    }
}

void shutdownKPAD(void)
{
    if (lbl_807947F0 != 0) {
        fn_804C0D00();
        lbl_807947F0 = 0;
    }
}

void setWpadCallback(void)
{
    s32 chan;
    PlayerPad* pad;

    chan = 0;
    pad = Psw;
    do {
        if (pad->field_0x30 != (u32)-1) {
            WPADSetExtensionCallback(chan, (void*)&fn_80043728);
            WPADSetSamplingCallback(chan, (void*)&fn_800438A4);
        }
        pad++;
        chan++;
    } while (chan < 4);
}


/* `GameModeExec__Fv` - pick the task body for the current game mode. */
void GameModeExec(void)
{
    switch (GameMode_ck()) {
    case 1:
        system_w.field_0x868 = 0;
        Tsk_Change((void*)&fn_8028BF1C, 4);
        return;
    case 2:
        if (system_w.field_0x7d3 == 1) {
            Tsk_Change((void*)&fn_803A13B4, 4);
            return;
        }
        Tsk_Change((void*)&fn_8021F3A8, 4);
        return;
    }
}

extern "C" void fn_80044A30(u8 chan);

/* Motor-off for one channel, plus the two per-channel motor flags. */
extern "C" void fn_80044A30(u8 chan)
{
    system_w.field_0x33[chan] = 0;
    system_w.field_0x37[chan] = 0;
    WPADControlMotor(chan, 0, 0);
}

extern "C" void fn_80044A54(void)
{
    u8 chan;

    chan = 0;
    do {
        fn_80044A30(chan);
        chan++;
    } while (chan < 4);
}

/* Tick the per-channel motor timers. */
extern "C" void fn_80044A90(void)
{
    s32 chan;

    chan = 0;
    do {
        if ((s8)system_w.field_0x33[chan] > 0) {
            system_w.field_0x33[chan] = system_w.field_0x33[chan] - 1;
        }
        if (system_w.field_0x37[chan] == 1 && (s8)system_w.field_0x33[chan] <= 0) {
            fn_80044A30((u8)chan);
        }
        chan++;
    } while (chan < 4);
}

/* Per-channel motor / LED state (fn_80041EA4 in the map). */
extern "C" void fn_80041EA4(u8 chan, s32 on)
{
    if (on != 0) {
        system_w.field_0x7e4[chan] = 1;
        system_w.field_0x7e0[chan] = 0x78;
        if (system_w.field_0x7dc[chan] == 0xFF) {
            system_w.field_0x7dc[chan] = 0;
        }
    } else {
        system_w.field_0x7dc[chan] = pad_chan_state[chan].motor_level_0x14;
    }
}

/* ------------------------------------------------------------------ *
 * 0x80044180  (acceleration clamp)
 * ------------------------------------------------------------------ */

extern "C" f32 clamp_acc(f32 value, f32 limit)
{
    f32 neg;

    if (value < lbl_80795AE8) {
        neg = -limit;
        if (value < neg) {
            return neg;
        }
        return value;
    }
    if (value > limit) {
        return limit;
    }
    return value;
}

/* ------------------------------------------------------------------ *
 * This unit's own `.bss` (`splits.txt` `.bss 0x806585B8..0x806694E8`), in address order.  Defined at the
 * foot of the file, after every use: with the definitions above the bodies MWCC folds the addresses
 * into one section base plus displacements, where the target emits a `lis`/`addi` pair per symbol.
 * ------------------------------------------------------------------ */
u32 rso_slot_load_base[5];             /* +0x806585B8: per-slot RSO load base, cleared by `fn_80040954` */
u32 rso_slot_unused_words[5];          /* +0x806585CC: cleared with `rso_slot_load_base`, read nowhere else */
SystemWork system_w;                   /* +0x806585E0: the game/system state block (`mh3_pad/system_w.h`) */
ScreenWork Screen_w;                   /* +0x8065903C: the screen geometry block (`mh3_pad/Screen_w.h`) */
u8 option_w[0x24];                     /* +0x80659090: the option table (`mh3_pad/option_w.h`) */
LbParamWork lb_param_w;                /* +0x806590B4: the lobby parameter block (`mh3_pad/lb_param_w.h`) */
TaskSlot task_slot_table[16];          /* +0x80659150: the task slot table `Tsk_Change` indexes */
PlayerPad Psw[4];                      /* +0x80659350: the per-player pad records (`mh3_pad/Psw.h`) */
PlayerPad Psw_prev[4];                 /* +0x8065A090: the second pad-record array `fn_80046B94` constructs (GUESS: previous frame) */
PadChanState pad_chan_state[4];        /* +0x8065ADD0: per-channel state records, cleared with `Psw` by `fn_80041AA4` */
u16 pad_btn_table_0[16];               /* +0x8065AE30: copy of the .data button table 0x80580E90 (GUESS name) */
u16 pad_btn_table_1[16];               /* +0x8065AE50: copy of the .data button table 0x80580EB0 (GUESS name) */
u16 pad_btn_table_2[16];               /* +0x8065AE70: copy of the .data button table 0x80580ED0 (GUESS name) */
u8 kpad_work[0x1B8];                   /* +0x8065AE90: KPAD work area `fn_80045048`/`fn_80044D78` use (GUESS name) */
u8 wpad_sampling_buf_fmt8[4][0x1518];  /* +0x8065B048: per-channel 100-sample buffer for data format 8 */
u8 wpad_sampling_buf_fmt5[4][0x1388];  /* +0x806604A8: per-channel 100-sample buffer for data format 5 */
u8 wpad_sampling_buf_fmt2[4][0x1068];  /* +0x806652C8: per-channel 100-sample buffer for data format 2 */
MEMAllocator wpad_mem2_allocator;      /* +0x80669468: allocator over the MEM2 pad heap (align 0x20) */
s16 pointer_hist_x[16];                /* +0x80669478: pointer x history ring, zeroed by `fn_80043BAC` (GUESS) */
s16 pointer_hist_y[16];                /* +0x80669498: pointer y history ring (GUESS) */
f32 pointer_center[3];                 /* +0x806694B8: half of `Screen_w.width_f`/`height_f` (GUESS name) */
f32 pointer_offset[3];                 /* +0x806694C4: zeroed pointer offset (GUESS name) */
