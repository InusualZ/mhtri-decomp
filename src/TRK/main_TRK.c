/*
 * TRK/main_TRK.c - the MetroTRK entry: `TRK_main` and `TRKNubMainLoop`.
 *
 * RANGE. .text 0x804688A0..0x804689C8 (2 functions in the map, 0x128 B); .sbss 0x80794E40..0x80794E48.
 * FLAGS. the `OS` lib's `cflags_os`.
 * NAMES. file name GUESS (MetroTRK `main_TRK`); the loop's locals (`shutdown_requested`, `input_polled`) are GUESS.
 * EVIDENCE. `.sbss` 0x80794E40 (`TRK_mainError`) is read only by `TRK_main`; the loop reads `gTRKInputPendingPtr`.
 * RESIDUALS. none known.
 * SHAPES. the loop pumps events until a shutdown event arrives; with no event it polls the serial input, and resumes
 *    the target when nothing is pending and the target is not stopped.
 */
#include "TRK/main_TRK.h"
#include "TRK/dispatch.h"
#include "TRK/msgbuf.h"
#include "TRK/nubevent.h"
#include "TRK/nubinit.h"
#include "TRK/serpoll.h"
#include "TRK/targcont.h"
#include "TRK/targimpl.h"

static s32 TRK_mainError;

s32 TRK_main(void)
{
    TRK_mainError = TRKInitializeNub();
    if (TRK_mainError == 0) {
        TRKNubWelcome();
        TRKNubMainLoop();
    }
    TRK_mainError = TRKTerminateNub();
    return TRK_mainError;
}

void TRKNubMainLoop(void)
{
    TRKEvent event;
    s32 shutdown_requested = 0;
    s32 input_polled = 0;

    while (!shutdown_requested) {
        if (TRKGetNextEvent(&event)) {
            input_polled = 0;
            switch (event.type) {
            case TRK_EVENT_REQUEST:
                TRKDispatchMessage(TRKGetBuffer(event.buffer_id));
                break;
            case TRK_EVENT_SHUTDOWN:
                shutdown_requested = 1;
                break;
            case TRK_EVENT_BREAKPOINT:
            case TRK_EVENT_EXCEPTION:
                TRKTargetInterrupt(&event);
                break;
            case TRK_EVENT_SUPPORT_REQUEST:
                TRKTargetSupportRequest();
                break;
            }
            TRKDestructEvent(&event);
        } else if (!input_polled || *gTRKInputPendingPtr != 0) {
            input_polled = 1;
            TRKGetInput();
        } else {
            if (!TRKTargetStopped()) {
                TRKTargetContinue();
            }
            input_polled = 0;
        }
    }
}
