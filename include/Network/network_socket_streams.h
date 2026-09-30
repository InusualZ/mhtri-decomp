/*
 * include/Network/network_socket_streams.h - the symbols `Network/network_socket_streams.cpp` owns that the rest of the Network band calls
 * (the socket users, the byte stream and the Pat manager's two pumps).
 *
 * The declarations moved out of `Network/network_transport.h` when `Network/network_transport.cpp` was split
 * into one unit per translation unit (docs/network-transport-split.md): a declaration belongs with the TU that
 * defines the symbol (rule 2).  The types they use are `Network/network_transport_types.h`'s.
 */

#ifndef NETWORK_NETWORK_SOCKET_STREAMS_H
#define NETWORK_NETWORK_SOCKET_STREAMS_H

#include "Network/network_transport_types.h"

extern "C" {

s32 getAvailableToRead(NetworkSocketUser* self);
s32 networkPeer_getAvailableToRead(NetworkSocketUser* self);
s32 networkPeer_closeSocket(NetworkSocketUser* self);
s32 networkPeer_clearReceiveSocket(NetworkSocketUser* self);
void networkPeer_clearReceiveBuffer(NetworkSingleTcp* self);
s32 networkPeer_openSocket(NetworkSingleTcp* self, const NetworkPeerAddress* address);
void networkPeer_release(NetworkSingleTcp* self);
void networkPeer_releaseSocket(NetworkSingleTcp* self);
void networkPeer_disconnect(NetworkSingleTcp* self);
void networkPeer_disconnectSocket(NetworkSingleTcp* self);
s32 NetworkSingleTcp_add(NetworkSingleTcp* self, NetworkPeerMcs* peer);
void NetworkSingleTcp_remove(NetworkSingleTcp* self, NetworkPeerMcs* peer);
s32 NetworkSingleTcp_send(NetworkSingleTcp* self, const u8* data, s32 size);
s32 NetworkSingleTcp_getError(NetworkSingleTcp* self);
void NetworkMultipleUdp_remove(NetworkMultipleUdp* self, const NetworkPeerAddress* address);
void NetworkMultipleUdp_reset(NetworkMultipleUdp* self, s32 peerIndex);
s32 NetworkMultipleUdp_send(NetworkMultipleUdp* self, s32 peerIndex, const u8* data, s32 size);
s32 NetworkMultipleUdp_receive(NetworkMultipleUdp* self, s32 peerIndex, u8* out, s32 capacity);
s32 NetworkMultipleUdp_getError(NetworkMultipleUdp* self);
void networkPeerStream_putByte(NetworkByteStream* self, u8 value);
void networkPeerStream_forwardRecord(NetworkByteStream* self, NetworkStreamSink* sink);
void networkPeerStream_pullRecord(NetworkByteStream* self, NetworkStreamSink* sink);
void networkPeerStream_putRecord(NetworkByteStream* self, const NetworkPeerRecord* record);
void networkPeerStream_putU16(NetworkByteStream* self, u16 value);
void networkPeerStream_putU32(NetworkByteStream* self, u32 value);
void networkPeerStream_takeByte(NetworkByteStream* self, u8* out);
void networkPeerStream_takeU32(NetworkByteStream* self, u32* out);
void networkPeerStream_takeRecord(NetworkByteStream* self, NetworkPeerRecord* record);
void networkPeerStream_readLength(NetworkByteStream* self, u16* out);
u8* networkPeer_getSocket(NetworkByteStream* self);
u32 networkPeer_getPeerId(NetworkByteStream* self);
void receivePatInterfaces(PatReceiver* receiver);
void flushPatRequests(PatRequestQueue* queue);
}

#endif /* NETWORK_NETWORK_SOCKET_STREAMS_H */
