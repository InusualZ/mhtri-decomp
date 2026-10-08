/*
 * TRK/msghndlr.c - the MetroTRK command handlers: connect, disconnect, reset, override, read/write memory and
 *    registers, continue, step, stop, set option.
 *
 * RANGE. .text 0x8046AADC..0x8046BA60 (13 functions in the map, 0xF84 B); .data 0x8060F7A8..0x8060F810; .sbss
 *    0x80794E70..0x80794E78.
 * FLAGS. the `OS` lib's `cflags_os` plus `-str reuse,pool` (configure.py `extra_cflags`: the three option strings sit
 *    contiguously behind `@stringBase0`).
 * NAMES. file name GUESS (MetroTRK `msghndlr`); `SetUseSerialIO` (map row renamed from fn_8046D410, partner of
 *    `GetUseSerialIO`), `trk_option_strings`, `gTRKSequence` and `gTRKConnected` are GUESS (the counter every reply
 *    stamps and the flag connect/disconnect write); `TRKReplyHeader`, `TRKRequestHeader` and the reply helpers are GUESS
 *    (the 0x40 B header every message starts with: length, command, status or options, then command arguments).
 * EVIDENCE. jump tables `.data` 0x8060F7A8 / 0x8060F7C4 (error-code switches of the two memory handlers), the option strings
 *    0x8060F7E0 and `.sbss` 0x80794E70 / 0x80794E74 are read only by this run.
 * RESIDUALS. every status reply (`trk_send_status`) keeps the sequence number in r5 and the constant status in r6, the target
 *    the other way round (one register-pair swap per reply, 12 rows at 95.7..99.8); `TRKDoStep` and `TRKDoWriteRegisters`
 *    also number their saved registers differently (message/start/end in r28..r30 against r30..r28; the error code in r29
 *    against r31) and `TRKDoWriteRegisters` places the `TRK_MessageSend` tail after the compare tree (4 B longer in
 *    `TRKDoStep`: the `li r3,0` return sits after the `b`); `trk_option_strings` is `@stringBase0` in our object. Measured
 *    without effect on the swap: reply field store order (6 permutations), `++`/`+ 1` spelling, `u8`/`s32` status
 *    parameter, macro against inline.
 * SHAPES. every handler answers with a 0x40 B reply header built in its own stack slot (status code mapped from the
 *    internal error), then returns 0; the register and memory handlers append the payload to the request buffer and send it
 *    with `TRK_MessageSend` instead.
 */
#include "TRK/msghndlr.h"
#include "TRK/mem_TRK.h"
#include "TRK/msg.h"
#include "TRK/msgbuf.h"
#include "TRK/nubevent.h"
#include "TRK/targcont.h"
#include "TRK/targimpl.h"
#include "TRK/TRK_WriteUARTN.h"
#include "TRK/dolphin_trk.h"
#include "OS/OSError.h"

#define TRK_HEADER_SIZE 0x40
#define TRK_COMMAND_REPLY 0x80
#define TRK_MEMORY_BUFFER_SIZE 0x820
#define TRK_REGISTER_REPLY_SIZE 0x468
#define TRK_OPTION_BAD_RANGE 0x02    /* request option bits (GUESS names) */
#define TRK_OPTION_UNMAPPED 0x08
#define TRK_OPTION_ALIGN 0x40

/* The 0x40 B header of a reply (size: 0x40). */
typedef struct TRKReplyHeader {
    /* +0x00 */ u32 length;        /* header plus payload bytes */
    /* +0x04 */ u8 command;        /* TRK_COMMAND_REPLY */
    /* +0x05 */ u8 pad_0x05[3];
    /* +0x08 */ u8 status;         /* 0, or the debugger error code */
    /* +0x09 */ u8 pad_0x09[3];
    /* +0x0C */ u32 sequence;      /* running message number */
    /* +0x10 */ u8 pad_0x10[0x30];
} TRKReplyHeader; /* size: 0x40 */

/* The start of a request message in a buffer's data (size: 0x18; the arguments overlay by command). */
typedef struct TRKRequestHeader {
    /* +0x00 */ u32 length;
    /* +0x04 */ u8 command;
    /* +0x05 */ u8 pad_0x05[3];
    /* +0x08 */ u8 options;
    /* +0x09 */ u8 pad_0x09[3];
    /* +0x0C */ union {
        u16 byte_count;         /* memory read/write */
        u16 first_register;     /* register read/write */
        u8 step_count;          /* step: instructions to run */
        u8 option_value;        /* set option */
    } arg0;
    /* +0x10 */ union {
        u32 address;            /* memory read/write */
        u16 last_register;      /* register read/write */
        u32 range_start;        /* step out of range */
    } arg1;
    /* +0x14 */ u32 range_end;
} TRKRequestHeader; /* size: 0x18 */

s32 gTRKConnected;
u32 gTRKSequence;

/* Writes a status reply header straight to the UART. */
static inline void trk_send_status(s32 status)
{
    TRKReplyHeader reply;

    TRK_memset(&reply, 0, sizeof(reply));
    reply.command = TRK_COMMAND_REPLY;
    reply.length = TRK_HEADER_SIZE;
    reply.status = status;
    reply.sequence = gTRKSequence + 1;
    gTRKSequence = reply.sequence + 1;
    TRK_WriteUARTN(&reply, TRK_HEADER_SIZE);
}

s32 GetTRKConnected(void)
{
    return gTRKConnected;
}

s32 TRK_DoConnect(TRKBuffer* message)
{
    gTRKConnected = 1;
    trk_send_status(0);
    return 0;
}

s32 TRKDoDisconnect(TRKBuffer* message)
{
    TRKEvent event;

    gTRKConnected = 0;
    trk_send_status(0);
    TRKConstructEvent(&event, TRK_EVENT_SHUTDOWN);
    TRKPostEvent(&event);
    return 0;
}

s32 TRKDoReset(TRKBuffer* message)
{
    trk_send_status(0);
    __TRKreset();
    return 0;
}

s32 TRKDoOverride(TRKBuffer* message)
{
    trk_send_status(0);
    TRKOverrideInterruptVectors();
    return 0;
}

s32 TRKDoReadMemory(TRKBuffer* message)
{
    TRKRequestHeader* request = (TRKRequestHeader*)message->data;
    __declspec(align(32)) u8 data[TRK_MEMORY_BUFFER_SIZE];
    s32 error = 0;
    u8 options = request->options;
    u32 address = request->arg1.address;
    u16 byte_count = request->arg0.byte_count;
    u32 length;

    if ((options & TRK_OPTION_BAD_RANGE) != 0) {
        trk_send_status(0x12);
        return 0;
    }
    if (error == 0) {
        TRKReplyHeader reply;

        length = byte_count;
        error = TRKTargetAccessMemory(data, address, &length, (options & TRK_OPTION_UNMAPPED) ? 0 : 1, 1);
        TRKResetBuffer(message, 0);
        if (error == 0) {
            TRK_memset(&reply, 0, sizeof(reply));
            reply.status = error;
            reply.length = length + TRK_HEADER_SIZE;
            reply.command = TRK_COMMAND_REPLY;
            reply.sequence = gTRKSequence;
            gTRKSequence = reply.sequence + 1;
            TRK_AppendBuffer(message, &reply, TRK_HEADER_SIZE);
            if ((options & TRK_OPTION_ALIGN) != 0) {
                error = TRK_AppendBuffer(message, data + (address & 0x1F), length);
            } else {
                error = TRK_AppendBuffer(message, data, length);
            }
        }
    }
    if (error != 0) {
        switch (error) {
        case 0x702:
            error = 0x15;
            break;
        case 0x700:
            error = 0x13;
            break;
        case 0x704:
            error = 0x21;
            break;
        case 0x705:
            error = 0x22;
            break;
        case 0x706:
            error = 0x20;
            break;
        default:
            error = 3;
            break;
        }
        trk_send_status(error);
        return 0;
    }
    return TRK_MessageSend(message);
}

s32 TRKDoWriteMemory(TRKBuffer* message)
{
    TRKRequestHeader* request = (TRKRequestHeader*)message->data;
    __declspec(align(32)) u8 data[TRK_MEMORY_BUFFER_SIZE];
    s32 error = 0;
    u8 options = request->options;
    u32 address = request->arg1.address;
    u16 byte_count = request->arg0.byte_count;
    u32 length;

    if ((options & TRK_OPTION_BAD_RANGE) != 0) {
        trk_send_status(0x12);
        return 0;
    }
    if (error == 0) {
        TRKReplyHeader reply;

        length = byte_count;
        TRK_SetBufferPosition(message, TRK_HEADER_SIZE);
        TRK_ReadBuffer(message, data, length);
        error = TRKTargetAccessMemory(data, address, &length, (options & TRK_OPTION_UNMAPPED) ? 0 : 1, 0);
        TRKResetBuffer(message, 0);
        if (error == 0) {
            TRK_memset(&reply, 0, sizeof(reply));
            reply.length = TRK_HEADER_SIZE;
            reply.command = TRK_COMMAND_REPLY;
            reply.status = error;
            reply.sequence = gTRKSequence;
            gTRKSequence = reply.sequence + 1;
            error = TRK_AppendBuffer(message, &reply, TRK_HEADER_SIZE);
        }
    }
    if (error != 0) {
        switch (error) {
        case 0x702:
            error = 0x15;
            break;
        case 0x700:
            error = 0x13;
            break;
        case 0x704:
            error = 0x21;
            break;
        case 0x705:
            error = 0x22;
            break;
        case 0x706:
            error = 0x20;
            break;
        default:
            error = 3;
            break;
        }
        trk_send_status(error);
        return 0;
    }
    return TRK_MessageSend(message);
}

s32 TRKDoReadRegisters(TRKBuffer* message)
{
    TRKRequestHeader* request = (TRKRequestHeader*)message->data;
    TRKReplyHeader reply;
    u32 count;
    s32 error;

    if (request->arg0.first_register > request->arg1.last_register) {
        trk_send_status(0x14);
        return 0;
    }
    reply.length = TRK_REGISTER_REPLY_SIZE;
    reply.command = TRK_COMMAND_REPLY;
    reply.sequence = gTRKSequence;
    gTRKSequence = reply.sequence + 1;
    TRKResetBuffer(message, 0);
    TRKAppendBuffer_ui8(message, &reply, TRK_HEADER_SIZE);
    error = TRKTargetAccessDefault(0, 0x24, message, &count, 1);
    if (error == 0) {
        error = TRKTargetAccessFP(0, 0x21, message, &count, 1);
    }
    if (error == 0) {
        error = TRKTargetAccessExtended1(0, 0x60, message, &count, 1);
    }
    if (error == 0) {
        error = TRKTargetAccessExtended2(0, 0x1F, message, &count, 1);
    }
    if (error != 0) {
        switch (error) {
        case 0x703:
            error = 0x12;
            break;
        case 0x701:
            error = 0x14;
            break;
        case 0x702:
            error = 0x15;
            break;
        case 0x704:
            error = 0x21;
            break;
        case 0x705:
            error = 0x22;
            break;
        case 0x706:
            error = 0x20;
            break;
        default:
            error = 3;
            break;
        }
        trk_send_status(error);
        return 0;
    }
    return TRK_MessageSend(message);
}

s32 TRKDoWriteRegisters(TRKBuffer* message)
{
    TRKRequestHeader* request = (TRKRequestHeader*)message->data;
    u8 kind = request->options;
    u16 first = request->arg0.first_register;
    u16 last = request->arg1.last_register;
    TRKReplyHeader reply;
    u32 count;
    s32 error;

    TRK_SetBufferPosition(message, 0);
    if (first > last) {
        trk_send_status(0x14);
        return 0;
    }
    TRK_SetBufferPosition(message, TRK_HEADER_SIZE);
    switch (kind) {
    case 0:
        error = TRKTargetAccessDefault(first, last, message, &count, 0);
        break;
    case 1:
        error = TRKTargetAccessFP(first, last, message, &count, 0);
        break;
    case 2:
        error = TRKTargetAccessExtended1(first, last, message, &count, 0);
        break;
    case 3:
        error = TRKTargetAccessExtended2(first, last, message, &count, 0);
        break;
    default:
        error = 0x703;
        break;
    }
    TRKResetBuffer(message, 0);
    if (error == 0) {
        TRK_memset(&reply, 0, sizeof(reply));
        reply.length = TRK_HEADER_SIZE;
        reply.command = TRK_COMMAND_REPLY;
        reply.status = error;
        reply.sequence = gTRKSequence;
        gTRKSequence = reply.sequence + 1;
        error = TRK_AppendBuffer(message, &reply, TRK_HEADER_SIZE);
    }
    switch (error) {
    case 0x703:
        error = 0x12;
        break;
    case 0x701:
        error = 0x14;
        break;
    case 0x302:
        error = 2;
        break;
    case 0x702:
        error = 0x15;
        break;
    case 0x704:
        error = 0x21;
        break;
    case 0x705:
        error = 0x22;
        break;
    case 0x706:
        error = 0x20;
        break;
    case 0:
        return TRK_MessageSend(message);
    default:
        error = 3;
        break;
    }
    trk_send_status(error);
    return 0;
}

s32 TRKDoContinue(TRKBuffer* message)
{
    if (TRKTargetStopped() == 0) {
        trk_send_status(0x16);
        return 0;
    }
    trk_send_status(0);
    return TRKTargetContinue();
}

s32 TRKDoStep(TRKBuffer* message)
{
    TRKRequestHeader* request = (TRKRequestHeader*)message->data;
    u8 kind;
    u32 count;
    u32 range_start;
    u32 range_end;
    u32 pc;

    TRK_SetBufferPosition(message, 0);
    kind = request->options;
    range_start = request->arg1.range_start;
    range_end = request->range_end;
    switch (kind) {
    case 0:
    case 0x10:
        count = request->arg0.step_count;
        if (count < 1) {
            trk_send_status(0x11);
            return 0;
        }
        break;
    case 1:
    case 0x11:
        pc = TRKTargetGetPC();
        if (pc < range_start || pc > range_end) {
            trk_send_status(0x11);
            return 0;
        }
        break;
    default:
        trk_send_status(0x12);
        return 0;
    }
    if (TRKTargetStopped() == 0) {
        trk_send_status(0x16);
        return 0;
    }
    trk_send_status(0);
    switch (kind) {
    case 0:
    case 0x10:
        return TRKTargetSingleStep(count, kind == 0x10);
    case 1:
    case 0x11:
        return TRKTargetStepOutOfRange(range_start, range_end, kind == 0x11);
    default:
        return 0;
    }
}

s32 TRKDoStop(TRKBuffer* message)
{
    u8 status;

    switch (TRKTargetStop()) {
    case 0:
        status = 0;
        break;
    case 0x704:
        status = 0x21;
        break;
    case 0x705:
        status = 0x22;
        break;
    case 0x706:
        status = 0x20;
        break;
    default:
        status = 1;
        break;
    }
    trk_send_status(status);
    return 0;
}

s32 TRKDoSetOption(TRKBuffer* message)
{
    TRKRequestHeader* request = (TRKRequestHeader*)message->data;
    u8 option_value = request->arg0.option_value;

    if (request->options == 1) {
        OSReport("\nMetroTRK Option : SerialIO - ");
        if (option_value != 0) {
            OSReport("Enable\n");
        } else {
            OSReport("Disable\n");
        }
        SetUseSerialIO(option_value);
    }
    trk_send_status(0);
    return 0;
}
