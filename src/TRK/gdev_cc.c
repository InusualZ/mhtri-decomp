/*
 * TRK/gdev_cc.c - the MetroTRK debugger channel over the EXI/DB interface: `gdev_cc_initialize`, `shutdown`,
 *    `open`, `close`, `read`, `write`, `pre_continue`, `post_stop`, `peek`, `initinterrupts`.
 *
 * RANGE. .text 0x80468378..0x804685F0 (10 functions in the map, 0x278 B); .bss 0x806F5020..0x806F5540; .sbss
 *    0x80794E38..0x80794E40.
 * FLAGS. the `OS` lib's `cflags_os`.
 * NAMES. the `gdev_cc_*` function names are the dump's where it labels the address; `gdev_cc_close`, `gdev_cc_read`,
 *    `gdev_cc_pre_continue`, `gdev_cc_post_stop`, `gdev_cc_peek` and `gdev_cc_initinterrupts` are GUESS (the dump labels the
 *    address with a neighbour or a placeholder; the names fit the unit scheme). `gdev_cc_buffer`, `gdev_cc_circle_buffer`,
 *    `gdev_cc_open_state` and `GdevCcOpenState` are GUESS (the bytes the bodies fill, queue through and test); the return
 *    codes are written as numbers.
 * EVIDENCE. ten consecutive `gdev_cc_*` dump names; `.bss` 0x806F5020 (0x500 B) and 0x806F5520 (0x20 B, the
 *    receive circle buffer) and `.sbss` 0x80794E38 (the 8 B open-state object) are read only here. `DBInitComm` .. `DBInitInterrupts` and the two
 *    4-byte stubs `pre_continue`/`post_stop` call belong to `TRK/exi2_comm.c`.
 * RESIDUALS. none: 10/10 rows byte-identical, relocations 28/28, `.bss` and `.sbss` layouts equal the target's, and the
 *    trial flip links to the retail DOL hash.
 * SHAPES. `read` and `peek` stage the DB bytes in a 0x500 B stack array, then queue them in the circle buffer. `.bss`:
 *    `gdev_cc_buffer` carries a zero initialiser so it is a real definition emitted ahead of the tentative
 *    `gdev_cc_circle_buffer` (playbook idea 158); a tentative object lands at its first code reference, which is the circle
 *    buffer in `gdev_cc_initialize`, so definition order alone put the 0x20 B object first.
 */
#include "TRK/CircleBuffer.h"
#include "TRK/DBInitComm.h"
#include "TRK/DBQueryData.h"
#include "TRK/DBRead.h"
#include "TRK/DBWrite.h"
#include "TRK/DBInitInterrupts.h"
#include "TRK/EXI2_Reserve.h"
#include "TRK/EXI2_Unreserve.h"
#pragma use_lmw_stmw on


#define GDEV_CC_BUFFER_SIZE 0x500

typedef struct GdevCcOpenState {
    /* +0x00 */ s32 is_open;       /* nonzero once gdev_cc_open ran */
    /* +0x04 */ s32 unused_0x04;   /* never read or written */
} GdevCcOpenState; /* size: 0x8 */

u8 gdev_cc_buffer[GDEV_CC_BUFFER_SIZE] = {0};
CircleBuffer gdev_cc_circle_buffer;
GdevCcOpenState gdev_cc_open_state;

s32 gdev_cc_initialize(u32* inputPendingPtrRef, void (*handler)(void))
{
    DBInitComm(inputPendingPtrRef, handler);
    CircleBufferInitialize(&gdev_cc_circle_buffer, gdev_cc_buffer, GDEV_CC_BUFFER_SIZE);
    return 0;
}

s32 gdev_cc_shutdown(void)
{
    return 0;
}

s32 gdev_cc_open(void)
{
    if (gdev_cc_open_state.is_open != 0) {
        return -10005;
    }
    gdev_cc_open_state.is_open = 1;
    return 0;
}

s32 gdev_cc_close(void)
{
    return 0;
}

s32 gdev_cc_read(u8* dst, s32 len)
{
    u8 staging[GDEV_CC_BUFFER_SIZE];
    s32 err = 0;
    s32 avail;

    if (gdev_cc_open_state.is_open == 0) {
        return -10001;
    }
    while (CBGetBytesAvailableForRead(&gdev_cc_circle_buffer) < len) {
        err = 0;
        avail = DBQueryData();
        if (avail != 0) {
            err = DBRead(staging, len);
            if (err == 0) {
                CircleBufferWriteBytes(&gdev_cc_circle_buffer, staging, avail);
            }
        }
    }
    if (err == 0) {
        CircleBufferReadBytes(&gdev_cc_circle_buffer, dst, len);
    }
    return err;
}

s32 gdev_cc_write(const u8* src, s32 len)
{
    s32 written;

    if (gdev_cc_open_state.is_open == 0) {
        return -10001;
    }
    while (len > 0) {
        written = DBWrite(src, len);
        if (written == 0) {
            break;
        }
        src += written;
        len -= written;
    }
    return 0;
}

s32 gdev_cc_pre_continue(void)
{
    EXI2_Unreserve();
    return 0;
}

s32 gdev_cc_post_stop(void)
{
    EXI2_Reserve();
    return 0;
}

s32 gdev_cc_peek(void)
{
    u8 staging[GDEV_CC_BUFFER_SIZE];
    s32 avail = DBQueryData();

    if (avail <= 0) {
        return 0;
    }
    if (DBRead(staging, avail) == 0) {
        CircleBufferWriteBytes(&gdev_cc_circle_buffer, staging, avail);
    } else {
        return -10009;
    }
    return avail;
}

s32 gdev_cc_initinterrupts(void)
{
    DBInitInterrupts();
    return 0;
}
