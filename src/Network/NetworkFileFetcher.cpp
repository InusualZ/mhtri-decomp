/*
 * Network/NetworkFileFetcher.cpp - the Pat-server file fetcher `NetworkFileFetcher`.
 *
 * SECTIONS. extab 0x8001B790..0x8001B850; extabindex 0x8003B910..0x8003B9D0; .text 0x803F6524..0x803F73C0;
 *   .data 0x805FC848..0x805FC8A8 (`onReply`'s jump table, then the class table 0x805FC880); .sdata
 *   0x80793950..0x80793958 (the "%d" format).
 *
 * WHAT IT IS. `NetworkFileFetcher` (table 0x805FC880): `open`/`read`/`list`/`close`/`queryChecksum` start a command
 *   that `poll` steps, sending `sendReqBinaryHead`/`Data`/`Foot` and the checksum request and waiting for the reply
 *   bits `onReply` (the Pat callback slot 2, through `fileFetcherReplyCallback`) sets.  `Network/NetworkFileFetcher.h`
 *   declares it with the classes of its three neighbour units: `NetworkFetcherBase` (`Network/NetworkFetcherBase.cpp`),
 *   `NetworkNullFetcher` (`Network/NetworkNullFetcher.cpp`) and `NetworkSocketBase` (`Network/NetworkSocketBase.cpp`).
 *   Every member name except `NetworkFileFetcher` and its constructor (the map's) is a GUESS from the bodies: the
 *   command numbers, the "unsupported" `write`/`remove` slots, the reply bits.
 *
 * WHY IT SITS HERE. the network pilot round 3 recut of the phase 4 stub `Network/NetworkCommunityPat.cpp`, cut four
 *   ways at the `.data` seams (request net3-c-47f5#1): a V->D seam at 0x805FC848 and zigzag seams at 0x805FC8A8 and
 *   0x805FC8D0 - one TU emits its jump tables first and its class tables in reverse.
 *
 * FLAGS. `cflags_network` with `-O3` like the session and transport siblings; file-scope `#pragma peephole off`
 *   (retail's unfused `extsh`+`cmpwi` of the deleting flag and `clrlwi`+`cmpwi` of the bit tests).
 *
 * RESIDUALS. 19 of 20 rows at 100 %.  `onReply` 99.62: the read reply's `transferred_20` copy sits in r4 and
 *   the available byte count in r7 where retail has them the other way round (all 24 declaration orders of the
 *   locals measured; dropping the `end` local put `end` in retail's r0).  The error code 0x800A0004 is relocated in the
 *   target (dtk reads it as `fn_8009F85C+0x7A8`); the `block_relocations` entries in `config.yml` remove those
 *   relocations.  `sendReqBinaryChecksum` (0x80400F28, request op 0x47) is a GUESS name.
 */

#include "Network/NetworkFileFetcher.h"
#include "Network/network_pat_control.h"         /* NetFetchError */
#include "Network/PatInterface.h"                /* PatInterface, setCallback, resetCallback */
#include "enemy/em020_ai.h"                      /* getInstance_ */
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "MSL_C/alloc.h"                         /* sscanf, snprintf */

#pragma peephole off
/* Retail calls every helper (the step functions, `setError`) rather than folding it into its caller. */
#pragma dont_inline on

/* The Pat callback slot 2: hands the reply to the fetcher installed with it. */
static void fileFetcherReplyCallback(u32 code, s32 requestId, s32 b, s32 c, const u32* message,
                                     NetworkFileFetcher* fetcher)
{
    fetcher->onReply(code, requestId, b, c, message);
}

/* Builds the fetcher: the Pat interface singleton when there is none, its reply callback, an idle command. */
NetworkFileFetcher::NetworkFileFetcher()
{
    if (getInstance_() == NULL) {
        new PatInterface();
    }
    if (getInstance_() != NULL) {
        setCallback(getInstance_(), (void (*)())fileFetcherReplyCallback, this, 2);
    }
    requestId_14 = -1;
    command_18 = 0;
    step_1C = 0;
    fileId_28 = 0;
    fileSize_2C = 0;
    handle_30 = 0;
    buffer_34 = NULL;
    bufferSize_38 = 0;
    offset_3C = 0;
    list_40 = NULL;
    unused_44 = 0;
}

/* Removes the reply callback and closes the file. */
NetworkFileFetcher::~NetworkFileFetcher()
{
    if (getInstance_() != NULL) {
        resetCallback(getInstance_(), 2);
    }
    close();
}

/* Steps the command in flight; 1 while it runs, 0 when it is done, -1 when it failed. */
s32 NetworkFileFetcher::step()
{
    switch (command_18) {
    case 1:
        return stepOpen();
    case 2:
        return stepRead();
    case 3:
        return stepWrite();
    case 4:
        return stepRemove();
    case 5:
        return stepList();
    case 6:
        return stepClose();
    case 7:
        return stepChecksum();
    }
    return 0;
}

/* Steps the command, then reports the bytes moved (or the checksum) and the percentage done. */
s32 NetworkFileFetcher::poll(u32* status)
{
    s32 result = step();

    status[0] = transferred_20;
    status[1] = progress_24;
    return result;
}

/* Starts opening the numbered file `path` names (1..128). */
s32 NetworkFileFetcher::open(s32 flags, const char* path)
{
    s32 id;

    if (command_18 != 0 || getInstance_() == NULL) {
        setError(0x800A0004, 0, 0);
        return -1;
    }
    id = 0;
    if ((u8)flags != 0 || path == NULL || sscanf(path, "%d", &id) != 1 || (u32)(id - 1) > 0x7F) {
        fileId_28 = 0;
        setError(0x800A0003, 0, 0);
        return -1;
    }
    fileId_28 = id;
    fileSize_2C = 0;
    handle_30 = 0;
    setError(0, 0, 0);
    command_18 = 1;
    step_1C = 0;
    return 0;
}

/* Starts reading at most `size` bytes of the open file into `buffer`. */
/* untyped: byte range (the caller's file buffer) */
s32 NetworkFileFetcher::read(void* buffer, s32 size)
{
    if (command_18 != 0 || getInstance_() == NULL) {
        setError(0x800A0004, 0, 0);
        return -1;
    }
    if (buffer == NULL) {
        setError(0x800A0003, 0, 0);
        return -1;
    }
    setError(0, 0, 0);
    command_18 = 2;
    step_1C = 0;
    buffer_34 = (u8*)buffer;
    bufferSize_38 = size;
    offset_3C = 0;
    return 0;
}

/* Writing is not supported. */
s32 NetworkFileFetcher::write()
{
    setError(0x800A0001, 0, 0);
    return -1;
}

/* Starts listing the server's files into `out`. */
s32 NetworkFileFetcher::list(u8* out)
{
    if (command_18 != 0) {
        setError(0x800A0004, 0, 0);
        return -1;
    }
    if (out == NULL) {
        setError(0x800A0003, 0, 0);
        return -1;
    }
    command_18 = 5;
    step_1C = 0;
    list_40 = out;
    return 0;
}

/* Removing is not supported. */
s32 NetworkFileFetcher::remove()
{
    setError(0x800A0001, 0, 0);
    return -1;
}

/* Starts closing the open file. */
s32 NetworkFileFetcher::close()
{
    if (command_18 != 0) {
        setError(0x800A0004, 0, 0);
        return -1;
    }
    command_18 = 6;
    step_1C = 0;
    return 0;
}

/* Starts asking the server for the checksum of the numbered file `path` names (1..128). */
s32 NetworkFileFetcher::queryChecksum(s32 flags, const char* path)
{
    s32 id;

    if (command_18 != 0 || getInstance_() == NULL) {
        setError(0x800A0004, 0, 0);
        return -1;
    }
    id = 0;
    if ((u8)flags != 0 || path == NULL || sscanf(path, "%d", &id) != 1 || (u32)(id - 1) > 0x7F) {
        fileId_28 = 0;
        setError(0x800A0003, 0, 0);
        return -1;
    }
    fileId_28 = id;
    fileSize_2C = 0;
    handle_30 = 0;
    setError(0, 0, 0);
    command_18 = 7;
    step_1C = 0;
    return 0;
}

/* The open command: the head request, then its reply (the handle and the size). */
s32 NetworkFileFetcher::stepOpen()
{
    switch (step_1C) {
    case 0:
        progress_24 = 5;
        transferred_20 = 0;
        step_1C = 3;
        break;
    case 3:
        progress_24 += 5;
        replies_10 = 0;
        requestId_14 = sendReqBinaryHead(getInstance_(), fileId_28, 5);
        step_1C++;
        break;
    case 4:
        if (replies_10 & 1) {
            setError(0x800A0011, 0, 0);
            step_1C = 9;
        } else if (replies_10 & 2) {
            setError(0x800A0012, 0, 0);
            step_1C = 9;
        } else if (replies_10 & 0x80) {
            step_1C++;
        }
        break;
    case 5:
        progress_24 = 100;
        command_18 = 0;
        step_1C = 0;
        return 0;
    case 9:
        command_18 = 0;
        step_1C = 0;
        return -1;
    }
    return 1;
}

/* The read command: data requests of at most 0x2000 bytes until the buffer or the file is exhausted. */
s32 NetworkFileFetcher::stepRead()
{
    u32 remaining;
    u32 size;
    u32 chunk;

    switch (step_1C) {
    case 0:
        progress_24 = 5;
        transferred_20 = 0;
        if (fileSize_2C == 0) {
            step_1C = 4;
        } else if (fileSize_2C < offset_3C) {
            step_1C = 4;
        } else {
            step_1C++;
        }
        break;
    case 1:
        progress_24 += 5;
        remaining = fileSize_2C - offset_3C;
        size = bufferSize_38;
        chunk = (remaining < size ? remaining : size) - transferred_20;
        if (chunk > 0x2000) {
            chunk = 0x2000;
        }
        replies_10 = 0;
        requestId_14 = sendReqBinaryData(getInstance_(), fileId_28, handle_30, offset_3C + transferred_20, chunk);
        step_1C++;
        break;
    case 2:
        if (replies_10 & 1) {
            setError(0x800A0011, 0, 0);
            step_1C = 9;
        } else if (replies_10 & 2) {
            setError(0x800A0012, 0, 0);
            step_1C = 9;
        } else if (replies_10 & 0x100) {
            step_1C++;
        }
        break;
    case 3:
        if (transferred_20 >= fileSize_2C || transferred_20 >= bufferSize_38) {
            step_1C++;
        } else {
            step_1C = 1;
        }
        break;
    case 4:
        progress_24 = 100;
        command_18 = 0;
        step_1C = 0;
        return 0;
    case 9:
        command_18 = 0;
        step_1C = 0;
        return -1;
    }
    return 1;
}

/* The write command fails at once. */
s32 NetworkFileFetcher::stepWrite()
{
    progress_24 = 0;
    return -1;
}

/* The list command: 128 entries, each named by its number. */
s32 NetworkFileFetcher::stepList()
{
    s32 offset;
    s32 i;

    if (step_1C == 0) {
        *(u32*)list_40 = 0x80;
        i = 0;
        offset = 0;
        do {
            snprintf((char*)list_40 + offset + 4, 0x100, "%d", i);
            offset += 0x100;
            i++;
        } while (i < 0x80);
        progress_24 = 100;
        command_18 = 0;
        step_1C = 0;
        return 0;
    }
    return 1;
}

/* The remove command fails at once. */
s32 NetworkFileFetcher::stepRemove()
{
    progress_24 = 0;
    return -1;
}

/* The close command: the foot request (none when no file is open), then its reply. */
s32 NetworkFileFetcher::stepClose()
{
    switch (step_1C) {
    case 0:
        progress_24 = 5;
        if (fileId_28 == 0) {
            step_1C = 2;
        } else {
            replies_10 = 0;
            requestId_14 = sendReqBinaryFoot(getInstance_(), fileId_28);
            step_1C++;
        }
        break;
    case 1:
        if (replies_10 & 1) {
            setError(0x800A0011, 0, 0);
            step_1C = 9;
        } else if (replies_10 & 2) {
            setError(0x800A0012, 0, 0);
            step_1C = 9;
        } else if (replies_10 & 0x200) {
            step_1C++;
        }
        break;
    case 2:
        progress_24 = 100;
        command_18 = 0;
        step_1C = 0;
        return 0;
    case 9:
        command_18 = 0;
        step_1C = 0;
        return -1;
    }
    return 1;
}

/* The checksum command: the checksum request, then its reply (reported as the bytes moved). */
s32 NetworkFileFetcher::stepChecksum()
{
    switch (step_1C) {
    case 0:
        progress_24 = 5;
        transferred_20 = 0;
        replies_10 = 0;
        requestId_14 = sendReqBinaryChecksum(getInstance_(), fileId_28);
        step_1C++;
        break;
    case 1:
        if (replies_10 & 1) {
            setError(0x800A0011, 0, 0);
            step_1C = 9;
        } else if (replies_10 & 2) {
            setError(0x800A0012, 0, 0);
            step_1C = 9;
        } else if (replies_10 & 0x40) {
            transferred_20 = handle_30;
            step_1C = 5;
        }
        break;
    case 5:
        progress_24 = 100;
        command_18 = 0;
        step_1C = 0;
        return 0;
    case 9:
        command_18 = 0;
        step_1C = 0;
        return -1;
    }
    return 1;
}

/* Turns a Pat reply into the reply bits the commands wait for, copying the data a read reply carries. */
void NetworkFileFetcher::onReply(u32 code, s32 requestId, s32 b, s32 c, const u32* message)
{
    u32 position;
    u32 start;
    u32 available;
    u32 size;

    switch (code) {
    case 0x8000:
    case 0x8007:
        replies_10 |= 1;
        break;
    case 0x8002:
        if (requestId == requestId_14) {
            replies_10 |= 2;
        }
        break;
    case 0x8006:
        replies_10 |= 1;
        break;
    case 0x8009:
        handle_30 = message[0];
        replies_10 |= 0x40;
        break;
    case 0x800A:
        handle_30 = message[0];
        fileSize_2C = message[1];
        replies_10 |= 0x80;
        break;
    case 0x800B:
        position = offset_3C + transferred_20;
        start = message[1];
        if (start + message[2] <= position || position < start) {
            replies_10 |= 2;
            break;
        }
        available = start + message[2] - position;
        size = bufferSize_38 - transferred_20;
        if (available < size) {
            size = available;
        }
        if (size != 0 && buffer_34 != NULL) {
            memcpy(buffer_34 + transferred_20, (const u8*)message[3] + (position - start), size);
        }
        transferred_20 += size;
        replies_10 |= 0x100;
        break;
    case 0x800C:
        replies_10 |= 0x200;
        break;
    }
}
