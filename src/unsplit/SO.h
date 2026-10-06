/*
 * unsplit/SO.h - the SO band's include hub: it declares nothing of its own (docs/plan.md 6.5 rule 2).
 *
 * `SOHtoNs`, `SOAddrInfo`, `SOGetAddrInfo`, `SOFreeAddrInfo` and `SOInetAtoN` are `SO/soi.cpp`'s and are declared
 * in `SO/soi.h`; the header is kept so its C and C++ includers reach them unchanged.
 */
#ifndef MHTRI_UNSPLIT_SO_H
#define MHTRI_UNSPLIT_SO_H

#include "types.h"
#include "SO/soi.h"                             /* SOSockAddrIn, SOConnect */

#endif /* MHTRI_UNSPLIT_SO_H */
