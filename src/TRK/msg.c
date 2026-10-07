/*
 * TRK/msg.c - `TRK_MessageSend` (stamps a message id and writes the framed message to the UART).
 *
 * RANGE. .text 0x8046A26C..0x8046A2D0 (1 function in the map, 0x64 B); .data 0x8060F758..0x8060F780; .sbss
 *    0x80794E68..0x80794E70.
 * FLAGS. the `OS` lib's `cflags_os`.
 * NAMES. file name GUESS (MetroTRK `msg`); `trk_message_id_counter` and `trk_write_uart_error_format` are GUESS (the
 *    sequence word the function bumps and the report string it prints).
 * EVIDENCE. `.data` 0x8060F758 (`MetroTRK - TRK_WriteUARTN returned %ld`) and `.sbss` 0x80794E68 are read only by it.
 * RESIDUALS. the id lives in r4 where the target uses r0 and the target zero-extends it (`clrlwi`) before the increment; the
 *    `OSReport` call carries a `crclr` the target lacks (its prototype is not variadic-looking); same instruction count.
 * SHAPES. the id counter skips 0 on wrap; a failed UART write is reported through `OSReport`.
 */
#include "TRK/msg.h"
#include "TRK/TRK_WriteUARTN.h"
#include "OS/OSError.h"

static u16 trk_message_id_counter;

s32 TRK_MessageSend(TRKMessage* msg)
{
    u32 id = trk_message_id_counter;
    s32 err;

    if (id == 0) {
        id = 1;
    }
    msg->message_id = id;
    trk_message_id_counter = (u16)id + 1;
    err = TRK_WriteUARTN(msg->header, msg->length);
    if (err != 0) {
        OSReport("MetroTRK - TRK_WriteUARTN returned %ld\n", err);
    }
    return 0;
}
