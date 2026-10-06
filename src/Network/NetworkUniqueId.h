/*
 * Network/NetworkUniqueId.h - the network unique id `Network/NetworkUniqueId.cpp` defines: a stream sink bound to its
 *   own 14-byte address record.
 */
#ifndef MHTRI_NETWORK_NETWORKUNIQUEID_H
#define MHTRI_NETWORK_NETWORKUNIQUEID_H

#include "types.h"
#include "Network/NetworkStreamSink.h"         /* NetworkStreamSink */

/* The raw address a unique id carries: its kind (1..5 are known kinds) and up to ten address bytes.  The
   helpers clear, compare and bind exactly 14 bytes, the record's size. */
struct NetworkUniqueIdData {
    /* +0x00 */ u8 kind_00;
    /* +0x01 */ u8 reserved_01[3];
    /* +0x04 */ u8 bytes_04[10];
};   /* size: 0x0E (evidence: the memset/memcmp length and the `attach` capacity) */

/* The unique id (the log strings name `NetworkUniqueId::exportTo` and `NetworkUniqueId::equals`): a stream sink
   whose block is its own address record at +0x10.  Table 0x805FCC58 is emitted from this unit (`bind`, defined
   out of line at the end of the unit, is the key function); the destructor's only copy sits at the left edge of
   `Network/NetworkPeerBase.cpp` (0x803CCDF8), so it is declared here and defined there. */
class NetworkUniqueId : public NetworkStreamSink {
public:
    NetworkUniqueId();
    /* +0x1C */ virtual void bind(u8* block, u32 size);
    /* +0x08 */ virtual ~NetworkUniqueId();
    /* +0x2C */ virtual void slot_2C();

    s32 isValid() const;
    void importFrom(u8 kind, const u8* data, u32 size);
    void exportTo(u8* out, u32 size) const;
    u32 equals(const NetworkUniqueId* other) const;

    /* +0x10 */ NetworkUniqueIdData data_10;
    /* +0x1E */ u8 pad_1E[2];
};   /* size: 0x20 (every record that embeds one leaves 0x20 bytes before its next field) */

#endif /* MHTRI_NETWORK_NETWORKUNIQUEID_H */
