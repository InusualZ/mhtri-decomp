/*
 * Network/NetworkPeerBase.cpp - the abstract peer, its `networkPeerError_*` record accessors and `NetworkUniqueId`'s
 *   deleting destructor.
 * RANGE. .text 0x803CCDF8-0x803CCF30 (6 functions); .data 0x805F94E0-0x805F9510 (the class table), extab, extabindex.
 *   One TU of the transport band; the cuts and their confidence: docs/network.md.
 * FLAGS. `-O3 -pool off` (configure.py; measured in docs/network.md); file-scope `#pragma peephole off`
 *   (playbook 39).
 * NAMES. `NetworkPeerBase` and the `networkPeerError_*` accessors are GUESSes from behaviour (marked in
 *   `Network/NetworkPeerBase.h`).
 * RESIDUALS. none.
 * SHAPES. The table is emitted from `NetworkPeerBase::destroy`, the key function (rule 10).  `NetworkUniqueId`'s
 *   deleting destructor (its table's +0x08 slot) is defined here: the range holds its only copy, at the left edge,
 *   a discovery cap.  Each `dont_inline` region keeps a retail `bl` that `-inline auto` folds.
 */
#include "types.h"
#include "Network/NetworkSessionBase.h"
#include "Network/NetworkResolverWii.h"
#include "Network/NetworkPeerBase.h"
#include "Network/network_socket_streams.h"
#include "Network/NetworkSessionStable.h"
#include "Network/NetworkSessionManager.h"
#include "Network/NetworkUniqueId.h"
/* `unsplit/Network.h` is the Network band's code half (`getNetworkLogger` and the socket-pool helpers); the OS
   thread calls it reaches are `NAND/nand.h`'s, the one spelling `unsplit/OS.h` includes too. */
#include "unsplit/Network.h"
#include "unsplit/Runtime.PPCEABI.H.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"

/* The range keeps retail's unfused forms: a separate `extsh`+`cmpwi` before the free, and the memset argument setup
   in source order (`addi` before the two `li`s) - the peephole pass folds both.  Scoped off for the file. */
#pragma peephole off

extern "C" {

/* Deleting destructor of the unique id: runs the sink's destructor, then frees the object when the caller asks
   for it. */
NetworkUniqueId::~NetworkUniqueId()
{
}

#pragma dont_inline on

/* Builds the error-record base: its table, then an empty record. */
NetworkPeerBase::NetworkPeerBase()
{
    memset(&source_04, 0, 0xC);
}

#pragma dont_inline off

/* Deleting destructor of the abstract peer: frees on request, nothing to chain. */
NetworkPeerBase* NetworkPeerBase::destroy(s16 flags)
{
    if (this != NULL) {
        if (flags > 0) {
            operator delete(this);
        }
    }
    return this;
}

/* Copies the three-word record out when the caller supplies somewhere to put it. */
void networkPeerError_get(NetworkPeerBase* self, NetworkPeerErrorRecord* out)
{
    if (out != NULL) {
        out->source = self->source_04;
        out->argument = self->argument_08;
        out->code = self->code_0C;
    }
}

/* Empties the record so the next failure can fill it. */
void networkPeerError_clear(NetworkPeerBase* self)
{
    memset(&self->source_04, 0, 0xC);
}

/* Records the first failure only: a record that is already filled is left alone. */
/* untyped: opaque handle passed through - the callers hand the peer they already hold */
void networkPeerError_set(void* self, const void* source, u32 argument, s32 code)
{
    NetworkPeerBase* error = (NetworkPeerBase*)self;

    if (error->source_04 == NULL) {
        error->source_04 = source;
        error->argument_08 = argument;
        error->code_0C = code;
    }
}

}
