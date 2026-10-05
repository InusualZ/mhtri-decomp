/*
 * Network/network_layer_io.cpp - the `PatInterface` packet layer: the `sendReq*` request builders and the
 * `recv*` packet handlers behind them.
 *
 * `.text` 0x804006A8..0x80413450.  Sections: extab 0x8001BE24..0x8001C9B4; extabindex 0x8003C138..0x8003D278;
 * .rodata 0x80570E20..0x80570E70; .data 0x80600BA0..0x806024B8; .sdata 0x80793970..0x80793990;
 * .sbss 0x80794CB8..0x80794CC0; .sdata2 0x8079C7E8..0x8079C874 (config/RMHE08/splits.txt).
 *
 * WHAT IT IS.  The `recv*` handlers are `PatInterface` virtuals (slots +0x10..+0x280 of the table 0x80602198, which
 * sits in this unit's `.data`) that `recvCommand` (slot +0x284) reaches through the packet table's member pointers
 * (`PacketTable_BaseOffset_ID1`, `__ptmf_scall`), so they are written as members and their map rows carry the
 * manglings (`recvAnsShut__12PatInterfaceFlPC15PatPacketHeader`; the parameter types are GUESSES from the bodies:
 * r4 is the packet table index `recvAnsNg` stores, r5 the 8-byte header).  The `sendReq*` builders stay C linkage:
 * the map names them unmangled and the session units call them directly.  Every handler starts with the same log
 * line, `"%s:<name> ok\n"` with the connected server's tag (`getServerName`) and a trailing `""`.
 *
 * TU.  The scouting report reads `Network/PatInterface.cpp` + `Network/network_state.cpp` + this unit's head as ONE
 * ~88 KB TU (the PatInterface table and its `.data` live here, the constructor and slot +0x08/+0x0C/+0x288 bodies in
 * `Network/PatInterface.cpp`); the units are kept apart (recorded, not merged).  The tail 0x8041241C..0x80413450
 * (`NetworkPool`, the NHTTP wrappers, `NetworkRandom`) is a separate TU.
 *
 * FLAGS.  `-O3` (configure.py, with the evidence) and a file-scope `#pragma peephole off`: retail keeps the unfused
 * `clrlwi r0,r0,16` before the ticket size's `sth` (recvAnsTicket/recvReqTicket 98.25/98.33 with the pass on, 100
 * off) and the `bge`+`b` pairs the pass folds; no written row scores lower with it off.
 *
 * SOURCE SHAPES.  Locals of one size are laid out in reverse declaration order (the later one at the lower
 * offset), so each handler declares them in the order retail's frame needs; a failed `readBodySlice` returns its
 * own result (retail branches to the epilogue with r3 intact); the stack-clamped list answers divide into a
 * `maxCount` local (retail's register choice for the magic-number division); `tags[count++] = v` with a known count
 * is retail's `li`/`li`/`stb` order; a selector written `x == 0 ? a : b` or `x != 0 ? b : a` decides which constant
 * is loaded first.  The request builders take the session units' `NetworkInstance*` and cast at each use (since request
 * net3-d-03c2#1 the band's `NetworkInstance` is a typedef of `PatInterface`, so the casts are no-ops).
 *
 * WRITTEN.  The request builders 0x804006A8..0x804040xx except sendReqLayerChildInfo, sendReqLayerUserInfoSet
 * (0x80401AF4), sendReqLayerUserListHead, sendReqLayerUserSearchHead, the layer binary/position/chat/tell notices
 * 0x80401EC8..0x804021A8, the mediation requests, sendReqLayerDetailSearchHead, sendReqCircleCreate/Info/Join/
 * MatchOptionSet/InfoSet/ListLayer/ListHead/UserList, the circle binary/chat/tell/value notices 0x8040294C..0x80403460,
 * sendReqCircleInfoNoticeSet, 0x8040354C/0x804035D8, sendReqUserSearchHead/Info/InfoMine, 0x80403A88/0x80403BF4,
 * sendReqFriendList, 0x80403DE4/0x80403EDC and sendReqChannelInfo; and every `recv*` handler except
 * recvNtcLayerBinary, recvNtcLayerUserPosition, recvAnsLayerDetailSearchData and recvNtcCircleBinary.  Not written:
 * the item readers/writers 0x8040E0D0..0x80412188, `recvCommand`, `dispatchSessionHandlers` and the separate tail TU.
 *
 * RESIDUALS.  recvAnsAgreementPageInfo 99.94 (retail addresses the page count as `infoPtr+12`, ours folds it to
 * `r1+28`).  Every body not written yet is 0 %; the vtable 0x80602198 and the packet table are not emitted (rule 10:
 * the key function, the destructor 0x803FCF8C, is `Network/PatInterface.cpp`'s, and the TU question above decides
 * where the table can be emitted).  `.data` cannot match as registered: `datagap` reads at least three TUs in it
 * (seams in [0x80602428, 0x80602490) and at 0x806024A0).  The connection base's readers are declared in
 * `Network/PatConnection.h` (requests net3-d-03c2#2..#12).
 */

#include "Network/network_layer_io.h"
#include "Network/PatInterface.h"
#include "Network/PatConnection.h"   /* the request writers */
#include "unsplit/Network.h"         /* getNetworkLogger */
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"

#pragma peephole off

inline const char* PatInterface::getServerName()
{
    if (serverType_65F0 == 1) {
        return "FMP";
    }
    if (serverType_65F0 == 0) {
        return "LMP";
    }
    if (serverType_65F0 == 2) {
        return "OPN";
    }
    if (serverType_65F0 == 3) {
        return "RFP";
    }
    return "???";
}

/* The log line every handler starts with: the connected server's tag, the format's own arguments, a trailing "". */
#define PAT_TRACE(fmt)                                                   \
    {                                                                    \
        const char* serverName = getServerName();                        \
        getNetworkLogger()->signal_0C(2, fmt, serverName, "");           \
    }
#define PAT_TRACE_OPCODE(fmt)                                                                          \
    {                                                                                                  \
        const char* serverName = getServerName();                                                      \
        getNetworkLogger()->signal_0C(2, fmt, serverName, header->opcode_04[0], header->opcode_04[1], ""); \
    }

/* Keeps a generic failure as the pending error unless one is already kept. */
#define PAT_KEEP_FAILURE()                       \
    if (pendingError_654C.code_00 == 0) {        \
        pendingError_654C.code_00 = 0x80000000;  \
        pendingError_654C.param1_04 = 0;         \
        pendingError_654C.param2_08 = 0;         \
    }

/* ---- the request builders: each opens a request with its op-code, writes the items, seals it and returns the
 * request id the answer is matched against (narrowed to 16 bits) ------------------------------------------------- */

/* Keeps a generic failure as the error and reports it (the server type has no such request). */
#define PAT_POST_FAILURE(pat)                    \
    {                                            \
        NetworkPostedError error;                \
        error.code_00 = 0x80000000;              \
        error.param1_04 = 0;                     \
        error.param2_08 = 0;                     \
        (pat)->postError(error);                 \
    }

/* Requests the FMP list's version from the lobby or FMP server. */
u32 sendReqFmpListVersion(NetworkInstance* self)
{
    u32 id;

    switch (((PatInterface*)self)->serverType_65F0) {
    case 0:
        id = flushBuffer((PatInterface*)self, 35, 0);
        break;
    case 1:
        id = flushBuffer((PatInterface*)self, 79, 0);
        break;
    default:
        PAT_POST_FAILURE((PatInterface*)self);
        return -1;
    }
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests the FMP list's head: the list version, the range and the item tags (all of them while the list is stale). */
u32 sendReqFmpListHead(NetworkInstance* self, s32 first, s32 count)
{
    u8 tags[7] = { 1, 8, 9, 7, 10, 11, 12 };
    u8 tagCount;
    u32 id;

    tagCount = ((PatInterface*)self)->fmpListReady_6C3C != 0 ? 7 : 3;
    switch (((PatInterface*)self)->serverType_65F0) {
    case 0:
        id = flushBuffer((PatInterface*)self, 37, 0);
        break;
    case 1:
        id = flushBuffer((PatInterface*)self, 81, 0);
        break;
    default:
        PAT_POST_FAILURE((PatInterface*)self);
        return -1;
    }
    writeUInt32((PatInterface*)self, ((PatInterface*)self)->fmpListActive_6C38);
    writeUInt32Shared((PatInterface*)self, first);
    writeUInt32Shared((PatInterface*)self, count);
    writeUInt8Array2((PatInterface*)self, tagCount, tags);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests a range of the FMP list. */
u32 sendReqFmpListData(NetworkInstance* self, u32 start, u32 count)
{
    u32 id;

    switch (((PatInterface*)self)->serverType_65F0) {
    case 0:
        id = flushBuffer((PatInterface*)self, 39, 0);
        break;
    case 1:
        id = flushBuffer((PatInterface*)self, 83, 0);
        break;
    default:
        PAT_POST_FAILURE((PatInterface*)self);
        return -1;
    }
    writeUInt32Shared((PatInterface*)self, start);
    writeUInt32Shared((PatInterface*)self, count);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Ends the FMP list request. */
u32 sendReqFmpListFoot(NetworkInstance* self)
{
    u32 id;

    switch (((PatInterface*)self)->serverType_65F0) {
    case 0:
        id = flushBuffer((PatInterface*)self, 41, 0);
        break;
    case 1:
        id = flushBuffer((PatInterface*)self, 85, 0);
        break;
    default:
        PAT_POST_FAILURE((PatInterface*)self);
        return -1;
    }
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests one FMP slot's details: two item tags when `brief`, else all nine. */
u32 sendReqFmpInfo(NetworkInstance* self, u32 value, s32 brief)
{
    u8 tags[9] = { 2, 3, 1, 8, 9, 7, 10, 11, 12 };
    u8 tagCount;
    u32 id;

    tagCount = brief == 0 ? 9 : 2;
    switch (((PatInterface*)self)->serverType_65F0) {
    case 0:
        id = flushBuffer((PatInterface*)self, 43, 0);
        break;
    case 1:
        id = flushBuffer((PatInterface*)self, 87, 0);
        break;
    default:
        PAT_POST_FAILURE((PatInterface*)self);
        return -1;
    }
    writeUInt32((PatInterface*)self, value);
    writeUInt8Array2((PatInterface*)self, tagCount, tags);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests the RFP server's address. */
u32 sendReqRfpConnect(NetworkInstance* self)
{
    u32 id = flushBuffer((PatInterface*)self, 45, 0);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests the lobby server's address. */
u32 sendReqLmpConnect(NetworkInstance* self)
{
    u32 id = flushBuffer((PatInterface*)self, 47, 0);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests the terms version. */
u32 sendReqTermsVersion(NetworkInstance* self)
{
    u32 id = flushBuffer((PatInterface*)self, 49, 0);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests one slice of the terms body. */
u32 sendReqTerms(NetworkInstance* self, u32 a, u32 b, u32 len)
{
    u32 id = flushBuffer((PatInterface*)self, 51, 0);
    writeUInt32((PatInterface*)self, a);
    writeUInt32((PatInterface*)self, b);
    writeUInt32((PatInterface*)self, len);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests the maintenance text. */
u32 sendReqMaintenance(NetworkInstance* self)
{
    u32 id = flushBuffer((PatInterface*)self, 53, 0);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests the announcement text. */
u32 sendReqAnnounce(NetworkInstance* self)
{
    u32 id = flushBuffer((PatInterface*)self, 55, 0);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests the no-charge text; returns 0, not the request id. */
u32 sendReqNoCharge(NetworkInstance* self)
{
    flushBuffer((PatInterface*)self, 57, 0);
    encryptBuffer((PatInterface*)self);
    return 0;
}

/* Requests the media version. */
u32 sendReqMediaVersionInfo(NetworkInstance* self)
{
    u32 id = flushBuffer((PatInterface*)self, 59, 0);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests the low-level vulgarity list's slice descriptor. */
u32 sendReqVulgarityInfoLow(NetworkInstance* self, s32 mode)
{
    u32 id = flushBuffer((PatInterface*)self, 65, 0);
    writeUInt32((PatInterface*)self, mode);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests one slice of the low-level vulgarity list. */
u32 sendReqVulgarityLow(NetworkInstance* self, s32 mode, u32 slice, u32 offset, u32 length)
{
    u32 id = flushBuffer((PatInterface*)self, 67, 0);
    writeUInt32((PatInterface*)self, mode);
    writeUInt32((PatInterface*)self, slice);
    writeUInt32((PatInterface*)self, offset);
    writeUInt32((PatInterface*)self, length);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Sends the authentication token. */
u32 sendReqAuthenticationToken(NetworkInstance* self, const char* token)
{
    u32 id = flushBuffer((PatInterface*)self, 69, 0);
    writeString((PatInterface*)self, token);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests a file's checksum. */
s32 sendReqBinaryChecksum(NetworkInstance* self, u8 fileId)
{
    u32 id = flushBuffer((PatInterface*)self, 71, 0);
    writeUInt8((PatInterface*)self, fileId);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests a file's head; `mode` selects how the answers are consumed. */
s32 sendReqBinaryHead(NetworkInstance* self, u8 fileId, s8 mode)
{
    u32 id;

    ((PatInterface*)self)->binaryMode_611A = mode;
    id = flushBuffer((PatInterface*)self, 73, 0);
    writeUInt8((PatInterface*)self, fileId);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests one chunk of a file. */
s32 sendReqBinaryData(NetworkInstance* self, u8 fileId, u32 handle, u32 offset, u32 size)
{
    u32 id = flushBuffer((PatInterface*)self, 75, 0);
    writeUInt8((PatInterface*)self, fileId);
    writeUInt32((PatInterface*)self, handle);
    writeUInt32((PatInterface*)self, offset);
    writeUInt32((PatInterface*)self, size);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Ends a file transfer. */
s32 sendReqBinaryFoot(NetworkInstance* self, u8 fileId)
{
    u32 id = flushBuffer((PatInterface*)self, 77, 0);
    writeUInt8((PatInterface*)self, fileId);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Starts the layer session, asking for the layer and user items. */
u32 sendReqLayerStart(NetworkInstance* self)
{
    u32 id;

    ((PatInterface*)self)->layerMovePending_D62C = 0;
    id = flushBuffer((PatInterface*)self, 89, 0);
    writeLayerItemRequest((PatInterface*)self);
    writeUserItemRequest((PatInterface*)self);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Ends the layer session. */
u32 sendReqLayerEnd(NetworkInstance* self)
{
    u32 id = flushBuffer((PatInterface*)self, 91, 0);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests a jump to the layer at `path`. */
u32 sendReqLayerJump(NetworkInstance* self, const u8* path, u32 value)
{
    u32 id = flushBuffer((PatInterface*)self, 94, 0);
    writeUnkShortArray((PatInterface*)self, path);
    writeUInt32((PatInterface*)self, value);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Starts creating a layer under `layerId`. */
u32 sendReqLayerCreateHead(NetworkInstance* self, s16 layerId)
{
    u32 id = flushBuffer((PatInterface*)self, 96, 0);
    writeShortPlusOne((PatInterface*)self, layerId);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Sends the new layer's settings (its name only when it has one) and tags. */
u32 sendReqLayerCreateSet(NetworkInstance* self, s16 layerId, PatLayerData* layer, PatTagList* tags)
{
    u8 items[3] = { 9, 10, 3 };
    u8 itemCount;
    u32 id;

    itemCount = 3;
    if (layer->name_014[0] == 0) {
        itemCount = 2;
    }
    id = flushBuffer((PatInterface*)self, 98, 0);
    writeShortPlusOne((PatInterface*)self, layerId);
    writeLayerDownData((PatInterface*)self, layer, itemCount, items);
    writeUnkByteIntStruct((PatInterface*)self, tags);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Ends creating a layer; without `move` no layer move stays pending. */
u32 sendReqLayerCreateFoot(NetworkInstance* self, s16 layerId, u8 move)
{
    u32 id;

    if (move == 0) {
        ((PatInterface*)self)->layerMovePending_D62C = 0;
    }
    id = flushBuffer((PatInterface*)self, 100, 0);
    writeShortPlusOne((PatInterface*)self, layerId);
    writeUInt8((PatInterface*)self, move);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests a move down into the layer `layerId`, sending the layer's item 12. */
u32 sendReqLayerDown(NetworkInstance* self, s16 layerId, PatLayerData* layer)
{
    u8 items[1] = { 12 };
    u32 id = flushBuffer((PatInterface*)self, 102, 0);
    writeShortPlusOne((PatInterface*)self, layerId);
    writeLayerDownData((PatInterface*)self, layer, 1, items);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests a move up a layer. */
u32 sendReqLayerUp(NetworkInstance* self)
{
    u32 id = flushBuffer((PatInterface*)self, 105, 0);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Tells the server the jump to `path` is ready. */
u32 sendReqLayerJumpReady(NetworkInstance* self, const u8* path, u32 value)
{
    u32 id = flushBuffer((PatInterface*)self, 108, 0);
    writeUnkShortArray((PatInterface*)self, path);
    writeUInt32((PatInterface*)self, value);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Tells the server to go ahead with the jump to `path`. */
u32 sendReqLayerJumpGo(NetworkInstance* self, const u8* path, u32 value)
{
    u32 id = flushBuffer((PatInterface*)self, 110, 0);
    writeUnkShortArray((PatInterface*)self, path);
    writeUInt32((PatInterface*)self, value);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Sends the layer's changed settings (item 23 when it carries the extra value) and tags. */
u32 sendReqLayerInfoSet(NetworkInstance* self, PatLayerData* layer, PatTagList* tags)
{
    u8 items[24];
    u8 itemCount;
    u32 id;

    itemCount = 0;
    if (layer->value_23E != 0) {
        items[itemCount++] = 23;
    }
    id = flushBuffer((PatInterface*)self, 112, 0);
    writeLayerDownData((PatInterface*)self, layer, itemCount, items);
    writeUnkByteIntStruct((PatInterface*)self, tags);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests the layer at `path`'s settings: the user count items for mode 4, the layer items for mode 3. */
u32 sendReqLayerInfo(NetworkInstance* self, const u8* path, const PatTagList* tags, s8 mode)
{
    u32 id = flushBuffer((PatInterface*)self, 115, 0);
    writeUnkShortArray((PatInterface*)self, path);
    if (mode == 4) {
        u8 items[2] = { 0x15, 0x16 };
        writeUInt8Array2((PatInterface*)self, 2, items);
    } else if (mode == 3) {
        writeLayerItemRequest((PatInterface*)self);
    }
    writeUnk2ByteArray((PatInterface*)self, tags);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests the parent layer's settings with the layer items. */
u32 sendReqLayerParentInfo(NetworkInstance* self, const PatTagList* tags)
{
    u32 id = flushBuffer((PatInterface*)self, 117, 0);
    writeLayerItemRequest((PatInterface*)self);
    writeUnk2ByteArray((PatInterface*)self, tags);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests the child layer list's head with the layer items. */
u32 sendReqLayerChildListHead(NetworkInstance* self, s32 first, s32 count)
{
    u32 id = flushBuffer((PatInterface*)self, 121, 0);
    writeUInt32Shared((PatInterface*)self, first);
    writeUInt32Shared((PatInterface*)self, count);
    writeLayerItemRequest((PatInterface*)self);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests a range of the child layer list. */
u32 sendReqLayerChildListData(NetworkInstance* self, s32 first, s32 count)
{
    u32 id = flushBuffer((PatInterface*)self, 123, 0);
    writeUInt32Shared((PatInterface*)self, first);
    writeUInt32Shared((PatInterface*)self, count);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Ends the child layer list request. */
u32 sendReqLayerChildListFoot(NetworkInstance* self)
{
    u32 id = flushBuffer((PatInterface*)self, 125, 0);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests the sibling layer list's head with the layer items. */
u32 sendReqLayerSiblingListHead(NetworkInstance* self, s32 first, s32 count)
{
    u32 id = flushBuffer((PatInterface*)self, 127, 0);
    writeUInt32Shared((PatInterface*)self, first);
    writeUInt32Shared((PatInterface*)self, count);
    writeLayerItemRequest((PatInterface*)self);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests a range of the sibling layer list. */
u32 sendReqLayerSiblingListData(NetworkInstance* self, s32 first, s32 count)
{
    u32 id = flushBuffer((PatInterface*)self, 129, 0);
    writeUInt32Shared((PatInterface*)self, first);
    writeUInt32Shared((PatInterface*)self, count);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Ends the sibling layer list request. */
u32 sendReqLayerSiblingListFoot(NetworkInstance* self)
{
    u32 id = flushBuffer((PatInterface*)self, 131, 0);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests the host of the layer at `path`. */
u32 sendReqLayerHost(NetworkInstance* self, const u8* path)
{
    u32 id = flushBuffer((PatInterface*)self, 133, 0);
    writeUnkShortArray((PatInterface*)self, path);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests the layer's users with five user items. */
u32 sendReqLayerUserList(NetworkInstance* self)
{
    u8 items[5] = { 1, 2, 3, 6, 7 };
    u32 id = flushBuffer((PatInterface*)self, 139, 0);
    writeUInt8Array2((PatInterface*)self, 5, items);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests a range of the layer user list. */
u32 sendReqLayerUserListData(NetworkInstance* self, s32 first, s32 count)
{
    u32 id = flushBuffer((PatInterface*)self, 143, 0);
    writeUInt32Shared((PatInterface*)self, first);
    writeUInt32Shared((PatInterface*)self, count);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Ends the layer user list request. */
u32 sendReqLayerUserListFoot(NetworkInstance* self)
{
    u32 id = flushBuffer((PatInterface*)self, 145, 0);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests a range of the layer user search. */
u32 sendReqLayerUserSearchData(NetworkInstance* self, s32 first, s32 count)
{
    u32 id = flushBuffer((PatInterface*)self, 149, 0);
    writeUInt32Shared((PatInterface*)self, first);
    writeUInt32Shared((PatInterface*)self, count);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Ends the layer user search request. */
u32 sendReqLayerUserSearchFoot(NetworkInstance* self)
{
    u32 id = flushBuffer((PatInterface*)self, 151, 0);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests leaving the circle. */
u32 sendReqCircleLeave(NetworkInstance* self, s32 circleId)
{
    u32 id = flushBuffer((PatInterface*)self, 187, 0);
    writeUInt32Shared((PatInterface*)self, circleId);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests the circle's match start. */
u32 sendReqCircleMatchStart(NetworkInstance* self)
{
    u32 id = flushBuffer((PatInterface*)self, 198, 0);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests the circle's match end. */
u32 sendReqCircleMatchEnd(NetworkInstance* self, s32 mode)
{
    u32 id = flushBuffer((PatInterface*)self, 201, 0);
    writeUInt8((PatInterface*)self, mode);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests a range of the circle list. */
u32 sendReqCircleListData(NetworkInstance* self, s32 first, s32 count)
{
    u32 id = flushBuffer((PatInterface*)self, 210, 0);
    writeUInt32Shared((PatInterface*)self, first);
    writeUInt32Shared((PatInterface*)self, count);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Ends the circle list request. */
u32 sendReqCircleListFoot(NetworkInstance* self)
{
    u32 id = flushBuffer((PatInterface*)self, 212, 0);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests kicking `userId` from the circle with an empty kick list. */
u32 sendReqCircleKick(NetworkInstance* self, const char* userId)
{
    PatKickList kick;
    u32 id = flushBuffer((PatInterface*)self, 214, 0);
    writeString((PatInterface*)self, userId);
    memset(&kick, 0, sizeof(PatKickList));
    kick.kind_000 = 1;
    writeUInt8((PatInterface*)self, 1);
    writeUInt8Array((PatInterface*)self, kick.data_002, kick.size_102);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests the circle's host. */
u32 sendReqCircleHost(NetworkInstance* self, s32 circleId)
{
    u32 id = flushBuffer((PatInterface*)self, 222, 0);
    writeUInt32Shared((PatInterface*)self, circleId);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Sends this user's search tags. */
u32 sendReqUserSearchSet(NetworkInstance* self, PatTagList* tags)
{
    u32 id = flushBuffer((PatInterface*)self, 252, 0);
    writeUnkByteIntStruct((PatInterface*)self, tags);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Sends this user's binary with its value. */
u32 sendReqUserBinarySet(NetworkInstance* self, u32 value, const u8* data, u32 size)
{
    u32 id = flushBuffer((PatInterface*)self, 254, 0);
    writeUInt32((PatInterface*)self, value);
    writeUInt8Array((PatInterface*)self, data, size);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Sends a binary notice: its kind, an optional text (an empty one when null) and two values. */
u32 sendReqUserBinaryNotice(NetworkInstance* self, u8 kind, const char* text, u32 a, u32 b)
{
    u32 id = flushBuffer((PatInterface*)self, 256, 0);
    writeUInt8((PatInterface*)self, kind);
    if (text != NULL) {
        writeString((PatInterface*)self, text);
    } else {
        writeUInt16((PatInterface*)self, 0);
    }
    writeUInt32((PatInterface*)self, a);
    writeUInt32((PatInterface*)self, b);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests a range of the user search. */
u32 sendReqUserSearchData(NetworkInstance* self, s32 first, s32 count)
{
    u32 id = flushBuffer((PatInterface*)self, 261, 0);
    writeUInt32Shared((PatInterface*)self, first);
    writeUInt32Shared((PatInterface*)self, count);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Ends the user search request. */
u32 sendReqUserSearchFoot(NetworkInstance* self)
{
    u32 id = flushBuffer((PatInterface*)self, 263, 0);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Answers a friend request (GUESS name: op-code 276, after the friend request 273). */
u32 sendReqFriendAccept(NetworkInstance* self, const char* userId, u8 accept)
{
    u32 id = flushBuffer((PatInterface*)self, 276, 0);
    writeString((PatInterface*)self, userId);
    writeUInt8((PatInterface*)self, accept);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Removes a friend (GUESS name: op-code 279). */
u32 sendReqFriendDelete(NetworkInstance* self, const char* userId)
{
    u32 id = flushBuffer((PatInterface*)self, 279, 0);
    writeString((PatInterface*)self, userId);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Removes a user from the black list (GUESS name: op-code 285). */
u32 sendReqBlackDelete(NetworkInstance* self, const char* userId)
{
    u32 id = flushBuffer((PatInterface*)self, 285, 0);
    writeString((PatInterface*)self, userId);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests a range of a reflect channel. */
u32 sendReqChannelData(NetworkInstance* self, u32 handle, u32 offset, u32 size)
{
    u32 id = flushBuffer((PatInterface*)self, 293, 0);
    writeUInt8((PatInterface*)self, ((PatInterface*)self)->warningKind_8BBC);
    writeUInt8((PatInterface*)self, handle);
    writeUInt32((PatInterface*)self, offset);
    writeUInt32((PatInterface*)self, size);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests the reflect connection. */
u32 sendReqConnect(NetworkInstance* self)
{
    u32 id = flushBuffer((PatInterface*)self, 295, 0);
    writeUInt8((PatInterface*)self, ((PatInterface*)self)->warningKind_8BBC);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Logs a packet whose op-code has no table entry. */
s32 PatInterface::recvNotProvided(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE_OPCODE("%s:recvNotProvided[%02Xx%02X]\n");
    return -1;
}

/* Keeps a negative reply's code and message as the error record unless an error is pending. */
s32 PatInterface::recvAnsNg(s32 index, const PatPacketHeader* header)
{
    u32 length;

    PAT_TRACE_OPCODE("%s:recvAnsNg[%02Xx%02X]\n");
    if (pendingError_654C.code_00 == 0) {
        readInt32(this, &errorRecord_613C.code_000);
        readString(this, &length, errorRecord_613C.message_008, 512);
        errorRecord_613C.index_004 = index;
    }
    return -1;
}

/* Reads an alert, keeps it as the error record unless an error is pending, and reports it to the session handlers. */
s32 PatInterface::recvAnsAlert(s32 index, const PatPacketHeader* header)
{
    u32 length;
    PatErrorRecord alert;

    PAT_TRACE_OPCODE("%s:recvAnsAlert[%02Xx%02X]\n");
    readInt32(this, &alert.code_000);
    readString(this, &length, alert.message_008, 512);
    alert.index_004 = index;
    if (pendingError_654C.code_00 == 0) {
        memcpy(&errorRecord_613C, &alert, sizeof(PatErrorRecord));
    }
    dispatchSessionHandlers(this, 0x8002, header->requestId_02, header->status_07, 1, (const u8*)&alert);
    return -3;
}

/* Answers the server's line check with an empty request. */
s32 PatInterface::recvReqLineCheck(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvReqLineCheck ok\n");
    flushBuffer(this, 1, 0);
    encryptBuffer(this);
    return 0;
}

/* Takes the server's game and date time and re-bases the clock on them. */
s32 PatInterface::recvAnsServerTime(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsServerTime ok\n");
    readUInt32_(this, &gameTimeBase_6128);
    readUInt32_(this, &serverTime_612C);
    clockAtSync_6120 = clock_611C;
    if (requestState_6135 == 15 || requestState_6135 == 85) {
        requestState_6135 += 5;
    }
    return 0;
}

/* Reads the shutdown answer and advances the session, or hands it to the session handlers. */
s32 PatInterface::recvAnsShut(s32 index, const PatPacketHeader* header)
{
    u8 value;

    PAT_TRACE("%s:recvAnsShut ok\n");
    readUInt8(this, &value);
    if (serverType_65F0 == 2) {
        sessionState_6132 += 5;
    } else {
        dispatchSessionHandlers(this, 0x8006, header->requestId_02, header->status_07, 1, &value);
    }
    return 0;
}

/* Keeps the server's shutdown notice (its code and message) in the shutdown record. */
s32 PatInterface::recvNtcShut(s32 index, const PatPacketHeader* header)
{
    u8 code;
    u32 length;

    PAT_TRACE("%s:recvNtcShut ok\n");
    readUInt8(this, &code);
    readString(this, &length, shutdownRecord_6344.message_008, 512);
    shutdownRecord_6344.code_000 = code;
    shutdownRecord_6344.index_004 = index;
    return -2;
}

/* Takes the address and port to reconnect to; only the opening server may send it. */
s32 PatInterface::recvNtcRecconect(s32 index, const PatPacketHeader* header)
{
    u32 length;

    PAT_TRACE("%s:recvNtcRecconect ok\n");
    if (serverType_65F0 != 2) {
        PAT_KEEP_FAILURE();
        return -1;
    }
    memset(&serverAddress_6A2C, 0, sizeof(PatServerAddress));
    readString(this, &length, serverAddress_6A2C.host_000, 256);
    readUInt16(this, &serverAddress_6A2C.port_104);
    flag_6558 = 0;
    sessionState_6132 = 1;
    return 0;
}

/* Answers the connection request with the login block's tagged items. */
s32 PatInterface::recvReqConnection(s32 index, const PatPacketHeader* header)
{
    u32 value;
    u8 count;
    PatTicket* ticket;

    PAT_TRACE("%s:recvReqConnection ok\n");
    readUInt32_(this, &value);
    flag_6558 = 1;
    u8 tags[10] = { 3, 4, 5, 6, 7, 8 };
    count = 6;
    if (loginInfoSent_82B4 == 0 || reflectName5C_65C8[0] != 0) {
        tags[count++] = 1;
    }
    ticket = ticket_8BB0;
    if (ticket != NULL && ticket->size_400 != 0) {
        tags[count++] = 2;
    }
    tags[count++] = 9;
    tags[count++] = 10;
    flushBuffer(this, 9, 0);
    putSomethingList(this, loginFields_82AC, count, tags);
    encryptBuffer(this);
    return 0;
}

/* Reads the login state for the connected server and advances that server's state machine. */
s32 PatInterface::recvNtcLogin(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvNtcLogin ok\n");
    if (serverType_65F0 == 2) {
        readUInt8(this, &patState_8254);
        sessionState_6132 += 5;
    } else if ((u32)serverType_65F0 <= 1) {
        readUInt8(this, &fmpPhase_894D);
        requestState_6135 += 5;
    } else if (serverType_65F0 == 3) {
        readUInt8(this, &fmpPhase_894D);
        subState_6133 += 5;
    } else {
        PAT_KEEP_FAILURE();
        return -1;
    }
    return 0;
}

/* Reads the login ticket into the caller's buffer. */
s32 PatInterface::recvAnsTicket(s32 index, const PatPacketHeader* header)
{
    u32 length;

    PAT_TRACE("%s:recvAnsTicket ok\n");
    if (ticket_8BB0 != NULL) {
        readUInt8Array(this, &length, ticket_8BB0->data_000, 1024);
        ticket_8BB0->size_400 = (u16)length;
    }
    requestState_6135 += 5;
    return 0;
}

/* Reads the login ticket and acknowledges it. */
s32 PatInterface::recvReqTicket(s32 index, const PatPacketHeader* header)
{
    u32 length;

    PAT_TRACE("%s:recvReqTicket ok\n");
    if (ticket_8BB0 != NULL) {
        readUInt8Array(this, &length, ticket_8BB0->data_000, 1024);
        ticket_8BB0->size_400 = (u16)length;
    }
    flushBuffer(this, 14, 0);
    encryptBuffer(this);
    return 0;
}

/* Reads a warning (kind, value, text into the reply buffer) and acknowledges it. */
s32 PatInterface::recvReqWarning(s32 index, const PatPacketHeader* header)
{
    u32 length;

    PAT_TRACE("%s:recvReqWarning ok\n");
    readUInt8(this, &warningKind_8BBC);
    readUInt32_(this, &warningValue_8BC0);
    if (replySize_8BC4 != 0) {
        readString(this, &length, (char*)replyBuffer_8BC8, replySize_8BC4 < 1024 ? replySize_8BC4 : 1024);
    }
    flushBuffer(this, 16, 0);
    encryptBuffer(this);
    requestState_6135 = 230;
    return 0;
}

/* Reads the 256-byte common key into its buffer. */
s32 PatInterface::recvAnsCommonKey(s32 index, const PatPacketHeader* header)
{
    u32 length;

    PAT_TRACE("%s:recvAnsCommonKey ok\n");
    if (commonKeyBuffer_8948 != NULL) {
        memset(commonKeyBuffer_8948, 0, 256);
        readUInt8Array(this, &length, commonKeyBuffer_8948, 256);
        commonKeyReady_894C = 1;
    }
    sessionState_6132 += 5;
    return 0;
}

/* Reads the login answer: the session flag, its message and the charge items. */
s32 PatInterface::recvAnsLoginInfo(s32 index, const PatPacketHeader* header)
{
    u32 length;

    PAT_TRACE("%s:recvAnsLoginInfo ok\n");
    readUInt8(this, &sessionReady_8950);
    readString(this, &length, termText_8951, 512);
    readChargeInfo(this, loginFields_82AC);
    connectionPhase_894F = sessionReady_8950;
    requestState_6135 += 5;
    return 0;
}

/* Reads the charge items of the login block. */
s32 PatInterface::recvAnsChargeInfo(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsChargeInfo ok\n");
    readChargeInfo(this, loginFields_82AC);
    requestState_6135 += 5;
    return 0;
}

/* Reads the user list's head: the number of rows to come. */
s32 PatInterface::recvAnsUserListHead(s32 index, const PatPacketHeader* header)
{
    s32 first;

    PAT_TRACE("%s:recvAnsUserListHead ok\n");
    readInt32(this, &first);
    readInt32(this, (s32*)&userRowCount_8BB4);
    requestState_6135 += 5;
    return 0;
}

/* Reads the user rows, shrinking the row stack to the count the server sends. */
s32 PatInterface::recvAnsUserListData(s32 index, const PatPacketHeader* header)
{
    s32 count;
    s32 first;

    PAT_TRACE("%s:recvAnsUserListData ok\n");
    readInt32(this, &first);
    readInt32(this, &count);
    if ((s32)userRowCount_8BB4 > count) {
        growStackSize(this, (userRowCount_8BB4 - count) * 92);
        userRowCount_8BB4 = count;
    }
    memset(userRows_8BB8, 0, userRowCount_8BB4 * 92);
    readUserObjects(this, (NetworkUserRow*)userRows_8BB8, userRowCount_8BB4);
    requestState_6135 += 5;
    return 0;
}

/* Ends the user list. */
s32 PatInterface::recvAnsUserListFoot(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsUserListFoot ok\n");
    requestState_6135 += 5;
    return 0;
}

/* Reads this client's own user row with the session flag and message. */
s32 PatInterface::recvAnsUserObject(s32 index, const PatPacketHeader* header)
{
    s32 value;
    u32 length;

    PAT_TRACE("%s:recvAnsUserObject ok\n");
    readUInt8(this, &sessionReady_8950);
    readString(this, &length, termText_8951, 512);
    readInt32(this, &value);
    readUserObjects(this, &userRow_8B54, 1);
    requestState_6135 += 5;
    return 0;
}

/* Reads the FMP list version and marks the list stale when it changed. */
s32 PatInterface::recvAnsFmpListVersion(s32 index, const PatPacketHeader* header)
{
    u32 version;

    PAT_TRACE("%s:recvAnsFmpListVersion ok\n");
    readUInt32_(this, &version);
    fmpListReady_6C3C = fmpListActive_6C38 != version;
    fmpListActive_6C38 = version;
    fmpState_6137 += 5;
    return 0;
}

/* Reads the FMP list's head: the number of slots to come. */
s32 PatInterface::recvAnsFmpListHead(s32 index, const PatPacketHeader* header)
{
    s32 first;

    PAT_TRACE("%s:recvAnsFmpListHead ok\n");
    readInt32(this, &first);
    readInt32(this, (s32*)&replyTotal_8BD0);
    fmpState_6137 += 5;
    return 0;
}

/* Reads FMP slots: the whole run into the table while the list is stale, else only the progress of known slots. */
s32 PatInterface::recvAnsFmpListData(s32 index, const PatPacketHeader* header)
{
    s32 count;
    s32 first;
    NetworkFmpSlot slot;
    u8 reserve[262];
    s32 i;
    s32 j;

    PAT_TRACE("%s:recvAnsFmpListData ok\n");
    readInt32(this, &first);
    readInt32(this, &count);
    if (count > (s32)(80 - replySent_8BCC)) {
        count = 80 - replySent_8BCC;
    }
    if (fmpListReady_6C3C != 0) {
        readFmpCompoundData(this, &fmpSlots_6C40[replySent_8BCC], reserve, count);
        fmpSlotCount_6608 += count;
    } else {
        for (i = 0; i < count; i++) {
            memset(&slot, 0, sizeof(NetworkFmpSlot));
            readFmpCompoundData(this, &slot, reserve, 1);
            if (slot.payload_00 != 0) {
                for (j = 0; j < (s32)fmpSlotCount_6608; j++) {
                    if (slot.payload_00 == fmpSlots_6C40[j].payload_00) {
                        fmpSlots_6C40[j].done_10 = slot.done_10;
                        fmpSlots_6C40[j].total_14 = slot.total_14;
                        break;
                    }
                }
            }
        }
    }
    replySent_8BCC += count;
    fmpState_6137 += 5;
    return 0;
}

/* Ends the FMP list. */
s32 PatInterface::recvAnsFmpListFoot(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsFmpListFoot ok\n");
    fmpState_6137 += 5;
    return 0;
}

/* Reads the chosen FMP slot's details and reports them to the session handlers. */
s32 PatInterface::recvAnsFmpInfo(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsFmpInfo ok\n");
    memset(fmpReserve_671A, 0, sizeof(fmpReserve_671A));
    readFmpCompoundData(this, &fmpSlots_6C40[fmpSelected_65F8], fmpReserve_671A, 1);
    dispatchSessionHandlers(this, 0x8008, header->requestId_02, header->status_07, 0, NULL);
    requestState_6135 += 5;
    return 0;
}

/* Reads the RFP server's address. */
s32 PatInterface::recvAnsRfpConnect(s32 index, const PatPacketHeader* header)
{
    u32 length;

    PAT_TRACE("%s:recvAnsRfpConnect ok\n");
    memset(&rfpServer_814E, 0, sizeof(PatServerAddress));
    readString(this, &length, rfpServer_814E.host_000, 256);
    readUInt16(this, &rfpServer_814E.port_104);
    rfpConnected_6610 = 1;
    requestState_6135 += 5;
    return 0;
}

/* Reads the lobby server's address. */
s32 PatInterface::recvAnsLmpConnect(s32 index, const PatPacketHeader* header)
{
    u32 length;

    PAT_TRACE("%s:recvAnsLmpConnect ok\n");
    memset(&lmpServer_6B32, 0, sizeof(PatServerAddress));
    readString(this, &length, lmpServer_6B32.host_000, 256);
    readUInt16(this, &lmpServer_6B32.port_104);
    sessionState_6132 += 5;
    return 0;
}

/* Reads the terms version (noting whether it changed) and the size of the terms body. */
s32 PatInterface::recvAnsTermsVersion(s32 index, const PatPacketHeader* header)
{
    u32 version;

    PAT_TRACE("%s:recvAnsTermsVersion ok\n");
    readUInt32_(this, &version);
    termsChanged_8268 = termsVersion_8264 != version;
    termsVersion_8264 = version;
    readUInt32_(this, &dataTotal_825C);
    sessionState_6132 += 5;
    return 0;
}

/* Reads one slice of the terms body. */
s32 PatInterface::recvAnsTerms(s32 index, const PatPacketHeader* header)
{
    s32 result;

    PAT_TRACE("%s:recvAnsTerms ok\n");
    result = readBodySlice(this, termsBufferPtr_828C, termsSize_826C);
    if (result < 0) {
        return result;
    }
    sessionState_6132 += 5;
    return 0;
}

/* Reads the maintenance text into its buffer. */
s32 PatInterface::recvAnsMaintenance(s32 index, const PatPacketHeader* header)
{
    u32 length;

    PAT_TRACE("%s:recvAnsMaintenance ok\n");
    readString(this, &length, (char*)maintenanceBuffer_8290, maintenanceSize_8270);
    sessionState_6132 += 5;
    return 0;
}

/* Reads the announcement text into its buffer. */
s32 PatInterface::recvAnsAnnounce(s32 index, const PatPacketHeader* header)
{
    u32 length;

    PAT_TRACE("%s:recvAnsAnnounce ok\n");
    readString(this, &length, (char*)announceBuffer_8294, announceSize_8274);
    sessionState_6132 += 5;
    return 0;
}

/* Reads the no-charge text into its buffer. */
s32 PatInterface::recvAnsNoCharge(s32 index, const PatPacketHeader* header)
{
    u32 length;

    PAT_TRACE("%s:recvAnsNoCharge ok\n");
    readString(this, &length, (char*)noChargeBuffer_8298, noChargeSize_8278);
    sessionState_6132 += 5;
    return 0;
}

/* Reads the media version, the patch message and the media version records. */
s32 PatInterface::recvAnsMediaVersionInfo(s32 index, const PatPacketHeader* header)
{
    u32 length;

    PAT_TRACE("%s:recvAnsMediaVersionInfo ok\n");
    readString(this, &length, mediaVersion_6588, 32);
    readString(this, &length, (char*)patchMessageBuffer_82A8, patchMessageSize_8288);
    memset(mediaVersionText_65A8, 0, 32);
    readMediaVersionData(this, NULL, 1);
    sessionState_6132 += 5;
    return 0;
}

/* Reads the high-level vulgarity list's slice descriptor and size. */
s32 PatInterface::recvAnsVulgarityInfoHigh(s32 index, const PatPacketHeader* header)
{
    u32 value;

    PAT_TRACE("%s:recvAnsVulgarityInfoHigh ok\n");
    readUInt32_(this, &value);
    readUInt32_(this, &sendSlice_8258);
    readUInt32_(this, &dataTotal_825C);
    sessionState_6132 += 5;
    return 0;
}

/* Reads one slice of the high-level vulgarity list. */
s32 PatInterface::recvAnsVulgarityHigh(s32 index, const PatPacketHeader* header)
{
    s32 result;
    u32 mode;

    PAT_TRACE("%s:recvAnsVulgarityHigh ok\n");
    readUInt32_(this, &mode);
    if (mode == 2) {
        result = readBodySlice(this, vulgarityHighBuffer_829C, vulgarityHighSize_827C);
        if (result < 0) {
            return result;
        }
    } else {
        result = readBodySlice(this, vulgarityHighBuffer_829C, vulgarityHighSize_827C);
        if (result < 0) {
            return result;
        }
    }
    sessionState_6132 += 5;
    return 0;
}

/* Reads the low-level vulgarity list's slice descriptor and size. */
s32 PatInterface::recvAnsVulgarityInfoLow(s32 index, const PatPacketHeader* header)
{
    u32 value;

    PAT_TRACE("%s:recvAnsVulgarityInfoLow ok\n");
    readUInt32_(this, &value);
    readUInt32_(this, &sendSlice_8258);
    readUInt32_(this, &dataTotal_825C);
    sessionState_6132 += 5;
    return 0;
}

/* Reads one slice of the low-level vulgarity list into the buffer its mode selects. */
s32 PatInterface::recvAnsVulgarityLow(s32 index, const PatPacketHeader* header)
{
    s32 result;
    u32 mode;

    PAT_TRACE("%s:recvAnsVulgarityLow ok\n");
    readUInt32_(this, &mode);
    if (mode == 2) {
        result = readBodySlice(this, vulgarityPtr_82A4, vulgaritySize_8284);
        if (result < 0) {
            return result;
        }
    } else {
        result = readBodySlice(this, userListPtr_82A0, userListSize_8280);
        if (result < 0) {
            return result;
        }
    }
    sessionState_6132 += 5;
    return 0;
}

/* Acknowledges the authentication token. */
s32 PatInterface::recvAnsAuthenticationToken(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsAuthenticationToken ok\n");
    sessionState_6132 += 5;
    return 0;
}

/* Reads a file's binary version and reports it to the session handlers. */
s32 PatInterface::recvAnsBinaryVersion(s32 index, const PatPacketHeader* header)
{
    u32 version;
    u8 fileId;

    PAT_TRACE("%s:recvAnsBinaryVersion ok\n");
    readUInt8(this, &fileId);
    readUInt32_(this, &version);
    dispatchSessionHandlers(this, 0x8009, header->requestId_02, header->status_07, 1, (const u8*)&version);
    return 0;
}

/* Reads a binary transfer's head: reported to the session handlers, or kept as the text transfer's size. */
s32 PatInterface::recvAnsBinaryHead(s32 index, const PatPacketHeader* header)
{
    u32 values[2];

    PAT_TRACE("%s:recvAnsBinaryHead ok\n");
    readUInt32_(this, &values[0]);
    readUInt32_(this, &values[1]);
    switch (binaryMode_611A) {
    case 5:
        dispatchSessionHandlers(this, 0x800A, header->requestId_02, header->status_07, 2, (const u8*)values);
        break;
    case 6:
        binaryTextReady_D408 = binarySize_D404 != values[0];
        binarySize_D404 = values[0];
        dataTotal_825C = values[1];
        requestState_6135 += 5;
        break;
    }
    return 0;
}

/* Reads one binary chunk onto the call stack: reported to the session handlers, or copied into the text buffer. */
s32 PatInterface::recvAnsBinaryData(s32 index, const PatPacketHeader* header)
{
    u32 stackSize;
    u32 length;
    PatBinaryChunk chunk;

    PAT_TRACE("%s:recvAnsBinaryData ok\n");
    readUInt32_(this, &chunk.fileHandle_0);
    readUInt32_(this, &chunk.offset_4);
    readUInt32_(this, &chunk.size_8);
    chunk.data_C = createStack(this, chunk.size_8, &stackSize);
    if (chunk.size_8 > stackSize) {
        chunk.size_8 = stackSize;
    }
    readUInt8Array(this, &length, chunk.data_C, chunk.size_8);
    switch (binaryMode_611A) {
    case 5:
        dispatchSessionHandlers(this, 0x800B, header->requestId_02, header->status_07, 1, (const u8*)&chunk);
        break;
    case 6:
        memset(binaryText_D409, 0, 512);
        if (chunk.offset_4 == 0) {
            binarySize_D404 = chunk.fileHandle_0;
            memcpy(binaryText_D409, chunk.data_C, chunk.size_8 < 511 ? chunk.size_8 : 511);
        }
        requestState_6135 += 5;
        break;
    }
    growStackSize(this, stackSize);
    return 0;
}

/* Ends a binary transfer. */
s32 PatInterface::recvAnsBinarFoot(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsBinaryFoot ok\n");
    switch (binaryMode_611A) {
    case 5:
        dispatchSessionHandlers(this, 0x800C, header->requestId_02, header->status_07, 0, NULL);
        break;
    case 6:
        requestState_6135 += 5;
        break;
    }
    return 0;
}

/* Reads the layer the session starts in and reports it. */
s32 PatInterface::recvAnsLayerStart(s32 index, const PatPacketHeader* header)
{
    PatLayerData layer;

    PAT_TRACE("%s:recvAnsLayerStart\n");
    memset(&layer, 0, sizeof(PatLayerData));
    readLayerData(this, &layer, 1);
    dispatchSessionHandlers(this, 0x800D, header->requestId_02, header->status_07, 1, (const u8*)&layer);
    return 0;
}

/* Reports the end of the layer session. */
s32 PatInterface::recvAnsLayerEnd(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsLayerEnd\n");
    dispatchSessionHandlers(this, 0x800E, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reads a layer's user count notice and reports it. */
s32 PatInterface::recvNtcLayerUserNum(s32 index, const PatPacketHeader* header)
{
    PatLayerInfo info;

    PAT_TRACE("%s:recvNtcLayerUserNum\n");
    memset(&info, 0, sizeof(PatLayerInfo));
    readUInt8(this, &info.kind_000);
    readLayerCountData(this, &info.layer_004, 1);
    dispatchSessionHandlers(this, 0x800F, NULL, header->status_07, 1, (const u8*)&info);
    return 0;
}

/* Reports the answer to a layer jump. */
s32 PatInterface::recvAnsLayerJump(s32 index, const PatPacketHeader* header)
{
    layerMovePending_D62C = 0;
    PAT_TRACE("%s:recvAnsLayerJump\n");
    dispatchSessionHandlers(this, 0x8010, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reads the created layer's number and reports the head of a layer creation. */
s32 PatInterface::recvAnsLayerCreateHead(s32 index, const PatPacketHeader* header)
{
    s16 layerId;

    PAT_TRACE("%s:recvAnsLayerCreateHead\n");
    readShortMinusOne(this, &layerId);
    dispatchSessionHandlers(this, 0x8011, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reads the layer's number and reports a layer creation's settings. */
s32 PatInterface::recvAnsLayerCreateSet(s32 index, const PatPacketHeader* header)
{
    s16 layerId;

    PAT_TRACE("%s:recvAnsLayerCreateSet\n");
    readShortMinusOne(this, &layerId);
    dispatchSessionHandlers(this, 0x8012, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reads the layer's number and reports the end of a layer creation. */
s32 PatInterface::recvAnsLayerCreateFoot(s32 index, const PatPacketHeader* header)
{
    s16 layerId;

    PAT_TRACE("%s:recvAnsLayerCreateFoot\n");
    readShortMinusOne(this, &layerId);
    dispatchSessionHandlers(this, 0x8013, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reads the layer moved down into and reports it. */
s32 PatInterface::recvAnsLayerDown(s32 index, const PatPacketHeader* header)
{
    s16 layerId;

    layerMovePending_D62C = 0;
    PAT_TRACE("%s:recvAnsLayerDown\n");
    readShortMinusOne(this, &layerId);
    dispatchSessionHandlers(this, 0x8014, header->requestId_02, header->status_07, 1, (const u8*)&layerId);
    return 0;
}

/* Reads the user who entered the layer and reports it. */
s32 PatInterface::recvNtcLayerIn(s32 index, const PatPacketHeader* header)
{
    u32 length;
    PatLayerUser user;

    PAT_TRACE("%s:recvNtcLayerIn\n");
    memset(&user, 0, sizeof(PatLayerUser));
    readString(this, &length, user.userId_000, 8);
    readLayerUserData(this, &user, 1);
    dispatchSessionHandlers(this, 0x8015, NULL, header->status_07, 1, (const u8*)&user);
    return 0;
}

/* Reports the answer to moving up a layer. */
s32 PatInterface::recvAnsLayerUp(s32 index, const PatPacketHeader* header)
{
    layerMovePending_D62C = 0;
    PAT_TRACE("%s:recvAnsLayerUp\n");
    dispatchSessionHandlers(this, 0x8016, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reads the user who left the layer and reports it. */
s32 PatInterface::recvNtcLayerOut(s32 index, const PatPacketHeader* header)
{
    u32 length;
    PatLayerUser user;

    PAT_TRACE("%s:recvNtcLayerOut\n");
    memset(&user, 0, sizeof(PatLayerUser));
    readString(this, &length, user.userId_000, 8);
    dispatchSessionHandlers(this, 0x8017, NULL, header->status_07, 1, (const u8*)&user);
    return 0;
}

/* Reads the layer jump's ready notice and reports it. */
s32 PatInterface::recvNtcLayerJumpReady(s32 index, const PatPacketHeader* header)
{
    u32 value;

    PAT_TRACE("%s:recvNtcLayerJumpReady\n");
    readUInt32_(this, &value);
    dispatchSessionHandlers(this, 0x8018, NULL, header->status_07, 1, (const u8*)&value);
    return 0;
}

/* Reports the layer jump's go notice. */
s32 PatInterface::recvNtcLayerJumpGo(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvNtcLayerJumpGo\n");
    dispatchSessionHandlers(this, 0x8019, NULL, header->status_07, 0, NULL);
    return 0;
}

/* Reports the answer to a layer setting. */
s32 PatInterface::recvAnsLayerInfoSe(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsLayerInfoSet\n");
    dispatchSessionHandlers(this, 0x801A, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reads a layer's changed settings and reports them. */
s32 PatInterface::recvNtcLayerInfoSet(s32 index, const PatPacketHeader* header)
{
    PatLayerInfo info;

    PAT_TRACE("%s:recvNtcLayerInfoSet\n");
    memset(&info, 0, sizeof(PatLayerInfo));
    readUnkShortArrayStruct(this, info.layer_004.path_004);
    readLayerData(this, &info.layer_004, 1);
    readUnkByteIntStruct(this, &info.tags_244, 1);
    dispatchSessionHandlers(this, 0x801B, NULL, header->status_07, 1, (const u8*)&info);
    return 0;
}

/* Reads a layer's settings and reports them as the current layer's or another one's. */
s32 PatInterface::recvAnsLayerInfo(s32 index, const PatPacketHeader* header)
{
    PatLayerInfo info;

    PAT_TRACE("%s:recvAnsLayerInfo\n");
    memset(&info, 0, sizeof(PatLayerInfo));
    readUnkShortArrayStruct(this, info.layer_004.path_004);
    readLayerData(this, &info.layer_004, 1);
    readUnkByteIntStruct(this, &info.tags_244, 1);
    if (info.layer_004.isCurrent_07D != 0) {
        dispatchSessionHandlers(this, 0x801C, header->requestId_02, header->status_07, 1, (const u8*)&info);
    } else {
        dispatchSessionHandlers(this, 0x801D, header->requestId_02, header->status_07, 1, (const u8*)&info);
    }
    return 0;
}

/* Reads the parent layer's settings and reports them. */
s32 PatInterface::recvAnsLayerParentInfo(s32 index, const PatPacketHeader* header)
{
    PatLayerInfo info;

    PAT_TRACE("%s:recvAnsLayerParentInfo\n");
    memset(&info, 0, sizeof(PatLayerInfo));
    readLayerData(this, &info.layer_004, 1);
    readUnkByteIntStruct(this, &info.tags_244, 1);
    dispatchSessionHandlers(this, 0x801D, header->requestId_02, header->status_07, 1, (const u8*)&info);
    return 0;
}

/* Reads a child layer's number and settings and reports them. */
s32 PatInterface::recvAnsLayerChildInfo(s32 index, const PatPacketHeader* header)
{
    s16 layerId;
    PatLayerInfo info;

    PAT_TRACE("%s:recvAnsLayerChildInfo\n");
    memset(&info, 0, sizeof(PatLayerInfo));
    readShortMinusOne(this, &layerId);
    readLayerData(this, &info.layer_004, 1);
    info.layer_004.layerId_054 = layerId;
    readUnkByteIntStruct(this, &info.tags_244, 1);
    dispatchSessionHandlers(this, 0x801E, header->requestId_02, header->status_07, 1, (const u8*)&info);
    return 0;
}

/* Reads the child layer list's head (first index and count) and reports it. */
s32 PatInterface::recvAnsLayerChildListHead(s32 index, const PatPacketHeader* header)
{
    s32 values[2];

    PAT_TRACE("%s:recvAnsLayerChildListHead\n");
    readInt32(this, &values[0]);
    readInt32(this, &values[1]);
    dispatchSessionHandlers(this, 0x801F, header->requestId_02, header->status_07, 2, (const u8*)values);
    return 0;
}

/* Reads child layer rows onto the call stack and reports them. */
s32 PatInterface::recvAnsLayerChildListData(s32 index, const PatPacketHeader* header)
{
    u32 stackSize;
    u32 maxCount;
    s32 values[2];
    PatLayerInfo* info;
    PatLayerInfo* list;
    s32 i;

    PAT_TRACE("%s:recvAnsLayerChildListData\n");
    readInt32(this, &values[0]);
    readInt32(this, &values[1]);
    list = (PatLayerInfo*)createStack(this, values[1] * sizeof(PatLayerInfo), &stackSize);
    maxCount = stackSize / sizeof(PatLayerInfo);
    if (values[1] > (s32)maxCount) {
        values[1] = maxCount;
    }
    for (i = 0, info = list; i < values[1]; info++, i++) {
        readLayerData(this, &info->layer_004, 1);
        readUnkByteIntStruct(this, &info->tags_244, 1);
    }
    dispatchSessionHandlers(this, 0x8020, header->requestId_02, header->status_07, values[1], (const u8*)list);
    growStackSize(this, stackSize);
    return 0;
}

/* Reports the end of the child layer list. */
s32 PatInterface::recvAnsLayerChildListFoot(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsLayerChildListFoot\n");
    dispatchSessionHandlers(this, 0x8021, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reads the sibling layer list's head (first index and count) and reports it. */
s32 PatInterface::recvAnsLayerSiblingListHead(s32 index, const PatPacketHeader* header)
{
    s32 values[2];

    PAT_TRACE("%s:recvAnsLayerSiblingListHead\n");
    readInt32(this, &values[0]);
    readInt32(this, &values[1]);
    dispatchSessionHandlers(this, 0x8022, header->requestId_02, header->status_07, 2, (const u8*)values);
    return 0;
}

/* Reads sibling layer rows onto the call stack and reports them. */
s32 PatInterface::recvAnsLayerSiblingListData(s32 index, const PatPacketHeader* header)
{
    u32 stackSize;
    u32 maxCount;
    s32 values[2];
    PatLayerInfo* info;
    PatLayerInfo* list;
    s32 i;

    PAT_TRACE("%s:recvAnsLayerSiblingListData\n");
    readInt32(this, &values[0]);
    readInt32(this, &values[1]);
    list = (PatLayerInfo*)createStack(this, values[1] * sizeof(PatLayerInfo), &stackSize);
    maxCount = stackSize / sizeof(PatLayerInfo);
    if (values[1] > (s32)maxCount) {
        values[1] = maxCount;
    }
    for (i = 0, info = list; i < values[1]; info++, i++) {
        readLayerData(this, &info->layer_004, 1);
        readUnkByteIntStruct(this, &info->tags_244, 1);
    }
    dispatchSessionHandlers(this, 0x8023, header->requestId_02, header->status_07, values[1], (const u8*)list);
    growStackSize(this, stackSize);
    return 0;
}

/* Reports the end of the sibling layer list. */
s32 PatInterface::recvAnsLayerSiblingListFoot(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsLayerSiblingListFoot\n");
    dispatchSessionHandlers(this, 0x8024, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reads the layer host (path, id and name) and reports it. */
s32 PatInterface::recvAnsLayerHost(s32 index, const PatPacketHeader* header)
{
    u32 length;
    PatLayerUserInfo host;

    PAT_TRACE("%s:recvAnsLayerHost\n");
    memset(&host, 0, sizeof(PatLayerUserInfo));
    readUnkShortArrayStruct(this, host.user_000.path_028);
    readString(this, &length, host.user_000.userId_000, 8);
    readString(this, &length, host.user_000.name_008, 32);
    dispatchSessionHandlers(this, 0x8025, header->requestId_02, header->status_07, 1, (const u8*)&host);
    return 0;
}

/* Reads a new layer host (path, id and name) and reports it. */
s32 PatInterface::recvNtcLayerHost(s32 index, const PatPacketHeader* header)
{
    u32 length;
    PatLayerUserInfo host;

    PAT_TRACE("%s:recvNtcLayerHost\n");
    memset(&host, 0, sizeof(PatLayerUserInfo));
    readUnkShortArrayStruct(this, host.user_000.path_028);
    readString(this, &length, host.user_000.userId_000, 8);
    readString(this, &length, host.user_000.name_008, 32);
    dispatchSessionHandlers(this, 0x8026, NULL, header->status_07, 1, (const u8*)&host);
    return 0;
}

/* Reports the answer to a layer user setting. */
s32 PatInterface::recvAnsLayerUserInfoSet(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsLayerUserInfoSet\n");
    dispatchSessionHandlers(this, 0x8027, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reads a layer user's changed settings and reports them. */
s32 PatInterface::recvNtcLayerUserInfoSet(s32 index, const PatPacketHeader* header)
{
    u32 length;
    PatLayerUser user;

    PAT_TRACE("%s:recvNtcLayerUserInfoSet\n");
    memset(&user, 0, sizeof(PatLayerUser));
    readString(this, &length, user.userId_000, 8);
    readLayerUserData(this, &user, 1);
    dispatchSessionHandlers(this, 0x8028, NULL, header->status_07, 1, (const u8*)&user);
    return 0;
}

/* Reads the layer's users onto the call stack and reports them. */
s32 PatInterface::recvAnsLayerUserList(s32 index, const PatPacketHeader* header)
{
    s32 count;
    u32 stackSize;
    u32 maxCount;
    PatLayerUser* list;

    PAT_TRACE("%s:recvAnsLayerUserList\n");
    readInt32(this, &count);
    list = (PatLayerUser*)createStack(this, count * sizeof(PatLayerUser), &stackSize);
    maxCount = stackSize / sizeof(PatLayerUser);
    if (count > (s32)maxCount) {
        count = maxCount;
    }
    readLayerUserData(this, list, count);
    dispatchSessionHandlers(this, 0x8029, header->requestId_02, header->status_07, count, (const u8*)list);
    growStackSize(this, stackSize);
    return 0;
}

/* Reads the layer user list's head (first index and count) and reports it. */
s32 PatInterface::recvAnsLayerUserListHead(s32 index, const PatPacketHeader* header)
{
    s32 values[2];

    PAT_TRACE("%s:recvAnsLayerUserListHead\n");
    readInt32(this, &values[0]);
    readInt32(this, &values[1]);
    dispatchSessionHandlers(this, 0x802A, header->requestId_02, header->status_07, 2, (const u8*)values);
    return 0;
}

/* Reads layer user rows onto the call stack and reports them. */
s32 PatInterface::recvAnsLayerUserListData(s32 index, const PatPacketHeader* header)
{
    u32 stackSize;
    u32 maxCount;
    s32 values[2];
    PatLayerUserInfo* info;
    PatLayerUserInfo* list;
    s32 i;

    PAT_TRACE("%s:recvAnsLayerUserListData\n");
    readInt32(this, &values[0]);
    readInt32(this, &values[1]);
    list = (PatLayerUserInfo*)createStack(this, values[1] * sizeof(PatLayerUserInfo), &stackSize);
    maxCount = stackSize / sizeof(PatLayerUserInfo);
    if (values[1] > (s32)maxCount) {
        values[1] = maxCount;
    }
    for (i = 0, info = list; i < values[1]; info++, i++) {
        readLayerUserData(this, &info->user_000, 1);
        readUnkByteIntStruct(this, &info->tags_140, 1);
    }
    dispatchSessionHandlers(this, 0x802B, header->requestId_02, header->status_07, values[1], (const u8*)list);
    growStackSize(this, stackSize);
    return 0;
}

/* Reports the end of the layer user list. */
s32 PatInterface::recvAnsLayerUserListFoot(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsLayerUserListFoot\n");
    dispatchSessionHandlers(this, 0x802C, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reads the layer user search's head (first index and count) and reports it. */
s32 PatInterface::recvAnsLayerUserSearchHead(s32 index, const PatPacketHeader* header)
{
    s32 values[2];

    PAT_TRACE("%s:recvAnsLayerUserSearchHead\n");
    readInt32(this, &values[0]);
    readInt32(this, &values[1]);
    dispatchSessionHandlers(this, 0x802D, header->requestId_02, header->status_07, 2, (const u8*)values);
    return 0;
}

/* Reads the layer user search's rows onto the call stack and reports them. */
s32 PatInterface::recvAnsLayerUserSearchData(s32 index, const PatPacketHeader* header)
{
    u32 stackSize;
    u32 maxCount;
    s32 values[2];
    PatLayerUserInfo* info;
    PatLayerUserInfo* list;
    s32 i;

    PAT_TRACE("%s:recvAnsLayerUserSearchData\n");
    readInt32(this, &values[0]);
    readInt32(this, &values[1]);
    list = (PatLayerUserInfo*)createStack(this, values[1] * sizeof(PatLayerUserInfo), &stackSize);
    maxCount = stackSize / sizeof(PatLayerUserInfo);
    if (values[1] > (s32)maxCount) {
        values[1] = maxCount;
    }
    for (i = 0, info = list; i < values[1]; info++, i++) {
        readLayerUserData(this, &info->user_000, 1);
        readUnkByteIntStruct(this, &info->tags_140, 1);
    }
    dispatchSessionHandlers(this, 0x802E, header->requestId_02, header->status_07, values[1], (const u8*)list);
    growStackSize(this, stackSize);
    return 0;
}

/* Reports the end of the layer user search. */
s32 PatInterface::recvAnsLayerUserSearchFoot(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsLayerUserSearchFoot\n");
    dispatchSessionHandlers(this, 0x802F, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reads a layer chat (the sender and the text) and reports it. */
s32 PatInterface::recvNtcLayerChat(s32 index, const PatPacketHeader* header)
{
    u32 length;
    u8 kind;
    PatChatMessage chat;

    PAT_TRACE("%s:recvNtcLayerChat\n");
    memset(&chat, 0, sizeof(PatChatMessage));
    readUInt8(this, &kind);
    readChatData(this, &chat.sender_100, 1);
    readString(this, &length, chat.text_000, 256);
    dispatchSessionHandlers(this, 0x8035, NULL, header->status_07, 1, (const u8*)&chat);
    return 0;
}

/* Reports the answer to a layer tell. */
s32 PatInterface::recvAnsLayerTell(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsLayerTell\n");
    dispatchSessionHandlers(this, 0x8036, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reads a layer tell (the sender's id and items, the text) and reports it. */
s32 PatInterface::recvNtcLayerTell(s32 index, const PatPacketHeader* header)
{
    u32 length;
    PatChatMessage chat;

    PAT_TRACE("%s:recvNtcLayerTell\n");
    memset(&chat, 0, sizeof(PatChatMessage));
    readString(this, &length, chat.sender_100.userId_08, 8);
    readChatData(this, &chat.sender_100, 1);
    readString(this, &length, chat.text_000, 256);
    dispatchSessionHandlers(this, 0x8037, NULL, header->status_07, 1, (const u8*)&chat);
    return 0;
}

/* Reads a low-priority layer tell and reports it. */
s32 PatInterface::recvNtcLayerTellLow(s32 index, const PatPacketHeader* header)
{
    u32 length;
    PatChatMessage chat;

    PAT_TRACE("%s:recvNtcLayerTellLow\n");
    memset(&chat, 0, sizeof(PatChatMessage));
    readString(this, &length, chat.sender_100.userId_08, 8);
    readChatData(this, &chat.sender_100, 1);
    readString(this, &length, chat.text_000, 256);
    dispatchSessionHandlers(this, 0x8038, NULL, header->status_07, 1, (const u8*)&chat);
    return 0;
}

/* Reads the answer to a mediation lock and reports it. */
s32 PatInterface::recvAnsLayerMediationLock(s32 index, const PatPacketHeader* header)
{
    u8 value;

    PAT_TRACE("%s:recvAnsLayerMediationLock\n");
    readUInt8(this, &value);
    dispatchSessionHandlers(this, 0x8039, header->requestId_02, header->status_07, 1, &value);
    return 0;
}

/* Reads a mediation lock notice and reports it. */
s32 PatInterface::recvNtcLayerMediationLock(s32 index, const PatPacketHeader* header)
{
    u32 length;
    PatMediation entry;

    PAT_TRACE("%s:recvNtcLayerMediationLock\n");
    memset(&entry, 0, sizeof(PatMediation));
    readString(this, &length, entry.userId_0, 8);
    readUInt8(this, &entry.value_8);
    readMediationData(this, &entry, 1);
    dispatchSessionHandlers(this, 0x803A, NULL, header->status_07, 1, (const u8*)&entry);
    return 0;
}

/* Reads the answer to a mediation unlock and reports it. */
s32 PatInterface::recvAnsLayerMediationUnlock(s32 index, const PatPacketHeader* header)
{
    u8 value;

    PAT_TRACE("%s:recvAnsLayerMediationUnlock\n");
    readUInt8(this, &value);
    dispatchSessionHandlers(this, 0x803B, header->requestId_02, header->status_07, 1, &value);
    return 0;
}

/* Reads a mediation unlock notice (the user id is restored after the entry's items) and reports it. */
s32 PatInterface::recvNtcLayerMediationUnlock(s32 index, const PatPacketHeader* header)
{
    u32 length;
    char userId[8];
    PatMediation entry;

    PAT_TRACE("%s:recvNtcLayerMediationUnlock\n");
    memset(&entry, 0, sizeof(PatMediation));
    readString(this, &length, userId, 8);
    readUInt8(this, &entry.value_8);
    readMediationData(this, &entry, 1);
    memcpy(entry.userId_0, userId, 8);
    dispatchSessionHandlers(this, 0x803C, NULL, header->status_07, 1, (const u8*)&entry);
    return 0;
}

/* Reads the mediation list (at most 32 entries) and reports it. */
s32 PatInterface::recvAnsLayerMediationList(s32 index, const PatPacketHeader* header)
{
    u8 count;
    u8 kind;
    s32 n;
    PatMediation entries[32];

    PAT_TRACE("%s:recvAnsLayerMediationList\n");
    memset(entries, 0, sizeof(entries));
    readUInt8(this, &kind);
    readUInt8(this, &count);
    n = (s32)count < 32 ? (s32)count : 32;
    readMediationData(this, entries, n);
    dispatchSessionHandlers(this, 0x803D, header->requestId_02, header->status_07, n, (const u8*)entries);
    return 0;
}

/* Reads the detail search's head (first index, count, total) and reports the first two. */
s32 PatInterface::recvAnsLayerDetailSearchHead(s32 index, const PatPacketHeader* header)
{
    s32 total;
    s32 values[2];

    PAT_TRACE("%s:recvAnsLayerDetailSearchHead\n");
    readInt32(this, &values[0]);
    readInt32(this, &values[1]);
    readInt32(this, &total);
    dispatchSessionHandlers(this, 0x803E, header->requestId_02, header->status_07, 2, (const u8*)values);
    return 0;
}

/* Reports the end of the detail search. */
s32 PatInterface::recvAnsLayerDetailSearchFoot(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsLayerDetailSearchFoot\n");
    dispatchSessionHandlers(this, 0x8040, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reads the created circle's id and reports it. */
s32 PatInterface::recvAnsCircleCreate(s32 index, const PatPacketHeader* header)
{
    s32 circleId;

    PAT_TRACE("%s:recvAnsCircleCreate ok\n");
    readInt32(this, &circleId);
    dispatchSessionHandlers(this, 0x8042, header->requestId_02, header->status_07, 1, (const u8*)&circleId);
    return 0;
}

/* Reads one circle's info and tag list and reports it. */
s32 PatInterface::recvAnsCircleInfo(s32 index, const PatPacketHeader* header)
{
    PatCircleEntry circle;

    PAT_TRACE("%s:recvAnsCircleInfo ok\n");
    memset(&circle, 0, sizeof(PatCircleEntry));
    readInt32(this, &circle.circleId_000);
    readCircleInfoDataArray(this, &circle, 1);
    readUnkByteIntStruct(this, &circle.tags_37C, 1);
    dispatchSessionHandlers(this, 0x8043, header->requestId_02, header->status_07, 1, (const u8*)&circle);
    return 0;
}

/* Reads the joined circle and this member's slot and reports them. */
s32 PatInterface::recvAnsCircleJoin(s32 index, const PatPacketHeader* header)
{
    PatCircleUser user;

    PAT_TRACE("%s:recvAnsCircleJoin ok\n");
    memset(&user, 0, sizeof(PatCircleUser));
    readInt32(this, &user.circleId_00);
    readCircleSlot(this, &user.slot_04);
    dispatchSessionHandlers(this, 0x8044, header->requestId_02, header->status_07, 1, (const u8*)&user);
    return 0;
}

/* Reads a member who joined the circle (id, name, slot, state) and reports it. */
s32 PatInterface::recvNtcCircleJoin(s32 index, const PatPacketHeader* header)
{
    u32 length;
    PatCircleUser user;

    PAT_TRACE("%s:recvNtcCircleJoin\n");
    memset(&user, 0, sizeof(PatCircleUser));
    readInt32(this, &user.circleId_00);
    readString(this, &length, user.userId_06, 8);
    readString(this, &length, user.name_0E, 32);
    readCircleSlot(this, &user.slot_04);
    readUInt8(this, &user.state_05);
    dispatchSessionHandlers(this, 0x8045, NULL, header->status_07, 1, (const u8*)&user);
    return 0;
}

/* Reads the left circle's id and reports it. */
s32 PatInterface::recvAnsCircleLeave(s32 index, const PatPacketHeader* header)
{
    s32 value;

    PAT_TRACE("%s:recvAnsCircleLeave ok\n");
    readInt32(this, &value);
    dispatchSessionHandlers(this, 0x8046, header->requestId_02, header->status_07, 1, (const u8*)&value);
    return 0;
}

/* Reads a member who left the circle (id, slot, state) and reports it. */
s32 PatInterface::recvNtcCircleLeave(s32 index, const PatPacketHeader* header)
{
    u32 length;
    PatCircleUser user;

    PAT_TRACE("%s:recvNtcCircleLeave\n");
    memset(&user, 0, sizeof(PatCircleUser));
    readInt32(this, &user.circleId_00);
    readString(this, &length, user.userId_06, 8);
    readCircleSlot(this, &user.slot_04);
    readUInt8(this, &user.state_05);
    dispatchSessionHandlers(this, 0x8047, NULL, header->status_07, 1, (const u8*)&user);
    return 0;
}

/* Reads the broken-up circle's id and reports it. */
s32 PatInterface::recvAnsCircleBreak(s32 index, const PatPacketHeader* header)
{
    s32 value;

    PAT_TRACE("%s:recvAnsCircleBreak ok\n");
    readInt32(this, &value);
    dispatchSessionHandlers(this, 0x8048, header->requestId_02, header->status_07, 1, (const u8*)&value);
    return 0;
}

/* Reads the id of a circle that broke up and reports it. */
s32 PatInterface::recvNtcCircleBreak(s32 index, const PatPacketHeader* header)
{
    s32 value;

    PAT_TRACE("%s:recvNtcCircleBreak ok\n");
    readInt32(this, &value);
    dispatchSessionHandlers(this, 0x8049, NULL, header->status_07, 1, (const u8*)&value);
    return 0;
}

/* Reports the answer to a match option setting. */
s32 PatInterface::recvAnsCircleMatchOptionSet(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsCircleMatchOptionSet ok\n");
    dispatchSessionHandlers(this, 0x804A, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reads a member's changed match options and reports them. */
s32 PatInterface::recvNtcCircleMatchOptionSet(s32 index, const PatPacketHeader* header)
{
    PatCircleMatch member;

    PAT_TRACE("%s:recvNtcCircleMatchOptionSet\n");
    memset(&member, 0, sizeof(PatCircleMatch));
    readCircleMatchData(this, &member, 1);
    dispatchSessionHandlers(this, 0x804B, NULL, header->status_07, 1, (const u8*)&member);
    return 0;
}

/* Reads a member's id and match options and reports them. */
s32 PatInterface::recvAnsCircleMatchOptionGet(s32 index, const PatPacketHeader* header)
{
    u32 length;
    PatCircleMatch member;

    PAT_TRACE("%s:recvAnsCircleMatchOptionGet\n");
    memset(&member, 0, sizeof(PatCircleMatch));
    readString(this, &length, member.userId_08, 8);
    readCircleMatchData(this, &member, 1);
    dispatchSessionHandlers(this, 0x804C, header->requestId_02, header->status_07, 1, (const u8*)&member);
    return 0;
}

/* Reports the answer to a match start. */
s32 PatInterface::recvAnsCircleMatchStart(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsCircleMatchStart ok\n");
    dispatchSessionHandlers(this, 0x804D, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reads the match start notice (the members' slots, ids, flags and values, an optional tail) and reports it. */
s32 PatInterface::recvNtcCircleMatchStart(s32 index, const PatPacketHeader* header)
{
    u16 remaining;
    u32 length;
    PatMatchStart start;
    PatCircleMatch members[4];
    PatCircleMatch* member;
    s32 i;

    PAT_TRACE("%s:recvNtcCircleMatchStart ok\n");
    memset(&start, 0, sizeof(PatMatchStart));
    memset(members, 0, sizeof(members));
    member = members;
    start.members_04 = member;
    beginReadBlock(this, &remaining);
    readInt32(this, &start.count_00);
    remaining -= 4;
    if (start.count_00 < 1 || start.count_00 > 4) {
        PAT_KEEP_FAILURE();
        return -1;
    }
    memset(member, 0, sizeof(members));
    for (i = 0; i < start.count_00; i++) {
        readCircleSlot(this, &member->slot_07);
        remaining -= 1;
        readString(this, &length, member->userId_08, 8);
        remaining -= (u16)(length + 2);
        readUInt8Array(this, &length, &member->flag_00, 1);
        remaining -= (u16)(length + 2);
        readUInt16(this, &member->value_04);
        remaining -= 2;
        member++;
    }
    if (remaining != 0) {
        readUInt8(this, &start.flag_08);
        readUInt32_(this, &start.values_0C[0]);
        readUInt32_(this, &start.values_0C[1]);
        readUInt32_(this, &start.values_0C[2]);
        readUInt32_(this, &start.values_0C[3]);
    }
    endReadBlock(this);
    readInt32(this, &matchStartValue_D630);
    dispatchSessionHandlers(this, 0x804E, NULL, header->status_07, start.count_00, (const u8*)&start);
    return 0;
}

/* Reports the answer to a match end. */
s32 PatInterface::recvAnsCircleMatchEnd(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsCircleMatchEnd ok\n");
    dispatchSessionHandlers(this, 0x804F, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reads the circle whose settings changed and reports it. */
s32 PatInterface::recvAnsCircleInfoSet(s32 index, const PatPacketHeader* header)
{
    s32 value;

    PAT_TRACE("%s:recvAnsCircleInfoSet ok\n");
    readInt32(this, &value);
    dispatchSessionHandlers(this, 0x8051, header->requestId_02, header->status_07, 1, (const u8*)&value);
    return 0;
}

/* Reads a circle's changed info and tag list and reports it. */
s32 PatInterface::recvNtcCircleInfoSet(s32 index, const PatPacketHeader* header)
{
    PatCircleEntry circle;

    PAT_TRACE("%s:recvNtcCircleInfoSet ok\n");
    memset(&circle, 0, sizeof(PatCircleEntry));
    readInt32(this, &circle.circleId_000);
    readCircleInfoDataArray(this, &circle, 1);
    readUnkByteIntStruct(this, &circle.tags_37C, 1);
    dispatchSessionHandlers(this, 0x8052, NULL, header->status_07, 1, (const u8*)&circle);
    return 0;
}

/* Reads the layer's circles onto the call stack and reports them. */
s32 PatInterface::recvAnsCircleListLayer(s32 index, const PatPacketHeader* header)
{
    u32 stackSize;
    u32 maxCount;
    s32 values[2];
    PatCircleEntry* info;
    PatCircleEntry* list;
    s32 i;

    PAT_TRACE("%s:recvAnsCircleListLayer ok\n");
    readInt32(this, &values[0]);
    readInt32(this, &values[1]);
    list = (PatCircleEntry*)createStack(this, values[1] * sizeof(PatCircleEntry), &stackSize);
    maxCount = stackSize / sizeof(PatCircleEntry);
    if (values[1] > (s32)maxCount) {
        values[1] = maxCount;
    }
    for (i = 0, info = list; i < values[1]; info++, i++) {
        readCircleInfoDataArray(this, info, 1);
        readUnkByteIntStruct(this, &info->tags_37C, 1);
    }
    dispatchSessionHandlers(this, 0x8053, header->requestId_02, header->status_07, values[1], (const u8*)list);
    growStackSize(this, stackSize);
    return 0;
}

/* Reads the circle search's head (first index and count) and reports it. */
s32 PatInterface::recvAnsCircleSearchHead(s32 index, const PatPacketHeader* header)
{
    s32 values[2];

    PAT_TRACE("%s:recvAnsCircleSearchHead ok\n");
    readInt32(this, &values[0]);
    readInt32(this, &values[1]);
    dispatchSessionHandlers(this, 0x8054, header->requestId_02, header->status_07, 2, (const u8*)values);
    return 0;
}

/* Reads the circle search's rows onto the call stack and reports them. */
s32 PatInterface::recvAnsCircleSearchData(s32 index, const PatPacketHeader* header)
{
    u32 stackSize;
    u32 maxCount;
    s32 values[2];
    PatCircleEntry* info;
    PatCircleEntry* list;
    s32 i;

    PAT_TRACE("%s:recvAnsCircleSearchData ok\n");
    readInt32(this, &values[0]);
    readInt32(this, &values[1]);
    list = (PatCircleEntry*)createStack(this, values[1] * sizeof(PatCircleEntry), &stackSize);
    maxCount = stackSize / sizeof(PatCircleEntry);
    if (values[1] > (s32)maxCount) {
        values[1] = maxCount;
    }
    for (i = 0, info = list; i < values[1]; info++, i++) {
        readCircleInfoDataArray(this, info, 1);
        readUnkByteIntStruct(this, &info->tags_37C, 1);
    }
    dispatchSessionHandlers(this, 0x8055, header->requestId_02, header->status_07, values[1], (const u8*)list);
    growStackSize(this, stackSize);
    return 0;
}

/* Reports the end of the circle search. */
s32 PatInterface::recvAnsCircleSearchFoot(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsCircleSearchFoot ok\n");
    dispatchSessionHandlers(this, 0x8056, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reports the answer to a kick. */
s32 PatInterface::recvAnsCircleKick(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsCircleKick\n");
    dispatchSessionHandlers(this, 0x8057, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reads a kick notice (its kind and byte string) and reports it. */
s32 PatInterface::recvNtcCircleKick(s32 index, const PatPacketHeader* header)
{
    u32 length;
    PatKickList kick;

    PAT_TRACE("%s:recvNtcCircleKick\n");
    memset(&kick, 0, sizeof(PatKickList));
    readUInt8(this, &kick.kind_000);
    readUInt8Array(this, &length, kick.data_002, 256);
    kick.size_102 = (u16)length;
    dispatchSessionHandlers(this, 0x8058, NULL, header->status_07, 1, (const u8*)&kick);
    return 0;
}

/* Reports the answer to clearing the kick list. */
s32 PatInterface::recvAnsCircleDeleteKickList(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsCircleDeleteKickList\n");
    dispatchSessionHandlers(this, 0x8059, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reports the answer to a host handover. */
s32 PatInterface::recvAnsCircleHostHandover(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsCircleHostHandover ok\n");
    dispatchSessionHandlers(this, 0x805A, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reads the new host of a handover (circle, slot, id, name, flag) and reports it. */
s32 PatInterface::recvNtcCircleHostHandover(s32 index, const PatPacketHeader* header)
{
    u32 length;
    u8 flag;
    PatCircleUser user;

    PAT_TRACE("%s:recvNtcCircleHostHandover ok\n");
    memset(&user, 0, sizeof(PatCircleUser));
    readInt32(this, &user.circleId_00);
    readCircleSlot(this, &user.slot_04);
    readString(this, &length, user.userId_06, 8);
    readString(this, &length, user.name_0E, 32);
    readUInt8Array(this, &length, &flag, 1);
    dispatchSessionHandlers(this, 0x805B, NULL, header->status_07, 1, (const u8*)&user);
    return 0;
}

/* Reads the circle's host (circle, slot, id, name) and reports it. */
s32 PatInterface::recvAnsCircleHost(s32 index, const PatPacketHeader* header)
{
    u32 length;
    PatCircleUser user;

    PAT_TRACE("%s:recvAnsCircleHost ok\n");
    memset(&user, 0, sizeof(PatCircleUser));
    readInt32(this, &user.circleId_00);
    readCircleSlot(this, &user.slot_04);
    readString(this, &length, user.userId_06, 8);
    readString(this, &length, user.name_0E, 32);
    dispatchSessionHandlers(this, 0x805C, header->requestId_02, header->status_07, 1, (const u8*)&user);
    return 0;
}

/* Reads a new circle host (circle, slot, id, name) and reports it. */
s32 PatInterface::recvNtcCircleHost(s32 index, const PatPacketHeader* header)
{
    u32 length;
    PatCircleUser user;

    PAT_TRACE("%s:recvNtcCircleHost ok\n");
    memset(&user, 0, sizeof(PatCircleUser));
    readInt32(this, &user.circleId_00);
    readCircleSlot(this, &user.slot_04);
    readString(this, &length, user.userId_06, 8);
    readString(this, &length, user.name_0E, 32);
    dispatchSessionHandlers(this, 0x805D, NULL, header->status_07, 1, (const u8*)&user);
    return 0;
}

/* Reads the circle's members (at most four) with their match options and reports them. */
s32 PatInterface::recvAnsCircleUserList(s32 index, const PatPacketHeader* header)
{
    s32 count;
    PatCircleMatch members[4];

    PAT_TRACE("%s:recvAnsCircleUserList ok\n");
    readInt32(this, &count);
    if (count > 4) {
        count = 4;
    }
    memset(members, 0, sizeof(members));
    readCircleMatchData(this, members, count);
    dispatchSessionHandlers(this, 0x805E, header->requestId_02, header->status_07, count, (const u8*)members);
    return 0;
}

/* Reads a circle chat (the sender and the text) and reports it. */
s32 PatInterface::recvNtcChat(s32 index, const PatPacketHeader* header)
{
    u32 length;
    PatChatMessage chat;

    PAT_TRACE("%s:recvNtcChat\n");
    memset(&chat, 0, sizeof(PatChatMessage));
    readChatData(this, &chat.sender_100, 1);
    readString(this, &length, chat.text_000, 256);
    dispatchSessionHandlers(this, 0x8060, NULL, header->status_07, 1, (const u8*)&chat);
    return 0;
}

/* Reports the answer to a circle tell. */
s32 PatInterface::recvAnsCircleTell(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsCircleTell\n");
    dispatchSessionHandlers(this, 0x8061, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reads a circle tell (the sender's id and items, the text) and reports it. */
s32 PatInterface::recvNtcCircleTell(s32 index, const PatPacketHeader* header)
{
    u32 length;
    PatChatMessage chat;

    PAT_TRACE("%s:recvNtcCircleTell\n");
    memset(&chat, 0, sizeof(PatChatMessage));
    readString(this, &length, chat.sender_100.userId_08, 8);
    readChatData(this, &chat.sender_100, 1);
    readString(this, &length, chat.text_000, 256);
    dispatchSessionHandlers(this, 0x8062, NULL, header->status_07, 1, (const u8*)&chat);
    return 0;
}

/* Reports the answer to the circle notice setting and advances the request state. */
s32 PatInterface::recvAnsCircleInfoNoticeSet(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsCircleInfoNoticeSet ok\n");
    dispatchSessionHandlers(this, 0x8063, header->requestId_02, header->status_07, 0, NULL);
    requestState_6135 += 5;
    return 0;
}

/* Reads a circle created in the layer and reports it while a layer move is pending. */
s32 PatInterface::recvNtcCircleListLayerCreate(s32 index, const PatPacketHeader* header)
{
    PatCircleEntry circle;

    PAT_TRACE("%s:recvNtcCircleListLayerCreate ok\n");
    memset(&circle, 0, sizeof(PatCircleEntry));
    readInt32(this, &circle.circleId_000);
    readCircleInfoDataArray(this, &circle, 1);
    readUnkByteIntStruct(this, &circle.tags_37C, 1);
    if (layerMovePending_D62C != 0) {
        dispatchSessionHandlers(this, 0x8064, NULL, header->status_07, 1, (const u8*)&circle);
    }
    return 0;
}

/* Reads a changed circle of the layer and reports it while a layer move is pending. */
s32 PatInterface::recvNtcCircleListLayerChange(s32 index, const PatPacketHeader* header)
{
    PatCircleEntry circle;

    PAT_TRACE("%s:recvNtcCircleListLayerChange ok\n");
    memset(&circle, 0, sizeof(PatCircleEntry));
    readInt32(this, &circle.circleId_000);
    readCircleInfoDataArray(this, &circle, 1);
    readUnkByteIntStruct(this, &circle.tags_37C, 1);
    if (layerMovePending_D62C != 0) {
        dispatchSessionHandlers(this, 0x8065, NULL, header->status_07, 1, (const u8*)&circle);
    }
    return 0;
}

/* Reads a deleted circle's id and reports it while a layer move is pending. */
s32 PatInterface::recvNtcCircleListLayerDelete(s32 index, const PatPacketHeader* header)
{
    s32 circleId;

    PAT_TRACE("%s:recvNtcCircleListLayerDelete ok\n");
    readInt32(this, &circleId);
    if (layerMovePending_D62C != 0) {
        dispatchSessionHandlers(this, 0x8066, NULL, header->status_07, 1, (const u8*)&circleId);
    }
    return 0;
}

/* Reports the answer to an MCS creation. */
s32 PatInterface::recvAnsMcsCreate(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsMcsCreate\n");
    dispatchSessionHandlers(this, 0x8067, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reads the MCS creation notice's value and reports it. */
s32 PatInterface::recvNtcMcsCreate(s32 index, const PatPacketHeader* header)
{
    s8 value;

    PAT_TRACE("%s:recvNtcMcsCreate\n");
    readUInt8_(this, &value);
    dispatchSessionHandlers(this, 0x8068, NULL, header->status_07, 1, (const u8*)&value);
    return 0;
}

/* Reads the MCS server a match starts on (address, port, name) and reports it. */
s32 PatInterface::recvNtcMcsStart(s32 index, const PatPacketHeader* header)
{
    u32 length;
    PatMcsServer server;

    PAT_TRACE("%s:recvNtcMcsStart\n");
    memset(&server, 0, sizeof(PatMcsServer));
    readString(this, &length, server.host_000, 256);
    readUInt16(this, &server.port_104);
    readString(this, &length, server.name_106, 32);
    dispatchSessionHandlers(this, 0x8069, NULL, header->status_07, 1, (const u8*)&server);
    return 0;
}

/* Reports the answer to a tell. */
s32 PatInterface::recvAnsTell(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsTell\n");
    dispatchSessionHandlers(this, 0x806A, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reads a tell (the sender's id and items, the text) and reports it. */
s32 PatInterface::recvNtcTell(s32 index, const PatPacketHeader* header)
{
    u32 length;
    PatChatMessage chat;

    PAT_TRACE("%s:recvNtcTell\n");
    memset(&chat, 0, sizeof(PatChatMessage));
    readString(this, &length, chat.sender_100.userId_08, 8);
    readChatData(this, &chat.sender_100, 1);
    readString(this, &length, chat.text_000, 256);
    dispatchSessionHandlers(this, 0x806B, NULL, header->status_07, 1, (const u8*)&chat);
    return 0;
}

/* Reports the answer to a user binary. */
s32 PatInterface::recvAnsBinaryUser(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsBinaryUser ok\n");
    dispatchSessionHandlers(this, 0x806C, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reads a user's binary notice (the sender and the bytes) and reports it. */
s32 PatInterface::recvNtcBinaryUser(s32 index, const PatPacketHeader* header)
{
    u32 length;
    PatBinaryMessage notice;

    PAT_TRACE("%s:recvNtcBinaryUser ok\n");
    memset(&notice.sender_104, 0, sizeof(PatNtcCompound));
    readString(this, &length, notice.sender_104.userId_04, 8);
    readNtcCompoundData(this, &notice.sender_104, 1);
    readUInt8Array(this, &length, notice.data_000, 256);
    notice.size_100 = (u16)length;
    dispatchSessionHandlers(this, 0x806D, NULL, header->status_07, 1, (const u8*)&notice);
    return 0;
}

/* Reads the server's binary notice; nothing is reported. */
s32 PatInterface::recvNtcBinaryServer(s32 index, const PatPacketHeader* header)
{
    u32 length;
    PatBinaryMessage notice;

    PAT_TRACE("%s:recvNtcBinaryServer ok\n");
    memset(&notice.sender_104, 0, sizeof(PatNtcCompound));
    readNtcCompoundData(this, &notice.sender_104, 1);
    readUInt8Array(this, &length, notice.data_000, 256);
    notice.size_100 = (u16)length;
    return 0;
}

/* Reports the answer to a user search setting. */
s32 PatInterface::recvAnsUserSearchSet(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsUserSearchSet\n");
    dispatchSessionHandlers(this, 0x806E, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reports the answer to a user binary setting. */
s32 PatInterface::recvAnsUserBinarySet(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsUserBinarySet\n");
    dispatchSessionHandlers(this, 0x806F, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reports the answer to a user binary notice. */
s32 PatInterface::recvAnsUserBinaryNotice(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsUserBinaryNotice\n");
    dispatchSessionHandlers(this, 0x8070, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reads a user's binary notice (kind, id, value, bytes) and reports it. */
s32 PatInterface::recvNtcUserBinaryNotice(s32 index, const PatPacketHeader* header)
{
    u32 length;
    PatUserBinaryNotice notice;

    PAT_TRACE("%s:recvNtcUserBinaryNotice\n");
    readUInt8(this, &notice.kind_008);
    readString(this, &length, notice.userId_000, 8);
    readUInt32_(this, &notice.value_110);
    readUInt8Array(this, &length, notice.data_009, 256);
    notice.size_10C = length;
    dispatchSessionHandlers(this, 0x8071, NULL, header->status_07, 1, (const u8*)&notice);
    return 0;
}

/* Reads the user search's head and reports its first value unless the search is silent. */
s32 PatInterface::recvAnsUserSearchHead(s32 index, const PatPacketHeader* header)
{
    s32 values[2];

    PAT_TRACE("%s:recvAnsUserSearchHead\n");
    readInt32(this, &values[0]);
    readInt32(this, &values[1]);
    if (searchMode_6118 == 0) {
        dispatchSessionHandlers(this, 0x8030, header->requestId_02, header->status_07, 1, (const u8*)values);
    }
    return 0;
}

/* Reads the user search's rows onto the call stack and reports them unless the search is silent. */
s32 PatInterface::recvAnsUserSearchData(s32 index, const PatPacketHeader* header)
{
    u32 stackSize;
    u32 maxCount;
    s32 values[2];
    PatUserSearch* info;
    PatUserSearch* list;
    s32 i;

    PAT_TRACE("%s:recvAnsUserSearchData\n");
    readInt32(this, &values[0]);
    readInt32(this, &values[1]);
    list = (PatUserSearch*)createStack(this, values[1] * sizeof(PatUserSearch), &stackSize);
    maxCount = stackSize / sizeof(PatUserSearch);
    if (values[1] > (s32)maxCount) {
        values[1] = maxCount;
    }
    for (i = 0, info = list; i < values[1]; info++, i++) {
        readUserSearchData(this, info, 1);
        readUnkByteIntStruct(this, &info->tags_230, 1);
    }
    if (searchMode_6118 == 0) {
        dispatchSessionHandlers(this, 0x8031, header->requestId_02, header->status_07, values[1], (const u8*)list);
    }
    growStackSize(this, stackSize);
    return 0;
}

/* Reports the end of the user search unless the search is silent. */
s32 PatInterface::recvAnsUserSearchFoot(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsUserSearchFoot\n");
    if (searchMode_6118 == 0) {
        dispatchSessionHandlers(this, 0x8032, header->requestId_02, header->status_07, 0, NULL);
    }
    return 0;
}

/* Reads one user search row and reports it as the search kind's event. */
s32 PatInterface::recvAnsUserSearchInfo(s32 index, const PatPacketHeader* header)
{
    PatUserSearch row;

    PAT_TRACE("%s:recvAnsUserSearchInfo\n");
    memset(&row, 0, sizeof(PatUserSearch));
    readUserSearchData(this, &row, 1);
    readUnkByteIntStruct(this, &row.tags_230, 1);
    switch (searchKind_6119) {
    case 1:
        dispatchSessionHandlers(this, 0x8072, header->requestId_02, header->status_07, 1, (const u8*)&row);
        break;
    case 2:
        dispatchSessionHandlers(this, 0x8073, header->requestId_02, header->status_07, 1, (const u8*)&row);
        break;
    }
    return 0;
}

/* Reads this client's own search row and keeps its two values. */
s32 PatInterface::recvAnsUserSearchInfoMine(s32 index, const PatPacketHeader* header)
{
    PatUserSearch row;

    PAT_TRACE("%s:recvAnsUserSearchInfoMine\n");
    memset(&row, 0, sizeof(PatUserSearch));
    readUserSearchData(this, &row, 1);
    mySearchValue_65E8 = row.value_228;
    mySearchValue_65EC = row.value_22C;
    requestState_6135 += 5;
    return 0;
}

/* Reports the answer to a user status setting. */
s32 PatInterface::recvAnsUserStatusSet(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsUserStatusSet\n");
    dispatchSessionHandlers(this, 0x8074, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reads a user status record and reports it. */
s32 PatInterface::recvAnsUserStatus(s32 index, const PatPacketHeader* header)
{
    u8 status[7];

    PAT_TRACE("%s:recvAnsUserStatus\n");
    memset(status, -1, sizeof(status));
    readUserStatusData(this, status, 1);
    dispatchSessionHandlers(this, 0x8075, header->requestId_02, header->status_07, 1, status);
    return 0;
}

/* Reports the answer to a friend request. */
s32 PatInterface::recvAnsFriendAdd(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsFriendAdd\n");
    dispatchSessionHandlers(this, 0x8076, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reads a friend request notice (the friend and its state) and reports it. */
s32 PatInterface::recvNtcFriendAdd(s32 index, const PatPacketHeader* header)
{
    u32 length;
    PatFriend entry;

    PAT_TRACE("%s:recvNtcFriendAdd\n");
    memset(&entry, 0, sizeof(PatFriend));
    readString(this, &length, entry.userId_04, 8);
    readFriendData(this, &entry, 1);
    readUInt8_(this, &entry.state_2C);
    dispatchSessionHandlers(this, 0x8077, NULL, header->status_07, 1, (const u8*)&entry);
    return 0;
}

/* Reports the answer to a friend acceptance. */
s32 PatInterface::recvAnsFriendAccept(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsFriendAccept\n");
    dispatchSessionHandlers(this, 0x8078, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reads a friend acceptance notice (the sender's id and items, the text) and reports it. */
s32 PatInterface::recvNtcFriendAccept(s32 index, const PatPacketHeader* header)
{
    u32 length;
    PatChatMessage chat;

    PAT_TRACE("%s:recvNtcFriendAccept\n");
    memset(&chat, 0, sizeof(PatChatMessage));
    readString(this, &length, chat.sender_100.userId_08, 8);
    readChatData(this, &chat.sender_100, 1);
    readString(this, &length, chat.text_000, 256);
    dispatchSessionHandlers(this, 0x8079, NULL, header->status_07, 1, (const u8*)&chat);
    return 0;
}

/* Reports the answer to a friend deletion. */
s32 PatInterface::recvAnsFriendDelete(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsFriendDelete\n");
    dispatchSessionHandlers(this, 0x807A, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reads the friend list onto the call stack and reports it. */
s32 PatInterface::recvAnsFriendList(s32 index, const PatPacketHeader* header)
{
    u32 stackSize;
    u32 maxCount;
    s32 values[2];
    PatFriend* list;

    PAT_TRACE("%s:recvAnsFriendList\n");
    readInt32(this, &values[0]);
    readInt32(this, &values[1]);
    list = (PatFriend*)createStack(this, values[1] * sizeof(PatFriend), &stackSize);
    maxCount = stackSize / sizeof(PatFriend);
    if (values[1] > (s32)maxCount) {
        values[1] = maxCount;
    }
    readFriendData(this, list, values[1]);
    dispatchSessionHandlers(this, 0x807B, header->requestId_02, header->status_07, values[1], (const u8*)list);
    growStackSize(this, stackSize);
    return 0;
}

/* Reports the answer to a black list addition. */
s32 PatInterface::recvAnsBlackAdd(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsBlackAdd\n");
    dispatchSessionHandlers(this, 0x807C, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reports the answer to a black list deletion. */
s32 PatInterface::recvAnsBlackDelete(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsBlackDelete\n");
    dispatchSessionHandlers(this, 0x807D, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reads the black list onto the call stack and reports it. */
s32 PatInterface::recvAnsBlackList(s32 index, const PatPacketHeader* header)
{
    u32 stackSize;
    u32 maxCount;
    s32 values[2];
    PatBlackListEntry* list;

    PAT_TRACE("%s:recvAnsBlackList\n");
    readInt32(this, &values[0]);
    readInt32(this, &values[1]);
    list = (PatBlackListEntry*)createStack(this, values[1] * sizeof(PatBlackListEntry), &stackSize);
    maxCount = stackSize / sizeof(PatBlackListEntry);
    if (values[1] > (s32)maxCount) {
        values[1] = maxCount;
    }
    readBlackListData(this, list, values[1]);
    dispatchSessionHandlers(this, 0x807E, header->requestId_02, header->status_07, values[1], (const u8*)list);
    growStackSize(this, stackSize);
    return 0;
}

/* Reads the agreement's page count and reports it. */
s32 PatInterface::recvAnsAgreementPageNum(s32 index, const PatPacketHeader* header)
{
    u8 version;
    u8 pageCount;

    PAT_TRACE("%s:recvAnsAgreementPageNum\n");
    readUInt8(this, &version);
    readUInt8(this, &pageCount);
    dispatchSessionHandlers(this, 0x807F, header->requestId_02, header->status_07, 1, &pageCount);
    return 0;
}

/* Reads the agreement's page info (its page records onto the call stack) and reports it. */
s32 PatInterface::recvAnsAgreementPageInfo(s32 index, const PatPacketHeader* header)
{
    PatAgreementInfo info;
    PatAgreementInfo* infoPtr;
    u32 stackSize;
    u8 kind;

    PAT_TRACE("%s:recvAnsAgreementPageInfo\n");
    readUInt8(this, &kind);
    infoPtr = &info;
    readUInt8(this, &infoPtr->version_00);
    readUInt8(this, &infoPtr->pageCount_0C);
    infoPtr->pages_10 = createStack(this, infoPtr->pageCount_0C * 40, &stackSize);
    if (infoPtr->pageCount_0C * 40 > stackSize) {
        PAT_KEEP_FAILURE();
        growStackSize(this, stackSize);
        return -1;
    }
    readAgreementPageData(this, infoPtr->pages_10, infoPtr->pageCount_0C);
    readAgreementInfoData(this, infoPtr, 1);
    dispatchSessionHandlers(this, 0x8080, header->requestId_02, header->status_07, 1, (const u8*)infoPtr);
    growStackSize(this, stackSize);
    return 0;
}

/* Reads one agreement page (its text onto the call stack) and reports it. */
s32 PatInterface::recvAnsAgreementPage(s32 index, const PatPacketHeader* header)
{
    PatAgreementPage page;
    u32 stackSize;
    u32 length;
    u8 pageIndex;
    u8 kind;

    PAT_TRACE("%s:recvAnsAgreementPage\n");
    readUInt8(this, &kind);
    readUInt8(this, &pageIndex);
    page.page_0 = pageIndex;
    readUInt32_(this, &page.value_4);
    readUInt32_(this, &page.size_8);
    page.data_C = createStack(this, page.size_8, &stackSize);
    if (page.size_8 > stackSize) {
        page.size_8 = stackSize;
    }
    readUInt8Array(this, &length, page.data_C, page.size_8);
    dispatchSessionHandlers(this, 0x8081, header->requestId_02, header->status_07, 1, (const u8*)&page);
    growStackSize(this, stackSize);
    return 0;
}

/* Reads the agreement answer and reports it. */
s32 PatInterface::recvAnsAgreement(s32 index, const PatPacketHeader* header)
{
    u8 value;

    PAT_TRACE("%s:recvAnsAgreement\n");
    readUInt8(this, &value);
    dispatchSessionHandlers(this, 0x8082, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}
