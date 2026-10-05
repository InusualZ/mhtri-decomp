/*
 * Network/NetworkFetcherBase.cpp - the abstract file fetcher `NetworkFetcherBase` and its error record.
 *
 * SECTIONS. extab 0x8001B780..0x8001B790; extabindex 0x8003B8F8..0x8003B910; .text 0x803F6458..0x803F6524;
 *   .data 0x805FC820..0x805FC848 (the class table: a destructor and six empty slots).
 *
 * WHAT IT IS. The interface `NetworkFileFetcher` and `NetworkNullFetcher` derive from (declared in
 *   `Network/NetworkFileFetcher.h`): the error triple at +0x04 that `copyError` hands out and `setError` fills
 *   once.  The class name is a GUESS from the bodies; `copyError` is the map's.
 *
 * WHY IT SITS HERE. Cut out of `Network/NetworkFileFetcher.cpp` (request net3-c-47f5#1): the `.data` V->D seam at
 *   0x805FC848 (this table, then `onReply`'s jump table) - MWCC emits a TU's jump tables before its class tables,
 *   so one object cannot produce retail's base table, jump table, fetcher table order.
 *
 * FLAGS. `cflags_network` with `-O3` like the fetcher; `#pragma peephole off` and `#pragma dont_inline on` as there.
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
