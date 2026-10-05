/*
 * Network/NetworkNullFetcher.cpp - the kind-2 file fetcher `NetworkNullFetcher`, whose operations all succeed at once.
 *
 * SECTIONS. extab 0x8001B850..0x8001B87C; extabindex 0x8003B9D0..0x8003B9F4; .text 0x803F73C0..0x803F74D8;
 *   .data 0x805FC8A8..0x805FC8D0 (the class table).
 *
 * WHAT IT IS. The fetcher `sNetworkLibraryWii::createFetcher` builds for kind 2 (declared in
 *   `Network/NetworkFileFetcher.h`): every slot returns 0 and `open` clears the error record.  The class name is a
 *   GUESS from the bodies.
 *
 * WHY IT SITS HERE. Cut out of `Network/NetworkFileFetcher.cpp` (request net3-c-47f5#1): the `.data` zigzag seam at
 *   0x805FC8A8 (this table follows the fetcher's going up, where one TU emits its tables in reverse).
 *
 * FLAGS. `cflags_network` with `-O3` like the fetcher; `#pragma peephole off` and `#pragma dont_inline on` as there.
 */

#include "Network/NetworkFileFetcher.h"

#pragma peephole off
#pragma dont_inline on

/* Builds the kind-2 fetcher over the fetcher base. */
NetworkNullFetcher::NetworkNullFetcher()
{
}

/* Destroys the kind-2 fetcher, closing it first. */
NetworkNullFetcher::~NetworkNullFetcher()
{
    close();
}

/* Nothing is ever in flight. */
s32 NetworkNullFetcher::poll(u32* status)
{
    return 0;
}

/* Opening succeeds at once. */
s32 NetworkNullFetcher::open(s32 flags, const char* path)
{
    setError(0, 0, 0);
    return 0;
}

/* Reading succeeds at once. */
/* untyped: byte range (the caller's file buffer) */
s32 NetworkNullFetcher::read(void* buffer, s32 size)
{
    return 0;
}

/* Writing succeeds at once. */
s32 NetworkNullFetcher::write()
{
    return 0;
}

/* Listing succeeds at once. */
s32 NetworkNullFetcher::list(u8* out)
{
    return 0;
}

/* Removing succeeds at once. */
s32 NetworkNullFetcher::remove()
{
    return 0;
}

/* Closing succeeds at once. */
s32 NetworkNullFetcher::close()
{
    return 0;
}
