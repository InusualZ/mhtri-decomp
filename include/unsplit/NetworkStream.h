/*
 * Network stream and socket helpers with no registered owner (docs/plan.md 6.5 rule 2) - the packet
 * reader/writer band at 0x803F7540..0x803FAE00 and the small socket-handle accessors the transport
 * band (the Network transport units, `Network/network_transport.h`) calls.
 *
 * Every address is outside a registered range, so no owner header exists and the declarations live here,
 * with the band.  The stream object is the `NetworkStreamWriterDefault` local the session band reserves
 * (`Network/NetworkSessionManager.h`); only a forward declaration is needed here.  Names come from what each body
 * does and what its callers pass (GUESS marks are on the ones carrying no string or dump evidence).
 *
 * Added with the transport body pass.
 */
#ifndef MHTRI_UNSPLIT_NETWORKSTREAM_H
#define MHTRI_UNSPLIT_NETWORKSTREAM_H

#include "types.h"

struct NetworkStreamWriterDefault;
struct NetworkStreamWriter;
struct NetworkSmallObject;
class NetworkStreamQueue;
class NetworkSocketHandle;

extern "C" {

/* ---- the packet (the 0x20-byte `NetworkStreamWriter` local) and the message queue it is read from ----
 * The session unit (`Network/NetworkSessionStable.cpp`) builds a packet on its stack, binds it to a block
 * and walks the framed messages in it; each queue stores such messages.  GUESS on every name: each is read
 * off the body and the call that reaches it. */

/* 0x803F89D0 - binds the packet to `size` bytes at `buffer`. */
/* untyped: byte range (the block the packet reads or writes) */
void networkPacket_attach(NetworkStreamWriter* self, const void* buffer, u32 size);

}

#endif /* MHTRI_UNSPLIT_NETWORKSTREAM_H */
