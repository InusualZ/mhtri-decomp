/*
 * TRK/serpoll.c - the MetroTRK serial poll layer: `TRKTestForPacket`, `TRKGetInput`, `TRKProcessInput`,
 *    `TRKInitializeSerialHandler` and its terminate twin.
 *
 * RANGE. .text 0x80469788..0x804698D0 (5 functions in the map, 0x148 B).
 * FLAGS. the `OS` lib's `cflags_os`.
 * NAMES. file name GUESS (MetroTRK `serpoll`); `packet` and `TRK_PACKET_HEADER_SIZE` are GUESS (the 0x40 B read that
 *    starts a packet and carries its length in the first word).
 * EVIDENCE. dump names; call-graph closure (all callees are the buffer and UART layers); no data.
 * RESIDUALS. none known.
 * SHAPES. a packet is read as a 0x40 B header, then the rest up to the length the header announces; any read failure
 *    releases the buffer and yields -1
 *    (a failed header read releases the `TRK_GetFreeBuffer` return value, as the target does).
 */
#include "TRK/serpoll.h"
#include "TRK/dolphin_trk.h"
#include "TRK/msgbuf.h"
#include "TRK/nubevent.h"

#define TRK_PACKET_HEADER_SIZE 0x40
#define TRK_PACKET_MAX_SIZE (TRK_MSG_BUFFER_DATA_SIZE + TRK_PACKET_HEADER_SIZE)

s32 TRKTestForPacket(void)
{
    u8 packet[TRK_PACKET_MAX_SIZE];
    s32 buffer_id;
    TRKBuffer* buffer;
    s32 result;

    if (TRKPollUART() <= 0) {
        return -1;
    }
    result = TRK_GetFreeBuffer(&buffer_id, &buffer);
    TRK_SetBufferPosition(buffer, 0);
    if (TRKReadUARTN(packet, TRK_PACKET_HEADER_SIZE) == 0) {
        TRKAppendBuffer_ui8(buffer, packet, TRK_PACKET_HEADER_SIZE);
        result = buffer_id;
        if (*(s32*)packet - TRK_PACKET_HEADER_SIZE > 0) {
            if (TRKReadUARTN(packet + TRK_PACKET_HEADER_SIZE, *(s32*)packet - TRK_PACKET_HEADER_SIZE) == 0) {
                TRKAppendBuffer_ui8(buffer, packet + TRK_PACKET_HEADER_SIZE, *(s32*)packet);
            } else {
                TRK_ReleaseBuffer(result);
                result = -1;
            }
        }
    } else {
        TRK_ReleaseBuffer(result);
        result = -1;
    }
    return result;
}

void TRKGetInput(void)
{
    s32 buffer_id = TRKTestForPacket();

    if (buffer_id != -1) {
        TRKProcessInput(buffer_id);
    }
}

void TRKProcessInput(s32 buffer_id)
{
    TRKEvent event;

    TRKConstructEvent(&event, TRK_EVENT_REQUEST);
    event.buffer_id = buffer_id;
    TRKPostEvent(&event);
}

s32 TRKInitializeSerialHandler(void)
{
    return 0;
}

s32 TRKTerminateSerialHandler(void)
{
    return 0;
}
