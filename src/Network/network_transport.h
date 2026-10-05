/*
 * Network/network_transport.h - the Network transport band's public header: the shared types plus each
 * unit's own declarations.
 *
 * `Network/network_transport.cpp` (`.text` 0x803CCDF8..0x803D3CE8) is now eight units - `NetworkPeerBase`,
 * `NetworkPeerBuffer`, `NetworkPeerUdp`, `NetworkPeerMcs`, `network_socket_streams`, `NetworkResolverWii`,
 * `NetworkSessionBase` and `NetworkSessionStable` (docs/network-transport-split.md).  The types are
 * `Network/network_transport_types.h`'s, and a symbol's declaration is in its owner's header; this file only
 * gathers them so `Network/NetworkSessionManager.h` and `unsplit/Network.h` keep one include.
 */

#ifndef NETWORK_NETWORK_TRANSPORT_H
#define NETWORK_NETWORK_TRANSPORT_H

#include "Network/network_transport_types.h"
#include "Network/NetworkPeerBase.h"
#include "Network/network_socket_streams.h"
#include "Network/NetworkResolverWii.h"
#include "Network/NetworkSessionBase.h"
#include "Network/NetworkSessionStable.h"

#endif /* NETWORK_NETWORK_TRANSPORT_H */
