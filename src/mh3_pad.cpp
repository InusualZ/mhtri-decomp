/*
 * rule 7 deferred: the symbol map has only fn_XXXXXXXX for the unnamed functions in this range
 * (checked with `python tools/symbols/dumpmap.py lookup <addr>`: every unnamed entry in this range is a
 * bare `zz_<addr>_` placeholder in the runtime dump as well, so there is no real name to recover).  The
 * functions the map does name (`Tsk_Change`, `GameModeExec`, `cnvt_eur_fname`, `initKPAD`, ...) use
 * their real C++ signatures.
 *
 * `mh3_pad.cpp` - the game-root pad / mode file.  `.text` 0x800408A8-0x80047398 (145 functions).
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
 * this unit's first extab record starts at 0x80006820).  The right edge 0x80047398 is NOT a
 * translation-unit boundary - it is where the pooled proposal was capped (`seam_note: capped at
 * --max-bytes, seam is a guess`); `fn_80047398` (0x5C B, own extab at 0x80006A90) continues the same
 * function stream.  This unit claims only the proposal's 145 functions.
 *
 * Sections: .text 0x800408A8-0x80047398, extab 0x80006820-0x80006A90,
 * extabindex 0x8001E5A0-0x8001E948, and the .ctors word 0x8056F2C4-0x8056F2C8 (dtk assigned it to
 * this unit on the split; the first C++ static constructor of the file).
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
 * config_requests: the `include/unsplit/unknown.h` field additions (field_0x20/0x21, field_0x33[4]/
 * field_0x37[4], field_0x7dc[4], field_0x868 - all previously `pad_*`), the `RSOModule` hoist out of
 * `src/RSO/runtime.c`, and the unsplit externs this unit needs.  See the outbox.
 */

#include "types.h"
#include "gx.h"
#include "nw4r/math.h"
#include "unsplit/unknown.h" /* SystemWork / system_w (rule 1/2) */
#include "RSO/runtime.h"     /* RSOModule + RSOStaticLocateObject (rule 2) */
#include "fn_80040598.h"     /* the game-root RSO loaders (rule 2) */
#include "ef/fn_800CDB2C.h"  /* fn_800CF208 / fn_800CEE2C (rule 2) */
#include "Runtime.PPCEABI.H/memset.h"
#include "unsplit/g3d.h"     /* fn_8007A510 (rule 2) */

/* ------------------------------------------------------------------ *
 * Local types
 * ------------------------------------------------------------------ */

/* The 0x20-byte task slot table at `lbl_80659150` (Tsk_Change / fn_800417F0 / fn_8004192C index it
 * by `slot << 5`, fn_80041694 walks all 16). */
typedef struct TaskSlot {
    /* +0x00 */ s16 state;
    /* +0x02 */ s16 timer;
    /* +0x04 */ void (*func)(struct TaskSlot*);
    /* +0x08 */ u8 pad_0x08[0x18];
} TaskSlot; /* size: 0x20 */

/* The per-player pad record at `Psw` (0x350 B stride; only the fields this unit reads are named -
 * the rest is padding until another unit needs it). */
typedef struct PlayerPad {
    /* +0x000 */ u8 pad_0x000[0x30];
    /* +0x030 */ u32 field_0x30;
    /* +0x034 */ u32 mode;
    /* +0x038 */ u8 pad_0x038[0x96];
    /* +0x0CE */ u16 field_0xce;
    /* +0x0D0 */ u8 pad_0x0d0[0x2];
    /* +0x0D2 */ u16 field_0xd2;
    /* +0x0D4 */ u8 pad_0x0d4[0x6];
    /* +0x0DA */ u16 field_0xda;
    /* +0x0DC */ u8 pad_0x0dc[0x1C];
    /* +0x0F8 */ u16 field_0xf8;
    /* +0x0FA */ u8 pad_0x0fa[0x6];
    /* +0x100 */ u16 field_0x100;
    /* +0x102 */ u8 pad_0x102[0x1C];
    /* +0x11E */ u16 field_0x11e;
    /* +0x120 */ u8 pad_0x120[0x215];
    /* +0x335 */ u8 field_0x335;
    /* +0x336 */ u8 pad_0x336[0x1A];
} PlayerPad; /* size: 0x350 */

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
extern u32 lbl_806585B8[];
extern u32 lbl_806585CC[];
extern TaskSlot lbl_80659150[];
extern PlayerPad Psw[];
extern u8 Screen_w[];
extern u8 lbl_80669468[]; /* MEM2 allocator record */

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
extern void fn_804463C4(void);
extern void fn_8028BF1C(void);
extern void fn_803A13B4(void);
extern void fn_8021F3A8(void);
extern u8 lbl_8065ADD0[];
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
    lbl_806585B8[0] = 0;
    lbl_806585CC[0] = 0;
    lbl_806585B8[1] = 0;
    lbl_806585CC[1] = 0;
    lbl_806585B8[2] = 0;
    lbl_806585CC[2] = 0;
    lbl_806585B8[3] = 0;
    lbl_806585CC[3] = 0;
    lbl_806585B8[4] = 0;
    lbl_806585CC[4] = 0;
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
        pools = lbl_806585B8;
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
        lbl_806585B8[lbl_80790E28[0]] = lbl_807947C0;
        lbl_807947D8 = (RSOModule*)fn_80040598(lbl_8057C9D8[0], (void*)lbl_806585B8[lbl_80790E28[0]],
                                                lbl_80790E28[0]);
        lbl_806585B8[lbl_80790E28[2]] = lbl_807947C0;
        lbl_807947DC = (RSOModule*)fn_80040598(lbl_8057C9D8[2], (void*)lbl_806585B8[lbl_80790E28[2]],
                                                lbl_80790E28[2]);
        lbl_806585B8[lbl_80790E28[3]] = lbl_807947C0;
        lbl_807947E0 = (RSOModule*)fn_80040598(lbl_8057C9D8[3], (void*)lbl_806585B8[lbl_80790E28[3]],
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
        lbl_806585B8[lbl_80790E28[0]] = lbl_807947C0;
        lbl_807947D8 = (RSOModule*)fn_80040598(lbl_8057C9D8[0], (void*)lbl_806585B8[lbl_80790E28[0]],
                                                lbl_80790E28[0]);
        lbl_806585B8[lbl_80790E28[1]] = lbl_807947C0;
        lbl_807947DC = (RSOModule*)fn_80040598(lbl_8057C9D8[1], (void*)lbl_806585B8[lbl_80790E28[1]],
                                                lbl_80790E28[1]);
        lbl_806585B8[lbl_80790E28[3]] = lbl_807947C0;
        lbl_807947E0 = (RSOModule*)fn_80040598(lbl_8057C9D8[4], (void*)lbl_806585B8[lbl_80790E28[3]],
                                                lbl_80790E28[3]);
        lbl_806585B8[3] = lbl_807947C0;
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
        lbl_806585B8[lbl_80790E28[4]] = lbl_807947C0;
        lbl_807947E4 = (RSOModule*)fn_80040598(lbl_8057C9D8[sel], (void*)lbl_806585B8[lbl_80790E28[4]],
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
        lbl_806585B8[lbl_80790E28[0]] = lbl_807947C0;
        lbl_807947D8 = (RSOModule*)fn_80040598(lbl_8057C9D8[0], (void*)lbl_806585B8[lbl_80790E28[0]],
                                                lbl_80790E28[0]);
        lbl_806585B8[lbl_80790E28[1]] = lbl_807947C0;
        lbl_807947DC = (RSOModule*)fn_80040598(lbl_8057C9D8[1], (void*)lbl_806585B8[lbl_80790E28[1]],
                                                lbl_80790E28[1]);
        lbl_806585B8[lbl_80790E28[5]] = lbl_807947C0;
        lbl_807947E0 = (RSOModule*)fn_80040598(lbl_8057C9D8[0x13], (void*)lbl_806585B8[lbl_80790E28[5]],
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
    memset(lbl_80659150, 0, 0x200);
}

/* Start a task slot running `func` (state 0xC). */
extern "C" void fn_800417F0(void* func, s16 slot)
{
    TaskSlot* task;

    task = &lbl_80659150[slot];
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
    lbl_80659150[slot].state = 0x10;
}

extern "C" void fn_80041878(s16 slot)
{
    lbl_80659150[slot].state = 1;
}

extern "C" void fn_80041894(s16 slot)
{
    lbl_80659150[slot].state = 2;
}

extern "C" void fn_800418B0(s16 slot)
{
    lbl_80659150[slot].state = 0;
}

/* Start a task slot with a fresh body (state 8).  `Tsk_Change__FPvs`: (void*, short). */
void Tsk_Change(void* func, s16 slot)
{
    TaskSlot* task;

    task = &lbl_80659150[slot];
    memset(task, 0, 0x20);
    task->state = 8;
    task->func = (void (*)(TaskSlot*))func;
}

extern "C" TaskSlot* fn_8004192C(s16 slot)
{
    return &lbl_80659150[slot];
}

/* The arena-select task entry: load the four-overlay mode and hand slot 4 to fn_804463C4. */
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
    Tsk_Change((void*)&fn_804463C4, 4);
}

extern "C" void fn_80041A64(void* block)
{
    MEMAllocFromAllocator(lbl_80669468, (u32)block);
}

extern "C" s32 fn_80041A74(void* block)
{
    MEMFreeToAllocator(lbl_80669468, block);
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

extern "C" void fn_80041E70(void* dst, void* src)
{
    ((f32*)dst)[0] = ((f32*)src)[0];
    ((f32*)dst)[1] = ((f32*)src)[1];
    ((f32*)dst)[2] = ((f32*)src)[2];
}

extern "C" void* copyVec3(void* dst, const void* src)
{
    fn_80041E70(dst, (void*)src);
    return dst;
}

/* The return is the first argument, as the callers use it (`mr r4,r3` after the `bl`); the body
 * leaves r3 alone, so `return out;` costs no instruction. */
extern "C" void* setVec3(void* out, f32 x, f32 y, f32 z)
{
    ((f32*)out)[0] = x;
    ((f32*)out)[1] = y;
    ((f32*)out)[2] = z;
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
extern "C" void VEC3_ctor(void* out)
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

/* ------------------------------------------------------------------ *
 * 0x80046EE4 - 0x800470B4  (system flags)
 * ------------------------------------------------------------------ */

extern "C" void fn_80046EE4(void)
{
    system_w.field_0x30 = 1;
}

extern "C" void fn_80046EF8(void)
{
    system_w.field_0x30 = 0;
}

extern "C" s32 fn_80046F0C(void)
{
    return system_w.field_0x30 != 0;
}

extern "C" s32 fn_80047058(void)
{
    return Screen_w[0x1A] != 0;
}

extern "C" s8 fn_800470B4(void)
{
    if (Screen_w[0x1A] != 0) {
        return (s8)Screen_w[0x1B];
    }
    return -1;
}

extern "C" u8 fn_800472DC(s32 arg0)
{
    return (u8)(fn_800470B4() * 2 + arg0);
}

extern "C" void fn_80046E98(void)
{
    if (system_w.field_0x21 != 0) {
        fn_8007A510();
    }
}

/* `setSoftresetFlag__Fb` */
void setSoftresetFlag(bool flag)
{
    if (flag) {
        system_w.field_0x20 = 0;
    } else {
        system_w.field_0x20 = 1;
    }
}

/* `GameModeExec__Fv` - pick the task body for the current game mode. */
void GameModeExec(void)
{
    switch (fn_800CF208()) {
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
        system_w.field_0x7dc[chan] = lbl_8065ADD0[chan * 0x18 + 0x14];
    }
}

/* ------------------------------------------------------------------ *
 * 0x80047234 - 0x8004726C  (32-bit word copy)
 * ------------------------------------------------------------------ */

extern "C" s32 fn_80047234(s32* src)
{
    return *src;
}

extern "C" void fn_8004726C(s32* dst, s32* src)
{
    *dst = *src;
}

extern "C" void* fn_8004723C(void* dst, const void* src)
{
    fn_8004726C((s32*)dst, (s32*)src);
    return dst;
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
