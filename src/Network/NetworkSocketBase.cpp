/*
 * Network/NetworkSocketBase.cpp - the abstract socket `NetworkSocketBase` that `NetworkSocketWii` implements: its
 *   constructor and destructor (the error source and code at +0x04/+0x08 start cleared).
 * RANGE. .text 0x803F74D8-0x803F7538 (2 functions); .data 0x805FC8D0-0x805FC920 (the class table: a destructor and
 *   sixteen pure slots), extab, extabindex.  The left edge is the `.data` zigzag seam at 0x805FC8D0; the two 8-byte
 *   error accessors after it (0x803F7538, no extab record) stay in `Network/NetworkSocketWii.cpp`.
 * FLAGS. `-O3` (configure.py) like the fetcher; file-scope `#pragma peephole off` and `#pragma dont_inline on` as there.
 * NAMES. `NetworkSocketBase` is a GUESS from the bodies (declared in `Network/NetworkFileFetcher.h`).
 * RESIDUALS. none in `.text`; `.data` is 0x4C of the claimed 0x50: the table is 19 words (the derived
 *   `__vt__16NetworkSocketWii` is 0x4C too), the last word is the alignment before `Network/NetworkSocketWii.cpp`'s
 *   `.data`.
 */

#include "Network/NetworkFileFetcher.h"

#pragma peephole off
#pragma dont_inline on

/* Builds the socket base: its table, no bytes available and no error. */
NetworkSocketBase::NetworkSocketBase()
{
    errorSource_04 = 0;
    errorCode_08 = 0;
}

/* Destroys the socket base. */
NetworkSocketBase::~NetworkSocketBase()
{
}
