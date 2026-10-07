/*
 * TRK/support.c - the MetroTRK host support requests: `TRK_RequestSend` and the file requests
 *    `TRKTargetSupportRequest` forwards (access, open, close, position).
 *
 * RANGE. .text 0x804698D0..0x80469F10 (6 functions in the map, 0x640 B); .data 0x8060F710..0x8060F758.
 * FLAGS. the `OS` lib's `cflags_os` plus `-str reuse,pool` (configure.py `extra_cflags`: pooled strings, `TRK_RequestSend` 95.5 -> 96.8)
 *    with `use_lmw_stmw on` (the bodies save r19..r31 with stmw) and `dont_inline` around
 *    `TRKStringLength` (the target calls it from the open request instead of inlining it).
 * NAMES. file name GUESS (MetroTRK `support`); `TRKStringLength`, `TRKSuppAccessFile`, `TRKSuppOpenFile`, `TRKSuppCloseFile`
 *    and `TRKSuppPositionFile` (map rows renamed from fn_) are GUESS (a string length, and the requests that send command
 *    bytes 0xD0/0xD1 read-write, 0xD2 open, 0xD3 close and 0xD4 position to the host); `TRKSupportPacket`,
 *    `TRKSupportReply` and their fields are GUESS from the offsets the bodies fill and read.
 * EVIDENCE. `.data` 0x8060F710 (`bad reply size`, `failed in RequestSend`) is read only by `TRK_RequestSend`; the four
 *    request builders are called only from `TRKTargetSupportRequest`.
 * RESIDUALS. register allocation only, same opcodes: `TRK_RequestSend` keeps one saved register more than the target (the
 *    constant -1 and the status variable do not share r31, 95 %); `TRKSuppAccessFile` numbers its parameter copies r26..r31
 *    where the target uses r24..r29 (98 %); `TRKSuppCloseFile` swaps r30/r31 (99 %).
 * SHAPES. every request fills a 0x40 B packet, appends it (and any payload) to a free buffer and sends it; a reply is
 *    a buffer whose command byte is 0x80 or more, anything below is a debugger command and is forwarded as input.
 */
#include "TRK/support.h"
#include "TRK/mem_TRK.h"
#include "TRK/msg.h"
#include "TRK/serpoll.h"
#include "OS/OSError.h"

#pragma use_lmw_stmw on

#define TRK_PACKET_SIZE 0x40
#define TRK_SUPPORT_CHUNK_SIZE 0x800
#define TRK_REPLY_COMMAND_MIN 0x80

#define TRK_CMD_SUPP_WRITE_FILE 0xD0
#define TRK_CMD_SUPP_READ_FILE 0xD1
#define TRK_CMD_SUPP_OPEN_FILE 0xD2
#define TRK_CMD_SUPP_CLOSE_FILE 0xD3
#define TRK_CMD_SUPP_POSITION_FILE 0xD4

/* The request packet the target sends to the host. */
typedef struct TRKSupportPacket {
    /* +0x00 */ u32 length;          /* packet length including any payload */
    /* +0x04 */ u8 command;          /* TRK_CMD_SUPP_* */
    /* +0x05 */ u8 pad_0x05[3];
    union {
        /* +0x08 */ s32 handle;      /* file handle */
        /* +0x08 */ u8 open_mode;    /* open mode of an open request */
    } file;
    union {
        /* +0x0C */ u16 count;       /* bytes moved by read/write, path length for open */
        /* +0x0C */ s32 position;    /* file position for the position request */
    } arg;
    /* +0x10 */ u8 origin;           /* position origin of a position request */
    /* +0x11 */ u8 pad_0x11[0x2F];
} TRKSupportPacket; /* size: 0x40 */

/* The reply as it lies in the message buffer data. */
typedef struct TRKSupportReply {
    /* +0x00 */ u32 length;
    /* +0x04 */ u8 command;          /* 0x80 for a reply */
    /* +0x05 */ u8 pad_0x05[3];
    union {
        /* +0x08 */ u8 error;        /* host error byte */
        /* +0x08 */ s32 handle;      /* the handle of an opened file */
    } result;
    /* +0x0C */ u32 pad_0x0C;
    /* +0x10 */ u32 io_result;       /* the host's file status */
    /* +0x14 */ u16 count;           /* bytes the host moved */
    /* +0x16 */ u8 pad_0x16[2];
    /* +0x18 */ s32 position;        /* the file position after a position request */
} TRKSupportReply; /* size: 0x1C (only the fields the requests read are named) */

#pragma dont_inline on
s32 TRKStringLength(const char* text)
{
    const char* p = text - 1;
    s32 length = -1;

    do {
        length++;
    } while ((u8)*++p != 0);
    return length;
}
#pragma dont_inline reset

s32 TRKSuppAccessFile(s32 handle, u8* data, u32* count, u32* io_result, u8 need_reply, u8 is_read)
{
    TRKSupportPacket packet;
    s32 reply_id;
    s32 buffer_id;
    TRKBuffer* buffer;
    TRKBuffer* reply;
    s32 err;
    s32 done;
    u32 offset;
    u32 chunk;
    u32 reply_count;
    u32 reply_io;
    u8 command;
    u32 length;

    if (data == NULL || *count == 0) {
        return 2;
    }
    done = 0;
    *io_result = 0;
    offset = 0;
    err = 0;
    while (!done && offset < *count && err == 0 && *io_result == 0) {
        TRK_memset(&packet, 0, TRK_PACKET_SIZE);
        chunk = TRK_SUPPORT_CHUNK_SIZE;
        if (*count - offset <= TRK_SUPPORT_CHUNK_SIZE) {
            chunk = *count - offset;
        }
        command = TRK_CMD_SUPP_WRITE_FILE;
        if (is_read) {
            command = TRK_CMD_SUPP_READ_FILE;
        }
        packet.command = command;
        length = TRK_PACKET_SIZE;
        if (!is_read) {
            length = chunk + TRK_PACKET_SIZE;
        }
        packet.length = length;
        packet.file.handle = handle;
        packet.arg.count = chunk;
        TRK_GetFreeBuffer(&buffer_id, &buffer);
        err = TRKAppendBuffer_ui8(buffer, &packet, TRK_PACKET_SIZE);
        if (!is_read && err == 0) {
            err = TRKAppendBuffer_ui8(buffer, data + offset, chunk);
        }
        if (err == 0) {
            if (need_reply) {
                err = TRK_RequestSend(buffer, &reply_id);
                if (err == 0) {
                    reply = TRKGetBuffer(reply_id);
                }
                reply_count = ((TRKSupportReply*)reply->data)->count;
                reply_io = (u8)((TRKSupportReply*)reply->data)->io_result;
                if (is_read && err == 0 && reply_count <= chunk) {
                    TRK_SetBufferPosition(reply, TRK_PACKET_SIZE);
                    err = TRKReadBuffer_ui8(reply, data + offset, reply_count);
                    if (err == 0x302) {
                        err = 0;
                    }
                }
                if (reply_count != chunk) {
                    chunk = reply_count;
                    done = 1;
                }
                *io_result = reply_io;
                TRK_ReleaseBuffer(reply_id);
            } else {
                err = TRK_MessageSend(buffer);
            }
        }
        TRK_ReleaseBuffer(buffer_id);
        offset += chunk;
    }
    *count = offset;
    return err;
}

s32 TRK_RequestSend(TRKBuffer* message, s32* reply_id)
{
    s32 err;
    s32 bad_reply;
    s32 status;
    s32 command;
    TRKBuffer* reply;

    err = TRK_MessageSend(message);
    if (err == 0) {
        bad_reply = 0;
        status = -1;
        for (;;) {
            *reply_id = TRKTestForPacket();
            if (*reply_id == -1) {
                continue;
            }
            reply = TRKGetBuffer(*reply_id);
            TRK_SetBufferPosition(reply, 0);
            command = reply->data[4];
            if ((u32)command >= TRK_REPLY_COMMAND_MIN) {
                break;
            }
            TRKProcessInput(*reply_id);
            *reply_id = -1;
        }
        if (*reply_id != -1) {
            if (reply->length < TRK_PACKET_SIZE) {
                OSReport("MetroTRK - bad reply size %ld\n", reply->length);
                bad_reply = 1;
            }
            if (err == 0 && bad_reply == 0) {
                status = ((TRKSupportReply*)reply->data)->result.error;
            }
            if (err == 0 && bad_reply == 0 && (command != TRK_REPLY_COMMAND_MIN || status != 0)) {
                bad_reply = 1;
            }
            if (err != 0 || bad_reply != 0) {
                TRK_ReleaseBuffer(*reply_id);
                *reply_id = -1;
            }
        }
    }
    if (*reply_id == -1) {
        OSReport("MetroTRK - failed in RequestSend\n");
        err = 0x800;
    }
    return err;
}

s32 TRKSuppOpenFile(const char* path, u8 mode, u32* handle, u32* io_result)
{
    TRKSupportPacket packet;
    s32 reply_id;
    s32 buffer_id;
    TRKBuffer* buffer;
    s32 err;
    TRKBuffer* reply;

    TRK_memset(&packet, 0, TRK_PACKET_SIZE);
    *handle = 0;
    packet.command = TRK_CMD_SUPP_OPEN_FILE;
    packet.length = TRKStringLength(path) + TRK_PACKET_SIZE + 1;
    packet.file.open_mode = mode;
    packet.arg.count = TRKStringLength(path) + 1;
    TRK_GetFreeBuffer(&buffer_id, &buffer);
    err = TRKAppendBuffer_ui8(buffer, &packet, TRK_PACKET_SIZE);
    if (err == 0) {
        err = TRKAppendBuffer_ui8(buffer, path, TRKStringLength(path) + 1);
    }
    if (err == 0) {
        *io_result = 0;
        err = TRK_RequestSend(buffer, &reply_id);
        if (err == 0) {
            reply = TRKGetBuffer(reply_id);
        }
        *io_result = ((TRKSupportReply*)reply->data)->io_result;
        *handle = ((TRKSupportReply*)reply->data)->result.handle;
        TRK_ReleaseBuffer(reply_id);
    }
    TRK_ReleaseBuffer(buffer_id);
    return err;
}

s32 TRKSuppCloseFile(s32 handle, u32* io_result)
{
    TRKSupportPacket packet;
    s32 reply_id;
    s32 buffer_id;
    TRKBuffer* buffer;
    TRKBuffer* reply;
    s32 err;

    TRK_memset(&packet, 0, TRK_PACKET_SIZE);
    packet.command = TRK_CMD_SUPP_CLOSE_FILE;
    packet.length = TRK_PACKET_SIZE;
    packet.file.handle = handle;
    err = TRK_GetFreeBuffer(&buffer_id, &buffer);
    if (err == 0) {
        err = TRKAppendBuffer_ui8(buffer, &packet, TRK_PACKET_SIZE);
    }
    if (err == 0) {
        *io_result = 0;
        err = TRK_RequestSend(buffer, &reply_id);
        if (err == 0) {
            reply = TRKGetBuffer(reply_id);
        }
        if (err == 0) {
            *io_result = ((TRKSupportReply*)reply->data)->io_result;
        }
        TRK_ReleaseBuffer(reply_id);
    }
    TRK_ReleaseBuffer(buffer_id);
    return err;
}

s32 TRKSuppPositionFile(s32 handle, s32* position, u8 origin, u32* io_result)
{
    TRKSupportPacket packet;
    s32 reply_id;
    s32 buffer_id;
    TRKBuffer* buffer;
    TRKBuffer* reply;
    s32 err;

    TRK_memset(&packet, 0, TRK_PACKET_SIZE);
    packet.command = TRK_CMD_SUPP_POSITION_FILE;
    packet.length = TRK_PACKET_SIZE;
    packet.file.handle = handle;
    packet.arg.position = *position;
    packet.origin = origin;
    err = TRK_GetFreeBuffer(&buffer_id, &buffer);
    if (err == 0) {
        err = TRKAppendBuffer_ui8(buffer, &packet, TRK_PACKET_SIZE);
    }
    if (err == 0) {
        *io_result = 0;
        *position = -1;
        err = TRK_RequestSend(buffer, &reply_id);
        if (err == 0) {
            reply = TRKGetBuffer(reply_id);
            if (reply != NULL) {
                *io_result = ((TRKSupportReply*)reply->data)->io_result;
                *position = ((TRKSupportReply*)reply->data)->position;
            }
        }
        TRK_ReleaseBuffer(reply_id);
    }
    TRK_ReleaseBuffer(buffer_id);
    return err;
}
