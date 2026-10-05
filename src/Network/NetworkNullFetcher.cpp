/*
 * Network/NetworkNullFetcher.cpp - the kind-2 file fetcher `sNetworkLibraryWii::createFetcher` builds: every slot
 *   returns 0 at once and `open` clears the error record.
 * RANGE. .text 0x803F73C0-0x803F74D8 (9 functions); .data 0x805FC8A8-0x805FC8D0 (the class table), extab, extabindex.
 *   The left edge is the `.data` zigzag seam at 0x805FC8A8 (this table follows the fetcher's going up, where one TU
 *   emits its tables in reverse).
 * FLAGS. `-O3` (configure.py) like the fetcher; file-scope `#pragma peephole off` and `#pragma dont_inline on` as there.
 * NAMES. `NetworkNullFetcher` is a GUESS from the bodies (declared in `Network/NetworkFileFetcher.h`).
 * RESIDUALS. none.
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
