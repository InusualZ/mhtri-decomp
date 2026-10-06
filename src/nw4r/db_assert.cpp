/*
 * nw4r/db_assert.cpp - nw4r::db's assert reporters: `Panic`, `Warning` and their console/stack-trace helpers.
 * RANGE. .text 0x80500868-0x80500CF8 (6 functions); .data 0x8062F030-0x8062F0B0, .bss 0x80760C48-0x80760C78, .sdata
 *   0x807941E0-0x807941E8, .sbss 0x80795758-0x80795768.  No extab, no .sdata2.
 * RANGE. Left edge 0x80500868: its first function reads the console pointer at .sbss 0x8079575C that only
 *   this range reads (Assertion_Printf_, VPanic, Warning, WarningAlarmFunc_).  0x80500770-0x80500868 before it
 *   (an empty varargs printer, a ring-buffer line count, a text-colour setter that tail-calls 0x805045A0) is
 *   nw4r::db console and ut code by content and stays in `WPAD/wpad.cpp` (unproven seam).
 * RANGE. Right edge 0x80500CF8: the next two functions are nw4r::math's table-driven exp/log (they read
 *   .data 0x8062F0B0/0x8062F1B8 and .sdata2 0x8079D480-0x8079D494), which `nw4r/math_arithmetic.cpp` owns.
 *   Every data piece here is read only from this range.
 * FLAGS. the `nw4r` lib block: GC/3.0a5.2 with `cflags_nw4r` (`cflags_os` + `-fp_contract off`); evidence in docs/nw4r.md.
 * NAMES. `Panic` and `Warning` are the map's manglings; by the nw4r source the others are a GUESS:
 *   `Assertion_Printf_` (fn_80500868), `ShowStack_` (fn_80500900), `VPanic` (fn_805009AC), `WarningAlarmFunc_`
 *   (fn_80500CE0), and the statics `sWarningAlarm`, `sWarningTime`, `sAssertionConsole`, `sDispWarningAuto`,
 *   `sAlarmInitialized`.
 * RESIDUALS. Left out of the bodies until their names are settled: VPanic's two calls 0x804E70C0(NULL) and
 *   0x804E7110(NULL) (filed as the VI retrace-callback setters; the runtime dump calls 0x804E7110
 *   AIRegisterDMACallback) and, in VPanic and Warning, the console print 0x80500770 (console, "%s:%d Panic:" /
 *   "%s:%d Warning:", file, line), 0x80500770 (console, "\n") and the scroll to the latest line through the line count
 *   0x805007D8 (console->viewTopLine = max(0, count - console->viewLines)); all four sit in `WPAD/wpad.cpp`.  With
 *   them written, VPanic and Panic were 100 % and Warning 99.3 % (r30/r31 swap in the scroll).
 * RESIDUALS. The `.sdata`/`.sbss` claims end on alignment pad the object does not emit.
 * SHAPES. ShowStack_ and VPanic are kept out of line by a scoped `#pragma dont_inline` (retail calls both).
 *   `Assertion_ShowConsole` and `GetWarningAlarm_` are nw4r's helpers: the first is unreferenced (the link drops it)
 *   and inlined into Warning, which is what loads sWarningTime before the console test and keeps &sWarningAlarm in r31.
 */

#include "nw4r/db_assert.h"
#include "nw4r/db_console.h"
#include "OS/OSAlarm.h"
#include "NAND/nand.h"
#include "NAND/OSSetAlarm.h"
#include "NAND/OSVReport.h"
#include "OS/PPCHalt.h"
#include "stdarg.h"


namespace nw4r {
namespace db {

static OSAlarm sWarningAlarm;
static u32 sWarningTime;
static ConsoleHandle sAssertionConsole;
static bool sDispWarningAuto = true;
static bool sAlarmInitialized;

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



/* Stops the system and reports the failure with a stack trace; halts when `halt` is set. */
static void VPanic(const char* file, int line, const char* fmt, va_list vlist, bool halt) {
    register u32 stackPointer;

    asm { lwz stackPointer, 0(r1) }

    OSDisableInterrupts();
    OSDisableScheduler();
    ShowStack_(stackPointer);

    if (sAssertionConsole) {
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
#pragma dont_inline reset

/* 0x80500AB0 (0x94): reports an assert failure and halts. */
void Panic(const char* file, int line, const char* fmt, ...) {
    va_list vlist;

    va_start(vlist, fmt);
    VPanic(file, line, fmt, vlist, true);
    va_end(vlist);
    PPCHalt();
}

/* Hides the assert console when the warning display time runs out. */
static void WarningAlarmFunc_(OSAlarm*, OSContext*) {
    if (sAssertionConsole) {
        sAssertionConsole->isVisible = false;
    }
}

/* The warning alarm, created on first use. */
static inline OSAlarm* GetWarningAlarm_(void) {
    if (!sAlarmInitialized) {
        OSCreateAlarm(&sWarningAlarm);
        sAlarmInitialized = true;
    }
    return &sWarningAlarm;
}

/* Shows the assert console for `time` ticks; nothing calls it out of line, so the link drops that copy. */
void Assertion_ShowConsole(u32 time) {
    if (sAssertionConsole) {
        OSAlarm* alarm = GetWarningAlarm_();

        OSCancelAlarm(alarm);
        sAssertionConsole->isVisible = true;
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
        if (sDispWarningAuto) {
            Assertion_ShowConsole(sWarningTime);
        }
    } else {
        OSReport("%s:%d Warning:", file, line);
        OSVReport(fmt, vlist);
        OSReport("\n");
    }
    va_end(vlist);
}

}  // namespace db
}  // namespace nw4r
