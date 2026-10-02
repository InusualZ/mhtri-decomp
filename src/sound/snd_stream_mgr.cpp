/* sound/snd_stream_mgr.cpp - the stream manager and its slot routines
 *
 * `.text` 0x800E9D00..0x800ED780, 17 functions written (the rest of the range is not decompiled yet).
 * Phase 4 (docs/splits/phase4): recut registered unit, built from `sound/fn_800E8E60.cpp`.
 * Name is a GUESS: the unit owns the stream manager `lbl_8069A810` (0xD8 B `.bss`).
 * Each function keeps the `#pragma` state it had in its retired source.
 */

#include "types.h"
#include "sound/sound_work.h"

extern "C" void fn_800E9E54(void* arg);
extern "C" SndAbVoice* fn_800EA7DC(SndAbVoiceHost* self);
extern "C" void fn_800EAB18(void* self, SndAbHost* host);
extern "C" void* fn_800EBA48(SndBitTable* self);
extern "C" void fn_800EBAA0(SndBitTable* self, u32 item);
extern "C" u32 fn_800EBD18(SndLookupHost* self, u32 idx);
extern "C" void fn_800EBD58(SndBdRec* dst, SndBdRec* src);
extern "C" u32 fn_800EC344(SndLookupHost* self, u32 idx);
extern "C" u32 fn_800EC380(SndLookupHost* self, u32 idx);
extern "C" SndEdRec* fn_800ED72C(SndEdRec* self);

extern "C" void fn_800ED6A4(void* obj);
extern "C" void* dtor_800E9DD0(void* self, s16 flags);

/* A host with four destructed sub-objects at +0, +8, +16 and +24. */
typedef struct SndQuadHost {
    /* +0x00 */ u8 member_0x00[8];
    /* +0x08 */ u8 member_0x08[8];
    /* +0x10 */ u8 member_0x10[8];
    /* +0x18 */ u8 member_0x18[8];
} SndQuadHost; /* size: 0x20 */

/* A host with a three-element array at +0x5C. */
typedef struct SndArrHost {
    /* +0x00 */ u8 pad_0x00[0x5C];
    /* +0x5C */ u8 array_0x5C[3 * 40];
} SndArrHost; /* size: 0xD4 */

#pragma peephole off

/* Rounds `a` up to the next multiple of `b` (zero when `b` is zero). */
extern "C" u32 fn_800EA7B8(u32 a, u32 b)
{
    return b != 0 ? (a + b - 1) / b * b : 0;
}

/* The block at +0x1C of `self` (the voice state fn_800EAB18 writes). */
extern "C" SndAbVoice* fn_800EA7DC(SndAbVoiceHost* self)
{
    return &self->voice;
}

/* Returns item `idx` of the table, or zero when it is out of range. */
extern "C" u32 fn_800EBCF4(SndU32Table* self, u32 idx)
{
    if (self->count <= idx) {
        return 0;
    }
    return self->items[idx];
}

/* Copies one two-word record. */
extern "C" void fn_800EBDC8(Pair32* dst, Pair32* src)
{
    *dst = *src;
}

/* The relocation state at +0x08 of `self`. */
extern "C" void* fn_800EC93C(SndRelocHost* self, u32 idx)
{
    return fn_800E7578(&self->state, idx);
}

/* The stream manager's constructor entry point. */
extern "C" void fn_800ED698(void)
{
    fn_800ED6A4(lbl_8069A810);
}

/* Hands a parameter to the stream work's virtual method 5. */
extern "C" void fn_800E9E54(void* arg)
{
    Sound2* obj = (Sound2*)fn_800E4A18();

    obj->vtable->method_14(obj, arg);
}

/* Enables the voice and resets its two counters. */
extern "C" void fn_800EAB18(void* self, SndAbHost* host)
{
    SndAbVoice* voice = fn_800EA7DC(host->voice);

    voice->enabled_0xAB = 1;
    voice->field_0xD4 = 0xFFFF;
    voice->field_0xD6 = 0;
}

/* Returns the first free item of the bit table and marks it used. */
extern "C" void* fn_800EBA48(SndBitTable* self)
{
    u32 i;
    u32 count = self->count;

    for (i = 0; i < count; i++) {
        if ((self->mask & (1u << i)) == 0) {
            u32 item = self->items[i];

            self->mask |= (1u << i);
            return (void*)item;
        }
    }
    return 0;
}

/* Releases the bit of the item equal to `item`. */
extern "C" void fn_800EBAA0(SndBitTable* self, u32 item)
{
    u32 i;

    for (i = 0; i < self->count; i++) {
        if (self->items[i] == item) {
            self->mask &= ~(1u << i);
        }
    }
}

/* Returns item `idx` of the lookup at +0x10. */
extern "C" u32 fn_800EBD18(SndLookupHost* self, u32 idx)
{
    SndLookup* l = &self->at_0x10;

    if (l->header == 0) {
        return 0;
    }
    if (l->header->count <= idx) {
        return 0;
    }
    return l->items[idx];
}

/* Copies one 0x28-byte record (four pairs then two words). */
extern "C" void fn_800EBD58(SndBdRec* dst, SndBdRec* src)
{
    fn_800EBDC8(&dst->p_0x00, &src->p_0x00);
    fn_800EBDC8(&dst->p_0x08, &src->p_0x08);
    fn_800EBDC8(&dst->p_0x10, &src->p_0x10);
    fn_800EBDC8(&dst->p_0x18, &src->p_0x18);
    dst->field_0x20 = src->field_0x20;
    dst->field_0x24 = src->field_0x24;
}

/* Returns item `idx` of the lookup at +0x00. */
extern "C" u32 fn_800EC344(SndLookupHost* self, u32 idx)
{
    SndLookup* l = &self->at_0x00;

    if (l->header == 0) {
        return 0;
    }
    if (l->header->count <= idx) {
        return 0;
    }
    return l->items[idx];
}

/* Returns item `idx` of the lookup at +0x18. */
extern "C" u32 fn_800EC380(SndLookupHost* self, u32 idx)
{
    SndLookup* l = &self->at_0x18;

    if (l->header == 0) {
        return 0;
    }
    if (l->header->count <= idx) {
        return 0;
    }
    return l->items[idx];
}

/* Constructs the four relocation states and clears the two trailing words. */
extern "C" SndEdRec* fn_800ED72C(SndEdRec* self)
{
    fn_800E7CA4(&self->states[0]);
    fn_800E7CA4(&self->states[1]);
    fn_800E7CA4(&self->states[2]);
    fn_800E7CA4(&self->states[3]);
    self->field_0x20 = 0;
    self->field_0x24 = 0;
    return self;
}

/* Destroys a three-element array of 40-byte records. */
extern "C" void* fn_800E9D58(SndArrHost* self, s16 flags)
{
    if (self != 0) {
        __destroy_arr(&self->array_0x5C, (void*)dtor_800E9DD0, 40, 3);
        dtor_800E54A8(self, 0);
        if (flags > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* Destroys the four sub-objects of the record. */
extern "C" void* dtor_800E9DD0(void* self, s16 flags)
{
    if (self != 0) {
        fn_800E5690(&((SndQuadHost*)self)->member_0x18, -1);
        fn_800E5690(&((SndQuadHost*)self)->member_0x10, -1);
        fn_800E5690(&((SndQuadHost*)self)->member_0x08, -1);
        fn_800E5690(&((SndQuadHost*)self)->member_0x00, -1);
        if (flags > 0) {
            operator delete(self);
        }
    }
    return self;
}
