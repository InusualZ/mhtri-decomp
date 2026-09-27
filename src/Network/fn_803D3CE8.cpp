/*
 * fn_803D3CE8.cpp - the Network session band, `.text` 0x803D3CE8..0x803D70B8.
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/dumpmap.py lookup`: every address in the range answers a `zz_XXXXXXXX_`
 * dump name, never a real function name).
 *
 * WHAT IT IS.  The serialization/state half of the Wii network session subsystem: the op-code
 * packet writers of `NetworkSessionStable`, the request pool and its state machine on
 * `NetworkSessionManager` (21 request slots, a two-slot `NetworkRequest` pool at +0x7C, virtual
 * dispatch), and the first virtual slots of `NetworkSessionManagerPat`.
 *
 * MODULE AND NAME (brief section 2, evidence order).  1. No `__FILE__` string is reachable: every
 * `lis`/`addi` pair in the range resolves to the float pool, a `.data` request descriptor or one of
 * the class format strings ("NetworkSessionStable::move ...", "NetworkSessionManager::deleteRequest
 * ...", "NetworkSessionManagerPat::final ..."), never to a source-file-name literal.  2.
 * `dumpmap.py lookup` answers only `zz_XXXXXXXX_` for the code and the runtime dump's own table
 * names (`NetworkSessionStable_VTable`, `NetworkSessionManagerVTable`,
 * `_803dc590NetworkSessionManagerPatVTable`) for the data.  3. The code and the vtables place the
 * band in `Network` (the registered neighbour is `Network/NetworkWiiMediator.c`; the classes are
 * `NetworkSession*`).  The tile spans more than one original TU (Stable methods at the front,
 * Manager in the middle, ManagerPat at the back), so no single evidenced file name covers it and
 * the file keeps the map's `fn_803D3CE8` stem (brief option 4): no name was invented.
 *
 * LANGUAGE AND SECTIONS.  C++ (`__nw__FUl` / `__dl__FPv` / `__ptmf_scall`), exceptions off, `.text`
 * only (the range carries no extab/extabindex).  Every plain `fn_` definition is `extern "C"`.
 *
 * STATUS / RESIDUALS.  See the outbox `config_requests` and the batch note.
 */

#include "types.h"
#include "Network/fn_803D3CE8.h"
#include "unsplit/NetworkData.h"

/* ---- this unit's own forward declarations ---- */
extern "C" {
void fn_803D3CE8(NetworkSessionStable*, u32);
void fn_803D3DA8(NetworkSessionStable*);
void fn_803D3E3C(NetworkSessionStable*, s8);
void fn_803D3ED8(NetworkSessionStable*, s8);
void fn_803D3FA8(NetworkSessionStable*, s8, u32, u32, s8, f32);
void fn_803D40CC(NetworkSessionStable*, u32, s8);
void fn_803D41D0(NetworkSessionStable*, const void*, u32, s8);
void fn_803D4290(NetworkSessionStable*, s8);
s8 fn_803D4858(NetworkSessionStable*, s8);

NetworkSessionManager* fn_803D4904(NetworkSessionManager*);
void fn_803D4A50(NetworkRequest*);
void* dtor_803D4AF8(NetworkRequest*, s16);
void fn_803D4B5C(NetworkRequest*);
NetworkRequest* fn_803D4B60(NetworkRequest*);
void* dtor_803D4B9C(NetworkSessionManager*, s16);
void fn_803D4C18(NetworkSessionManager*, u32, u32);
void fn_803D4C24(NetworkSessionManager*);
void fn_803D4D20(NetworkSessionManager*);
void fn_803D4DE4(NetworkSessionManager*);
s32 fn_803D5070(NetworkRequest*);
void zz_03d5084_ptmf_scall(NetworkRequest*);
void fn_803D50D8(NetworkSessionManager*);
void fn_803D5150(NetworkRequest*, NetworkSessionManager*, NetworkRequestDesc, u32, ...);
void fn_803D528C(NetworkSessionManager*);
void fn_803D5318(NetworkSessionManager*, u32, u32);
void fn_803D53B0(NetworkSessionManager*, u32);
void fn_803D5438(NetworkSessionManager*, u32);
void fn_803D54C0(NetworkSessionManager*, u32);
void fn_803D5548(NetworkSessionManager*, u32);
void fn_803D55D0(NetworkSessionManager*);
void fn_803D5648(NetworkSessionManager*, u32);
void fn_803D56D0(NetworkSessionManager*, s8);
void fn_803D5758(NetworkSessionManager*);
void fn_803D57D0(NetworkSessionManager*, u32);
void fn_803D5858(NetworkSessionManager*);
void fn_803D58D0(NetworkSessionManager*);
void fn_803D5948(NetworkSessionManager*);
void fn_803D59C0(NetworkSessionManager*);
void fn_803D5A38(NetworkSessionManager*, u32, u32, s8);
void fn_803D5AE0(NetworkSessionManager*);
void fn_803D5B58(NetworkSessionManager*, u32, u32);
void fn_803D5BF0(NetworkSessionManager*, u32, u32);
void fn_803D5C88(NetworkSessionManager*, u32);
void fn_803D5D10(NetworkSessionManager*);
void fn_803D5D58(NetworkRequest*);
s32 fn_803D5D64(NetworkRequest*);
void fn_803D5D6C(NetworkSessionManager*);
void fn_803D5DB4(NetworkSessionManager*, s8);
void fn_803D5DC4(NetworkSessionManager*, s8);
s32 fn_803D5DCC(NetworkSessionManager*, s8);
f32 fn_803D5E38(NetworkSessionManager*, s8);
void fn_803D5EA4(NetworkSessionManager*);
void fn_803D5EF8(NetworkSessionManager*, u32, u32, u8);
void fn_803D5F4C(NetworkSessionManager*, u32, u32);
void fn_803D5F9C(NetworkSessionManager*, u32, u32, u8);
NetworkRequest* fn_803D6430(NetworkSessionManager*);
void fn_803D64A4(NetworkSessionManager*, NetworkRequest**);
s32 fn_803D6514(NetworkRequest*, u32*);
void fn_803D658C(NetworkRequest*, u32, u32, u32);
s32 fn_803D65F4(NetworkRequest*, u32);
}

/* ---- the debug/assert manager the range logs through ---- */
typedef struct DebugVtable {
    u8 pad00[0x10];                       /* +0x00 (approximation: only the logged slots are named) */
    void (*warn_10)(void* self, const char* fmt, ...);                  /* +0x10 */
    void (*log_14)(void* self, const char* fmt);                   /* +0x14 */
} DebugVtable;   /* size: 0x18 (approximation: the full debug-manager vtable is longer) */

struct DebugManager {
    DebugVtable* vtable;   /* +0x00 */
};   /* size: 0x04 */

/* ---- extra neighbouring globals ---- */
/* The four addresses themselves are declared in the band's data header (rule 2).  They cannot come
   from `include/unsplit/Network.h`: that header declares `dtor_803CA338(void*, s32)` where this
   file's own header declares `dtor_803CA338(void*)`, and including both fails to compile - the
   reason `include/unsplit/NetworkData.h` exists. */
extern "C" {
struct DebugManager;
DebugManager* fn_803C9974(void);
}

/* ----------------------------------------------------------------------------------------- */
/* NetworkSessionStable - op-code packet writers                                             */
/* ----------------------------------------------------------------------------------------- */

extern "C" void fn_803D3CE8(NetworkSessionStable* self, u32 value)
{
    u8 stream[0x24];
    s8 term;
    u16 n;

    fn_803CB9B4(stream);
    fn_803F89D0(stream, self->sendBuffer_0D, 0x400);
    fn_803F8A14(stream, 0);
    n = writeByte(stream, 1);
    writeSize(stream, (u16)(n + writeUInt(stream, value)));
    term = -1;
    fn_803D39BC(self, stream, 0, 1, &term, 0xFF);
    dtor_803CB958(stream, -1);
}

extern "C" void fn_803D3DA8(NetworkSessionStable* self)
{
    u8 stream[0x24];
    s8 term;

    fn_803CB9B4(stream);
    fn_803F89D0(stream, self->sendBuffer_0D, 0x400);
    fn_803F8A14(stream, 0);
    writeSize(stream, writeByte(stream, 2));
    term = -1;
    fn_803D39BC(self, stream, 0, 1, &term, 0xFF);
    dtor_803CB958(stream, -1);
}

extern "C" void fn_803D3E3C(NetworkSessionStable* self, s8 value)
{
    u8 stream[0x24];
    s8 term;

    fn_803CB9B4(stream);
    fn_803F89D0(stream, self->sendBuffer_0D, 0x400);
    fn_803F8A14(stream, 0);
    writeSize(stream, writeByte(stream, 6));
    term = value;
    fn_803D39BC(self, stream, 0, 1, &term, 0xFF);
    dtor_803CB958(stream, -1);
}

extern "C" void fn_803D3ED8(NetworkSessionStable* self, s8 idx)
{
    u8 stream[0x24];
    s8 term;
    u16 n;

    fn_803CB9B4(stream);
    fn_803F89D0(stream, self->sendBuffer_0D, 0x400);
    fn_803F8A14(stream, 0);
    n = writeByte(stream, 10);
    writeSize(stream, (u16)(n + fn_803F8BDC(stream, &self->slots_14838[idx].state_20)));
    term = -2;
    fn_803D39BC(self, stream, 0, 1, &term, 0xFF);
    dtor_803CB958(stream, -1);
}

extern "C" void fn_803D3FA8(NetworkSessionStable* self, s8 a, u32 b, u32 c, s8 d, f32 f)
{
    u8 stream[0x24];
    s8 term;
    u32 scaled;
    u16 n;
    u32 n2;
    u32 n3;
    u32 n4;

    scaled = 0;
    fn_803CB9B4(stream);
    scaled = (u32)(1000.0f * f);
    fn_803F89D0(stream, self->sendBuffer_0D, 0x400);
    fn_803F8A14(stream, 0);
    n = writeByte(stream, 11);
    n2 = n + fn_803F8BDC(stream, (const void*)b);
    n3 = n2 + writeUInt(stream, c);
    n4 = n3 + writeUInt(stream, scaled);
    writeSize(stream, (u16)(n4 + writeByte(stream, (u32)d)));
    term = a;
    fn_803D39BC(self, stream, 0, 1, &term, 0xFF);
    dtor_803CB958(stream, -1);
}

extern "C" void fn_803D40CC(NetworkSessionStable* self, u32 has_extra, s8 idx)
{
    u8 stream[0x24];
    s8 term;
    u16 n;

    fn_803CB9B4(stream);
    fn_803F89D0(stream, self->sendBuffer_0D, 0x400);
    fn_803F8A14(stream, 0);
    if (has_extra != 0) {
        n = writeByte(stream, 9);
    } else {
        n = writeByte(stream, 8);
    }
    n = (u16)(n + writeUInt(stream, self->slots_14838[idx].playerId_40));
    writeSize(stream, (u16)(n + writeUInt(stream, self->tick_16CD8)));
    term = idx;
    fn_803D39BC(self, stream, 0, 1, &term, 0xFF);
    dtor_803CB958(stream, -1);
}

extern "C" void fn_803D41D0(NetworkSessionStable* self, const void* data, u32 len, s8 idx)
{
    u8 stream[0x24];
    s8 term;
    u16 n;

    fn_803CB9B4(stream);
    fn_803F89D0(stream, self->sendBuffer_0D, 0x400);
    fn_803F8A14(stream, 0);
    n = writeByte(stream, 4);
    writeSize(stream, (u16)(n + writeBytes(stream, data, len)));
    term = idx;
    fn_803D39BC(self, stream, 0, 1, &term, 0xFF);
    dtor_803CB958(stream, -1);
}

/* ----------------------------------------------------------------------------------------- */
/* NetworkSessionStable - performance / state                                                */
/* ----------------------------------------------------------------------------------------- */

extern "C" s8 fn_803D4858(NetworkSessionStable* self, s8 idx)
{
    NetworkSessionSlot* slot;
    NetworkSessionSlot* sub;
    s8 subIdx;

    if (idx < 0 || idx >= 4) {
        return -1;
    }
    slot = &self->slots_14838[idx];
    if (slot->active_1C == 0) {
        return -1;
    }
    if (slot->linked_09 != 0) {
        return idx;
    }
    subIdx = slot->ownerIndex_00;
    if (subIdx < 0) {
        return -1;
    }
    sub = &self->slots_14838[subIdx];
    if (sub->active_1C == 0) {
        return -1;
    }
    if (sub->linked_09 != 0) {
        return subIdx;
    }
    if (sub->ready_0B != 0) {
        return subIdx;
    }
    return -1;
}

extern "C" void fn_803D4290(NetworkSessionStable* self, s8 idx)
{
    NetworkSessionSlot* slot;
    f32 a;
    f32 b;
    f32 r;

    if (fn_803D4858(self, idx) < 0) {
        return;
    }
    slot = &self->slots_14838[idx];
    a = 2.0f * self->vtable->getFloat_8C(self, idx);
    b = 2.0f * slot->rate2_BC;
    r = a;
    if (b > r) {
        r = b;
    }
    if (r < 0.1f) {
        r = 0.1f;
    }
    if (r > 1.0f) {
        r = 1.0f;
    }
    slot->rate_B8 = r;
}

/* ----------------------------------------------------------------------------------------- */
/* NetworkSessionManager - construction / pool                                                */
/* ----------------------------------------------------------------------------------------- */

extern "C" NetworkSessionManager* fn_803D4904(NetworkSessionManager* self)
{
    s32 i;

    self->vtable = &NetworkSessionManagerVTable;
    __construct_array(&self->pool_7C[0], (void*)fn_803D4B60, (void*)dtor_803D4AF8, 0xA4, 2);
    self->unused_04 = 0;
    self->unused_08 = 0;
    self->buffer = 0;
    for (i = 0; i < 21; i++) {
        self->requests_10[i] = 0;
        self->request_state_64[i] = 0;
    }
    self->unused_79 = 1;
    self->unused_7A = 1;
    for (i = 0; i < 2; i++) {
        fn_803D4A50(&self->pool_7C[i]);
    }
    return self;
}

extern "C" void fn_803D4A50(NetworkRequest* self)
{
    self->unused_00 = 0;
    self->interval_4C = 0.0f;
    self->timeout_50 = 0.0f;
    self->requestId_70 = 0;
    self->unused_24 = 0;
    self->cancelled_74 = 0;
    self->owner_94 = 0;
    self->desc_98 = NetworkRequest_defaultDescriptor[0];
    self->desc_9C = NetworkRequest_defaultDescriptor[1];
    self->desc_A0 = NetworkRequest_defaultDescriptor[2];
    self->count_28 = 0;
    self->record_54 = 0;
    self->record_58 = 0;
    self->record_5C = 0;
    self->unused_60 = 0;
    self->unused_64 = 0;
    self->unused_68 = 0;
    self->unused_6C = 0;
    self->unused_04 = 0;
    self->unused_08 = 0;
    self->buffer = 0;
    self->unused_10 = 0;
    self->unused_14 = 0;
    self->unused_18 = 0;
    self->unused_1C = 0;
    self->unused_20 = 0;
    self->args_2C[0] = 0;
    self->args_2C[1] = 0;
    self->args_2C[2] = 0;
    self->args_2C[3] = 0;
    self->args_2C[4] = 0;
    self->args_2C[5] = 0;
    self->args_2C[6] = 0;
    self->args_2C[7] = 0;
}

extern "C" void* dtor_803D4AF8(NetworkRequest* self, s16 flags)
{
    if (self != 0) {
        fn_803D4A50(self);
        dtor_803CA338(self->mutex_78);
        if (flags > 0) {
            operator delete(self);
        }
    }
    return self;
}

extern "C" void fn_803D4B5C(NetworkRequest* self)
{
    fn_803D4A50(self);
}

extern "C" NetworkRequest* fn_803D4B60(NetworkRequest* self)
{
    fn_803CA37C(self->mutex_78);
    fn_803D4A50(self);
    return self;
}

extern "C" void* dtor_803D4B9C(NetworkSessionManager* self, s16 flags)
{
    if (self != 0) {
        self->vtable = &NetworkSessionManagerVTable;
        fn_803D4D20(self);
        __destroy_arr(&self->pool_7C[0], (void*)dtor_803D4AF8, 0xA4, 2);
        if (flags > 0) {
            operator delete(self);
        }
    }
    return self;
}

extern "C" void fn_803D4C18(NetworkSessionManager* self, u32 a, u32 b)
{
    self->unused_04 = a;
    self->unused_08 = b;
    fn_803D4C24(self);
}

extern "C" void fn_803D4C24(NetworkSessionManager* self)
{
    s32 i;

    self->buffer = 0;
    for (i = 0; i < 21; i++) {
        self->requests_10[i] = 0;
        self->request_state_64[i] = 0;
    }
    for (i = 0; i < 2; i++) {
        fn_803D4A50(&self->pool_7C[i]);
    }
}

extern "C" void fn_803D4D20(NetworkSessionManager* self)
{
    NetworkBuffer* buf;
    s32 i;

    buf = self->buffer;
    if (buf != 0) {
        buf->vtable->end_14(buf);
        buf = self->buffer;
        if (buf != 0) {
            buf->vtable->destroy_08(buf, 1);
            self->buffer = 0;
        }
    }
    for (i = 0; i < 0x15; i++) {
        fn_803D64A4(self, &self->requests_10[i]);
    }
    for (i = 0; i < 2; i++) {
        fn_803D4B5C(&self->pool_7C[i]);
    }
}

#pragma dont_inline on
extern "C" s32 fn_803D5070(NetworkRequest* self)
{
    return self->owner_94 != 0;
}
#pragma dont_inline off

extern "C" void zz_03d5084_ptmf_scall(NetworkRequest* self)
{
    if (self->owner_94 != 0 && __ptmf_scall(self) != 0) {
        fn_803D4B5C(self);
    }
}

extern "C" void fn_803D5150(NetworkRequest* req, NetworkSessionManager* owner,
                            NetworkRequestDesc desc, u32 count, ...)
{
    u32 i;

    fn_803D4A50(req);
    req->timeout_50 = 0.0f;
    req->requestId_70 = NetworkRequest_idCounter;
    NetworkRequest_idCounter = req->requestId_70 + 1;
    req->owner_94 = owner;
    req->desc_98 = desc.id_0;
    req->desc_9C = desc.value_4;
    req->desc_A0 = desc.type_8;
    req->count_28 = count > 8 ? 8 : count;
    for (i = 0; i < req->count_28; i++) {
        req->args_2C[i] = 0;
    }
}

/* ----------------------------------------------------------------------------------------- */
/* NetworkSessionManager - the lazy request allocators                                        */
/* ----------------------------------------------------------------------------------------- */

extern "C" void fn_803D50D8(NetworkSessionManager* self)
{
    NetworkRequest* req;

    if (self->requests_10[1] == 0) {
        req = fn_803D6430(self);
        if (req != 0) {
            self->requests_10[1] = req;
            fn_803D5150(req, self, lbl_805FA7CC, 0);
        }
    }
}

extern "C" void fn_803D528C(NetworkSessionManager* self)
{
    NetworkRequest* req;

    if (self->requests_10[2] == 0) {
        req = fn_803D6430(self);
        if (req != 0) {
            self->requests_10[2] = req;
            fn_803D5150(req, self, lbl_805FA7D8, 0);
        }
    }
}

extern "C" s32 fn_803D5304(NetworkSessionManager* self)
{
    return self->buffer != 0;
}

extern "C" void fn_803D5318(NetworkSessionManager* self, u32 a, u32 b)
{
    NetworkRequest* req;

    if (self->requests_10[3] == 0) {
        req = fn_803D6430(self);
        if (req != 0) {
            self->requests_10[3] = req;
            fn_803D5150(req, self, lbl_805FA7E4, 2, a, b);
        }
    }
}

extern "C" void fn_803D53B0(NetworkSessionManager* self, u32 a)
{
    NetworkRequest* req;

    if (self->requests_10[4] == 0) {
        req = fn_803D6430(self);
        if (req != 0) {
            self->requests_10[4] = req;
            fn_803D5150(req, self, lbl_805FA7F0, 1, a);
        }
    }
}

extern "C" void fn_803D5438(NetworkSessionManager* self, u32 a)
{
    NetworkRequest* req;

    if (self->requests_10[5] == 0) {
        req = fn_803D6430(self);
        if (req != 0) {
            self->requests_10[5] = req;
            fn_803D5150(req, self, lbl_805FA7FC, 1, a);
        }
    }
}

extern "C" void fn_803D54C0(NetworkSessionManager* self, u32 a)
{
    NetworkRequest* req;

    if (self->requests_10[6] == 0) {
        req = fn_803D6430(self);
        if (req != 0) {
            self->requests_10[6] = req;
            fn_803D5150(req, self, lbl_805FA808, 1, a);
        }
    }
}

extern "C" void fn_803D5548(NetworkSessionManager* self, u32 a)
{
    NetworkRequest* req;

    if (self->requests_10[6] == 0) {
        req = fn_803D6430(self);
        if (req != 0) {
            self->requests_10[6] = req;
            fn_803D5150(req, self, lbl_805FA814, 1, a);
        }
    }
}

extern "C" void fn_803D55D0(NetworkSessionManager* self)
{
    NetworkRequest* req;

    if (self->requests_10[7] == 0) {
        req = fn_803D6430(self);
        if (req != 0) {
            self->requests_10[7] = req;
            fn_803D5150(req, self, lbl_805FA820, 0);
        }
    }
}

extern "C" void fn_803D5648(NetworkSessionManager* self, u32 a)
{
    NetworkRequest* req;

    if (self->requests_10[16] == 0) {
        req = fn_803D6430(self);
        if (req != 0) {
            self->requests_10[16] = req;
            fn_803D5150(req, self, lbl_805FA82C, 1, a);
        }
    }
}

extern "C" void fn_803D56D0(NetworkSessionManager* self, s8 a)
{
    NetworkRequest* req;

    if (self->requests_10[17] == 0) {
        req = fn_803D6430(self);
        if (req != 0) {
            self->requests_10[17] = req;
            fn_803D5150(req, self, lbl_805FA838, 1, (u32)a);
        }
    }
}

extern "C" void fn_803D5758(NetworkSessionManager* self)
{
    NetworkRequest* req;

    if (self->requests_10[20] == 0) {
        req = fn_803D6430(self);
        if (req != 0) {
            self->requests_10[20] = req;
            fn_803D5150(req, self, lbl_805FA844, 0);
        }
    }
}

extern "C" void fn_803D57D0(NetworkSessionManager* self, u32 a)
{
    NetworkRequest* req;

    if (self->requests_10[8] == 0) {
        req = fn_803D6430(self);
        if (req != 0) {
            self->requests_10[8] = req;
            fn_803D5150(req, self, lbl_805FA850, 1, a);
        }
    }
}

extern "C" void fn_803D5858(NetworkSessionManager* self)
{
    NetworkRequest* req;

    if (self->requests_10[9] == 0) {
        req = fn_803D6430(self);
        if (req != 0) {
            self->requests_10[9] = req;
            fn_803D5150(req, self, lbl_805FA85C, 0);
        }
    }
}

extern "C" void fn_803D58D0(NetworkSessionManager* self)
{
    NetworkRequest* req;

    if (self->requests_10[19] == 0) {
        req = fn_803D6430(self);
        if (req != 0) {
            self->requests_10[19] = req;
            fn_803D5150(req, self, lbl_805FA868, 0);
        }
    }
}

extern "C" void fn_803D5948(NetworkSessionManager* self)
{
    NetworkRequest* req;

    if (self->requests_10[10] == 0) {
        req = fn_803D6430(self);
        if (req != 0) {
            self->requests_10[10] = req;
            fn_803D5150(req, self, lbl_805FA874, 0);
        }
    }
}

extern "C" void fn_803D59C0(NetworkSessionManager* self)
{
    NetworkRequest* req;

    if (self->requests_10[11] == 0) {
        req = fn_803D6430(self);
        if (req != 0) {
            self->requests_10[11] = req;
            fn_803D5150(req, self, lbl_805FA880, 0);
        }
    }
}

extern "C" void fn_803D5A38(NetworkSessionManager* self, u32 a, u32 b, s8 c)
{
    NetworkRequest* req;

    if (self->requests_10[15] == 0) {
        req = fn_803D6430(self);
        if (req != 0) {
            self->requests_10[15] = req;
            fn_803D5150(req, self, lbl_805FA88C, 3, a, b, (u32)c);
        }
    }
}

extern "C" void fn_803D5AE0(NetworkSessionManager* self)
{
    NetworkRequest* req;

    if (self->requests_10[12] == 0) {
        req = fn_803D6430(self);
        if (req != 0) {
            self->requests_10[12] = req;
            fn_803D5150(req, self, lbl_805FA898, 0);
        }
    }
}

extern "C" void fn_803D5B58(NetworkSessionManager* self, u32 a, u32 b)
{
    NetworkRequest* req;

    if (self->requests_10[13] == 0) {
        req = fn_803D6430(self);
        if (req != 0) {
            self->requests_10[13] = req;
            fn_803D5150(req, self, lbl_805FA8A4, 2, a, b);
        }
    }
}

extern "C" void fn_803D5BF0(NetworkSessionManager* self, u32 a, u32 b)
{
    NetworkRequest* req;

    if (self->requests_10[14] == 0) {
        req = fn_803D6430(self);
        if (req != 0) {
            self->requests_10[14] = req;
            fn_803D5150(req, self, lbl_805FA8B0, 2, a, b);
        }
    }
}

extern "C" void fn_803D5C88(NetworkSessionManager* self, u32 a)
{
    NetworkRequest* req;

    if (self->requests_10[18] == 0) {
        req = fn_803D6430(self);
        if (req != 0) {
            self->requests_10[18] = req;
            fn_803D5150(req, self, lbl_805FA8BC, 1, a);
        }
    }
}

/* ----------------------------------------------------------------------------------------- */
/* NetworkSessionManager - accessors / small virtuals                                         */
/* ----------------------------------------------------------------------------------------- */

extern "C" void fn_803D5D10(NetworkSessionManager* self)
{
    NetworkRequest* req = self->requests_10[4];

    if (req != 0 && fn_803D5D64(req) == 0) {
        fn_803D5D58(req);
    }
}

#pragma dont_inline on
extern "C" void fn_803D5D58(NetworkRequest* self)
{
    self->cancelled_74 = 1;
}

extern "C" s32 fn_803D5D64(NetworkRequest* self)
{
    return self->cancelled_74;
}
#pragma dont_inline off

extern "C" void fn_803D5D6C(NetworkSessionManager* self)
{
    NetworkRequest* req = self->requests_10[14];

    if (req != 0 && fn_803D5D64(req) == 0) {
        fn_803D5D58(req);
    }
}

extern "C" void fn_803D5DB4(NetworkSessionManager* self, s8 value)
{
    self->unused_79 = value;
}

extern "C" void fn_803D5DBC(void* self, s32 value)
{
    fn_803CF66C(value);
}

extern "C" void fn_803D5DC4(NetworkSessionManager* self, s8 value)
{
    self->unused_7A = value;
}

extern "C" s32 fn_803D5DCC(NetworkSessionManager* self, s8 value)
{
    if (self->buffer == 0) {
        return 0;
    }
    return self->buffer->vtable->getInt_90(self->buffer, self->vtable->mapId_1C0(self, value));
}

extern "C" f32 fn_803D5E38(NetworkSessionManager* self, s8 value)
{
    if (self->buffer == 0) {
        return 0.0f;
    }
    return self->buffer->vtable->getFloat_8C(self->buffer, self->vtable->mapId_1C0(self, value));
}

extern "C" void fn_803D5EA4(NetworkSessionManager* self)
{
    s8 data[4];

    data[0] = 0;
    data[1] = 1;
    data[2] = 2;
    data[3] = 3;
    self->vtable->sendBatch_138(self, 4, data);
}

extern "C" void fn_803D5EF8(NetworkSessionManager* self, u32 a, u32 b, u8 c)
{
    s8 data;
    NetworkBuffer* buf;

    data = -1;
    buf = self->buffer;
    if (buf != 0) {
        buf->vtable->put_38(buf, a, b, 1, 1, &data, c);
    }
}

extern "C" void fn_803D5F4C(NetworkSessionManager* self, u32 a, u32 b)
{
    s8 data;
    NetworkBuffer* buf;

    data = -2;
    buf = self->buffer;
    if (buf != 0) {
        buf->vtable->put_38(buf, a, b, 0, 1, &data, 0xFF);
    }
}

extern "C" void fn_803D5F9C(NetworkSessionManager* self, u32 a, u32 b, u8 c)
{
    s8 data;
    NetworkBuffer* buf;

    data = -2;
    buf = self->buffer;
    if (buf != 0) {
        buf->vtable->put_38(buf, a, b, 1, 1, &data, c);
    }
}

extern "C" void fn_803D62D0(NetworkSessionManager* self)
{
    if (self->buffer != 0 && self->vtable->canSend_28(self) != 0) {
        self->buffer->vtable->flush_44(self->buffer);
    }
}

/* ----------------------------------------------------------------------------------------- */
/* NetworkSessionManager - pool + mutex helpers                                               */
/* ----------------------------------------------------------------------------------------- */

extern "C" NetworkRequest* fn_803D6430(NetworkSessionManager* self)
{
    s32 i;

    for (i = 0; i < 2; i++) {
        if (fn_803D5070(&self->pool_7C[i]) == 0) {
            return &self->pool_7C[i];
        }
    }
    return 0;
}

extern "C" void fn_803D64A4(NetworkSessionManager* self, NetworkRequest** slot)
{
    if (*slot != 0) {
        if (fn_803D5070(*slot) != 0) {
            DebugManager* log = fn_803C9974();
            log->vtable->log_14(log, NetworkSessionManager_deleteRequestMessage);
        }
        fn_803D4B5C(*slot);
    }
    *slot = 0;
}

extern "C" s32 fn_803D6514(NetworkRequest* self, u32* out)
{
    s32 result;

    result = 0;
    LockMutex(self->mutex_78);
    if (self->record_54 != 0) {
        result = 1;
        out[0] = self->record_54;
        out[1] = self->record_58;
        out[2] = self->record_5C;
    }
    UnlockMutex(self->mutex_78);
    return result;
}

extern "C" void fn_803D658C(NetworkRequest* self, u32 a, u32 b, u32 c)
{
    LockMutex(self->mutex_78);
    self->record_58 = b;
    self->record_5C = c;
    self->record_54 = a;
    UnlockMutex(self->mutex_78);
}

extern "C" s32 fn_803D65F4(NetworkRequest* self, u32 idx)
{
    u32 count;
    DebugManager* log;

    count = self->count_28;
    if (count <= idx) {
        log = fn_803C9974();
        log->vtable->warn_10(log, NetworkRequest_getArgumentMessage, count, idx);
        return 0;
    }
    return (s32)self->args_2C[idx];
}
