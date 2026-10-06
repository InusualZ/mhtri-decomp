/*
 * Network/NetworkConnectionStable.h - the free entry points of `Network/NetworkConnectionStable.cpp` (the class itself is
 *   declared in `Network/NetworkSessionStable.h`).
 */
#ifndef MHTRI_NETWORK_NETWORKCONNECTIONSTABLE_H
#define MHTRI_NETWORK_NETWORKCONNECTIONSTABLE_H

#include "types.h"

class NetworkConnectionStable;      /* Network/NetworkSessionStable.h */
class NetworkStreamWriterDefault;   /* Network/NetworkStreamSink.h */

extern "C" {

/* 0x803CBA9C / 0x803CBB98 - the frame writer's entry points on a connection that `NetworkSessionStable` drives
 * (attach a default writer; hand it `length` bytes of channel `kind`, or none). */
void networkStreamWriter_attach(NetworkConnectionStable* connection, NetworkStreamWriterDefault* stream);
void networkStreamWriter_reserve(NetworkConnectionStable* connection, const u8* bytes, u32 length, s8 kind);

}

#endif /* MHTRI_NETWORK_NETWORKCONNECTIONSTABLE_H */
