/*
 * Network/network_pat_control.cpp - the 0x80423E74..0x80432104 network pat-control unit: the work-record /
 * PatCamellia band (0x80423E74..0x80429B94, 77 functions, absorbed from `fn_80423E74.cpp` at phase 4, second header
 * below, same `cflags_main`) followed by the 0x80429B94-0x8043065C pat-control band
 * (114 functions, 27336 B): the server/city/room select, friend and community lists and the terms and
 * file-fetch steps the per-frame `updateNetworkPatControl` state machine drives through the `net_ctrl_wk`
 * work record.
 *
 * Name: no `__FILE__` string is in the range; the stem follows the Pat vocabulary the band calls
 * (`getPatsObject`, `getNetworkSessionManagerPat`, `getNetworkLayerPat`, ...) and its `Network/` siblings.
 * Lib/flags: game-root `main` lib (`cflags_main`, Wii/1.3 -O3, exceptions on: the object has extab).
 *
 * Sections: .text 0x80429B94..0x8043065C, extab 0x8001D368..0x8001D558, extabindex 0x8003DE54..0x8003E0DC,
 * .rodata 0x80571EA0..0x80572240 (`pat_ca_cert`, the only referrer is `updateNetworkPatControl`), .sbss
 * 0x80794CF8, and the `.data` runs 0x806038E8..0x80603CB8 (6 ranges in splits.txt, true claim 0x3D0 B =
 * the object's emit; `flipcheck` misreads a multi-range claim as "0x4C (+900)").  0x80603CB8..0x80603D10
 * (`lbl_80603CB8`, the jump table 0x80603CE4) is released: only `fn_8043065C` reads it.
 *
 * Class model: `NetCtrlWk` (0xC4A0 B; only it is under `#pragma pack(1)`, for the `u32` run at the odd
 * offset +0x7996) is the `net_ctrl_wk` singleton with its static and instance members; the Pat layers are
 * the virtual classes in `Network/NetworkLayerPat.h` and `Network/NetworkCommunityPat.h` (slots declared,
 * never defined, so no vtable is emitted).  C-linkage free functions stay only for names other units call.
 *
 * Load-bearing shapes (measured): `#pragma peephole off` is per-function evidenced - with it removed 55
 * functions score lower (collectEvents 95.6 -> 63.3, copyPeerList 100 -> 92.7, setTextSize 100 -> 66.7, the
 * unit 93.61 -> 90.25 %) and none scores higher, so it stays for the whole file; server-index scans are
 * loops (MWCC unrolls them to the target shape); `(dst++)->assign(src++)` gives the target's
 * pointer-increment order in copyPeerList; `ai_npc_reaction_forward()` is called with no argument (r3 is
 * the caller's leftover in retail).
 *
 * Names: `updateNetworkPatControl`, `isCityMode` (mode byte 3: set when the control enters state 0x19
 * after the roster refresh; GUESS) and the field/member names of NetCtrlWk and the record types are
 * derived from use and offsets.  About 150 callee and record names are GUESSES from the caller's use
 * (never from a retail symbol): the blocks marked GUESS in `include/unsplit/Network.h`,
 * `Network/NetworkWiiMediator.h`, `Network/NetworkLayerPat.h`, `Network/NetworkSessionManager.h` and this
 * unit's own header.  Left as they are: `isReadyCountOne` (the body is `ready_count_0x044 == 1`, what
 * the count is stays unknown) and `resetFailureState` (a `blr` stub).
 *
 * Residuals (re-measure: `python tools/units/recompile.py Network/network_pat_control --measure <symbol>`):
 *  - 27 of 114 functions are open (87 at 100 %).  Worst: `get_server_type_name` 44.17 % (48 B), then
 *    copyRosterLists 82.6, getUtf8CharLength 84.1, submitSlotRequest 86.4, getPrintedWidth 88.2,
 *    `updateNetworkPatControl` 88.30 %, and the other 21 above 88 %: register allocation and unroll shape.
 *    `updateNetworkPatControl` also lost 0.04 when the shared band declared `lobby_world_block` as
 *    `u8[]` (the retail read is a `.sbss` pointer via r13; the array is addressed with lis/@l).
 *  - .text is 0x6A6C against the target's 0x6AC8 (-92 B); extab 0x1E0 against 0x1F0.
 *  - Data: the `.rodata` certificate is byte-identical after linking; the split object holds a relocation
 *    at +0x244 (dtk read the DER bytes 8014CB40 as a pointer), so the object compare shows 4 B. The
 *    `.sdata` 0x807939A0.. run (interleaved with fn_80423E74's 0x807939A8) and the `.sdata2` words
 *    0x8079C890/0x8079C898 (also read by the first band of this unit and the unsplit 0x80431690) are shared, so they
 *    stay unclaimed and their externs remain; our object also emits 0x10 of each of `.sdata`/`.sdata2` from its own pools (`@NNNN` where the target names lbl_8079C898/lbl_8079C890/lbl_807939C4/SPACE_STR).
 *  - Two rule-2 findings the lint still counts: `lb_entry_flags_clear` and `lb_quest_board_reset` have
 *    registered owners, but their owner headers cannot be included beside `unsplit/lobby.h`
 *    (`lobby_w`/`_mh_ivec2_` redefinitions), so the prototypes sit in the leaf headers
 *    `lobby/lb_entry_flags_clear.h` and `lobby/lb_quest_board_reset.h`, which the lint's `_owns` does not
 *    recognise as the owners'.
 */

/* Absorbed unit `fn_80423E74.cpp` (phase 4 fold):
 * fn_80423E74.cpp - the 0x80423E74-0x80429B94 band (23840 B, 77 functions): the PatCamellia wrapper
 * over the retail Camellia cipher plus the network work record's arena vectors, slot table and message
 * pool that the `net_ctrl_wk` singleton carries.
 *
 * Home and lib (brief section 2).  No `__FILE__` string covers the range: its own `.data` pool is the
 * two dispatch jump tables at 0x80603750 / 0x806037F4 and the twenty-odd bytes `so alloc fail` /
 * `so free fail` / `dwc alloc fail` / `dwc free fail` / `mh3uswii` at 0x80603888 (checked with
 * `python tools/symbols/dumpmap.py lookup <addr>`: the symbols there answer `_80425abcs_so_alloc_fail_80603888`
 * and `lbl_806C...` string rows, never a bare source-file name).  The runtime dump answers only
 * `zz_0423e74_`-style placeholders for every address in the range except `PatCryptDecrypt` and
 * `setErrorHappened`.  So the module is the same un-moduled game-root band as its link neighbour
 * `Network/network_pat_control.cpp` (the network pat-control band that starts where this one ends and also
 * dereferences `net_ctrl_wk` and calls `getPatsObject`/`getNetworkLayerPat`), and the file takes the
 * game-root `main` lib and `cflags_main` (Wii/1.3, -O3, -inline noauto, -Cpp_exceptions on - the target
 * object carries extab/extabindex).  The stem is the map's `fn_80423E74` (classes 3/4 in the brief).
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * tools/symbols/dumpmap.py: every address in the range answers `zz_<addr>_` or a global name, never a
 * `__FILE__` emitter; `PatCryptDecrypt` and `setErrorHappened` are the only real names in the map).
 *
 * Sections this unit owns: .text 0x80423E74-0x80429B94, extab 0x8001D1B4-0x8001D368,
 * extabindex 0x8003DC44-0x8003DE54, and the two `.data` runs its functions reference
 * (0x80603750-0x80603858, the two jump tables; 0x80603888-0x806038D8, the alloc-failure strings).
 * The array at 0x80603858 and the string at 0x806038D8 belong to the neighbour
 * `Network/network_pat_control.cpp`.
 *
 * Residuals: the two state machines `fn_804247D0` (0xE78, three jump tables) and `updateMessagePool`
 * (0x1580, one jump table) and the message-pool families that follow them are not reconstructed yet;
 * every function that is reconstructed below is listed in the outbox.  Re-measure with
 * `python tools/units/recompile.py fn_80423E74 --measure <symbol>`.
 */

#include "Network/network_pat_control.h"
#include "Network/NetworkSessionManager.h"   /* NetworkSessionManagerPat - the class the band dispatches through */
#include "Runtime.PPCEABI.H/memset.h"            /* memset, owner Runtime.PPCEABI.H/memset.c (rule 2) */
#include "unsplit/Runtime.PPCEABI.H.h"           /* strcpy - unowned MSL helper (rule 2's unsplit gap) */
#include "g3d/g3d_anmchr.h"                      /* flfntStrLen, owner g3d/g3d_anmchr.cpp (rule 2) */
#include "enemy/em020_ai.h"
#include "enemy/em_pop.h"                          /* matchesFileVersion, owner enemy/em_pop.cpp (rule 2) */
#include "fn_80047398.h"                          /* decodePlayerCard, owner fn_80047398.cpp (rule 2) */
#include "Network/NetworkCommunityPat.h"
#include "Network/NetworkWiiMediator.h"
#include "sound/fn_800E46E8.h"
#include "sound/fn_800D7F54.h"
#include "menu/menu_message.h"
#include "menu/get_pop_dat_ptr.h"
#include "ai/fn_802D44F4.h"
#include "unsplit/Network.h"
#include "Network/NetworkSessionManagerPat.h"
#include "unsplit/lobby.h"
#include "lobby/lb_entry_flags_clear.h"          /* lb_entry_flags_clear, owner lobby/lb_companion_ui.cpp (rule 2) */
#include "lobby/lb_quest_board_reset.h"          /* lb_quest_board_reset, owner lobby/lb_quest_board.cpp (rule 2) */
#include "unsplit/unknown.h"
#include "fn_8004CAD8.h"
#include "quest/quest_entry.h"                     /* get_userdata (declared there for its consumers) */
#include "ef/fn_800CDB2C.h"
#include "Network/NetworkPeerBase.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The paged 16-byte PatCamellia context handed out by PatCryptEncrypt/PatCryptDecrypt.  The header is a
 * big-endian u16 length in the first two bytes; the payload follows it, one 16-byte Camellia block at a
 * time, each block chained into the next by the 4-byte rotation the two functions share. */
typedef struct PatCryptBuf {
    /* +0x00 */ u8 bytes_0x00[16];
} PatCryptBuf; /* size: 0x10 */

/*
 * Wrapper over Camellia_DecryptBlock's key schedule: build the 256-bit PatCamellia key schedule from
 * `rawKey`.  A tail call, so the retail body is a bare `b Camellia_Ekeygen`.
 */
void fn_80423E74(const u8* rawKey)
{
    Camellia_Ekeygen(0x100, rawKey, lbl_806D3670);
}

/*
 * PatCryptEncrypt: encrypt `*len` bytes of `buf` in place, writing the big-endian length header and
 * returning it in `*len` (already framed to whole blocks).  The block is a 4-byte rotation of the
 * 16-byte working buffer, followed by one Camellia block per full buffer.
 */
s32 fn_80423E88(u8* buf, u16* len)
{
    u8 hdr[4];
    u8 in[16];
    u8 blk[16];
    u16 n;
    int i;
    int j;
    int m;

    if (buf == 0 || len == 0 || *len == 0) {
        return 0;
    }
    memset(hdr, 0, 4);
    n = *len;
    hdr[0] = (u8)(n >> 8);
    hdr[1] = (u8)n;
    memcpy(blk, hdr, 4);
    j = 4;
    m = (*len < 16) ? *len : 16;
    memcpy(in, buf, m);
    i = 0;
    for (;;) {
        blk[j % 16] = in[i % 16];
        j++;
        i++;
        if ((j % 16) == 0) {
            Camellia_EncryptBlock(0x100, blk, lbl_806D3670, blk);
            memcpy(buf + j - 16, blk, 16);
            if ((s32)*len <= i) {
                break;
            }
        }
        if ((i % 16) == 0) {
            m = *len - i;
            if (m > 16) {
                m = 16;
            }
            if (m > 0) {
                memcpy(in, buf + i, m);
            }
        }
    }
    *len = (u16)j;
    return 0;
}

/*
 * PatCryptDecrypt: the inverse of fn_80423E88.  Rejects a length that is not a whole number of 16-byte
 * blocks with 0x8000, otherwise decrypts the framed buffer in place and rewrites `*len` with the
 * payload length the header carries.
 */
s32 PatCryptDecrypt(u8* buf, u16* len)
{
    u8 hdr[4];
    u8 blk[16];
    u16 n;
    int i;
    int j;

    if (buf == 0 || len == 0 || *len == 0) {
        return 0;
    }
    if (((s32)*len % 16) != 0) {
        return 0x8000;
    }
    memcpy(blk, buf, 16);
    Camellia_DecryptBlock(0x100, blk, lbl_806D3670, blk);
    memcpy(hdr, blk, 4);
    n = (u16)((hdr[0] << 8) | hdr[1]);
    j = 4;
    i = 0;
    do {
        if (j < n) {
            buf[j] = blk[i % 16];
        }
        i++;
        j++;
        if ((i % 16) == 0) {
            memcpy(blk, buf + i, 16);
            Camellia_DecryptBlock(0x100, blk, lbl_806D3670, blk);
        }
    } while (i < (s32)*len && j < n);
    *len = (u16)j;
    return 0;
}

/*
 * Point the record's three parallel 64-entry arrays at the shared 0x400-block arena, allocating the
 * arena on first use: arrA[k] gets the k-th 0x400 block, arrB/arrC are cleared.
 */
void setupArenaVectors(NetCtrlWk* work)
{
    u8* base;
    u32 k;

    if (lbl_80794CEC == 0) {
        lbl_80794CEC = fn_800404BC(0x10000);
    }
    base = lbl_80794CEC;
    for (k = 0; k < 64; k++) {
        work->arrA_0x7A98[k] = (u32*)(base + k * 0x400);
        work->arrB_0x7B98[k] = 0;
        work->arrC_0x7C98[k] = 0;
    }
}

/*
 * The same arena re-point, without the allocation: used to rebuild the arrays for a record whose arena
 * already exists (the record reset path).
 */
void fn_80424308(NetCtrlWk* work)
{
    u8* base = lbl_80794CEC;
    u32 k;

    if (base != 0) {
        for (k = 0; k < 64; k++) {
            work->arrA_0x7A98[k] = (u32*)(base + k * 0x400);
            work->arrB_0x7B98[k] = 0;
            work->arrC_0x7C98[k] = 0;
        }
    }
}

/*
 * Hand out the first free arena block: mark its arrC slot used and return the block pointer (arrA),
 * or NULL when all 64 are taken.
 */
u32* fn_80424444(void)
{
    NetCtrlWk* work = net_ctrl_wk;
    u32 i;

    for (i = 0; i < 0x40; i++) {
        if (work->arrC_0x7C98[i] == 0) {
            work->arrC_0x7C98[i] = 1;
            return work->arrA_0x7A98[i];
        }
    }
    return 0;
}

/*
 * Release the arena block `block`: clear its arrB value and mark its arrC slot free.
 */
void fn_8042448C(u32 block)
{
    NetCtrlWk* work = net_ctrl_wk;
    u32 i;

    for (i = 0; i < 0x40; i++) {
        if ((u32)work->arrA_0x7A98[i] == block) {
            work->arrB_0x7B98[i] = 0;
            work->arrC_0x7C98[i] = 0;
            return;
        }
    }
}

/*
 * Store `value` in the arrB side of the entry that owns `block`.
 */
void fn_804244D8(u32 block, u32 value)
{
    NetCtrlWk* work = net_ctrl_wk;
    u32 i;

    for (i = 0; i < 0x40; i++) {
        if ((u32)work->arrA_0x7A98[i] == block) {
            work->arrB_0x7B98[i] = value;
            return;
        }
    }
}

/*
 * Clear the arrB/arrC sides of all 64 entries of `work`.
 */
void resetNetSlots(NetCtrlWk* work)
{
    u32 i;

    for (i = 0; i < 0x40; i++) {
        work->arrB_0x7B98[i] = 0;
        work->arrC_0x7C98[i] = 0;
    }
}

/*
 * Reset the 100-record slot table: every slot's state byte goes to 0, then slot 0 is made live and the
 * record's slot list is pointed at it, with the two float members and the mode word seeded.
 */
void fn_804245C8(void)
{
    NetCtrlWk* work = net_ctrl_wk;
    u32 i;

    for (i = 0; i < 100; i++) {
        work->slots_0x3ED4[i].state_0x00 = 0;
    }
    work->slots_0x3ED4[0].state_0x00 = 1;
    work->slot_list_0x3ED0 = &work->slots_0x3ED4[0];
    work->slot_list_0x3ED0->valueA_0x3C = lbl_8079C888;
    work->slot_list_0x3ED0->valueB_0x40 = lbl_8079C888;
    work->slot_list_0x3ED0->mode_0x54 = 1;
    fn_80424308(work);
}

/*
 * Find the live slot whose owner pointer is `owner`, or NULL when none is.
 */
NetSlot* fn_804246F8(void* owner)
{
    NetCtrlWk* work = net_ctrl_wk;
    NetSlot* slot = work->slots_0x3ED4;
    u32 i;

    for (i = 0; i < 100; i++) {
        if (slot->state_0x00 != 0 && slot->owner_0x04 == owner) {
            return slot;
        }
        slot++;
    }
    return 0;
}

/*
 * Hand out the first free record of the 128-entry pool at `pool_0x6320`: mark it in use, bump the pool's
 * live count and return a pointer to the record's payload (record + 8).
 */
NetPoolEntry* fn_804256F4(void)
{
    NetCtrlWk* work = net_ctrl_wk;
    NetPoolEntry* pool = work->pool_0x6320;
    u32 i;

    for (i = 0; i < 0x80; i++) {
        if (pool[i].in_use_0x08 == 0) {
            pool[i].in_use_0x08 = 1;
            pool->count_0x04 = pool->count_0x04 + 1;
            return &pool[i];
        }
    }
    return 0;
}

/*
 * Release one record back to the pool and drop the pool's live count.
 */
void fn_80425750(NetPoolEntry* entry)
{
    NetCtrlWk* work = net_ctrl_wk;

    memset(entry, 0, 0x20);
    work->pool_0x6320[0].count_0x04 = work->pool_0x6320[0].count_0x04 - 1;
}

/*
 * Reset the work record's stall counter.
 */
void fn_80426D10(void)
{
    net_ctrl_wk->counter_0xC364 = 0;
}

/*
 * Clear the four-entry message table back to the record's own name, free its 64 flags and mark entry 0
 * live.  Guarded on the network layer being up.
 */
void fn_80427024(void)
{
    NetCtrlWk* work = net_ctrl_wk;
    u32 i;

    if (getNetworkLayerPat(getPatsObject(), 0) != 0) {
        memset(work->msgTable_0x7488, 0, 0x480);
        work->used_0x7988[0] = 0;
        work->used_0x7988[1] = 0;
        work->used_0x7988[2] = 0;
        work->used_0x7988[3] = 0;
        strcpy((char*)work->msgTable_0x7488, work->name_0x7368);
        strcpy((char*)&work->msgTable_0x7488[0xA], work->name2_0x7372);
        work->used_0x7988[0] = 1;
        for (i = 0; i < 0x40; i++) {
            memset(&work->slots_0x7996[i], 0, 4);
        }
    }
}

/*
 * Hand out the first free 4-byte record of the 64-record flag array at +0x7996, or NULL when none is
 * free.
 */
u32* fn_80427240(void)
{
    NetCtrlWk* work = net_ctrl_wk;
    u32 i;

    for (i = 0; i < 0x40; i++) {
        if (work->slots_0x7996[i] == 0) {
            return &work->slots_0x7996[i];
        }
    }
    return 0;
}

/*
 * Empty body: the retail symbol is a bare `blr`.
 */
void fn_80427630(void)
{
}

/*
 * The record's own display name, or NULL when the network layer is not up.
 */
char* fn_80427634(void)
{
    NetCtrlWk* work = net_ctrl_wk;

    if (getNetworkLayerPat(getPatsObject(), 0) == 0) {
        return 0;
    }
    return work->name_0x7368;
}

/*
 * The `index`-th 0x120-byte slot of the message table, or NULL when the network layer is not up.
 */
char* fn_8042767C(s32 index)
{
    NetCtrlWk* work = net_ctrl_wk;

    if (getNetworkLayerPat(getPatsObject(), 0) == 0) {
        return 0;
    }
    return (char*)(work->msgTable_0x7488 + index * 0x120);
}

/*
 * Forward the record's own state to isCityMode, or 0 when the network layer is not up.
 */
s32 fn_804276D8(void)
{
    if (getNetworkLayerPat(getPatsObject(), 0) == 0) {
        return 0;
    }
    return isCityMode();
}

/*
 * The `index`-th 0x2510-byte sub-record of the layer's 0xF1B0 table, as an address.
 */
s32 fn_804281C4(s32 index)
{
    return (s32)((u8*)getNetworkLayerPat(getPatsObject(), 0) + 0xF1B0 + index * 0x2510);
}

/*
 * The message-table fill state.
 */
s32 fn_80428208(void)
{
    return net_ctrl_wk->msg_state_0xC300;
}

/*
 * Clear the message-table fill state.
 */
void fn_80428218(void)
{
    net_ctrl_wk->msg_state_0xC300 = 0;
}

/*
 * Ask the resource system to release store `id` (a bare tail call).
 */
s32 fn_80428B08(void)
{
    return fn_804C2380(4);
}

#ifdef __cplusplus
}
#endif


#pragma peephole off

/* The work-record singleton itself (`.sbss:0x80794CF8`, 4 B).  Its owner TU is unregistered and every
 * function in this band dereferences it, so this unit claims the range (rule 12) and defines it here;
 * the 0x80423E74 band above reaches the same row through the header's `extern`. */
NetCtrlWk* net_ctrl_wk;

/* The Nintendo Class 2 CA certificate (DER, 0x39C bytes) the pat interface pins for its secure
 * connections, and the length handed to it (the map's 0x80571EA0 `.rodata` blob and 0x807939A0
 * `.sdata` word). */
const u8 pat_ca_cert[0x3A0] = {
    0x30, 0x82, 0x03, 0x98, 0x30, 0x82, 0x03, 0x01, 0xA0, 0x03, 0x02, 0x01, 0x02, 0x02, 0x01, 0x01,
    0x30, 0x0D, 0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x05, 0x05, 0x00, 0x30,
    0x81, 0x95, 0x31, 0x0B, 0x30, 0x09, 0x06, 0x03, 0x55, 0x04, 0x06, 0x13, 0x02, 0x55, 0x53, 0x31,
    0x13, 0x30, 0x11, 0x06, 0x03, 0x55, 0x04, 0x08, 0x13, 0x0A, 0x57, 0x61, 0x73, 0x68, 0x69, 0x6E,
    0x67, 0x74, 0x6F, 0x6E, 0x31, 0x21, 0x30, 0x1F, 0x06, 0x03, 0x55, 0x04, 0x0A, 0x13, 0x18, 0x4E,
    0x69, 0x6E, 0x74, 0x65, 0x6E, 0x64, 0x6F, 0x20, 0x6F, 0x66, 0x20, 0x41, 0x6D, 0x65, 0x72, 0x69,
    0x63, 0x61, 0x20, 0x49, 0x6E, 0x63, 0x2E, 0x31, 0x0C, 0x30, 0x0A, 0x06, 0x03, 0x55, 0x04, 0x0B,
    0x13, 0x03, 0x4E, 0x4F, 0x41, 0x31, 0x1C, 0x30, 0x1A, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13, 0x13,
    0x4E, 0x69, 0x6E, 0x74, 0x65, 0x6E, 0x64, 0x6F, 0x20, 0x43, 0x6C, 0x61, 0x73, 0x73, 0x20, 0x32,
    0x20, 0x43, 0x41, 0x31, 0x22, 0x30, 0x20, 0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01,
    0x09, 0x01, 0x16, 0x13, 0x63, 0x61, 0x40, 0x6E, 0x6F, 0x61, 0x2E, 0x6E, 0x69, 0x6E, 0x74, 0x65,
    0x6E, 0x64, 0x6F, 0x2E, 0x63, 0x6F, 0x6D, 0x30, 0x1E, 0x17, 0x0D, 0x30, 0x37, 0x30, 0x36, 0x31,
    0x35, 0x31, 0x39, 0x33, 0x34, 0x33, 0x33, 0x5A, 0x17, 0x0D, 0x34, 0x39, 0x31, 0x32, 0x32, 0x38,
    0x31, 0x32, 0x30, 0x30, 0x30, 0x30, 0x5A, 0x30, 0x81, 0x95, 0x31, 0x0B, 0x30, 0x09, 0x06, 0x03,
    0x55, 0x04, 0x06, 0x13, 0x02, 0x55, 0x53, 0x31, 0x13, 0x30, 0x11, 0x06, 0x03, 0x55, 0x04, 0x08,
    0x13, 0x0A, 0x57, 0x61, 0x73, 0x68, 0x69, 0x6E, 0x67, 0x74, 0x6F, 0x6E, 0x31, 0x21, 0x30, 0x1F,
    0x06, 0x03, 0x55, 0x04, 0x0A, 0x13, 0x18, 0x4E, 0x69, 0x6E, 0x74, 0x65, 0x6E, 0x64, 0x6F, 0x20,
    0x6F, 0x66, 0x20, 0x41, 0x6D, 0x65, 0x72, 0x69, 0x63, 0x61, 0x20, 0x49, 0x6E, 0x63, 0x2E, 0x31,
    0x0C, 0x30, 0x0A, 0x06, 0x03, 0x55, 0x04, 0x0B, 0x13, 0x03, 0x4E, 0x4F, 0x41, 0x31, 0x1C, 0x30,
    0x1A, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13, 0x13, 0x4E, 0x69, 0x6E, 0x74, 0x65, 0x6E, 0x64, 0x6F,
    0x20, 0x43, 0x6C, 0x61, 0x73, 0x73, 0x20, 0x32, 0x20, 0x43, 0x41, 0x31, 0x22, 0x30, 0x20, 0x06,
    0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x09, 0x01, 0x16, 0x13, 0x63, 0x61, 0x40, 0x6E,
    0x6F, 0x61, 0x2E, 0x6E, 0x69, 0x6E, 0x74, 0x65, 0x6E, 0x64, 0x6F, 0x2E, 0x63, 0x6F, 0x6D, 0x30,
    0x81, 0x9F, 0x30, 0x0D, 0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x01, 0x05,
    0x00, 0x03, 0x81, 0x8D, 0x00, 0x30, 0x81, 0x89, 0x02, 0x81, 0x81, 0x00, 0xB4, 0xA8, 0x0F, 0xE3,
    0x80, 0x02, 0xA7, 0xAD, 0xFB, 0x59, 0x9D, 0xE1, 0x92, 0x9D, 0x19, 0x7B, 0x62, 0x00, 0x96, 0xCE,
    0x13, 0x2E, 0x5B, 0x83, 0xC0, 0x33, 0x26, 0xA6, 0x39, 0x34, 0xAC, 0x8E, 0xE7, 0x7F, 0x69, 0x7E,
    0xD4, 0xC0, 0x7C, 0xB7, 0x60, 0x86, 0xAF, 0xB6, 0x45, 0x06, 0x36, 0xE4, 0x5D, 0xEA, 0xED, 0x15,
    0x54, 0x36, 0x1D, 0x57, 0xB6, 0x9E, 0x0E, 0x07, 0x43, 0x62, 0x10, 0xEE, 0x35, 0xC2, 0x41, 0x34,
    0x38, 0xD0, 0xDD, 0x52, 0xAE, 0x86, 0xFD, 0x0C, 0x9D, 0x29, 0xFA, 0x3E, 0xB6, 0xBB, 0xD9, 0x51,
    0x38, 0x26, 0x24, 0x87, 0xA2, 0x20, 0x37, 0x4E, 0x8E, 0xBC, 0xD1, 0x04, 0x5B, 0x98, 0x18, 0x7E,
    0xA8, 0x54, 0xFD, 0xD0, 0xBF, 0xA6, 0x1B, 0x48, 0x0E, 0xE6, 0x19, 0xB4, 0xDF, 0xC4, 0x3F, 0x8A,
    0x19, 0xCE, 0xA0, 0x14, 0x23, 0xB1, 0x47, 0x12, 0x2C, 0x1E, 0x8C, 0xA7, 0x02, 0x03, 0x01, 0x00,
    0x01, 0xA3, 0x81, 0xF5, 0x30, 0x81, 0xF2, 0x30, 0x1D, 0x06, 0x03, 0x55, 0x1D, 0x0E, 0x04, 0x16,
    0x04, 0x14, 0xCB, 0x40, 0x16, 0x7C, 0xB1, 0x37, 0x2B, 0x26, 0x5A, 0x35, 0xDE, 0xBE, 0xF1, 0x5B,
    0x50, 0x8A, 0x8D, 0x0C, 0xD4, 0xBD, 0x30, 0x81, 0xC2, 0x06, 0x03, 0x55, 0x1D, 0x23, 0x04, 0x81,
    0xBA, 0x30, 0x81, 0xB7, 0x80, 0x14, 0xCB, 0x40, 0x16, 0x7C, 0xB1, 0x37, 0x2B, 0x26, 0x5A, 0x35,
    0xDE, 0xBE, 0xF1, 0x5B, 0x50, 0x8A, 0x8D, 0x0C, 0xD4, 0xBD, 0xA1, 0x81, 0x9B, 0xA4, 0x81, 0x98,
    0x30, 0x81, 0x95, 0x31, 0x0B, 0x30, 0x09, 0x06, 0x03, 0x55, 0x04, 0x06, 0x13, 0x02, 0x55, 0x53,
    0x31, 0x13, 0x30, 0x11, 0x06, 0x03, 0x55, 0x04, 0x08, 0x13, 0x0A, 0x57, 0x61, 0x73, 0x68, 0x69,
    0x6E, 0x67, 0x74, 0x6F, 0x6E, 0x31, 0x21, 0x30, 0x1F, 0x06, 0x03, 0x55, 0x04, 0x0A, 0x13, 0x18,
    0x4E, 0x69, 0x6E, 0x74, 0x65, 0x6E, 0x64, 0x6F, 0x20, 0x6F, 0x66, 0x20, 0x41, 0x6D, 0x65, 0x72,
    0x69, 0x63, 0x61, 0x20, 0x49, 0x6E, 0x63, 0x2E, 0x31, 0x0C, 0x30, 0x0A, 0x06, 0x03, 0x55, 0x04,
    0x0B, 0x13, 0x03, 0x4E, 0x4F, 0x41, 0x31, 0x1C, 0x30, 0x1A, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13,
    0x13, 0x4E, 0x69, 0x6E, 0x74, 0x65, 0x6E, 0x64, 0x6F, 0x20, 0x43, 0x6C, 0x61, 0x73, 0x73, 0x20,
    0x32, 0x20, 0x43, 0x41, 0x31, 0x22, 0x30, 0x20, 0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D,
    0x01, 0x09, 0x01, 0x16, 0x13, 0x63, 0x61, 0x40, 0x6E, 0x6F, 0x61, 0x2E, 0x6E, 0x69, 0x6E, 0x74,
    0x65, 0x6E, 0x64, 0x6F, 0x2E, 0x63, 0x6F, 0x6D, 0x82, 0x01, 0x01, 0x30, 0x0C, 0x06, 0x03, 0x55,
    0x1D, 0x13, 0x04, 0x05, 0x30, 0x03, 0x01, 0x01, 0xFF, 0x30, 0x0D, 0x06, 0x09, 0x2A, 0x86, 0x48,
    0x86, 0xF7, 0x0D, 0x01, 0x01, 0x05, 0x05, 0x00, 0x03, 0x81, 0x81, 0x00, 0x9C, 0x87, 0x51, 0xC1,
    0xE2, 0x26, 0x99, 0xC6, 0x97, 0x01, 0x3F, 0x5E, 0x89, 0xD9, 0x06, 0x28, 0x06, 0xB5, 0xA6, 0xC9,
    0xA4, 0x6F, 0xEA, 0xFE, 0xB2, 0xA8, 0x8A, 0x19, 0x08, 0x2A, 0xA9, 0xD3, 0x21, 0xB9, 0xCA, 0xFF,
    0xD0, 0x10, 0xE3, 0x45, 0x54, 0x65, 0x18, 0x1E, 0xBA, 0x96, 0x34, 0x6D, 0xDB, 0x0D, 0xAE, 0x7A,
    0x16, 0x4B, 0xAD, 0x4B, 0xA3, 0x3A, 0x44, 0xAE, 0x78, 0x73, 0x24, 0x70, 0xAD, 0xC7, 0x4E, 0x5A,
    0x63, 0x71, 0x56, 0x6E, 0x45, 0x5A, 0x63, 0x53, 0x09, 0xC2, 0xCF, 0x91, 0x48, 0x23, 0x3F, 0x43,
    0xC1, 0x6C, 0x9D, 0x78, 0x44, 0x93, 0x66, 0x40, 0xD8, 0x83, 0xC3, 0x98, 0xE3, 0xC9, 0x83, 0x9D,
    0x62, 0xBC, 0x38, 0x84, 0x46, 0x83, 0x45, 0x26, 0x7C, 0x27, 0x74, 0x25, 0x3B, 0xB3, 0x03, 0x66,
    0xD4, 0xD6, 0x19, 0xFE, 0x76, 0xD8, 0xD9, 0x6A, 0x99, 0x1C, 0x60, 0xE7, 0x00, 0x00, 0x00, 0x00,
};
s32 pat_ca_cert_size = 0x39C;

/* This unit's own functions are members of `NetCtrlWk` (and of the small records below) or
 * free helpers; each is declared in `include/Network/network_pat_control.h`. */

/*
 * The per-frame update of the online control: it services the queued action, then runs the state machine
 * selected by `sub_state_0x017` (login, server select, city select, lobby entry and shutdown).
 */
void updateNetworkPatControl(void)
{
    NetCtrlWk* work = net_ctrl_wk;
    NetListMenu* menu = work->list_0xBF34;
    NetDialog* dialog = work->dialog_0xBF30;
    u16 pressed = Psw[0].button_0x2C0.pressed_0x04;
    u16 held = Psw[0].button_0x2C0.held_0x14 | pressed;

    if (work->sub_state_0x017 != 0x21 && work->flag_0xC1BF != 1 && work->flag_0xC1BE != 1 &&
        work->sub_state_0x017 < 0x32 &&
        (work->error_0x054 != 0 || work->error_0x058 != 0 || work->error_0x05C != 0 || work->error_0x060 != 0 ||
         work->error_code_0xC258 != 0)) {
        setErrorHappened(work);
    }
    if (work->flag_0xC499 == 1 && work->flag_0xC49A == 0) {
        if (isTermsCheckFinished(getPatTerms()) == 1) {
            work->flag_0xC49A = 1;
            work->flag_0xC499 = 0;
        }
    }
    tickPatControl();
    switch (work->action_0xC152) {
    case 1:
        runPendingAction1();
        break;
    case 3:
        if (work->error_0x05C == 0) {
            work->status_0xA18C = 0;
            buildRosterSync(&work->roster_sync_0x61CC);
            getNetworkCommunityPat(getPatsObject(), 0)->syncFriends(&work->roster_sync_0x61CC);
        }
        break;
    case 4:
        net_session_close_start();
        break;
    case 5:
        runPendingAction5();
        break;
    case 6:
        runPendingAction6();
        break;
    case 7:
        runPendingAction7();
        break;
    case 8:
        runPendingAction8();
        break;
    case 2:
        getNetworkSessionManagerPat(getPatsObject(), 0)->setCircleRecords(work->record_0xC3F4, 0x88);
        break;
    }
    work->action_0xC152 = 0;
    if (work->flag_0xC499 != 0 && work->flag_0xC49A == 0) {
        if (work->flag_0xC49E == 1) {
            if (isTermsUpdateFinished(getPatTerms()) == 1) {
                work->flag_0xC49E = 2;
                setTransferMode(0);
            }
        }
        if (work->flag_0xC49E == 3) {
            work->flag_0xC49E = 0;
        }
    }
    switch (work->sub_state_0x017) {
    case 0x0:
        switch (work->step_0x018) {
        case 0:
            startBootLoad();
            work->step_0x018++;
            /* fallthrough */
        case 1: {
            s32 result = pollBootLoad();

            if (result != 0) {
                if (result == 1) {
                    work->sub_state_0x017 = 2;
                    return;
                }
                work->step_0x018++;
                return;
            }
            return;
        }
        case 2:
            work->sub_state_0x017 = 2;
            return;
        default:
            return;
        }
        break;
    case 0x2:
        getNetworkLogger()->setLevel_18(3);
        if (getInstance_() != NULL) {
            if (isMaintenanceMode(getInstance()) != 0) {
                abortNetworkControl(work, 1);
                return;
            }
            resetMediatorState(getInstance());
            updateOpeningState(getInstance(), 1, 0x9003C360, 0x4000);
            updateOpeningState(getInstance(), 2, 0x90040360, 0x4000);
            work->callback_0xC268();
            setReflectPageRange(getInstance(), (u32)patReflectCallback, 0);
            setReflectField30(getInstance(), 3);
            {
                s32 event = dispatchReflectEvent();

                setReflectField34(getInstance(), event);
            }
            work->group_0xC494 = language_group_table[system_w.field_0x09 * 2];
            work->group_max_0xC490 = language_group_table[system_w.field_0x09 * 2 + 1];
            setReflectField38(getInstance(), work->group_max_0xC490);
            setReflectName3C(getInstance(), work->player_name_0xC1C8);
            setReflectName5C(getInstance(), work->support_code_0xC1DE);
            updateTermVersion(getInstance(), 0);
            updatePatInterface180(getInstance(), (u32)pat_server_host, (u32)pat_ca_cert, pat_ca_cert_size);
            resetFailureState(work);
        }
        work->status_0x174 = 0;
        work->sub_state_0x017 = 0x96;
        return;
    case 0x96:
        work->status_0x174 = 0;
        resetMediatorFlags(getInstance());
        work->sub_state_0x017 = 0x97;
        return;
    case 0x97:
        if (work->status_0x174 != 0) {
            if (work->status_0x174 < 0) {
                abortNetworkControl(work, 2);
                return;
            }
            work->sub_state_0x017 = 0xA0;
            work->step_0x018 = 0;
            return;
        }
        break;
    case 0x98:
        switch (work->step_0x018) {
        case 0:
            if ((pressed & 0x20) != 0) {
                work->step_0x018 = 1;
                dialog->mode_0x126 = 1;
                dialog->value_0x127 = 0xE;
                dialog->timer_0x128 = 0xF;
                sysSE_req(1);
                return;
            }
            if (dialog->flag_0x124 != 0) {
                if ((pressed & 8) != 0 && dialog->flag_0x125 == 0) {
                    sysSE_req(3);
                    dialog->flag_0x125 = 1;
                    return;
                }
                if ((pressed & 4) != 0 && dialog->flag_0x125 == 1) {
                    sysSE_req(3);
                    dialog->flag_0x125 = 0;
                    return;
                }
            }
            if (dialog->timer_0x128 > 0) {
                dialog->timer_0x128--;
                return;
            }
            if ((pressed & 0x10) != 0) {
                sysSE_req(0);
                if (dialog->flag_0x125 == 1) {
                    work->terms_version_0xC200 = getInstance()->getOpeningTermsVersion();
                    work->callback_0xC270();
                    work->flag_0xC369 = 1;
                    return;
                }
                work->step_0x018 = 1;
                dialog->mode_0x126 = 1;
                dialog->value_0x127 = 0xE;
                dialog->timer_0x128 = 0xF;
                return;
            }
            break;
        case 1:
            if (dialog->timer_0x128 > 0) {
                dialog->timer_0x128--;
                return;
            }
            if ((pressed & 0x10) != 0) {
                sysSE_req(0);
                work->sub_state_0x017 = 0x5A;
                work->step_0x018 = 0;
                return;
            }
            if ((pressed & 0x20) != 0) {
                work->step_0x018 = 0;
                dialog->mode_0x126 = 0;
                dialog->value_0x127 = 0;
                dialog->timer_0x128 = 0xF;
                sysSE_req(1);
                return;
            }
            break;
        }
        break;
    case 0x99:
        if (dialog->timer_0x128 > 0) {
            dialog->timer_0x128--;
            return;
        }
        if ((pressed & 0x10) != 0) {
            sysSE_req(0);
            work->sub_state_0x017 = 0x5A;
            work->step_0x018 = 0;
            work->field_0xC378 = 0;
            return;
        }
        break;
    case 0x9A:
        switch (work->step_0x018) {
        case 0:
            if (dialog->mode_0x126 == 1) {
                if (dialog->timer_0x128 > 0) {
                    dialog->timer_0x128--;
                    return;
                }
                if ((pressed & 0x10) != 0) {
                    sysSE_req(0);
                    work->sub_state_0x017 = 0x5A;
                    work->step_0x018 = 0;
                    dialog->mode_0x126 = 0;
                    dialog->value_0x127 = 0;
                    return;
                }
                if ((pressed & 0x20) != 0) {
                    dialog->mode_0x126 = 0;
                    dialog->value_0x127 = 0;
                    dialog->timer_0x128 = 0xF;
                    sysSE_req(1);
                    return;
                }
            } else {
                if (dialog->timer_0x128 > 0) {
                    dialog->timer_0x128--;
                    return;
                }
                if ((pressed & 0x20) != 0) {
                    dialog->mode_0x126 = 1;
                    dialog->value_0x127 = 0xD;
                    dialog->timer_0x128 = 0xF;
                    sysSE_req(1);
                    return;
                }
                if ((pressed & 0x10) != 0) {
                    sysSE_req(0);
                    work->step_0x018++;
                    return;
                }
            }
            break;
        case 1:
            if (system_w.field_0x86A == 0) {
                work->sub_state_0x017 = 0xAC;
            } else {
                work->sub_state_0x017 = 3;
            }
            work->step_0x018 = 0;
            return;
        }
        break;
    case 0xA0:
        work->callback_0xC274();
        return;
    case 0xA1:
        if (work->step_0x018 != 1) {
            return;
        }
        if (work->callback_0xC27C() != 0) {
            work->sub_state_0x017 = 7;
            work->step_0x018 = 0x64;
            work->substep_0x019 = 0;
            work->cursor_0x01C = 1;
            work->cursor_0x020 = 0;
            work->scroll_0x024 = 0;
            work->repeat_0x02C = 0;
            work->repeat_0x030 = 0;
            work->hold_0xC48C = 0xF;
            return;
        }
        break;
    case 0xA8:
        dialog->timer_0x128++;
        work->sub_state_0x017 = 0xA9;
        return;
    case 0xA9:
        dialog->timer_0x128++;
        if (work->flag_0xC1BD == 1) {
            abortNetworkControl(work, 0);
            return;
        }
        work->sub_state_0x017 = 8;
        work->step_0x018 = 1;
        return;
    case 0xAC:
        work->status_0x174 = 0;
        work->error_0x058 = 0;
        work->error_code_0xC258 = 0;
        getNetworkLayerPat(getPatsObject(), 0)->closeSession_1C();
        work->flag_0x015 = 0;
        work->sub_state_0x017 = 0xAA;
        dialog->timer_0x128 = 0;
        return;
    case 0xAA:
        dialog->timer_0x128++;
        if (work->status_0x174 > 0 || work->error_0x058 != 0) {
            if (work->status_0x174 < 0 || work->error_0x058 < 0) {
                if (work->net_error_0x134 + 0x7FFA0000 == 0x34U) {
                    dialog->timer_0x128 = (getWarningUInt(getInstance()) + 1) * 0x1E;
                    work->sub_state_0x017 = 0xAE;
                    return;
                }
                if (work->net_error_0x138 == 0x72) {
                    work->sub_state_0x017 = 0xB3;
                    return;
                }
                abortNetworkControl(work, 3);
                return;
            }
            getAccountName(getInstance(), work->support_code_0xC1DE, 0x20);
            startAccountLoad();
            work->sub_state_0x017 = 0xAB;
            work->step_0x018 = 0;
            return;
        }
        break;
    case 0xAE:
        if (dialog->timer_0x128 >= 0x1E) {
            dialog->timer_0x128--;
            return;
        }
        if ((pressed & 0x10) != 0) {
            sysSE_req(0);
            if (queryOpeningFlag278(getInstance()) != 0) {
                work->sub_state_0x017 = 0xAF;
                work->callback_0xC280();
                return;
            }
            work->sub_state_0x017 = 0x5A;
            work->step_0x018 = 0;
            return;
        }
        break;
    case 0xAF: {
        s32 result = work->callback_0xC284();

        if (result < 0) {
            abortNetworkControl(work, 3);
            return;
        }
        if (result > 0) {
            work->sub_state_0x017 = 0x5A;
            work->step_0x018 = 0;
            return;
        }
        break;
    }
    case 0xAB: {
        s16 counter = dialog->timer_0x128;

        dialog->timer_0x128 = counter + 1;
        if (pollAccountLoad(counter) != 0) {
            work->sub_state_0x017 = 0xB1;
            work->step_0x018 = 0;
            return;
        }
        break;
    }
    case 0xB1:
        if (queryOpeningFlag2A8(getInstance()) == 0) {
            work->sub_state_0x017 = 0xB3;
            work->step_0x018 = 0;
            return;
        }
        dialog->timer_0x128 = (getWarningUInt(getInstance()) + 1) * 0x1E;
        work->sub_state_0x017 = 0xB2;
        work->step_0x018 = 0;
        return;
    case 0xB2:
        switch (work->step_0x018) {
        case 0:
            if (dialog->timer_0x128 >= 0x1E) {
                dialog->timer_0x128--;
                return;
            }
            work->step_0x018++;
            dialog->mode_0x126 = 0;
            return;
        case 1:
            if (dialog->mode_0x126 == 1) {
                if (dialog->timer_0x128 > 0) {
                    dialog->timer_0x128--;
                    return;
                }
                if ((pressed & 0x10) != 0) {
                    sysSE_req(0);
                    work->sub_state_0x017 = 0x5A;
                    work->step_0x018 = 0;
                    dialog->mode_0x126 = 0;
                    dialog->value_0x127 = 0;
                    return;
                }
                if ((pressed & 0x20) != 0) {
                    dialog->mode_0x126 = 0;
                    dialog->value_0x127 = 0;
                    sysSE_req(1);
                    return;
                }
            } else {
                if ((pressed & 0x20) != 0) {
                    dialog->mode_0x126 = 1;
                    dialog->value_0x127 = 0xD;
                    dialog->timer_0x128 = 0xF;
                    sysSE_req(1);
                    return;
                }
                if ((pressed & 0x10) != 0) {
                    sysSE_req(0);
                    work->sub_state_0x017 = 0xB3;
                    work->step_0x018 = 0;
                    return;
                }
            }
            break;
        }
        break;
    case 0xB3:
        if (queryOpeningFlag2C0(getInstance()) == 0) {
            NetUserData* user;

            work->flag_0x015 = 1;
            work->sub_state_0x017 = 7;
            work->step_0x018 = 0x64;
            work->substep_0x019 = 1;
            setupArenaVectors(work);
            work->cursor_0x01C = 1;
            work->cursor_0x020 = 0;
            work->scroll_0x024 = 0;
            work->repeat_0x02C = 0;
            work->repeat_0x030 = 0;
            work->hold_0xC48C = 0xF;
            user = (NetUserData*)get_userdata();
            work->name2_0x7372[0] = 0;
            strcpy(work->name2_0x7372, user->name_0x03);
            if (work->flag_0xC499 == 1 && work->flag_0xC49A == 0) {
                startTermsUpdate(getInstance());
                work->flag_0xC49E = 1;
                return;
            }
        } else {
            work->sub_state_0x017 = 0xB4;
            work->step_0x018 = 0;
            dialog->timer_0x128 = 0xF;
            return;
        }
        break;
    case 0xB4:
        if (dialog->timer_0x128 > 0) {
            dialog->timer_0x128--;
            return;
        }
        if ((pressed & 0x10) != 0) {
            sysSE_req(0);
            work->sub_state_0x017 = 0x5A;
            work->step_0x018 = 0;
            return;
        }
        break;
    case 0x7:
        for (;;) {
            switch (work->step_0x018) {
            case 0x0: {
                s32 handled = 0;

                ai_npc_reaction_forward();
                if (work->repeat_0x02C > 0) {
                    work->repeat_0x02C--;
                }
                if (work->repeat_0x030 > 0) {
                    work->repeat_0x030--;
                }
                if (work->hold_0xC48C > 0) {
                    work->hold_0xC48C--;
                    return;
                }
                if ((pressed & 8) != 0 && work->cursor_0x01C == 0) {
                    sysSE_req(3);
                    work->cursor_0x01C = 1;
                    return;
                }
                if ((pressed & 4) != 0 && work->cursor_0x01C == 1) {
                    sysSE_req(3);
                    work->cursor_0x01C = 0;
                    return;
                }
                if (work->cursor_0x01C != 0) {
                    if ((held & 1) != 0 && (work->cursor_0x020 > 0 || work->scroll_0x024 > 0)) {
                        if (work->cursor_0x020 > 0) {
                            sysSE_req(3);
                            work->cursor_0x020--;
                            return;
                        }
                        sysSE_req(3);
                        work->scroll_0x024--;
                        if (work->scroll_0x024 > 0) {
                            work->repeat_0x02C = 4;
                            return;
                        }
                        handled = 1;
                    } else if ((held & 2) != 0 &&
                               (work->cursor_0x020 < 2 ||
                                (u32)(work->scroll_0x024 + work->cursor_0x020) < (u32)(work->row_count_0x178 - 1))) {
                        if (work->cursor_0x020 < 2) {
                            sysSE_req(3);
                            work->cursor_0x020++;
                            return;
                        }
                        sysSE_req(3);
                        work->scroll_0x024++;
                        if ((u32)(work->scroll_0x024 + work->cursor_0x020) < (u32)(work->row_count_0x178 - 1)) {
                            work->repeat_0x030 = 4;
                            return;
                        }
                        handled = 1;
                    }
                }
                if (handled == 0) {
                    if ((held & 0x20) != 0) {
                        sysSE_req(1);
                        work->step_0x018 = 1;
                        dialog->timer_0x128 = 0xF;
                        return;
                    }
                    if ((pressed & 0x10) != 0) {
                        sysSE_req(0);
                        if (work->cursor_0x01C == 0) {
                            work->sub_state_0x017 = 0xA1;
                            work->step_0x018 = 1;
                            work->substep_0x019 = 0;
                            work->callback_0xC278();
                            work->screen_0xC158 = 4;
                            work->screen_0xC159 = 8;
                            work->field_0xC378 = 1;
                            return;
                        }
                        {
                            NetUserData* user = (NetUserData*)get_userdata();
                            NetRowRec* row = &work->rows_0x17C[work->scroll_0x024 + work->cursor_0x020];

                            work->status_0x6248 = 0;
                            work->status_0x174 = 0;
                            setPatField854(getInstance_(), row->id_0x00);
                            setPatField860(getInstance_(), user->name_0x03);
                            setPatByteD400(getInstance_(), (u8)(work->group_0xC494 * 15 + 5));
                            setPatByte6138On(getInstance_());
                            work->name_0x7368[0] = 0;
                            work->name2_0x7372[0] = 0;
                            strcpy(work->name_0x7368, row->name_0x04);
                            strcpy(work->name2_0x7372, user->name_0x03);
                            work->sub_state_0x017 = 8;
                            work->step_0x018 = 0;
                            return;
                        }
                    }
                }
                break;
            }
            case 0x1:
                if (dialog->timer_0x128 > 0) {
                    dialog->timer_0x128--;
                    return;
                }
                if ((pressed & 0x20) != 0) {
                    sysSE_req(1);
                    work->step_0x018 = 0;
                    return;
                }
                if ((pressed & 0x10) != 0) {
                    sysSE_req(0);
                    work->sub_state_0x017 = 0x5A;
                    work->step_0x018 = 0;
                    return;
                }
                break;
            case 0x64:
                if (get_option_cfg(0x1C) == 0) {
                    work->step_0x018 = 0;
                    continue;
                }
                if (work->flag_0xC49F == 1) {
                    work->step_0x018 = 0;
                    continue;
                }
                if (work->flag_0xC499 == 0 && work->flag_0xC49A == 0) {
                    work->step_0x018 = 0;
                    continue;
                }
                work->step_0x018 = 0x65;
                /* fallthrough */
            case 0x65:
                if ((pressed & 0x10) != 0) {
                    sysSE_req(0);
                    work->step_0x018 = 0;
                    work->flag_0xC49F = 1;
                    return;
                }
                break;
            }
            break;
        }
        break;
    case 0x8:
        dialog->timer_0x128++;
        switch (work->step_0x018) {
        case 0:
            if (work->status_0x6248 != 0) {
                if (work->status_0x6248 < 0) {
                    setErrorHappened(work);
                    return;
                }
                work->sub_state_0x017 = 0xA8;
                return;
            }
            break;
        case 1:
            work->mode_0x010 = 2;
            getNetworkSessionManagerPat(getPatsObject(), 0)->request364();
            work->sub_state_0x017 = 9;
            work->step_0x018 = 0;
            work->fetch_step_0x82D0 = 0;
            work->fetch_index_0x82D4 = 0;
            return;
        }
        break;
    case 0x9:
        dialog->timer_0x128++;
        if (work->entered_0x014 == 1) {
            work->sub_state_0x017 = 0xA;
            getNetworkCommunityPat(getPatsObject(), 0)->openCommunity_1C();
            return;
        }
        break;
    case 0xA:
        dialog->timer_0x128++;
        if (work->flag_0x016 == 1) {
            flushRosterSync();
            work->sub_state_0x017 = 5;
            work->step_0x018 = 0;
            work->fetch_step_0x82D0 = 0;
            work->fetch_index_0x82D4 = 0;
            return;
        }
        break;
    case 0x5: {
        s32 result;

        dialog->timer_0x128++;
        result = work->stepFetch(1);
        if (result != 0) {
            if (result < 0) {
                setErrorCode(5);
                setErrorHappened(work);
                return;
            }
            {
                s32 i;

                for (i = 0; i < 8; i++) {
                    work->fetch_timeouts_0xC37C[i] = get_server_big_data_timeout_element(i);
                }
            }
            work->status_0x628C = 0;
            getNetworkLayerPat(getPatsObject(), 0)->requestServers_24(0x50);
            work->sub_state_0x017 = 0xB;
            dialog->timer_0x128 = 0;
            work->step_0x018 = 0;
            work->cursor_0x01C = 0;
            work->flag_0xC3F0 = 0;
            return;
        }
        break;
    }
    case 0xB:
        dialog->timer_0x128++;
        if (work->status_0x628C != 0) {
            NetSrvRec* server;
            u32 i;
            s32 n;

            if (work->status_0x628C < 0) {
                setErrorHappened(work);
                return;
            }
            work->sub_state_0x017 = 0xC;
            work->step_0x018 = 0;
            work->substep_0x019 = 0;
            work->cursor_0x01C = 0;
            work->type_ids_0xA100[0] = 0;
            work->type_ids_0xA100[1] = 0;
            work->type_ids_0xA100[2] = 0;
            work->type_ids_0xA100[3] = 0;
            work->field_0xA120[0] = 0;
            work->field_0xA120[1] = 0;
            work->field_0xA120[2] = 0;
            work->field_0xA120[3] = 0;
            work->group_count_0xA110 = 0;
            work->selected_server_index_0xA114 = 0;
            work->field_0xA118 = 0;
            work->chosen_type_0xA11C = 0;
            server = work->servers_0x668;
            for (i = 0; i < work->server_count_0x660; i++, server++) {
                s32 type = server->type_0x3C - 1;

                if (type >= 0) {
                    work->type_ids_0xA100[type]++;
                }
            }
            for (n = 0; n < 4; n++) {
                if (work->type_ids_0xA100[n] != 0) {
                    strcpy(menu->rows_0x00C[work->group_count_0xA110].name_0x02, get_server_type_name(n));
                    strcpy(menu->rows_0x00C[work->group_count_0xA110].description_0x12, get_server_type_desc(n));
                    menu->rows_0x00C[work->group_count_0xA110].id_0x00 = work->type_ids_0xA100[n];
                    work->group_count_0xA110++;
                }
            }
            menu->count_0x008 = work->group_count_0xA110;
            menu->cursor_0x00A = 0;
            menu->mode_0x000 = 2;
            for (; n < 8; n++) {
                menu->rows_0x00C[n].id_0x00 = 0;
                menu->rows_0x00C[n].name_0x02[0] = 0;
                menu->rows_0x00C[n].description_0x12[0] = 0;
            }
            work->screen_0xC150 = 1;
            work->hold_0xC48C = 0xF;
            return;
        }
        break;
    case 0xC:
        ai_npc_reaction_forward();
        menu->field_0x006 = 0;
        switch (work->step_0x018) {
        case 0:
            switch (work->substep_0x019) {
            case 0:
                if (work->hold_0xC48C > 0) {
                    work->hold_0xC48C--;
                } else if ((held & 3) != 0) {
                    menu->cursor_0x00A = menu_cursor_step(menu->cursor_0x00A, work->group_count_0xA110, held, 1, 2);
                    work->cursor_0x01C = menu->cursor_0x00A;
                } else if ((held & 0x20) != 0) {
                    sysSE_req(1);
                    work->step_0x018 = 2;
                    work->substep_0x019 = 0;
                    dialog->timer_0x128 = 0xF;
                } else if ((pressed & 0x10) != 0) {
                    s32 seen = 0;
                    s32 chosen;

                    for (chosen = 0; chosen < 4; chosen++) {
                        if (work->type_ids_0xA100[chosen] != 0) {
                            if (seen == work->cursor_0x01C) {
                                break;
                            }
                            seen++;
                        }
                    }
                    if (chosen >= 4 || can_enter_server(chosen, get_hunter_rank_max(*(u8**)lobby_world_block)) == 0) {
                        work->substep_0x019 = 1;
                        dialog->timer_0x128 = 0xF;
                        sysSE_req(2);
                    } else {
                        s32 i;

                        sysSE_req(0);
                        work->chosen_type_0xA11C = chosen;
                        work->step_0x018 = 1;
                        work->substep_0x019 = 0;
                        work->cursor_0x020 = 0;
                        work->screen_0xC150 = 2;
                        menu->cursor_0x49C = 0;
                        menu->mode_0x000 = 2;
                        menu->count_0x49E = 0;
                        for (i = 0; i < work->type_ids_0xA100[work->chosen_type_0xA11C]; i++) {
                            menu->count_0x49E++;
                            if (menu->count_0x49E >= 8) {
                                break;
                            }
                        }
                        NetCtrlWk::listServers(work->chosen_type_0xA11C + 1, menu->views_0x4A4, 0, 8);
                    }
                }
                break;
            case 1:
                if (dialog->timer_0x128 > 0) {
                    dialog->timer_0x128--;
                } else if ((pressed & 0x10) != 0) {
                    work->substep_0x019 = 0;
                    sysSE_req(0);
                }
                break;
            }
            refreshServerScreen(work);
            return;
        case 1: {
            s32 i;

            menu->page_0x4A0 = work->cursor_0x020 / 8;
            menu->pages_0x4A2 = work->type_ids_0xA100[work->chosen_type_0xA11C] / 8;
            if (work->type_ids_0xA100[work->chosen_type_0xA11C] % 8 != 0) {
                menu->pages_0x4A2++;
            }
            menu->count_0x49E = 0;
            for (i = menu->page_0x4A0 * 8; i < work->type_ids_0xA100[work->chosen_type_0xA11C]; i++) {
                menu->count_0x49E++;
                if (menu->count_0x49E >= 8) {
                    break;
                }
            }
            NetCtrlWk::listServers(work->chosen_type_0xA11C + 1, menu->views_0x4A4, menu->page_0x4A0 * 8, 8);
            if (menu->cursor_0x49C >= menu->count_0x49E) {
                menu->cursor_0x49C = menu->count_0x49E - 1;
            }
            if ((held & 3) != 0) {
                menu->cursor_0x49C = menu_cursor_step(menu->cursor_0x49C, menu->count_0x49E, held, 1, 2);
                work->cursor_0x020 = menu->cursor_0x49C + menu->page_0x4A0 * 8;
                return;
            }
            if ((held & 0xC) != 0) {
                menu->page_0x4A0 = menu_cursor_step_forward(menu->page_0x4A0, menu->pages_0x4A2, held, 4, 8, 6, (u16*)&menu->field_0x006);
                work->cursor_0x020 = menu->cursor_0x49C + menu->page_0x4A0 * 8;
                return;
            }
            if ((pressed & 0x20) != 0) {
                work->step_0x018 = 0;
                work->screen_0xC150 = 1;
                menu->mode_0x000 = 2;
                sysSE_req(1);
                work->hold_0xC48C = 0xF;
                return;
            }
            if ((pressed & 0x10) != 0) {
                NetSrvRec* server;
                s32 seen;
                u32 index;

                sysSE_req(0);
                seen = 0;
                server = work->servers_0x668;
                for (index = 0; index < work->server_count_0x660; index++, server++) {
                    s32 type = server->type_0x3C - 1;

                    if (type >= 0 && type == work->chosen_type_0xA11C) {
                        if (seen == work->cursor_0x020) {
                            work->selected_server_index_0xA114 = index;
                            break;
                        }
                        seen++;
                    }
                }
                work->field_0xA120[0] = work->cursor_0x01C;
                work->field_0xA120[1] = work->cursor_0x020;
                work->status_0x6290 = 0;
                getNetworkLayerPat(getPatsObject(), 0)->selectServer_28(server->id_0x00);
                work->sub_state_0x017 = 0xF;
                dialog->timer_0x128 = 0;
                work->step_0x018 = 0;
                work->substep_0x019 = 0;
                work->cursor_0x01C = 0;
                work->cursor_0x020 = 0;
                work->screen_0xC150 = 3;
                return;
            }
            refreshServerScreen(work);
            return;
        }
        case 2:
            if (dialog->timer_0x128 > 0) {
                dialog->timer_0x128--;
                return;
            }
            if ((pressed & 0x20) != 0) {
                sysSE_req(1);
                work->step_0x018 = 0;
                return;
            }
            if ((pressed & 0x10) != 0) {
                sysSE_req(0);
                work->sub_state_0x017 = 0x5A;
                work->step_0x018 = 0;
                work->substep_0x019 = 0;
                work->screen_0xC150 = 0;
                return;
            }
            break;
        }
        break;
    case 0xF: {
        s16 counter = dialog->timer_0x128;

        dialog->timer_0x128 = counter + 1;
        ai_npc_reaction_forward();
        if (work->status_0x6290 != 0) {
            NetworkSmallObject id;

            if (work->status_0x6290 < 0) {
                failNetworkControl(work);
                return;
            }
            networkSmallObject_construct(&id);
            getNetworkLayerPat(getPatsObject(), 0)->readServerId_2C((NetId*)&id);
            work->name_0x7368[0] = 0;
            formatNetId(work->name_0x7368, (NetId*)&id);
            NetworkSmallObjectSink::destroy(&id);
            flushRosterSync();
            refreshRosterCache();
            work->sub_state_0x017 = 0x10;
            dialog->timer_0x128 = 0;
            work->step_0x018 = 0;
            work->fetch_step_0x82D0 = 0;
            work->fetch_index_0x82D4 = 0;
            return;
        }
        break;
    }
    case 0x10: {
        s16 counter = dialog->timer_0x128;
        s32 result;

        dialog->timer_0x128 = counter + 1;
        ai_npc_reaction_forward();
        result = work->stepFetch(2);
        if (result != 0) {
            if (result < 0) {
                setErrorCode(5);
                setErrorHappened(work);
                return;
            }
            work->sub_state_0x017 = 0x11;
            dialog->timer_0x128 = 0;
            work->step_0x018 = 0;
            work->fetch_step_0x82D0 = 0;
            work->fetch_index_0x82D4 = 0;
            return;
        }
        break;
    }
    case 0x11: {
        s16 counter = dialog->timer_0x128;
        s32 result;

        dialog->timer_0x128 = counter + 1;
        ai_npc_reaction_forward();
        result = work->stepFetch(3);
        if (result != 0) {
            work->timeout_0xC36C = get_server_big_data_timeout_element(0);
            if (result < 0) {
                setErrorCode(5);
                setErrorHappened(work);
                return;
            }
            work->sub_state_0x017 = 0x12;
            dialog->timer_0x128 = 0;
            work->step_0x018 = 0;
            work->fetch_step_0x82D0 = 0;
            work->fetch_index_0x82D4 = 0;
            return;
        }
        break;
    }
    case 0x12: {
        s16 counter = dialog->timer_0x128;
        s32 result;

        dialog->timer_0x128 = counter + 1;
        ai_npc_reaction_forward();
        result = work->stepFetch(4);
        if (result != 0) {
            if (result < 0) {
                setErrorCode(5);
                setErrorHappened(work);
                return;
            }
            work->status_0xA15C = 0;
            getNetworkCommunityPat(getPatsObject(), 0)->requestNews_38();
            work->sub_state_0x017 = 0x13;
            dialog->timer_0x128 = 0;
            work->step_0x018 = 0;
            work->fetch_step_0x82D0 = 0;
            work->fetch_index_0x82D4 = 0;
            return;
        }
        break;
    }
    case 0x13: {
        s16 counter = dialog->timer_0x128;

        dialog->timer_0x128 = counter + 1;
        ai_npc_reaction_forward();
        if (work->status_0xA15C != 0) {
            if (work->status_0xA15C < 0) {
                setErrorCode(5);
                setErrorHappened(work);
                return;
            }
            work->status_0xA1A4 = 0;
            getNetworkCommunityPat(getPatsObject(), 0)->requestBlockList();
            work->sub_state_0x017 = 0x14;
            dialog->timer_0x128 = 0;
            work->step_0x018 = 0;
            work->fetch_step_0x82D0 = 0;
            work->fetch_index_0x82D4 = 0;
            return;
        }
        break;
    }
    case 0x14: {
        s16 counter = dialog->timer_0x128;

        dialog->timer_0x128 = counter + 1;
        ai_npc_reaction_forward();
        if (work->status_0xA1A4 != 0) {
            if (work->status_0xA1A4 < 0) {
                setErrorCode(5);
                setErrorHappened(work);
                return;
            }
            work->layer_state_0x064 = 0;
            if (isOnlineFlagClear() == 0) {
                work->status_0xA18C = 0;
                buildRosterSync(&work->roster_sync_0x61CC);
                getNetworkCommunityPat(getPatsObject(), 0)->syncFriends(&work->roster_sync_0x61CC);
            }
            work->sub_state_0x017 = 0x15;
            work->step_0x018 = 0;
            work->fetch_step_0x82D0 = 0;
            work->fetch_index_0x82D4 = 0;
            return;
        }
        break;
    }
    case 0x15: {
        s16 counter = dialog->timer_0x128;

        dialog->timer_0x128 = counter + 1;
        ai_npc_reaction_forward();
        if (isOnlineFlagClear() == 0) {
            if (work->status_0xA18C == 0) {
                break;
            }
            if (work->status_0xA18C < 0) {
                setErrorCode(5);
                setErrorHappened(work);
                return;
            }
        }
        work->status_0x6268 = 0;
        getNetworkLayerPat(getPatsObject(), 0)->requestCities_48(0x28);
        work->sub_state_0x017 = 0x16;
        work->step_0x018 = 0;
        work->substep_0x019 = 0;
        work->layer_state_0x064 = 0;
        dialog->timer_0x128 = 0;
        syncScheduleClock(work);
        menu->count_0x49E = 0;
        NetCtrlWk::copyCityListViews(menu->views2_0x60C, 0, 8);
        return;
    }
    case 0x16:
        ai_npc_reaction_forward();
        menu->field_0x006 = 0;
        switch (work->step_0x018) {
        case 0:
            if (work->substep_0x019 == 0) {
                dialog->timer_0x128++;
                if (work->status_0x6268 == 0) {
                    break;
                }
                if (work->status_0x6268 < 0) {
                    failNetworkControl(work);
                    return;
                }
            }
            work->total_0xA140 = getNetworkLayerPat(getPatsObject(), 0)->cities_540.count_0x000;
            work->per_page_0xA13C = 8;
            work->pages_0xA138 = work->total_0xA140 / work->per_page_0xA13C;
            if (work->total_0xA140 % work->per_page_0xA13C != 0) {
                work->pages_0xA138++;
            }
            work->step_0x018 = 1;
            work->hold_0xC48C = 0xF;
            work->cursor_0x01C = 0;
            work->cursor_0x020 = 0;
            menu->cursor2_0x604 = 0;
            menu->pages2_0x60A = work->pages_0xA138;
            menu->page2_0x608 = 0;
            menu->mode_0x000 = 3;
            return;
        case 1: {
            s32 i;

            menu->count2_0x606 = 0;
            for (i = menu->page2_0x608 * 8; i < work->total_0xA140; i++) {
                menu->count2_0x606++;
                if (menu->count2_0x606 >= 8) {
                    break;
                }
            }
            NetCtrlWk::copyCityListViews(menu->views2_0x60C, menu->page2_0x608 * 8, 8);
            if (work->hold_0xC48C > 0) {
                work->hold_0xC48C--;
                return;
            }
            if ((held & 3) != 0) {
                menu->cursor2_0x604 = menu_cursor_step(menu->cursor2_0x604, menu->count2_0x606, held, 1, 2);
                work->cursor_0x020 = menu->cursor2_0x604;
                return;
            }
            if ((held & 0xC) != 0) {
                menu->page2_0x608 = menu_cursor_step_forward(menu->page2_0x608, menu->pages2_0x60A, held, 4, 8, 6, (u16*)&menu->field_0x006);
                work->cursor_0x01C = menu->page2_0x608;
                return;
            }
            if ((pressed & 0x20) != 0) {
                work->sub_state_0x017 = 0xC;
                work->step_0x018 = 1;
                work->substep_0x019 = 0;
                work->cursor_0x01C = work->field_0xA120[0];
                work->cursor_0x020 = work->field_0xA120[1];
                work->screen_0xC150 = 2;
                menu->mode_0x000 = 2;
                sysSE_req(1);
                work->hold_0xC48C = 0xF;
                return;
            }
            if ((pressed & 0x10) != 0) {
                NetCityList* list;
                NetCityRec* city;

                sysSE_req(0);
                list = &getNetworkLayerPat(getPatsObject(), 0)->cities_540;
                work->chosen_city_0xA134 = work->cursor_0x020 + work->cursor_0x01C * work->per_page_0xA13C;
                city = &list->entries_0x004[work->chosen_city_0xA134];
                strcpy(work->account_name_0xC0C4, city->name_0x04);
                work->status_0x6258 = 0;
                getNetworkLayerPat(getPatsObject(), 0)->selectCity_3C(city->id_0x00);
                work->sub_state_0x017 = 0x17;
                work->step_0x018 = 0;
                work->substep_0x019 = 0;
                work->cursor_0x01C = 0;
                work->cursor_0x020 = 0;
            }
            refreshServerScreen(work);
            return;
        }
        }
        break;
    case 0x17:
        if (work->status_0x6258 != 0) {
            if (work->status_0x6258 < 0) {
                failNetworkControl(work);
                work->account_name_0xC0C4[0] = 0;
                return;
            }
            refreshRosterCache();
            resetMessagePool();
            work->sub_state_0x017 = 0x19;
            work->mode_0x010 = 3;
            work->flag_0x82C4 = 0;
            work->flag_0x82C5 = 0;
            work->flag_0x82C7 = 0;
            work->field_0x82C8 = 0;
            work->screen_0xC150 = 4;
            clearRefreshTimeout();
            return;
        }
        break;
    case 0x18:
        if (work->status_0x6258 != 0) {
            if (work->status_0x6258 < 0) {
                setErrorHappened(work);
                return;
            }
            resetMessagePool();
            work->sub_state_0x017 = 0x19;
            work->mode_0x010 = 3;
            work->counter_0xC360 = 0;
            work->counter_0xC362 = 0;
            work->counter_0xC364 = 0;
            return;
        }
        break;
    case 0x1F:
        switch (work->step_0x018) {
        case 0:
            if (work->status_0xA18C != 0) {
                if (work->status_0xA18C < 0) {
                    setErrorCode(5);
                    setErrorHappened(work);
                } else {
                    work->status_0xA174 = 0;
                    startRosterFetch(0);
                    work->step_0x018 = 1;
                }
            }
            break;
        case 1:
            if (work->status_0xA174 != 0) {
                if (work->status_0xA174 < 0) {
                    setErrorCode(5);
                    setErrorHappened(work);
                } else {
                    work->step_0x018 = 2;
                    sysSE_stop(0x1F);
                    resumeSoundEngine();
                    work->delay_0xC488 = 0x3C;
                }
            }
            break;
        case 2:
            if (work->delay_0xC488 > 0) {
                work->delay_0xC488--;
            } else {
                lb_entry_flags_clear();
                work->sub_state_0x017 = 0x20;
                work->step_0x018 = 0;
            }
            break;
        }
        /* fallthrough */
    case 0x19:
    case 0x1A:
    case 0x1B:
    case 0x1C:
    case 0x1D:
    case 0x1E:
        if (work->flag_0xC3F0 == 1) {
            work->sub_state_0x017 = 0x26;
            work->step_0x018 = 0;
            work->mode_0x010 = 2;
            return;
        }
        if (work->flag_0xBF2C == 1) {
            work->sub_state_0x017 = 0x5A;
            work->step_0x018 = 0;
            return;
        }
        updateMessagePool();
        if (work->layer_state_0x064 == 2) {
            switch (work->flag_0x82C5) {
            case 0:
                work->flag_0x82C5 = 1;
                /* fallthrough */
            case 1: {
                s32 result = work->stepFileDownloads();

                if (result != 0) {
                    if (result < 0) {
                        work->sub_error_0xC259 = 1;
                    } else {
                        work->flag_0x82C6 = 1;
                    }
                    work->flag_0x82C5 = 2;
                }
                break;
            }
            default:
                switch (work->flag_0x82C7) {
                case 0:
                    work->flag_0x82C7 = 1;
                    work->link_state_0xC0BC = 0;
                    work->status_0x62D0 = 0;
                    getNetworkLayerPat(getPatsObject(), 0)->requestAccount_84(0x20);
                    /* fallthrough */
                case 1:
                    if (work->link_state_0xC0BC != 0) {
                        work->flag_0x82C7 = 2;
                    }
                    break;
                default:
                    if (work->flag_0x82C4 == 0) {
                        work->flag_0x82C4 = 1;
                        startShutdownTimer();
                    }
                    break;
                }
                break;
            }
        }
        updateTransferQueue();
        updateTransferMode();
        return;
    case 0x3:
        work->entered_0x014 = 0;
        getNetworkSessionManagerPat(getPatsObject(), 0)->request364();
        work->sub_state_0x017 = 0xD;
        return;
    case 0xD:
        if (work->status_0x174 != 0) {
            if (work->status_0x174 < 0) {
                setErrorHappened(work);
                return;
            }
            work->sub_state_0x017 = 7;
            work->step_0x018 = 0;
            work->entered_0x014 = 1;
            setupArenaVectors(work);
            dialog->timer_0x128 = 0xF;
            return;
        }
        break;
    case 0x20:
        if (work->flag_0x04C == 1) {
            work->counter_0xC360 = 0;
            work->counter_0xC362 = 0;
            work->counter_0xC364 = 0;
            work->sub_state_0x017 = 0x19;
            work->mode_0x010 = 3;
            work->flag_0x82C4 = 0;
            work->flag_0x82C5 = 0;
            work->flag_0x82C7 = 0;
            work->field_0x82C8 = 0;
            lb_quest_board_reset(0, 0);
            return;
        }
        work->state_0x011 = 7;
        work->sub_state_0x017 = 0x21;
        work->step_0x018 = 0;
        return;
    case 0x21: {
        NetCtrlEntry* entry;
        s32 i;

        for (i = 0; i < 16; i++) {
            if (work->entries_0x7CD8[i].in_use_0x00 == 2) {
                break;
            }
        }
        if (i == 16) {
            entry = work->entries_0x7CD8;
            for (i = 0; i < 16; i++, entry++) {
                if (entry->in_use_0x00 == 1) {
                    if (entry->kind_0x02 == 0 || entry->kind_0x02 == 1) {
                        clearPhaseSlot(0x12);
                        getNetworkSessionManagerPat(getPatsObject(), 0)->request412((u32)entry->name_0x04, entry->value_0x4C, -1);
                    } else {
                        s8 slot = lookupFriendSlot((u8*)entry->text_0x50);

                        if (slot >= 0) {
                            clearPhaseSlot(0x12);
                            getNetworkSessionManagerPat(getPatsObject(), 0)->request412((u32)entry->name_0x04, entry->value_0x4C, slot);
                        } else {
                            work->status_0x6280 = 0;
                            getNetworkLayerPat(getPatsObject(), 0)->sendMessage_64(entry->name_0x04, entry->value_0x4C, 1, entry->flags_0x03);
                        }
                    }
                    entry->in_use_0x00 = 2;
                    break;
                }
            }
        }
        updateTransferQueue();
        updateTransferMode();
        return;
    }
    case 0x22:
        work->flag_0x82C4 = 0;
        work->flag_0x82C5 = 0;
        work->flag_0x82C7 = 0;
        return;
    case 0x24:
        if (work->step_0x018 != 0) {
            work->step_0x018--;
            return;
        }
        if ((pressed & 0x10) != 0) {
            sysSE_req(0);
            work->sub_error_0xC259 = 0;
            work->status_0x628C = 0;
            getNetworkLayerPat(getPatsObject(), 0)->requestServers_24(0x50);
            work->sub_state_0x017 = 0xB;
            dialog->timer_0x128 = 0;
            work->step_0x018 = 0;
            work->substep_0x019 = 0;
            work->cursor_0x01C = 0;
            work->flag_0xC3F0 = 0;
            return;
        }
        break;
    case 0x25:
        if (work->step_0x018 != 0) {
            work->step_0x018--;
            return;
        }
        if ((pressed & 0x10) != 0) {
            sysSE_req(0);
            work->sub_error_0xC259 = 0;
            work->sub_state_0x017 = 0x16;
            work->step_0x018 = 0;
            work->substep_0x019 = 1;
            work->layer_state_0x064 = 0;
            syncScheduleClock(work);
            work->screen_0xC150 = 3;
            work->flag_0x82C4 = 0;
            work->flag_0x82C5 = 0;
            work->flag_0x82C7 = 0;
            work->total_0xA140 = 0;
            work->per_page_0xA13C = 0;
            work->pages_0xA138 = 0;
            work->cursor_0x01C = 0;
            work->cursor_0x020 = 0;
            menu->cursor2_0x604 = 0;
            menu->pages2_0x60A = work->pages_0xA138;
            menu->page2_0x608 = 0;
            menu->mode_0x000 = 4;
            NetCtrlWk::copyCityListViews(menu->views2_0x60C, menu->page2_0x608 * 8, 8);
            return;
        }
        break;
    case 0x26:
        work->dialog_0xBF30 = work->create_dialog_0xC25C(work);
        allocateDialogRecord();
        work->mode_0x010 = 2;
        work->status_0x628C = 0;
        getNetworkLayerPat(getPatsObject(), 0)->requestServers_24(0x50);
        work->sub_state_0x017 = 0xB;
        dialog->timer_0x128 = 0;
        work->step_0x018 = 0;
        work->cursor_0x01C = 0;
        work->flag_0xC3F0 = 0;
        work->flag_0xC3F2 = 1;
        work->screen_0xC150 = 1;
        return;
    case 0x5A:
        switch (work->step_0x018) {
        case 0:
            setTransferMode(0);
            if (NetCtrlWk::isSessionManagerReady() == 1) {
                work->flag_0x098 = 0;
                clearPhaseSlot(0x20);
                getNetworkSessionManagerPat(getPatsObject(), 0)->request444();
                work->step_0x018++;
                return;
            }
            work->step_0x018 = 4;
            return;
        case 1:
            if (isPhaseSlotDone(0x20) != 0) {
                clearPhaseSlot(0x10);
                getNetworkSessionManagerPat(getPatsObject(), 0)->request408();
                work->step_0x018++;
                return;
            }
            break;
        case 2:
            if (isPhaseSlotDone(0x10) != 0) {
                clearPhaseSlot(8);
                getNetworkSessionManagerPat(getPatsObject(), 0)->request392();
                work->step_0x018++;
                return;
            }
            break;
        case 3:
            if (isPhaseSlotDone(8) != 0) {
                work->step_0x018++;
                return;
            }
            break;
        case 4:
            if (getNetworkCommunityPat(getPatsObject(), 0) != NULL) {
                work->status_0xA14C = 0;
                getNetworkCommunityPat(getPatsObject(), 0)->shutdown_20();
            }
            work->step_0x018++;
            return;
        case 5:
            if (getNetworkCommunityPat(getPatsObject(), 0) == NULL || work->status_0xA14C != 0) {
                if (getNetworkSessionManagerPat(getPatsObject(), 0) != NULL) {
                    clearPhaseSlot(2);
                    getNetworkSessionManagerPat(getPatsObject(), 0)->request368();
                }
                work->step_0x018++;
                return;
            }
            break;
        case 6:
            if (getNetworkSessionManagerPat(getPatsObject(), 0) == NULL || isPhaseSlotDone(2) != 0) {
                if (getNetworkLayerPat(getPatsObject(), 0) != NULL) {
                    work->status_0x624C = 0;
                    getNetworkLayerPat(getPatsObject(), 0)->shutdown_20();
                }
                work->step_0x018++;
                return;
            }
            break;
        case 7:
            if (getNetworkLayerPat(getPatsObject(), 0) == NULL || work->status_0x624C != 0) {
                work->step_0x018++;
                return;
            }
            break;
        case 8:
            work->step_0x018++;
            return;
        case 9:
            work->status_0x174 = 0;
            resetMediatorFlag1D(getInstance());
            work->step_0x018++;
            return;
        case 10:
            if (work->status_0x174 != 0) {
                work->step_0x018++;
                return;
            }
            break;
        case 11:
            work->step_0x018++;
            return;
        case 12:
            pmic_disp_off();
            work->flag_0xC499 = 0;
            if (work->flag_0xBF2E != 0) {
                startTermsCheck(getInstance());
                work->sub_state_0x017 = 0x65;
                work->step_0x018 = 0;
                return;
            }
            if (work->flag_0xBF2D != 0) {
                startTermsCheck(getInstance());
                work->sub_state_0x017 = 0x64;
                work->step_0x018 = 0;
                work->flag_0xBF2D = 2;
                return;
            }
            work->sub_state_0x017 = 0x62;
            work->step_0x018 = 0;
            return;
        }
        break;
    case 0x62:
        if (getNetworkCommunityPat(getPatsObject(), 0) != NULL) {
            deleteNetworkCommunityPat(getPatsObject(), 0);
        }
        if (getNetworkSessionManagerPat(getPatsObject(), 0) != NULL) {
            deleteNetworkSessionManagerPat(getPatsObject(), 0);
        }
        if (getNetworkLayerPat(getPatsObject(), 0) != NULL) {
            deleteNetworkLayerPat(getPatsObject(), 0);
        }
        loadPatInterfaceBuffers();
        clearNetworkPat(getPatsObject());
        resetPatInterfaces();
        work->reset_0x000 = 0;
        work->flag_0xBF2C = 2;
        system_w.field_0x86b = 0;
        resetSystemState(&system_w);
        work->sub_state_0x017 = 0x63;
        work->step_0x018 = 0;
        return;
    case 0x64:
        if (lobby_w.field_0x0B0 == 2 || lobby_w.field_0x0B0 == 4) {
            work->sub_state_0x017 = 0x62;
            work->step_0x018 = 0;
            return;
        }
        break;
    case 0x65:
        if (work->flag_0xBF2E == 7) {
            work->sub_state_0x017 = 0x62;
            work->step_0x018 = 0;
        }
        break;
    }
}

/*
 * Focus the network-control work when it is idle.
 */
void NetCtrlWk::updateIfActive(void)
{
    if (net_ctrl_wk->state_0x011 != 0) {
        updateNetworkPatControl();
    }
}

/*
 * Whether the control is in one of its two "server select" sub-states.
 */
BOOL NetCtrlWk::isServerSelectSubState(void)
{
    u8 state = net_ctrl_wk->sub_state_0x017;

    if (state == 0x1F || (u8)(state + 0xE0) <= 1) {
        return TRUE;
    }
    return FALSE;
}

/*
 * The currently selected server id.
 */
u8 NetCtrlWk::getSelectedServer(void)
{
    return net_ctrl_wk->selected_server_0x06A;
}

/*
 * How many of the four server slots are occupied.
 */
s8 countOccupiedServerSlots(void)
{
    NetCtrlWk* work = net_ctrl_wk;
    s32 count;
    s32 i;

    if (work == NULL) {
        return 0;
    }
    count = 0;
    for (i = 0; i < 4; i++) {
        if (work->server_slot_state_0x040[i] == 2) {
            count++;
        }
    }
    return (s8)count;
}

/*
 * The network session manager's readiness probe.
 */
BOOL NetCtrlWk::isSessionManagerReady(void)
{
    return isNetworkSessionManagerPatReady(getNetworkSessionManagerPat(getPatsObject(), 0));
}
/*
 * The lowest occupied server index, or 0 when none is.
 */
u8 NetCtrlWk::getLowestServerIndex(void)
{
    NetCtrlWk* work = net_ctrl_wk;
    u8 best = 0xFF;
    s8 value;
    s32 i;

    if (work == NULL) {
        return 0;
    }
    if (!NetCtrlWk::isSessionManagerReady()) {
        return 0;
    }
    if (work->state_0x011 == 7) {
        for (i = 0; i < 4; i++) {
            value = work->server_index_0x074[i];
            if (value != -1 && best > value) {
                best = value;
            }
        }
    }
    if (best == 0xFF) {
        return 0;
    }
    return best;
}

/*
 * Tell the session manager that a server row was entered.
 */
/* untyped: byte range - the command record the session manager sends to the players */
void broadcastSessionCommand(void* cmd, u32 size)
{
    NetCtrlWk* work = net_ctrl_wk;

    if (NetCtrlWk::isSessionManagerReady() != 0 && work->state_0x011 == 7) {
        NetworkSessionManagerPat* manager = getNetworkSessionManagerPat(getPatsObject(), 0);

        manager->broadcastPlayerSlots((u32)cmd, size);
    }
}

/*
 * Tell the session manager that the server list was left.
 */
void NetCtrlWk::flushSession(void)
{
    NetCtrlWk* work = net_ctrl_wk;

    if (isSessionManagerReady() != 0 && work->state_0x011 == 7) {
        NetworkSessionManagerPat* manager = getNetworkSessionManagerPat(getPatsObject(), 0);

        manager->flush();
    }
}

/*
 * Whether the low byte of the server rows is the given row.
 */
BOOL NetCtrlWk::hasServerIndex(u32 server_index)
{
    NetCtrlWk* work = net_ctrl_wk;

    if (work == NULL) {
        return FALSE;
    }
    if (!isSessionManagerReady()) {
        return FALSE;
    }
    if (work->state_0x011 == 7) {
        if ((u8)server_index == work->server_index_0x074[0]) {
            return TRUE;
        }
        if ((u8)server_index == work->server_index_0x074[1]) {
            return TRUE;
        }
        if ((u8)server_index == work->server_index_0x074[2]) {
            return TRUE;
        }
        if ((u8)server_index == work->server_index_0x074[3]) {
            return TRUE;
        }
    }
    return FALSE;
}

/*
 * Whether the server slot at the given index is occupied.
 */
s32 isServerSlotOccupied(u8 index)
{
    NetCtrlWk* work = net_ctrl_wk;

    if (work == NULL) {
        return FALSE;
    }
    return work->server_slot_state_0x040[index] == 2;
}

/*
 * Whether the control is in its server-select state.
 */
u32 isServerSelectState(void)
{
    return net_ctrl_wk->state_0x011 == 7;
}

/*
 * How many of the four server rows carry a valid index.
 */
u8 NetCtrlWk::countValidServerIndexes(void)
{
    NetCtrlWk* work = net_ctrl_wk;
    s32 count = 0;
    s32 i;

    if (work->state_0x011 == 7) {
        for (i = 0; i < 4; i++) {
            if (work->server_index_0x074[i] != -1) {
                count++;
            }
        }
    }
    return (u8)count;
}

/*
 * Whether the ready counter is at one.
 */
u32 isReadyCountOne(void)
{
    return net_ctrl_wk->ready_count_0x044 == 1;
}

/*
 * Whether the control is in city mode (its mode byte is 3).
 */
s32 isCityMode(void)
{
    return net_ctrl_wk->mode_0x010 == 3;
}

/*
 * Whether the control is in city mode, as a member (a second retail copy of the same test).
 */
BOOL NetCtrlWk::isCityModeAlias(void)
{
    return net_ctrl_wk->mode_0x010 == 3;
}

/*
 * Reserve the first free entry of the server/message table.
 */
NetCtrlEntry* NetCtrlWk::reserveEntry(void)
{
    NetCtrlWk* work = net_ctrl_wk;

    for (u32 i = 0; i < 16; i++) {
        if (work->entries_0x7CD8[i].in_use_0x00 == 0) {
            work->entries_0x7CD8[i].in_use_0x00 = 1;
            return &work->entries_0x7CD8[i];
        }
    }
    return NULL;
}

/*
 * Whether the per-session flag is set.
 */
BOOL NetCtrlWk::isSessionFlagSet(void)
{
    return net_ctrl_wk->flag_0xC3F2 == 1;
}

/*
 * Clear the per-session flag.
 */
void NetCtrlWk::clearSessionFlag(void)
{
    net_ctrl_wk->flag_0xC3F2 = 0;
}

/*
 * The length of the character (1-3 bytes) starting at the given shift-JIS byte, or 0 when invalid.
 */
s32 getUtf8CharLength(u8* bytes)
{
    s32 first = bytes[0];

    if (first <= 0x7F) {
        return 1;
    }
    if ((u32)(first - 0xC0) <= 0x1F) {
        s32 second = bytes[1];

        if (second == 0 || (u32)(second - 0x80) > 0x3F) {
            return 0;
        }
        return 2;
    }
    if ((u32)(first - 0xE0) <= 0x0F) {
        s32 second = bytes[1];

        if (second == 0) {
            return 0;
        }
        if ((u32)(second - 0x80) > 0x3F) {
            return 0;
        }
        s32 fourth = bytes[3];

        if (fourth == 0) {
            return 0;
        }
        if ((u32)(fourth - 0x80) > 0x3F) {
            return 0;
        }
        return 3;
    }
    return 0;
}

/*
 * The printed width of a shift-JIS string: its character count less its space characters.
 */
s32 getPrintedWidth(char* text)
{
    char buffer[0x4C];
    char* cursor = buffer;

    buffer[0] = 0;
    strcpy(buffer, text);
    s32 length = flfntStrLen(buffer);
    s32 spaces = 0;

    for (;;) {
        s32 step = getUtf8CharLength((u8*)cursor);

        if (step == 0) {
            break;
        }
        if (step == 1 && *cursor == 0x20) {
            spaces++;
        }
        cursor += step;
    }
    return length - spaces;
}

/*
 * Post a named server/message entry.
 */
BOOL NetCtrlWk::postEntry(u8 kind, char* name, u32 value, u8 flags)
{
    if (net_ctrl_wk == NULL) {
        return FALSE;
    }
    if (name == NULL) {
        return FALSE;
    }
    if (!getPrintedWidth(name)) {
        sysSE_req(2);
        return FALSE;
    }
    NetCtrlEntry* entry = reserveEntry();

    if (entry == NULL) {
        return FALSE;
    }
    strcpy(entry->name_0x04, name);
    entry->flags_0x03 = flags;
    entry->value_0x4C = value;
    entry->kind_0x02 = kind;
    memset(entry->text_0x50, 0, 0xA);
    return TRUE;
}

/*
 * Post a named server/message entry with a second line of text.
 */
BOOL NetCtrlWk::postEntryWithText(u8 kind, char* name, u32 value, u8 flags, char* text)
{
    if (net_ctrl_wk == NULL) {
        return FALSE;
    }
    if (name == NULL) {
        return FALSE;
    }
    if (!getPrintedWidth(name)) {
        sysSE_req(2);
        return FALSE;
    }
    NetCtrlEntry* entry = reserveEntry();

    if (entry == NULL) {
        return FALSE;
    }
    strcpy(entry->name_0x04, name);
    entry->flags_0x03 = flags;
    entry->value_0x4C = value;
    entry->kind_0x02 = kind;
    memset(entry->text_0x50, 0, 0xA);
    if (text != NULL) {
        strcpy(entry->text_0x50, text);
    }
    return TRUE;
}

/*
 * Restart the network session after a disconnect: reset the counters and flags and re-enter the login state.
 */
BOOL NetCtrlWk::restartSession(void)
{
    NetCtrlWk* work = net_ctrl_wk;

    if (getLinkStatus() != 1 && system_w.net_result_wait_0x8c1 != 1 && work->sub_state_0x017 != 0x22) {
        return FALSE;
    }
    work->sub_state_0x017 = 0x19;
    work->mode_0x010 = 3;
    work->counter_0xC362 = 0;
    work->counter_0xC360 = 0;
    work->counter_0xC364 = 0;
    work->flag_0x82C4 = 0;
    work->flag_0x82C5 = 0;
    work->flag_0x82C7 = 0;
    resetLinkState();
    resetNetSlots(work);
    syncScheduleClock(work);
    lobby_w.field_0x176 = 2;
    return TRUE;
}

/*
 * Whether the server-request state is at two.
 */
BOOL NetCtrlWk::isRequestStateTwo(void)
{
    return net_ctrl_wk->flag_0xBF2C == 2;
}

/*
 * Advise the server-request state.
 */
void NetCtrlWk::setRequestStateOne(void)
{
    net_ctrl_wk->flag_0xBF2C = 1;
}

/*
 * Copy the peer whose id matches the profile's id slot into the player card.
 */
BOOL NetCtrlWk::loadPeerCard(NetPlayerCard* card, u8 slot)
{
    NetCtrlWk* work = net_ctrl_wk;
    NetProfileRec* profile = &work->profiles_0x168[work->profile_index_0x170];
    NetId* slot_id;
    NetPeerRec* peer;
    s32 i;

    if (profile == NULL) {
        return FALSE;
    }
    slot_id = &profile->ids_0x0B0[slot].id_0x00;
    peer = work->peers_0x7488;
    for (i = 0; i < 4; i++, peer++) {
        if (isSameNetId(slot_id, &peer->id_0x00) == 1) {
            decodePlayerCard((const u8*)&peer->blob_0x20, card);
            strcpy(card->name_0x5CA, peer->name_0x0A.text_0x00);
            strcpy(card->id_0x5DB, peer->id_0x00.text_0x00);
            return TRUE;
        }
    }
    return FALSE;
}

/*
 * The name field of the peer whose id matches, or NULL.
 */
char* NetCtrlWk::findPeerName(NetId* id)
{
    NetCtrlWk* work = net_ctrl_wk;
    NetPeerRec* peer = work->peers_0x7488;
    s32 i;

    for (i = 0; i < 4; i++, peer++) {
        if (isSameNetId(id, &peer->id_0x00) == 1) {
            return peer->name_0x0A.text_0x00;
        }
    }
    return NULL;
}

/*
 * Whether all three connection flags reached state two, or an error code is pending.
 */
BOOL NetCtrlWk::isConnectionSettled(void)
{
    NetCtrlWk* work = net_ctrl_wk;

    if (work->flag_0x82C5 == 2 && work->flag_0x82C7 == 2 && work->flag_0x82C4 == 2) {
        return TRUE;
    }
    if (work->sub_error_0xC259 != 0) {
        return TRUE;
    }
    return work->error_code_0xC258 != 0;
}

/*
 * Whether the server-info download is under way or done, starting the fetch when idle.
 */
BOOL NetCtrlWk::pollBigDataFetch(void)
{
    NetCtrlWk* work = net_ctrl_wk;
    s32 result;

    if (work->timeout_0xC36C > 0) {
        return TRUE;
    }
    if (work->sub_error_0xC259 != 0) {
        return TRUE;
    }
    if (work->error_code_0xC258 != 0) {
        return TRUE;
    }
    result = work->stepFetch(3);
    if (result == 0) {
        return FALSE;
    }
    if (result == 1) {
        updateLobbyEventFlags();
    }
    work->timeout_0xC36C = get_server_big_data_timeout_element(0);
    return TRUE;
}

/*
 * Whether an error is pending or the schedule fetch made progress.
 */
BOOL NetCtrlWk::pollNoticeFetch(void)
{
    NetCtrlWk* work = net_ctrl_wk;

    if (work->sub_error_0xC259 != 0) {
        return TRUE;
    }
    if (work->error_code_0xC258 != 0) {
        return TRUE;
    }
    return work->stepFetch(4) != 0;
}

/*
 * Raise the per-session request flag.
 */
void NetCtrlWk::raiseRequestFlag(void)
{
    if (net_ctrl_wk != NULL) {
        net_ctrl_wk->flag_0xC1BF = 1;
    }
}

/*
 * Whether `value` passes filter `index` of the first filter table.
 */
BOOL NetCtrlWk::passesFirstFilter(s32 index, u16 value)
{
    NetFilterRec* filter;

    if (net_ctrl_wk == NULL) {
        return FALSE;
    }
    filter = &net_ctrl_wk->filters_0x8C64[index];
    if (filter->kind_0x1F != 1) {
        return TRUE;
    }
    if (filter->low_0x20 <= value && filter->high_0x22 >= value) {
        return TRUE;
    }
    return FALSE;
}

/*
 * Whether `value` lies inside the id range of server `index`.
 */
BOOL can_enter_server(s32 index, u32 value)
{
    NetCtrlWk* work = net_ctrl_wk;
    NetRange* range;
    u16 high;
    u16 low;

    if (work == NULL) {
        return FALSE;
    }
    range = &work->ranges_0x9684[index];
    high = range->high_0x02;
    low = range->low_0x00;
    if (low <= (u16)value && high >= (u16)value) {
        return TRUE;
    }
    return FALSE;
}

/*
 * Step the fetch of server file `kind` (1..4): create the client, open the numbered file, poll the
 * transfer, read it into its buffer and close; returns 1 when done, -1 on error and 0 while running.
 */
s32 NetCtrlWk::stepFetch(s32 kind)
{
    u32 status;
    char path[16];
    void* buffer;
    s32 size;
    u32* checksum;
    NetworkFileFetcher* fetcher;
    s32 file_number;

    switch (kind) {
    case 1:
        buffer = server_types_0x82D8;
        checksum = &fetch_sums_0xA0E4[0];
        size = 5132;
        break;
    case 2:
        buffer = &times_0x96E4;
        checksum = &fetch_sums_0xA0E4[1];
        size = 12;
        break;
    case 3:
        buffer = &bigdata_0x96F0;
        checksum = &fetch_sums_0xA0E4[2];
        size = 2308;
        break;
    case 4:
        buffer = &notice_0x9FF4;
        checksum = &fetch_sums_0xA0E4[3];
        size = 240;
        break;
    default:
        return -1;
    }
    fetcher = fetcher_0x82CC;
    file_number = kind + group_0xC494 * 15;
    switch (fetch_step_0x82D0) {
    case 0:
        fetcher = new NetworkFileFetcher;
        fetcher_0x82CC = fetcher;
        if (fetcher == NULL) {
            fetch_error_0xA0F4.code_0x0 = 0x80000001;
            fetch_error_0xA0F4.detail_0x4 = 0;
            fetch_error_0xA0F4.reason_0x8 = 0x80000000;
            return -1;
        }
        snprintf(path, 16, "%d", file_number);
        if (fetcher->NetworkFileFetcher::open(0, path) < 0) {
            fetch_step_0x82D0 = 10;
        } else {
            fetch_step_0x82D0++;
        }
        break;
    case 1: {
        s32 result = fetcher->poll(&status);

        if (result == 0) {
            if (*checksum == status) {
                fetch_step_0x82D0 = 8;
            } else {
                *checksum = status;
                fetch_step_0x82D0++;
            }
        } else if (result < 0) {
            fetch_step_0x82D0 = 10;
        }
        break;
    }
    case 2:
        snprintf(path, 16, "%d", file_number);
        if (fetcher->open(0, path) < 0) {
            fetch_step_0x82D0 = 10;
        } else {
            fetch_step_0x82D0++;
        }
        break;
    case 3: {
        s32 result = fetcher->poll(&status);

        if (result == 0) {
            fetch_step_0x82D0++;
        } else if (result < 0) {
            fetch_step_0x82D0 = 10;
        }
        break;
    }
    case 4:
        if (fetcher->read(buffer, size) < 0) {
            fetch_step_0x82D0 = 10;
        } else {
            fetch_step_0x82D0++;
        }
        break;
    case 5: {
        s32 result = fetcher->poll(&status);

        if (result == 0) {
            fetch_step_0x82D0++;
        } else if (result < 0) {
            fetch_step_0x82D0 = 10;
        }
        break;
    }
    case 6:
        if (fetcher->close() < 0) {
            fetch_step_0x82D0 = 10;
        } else {
            fetch_step_0x82D0++;
        }
        break;
    case 7: {
        s32 result = fetcher->poll(&status);

        if (result == 0) {
            fetch_step_0x82D0++;
        } else if (result < 0) {
            fetch_step_0x82D0 = 10;
        }
        break;
    }
    case 8:
        fetch_step_0x82D0 = 0;
        fetch_index_0x82D4 = 0;
        delete fetcher;
        fetcher_0x82CC = NULL;
        return 1;
    case 10:
        *checksum = 0;
        fetch_step_0x82D0 = 0;
        fetch_index_0x82D4 = 0;
        fetcher->copyError(&fetch_error_0xA0F4);
        delete fetcher_0x82CC;
        fetcher_0x82CC = NULL;
        return -1;
    }
    return 0;
}

/*
 * The network singleton's game time, or 0 when it is not up.
 */
u32 NetCtrlWk::getNetworkTime(void)
{
    if (getInstance_() == NULL) {
        return 0;
    }
    return getGameTime(getInstance_());
}

/*
 * How far the game time has progressed through the current schedule period (0..1).
 */
f32 NetSchedule::getPhase(void)
{
    u32 now;

    if (getInstance_() == NULL) {
        return 0.0f;
    }
    now = getGameTime(getInstance_());
    if (period_0x04 == 0) {
        period_0x04 = 3000;
    }
    return (f32)((now - base_0x00) % period_0x04) / (f32)period_0x04;
}

/*
 * Which part of the event window the game time is in: 1 before the warning point, 2 before the end,
 * 0 outside the window.
 */
s32 NetRaidTimes::classifyNow(void)
{
    u32 now;

    if (getInstance_() == NULL) {
        return 0;
    }
    now = getGameTime(getInstance_());
    if (start_0x00 < now) {
        if (now < warning_0x04) {
            return 1;
        }
        if (now < end_0x08) {
            return 2;
        }
    }
    return 0;
}

/*
 * Copy the id, name and description of server type `type` (1-based) into `info`.
 */
BOOL NetCtrlWk::getServerTypeInfo(s32 type, NetServerTypeInfo* info)
{
    NetCtrlWk* work = net_ctrl_wk;
    s32 index = type - 1;

    if (index < 0) {
        return FALSE;
    }
    info->id_0x00 = work->type_ids_0xA100[index];
    strcpy(info->name_0x02, get_server_type_name(index));
    strcpy(info->description_0x12, get_server_type_desc(index));
    return TRUE;
}

/*
 * Copy the servers of `type` into `views`, skipping the first `skip` and stopping after `max`; returns the count.
 */
s32 NetCtrlWk::listServers(s32 type, NetServerView* views, s32 skip, s32 max)
{
    NetCtrlWk* work = net_ctrl_wk;
    NetSrvRec* server;
    s32 skipped;
    s32 count;
    u32 i;

    if ((u32)(type - 1) > 3) {
        return 0;
    }
    skipped = 0;
    count = 0;
    server = work->servers_0x668;
    for (i = 0; i < work->server_count_0x660; i++, server++) {
        if (server->type_0x3C != type) {
            continue;
        }
        if (skipped < skip) {
            skipped++;
            continue;
        }
        views->population_0x00 = server->population_0x28;
        views->capacity_0x04 = server->capacity_0x2C;
        views->kind_0x08 = 0x28;
        views->id_0x0A = server->id_0x00;
        strcpy(views->name_0x0C, server->name_0x04);
        count++;
        if (count >= max) {
            break;
        }
        views++;
    }
    return count;
}

/*
 * The number of pages of `per_page` rooms the room list needs.
 */
s32 NetCtrlWk::getRoomPageCount(s32 per_page)
{
    NetworkLayerPat* layer = getNetworkLayerPat(getPatsObject(), 0);
    s32 count = layer->rooms_18A4.count_0x000;
    s32 pages;

    if (count == 0) {
        return 0;
    }
    pages = count / per_page;
    if (count % per_page != 0) {
        pages++;
    }
    return pages;
}

/*
 * Copy up to `max` rooms starting at `first` into `views`; returns how many were copied.
 */
s32 NetCtrlWk::copyRoomViews(NetListView* views, u32 first, s32 max)
{
    NetRoomList* list = &getNetworkLayerPat(getPatsObject(), 0)->rooms_18A4;
    u32 count = list->count_0x000;
    NetRoomRec* room;
    u32 end;
    s32 copied;

    if (first >= count) {
        return 0;
    }
    end = first + max;
    if (end > count) {
        end = count;
    }
    copied = 0;
    room = &list->entries_0x004[first];
    for (; (s32)first < (s32)end; first++, room++, views++) {
        views->population_0x00 = room->population_0x88;
        views->capacity_0x04 = room->capacity_0x8C;
        views->id_0x08 = room->id_0x40;
        strcpy(views->name_0x0A, room->name_0x44);
        copied++;
    }
    return copied;
}

/*
 * Copy up to `max` cities starting at `first` into `views`; returns how many were copied.
 */
s32 NetCtrlWk::copyCityListViews(NetListView* views, u32 first, s32 max)
{
    NetCityList* list = &getNetworkLayerPat(getPatsObject(), 0)->cities_540;
    u32 count = list->count_0x000;
    u32 end;
    s32 copied;
    NetCityRec* city;

    if (first >= count) {
        return 0;
    }
    end = first + max;
    if (end > count) {
        end = count;
    }
    copied = 0;
    city = &list->entries_0x004[first];
    for (; (s32)first < (s32)end; first++, city++, views++) {
        views->population_0x00 = city->population_0x48;
        views->capacity_0x04 = city->capacity_0x4C;
        views->id_0x08 = city->id_0x00;
        strcpy(views->name_0x0A, city->name_0x04);
        copied++;
    }
    return copied;
}

/*
 * The number of pages of `per_page` cities the city list needs.
 */
s32 NetCtrlWk::getCityPageCount(s32 per_page)
{
    NetworkLayerPat* layer = getNetworkLayerPat(getPatsObject(), 0);
    s32 count = layer->cities_540.count_0x000;
    s32 pages;

    if (count == 0) {
        return 0;
    }
    pages = count / per_page;
    if (count % per_page != 0) {
        pages++;
    }
    return pages;
}

/*
 * The number of cities in the layer's city list.
 */
s32 NetCtrlWk::getCityCount(void)
{
    return getNetworkLayerPat(getPatsObject(), 0)->cities_540.count_0x000;
}

/*
 * Copy up to `max` city rows starting at `first` into `views` with their open/closed/busy/full state.
 */
s32 NetCtrlWk::copyCityViews(NetCityView* views, u32 first, s32 max, u8 keep_counts)
{
    s32 copied = 0;
    NetCityList* list = &getNetworkLayerPat(getPatsObject(), 0)->cities_540;
    u32 count = list->count_0x000;
    NetCityRec* city;
    u32 end;

    if (first >= count) {
        return 0;
    }
    end = first + max;
    if (end > count) {
        end = count;
    }
    city = &list->entries_0x004[first];
    for (; (s32)first < (s32)end; first++, city++, views++, copied++) {
        s32 i;

        if (keep_counts == 0) {
            views->population_0x00 = city->population_0x48;
            views->capacity_0x04 = city->capacity_0x4C;
        }
        views->id_0x08 = city->id_0x00;
        views->order_0x0A = city->order_0x50;
        views->state_0x0C = 0;
        if (city->closed_0x78 == 1) {
            views->state_0x0C = 1;
        } else {
            views->state_0x0C = 2;
            if (city->hidden_0x79 == 1) {
                views->state_0x0C = 0;
            } else {
                views->state_0x0C = 2;
                if (city->population_0x48 >= city->capacity_0x4C) {
                    views->state_0x0C = 3;
                }
            }
        }
        strcpy(views->name_0x10, city->name_0x04);
        for (i = 0; i < 4; i++) {
            views->values_0x20[i] = 0;
            if (city->values_0x58[i].value_0x04 != -1) {
                views->values_0x20[i] = city->values_0x58[i].value_0x04;
            }
        }
    }
    return copied;
}

/*
 * Raise the second per-session request flag.
 */
void NetCtrlWk::raiseSecondRequestFlag(void)
{
    if (net_ctrl_wk != NULL) {
        net_ctrl_wk->flag_0xC3F0 = 1;
    }
}

/*
 * Queue command 3 (list refresh) and clear the caller's result byte.
 */
void NetCtrlWk::requestListRefresh(s8* result)
{
    *result = 0;
    queueNetCommand(3, result, 0, 0, NULL);
}

/*
 * The list-refresh request again (a second retail copy that other callers reach).
 */
void NetCtrlWk::requestListRefreshAlias(s8* result)
{
    *result = 0;
    queueNetCommand(3, result, 0, 0, NULL);
}

/*
 * Queue the request to enter city `index` (command 4, or 0x10 for a hidden city), or set the error
 * that explains why the city cannot be entered.
 */
s32 NetCtrlWk::requestEnterCity(s8* result, s32 index, u8 forced)
{
    NetCtrlWk* work = net_ctrl_wk;
    NetCityRec* city;

    if (work != NULL) {
        work->flag_0x82C4 = 0;
    }
    *result = 0;
    city = &getNetworkLayerPat(getPatsObject(), 0)->cities_540.entries_0x004[index];
    if (city->hidden_0x79 == 0) {
        if (city->population_0x48 == 0) {
            work->sub_error_0xC259 = 9;
            return 0;
        }
        if (city->population_0x48 == city->capacity_0x4C) {
            work->sub_error_0xC259 = 8;
            return 0;
        }
        if (city->kind_0x54 == 4) {
            work->settings_0x6210[0] = city->values_0x58[0].value_0x04;
            work->settings_0x6210[1] = city->values_0x58[1].value_0x04;
            work->settings_0x6210[2] = city->values_0x58[2].value_0x04;
            work->settings_0x6210[3] = city->values_0x58[3].value_0x04;
        }
        return queueNetCommand(4, result, 0, 1, &index);
    }
    if (forced == 1) {
        work->sub_error_0xC259 = 9;
        return 0;
    }
    return queueNetCommand(0x10, result, 0, 1, &index);
}

/*
 * Report 1 at once when flag 0x69 is set, else queue command 8.
 */
s32 NetCtrlWk::requestCommand08(s8* result)
{
    NetCtrlWk* work = net_ctrl_wk;

    *result = 0;
    if (work->flag_0x069 == 0) {
        return queueNetCommand(8, result, 0, 0, NULL);
    }
    *result = 1;
    return 1;
}

/*
 * Report 1 at once when flag 0x68 is set, else queue command 7.
 */
s32 NetCtrlWk::requestCommand07(s8* result)
{
    NetCtrlWk* work = net_ctrl_wk;

    *result = 0;
    if (work->flag_0x068 == 0) {
        return queueNetCommand(7, result, 0, 0, NULL);
    }
    *result = 1;
    return 1;
}

/*
 * Queue command 0xE with one argument.
 */
void NetCtrlWk::requestCommand0E(s8* result, s32 value)
{
    *result = 0;
    queueNetCommand(0xE, result, 0, 1, &value);
}

/*
 * Queue command 0xF with one argument.
 */
void NetCtrlWk::requestCommand0F(s8* result, s32 value)
{
    *result = 0;
    queueNetCommand(0xF, result, 0, 1, &value);
}

/*
 * Queue command 0x17 with one argument.
 */
void NetCtrlWk::requestCommand17(s8* result, s32 value)
{
    *result = 0;
    queueNetCommand(0x17, result, 0, 1, &value);
}

/*
 * Queue command 0x18 with the caller's argument list.
 */
void NetCtrlWk::requestCommand18(const s32* args, s8* result)
{
    *result = 0;
    queueNetCommand(0x18, result, 0, 1, args);
}

/*
 * Submit the four settings to the layer, remember them and queue command 0x11.
 */
void NetCtrlWk::requestSettingsCommand(s8* result, s32 value, const NetSettingsWords* settings)
{
    NetCtrlWk* work = net_ctrl_wk;
    NetLayerSettings request;

    memset(&request, 0, sizeof(request));
    request.count_0x00 = 4;
    request.pairs_0x04[0].enabled_0x0 = 1;
    request.pairs_0x04[0].value_0x4 = settings->word_0x00;
    request.pairs_0x04[1].enabled_0x0 = 1;
    request.pairs_0x04[1].value_0x4 = settings->word_0x04;
    request.pairs_0x04[2].enabled_0x0 = 1;
    request.pairs_0x04[2].value_0x4 = settings->word_0x08;
    request.pairs_0x04[3].enabled_0x0 = 1;
    request.pairs_0x04[3].value_0x4 = settings->word_0x0C;
    getNetworkLayerPat(getPatsObject(), 0)->submitSettings_98(&request);
    work->settings_0x6210[0] = settings->word_0x00;
    work->settings_0x6210[1] = settings->word_0x04;
    work->settings_0x6210[2] = settings->word_0x08;
    work->settings_0x6210[3] = settings->word_0x0C;
    queueNetCommand(0x11, result, 0, 1, &value);
}

/*
 * Queue command 0x12 with one argument.
 */
void NetCtrlWk::requestCommand12(s8* result, s32 value)
{
    queueNetCommand(0x12, result, 0, 1, &value);
}

/*
 * Copy the connected-peer list out for the caller and return its count.
 */
s32 NetCtrlWk::copyPeerList(NetPeerList* list)
{
    NetCtrlWk* work = net_ctrl_wk;
    NetPeerRec* dst;
    NetPeerRec* src;
    s32 i;

    list->count_0x000 = work->peer_count_0x62E8;
    dst = list->peers_0x004;
    src = work->peers_0x7488;
    for (i = 0; i < 4; i++) {
        (dst++)->assign(src++);
    }
    return work->peer_count_0x62E8;
}

/*
 * Copy one connected-peer record.
 */
void NetPeerRec::assign(const NetPeerRec* src)
{
    id_0x00 = src->id_0x00;
    name_0x0A = src->name_0x0A;
    blob_0x20 = src->blob_0x20;
}

/*
 * The name of server type `index`, or a blank when out of range.
 */
char* get_server_type_name(s32 index)
{
    NetServerType* types = net_ctrl_wk->server_types_0x82D8;

    if (index >= 0 && index < 4) {
        return types[index].name_0x00;
    }
    return " ";
}

/*
 * The description of server type `index`.
 */
char* get_server_type_desc(s32 index)
{
    return net_ctrl_wk->server_types_0x82D8[index].description_0x18;
}

/*
 * Collect the events of `category` that are enabled, as their indices, up to `max`; returns the count.
 */
s32 NetCtrlWk::collectEvents(s8* out, u8 category, s32 max)
{
    NetCtrlWk* work = net_ctrl_wk;
    NetEventRec* event;
    s32 count;
    s32 index;
    s32 bit;
    s32 i;

    if (work == NULL) {
        return 0;
    }
    event = work->events_0x85E4;
    count = 0;
    index = 0;
    bit = 1 << category;
    for (i = 0; i < 32; i++, index++, event++) {
        if (event->enabled_0x31 != 0 && (event->mask_0x32 & bit) != 0) {
            *out++ = (s8)index;
            count++;
            if (max <= count) {
                return count;
            }
        }
    }
    return count;
}

/*
 * Collect the enabled filters of the second table that accept `value`, as their indices, up to `max`.
 */
s32 NetCtrlWk::collectSecondFilters(s8* out, u16 value, const u16* table_a, const u16* table_b, s32 table_count, s32 max)
{
    NetCtrlWk* work = net_ctrl_wk;
    NetFilterRec* filter;
    s32 count;
    s32 index;
    s32 i;

    if (work == NULL) {
        return 0;
    }
    filter = work->filters_0x8D84;
    count = 0;
    index = 0;
    for (i = 0; i < 64; i++, index++, filter++) {
        if (filter->enabled_0x1E != 0) {
            s32 match = 0;

            switch (filter->kind_0x1F) {
            case 0:
                match = 1;
                break;
            case 1:
                if (value >= filter->low_0x20 && value <= filter->high_0x22) {
                    match = 1;
                }
                break;
            case 2:
                if (filter->low_0x20 < table_count) {
                    s32 offset = filter->low_0x20 * 2;

                    if (*(const u16*)((const u8*)table_a + offset) != 0) {
                        match = 1;
                    }
                    if (*(const u16*)((const u8*)table_b + offset) != 0) {
                        match = 1;
                    }
                }
                break;
            }
            if (match == 1) {
                *out++ = (s8)index;
                count++;
                if (max <= count) {
                    return count;
                }
            }
        }
    }
    return count;
}

/*
 * Collect the enabled filters of the first table whose id range contains `value`, up to `max`.
 */
s32 NetCtrlWk::collectFirstFilters(s8* out, u16 value, s32 max)
{
    NetCtrlWk* work = net_ctrl_wk;
    NetFilterRec* filter;
    s32 count;
    s8 index;
    s32 i;

    if (work == NULL) {
        return 0;
    }
    filter = work->filters_0x8C64;
    count = 0;
    index = 0;
    for (i = 0; i < 8; i++, index++, filter++) {
        if (filter->enabled_0x1E != 0 && value >= filter->low_0x20 && value <= filter->high_0x22) {
            *out++ = index;
            count++;
            if (max <= count) {
                return count;
            }
        }
    }
    return count;
}

/*
 * Event entry `index`.
 */
NetEventRec* NetCtrlWk::getEvent(s32 index)
{
    return &net_ctrl_wk->events_0x85E4[index];
}

/*
 * Filter `index` of the first filter table.
 */
NetFilterRec* NetCtrlWk::getFirstFilter(s32 index)
{
    return &net_ctrl_wk->filters_0x8C64[index];
}

/*
 * Filter `index` of the second filter table.
 */
NetFilterRec* NetCtrlWk::getSecondFilter(s32 index)
{
    return &net_ctrl_wk->filters_0x8D84[index];
}

/*
 * Whether invite `id` (1-based) carries this player's own name: 1 when it does, 0 with a request queued
 * when its slot is unfilled, and -1 when it names someone else.
 */
s32 NetCtrlWk::checkOwnInvite(u8 id, s8* result)
{
    NetCtrlWk* work = net_ctrl_wk;
    NetInviteRec* invite;
    s32 arg;

    if (work == NULL) {
        *result = -1;
        return 0;
    }
    if (work->link_state_0xC0BC != 1) {
        *result = -1;
        return 0;
    }
    invite = &work->invites_0xBF3C[id - 1];
    if (invite->valid_0x0B == 1) {
        if (strcmp(work->name_0x7368, invite->name_0x00) == 0) {
            *result = 1;
            return 1;
        }
        *result = -1;
        return 0;
    }
    *result = 0;
    arg = id;
    return queueNetCommand(0x14, result, 0, 1, &arg);
}

/*
 * Whether invite `id` (1-based) is free or names someone else; queues a request otherwise.
 */
s32 checkOtherInvite(u8 id, s8* result)
{
    NetCtrlWk* work = net_ctrl_wk;
    NetInviteRec* invite;
    s32 arg;

    if (work == NULL) {
        *result = -1;
        return 0;
    }
    if (work->link_state_0xC0BC != 1) {
        *result = -1;
        return 0;
    }
    invite = &work->invites_0xBF3C[id - 1];
    if (invite->valid_0x0B == 0) {
        *result = 1;
        return 1;
    }
    if (strcmp(work->name_0x7368, invite->name_0x00) != 0) {
        *result = 1;
        return 1;
    }
    *result = 0;
    arg = id;
    return queueNetCommand(0x15, result, 0, 1, &arg);
}

/*
 * Whether the account link flag is set.
 */
BOOL NetCtrlWk::isAccountLinked(void)
{
    if (net_ctrl_wk == NULL) {
        return FALSE;
    }
    return net_ctrl_wk->flag_0xC0BD != 0;
}

/*
 * The type name of the selected server, or a blank when none is selected.
 */
char* NetCtrlWk::getSelectedServerTypeName(void)
{
    NetCtrlWk* work = net_ctrl_wk;
    NetServerType* types = work->server_types_0x82D8;
    NetSrvRec* server;

    if (work == NULL) {
        return " ";
    }
    server = &work->servers_0x668[work->selected_server_index_0xA114];
    if ((server->flags_0x38 | server->type_0x3C) == 0) {
        return " ";
    }
    return types[(s32)((s64)server->type_0x3C - 1)].name_0x00;
}

/*
 * The name of the selected server, or a blank when the work record is absent.
 */
char* NetCtrlWk::getSelectedServerName(void)
{
    NetCtrlWk* work = net_ctrl_wk;

    if (work == NULL) {
        return " ";
    }
    return work->servers_0x668[work->selected_server_index_0xA114].name_0x04;
}

/*
 * The account name, or a blank when it is unset.
 */
char* NetCtrlWk::getAccountNameOrBlank(void)
{
    NetCtrlWk* work = net_ctrl_wk;

    if (work == NULL) {
        return " ";
    }
    if (work->account_name_0xC0C4[0] == 0 && work == NULL) {
        return " ";
    }
    return work->account_name_0xC0C4;
}

/*
 * The nickname, or a blank when it is unset.
 */
char* NetCtrlWk::getNicknameOrBlank(void)
{
    NetCtrlWk* work = net_ctrl_wk;

    if (work == NULL) {
        return " ";
    }
    if (work->nickname_0xC104[0] == 0) {
        return " ";
    }
    return work->nickname_0xC104;
}

/*
 * The text of big-data record `index`, or a blank when the work record is absent.
 */
char* NetCtrlWk::getBigDataText(u8 index)
{
    NetCtrlWk* work = net_ctrl_wk;

    if (work == NULL) {
        return " ";
    }
    return work->bigdata_0x96F0.records_0x000[index].text_0x000;
}

/*
 * Mirror the event bits into the lobby's four event flags (all of them while the event is running).
 */
void NetCtrlWk::updateLobbyEventFlags(void)
{
    NetCtrlWk* work = net_ctrl_wk;

    lobby_w.field_0x15C = 0;
    lobby_w.field_0x15D = 0;
    lobby_w.field_0x15E = 0;
    lobby_w.field_0x160 = 0;
    if (work != NULL) {
        if ((work->bigdata_0x96F0.event_bits_0x900 & 1) != 0) {
            lobby_w.field_0x15C = 1;
        }
        if ((work->bigdata_0x96F0.event_bits_0x900 & 2) != 0) {
            lobby_w.field_0x15D = 1;
        }
        if ((work->bigdata_0x96F0.event_bits_0x900 & 4) != 0) {
            lobby_w.field_0x15E = 1;
        }
        if ((work->bigdata_0x96F0.event_bits_0x900 & 8) != 0) {
            lobby_w.field_0x160 = 1;
        }
        if (lobby_w.field_0x15F != 0) {
            lobby_w.field_0x15C = 1;
            lobby_w.field_0x15D = 1;
            lobby_w.field_0x15E = 1;
            lobby_w.field_0x160 = 1;
        }
    }
}

/*
 * The fourth fetch's notice block, or NULL when the work record is absent.
 */
NetServerNotice* NetCtrlWk::getServerNotice(void)
{
    if (net_ctrl_wk == NULL) {
        return NULL;
    }
    return &net_ctrl_wk->notice_0x9FF4;
}

/*
 * The timeout (in frames) the big-data request `index` waits for.
 */
u16 get_server_big_data_timeout_element(s32 index)
{
    return net_ctrl_wk->timeouts_0x9694[index];
}

/*
 * The type index (0-based) of the selected server, or 0 when the work record is absent.
 */
s32 NetCtrlWk::getSelectedServerTypeIndex(void)
{
    NetCtrlWk* work = net_ctrl_wk;

    if (work == NULL) {
        return 0;
    }
    return (s32)((s64)work->servers_0x668[work->selected_server_index_0xA114].type_0x3C - 1);
}

/*
 * The lobby-ready byte, once the control has reached its online states.
 */
u8 NetCtrlWk::getLobbyReadyByte(void)
{
    if (net_ctrl_wk == NULL) {
        return 0;
    }
    if (net_ctrl_wk->sub_state_0x017 < 0x32) {
        return 0;
    }
    return net_ctrl_wk->flag_0xBF2D;
}

/*
 * Record a network error code (-1 always wins; any other code only when none is pending).
 */
void setErrorCode(s8 code)
{
    if (net_ctrl_wk != NULL) {
        if (code == -1) {
            net_ctrl_wk->error_code_0xC258 = -1;
            return;
        }
        if (code != 0 && net_ctrl_wk->error_code_0xC258 == 0) {
            net_ctrl_wk->error_code_0xC258 = code;
        }
    }
}

/*
 * Whether a sub-error is pending.
 */
BOOL NetCtrlWk::hasSubError(void)
{
    if (net_ctrl_wk == NULL) {
        return FALSE;
    }
    return net_ctrl_wk->sub_error_0xC259 != 0;
}

/*
 * Clear the pending sub-error.
 */
void NetCtrlWk::clearSubError(void)
{
    if (net_ctrl_wk != NULL) {
        net_ctrl_wk->sub_error_0xC259 = 0;
    }
}

/*
 * Record a sub-error code when it is non-zero.
 */
void NetCtrlWk::setSubError(s8 code)
{
    if (net_ctrl_wk != NULL && code != 0) {
        net_ctrl_wk->sub_error_0xC259 = code;
    }
}

/*
 * The message for the pending network error, or 0 when there is none.
 */
char* NetCtrlWk::getErrorMessage(void)
{
    if (net_ctrl_wk == NULL) {
        return NULL;
    }
    switch (net_ctrl_wk->error_code_0xC258) {
    case 0:
        return NULL;
    case -1:
        return getDefaultErrorMessage();
    default:
        return MH3GetErrorString2(net_ctrl_wk->error_code_0xC258);
    }
}

/*
 * The byte the layer mode flag stores at +0xC498.
 */
u8 NetCtrlWk::getLayerModeByte(void)
{
    if (net_ctrl_wk == NULL) {
        return 0;
    }
    return net_ctrl_wk->field_0xC498;
}

/*
 * Set the byte at +0xC498.
 */
void NetCtrlWk::setLayerModeByte(s32 value)
{
    if (net_ctrl_wk != NULL) {
        net_ctrl_wk->field_0xC498 = (s8)value;
    }
}

/*
 * The message for the pending sub-error, or 0 when there is none.
 */
char* get_network_sub_error_msg(void)
{
    if (net_ctrl_wk == NULL) {
        return NULL;
    }
    switch (net_ctrl_wk->sub_error_0xC259) {
    case 0:
        return NULL;
    case -1:
        return getDefaultSubErrorMessage();
    default:
        return MH3GetErrorString2(net_ctrl_wk->sub_error_0xC259);
    }
}

/*
 * Classify the event window against the game time and update the lobby's event flags; returns the class.
 */
s32 CalculateEvents(void)
{
    s32 window;

    if (net_ctrl_wk == NULL) {
        return 0;
    }
    window = net_ctrl_wk->times_0x96E4.classifyNow();
    lobby_w.field_0x161 = 0;
    lobby_w.field_0x15F = 0;
    switch (window) {
    case 1:
        lobby_w.field_0x161 = 1;
        break;
    case 2:
        lobby_w.field_0x15F = 1;
        NetCtrlWk::updateLobbyEventFlags();
        break;
    }
    return window;
}

/*
 * The schedule phase (0..1), or 0 when the work record is absent.
 */
f32 NetCtrlWk::getSchedulePhase(void)
{
    if (net_ctrl_wk == NULL) {
        return 0.0f;
    }
    return net_ctrl_wk->schedule_0x85DC.getPhase();
}

/*
 * The layer-state byte for slot `index`: the settings word for 0..3, and the layer's status words for 4 and 5.
 */
u8 NetCtrlWk::getLayerSetting(s32 index)
{
    NetCtrlWk* work = net_ctrl_wk;

    if (work == NULL) {
        return 0xFF;
    }
    if (index == 4) {
        if (getNetworkLayerPat(getPatsObject(), 0) != NULL && work->layer_state_0x064 == 2) {
            return getNetworkLayerPat(getPatsObject(), 0)->status_F030;
        }
        return 0;
    } else if (index == 5) {
        if (getNetworkLayerPat(getPatsObject(), 0) != NULL && work->layer_state_0x064 == 2) {
            return getNetworkLayerPat(getPatsObject(), 0)->status_F034;
        }
        return 0;
    }
    return work->settings_0x6210[index];
}

/*
 * Copy the roster and the recent-player list out for the caller as name and id-text rows.
 */
void NetCtrlWk::copyRosterLists(NetRosterListView* roster, NetRecentListView* recent)
{
    NetCtrlWk* work = net_ctrl_wk;
    s32 i;

    if (work == NULL) {
        return;
    }
    memset(roster, 0, sizeof(*roster));
    memset(recent, 0, sizeof(*recent));
    roster->count_0x000 = work->roster_count_0xA1B8;
    for (i = 0; i < roster->count_0x000; i++) {
        memcpy(roster->rows_0x004[i].name_0x00, work->roster_0xA1BC[i].name_0x20, 0x14);
        formatNetId(roster->rows_0x004[i].id_text_0x14, &work->roster_0xA1BC[i].id_0x00);
    }
    recent->count_0x000 = work->recent_count_0xBB84;
    for (i = 0; i < recent->count_0x000; i++) {
        memcpy(recent->rows_0x004[i].name_0x00, work->recent_0xBB88[i].name_0x20, 0x14);
        formatNetId(recent->rows_0x004[i].id_text_0x14, &work->recent_0xBB88[i].id_0x00);
    }
}

/*
 * Send a friend request with a message to `id`.
 */
BOOL NetCtrlWk::sendFriendRequest(const NetId* id, const char* message, s8* result)
{
    NetCtrlWk* work = net_ctrl_wk;

    if (work == NULL) {
        return FALSE;
    }
    importNetId(&work->request_id_0xC340, id);
    work->result_0xC290 = result;
    *result = 0;
    strcpy(work->message_0xBF08, message);
    getNetworkCommunityPat(getPatsObject(), 0)->sendFriendRequest(&work->request_id_0xC340);
    return TRUE;
}

/*
 * Accept the friend request from `id`.
 */
BOOL NetCtrlWk::acceptFriendRequest(const NetId* id, s8* result)
{
    NetCtrlWk* work = net_ctrl_wk;

    if (work == NULL) {
        return FALSE;
    }
    importNetId(&work->request_id_0xC340, id);
    work->result_0xC290 = result;
    *result = 0;
    getNetworkCommunityPat(getPatsObject(), 0)->acceptFriendRequest(&work->request_id_0xC340);
    return TRUE;
}

/*
 * Remove `id` from the friend list.
 */
BOOL NetCtrlWk::removeFriend(const NetId* id, s8* result)
{
    NetCtrlWk* work = net_ctrl_wk;

    if (work == NULL) {
        return FALSE;
    }
    work->result_0xC290 = result;
    *result = 0;
    importNetId(&work->request_id_0xC340, id);
    getNetworkCommunityPat(getPatsObject(), 0)->removeFriend(&work->request_id_0xC340);
    return TRUE;
}

/*
 * Send an invite of type `kind` to `id`.
 */
BOOL NetCtrlWk::inviteFriend(const NetId* id, s32 kind, s8* result)
{
    NetCtrlWk* work = net_ctrl_wk;

    if (work == NULL) {
        return FALSE;
    }
    importNetId(&work->request_id_0xC340, id);
    work->result_0xC290 = result;
    if (result != NULL) {
        *result = 0;
    }
    getNetworkCommunityPat(getPatsObject(), 0)->inviteFriend(&work->request_id_0xC340, kind);
    return TRUE;
}

/*
 * Submit a layer request carrying the text `text` and start the layer's page-100 refresh.
 */
BOOL NetCtrlWk::submitTextSelect(const char* text, s8* result)
{
    NetCtrlWk* work = net_ctrl_wk;

    if (work == NULL) {
        return FALSE;
    }
    initNetLayerRequest(&work->layer_request_0xC294);
    work->result_0xC290 = result;
    *result = 0;
    strcpy(work->text_0xC2E8, text);
    getNetworkLayerPat(getPatsObject(), 0)->submitSelect_A4(&work->layer_request_0xC294);
    getNetworkLayerPat(getPatsObject(), 0)->requestRefresh_6C(100, 1);
    return TRUE;
}

/*
 * Submit a layer request carrying the id `id` and start the layer's page-100 refresh.
 */
BOOL NetCtrlWk::submitIdSelect(const NetId* id, s8* result)
{
    NetCtrlWk* work = net_ctrl_wk;

    if (work == NULL) {
        return FALSE;
    }
    initNetLayerRequest(&work->layer_request_0xC294);
    work->result_0xC290 = result;
    *result = 0;
    importNetId(&work->target_id_0xC2C8, id);
    getNetworkLayerPat(getPatsObject(), 0)->submitSelect_A4(&work->layer_request_0xC294);
    getNetworkLayerPat(getPatsObject(), 0)->requestRefresh_6C(100, 1);
    return TRUE;
}

/*
 * Start a layer refresh unless one is running; returns whether it started (or is already under way).
 */
BOOL NetCtrlWk::startLayerRefresh(s8* result)
{
    NetCtrlWk* work = net_ctrl_wk;

    if (work == NULL) {
        return FALSE;
    }
    if (work->timeout_0xC484 > 0) {
        *result = 1;
        return TRUE;
    }
    initNetLayerRequest(&work->layer_request_0xC294);
    work->result_0xC290 = result;
    *result = 0;
    getNetworkLayerPat(getPatsObject(), 0)->submitSelect_A4(&work->layer_request_0xC294);
    if (work->layer_state_0x064 == 2) {
        getNetworkLayerPat(getPatsObject(), 0)->requestRefresh_6C(100, 8);
        work->flag_0xC480 = 1;
        work->timeout_0xC484 = get_server_big_data_timeout_element(4);
        return TRUE;
    } else if (work->layer_state_0x064 == 1) {
        getNetworkLayerPat(getPatsObject(), 0)->requestRefresh_6C(100, 4);
        work->flag_0xC480 = 1;
        work->timeout_0xC484 = get_server_big_data_timeout_element(4);
        return TRUE;
    }
    return FALSE;
}

/*
 * Submit the four slot ids that are set as one layer request and ask the layer for a page of 40.
 */
BOOL NetCtrlWk::submitSlotRequest(const NetSlotIds* ids, s8* result)
{
    NetCtrlWk* work = net_ctrl_wk;
    NetLayerRequestItem* item;
    s32 count;

    if (work == NULL) {
        return FALSE;
    }
    initNetLayerRequest(&work->layer_request_0xC294);
    count = 0;
    item = work->layer_request_0xC294.items_0x04;
    if (ids->slot_0x00 != -1) {
        item->slot_0x00 = 0;
        item->enabled_0x01 = 1;
        item->flag_0x04 = 1;
        item->value_0x08 = ids->slot_0x00;
        item++;
        count = 1;
    }
    if (ids->slot_0x04 != -1) {
        item->slot_0x00 = 1;
        item->enabled_0x01 = 1;
        item->flag_0x04 = 1;
        item->value_0x08 = ids->slot_0x04;
        item++;
        count++;
    }
    if (ids->slot_0x08 != -1) {
        item->slot_0x00 = 2;
        item->enabled_0x01 = 1;
        item->flag_0x04 = 1;
        item->value_0x08 = ids->slot_0x08;
        item++;
        count++;
    }
    if (ids->slot_0x0C != -1) {
        item->slot_0x00 = 3;
        item->enabled_0x01 = 1;
        item->flag_0x04 = 1;
        item->value_0x08 = ids->slot_0x0C;
        count++;
    }
    if (count == 0) {
        return FALSE;
    }
    work->layer_request_0xC294.count_0x00 = count;
    work->result_0xC290 = result;
    *result = 0;
    getNetworkLayerPat(getPatsObject(), 0)->submitRequest_9C(&work->layer_request_0xC294);
    getNetworkLayerPat(getPatsObject(), 0)->setPageSize_68(0x28);
    return TRUE;
}

/*
 * Copy the friend list (the layer's when `use_own` is 0, else the work record's own) out as rows.
 */
void NetCtrlWk::copyFriendList(NetFriendListView* out, u8 use_own)
{
    NetFriendEntry* entry;
    NetFriendView* row;
    s32 i;

    if (use_own == 0) {
        out->count_0x00 = getNetworkLayerPat(getPatsObject(), 0)->friendCount_6BC30;
        entry = getNetworkLayerPat(getPatsObject(), 0)->friends_6BC34;
    } else {
        out->count_0x00 = net_ctrl_wk->friend_list_0xC47C->count_0x00;
        entry = net_ctrl_wk->friend_list_0xC47C->entries_0x04;
    }
    row = out->rows_0x04;
    for (i = 0; i < out->count_0x00; i++, entry++, row++) {
        formatNetId(row->id_text_0x14, &entry->id_0x00);
        strcpy(row->name_0x00, entry->name_0x20);
        if (entry->online_0x38 != 0) {
            row->status_0x20 = entry->status_0x40 >> 24;
            row->area_0x1E = entry->area_0x48 >> 16;
            row->pad_0x21 = 0;
        }
    }
}

/*
 * Copy up to `max` community rows starting at `first` into `views`; `*total` receives the community count.
 */
s32 NetCtrlWk::copyCommunityViews(NetCommunityView* views, s16 first, s32 max, s16* total)
{
    NetworkLayerPat* layer = getNetworkLayerPat(getPatsObject(), 0);
    s32 count = layer->communityCount_F1AC;
    NetCommunityRec* community;
    s32 copied;
    s16 index;

    *total = count;
    if (count - 1 < first) {
        return 0;
    }
    community = &getNetworkLayerPat(getPatsObject(), 0)->communities_F1B0[first];
    copied = 0;
    for (index = first; index < first + max; index++, community++, views++) {
        if (index >= count || index - first >= max) {
            break;
        }
        views->valid_0x00 = 1;
        strcpy(views->name_0x01, community->name_0x0040);
        strcpy(views->leader_0x21, community->leader_0x0060);
        strcpy(views->comment_0x41, community->comment_0x00A0);
        views->value_0x54 = community->value_0x00E0;
        views->value_0x58 = community->value_0x00EC;
        views->value_0x5C = community->value_0x00E8;
        views->index_0x60 = index;
        views->slots_0x64[0] = community->slot_0x00FC;
        views->slots_0x64[1] = community->slot_0x0104;
        views->slots_0x64[2] = community->slot_0x010C;
        views->slots_0x64[3] = community->slot_0x0114;
        copied++;
    }
    return copied;
}

/*
 * Whether the friend with id `id` has its detail flag set.
 */
BOOL NetCtrlWk::hasFriendDetail(const NetId* id)
{
    NetFriendRec* friend_rec;
    s32 i;

    if (net_ctrl_wk == NULL) {
        return FALSE;
    }
    friend_rec = getNetworkLayerPat(getPatsObject(), 0)->friends_3568.entries_0x004;
    for (i = 0; i < 100; i++, friend_rec++) {
        if (isSameNetId(&friend_rec->id_0x00, id) != 0 && friend_rec->valid_0x35 != 0) {
            return getNetworkLayerPat(getPatsObject(), 0)->details_5958[i].flag_0x101 != 0;
        }
    }
    return FALSE;
}

/*
 * The online-support code string, or NULL when the work record is absent.
 */
char* getOnlineSupportCode(void)
{
    if (net_ctrl_wk == NULL) {
        return NULL;
    }
    return net_ctrl_wk->support_code_0xC1DE;
}

/*
 * Select the fetch mode: 1 clears the fetch-off byte, anything else sets it.
 */
void NetCtrlWk::setFetchMode(u32 mode)
{
    if (net_ctrl_wk != NULL) {
        if (mode == 1) {
            net_ctrl_wk->flag_0xC48A = 0;
            setTransferMode(1);
            return;
        }
        net_ctrl_wk->flag_0xC48A = 1;
        setTransferMode(0);
    }
}

/*
 * Step the download of the ten numbered data files into their buffers; returns 1 when all are done, -1 on
 * error and 0 while running.
 */
s32 NetCtrlWk::stepFileDownloads(void)
{
    u32 status;
    char path[16];
    s32 index = fetch_index_0x82D4;
    u32* sum = &file_sums_0xC3C4[index];
    void* buffer = file_buffers_0xC39C[index];
    NetworkFileFetcher* fetcher = fetcher_0x82CC;

    switch (fetch_step_0x82D0) {
    case 0:
        fetch_step_0x82D0++;
        fetch_index_0x82D4 = 0;
        break;
    case 1:
        fetcher = new NetworkFileFetcher;
        fetcher_0x82CC = fetcher;
        if (fetcher == NULL) {
            fetch_error_0xA0F4.code_0x0 = 0x80000001;
            fetch_error_0xA0F4.detail_0x4 = 0;
            fetch_error_0xA0F4.reason_0x8 = 0x80000000;
            return -1;
        }
        snprintf(path, 16, "%d", fetch_index_0x82D4 + group_0xC494 * 15 + 6);
        if (fetcher->NetworkFileFetcher::open(0, path) < 0) {
            fetch_step_0x82D0 = 10;
        } else {
            fetch_step_0x82D0++;
        }
        break;
    case 2: {
        s32 result = fetcher->poll(&status);

        if (result == 0) {
            if (*sum == status) {
                fetch_step_0x82D0 = 9;
            } else {
                *sum = status;
                if (*sum == 0xFFFFFFFF) {
                    fetch_step_0x82D0 = 9;
                } else {
                    fetch_step_0x82D0++;
                }
            }
        } else if (result < 0) {
            fetch_step_0x82D0 = 10;
        }
        break;
    }
    case 3:
        snprintf(path, 16, "%d", index + group_0xC494 * 15 + 6);
        if (fetcher->open(0, path) < 0) {
            fetch_step_0x82D0 = 10;
        } else {
            fetch_step_0x82D0++;
        }
        break;
    case 4: {
        s32 result = fetcher->poll(&status);

        if (result == 0) {
            fetch_step_0x82D0++;
        } else if (result < 0) {
            fetch_step_0x82D0 = 10;
        }
        break;
    }
    case 5:
        if (fetcher->read(buffer, 0x400) < 0) {
            fetch_step_0x82D0 = 10;
        } else {
            fetch_step_0x82D0++;
        }
        break;
    case 6: {
        s32 result = fetcher->poll(&status);

        if (result == 0) {
            fetch_step_0x82D0++;
        } else if (result < 0) {
            fetch_step_0x82D0 = 10;
        }
        break;
    }
    case 7:
        if (fetcher->close() < 0) {
            fetch_step_0x82D0 = 10;
        } else {
            fetch_step_0x82D0++;
        }
        break;
    case 8: {
        s32 result = fetcher->poll(&status);

        if (result == 0) {
            fetch_step_0x82D0++;
        } else if (result < 0) {
            fetch_step_0x82D0 = 10;
        }
        break;
    }
    case 9:
        delete fetcher;
        fetcher_0x82CC = NULL;
        if (fetch_index_0x82D4 >= 9) {
            fetch_step_0x82D0 = 0;
            fetch_index_0x82D4 = 0;
            return 1;
        }
        fetch_step_0x82D0 = 1;
        fetch_index_0x82D4++;
        break;
    case 10:
        memset(file_sums_0xC3C4, 0, sizeof(file_sums_0xC3C4));
        fetch_step_0x82D0 = 0;
        fetch_index_0x82D4 = 0;
        fetcher->copyError(&fetch_error_0xA0F4);
        delete fetcher_0x82CC;
        fetcher_0x82CC = NULL;
        return -1;
    }
    return 0;
}

/*
 * Copy the 0x2000-byte staging buffer into the caller's buffer.
 */
void NetCtrlWk::copyStaging(u8* dst)
{
    memcpy(dst, net_ctrl_wk->staging_0xC3EC, 0x2000);
}

/*
 * Step the download of numbered file `file` into the staging buffer and verify it against `version`;
 * returns 1 when done, -1 on error and 0 while running.
 */
s32 NetCtrlWk::stepStagingDownload(s32 file, u16 version)
{
    u32 status;
    char path[16];
    u8* staging = staging_0xC3EC;
    NetworkFileFetcher* fetcher = fetcher_0x82CC;

    switch (fetch_step_0x82D0) {
    case 0:
        fetcher = new NetworkFileFetcher;
        fetcher_0x82CC = fetcher;
        if (fetcher == NULL) {
            fetch_error_0xA0F4.code_0x0 = 0x80000001;
            fetch_error_0xA0F4.detail_0x4 = 0;
            fetch_error_0xA0F4.reason_0x8 = 0x80000000;
            return -1;
        }
        snprintf(path, 16, "%d", file + group_0xC494 * 15 + 6);
        if (fetcher->open(0, path) < 0) {
            fetch_step_0x82D0 = 10;
        } else {
            fetch_step_0x82D0++;
        }
        break;
    case 1: {
        s32 result = fetcher->poll(&status);

        if (result == 0) {
            fetch_step_0x82D0++;
        } else if (result < 0) {
            fetch_step_0x82D0 = 10;
        }
        break;
    }
    case 2:
        if (fetcher->read(staging, 0x3000) < 0) {
            fetch_step_0x82D0 = 10;
        } else {
            fetch_step_0x82D0++;
        }
        break;
    case 3: {
        s32 result = fetcher->poll(&status);

        if (result == 0) {
            fetch_step_0x82D0++;
        } else if (result < 0) {
            fetch_step_0x82D0 = 10;
        }
        break;
    }
    case 4:
        if (fetcher->close() < 0) {
            fetch_step_0x82D0 = 10;
        } else {
            fetch_step_0x82D0++;
        }
        break;
    case 5: {
        s32 result = fetcher->poll(&status);

        if (result == 0) {
            fetch_step_0x82D0++;
        } else if (result < 0) {
            fetch_step_0x82D0 = 10;
        }
        break;
    }
    case 6:
        snprintf(path, 16, "%d", file + group_0xC494 * 15 + 6);
        if (fetcher->NetworkFileFetcher::open(0, path) < 0) {
            fetch_step_0x82D0 = 10;
        } else {
            fetch_step_0x82D0++;
        }
        break;
    case 7: {
        s32 result = fetcher->poll(&status);

        if (result == 0) {
            if (status == 0xFFFFFFFF) {
                fetch_step_0x82D0 = 10;
            } else if (matchesFileVersion(staging, (u16)version) == 0) {
                fetch_step_0x82D0 = 10;
            } else {
                fetch_step_0x82D0++;
            }
        } else if (result < 0) {
            fetch_step_0x82D0 = 10;
        }
        break;
    }
    case 8:
        fetch_step_0x82D0 = 0;
        fetch_index_0x82D4 = 0;
        delete fetcher;
        fetcher_0x82CC = NULL;
        return 1;
    case 10:
        fetch_step_0x82D0 = 0;
        fetch_index_0x82D4 = 0;
        fetcher->copyError(&fetch_error_0xA0F4);
        delete fetcher_0x82CC;
        fetcher_0x82CC = NULL;
        sub_error_0xC259 = 0x10;
        return -1;
    }
    return 0;
}

/*
 * Step the staging download of `file` at `version` on the work record, or fail with -1 when it is absent.
 */
s32 NetCtrlWk::stepStagingDownloadIfUp(s32 file, u32 version)
{
    if (net_ctrl_wk == NULL) {
        return -1;
    }
    return net_ctrl_wk->stepStagingDownload(file, (u16)version);
}

/* The text colours (RGBA) the message renderer selects from. */
u32 text_color_table[8] = {
    0x000000FF, 0xFF435DFF, 0x56FF56FF, 0xFFFF50FF, 0x747EFFFF, 0xFF84FFFF, 0x57FFFFFF, 0xF0F0F0FF,
};

/*
 * Select the text colour `index` from the colour table.
 */
void setTextColor(u8 index)
{
    flfntSetColor(text_color_table[index]);
}

/*
 * Set the text size to a square of `size`.
 */
void setTextSize(s16 size)
{
    flfntSetSize(size, size);
}

/*
 * Print `text` at (x, y), splitting it into runs of at most 300 characters.
 */
void printTextRuns(s16 x, s16 y, s32 unused, char* text)
{
    char run[0x400];
    char* end;

    flfntSetPos(x, y);
    for (;;) {
        u32 length;

        end = flKnjMsgNumPtr(text, 300);
        if (end == NULL) {
            break;
        }
        u32 i;

        memset(run, 0, sizeof(run));
        length = end - text;
        i = 0;
        for (; i < length; i++) {
            run[i] = text[i];
        }
        text += length;
        flfntPrintf(run);
        flfntFlush();
        flfntStackReset();
    }
    flfntPrintf(text);
    flfntFlush();
    flfntStackReset();
}

/*
 * Read the tag name up to the closing '>' into the tag buffer; an over-long or multi-byte name is
 * replaced by "END" and flagged.
 */
void NetTextTagState::parseTag(void)
{
    char* out = tag_0x2D8;
    u16 length;

    memset(out, 0, sizeof(tag_0x2D8));
    length = 0;
    for (;;) {
        char c = *cursor_0x308;
        s32 width;

        *out = c;
        if (c == '>') {
            break;
        }
        out++;
        cursor_0x308++;
        length++;
        width = getUtf8CharLength((u8*)cursor_0x308);
        if (length >= 0x19 || width > 1) {
            memset(tag_0x2D8, 0, sizeof(tag_0x2D8));
            strcpy(tag_0x2D8, "END");
            status_0x30D = 2;
            return;
        }
    }
    cursor_0x308++;
}

