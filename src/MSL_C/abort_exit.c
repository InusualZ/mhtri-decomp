/*
 * MSL_C/abort_exit.c - the abort path and the runtime-constraint handler word.
 *
 * RANGE. .text 0x80463D98..0x80463DE4 (2 functions in the map, 0x4C B); .sbss 0x80794E18..0x80794E28.
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi`.
 * NAMES. file name GUESS (MSL `abort_exit`); `abort` (map row fn_80463D98) is a GUESS (raise(1), set a flag, exit(1)); the flag
 *    `__aborting` (lbl_80794E18) is a GUESS.
 * EVIDENCE. `.sbss` 0x80794E18 (abort-in-progress flag, written by the first function), 0x80794E1C
 *    (`__stdio_exit`, stored by `__stdio_atexit`) and 0x80794E20 (constraint handler, read by the second
 *    function) are consecutive and follow the console unit's flag.
 * RESIDUALS. COARSE: the constraint handler may be a separate TU.
 * SHAPES. the constraint handler call is a tail call through the handler word.
 */
#include "MSL_C/abort_exit.h"
#include "MSL_C/signal.h"
#include "Runtime.PPCEABI.H/exit.h"

/* untyped: caller-owned pointer */
typedef void (*ConstraintHandler)(const char* message, void* pointer, s32 error);

static ConstraintHandler __msl_constraint_handler;
void (*__stdio_exit)(void);
s32 __aborting;

void abort(void)
{
    raise(1);
    __aborting = 1;
    exit(1);
}

/* untyped: caller-owned pointer */
void __msl_runtime_constraint_violation_s(const char* message, void* pointer, s32 error)
{
    if (__msl_constraint_handler != NULL) {
        __msl_constraint_handler(message, pointer, error);
    }
}
