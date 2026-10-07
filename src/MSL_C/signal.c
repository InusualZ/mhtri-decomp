/*
 * MSL_C/signal.c - `raise`: the signal handler table dispatch.
 *
 * RANGE. .text 0x8045F4AC..0x8045F554 (1 functions in the map, 0xA8 B); .bss 0x806F5000..0x806F5020.
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi`.
 * NAMES. file name GUESS (MSL `signal`); `raise` (map row fn_8045F4AC) is a GUESS (the dispatch of a seven-entry handler
 *    table that `abort` calls); `signal_handlers` (map row lbl_806F5000) is a GUESS.
 * EVIDENCE. `.bss` 0x806F5000 (0x20 B, 7 handler slots) is read only by it, and it calls `exit` for an unhandled
 *    signal; the abort unit calls it.
 * RESIDUALS. none known.
 * SHAPES. handler value 1 means ignore, 0 means the default action (exit, except for signal 1 which returns).
 */
#include "MSL_C/signal.h"
#include "Runtime.PPCEABI.H/exit.h"

#define SIGNAL_COUNT 7
#define SIG_DFL 0
#define SIG_IGN 1

typedef void (*SignalHandler)(s32);

SignalHandler signal_handlers[SIGNAL_COUNT];

s32 raise(s32 signal)
{
    SignalHandler handler;
    u32 index = signal - 1;

    if (index > SIGNAL_COUNT - 1) {
        return -1;
    }
    handler = signal_handlers[index];
    if (handler != (SignalHandler)SIG_IGN) {
        signal_handlers[index] = (SignalHandler)SIG_DFL;
    }
    if (handler == (SignalHandler)SIG_IGN || (handler == (SignalHandler)SIG_DFL && signal == 1)) {
        return 0;
    }
    if (handler == (SignalHandler)SIG_DFL) {
        exit(0);
    }
    handler(signal);
    return 0;
}
