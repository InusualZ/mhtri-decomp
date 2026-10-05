/*
 * Network/NetworkSocketWii.cpp - the Wii socket `NetworkSocketWii` over the SO library (SSL for mode 4), and the
 *   two error accessors the socket streams poll.
 *
 * SECTIONS. extab 0x8001B884..0x8001B918; extabindex 0x8003BA00..0x8003BAC0; .text 0x803F7538..0x803F84B8;
 *   .data 0x805FC920..0x805FC9D0 (the two log strings and the table 0x805FC984); .sdata2 0x8079C7B8..0x8079C7C0
 *   (the 120-second SSL handshake limit).
 *
 * WHAT IT IS. The "NetworkSocketWii::init()"/"::open()" strings name the class; it implements the sixteen slots of
 *   `NetworkSocketBase` (`Network/NetworkSocketBase.cpp`): `open` creates the SO socket (stream for modes 1/4,
 *   datagram for 2) and sets it non-blocking with its options, `openSecure` adds the SSL context, `connect` /
 *   `pollConnect` run the non-blocking connect (and the SSL handshake), `listen`/`accept` the server side,
 *   `send`/`receive` move the bytes.  Failures leave an error source (0x8001000x) and the SO/SSL result in the base's
 *   error words.  Slot, field and helper names are GUESSes from the bodies; the SO callees are the RVL SO API in
 *   its own order (the map rows renamed in this change: `SOSocket` .. `SOPoll`).
 *
 * WHY IT SITS HERE. the network pilot round 3 recut of the phase 4 stub `Network/NetworkCommunityPat.cpp`.  Left edge
 *   0x803F7538: the two 8-byte accessors read the base's error words and carry no extab record, so nothing pins
 *   them to the socket base before them (request net3-c-47f5#1 offers both placements).
 *
 * FLAGS. `cflags_network` with `-O3` like the transport siblings; file-scope `#pragma peephole off` (retail's
 *   unfused `extsh`+`cmpwi` and bit tests) and `#pragma dont_inline on` (retail calls `setOptions` and
 *   `getErrorSource`).
 *
 * RESIDUALS. Every `.text` row matches.  The error
 *   sources 0x80010001..0x80010015 are relocated in the target (dtk reads them as `@etb_8000FFFC+N` and friends);
 *   the `block_relocations` entries in `config.yml` remove those relocations.  `SOiGetLastError` (0x8051F110, the
 *   thread error accessor) is a GUESS name.
 */

#include "Network/NetworkSocketWii.h"
#include "unsplit/Network.h"                     /* getNetworkLogger - no registered owner */
#include "unsplit/SO.h"                          /* SOHtoNs */
#include "SO/soi.h"                              /* SOSocket .. SOPoll, SOSockAddrIn, SOPollFD, SOiGetLastError */
#include "SSL/ssl.h"                             /* SSLNew, SSLConnect, SSLDoHandshake, SSLRead, SSLWrite, ... */
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"

#pragma peephole off
#pragma dont_inline on

extern "C" {

/* The socket's error source (its +0x04 word). */
/* untyped: opaque handle passed through - the transport users hold the socket through their own view */
s32 getBytesAvailableToRead(void* handle)
{
    return ((NetworkSocketBase*)handle)->errorSource_04;
}

/* The socket's error code (its +0x08 word). */
s32 networkSocketHandle_getLastError(NetworkSocketHandle* handle)
{
    return ((NetworkSocketBase*)handle)->errorCode_08;
}

}

/* Builds a closed socket: no descriptor, no mode, no SSL context. */
NetworkSocketWii::NetworkSocketWii()
{
    socket_0C = -1;
    connected_10 = 0;
    mode_14 = 0;
    sslContext_18 = 0;
    connectStep_1C = 0;
}

/* Closes the socket on the way out. */
NetworkSocketWii::~NetworkSocketWii()
{
    shutdown();
}

/* Makes the socket non-blocking and sets its options: keep-alive, and for a stream socket no-delay and the two
   buffer sizes. */
s32 NetworkSocketWii::setOptions()
{
    u32 size;
    u32 value;
    s32 flags;

    flags = SOFcntl(socket_0C, 3, 0);
    if (SOFcntl(socket_0C, 4, flags | 4) < 0) {
        return -1;
    }
    value = 1;
    if (SOSetSockOpt(socket_0C, 0xFFFF, 4, &value, sizeof(value)) < 0) {
        return -1;
    }
    if (mode_14 == 1 || mode_14 == 4) {
        value = 1;
        if (SOSetSockOpt(socket_0C, 6, 0x2001, &value, sizeof(value)) < 0) {
            return -1;
        }
        size = 0x1000;
        if (SOSetSockOpt(socket_0C, 0xFFFF, 0x1001, &size, sizeof(size)) < 0) {
            return -1;
        }
        size = 0x2000;
        if (SOSetSockOpt(socket_0C, 0xFFFF, 0x1002, &size, sizeof(size)) < 0) {
            return -1;
        }
    }
    return 0;
}

/* The error source a failed SO/SSL call leaves. */
u32 NetworkSocketWii::getErrorSource(s32 code)
{
    return 0x80010000;
}

/* Opens the socket in `mode` (1 TCP, 2 UDP, 4 TCP under SSL). */
s32 NetworkSocketWii::open(s32 mode)
{
    BOOL stream;
    s32 fd;

    if (socket_0C >= 0 || sslContext_18 > 0) {
        errorCode_08 = 0;
        errorSource_04 = 0x80010002;
        return -1;
    }
    errorSource_04 = 0;
    errorCode_08 = 0;
    connected_10 = 0;
    mode_14 = mode;
    stream = TRUE;
    if (mode != 1 && mode != 4) {
        stream = FALSE;
    }
    fd = SOSocket(2, stream ? 1 : 2, 0);
    socket_0C = fd;
    if (fd < 0) {
        socket_0C = -1;
        errorCode_08 = fd;
        errorSource_04 = 0x80010011;
        return -1;
    }
    if (setOptions() < 0) {
        close();
        errorCode_08 = SOiGetLastError();
        errorSource_04 = 0x80010012;
        return -1;
    }
    return 0;
}

/* Opens a stream socket and the SSL context for `host`, with the root certificate when one is given. */
s32 NetworkSocketWii::openSecure(const char* host, const u8* rootCA, s32 rootCASize)
{
    s32 ssl;
    s32 result;

    if (open(4) < 0) {
        return -1;
    }
    getNetworkLogger()->signal_0C(1, "NetworkSocketWii::init(): socket conect SSL\n");
    ssl = SSLNew(4, (char*)host);
    sslContext_18 = ssl;
    if (ssl < 0) {
        sslContext_18 = 0;
        errorCode_08 = ssl;
        errorSource_04 = 0x80010000;
        return -1;
    }
    if (rootCASize > 0) {
        result = SSLSetRootCA(ssl, (u32)rootCA, rootCASize);
        if (result < 0) {
            errorCode_08 = result;
            errorSource_04 = 0x80010000;
            return -1;
        }
    }
    return 0;
}

/* Shuts the socket down by closing it. */
s32 NetworkSocketWii::shutdown()
{
    return close();
}

/* Starts the non-blocking connect to `address` (a would-block result is not a failure). */
s32 NetworkSocketWii::connect(const NetworkPeerAddress* address)
{
    SOSockAddrIn sa;
    s32 result;

    if (mode_14 == 2) {
        errorCode_08 = 0;
        errorSource_04 = 0x80010001;
        return -1;
    }
    getNetworkLogger()->signal_0C(1, "NetworkSocketWii::open(): IP[%d:%d:%d:%d] PORT[%d]\n", address->ip_00[0],
                                  address->ip_00[1], address->ip_00[2], address->ip_00[3], address->port_04);
    memset(&sa, 0, sizeof(sa));
    sa.len = 8;
    sa.family = 2;
    memcpy(&sa.addr, address->ip_00, 4);
    sa.port = SOHtoNs(address->port_04);
    connectStep_1C = 0;
    result = SOConnect(socket_0C, &sa);
    if (result < 0) {
        if (result == -26) {
            return 0;
        }
        errorCode_08 = result;
        errorSource_04 = getErrorSource(result);
        return -1;
    }
    return 0;
}

/* Steps the connect: polls the socket, then runs the SSL connect and handshake for mode 4; 1 once connected,
   0 while waiting, -1 on failure (the handshake gives up after 120 seconds). */
s32 NetworkSocketWii::pollConnect()
{
    SOPollFD poll;
    s32 result;

    if (mode_14 == 2) {
        errorCode_08 = 0;
        errorSource_04 = 0x80010001;
        return -1;
    }
    if (mode_14 == 1 || mode_14 == 4) {
        switch (connectStep_1C) {
        default:
            return -1;
        case 0:
            if (socket_0C < 0) {
                return -1;
            }
            poll.fd = socket_0C;
            poll.events = 0x1F;
            poll.revents = 0;
            result = SOPoll(&poll, 1, 0);
            if (result < 0) {
                if (result == -10) {
                    return 0;
                }
                errorCode_08 = result;
                errorSource_04 = getErrorSource(result);
                return -1;
            }
            if ((poll.revents & 0x20) || ((poll.revents & 1) && (poll.revents & 8))) {
                errorCode_08 = 0;
                errorSource_04 = 0x80010000;
                return -1;
            }
            if (mode_14 == 4) {
                if (sslContext_18 <= 0) {
                    return -1;
                }
                result = SSLConnect(sslContext_18, socket_0C);
                if (result < 0) {
                    errorCode_08 = result;
                    errorSource_04 = 0x80010000;
                    return -1;
                }
                handshakeStart_20 = getNetworkLogger()->getTime_60();
                connectStep_1C++;
                return 0;
            }
            break;
        case 1:
            if (sslContext_18 <= 0) {
                return -1;
            }
            result = SSLDoHandshake(sslContext_18);
            if (result < 0) {
                if (result == -2 || result == -3 || result == -7) {
                    if (getNetworkLogger()->getTime_60() - handshakeStart_20 > 120.0f) {
                        errorCode_08 = 0;
                        errorSource_04 = 0x80010000;
                        return -1;
                    }
                    return 0;
                }
                errorCode_08 = result;
                errorSource_04 = getErrorSource(result);
                return -1;
            }
            break;
        }
    }
    connected_10 = 1;
    return 1;
}

/* Binds the socket to the port of `address` on any local address, and listens on a TCP socket. */
s32 NetworkSocketWii::listen(const NetworkPeerAddress* address)
{
    SOSockAddrIn sa;
    s32 result;

    if (mode_14 == 4) {
        errorCode_08 = 0;
        errorSource_04 = 0x80010001;
        return -1;
    }
    memset(&sa, 0, sizeof(sa));
    sa.len = 8;
    sa.family = 2;
    sa.addr = DWCi_peerTokenFromSocket(0);
    sa.port = SOHtoNs(address->port_04);
    result = SOBind(socket_0C, &sa);
    if (result < 0) {
        errorCode_08 = result;
        errorSource_04 = getErrorSource(result);
        return -1;
    }
    if (mode_14 == 1) {
        result = SOListen(socket_0C, 3);
        if (result < 0) {
            errorCode_08 = result;
            errorSource_04 = getErrorSource(result);
            return -1;
        }
    }
    connected_10 = 1;
    return 0;
}

/* Accepts a connection on a listening TCP socket. */
s32 NetworkSocketWii::accept()
{
    SOSockAddrIn sa;
    s32 result;

    if (mode_14 == 2 || mode_14 == 4) {
        errorCode_08 = 0;
        errorSource_04 = 0x80010001;
        return -1;
    }
    memcpy(&sa, NULL, sizeof(sa));
    sa.len = 8;
    result = SOAccept(socket_0C, &sa);
    if (result < 0) {
        errorCode_08 = result;
        errorSource_04 = getErrorSource(result);
        return -1;
    }
    return 0;
}

/* Whether the socket is connected (or bound). */
s32 NetworkSocketWii::isConnected()
{
    return connected_10;
}

/* The socket's own address and port. */
s32 NetworkSocketWii::getLocalAddress(NetworkPeerAddress* out)
{
    SOSockAddrIn sa;
    s32 result;

    if (isConnected() == 0) {
        errorCode_08 = 0;
        errorSource_04 = 0x80010013;
        return -1;
    }
    memcpy(&sa, NULL, sizeof(sa));
    sa.len = 8;
    result = SOGetSockName(socket_0C, &sa);
    if (result < 0) {
        errorCode_08 = result;
        errorSource_04 = getErrorSource(result);
        return -1;
    }
    memcpy(out->ip_00, &sa.addr, 4);
    out->port_04 = sa.port;
    return 0;
}

/* The connected peer's address and port (not for a UDP socket). */
s32 NetworkSocketWii::getPeerAddress(NetworkPeerAddress* out)
{
    SOSockAddrIn sa;
    s32 result;

    if (mode_14 == 2) {
        errorCode_08 = 0;
        errorSource_04 = 0x80010001;
        return -1;
    }
    if (isConnected() == 0) {
        errorCode_08 = 0;
        errorSource_04 = 0x80010013;
        return -1;
    }
    memcpy(&sa, NULL, sizeof(sa));
    sa.len = 8;
    result = SOGetPeerName(socket_0C, &sa);
    if (result < 0) {
        errorCode_08 = result;
        errorSource_04 = getErrorSource(result);
        return -1;
    }
    memcpy(out->ip_00, &sa.addr, 4);
    out->port_04 = sa.port;
    return 0;
}

/* Releases the SSL context and closes the descriptor, clearing the socket's state. */
s32 NetworkSocketWii::close()
{
    if (sslContext_18 > 0) {
        SSLShutdown(sslContext_18);
        sslContext_18 = 0;
    }
    if (socket_0C >= 0) {
        SOClose(socket_0C);
        errorSource_04 = 0;
        errorCode_08 = 0;
        socket_0C = -1;
        connected_10 = 0;
        mode_14 = 0;
    }
    return 0;
}

/* Sends `size` bytes (to `address` on a UDP socket); the byte count, or -1 when the send failed or was short. */
s32 NetworkSocketWii::send(const u8* data, s32 size, const NetworkPeerAddress* address)
{
    SOSockAddrIn sa;
    s32 result = -1;

    if (size < 0) {
        errorCode_08 = 0;
        errorSource_04 = 0x80010003;
        return -1;
    }
    if (mode_14 == 2) {
        if (address == NULL) {
            errorCode_08 = 0;
            errorSource_04 = 0x80010003;
            return -1;
        }
        memset(&sa, 0, sizeof(sa));
        sa.len = 8;
        sa.family = 2;
        memcpy(&sa.addr, address->ip_00, 4);
        sa.port = SOHtoNs(address->port_04);
        result = SOSendTo(socket_0C, data, size, 0, &sa);
    }
    if (mode_14 == 1) {
        result = SOSend(socket_0C, (void*)data, size, 0);
    }
    if (mode_14 == 4) {
        result = SSLWrite(sslContext_18, (void*)data, size);
    }
    if (result < 0) {
        errorCode_08 = result;
        errorSource_04 = getErrorSource(result);
        connected_10 = 0;
        return -1;
    }
    if (result != size) {
        errorCode_08 = 0;
        errorSource_04 = 0x80010014;
        connected_10 = 0;
        return -1;
    }
    return result;
}

/* Receives at most `size` bytes (and the sender's address on a UDP socket); 0 when nothing is waiting, the byte
   count, or -1 when the receive failed or a stream was closed by the peer. */
s32 NetworkSocketWii::receive(u8* out, s32 size, NetworkPeerAddress* address)
{
    SOSockAddrIn sa;
    s32 result = -1;

    if (size < 0) {
        errorCode_08 = 0;
        errorSource_04 = 0x80010003;
        return -1;
    }
    if (mode_14 == 2) {
        if (address == NULL) {
            errorCode_08 = 0;
            errorSource_04 = 0x80010003;
            return -1;
        }
        memset(&sa, 0, sizeof(sa));
        sa.len = 8;
        result = SORecvFrom(socket_0C, out, size, 0, &sa);
        if (result == -6) {
            return 0;
        }
    }
    if (mode_14 == 1) {
        result = SORecv(socket_0C, out, size, 0);
        if (result == -6) {
            return 0;
        }
    }
    if (mode_14 == 4) {
        result = SSLRead(sslContext_18, out, size);
        if (result == -2) {
            return 0;
        }
    }
    if (result < 0) {
        errorCode_08 = result;
        errorSource_04 = getErrorSource(result);
        connected_10 = 0;
        return -1;
    }
    if (mode_14 == 2) {
        if (result == 0) {
            return 0;
        }
        memcpy(address->ip_00, &sa.addr, 4);
        address->port_04 = SOAddressToHostPort(sa.port);
    }
    if ((mode_14 == 1 || mode_14 == 4) && result == 0) {
        errorCode_08 = 0;
        errorSource_04 = 0x80010015;
        connected_10 = 0;
        return -1;
    }
    return result;
}

/* The wildcard address (0.0.0.0); the port is left alone. */
void NetworkSocketWii::getAnyAddress(NetworkPeerAddress* out)
{
    u32 any = 0;

    memcpy(out->ip_00, &any, 4);
}

/* The broadcast address (255.255.255.255); the port is left alone. */
void NetworkSocketWii::getBroadcastAddress(NetworkPeerAddress* out)
{
    u32 all = 0xFFFFFFFF;

    memcpy(out->ip_00, &all, 4);
}

/* The mode the socket was opened in. */
s32 NetworkSocketWii::getMode()
{
    return mode_14;
}
