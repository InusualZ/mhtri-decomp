/*
 * Network/NetworkSocketBase.cpp - the abstract socket `NetworkSocketBase` that `NetworkSocketWii` implements.
 *
 * SECTIONS. extab 0x8001B87C..0x8001B884; extabindex 0x8003B9F4..0x8003BA00; .text 0x803F74D8..0x803F7538;
 *   .data 0x805FC8D0..0x805FC920 (the class table: a destructor and sixteen pure slots).
 *
 * WHAT IT IS. The constructor and destructor of the socket interface (declared in `Network/NetworkFileFetcher.h`):
 *   the error source and code at +0x04/+0x08 start cleared.  The class name is a GUESS from the bodies.
 *
 * WHY IT SITS HERE. Cut out of `Network/NetworkFileFetcher.cpp` (request net3-c-47f5#1): the `.data` zigzag seam at
 *   0x805FC8D0.  The two 8-byte error accessors after it (0x803F7538, no extab record) stay in
 *   `Network/NetworkSocketWii.cpp`, whose range already starts there.
 *
 * FLAGS. `cflags_network` with `-O3` like the fetcher; `#pragma peephole off` and `#pragma dont_inline on` as there.
 *
 * RESIDUALS. Every `.text` row matches.  `.data` 0x4C of the claimed 0x50: the table is 19 words (the derived
 *   `__vt__16NetworkSocketWii` is 0x4C too), the last word is the alignment before `Network/NetworkSocketWii.cpp`'s `.data`.
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
