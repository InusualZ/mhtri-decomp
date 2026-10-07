/*
 * EUART/EUART.c - the SDK EXI UART debug output (`EUARTInit`, `InitializeUART`, `WriteUARTN`).
 *
 * RANGE. `.text` 0x804AFB50-0x804AFED0 (3 functions / 0x374 B); `.sbss` 0x80795188-0x80795198; `.sdata2`
 *   0x8079D0E8-0x8079D0F0.
 *   - four `.sbss` statics 0x80795188..0x80795198 and `.sdata2` 0x8079D0E8 are read only by these three functions;
 *     `WriteUARTN` calls into the EXI bus driver that follows
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`) with `cflags_base` (-O4,p, default alignment 16).
 * NAMES. the unit name is a GUESS (the library's first source file); the four statics are GUESSes from their use
 *   (`UARTInitialized`, `UARTError`, `UARTReserved`, `UARTEnabled`) and `UARTConstants` is the 8-byte `.sdata2`
 *   pair `{EXI_FREQ_16MHZ, 0}` (only word 0 is read).
 * RESIDUALS. none recorded yet.
 */

#include "types.h"
#include "EUART/EUART.h"
#include "EXI/EXIBios.h"
#include "EXI/ProbeBarnacle.h"
#include "OS/OS.h"
#include "OS/OSInterrupt.h"

#define UART_ENABLED_MAGIC 0xA5FF005A

/* `.sbss` is laid out in reverse definition order. */
static u32 UARTEnabled;
static u32 UARTReserved;
static u32 UARTError;
static u32 UARTInitialized;

static const u32 UARTConstants[2] = {EXI_FREQ_16MHZ, 0};

/* Probes the console type and sets the UART up on EXI channel 0, device 1. */
BOOL EUARTInit(void)
{
    BOOL enabled;
    u8 data;

    if (UARTInitialized) {
        return TRUE;
    }

    if (!(OSGetConsoleType() & 0x10000000)) {
        UARTError = 2;
        return FALSE;
    }

    enabled = OSDisableInterrupts();

    data = 0xF2;
    if (!EXIWriteReg(EXI_CHAN_0, EXI_DEV_INT, 0xB0000000, &data, 1)) {
        UARTError = 5;
        OSRestoreInterrupts(enabled);
        return FALSE;
    }

    data = 0xF3;
    if (!EXIWriteReg(EXI_CHAN_0, EXI_DEV_INT, 0xB0000000, &data, 1)) {
        UARTError = 5;
        OSRestoreInterrupts(enabled);
        return FALSE;
    }

    OSRestoreInterrupts(enabled);
    UARTInitialized = TRUE;
    UARTError = 0;
    UARTReserved = 0;
    return TRUE;
}

/* Enables UART output when the console is a development unit. */
s32 InitializeUART(u32 baud_rate)
{
    if (!(OSGetConsoleType() & 0x10000000)) {
        UARTEnabled = 0;
        return 2;
    }

    UARTEnabled = UART_ENABLED_MAGIC;
    return 0;
}

/* Writes a byte string to the UART through the EXI FIFO, translating line feeds to carriage returns. */
s32 WriteUARTN(char* buf, u32 len)
{
    u32 freq;
    s32 qLen;
    u32 cmd;
    u32 txData;
    s32 error;
    u32 status;
    u32 pollCmd;
    char* ptr;

    if (UARTEnabled != UART_ENABLED_MAGIC) {
        return 2;
    }

    if (!UARTInitialized && !EUARTInit()) {
        return 2;
    }

    if (!UARTInitialized) {
        UARTError = 1;
        return 2;
    }

    if (!EXILock(EXI_CHAN_0, EXI_DEV_INT, NULL)) {
        return 0;
    }

    for (ptr = buf; (u32)(ptr - buf) < len; ptr++) {
        if (*ptr == '\n') {
            *ptr = '\r';
        }
    }

    freq = UARTConstants[0];
    cmd = 0xB0000100;
    error = 0;

    while (len) {
        if (!EXISelect(EXI_CHAN_0, EXI_DEV_INT, freq)) {
            qLen = -1;
        } else {
            pollCmd = 0x30000100;
            EXIImm(EXI_CHAN_0, &pollCmd, 4, EXI_WRITE, NULL);
            EXISync(EXI_CHAN_0);
            EXIImm(EXI_CHAN_0, &status, 4, EXI_READ, NULL);
            EXISync(EXI_CHAN_0);
            EXIDeselect(EXI_CHAN_0);
            qLen = 32 - ((status >> 24) & 0x3F);
        }

        if (qLen < 0) {
            error = 3;
            break;
        }

        if (qLen == 32) {
            if (!EXISelect(EXI_CHAN_0, EXI_DEV_INT, freq)) {
                error = 3;
                break;
            }
            EXIImm(EXI_CHAN_0, &cmd, 4, EXI_WRITE, NULL);
            EXISync(EXI_CHAN_0);

            while (qLen > 0 && len) {
                txData = *buf << 24;
                EXIImm(EXI_CHAN_0, &txData, 4, EXI_WRITE, NULL);
                EXISync(EXI_CHAN_0);
                buf++;
                qLen--;
                len--;
            }
            EXIDeselect(EXI_CHAN_0);
        }
    }

    EXIUnlock(EXI_CHAN_0);
    return error;
}
