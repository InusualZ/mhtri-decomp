/*
 * Network/NetworkUniqueId.cpp - the network unique id: the 14-byte address record helpers and the `NetworkUniqueId`
 *   stream sink built around one.
 * RANGE. .text 0x803F84B8-0x803F89CC (11 functions); .data 0x805FC9D0-0x805FCC98 (the log strings, then the table
 *   0x805FCC58), extab, extabindex.  Each cut is a function start where the `.data` run, the `.sdata2` pool and the
 *   extab/extabindex tables all change owner together.
 * FLAGS. `-O3 -pool off` (configure.py) like the transport siblings (each log string is addressed with its own
 *   `lis`/`addi`); file-scope `#pragma peephole off` and `#pragma dont_inline on` (retail calls the record helpers from
 *   the members; `-inline auto` folds them).
 * NAMES. From the unit's own log strings: `NetworkUniqueIdIsValid`, `NetworkUniqueIdImportFrom`,
 *   `NetworkUniqueIdExportTo`, `NetworkUniqueIdEquals` (C-style, `this`/`arg` in their messages) and
 *   `NetworkUniqueId::exportTo`/`::equals`; `isValid` and `importFrom` log nothing and are GUESSes; `bind`/`equals`
 *   (the table's +0x1C/+0x2C) are the sink's.
 * RESIDUALS. none in `.text`; `.data` is 0x2C4 B against the claim's 0x2C8 (the claim ends on the 8-aligned start of the
 *   next unit's `.data`), alignment fill (`flipcheck`'s pad rule); the
 *   log strings pair by address, not by name (`@NNN` against the map's `lbl_805FC9D0..`).
 * SHAPES. The destructor (the table's +0x08) is compiled in `Network/NetworkPeerBase.cpp`, whose range holds its only
 *   copy.
 */

#include "Network/NetworkUniqueId.h"
#include "unsplit/Network.h"                     /* getNetworkLogger - no registered owner */
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "MSL_C/alloc.h"                         /* memcmp - owner MSL_C/alloc.cpp */

#pragma peephole off
/* Retail calls every record helper rather than folding it into the member that uses it. */
#pragma dont_inline on

/* True when the record holds one of the five known address kinds. */
s32 NetworkUniqueIdIsValid(const NetworkUniqueIdData* id)
{
    if (id == NULL) {
        getNetworkLogger()->warn_10("NetworkUniqueIdIsValid: this is null.\n");
        return 0;
    }
    if (id->kind_00 == 0 || id->kind_00 >= 6) {
        return 0;
    }
    return 1;
}

/* Sets the record to `size` bytes (at most ten) of address kind `kind`, warning on a bad argument. */
void NetworkUniqueIdImportFrom(NetworkUniqueIdData* id, u8 kind, const u8* data, u32 size)
{
    if (id == NULL) {
        getNetworkLogger()->warn_10("NetworkUniqueIdImportFrom: this is null.\n");
        return;
    }
    if (data == NULL) {
        getNetworkLogger()->warn_10("NetworkUniqueIdImportFrom: arg->data is null.\n");
        return;
    }
    if (size > sizeof(id->bytes_04)) {
        getNetworkLogger()->warn_10("NetworkUniqueIdImportFrom: this->max < arg->size\n");
        return;
    }
    if (size == 0) {
        getNetworkLogger()->log_14("NetworkUniqueIdImportFrom: arg->size is zero.\n");
        return;
    }
    memset(id, 0, sizeof(*id));
    id->kind_00 = kind;
    id->reserved_01[0] = 0;
    id->reserved_01[1] = 0;
    id->reserved_01[2] = 0;
    memcpy(id->bytes_04, data, size);
}

/* Copies `size` address bytes (at most ten) out of the record, warning on a bad argument. */
void NetworkUniqueIdExportTo(const NetworkUniqueIdData* id, u8* out, u32 size)
{
    if (id == NULL) {
        getNetworkLogger()->warn_10("NetworkUniqueIdExportTo: this is null.\n");
        return;
    }
    if (out == NULL) {
        getNetworkLogger()->warn_10("NetworkUniqueIdExportTo: arg->data is null.\n");
        return;
    }
    if (size > sizeof(id->bytes_04)) {
        getNetworkLogger()->warn_10("NetworkUniqueIdExportTo: this->len < arg->size\n");
        return;
    }
    memcpy(out, id->bytes_04, size);
}

/* True when both records are of the same kind (1..4) and all 14 bytes agree. */
BOOL NetworkUniqueIdEquals(const NetworkUniqueIdData* a, const NetworkUniqueIdData* b)
{
    s32 kind;

    if (a == NULL) {
        getNetworkLogger()->warn_10("NetworkUniqueIdEquals: arg1 is null.\n");
        return FALSE;
    }
    if (b == NULL) {
        getNetworkLogger()->warn_10("NetworkUniqueIdEquals: arg2 is null.\n");
        return FALSE;
    }
    kind = a->kind_00;
    if (kind != (s32)b->kind_00) {
        return FALSE;
    }
    switch (kind) {
    case 1:
    case 2:
    case 3:
    case 4:
        return memcmp(a, b, sizeof(*a)) == 0;
    }
    return FALSE;
}

/* Builds the sink and binds it to the id's own empty record. */
NetworkUniqueId::NetworkUniqueId()
{
    attach((u8*)&data_10, sizeof(data_10));
}

/* True when the id is bound and its record holds a known kind. */
s32 NetworkUniqueId::isValid() const
{
    if (data_04 == NULL) {
        return 0;
    }
    return NetworkUniqueIdIsValid(&data_10);
}

/* Re-binds the record, marks it full and fills it with `size` address bytes of kind `kind`. */
void NetworkUniqueId::importFrom(u8 kind, const u8* data, u32 size)
{
    attach((u8*)&data_10, sizeof(data_10));
    used_0C = capacity_08;
    NetworkUniqueIdImportFrom(&data_10, kind, data, size);
}

/* Copies the id's address bytes out, warning when the id was never bound. */
void NetworkUniqueId::exportTo(u8* out, u32 size) const
{
    if (data_04 == NULL) {
        getNetworkLogger()->warn_10("NetworkUniqueId::exportTo: this is not initialized.\n");
        return;
    }
    NetworkUniqueIdExportTo(&data_10, out, size);
}

/* True when both ids are bound and their records agree. */
u32 NetworkUniqueId::equals(const NetworkUniqueId* other) const
{
    if (data_04 == NULL) {
        getNetworkLogger()->warn_10("NetworkUniqueId::equals: this is not initialized.\n");
        return 0;
    }
    if (other == NULL) {
        getNetworkLogger()->warn_10("NetworkUniqueId::equals: arg is null.\n");
        return 0;
    }
    if (other->data_04 == NULL) {
        getNetworkLogger()->warn_10("NetworkUniqueId::equals: arg is not initialized.\n");
        return 0;
    }
    return NetworkUniqueIdEquals(&data_10, &other->data_10);
}

/* Forwards to the sink's own `bind`. */
void NetworkUniqueId::bind(u8* block, u32 size)
{
    NetworkStreamSink::bind(block, size);
}

/* Forwards to the sink's byte comparison. */
u32 NetworkUniqueId::equals(const NetworkStreamSink* other) const
{
    return NetworkStreamSink::equals(other);
}
