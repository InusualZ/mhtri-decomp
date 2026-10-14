/*
 * nw4r/db_assert.cpp - nw4r::db's assert reporters: `Panic`, `Warning` and their console/stack-trace helpers.
 * RANGE. .text 0x80500868-0x80500CF8 (6 functions); .data 0x8062F030-0x8062F0B0, .bss 0x80760C48-0x80760C78, .sdata
 *   0x807941E0-0x807941E8, .sbss 0x80795758-0x80795768.  No extab, no .sdata2.
 * RANGE. Left edge 0x80500868: its first function reads the console pointer at .sbss 0x8079575C that only
 *   this range reads (Assertion_Printf_, VPanic, Warning, WarningAlarmFunc_).  0x80500770-0x80500868 before it
 *   (an empty varargs printer, a ring-buffer line count, a text-colour setter that tail-calls 0x805045A0) is
 *   nw4r::db console and ut code by content and sits in `nw4r/db_console.cpp` (unproven seam).
 * RANGE. Right edge 0x80500CF8: the next two functions are nw4r::math's table-driven exp/log (they read
 *   .data 0x8062F0B0/0x8062F1B8 and .sdata2 0x8079D480-0x8079D494), which `nw4r/math_arithmetic.cpp` owns.
 *   Every data piece here is read only from this range.
 * FLAGS. the `nw4r` lib block: GC/3.0a5.2 with `cflags_nw4r` (`cflags_os` + `-fp_contract off`); evidence in docs/nw4r.md.
 * NAMES. `Panic` and `Warning` are the map's manglings; by the nw4r source the others are a GUESS:
 *   `Assertion_Printf_` (fn_80500868), `ShowStack_` (fn_80500900), `VPanic` (fn_805009AC), `WarningAlarmFunc_`
 *   (fn_80500CE0), and the statics `sWarningAlarm`, `sWarningTime`, `sAssertionConsole`, `sDispWarningAuto`,
 *   `sAlarmInitialized`.
 * RESIDUALS. Warning: the scroll keeps the console in r30 and its view height in r31 where retail has r31/r30 (6 bytes);
 *   the other spellings tried swap the warning time and the alarm pointer instead (the time read through a local
 *   is the closer of the two).
 * RESIDUALS. The string pools are `@` symbols in ours where the map names them `lbl_8062F030`, `lbl_8062F090`,
 *   `lbl_8062F0A0` and `lbl_807941E4`.
 * RESIDUALS. The `.sdata`/`.sbss` claims end on alignment pad the object does not emit.
 * SHAPES. ShowStack_ is kept out of line by a `#pragma dont_inline` scoped to its own definition (retail calls it);
 *   VPanic stays out of line by size and must sit outside that scope, or the console scroll is called, not inlined.
 *   `Assertion_ShowConsole` and `GetWarningAlarm_` are nw4r's helpers, `inline` so the object carries no unreferenced
 *   copy; Warning inlines them, which is what loads sWarningTime before the console test.  WarningAlarmFunc_ is
 *   forward-declared and defined last (retail emits it after Warning).  The console scroll and visibility are
 *   nw4r's db_console inlines (GetViewHeight, SetViewBaseLine, ShowLatestLine, SetVisible, an empty VPrintf).
 */

#include "nw4r/db_assert.h"
#include "nw4r/db_console.h"
#include "OS/OSAlarm.h"
#include "NAND/nand.h"
#include "OS/OSSetAlarm.h"
#include "OS/OSVReport.h"
#include "OS/OSThread.h"
#include "OS/PPCHalt.h"
#include "VI/vi.h"
#include "stdarg.h"


namespace nw4r {
namespace db {

static OSAlarm sWarningAlarm;
static u32 sWarningTime;
static ConsoleHandle sAssertionConsole;
static bool sDispWarningAuto = true;
static bool sAlarmInitialized;

/* The number of lines the console window shows. */
static inline u16 Console_GetViewHeight(ConsoleHandle console) {
    return console->viewLines;
}

/* Sets the first line the console window shows. */
static inline void Console_SetViewBaseLine(ConsoleHandle console, s32 line) {
    console->viewTopLine = line;
}

/* Scrolls the console so its latest line is the last one shown. */
static inline s32 Console_ShowLatestLine(ConsoleHandle console) {
    s32 baseLine = Console_GetTotalLines(console) - Console_GetViewHeight(console);

    if (baseLine < 0) {
        baseLine = 0;
    }
    Console_SetViewBaseLine(console, baseLine);
    return baseLine;
}

/* Prints to the OS report when no assert console is attached. */
static void Assertion_Printf_(const char* fmt, ...) {
    va_list vlist;

    va_start(vlist, fmt);
    if (sAssertionConsole == NULL) {
        OSVReport(fmt, vlist);
    }
    va_end(vlist);
}

/* Prints the stack back-chain from `sp`, at most 16 frames. */
#pragma dont_inline on
static void ShowStack_(u32 sp) {
    u32* p;
    u32 i;

    Assertion_Printf_("-------------------------------- TRACE\n");
    Assertion_Printf_("Address:   BackChain   LR save\n");

    p = reinterpret_cast<u32*>(sp);
    for (i = 0; i < 16; i++) {
        if (p == NULL || reinterpret_cast<u32>(p) == 0xFFFFFFFF || !(reinterpret_cast<u32>(p) & 0x80000000)) {
            break;
        }
        Assertion_Printf_("%08X:  %08X    %08X ", p, p[0], p[1]);
        Assertion_Printf_("\n");
        p = reinterpret_cast<u32*>(*p);
    }
}
#pragma dont_inline reset

/* Stops the system and reports the failure with a stack trace; halts when `halt` is set. */
static void VPanic(const char* file, int line, const char* fmt, va_list vlist, bool halt) {
    register u32 stackPointer;

    asm { lwz stackPointer, 0(r1) }

    OSDisableInterrupts();
    OSDisableScheduler();
    VISetPreRetraceCallback(NULL);
    VISetPostRetraceCallback(NULL);
    ShowStack_(stackPointer);

    if (sAssertionConsole) {
        Console_Printf(sAssertionConsole, "%s:%d Panic:", file, line);
        Console_Printf(sAssertionConsole, "\n");
        Console_ShowLatestLine(sAssertionConsole);
        sAssertionConsole->isVisible = true;
    } else {
        OSReport("%s:%d Panic:", file, line);
        OSVReport(fmt, vlist);
        OSReport("\n");
    }

    if (halt) {
        PPCHalt();
    }
}

/* 0x80500AB0 (0x94): reports an assert failure and halts. */
void Panic(const char* file, int line, const char* fmt, ...) {
    va_list vlist;

    va_start(vlist, fmt);
    VPanic(file, line, fmt, vlist, true);
    va_end(vlist);
    PPCHalt();
}

static void WarningAlarmFunc_(OSAlarm*, OSContext*);

/* Prints a va_list to the console (the retail build compiles the body away). */
static inline void Console_VPrintf(ConsoleHandle, const char*, va_list) {}

/* Shows or hides the console window. */
static inline void Console_SetVisible(ConsoleHandle console, bool visible) {
    console->isVisible = visible;
}

/* The warning alarm, created on first use. */
static inline OSAlarm* GetWarningAlarm_(void) {
    if (!sAlarmInitialized) {
        OSCreateAlarm(&sWarningAlarm);
        sAlarmInitialized = true;
    }
    return &sWarningAlarm;
}

/* Shows the assert console for `time` ticks. */
inline void Assertion_ShowConsole(u32 time) {
    if (sAssertionConsole) {
        OSAlarm* alarm = GetWarningAlarm_();

        OSCancelAlarm(alarm);
        Console_SetVisible(sAssertionConsole, true);
        if (time != 0) {
            OSSetAlarm(alarm, time, WarningAlarmFunc_);
        }
    }
}

/* 0x80500B44 (0x19C): reports a warning and keeps running; the console shows it for the warning time. */
void Warning(const char* file, int line, const char* fmt, ...) {
    va_list vlist;

    va_start(vlist, fmt);
    if (sAssertionConsole) {
        Console_Printf(sAssertionConsole, "%s:%d Warning:", file, line);
        Console_VPrintf(sAssertionConsole, fmt, vlist);
        Console_Printf(sAssertionConsole, "\n");
        Console_ShowLatestLine(sAssertionConsole);
        if (sDispWarningAuto) {
            u32 time = sWarningTime;
            Assertion_ShowConsole(time);
        }
    } else {
        OSReport("%s:%d Warning:", file, line);
        OSVReport(fmt, vlist);
        OSReport("\n");
    }
    va_end(vlist);
}

/* 0x80500CE0 (0x18): hides the assert console when the warning display time runs out. */
static void WarningAlarmFunc_(OSAlarm*, OSContext*) {
    if (sAssertionConsole) {
        Console_SetVisible(sAssertionConsole, false);
    }
}

}  // namespace db
}  // namespace nw4r
