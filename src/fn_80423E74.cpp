/*
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

#include "fn_80423E74.h"

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
