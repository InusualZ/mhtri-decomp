/*
 * TRK/gdev_cc.c - the MetroTRK debugger channel over the EXI/DB interface: `gdev_cc_initialize`, `shutdown`,
 *    `open`, `close`, `read`, `write`, `pre_continue`, `post_stop`, `peek`, `initinterrupts`.
 *
 * RANGE. .text 0x80468378..0x804685F0 (10 functions in the map, 0x278 B); .bss 0x806F5020..0x806F5540; .sbss
 *    0x80794E38..0x80794E40.
 * FLAGS. the `OS` lib's `cflags_os`.
 * NAMES. function names are the dump's (`gdev_cc_open`, `close`, `read`, `pre_continue`, `post_stop`, `peek`, `initinterrupts` are `gdev_cc_close` is a GUESS (the dump labels the address with a neighbour or a placeholder; the name fits the unit scheme). `gdev_cc_read` is a GUESS (the dump labels the address with a neighbour or a placeholder; the name fits the unit scheme). `gdev_cc_pre_continue` is a GUESS (the dump labels the address with a neighbour or a placeholder; the name fits the unit scheme). `gdev_cc_post_stop` is a GUESS (the dump labels the address with a neighbour or a placeholder; the name fits the unit scheme). `gdev_cc_peek` is a GUESS (the dump labels the address with a neighbour or a placeholder; the name fits the unit scheme). `gdev_cc_initinterrupts` is a GUESS (the dump labels the address with a neighbour or a placeholder; the name fits the unit scheme).
 *    the map's and GUESS where the dump's neighbours disagree); `gdev_cc_buffer`, `gdev_cc_circle_buffer`, `gdev_cc_open_flag` are
 *    GUESS (the bytes the bodies fill, queue through and test); the return codes are written as numbers.
 * EVIDENCE. ten consecutive `gdev_cc_*` dump names; `.bss` 0x806F5020 (0x500 B) and 0x806F5520 (0x20 B, the
 *    receive circle buffer) and `.sbss` 0x80794E38 are read only here. `DBInitComm` .. `DBInitInterrupts` and the two
 *    4-byte stubs `pre_continue`/`post_stop` call belong to `VF/vf.cpp`.
 * RESIDUALS. link order: the compiler emits the 0x20 B circle buffer before the 0x500 B receive buffer in `.bss`
 *    (the target has the receive buffer first, 0x806F5020), which shifts the DOL hash, so the unit stays NonMatching
 *    although every row is at 100 %; the open flag map row is 8 B, the source emits a 4 B word.
 * SHAPES. `read` and `peek` stage the DB bytes in a 0x500 B stack array, then queue them in the circle buffer.
 */
#include "TRK/CircleBuffer.h"
#include "VF/DBInitComm.h"
#include "VF/DBQueryData.h"
#include "VF/DBRead.h"
#include "VF/DBWrite.h"
#include "VF/DBInitInterrupts.h"
#include "VF/EXI2_Reserve.h"
#include "VF/EXI2_Unreserve.h"
#pragma use_lmw_stmw on


#define GDEV_CC_BUFFER_SIZE 0x500

u8 gdev_cc_buffer[GDEV_CC_BUFFER_SIZE];
CircleBuffer gdev_cc_circle_buffer;
s32 gdev_cc_open_flag;

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
    if (gdev_cc_open_flag != 0) {
        return -10005;
    }
    gdev_cc_open_flag = 1;
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

    if (gdev_cc_open_flag == 0) {
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

    if (gdev_cc_open_flag == 0) {
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
