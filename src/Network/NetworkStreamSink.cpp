/*
 * Network/NetworkStreamSink.cpp - the stream buffer (`NetworkStreamSink`; "NetworkBuffer::" in its log strings) with its
 *   frame scrambler and CRC, and the library accessor `getNetworkLogger`.
 * RANGE. .text 0x803C987C-0x803CA1D4 (15 functions), extab 0x80019554-0x80019584, extabindex 0x80039E64-0x80039EAC
 *   (6 entries), .rodata 0x80570C20-0x80570E20 (the CRC-16 table `checksum` reads), .data 0x805F8DB0-0x805F9190 (the
 *   log strings and `__vt__17NetworkStreamSink`).  No .bss, .sdata, .sbss or .sdata2 of its own.
 * RANGE. Left edge 0x803C987C: cut from `lobby/lb_server_sel_trans.cpp` (no `.text` reference crosses it, the `.data`
 *   labels and the extab/extabindex records split at the same function).  Right edge: the `.data` zigzag at 0x805F9190
 *   (this table is followed by a rising one, the connection base's, first read by 0x803CA1D4); `checksum` is the last
 *   function reading this unit's data and the extabindex records split there (`Network/NetworkConnection.cpp`).
 * FLAGS. `-O3 -inline noauto -pool off` (configure.py; docs/network.md), file-scope `#pragma peephole off` (the
 *   destructor's `extsh`, the scramblers' `clrlwi` before each `stb`).
 * NAMES. The file name is a GUESS from the class, `NetworkStreamSink`; its log strings call the class `NetworkBuffer`
 *   and the slots `init` (`clear`), `serialize` (`fill`), `deserialize` (`put`), `duplicate` (`copyFrom`) and `equals`
 *   (`slot_2C`).  `networkCrc16Table` (0x80570C20, the CRC-16/CCITT table) is a GUESS from its content.
 * RESIDUALS. `slot_2C` (0x803C9CD8) is not written: retail takes the other buffer and returns the verdict, which
 *   `virtual void slot_2C()` (the declaration `NetworkUniqueId` overrides as declared) cannot carry.  Data: `.rodata`
 *   is byte-identical; `.data` matches up to 0x2D8 (the `equals` strings are not emitted while `slot_2C` has no body).
 */

#include "Network/NetworkStreamSink.h"
#include "Network/sNetworkLibrary.h"        /* sNetworkLibrary::mpInstance */
#include "unsplit/Network.h"                /* NetworkLogger */
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "MSL_C/alloc.h"                    /* memcmp */

#pragma peephole off

/* 0x80570C20 - the CRC-16/CCITT table (polynomial 0x1021) `checksum` reads. */
static const u16 networkCrc16Table[256] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
    0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
    0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
    0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
    0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
    0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
    0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
    0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
    0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
    0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
    0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
    0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
    0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
    0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
    0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
    0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
    0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
    0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
    0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
    0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
    0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
    0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
    0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0,
};

/* ==== the stream buffer ("NetworkBuffer::" in its log strings) ================================================ */

NetworkStreamSink::NetworkStreamSink()
{
    data_04 = NULL;
    capacity_08 = 0;
    used_0C = 0;
}

NetworkStreamSink::~NetworkStreamSink()
{
}

/* Takes the stored bytes; empty in the base class. */
void NetworkStreamSink::onFlush(u8* data, u32 size)
{
}

/* Hands the stored bytes to `onFlush`. */
void NetworkStreamSink::flush()
{
    onFlush(data_04, used_0C);
}

/* Binds the buffer to an empty block and clears the block. */
void NetworkStreamSink::attach(u8* block, u32 capacity)
{
    data_04 = block;
    capacity_08 = capacity;
    used_0C = 0;
    memset(block, 0, capacity);
}

/* Clears the whole block ("init"). */
void NetworkStreamSink::clear()
{
    if (data_04 == NULL) {
        getNetworkLogger()->warn_10("NetworkBuffer::init: this->buf is null.\n");
        return;
    }
    memset(data_04, 0, capacity_08);
}

/* The library singleton the band's log and byte-order calls go through. */
NetworkLogger* getNetworkLogger(void)
{
    return (NetworkLogger*)sNetworkLibrary::mpInstance;
}

/* Binds the buffer to a block that is full. */
void NetworkStreamSink::bind(u8* block, u32 size)
{
    data_04 = block;
    capacity_08 = size;
    used_0C = size;
}

/* Copies the stored bytes out ("serialize"): their count, or -1 when there is no block, no output or too little
 * room. */
s32 NetworkStreamSink::fill(u8* out, u32 size)
{
    if (data_04 == NULL) {
        getNetworkLogger()->warn_10("NetworkBuffer::serialize: this->buf is null.\n");
        return -1;
    }
    if (out == NULL) {
        getNetworkLogger()->warn_10("NetworkBuffer::serialize: arg->data_buf is null.\n");
        return -1;
    }
    if (size < used_0C) {
        getNetworkLogger()->warn_10("NetworkBuffer::serialize: arg->data_buf_size < this->len\n");
        return -1;
    }
    if (used_0C == 0) {
        getNetworkLogger()->log_14("NetworkBuffer::serialize: this->len is zero.\n");
        return 0;
    }
    memcpy(out, data_04, used_0C);
    return used_0C;
}

/* Stores `size` bytes ("deserialize"): their count, or -1 when there is no block, no input or too much. */
s32 NetworkStreamSink::put(const u8* data, u32 size)
{
    if (data_04 == NULL) {
        getNetworkLogger()->warn_10("NetworkBuffer::deserialize: this->buf is null.\n");
        return -1;
    }
    if (data == NULL) {
        getNetworkLogger()->warn_10("NetworkBuffer::deserialize: arg->data_buf is null.\n");
        return -1;
    }
    if (capacity_08 < size) {
        getNetworkLogger()->warn_10("NetworkBuffer::deserialize: this->max < arg->data_size\n");
        return -1;
    }
    if (size == 0) {
        getNetworkLogger()->log_14("NetworkBuffer::deserialize: arg->data_size is zero.\n");
        return 0;
    }
    used_0C = size;
    memcpy(data_04, data, size);
    return used_0C;
}

/* Copies another buffer's stored bytes in ("duplicate"): their count, or -1. */
s32 NetworkStreamSink::copyFrom(const u8* src)
{
    const NetworkStreamSink* other = (const NetworkStreamSink*)src;

    if (data_04 == NULL) {
        getNetworkLogger()->warn_10("NetworkBuffer::duplicate: this->buf is null.\n");
        return -1;
    }
    if (other == NULL) {
        getNetworkLogger()->warn_10("NetworkBuffer::duplicate: arg->obj is null.\n");
        return -1;
    }
    if (other->data_04 == NULL) {
        getNetworkLogger()->warn_10("NetworkBuffer::duplicate: arg->obj->buf is null.\n");
        return -1;
    }
    if (capacity_08 < other->used_0C) {
        getNetworkLogger()->warn_10("NetworkBuffer::duplicate: this->max < arg->obj->len\n");
        return -1;
    }
    if (other->used_0C == 0) {
        getNetworkLogger()->log_14("NetworkBuffer::duplicate: arg->obj->len is zero.\n");
        return 0;
    }
    memcpy(data_04, other->data_04, other->used_0C);
    used_0C = other->used_0C;
    return used_0C;
}

/* Scrambles `size` bytes from `offset`: each byte is XORed with the previous plain byte, the first with `key`. */
void NetworkStreamSink::encrypt(u8 key, u16 offset, u16 size)
{
    u8* p = &data_04[offset];
    u16 i;

    for (i = 0; i < size; i++, p++) {
        u8 plain = *p;
        *p = plain ^ key;
        key = plain;
    }
}

/* Undoes `encrypt`. */
void NetworkStreamSink::decrypt(u8 key, u16 offset, u16 size)
{
    u8* p = &data_04[offset];
    u16 i;

    for (i = 0; i < size; i++, p++) {
        u8 plain = *p ^ key;
        *p = plain;
        key = plain;
    }
}

/* The CRC-16/CCITT of the first `size` bytes (seed 0xFFFF). */
u16 NetworkStreamSink::checksum(u16 size)
{
    u16 crc = 0xFFFF;
    u8* p = data_04;
    u16 i;

    for (i = 0; i < size; i++, p++) {
        crc = (crc << 8) ^ networkCrc16Table[(u8)((crc >> 8) ^ *p)];
    }
    return crc;
}
