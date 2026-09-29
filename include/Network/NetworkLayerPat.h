/*
 * include/Network/NetworkLayerPat.h - the `NetworkLayerPat` view `Network/NetworkLayerPatStep.cpp` needs.
 *
 * `NetworkLayerPat` is the +0x0C element of the `NetworkPat` holder (include/Network/NetworkPat.h):
 * the class `fn_803E0C18` builds, whose vtable is `lbl_805FC1E0`
 * (0x805FC1E0, 0x144 B) and whose own method names ("NetworkLayerPat::move ...") come from the pool
 * strings in the band's `.data`.  Only the fields the request state machine reads are named; the rest is
 * padding at its real offset.  The table is **not** modelled here (the class is non-virtual in this
 * view): declaring the virtuals would make MWCC emit the table into our object, and it belongs to the
 * band's other bodies.  The size is approximate: the constructor builds members out to +0x7E128.
 */
#ifndef MHTRI_NETWORK_NETWORKLAYERPAT_H
#define MHTRI_NETWORK_NETWORKLAYERPAT_H

#include "types.h"
#include "Network/fn_803D3CE8.h"   /* NetworkRequest - the record the state machine advances */

class NetworkLayerPat {   /* size: 0xF1A0 (approximate) */
public:
    /* +0x0000 */ u32 vtable_00;            /* the class's table pointer (lbl_805FC1E0) - never written here */
    /* +0x0004 */ u8 pad_04[0x04];
    /* +0x0008 */ u32 context_08;           /* the context word every layer event is reported with */
    /* +0x000C */ u8 pad_0C[0x318];
    /* +0x0324 */ u32 requestFlags_324;     /* bit0 = session lost, bit1 = cancelled, higher bits = replies */
    /* +0x0328 */ u8 pad_328[0x54];
    /* +0x037C */ u32 requestResult_37C;    /* the id the last `sendReqLayer*` returned */
    /* +0x0380 */ u8 pad_380[0x50];
    /* +0x03D0 */ u8 connected_3D0;         /* zero = no session: the request cannot start */
    /* +0x03D1 */ u8 busy_3D1;              /* set while a request is in flight; the finishing states clear it */
    /* +0x03D2 */ u8 pad_3D2[0xF2];
    /* +0x04C4 */ u8 hostMode_4C4;          /* selects the user-list step after the child-info reply */
    /* +0x04C5 */ u8 pad_4C5[0x03];
    /* +0x04C8 */ s32 memberCount_4C8;      /* must be positive for a request to start */
    /* +0x04CC */ u8 pad_4CC[0xECD0];
    /* +0xF19C */ s32 pendingRequestId_F19C;   /* GUESS: a request id is already pending when it is >= 0 */

    /* Advances the request state in `request` by one step; true once the request finished. */
    bool stepRequest(NetworkRequest* request);
};

#endif
