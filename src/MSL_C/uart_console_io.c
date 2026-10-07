/*
 * MSL_C/uart_console_io.c - the console stream callbacks: the console writer (initialises the UART once, writes
 *    through TRK's console write) and the console close stub.
 *
 * RANGE. .text 0x80463CC0..0x80463D98 (2 functions in the map, 0xD8 B); .sbss 0x80794E10..0x80794E18.
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi`.
 * NAMES. file name GUESS (MSL `uart_console_io`); `__write_console` (map row fn_80463CC0) is a GUESS, `__close_console`
 *    (fn_80463D90) is a GUESS, `uart_initialized` (lbl_80794E10) is a GUESS.
 * EVIDENCE. `__files` stores both functions as the write and close callbacks of the three standard streams; the
 *    writer calls `InitializeUART`, `WriteUARTN` and `OSGetConsoleType`; its once-flag is `.sbss` 0x80794E10.
 * RESIDUALS. none known.
 * SHAPES. the UART is used only when the console type has the 0x20000000 bit clear.
 */
#include "MSL_C/uart_console_io.h"
#include "EUART/EUART.h"
#include "OS/OS.h"
#include "TRK/mslsupp.h"

static s32 uart_initialized;

s32 __write_console(s32 handle, u8* buffer, u32* count, void (*idle_proc)(void))
{
    s32 err;

    if ((OSGetConsoleType() & 0x20000000) == 0) {
        err = 0;
        if (uart_initialized == 0) {
            err = InitializeUART(0xE100);
            if (err == 0) {
                uart_initialized = 1;
            }
        }
        if (err != 0) {
            return 1;
        }
        if (WriteUARTN((char*)buffer, *count) != 0) {
            *count = 0;
            return 1;
        }
    }
    __TRK_write_console(handle, buffer, count, idle_proc);
    return 0;
}

s32 __close_console(s32 handle)
{
    return 0;
}
