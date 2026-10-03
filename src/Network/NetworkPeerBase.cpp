/*
 * Network/NetworkPeerBase.cpp - the abstract peer (the class holding the error record every peer constructor
 *   chains), its deleting destructor, the `networkPeerError_*` record accessors and the small object's deleting
 *   destructor.
 *
 * One translation unit of the retail Network transport band, split out of `Network/network_transport.cpp`
 * (docs/network-transport-split.md holds the evidence and the confidence of each cut).  `.text`
 * 0x803CCDF8..0x803CCF30, `.data` 0x805F94E0..0x805F9510, extab 0x800198C8..0x800198E0, extabindex
 * 0x8003A0BC..0x8003A0E0.
 *
 * NAMES.  `NetworkPeerBase`, the `networkPeerError_*` accessors and `NetworkSmallObjectSink` are GUESSed names
 * (derived from behaviour); the sink's deleting destructor sits at the range's left edge, a discovery cap, so its TU is
 * unproven.  Every name here is the map's or a derived one; the derived ones are marked GUESS in
 * `Network/network_transport_types.h`.
 *
 * TABLE.  Its table (0x805F94E0, 0x30 B) is emitted from `NetworkPeerBase::destroy`, the key function (rule 10).
 *
 * FLAGS.  C++ under `cflags_network` (`-Cpp_exceptions on` gives the `extab`), per-unit `-O3`/`-pool off` (`configure.py`);
 * file-scope `#pragma peephole off` (playbook 39); each `dont_inline` region keeps a retail `bl` that `-inline auto` folds.
 *
 * RESIDUALS.  All 6 rows at 100 %; `flipcheck.py` reports READY (`.text`, `.data`, `extab`, `extabindex` byte-
 * identical).
 */
#include "types.h"
#include "Network/network_transport.h"
#include "Network/NetworkSessionManager.h"
/* `unsplit/Network.h` is the Network band's code half (`getNetworkLogger` and the socket-pool helpers).  It
   cannot be included beside `unsplit/OS.h`: the two band headers declare `OSCreateThread`/`OSResumeThread` with
   different signatures and a TU that sees both fails with `(10197) illegal function overloading`. */
#include "unsplit/Network.h"
#include "unsplit/Runtime.PPCEABI.H.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"

/* The range keeps retail's unfused forms: a separate `extsh`+`cmpwi` before the free, and the memset argument setup
   in source order (`addi` before the two `li`s) - the peephole pass folds both.  Scoped off for the file. */
#pragma peephole off

extern "C" {

/* Deleting destructor of the small transport object: runs the sink's destructor, then frees the object when the
   caller asks for it. */
NetworkSmallObjectSink::~NetworkSmallObjectSink()
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
