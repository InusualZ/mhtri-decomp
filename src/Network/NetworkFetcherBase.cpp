/*
 * Network/NetworkFetcherBase.cpp - the abstract file fetcher `NetworkFetcherBase` that `NetworkFileFetcher` and
 *   `NetworkNullFetcher` derive from: the error triple at +0x04 that `copyError` hands out and `setError` fills once.
 * RANGE. .text 0x803F6458-0x803F6524 (4 functions); .data 0x805FC820-0x805FC848 (the class table: a destructor and six
 *   empty slots), extab, extabindex.  The right edge is the `.data` V->D seam at 0x805FC848 (this table, then
 *   `onReply`'s jump table): MWCC emits a TU's jump tables before its class tables, so one object cannot produce
 *   retail's base table, jump table, fetcher table order.
 * FLAGS. `-O3` (configure.py) like the fetcher; file-scope `#pragma peephole off` and `#pragma dont_inline on` as there.
 * NAMES. `NetworkFetcherBase` is a GUESS from the bodies (declared in `Network/NetworkFileFetcher.h`); `copyError` is
 *   the map's.
 * RESIDUALS. none.
 */

#include "Network/NetworkFileFetcher.h"
#include "Network/network_pat_control.h"         /* NetFetchError */
#include "Runtime.PPCEABI.H/memset.h"

#pragma peephole off
#pragma dont_inline on

/* Builds the fetcher interface with an empty error record. */
NetworkFetcherBase::NetworkFetcherBase()
{
    memset(&errorCode_04, 0, 0xC);
}

/* Destroys the fetcher interface. */
NetworkFetcherBase::~NetworkFetcherBase()
{
}

/* Copies the error record out when the caller supplies somewhere to put it. */
void NetworkFetcherBase::copyError(NetFetchError* error)
{
    if (error != NULL) {
        error->code_0x0 = errorCode_04;
        error->detail_0x4 = errorDetail_08;
        error->reason_0x8 = errorReason_0C;
    }
}

/* Records the first failure only: a record that is already filled is left alone. */
void NetworkFetcherBase::setError(u32 code, u32 detail, u32 reason)
{
    if (errorCode_04 == 0) {
        errorCode_04 = code;
        errorDetail_08 = detail;
        errorReason_0C = reason;
    }
}
