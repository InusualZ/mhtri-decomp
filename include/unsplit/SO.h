/*
 * SO declarations with no registered owner (docs/plan.md 6.5 rule 2).
 *
 * The three SDK socket helpers at 0x80520604/0x8052065C/0x80520668 - `SOAddressToString` (a `u32*` address
 * word rendered as text), `SOAddressToHostPort` (a port through the SDK's own conversion) and `SOHtoNs`
 * (host -> network u16) - sit in the SO/SSL band that no registered unit covers.  stylelint's rule 2
 * cannot place them (`resolve` returns `unsplit` with no module: the nearest registered ranges are
 * `NWC24/nwc24_io.c` below and the game-UI band above, different modules); `SOHtoNs`'s own `SO`
 * prefix and its neighbours' socket vocabulary name the library, and no `include/SO/` owner exists,
 * so this file is their home.
 *
 * `SOHtoNs` was declared in `include/unsplit/Network.h` (the Network band's header, which is
 * C++-only because it carries a class); it moved here and that header now includes this one, so the
 * symbol has one home and the DWCi units (plain `.c`) can reach it.
 *
 * Added with the networking conformance pass.
 */
#ifndef MHTRI_UNSPLIT_SO_H
#define MHTRI_UNSPLIT_SO_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80520604 - format the network-order address word at *addr as "a.b.c.d". */
char* SOAddressToString(u32* addr);

/* 0x8052065C - the port conversion `DWCi_sendControlFrame`/`DWCi_sendTo` store into a connection. */
u16 SOAddressToHostPort(u16 port);

/* 0x80520668 - host-to-network 16-bit byte swap. */
u16 SOHtoNs(u16 port);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_UNSPLIT_SO_H */
