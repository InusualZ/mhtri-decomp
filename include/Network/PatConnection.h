/*
 * include/Network/PatConnection.h - the free functions of `src/Network/PatConnection.cpp` (`.text` 0x803FAE9C..0x803FCC34): the session band's request writers.
 * Moved here from `Network/NetworkCommunityPat.h` when the network pilot round 3 recut gave the range its own unit
 * (docs/plan.md 6.5 rule 2: the owner declares).
 */
#ifndef MHTRI_NETWORK_PATCONNECTION_H
#define MHTRI_NETWORK_PATCONNECTION_H

#include "types.h"

class PatInterface;                       /* include/Network/PatInterface.h */
typedef PatInterface NetworkStateMachine;  /* the state machine's spelling of the singleton */

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

/* 0x803FBB04 */
void readUInt8(NetworkStateMachine* self, u8* out);
/* 0x803FBB20 */
void readUInt16(NetworkStateMachine* self, u16* out);
/* 0x803FBB8C */
void readUInt32_(NetworkStateMachine* self, u32* out);
/* 0x803FBC6C */
void readUInt8_(NetworkStateMachine* self, s8* out);
/* 0x803FBCE0 */
void readInt32(NetworkStateMachine* self, s32* out);
/* 0x803FBD18 */
void readString(NetworkStateMachine* self, u32* length, char* buffer, u16 size);
/* 0x803FBDDC */
void readUInt8Array(NetworkStateMachine* self, u32* length, u8* buffer, u16 size);
/* 0x803FBE88 */
void beginReadBlock(NetworkStateMachine* self, u16* size);
/* 0x803FBED0 */
void endReadBlock(NetworkStateMachine* self);
/* 0x803FC40C */
void writeInt16(NetworkStateMachine* self, s16 value);
/* 0x803FC418 */
u32 writeString(NetworkStateMachine* self, const char* text);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_NETWORK_PATCONNECTION_H */
