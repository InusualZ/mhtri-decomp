/*
 * Network stream and socket helpers with no registered owner (docs/plan.md 6.5 rule 2) - the packet
 * reader/writer band at 0x803F7540..0x803FAE00 and the small socket-handle accessors the transport
 * band (`Network/network_transport.cpp`) calls.
 *
 * Every address is outside a registered range, so no owner header exists and the declarations live here,
 * with the band.  The stream object is the `NetworkStreamWriterDefault` local the session band reserves
 * (`Network/fn_803D3CE8.h`); only a forward declaration is needed here.  Names come from what each body
 * does and what its callers pass (GUESS marks are on the ones carrying no string or dump evidence).
 *
 * Added with the transport body pass.
 */
#ifndef MHTRI_UNSPLIT_NETWORKSTREAM_H
#define MHTRI_UNSPLIT_NETWORKSTREAM_H

#include "types.h"

struct NetworkStreamWriterDefault;
class NetworkSocketHandle;

extern "C" {

/* 0x8045DFA0 - the C library's `rand` (the 0x41C64E6D linear congruential step, top 15 bits). */
s32 rand(void);

/* 0x803F7540 - the last error code the socket handle recorded (its +0x08 word). */
s32 networkSocketHandle_getLastError(NetworkSocketHandle* handle);

/* 0x803F9E04 - binds a stream to the byte block it reads packets from (GUESS: the parent's init, then
 * the block pointer stored at +0x10). */
void networkStreamReader_attach(NetworkStreamWriterDefault* self, u8* block, u32 size);
/* 0x803F9E40 - true when a whole packet sits at the front of the block. */
s32 make_sure_enough_space(NetworkStreamWriterDefault* self);
/* 0x803F9EBC - copies the leading packet out; 0 when it is incomplete, -1 when `capacity` is too small,
 * else the packet's length. */
s32 copy_from_buffer(NetworkStreamWriterDefault* self, u8* out, u32 capacity);
/* 0x803F9F74 - drops the leading packet from the block and returns its length (0 when incomplete). */
s32 networkStreamReader_consumePacket(NetworkStreamWriterDefault* self);
/* 0x803FA0E8 - the length field of the leading packet (its payload size plus the 22-byte header). */
u16 read_size_from_buffer(NetworkStreamWriterDefault* self);

}

#endif /* MHTRI_UNSPLIT_NETWORKSTREAM_H */
