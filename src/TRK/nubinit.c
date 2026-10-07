/*
 * TRK/nubinit.c - the MetroTRK nub initialisation: `TRKInitializeNub`, `TRKTerminateNub`, `TRKNubWelcome`,
 *    `TRKInitializeEndian`.
 *
 * RANGE. .text 0x80469638..0x80469788 (4 functions in the map, 0x150 B); .data 0x8060F6F0..0x8060F710; .sbss
 *    0x80794E58..0x80794E68.
 * FLAGS. the `OS` lib's `cflags_os`.
 * NAMES. file name GUESS (MetroTRK `nubinit`); `gTRKByteOrder` and `TRKByteOrder` are GUESS (the 8 B object
 *    `TRKInitializeEndian` writes and the buffer readers test); `TRKTerminateSerialHandler` (map row renamed from
 *    fn_804698C8) is GUESS (the 8 B partner of `TRKInitializeSerialHandler`, called from `TRKTerminateNub`).
 * EVIDENCE. `.data` 0x8060F6F0 (`MetroTRK for Revolution v0.4`) is read only by `TRKNubWelcome`;
 *    `TRKInitializeEndian` writes `.sbss` 0x80794E58 and `TRKInitializeNub` touches 0x80794E60.
 * RESIDUALS. none known.
 * SHAPES. each init step runs only while the previous returned 0; the endian probe reads four bytes as one word.
 */
#include "TRK/nubinit.h"
#include "TRK/dolphin_trk.h"
#include "TRK/msgbuf.h"
#include "TRK/nubevent.h"
#include "TRK/serpoll.h"
#include "TRK/targimpl.h"

u8* gTRKInputPendingPtr;
TRKByteOrder gTRKByteOrder;

s32 TRKInitializeNub(void)
{
    s32 err = TRKInitializeEndian();
    s32 uart_err;

    if (err == 0) {
        err = TRKInitializeEventQueue();
    }
    if (err == 0) {
        err = TRKInitializeMessageBuffers();
    }
    InitializeProgramEndTrap();
    if (err == 0) {
        err = TRKInitializeSerialHandler();
    }
    if (err == 0) {
        err = TRKInitializeTarget();
    }
    if (err == 0) {
        uart_err = TRKInitializeIntDrivenUART(1, 0, &gTRKInputPendingPtr);
        TRKTargetSetInputPendingPtr(gTRKInputPendingPtr);
        if (uart_err != 0) {
            err = uart_err;
        }
    }
    return err;
}

s32 TRKTerminateNub(void)
{
    TRKTerminateSerialHandler();
    return 0;
}

void TRKNubWelcome(void)
{
    TRK_board_display("MetroTRK for Revolution v0.4");
}

s32 TRKInitializeEndian(void)
{
    u8 probe[4];
    s32 err = 0;

    gTRKByteOrder.big_endian = 1;
    probe[0] = 0x12;
    probe[1] = 0x34;
    probe[2] = 0x56;
    probe[3] = 0x78;
    if (*(u32*)probe == 0x12345678) {
        gTRKByteOrder.big_endian = 1;
    } else if (*(u32*)probe == 0x78563412) {
        gTRKByteOrder.big_endian = 0;
    } else {
        err = 1;
    }
    return err;
}
