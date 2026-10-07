/*
 * TRK/msg.c - `TRK_MessageSend` (stamps a message id and writes the framed message to the UART).
 *
 * RANGE. .text 0x8046A26C..0x8046A2D0 (1 function in the map, 0x64 B); .data 0x8060F758..0x8060F780; .sbss
 *    0x80794E68..0x80794E70.
 * FLAGS. the `OS` lib's `cflags_os`.
 * NAMES. file name GUESS (MetroTRK `msg`); `trk_message_id_counter` and `trk_write_uart_error_format` are GUESS (the
 *    sequence word the function bumps and the report string it prints).
 * EVIDENCE. `.data` 0x8060F758 (`MetroTRK - TRK_WriteUARTN returned %ld`) and `.sbss` 0x80794E68 are read only by it.
 * RESIDUALS. the id lives in r4 where the target uses r0 and the target zero-extends it (`clrlwi`) before the increment, which
 *    the compiler omits (96 B vs 100 B); the `OSReport` call goes through a fixed-argument prototype so it
 *    carries no `crclr`, as the target.
 * SHAPES. the id counter skips 0 on wrap; a failed UART write is reported through `OSReport`.
 */
#include "TRK/msg.h"
#include "TRK/TRK_WriteUARTN.h"
#include "TRK/trk_report.h"

/* The frame at the start of a message buffer's data. */
typedef struct TRKFrame {
    /* +0x00 */ u8 header[6];      /* first bytes of the frame */
    /* +0x06 */ u16 message_id;    /* sequence number stamped on send */
} TRKFrame; /* size: 0x8 (the frame continues with the payload) */

static u16 trk_message_id_counter;

s32 TRK_MessageSend(TRKBuffer* buffer)
{
    u16 id = trk_message_id_counter;
    s32 err;

    if (id == 0) {
        id = 1;
    }
    ((TRKFrame*)buffer->data)->message_id = id;
    trk_message_id_counter = id + 1;
    err = TRK_WriteUARTN(buffer->data, buffer->length);
    if (err != 0) {
        TRK_REPORT_S32("MetroTRK - TRK_WriteUARTN returned %ld\n", err);
    }
    return 0;
}
