/*
 * include/Network/PatConnection.h - the free functions of `src/Network/PatConnection.cpp` (`.text` 0x803FAE9C..0x803FCC34): the session band's request writers.
 * Moved here from `Network/NetworkCommunityPat.h` when the network pilot round 3 recut gave the range its own unit
 * (docs/plan.md 6.5 rule 2: the owner declares).
 */
#ifndef MHTRI_NETWORK_PATCONNECTION_H
#define MHTRI_NETWORK_PATCONNECTION_H

#include "types.h"

typedef struct NetworkStateMachine NetworkStateMachine;   /* include/Network/network_state.h */

#ifdef __cplusplus
extern "C" {
#endif

/* ---- the session band's request writers (moved here from `Network/network_state.h`; the state machine
   passes its own view of the session object) */
u32  flushBuffer(NetworkStateMachine* self, u32 opcode, u32 flags);
void encryptBuffer(NetworkStateMachine* self);
u32  writeUInt8(NetworkStateMachine* self, u8 value);
void writeUInt16(NetworkStateMachine* self, u16 value);
void writeUInt32(NetworkStateMachine* self, u32 value);
void writeUInt32Shared(NetworkStateMachine* self, u32 value);
void writeUInt8Array(NetworkStateMachine* self, const u8* data, u16 count);
void writeBool(NetworkStateMachine* self, s8 value);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_NETWORK_PATCONNECTION_H */
