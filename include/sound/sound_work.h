/*
 * The `sound` band's shared types, globals and cross-unit prototypes, as used by
 * `sound/fn_800E8E60.cpp`.
 *
 * The types below describe the sound runtime the preceding TU `sound/fn_800E46E8.cpp` first reconstructed;
 * the second unit to need them (this one) is where they move to a header (docs/plan.md 6.5 rule 1).  The
 * fields this unit reaches that the first unit left as padding are named here, their offsets unchanged.
 *
 * The declarations are the band's and the SDK's symbols this unit calls.  They live here rather than in the
 * consumer's `src/` so the unit's own file makes no `extern` declaration for a symbol another unit owns
 * (docs/plan.md 6.5 rule 2).  A follow-up should make `sound/fn_800E46E8.cpp` include this file in place of
 * its own copies (a `shared-file` request is in the outbox).
 */
#ifndef MHTRI_SOUND_WORK_H
#define MHTRI_SOUND_WORK_H

#include "types.h"

/* A node of the sound runtime's circular doubly-linked lists.  `fn_800E9AE4` strides a range of them by 76
 * and `fn_800E9C08`/`fn_800E60F8` return `this + 76`, so a node is 0x4C bytes.  `+0x00` is the vtable slot
 * the pool constructors install (fn_800E9CB0/fn_800E9C74); +0x04/+0x08 are the links. */
typedef struct SndListNode {
    /* +0x00 */ const void* vtable;
    /* +0x04 */ struct SndListNode* next;
    /* +0x08 */ struct SndListNode* prev;
    /* +0x0C */ u8 pad_0x0C[0x40];
} SndListNode; /* size: 0x4C */

/* The 96-node pool `lbl_80698AF0` (`fn_800E9C1C` constructs it): a free list, an active list, the node
 * storage and the "initialised" flag right after it.  `fn_800E9960` tests the flag at +0x1D18. */
typedef struct SndPool {
    /* +0x0000 */ SndListNode free_list;
    /* +0x004C */ SndListNode active_list;
    /* +0x0098 */ SndListNode nodes[96];
    /* +0x1D18 */ u8 initialised;
} SndPool; /* size: 0x1D19 */

/* A 0x110-byte pool node (the second pool, `fn_800ED8DC`/`fn_800EDF1C`). */
typedef struct SndNode2 {
    /* +0x000 */ const void* vtable;
    /* +0x004 */ struct SndNode2* next;
    /* +0x008 */ struct SndNode2* prev;
    /* +0x00C */ u32 field_0x0C;
    /* +0x010 */ u32 field_0x10;
    /* +0x014 */ u32 field_0x14;
    /* +0x018 */ u8 field_0x18;
    /* +0x019 */ u8 field_0x19;
    /* +0x01A */ u8 pad_0x1A[0xF6];
} SndNode2; /* size: 0x110 */

/* The 96-node second pool (`fn_800EDF1C` constructs it). */
typedef struct SndPool2 {
    /* +0x0000 */ SndNode2 free_list;
    /* +0x0110 */ SndNode2 active_list;
    /* +0x0220 */ SndNode2 nodes[96];
    /* +0x6820 */ u8 initialised;
} SndPool2; /* size: 0x6821 */

/* A reference slot and its target: `fn_800E960C` reads `value` of `*slot`. */
typedef struct SndRef {
    /* +0x00 */ u32 unused_0x00;
    /* +0x04 */ u32 value;
} SndRef; /* size: 0x8 */

/* The five reference slots `fn_800E960C` selects between. */
typedef struct SndRefTable {
    /* +0x00 */ SndRef* ref_00;
    /* +0x04 */ u8 pad_0x04[4];
    /* +0x08 */ SndRef* ref_08;
    /* +0x0C */ u8 pad_0x0C[4];
    /* +0x10 */ SndRef* ref_10;
    /* +0x14 */ u8 pad_0x14[4];
    /* +0x18 */ SndRef* ref_18;
    /* +0x1C */ u8 pad_0x1C[0x24];
    /* +0x40 */ SndRef* ref_40;
} SndRefTable; /* size: 0x44 */

/* The object fn_800E96BC dispatches loop handling through; slots 5, 6 and 9 are reached here. */
typedef struct Sound2Vtbl {
    /* +0x00 */ void (*method_00)(void);
    /* +0x04 */ void (*method_04)(void);
    /* +0x08 */ void (*method_08)(void);
    /* +0x0C */ void (*method_0C)(void);
    /* +0x10 */ void (*method_10)(void);
    /* +0x14 */ void (*method_14)(void* self, void* arg);
    /* +0x18 */ void (*method_18)(void* self, void* arg);
    /* +0x1C */ void (*method_1C)(void* self, void* arg);
    /* +0x20 */ void (*method_20)(void* self, void* arg);
    /* +0x24 */ void (*method_24)(void* self, void* arg);
} Sound2Vtbl; /* size: 0x28 */

typedef struct Sound2 {
    /* +0x00 */ const Sound2Vtbl* vtable;
} Sound2; /* size: 0x4 */

/* The AX voice block of a sound slot (`fn_800E5F38` returns it).  The first unit left the loop machine's
 * value and per-state steps as padding; they are named here. */
typedef struct AxVoiceBlock {
    /* +0x00 */ u8 unused_00[2];
    /* +0x02 */ s16 field_02;   /* pan */
    /* +0x04 */ s16 loop_value; /* the value fn_800E96BC ramps */
    /* +0x06 */ s16 field_06;
    /* +0x08 */ u8 unused_08[8];
    /* +0x10 */ s16 field_10;
    /* +0x12 */ s16 field_12;
    /* +0x14 */ s16 field_14;
    /* +0x16 */ s16 field_16;
    /* +0x18 */ s16 field_18;
    /* +0x1A */ u16 field_1A;   /* pitch */
    /* +0x1C */ u8 unused_1C[2];
    /* +0x1E */ u8 field_1E;    /* pitch-args initialised */
    /* +0x20 */ u32 field_20;   /* loop length, (u32)-1 when unset */
    /* +0x24 */ u32 field_24;   /* release countdown, (u32)-1 when unset */
    /* +0x28 */ u8 unused_28[4];
    /* +0x2C */ u32 field_2C;   /* loop position */
    /* +0x30 */ s16 loop_rise_step;    /* state 1 adds it to loop_value */
    /* +0x32 */ s16 loop_fall_base;    /* state 2 computes loop_fall_base - loop_value */
    /* +0x34 */ s16 loop_fall_min;     /* state 2 clamps loop_value at it */
    /* +0x36 */ s16 loop_hold_step;    /* state 3 adds it to loop_value */
    /* +0x38 */ s16 loop_release_step; /* state 5 subtracts it from loop_value */
    /* +0x3A */ u8 field_3A;    /* loop state, set to 5 when it wraps */
    /* +0x3B */ u8 unused_3B[5];
    /* +0x40 */ struct AxHandle* field_40;
    /* +0x44 */ struct AxVoice* voice;
    /* +0x48 */ u8 unused_48[2];
    /* +0x4A */ s16 field_4A;
    /* +0x4C */ s16 field_4C;   /* pitch upper limit */
    /* +0x4E */ s16 field_4E;   /* pitch lower limit (negated) */
    /* +0x50 */ u8 unused_50[0x0A];
    /* +0x5A */ s16 field_5A;
} AxVoiceBlock; /* size: 0x5C */

/* One sound slot: a flag word, the owner pointer and the AX voice block. */
typedef struct SoundSlot {
    /* +0x00 */ u32 unused_00;
    /* +0x04 */ u32 unused_04;
    /* +0x08 */ u32 unused_08;
    /* +0x0C */ u32 flags;
    /* +0x10 */ u32 unused_10;
    /* +0x14 */ void* unused_14;
    /* +0x18 */ u8 unused_18[4];
    /* +0x1C */ AxVoiceBlock ax;
} SoundSlot; /* size: 0x64 */

/* The AX handle `fn_800E8E60` reads a word from. */
typedef struct AxHandle {
    /* +0x00 */ u32 unused_00;
    /* +0x04 */ u32 field_04;
} AxHandle; /* size: 0x8 */

/* The AX voice a slot holds. */
typedef struct AxVoice {
    /* +0x00 */ u8 unused_00[0x38];
    /* +0x38 */ u16 field_38;
} AxVoice; /* size: 0x3A */

/* A relocation state, stride 8: a table and its resolved entry array. */
typedef struct RelocState {
    /* +0x00 */ void* table;
    /* +0x04 */ void** entries;
} RelocState; /* size: 0x8 */

/* A block with a RelocState at +0x08 (`fn_800EC93C`). */
typedef struct SndRelocHost {
    /* +0x00 */ u8 pad_0x00[8];
    /* +0x08 */ RelocState state;
} SndRelocHost; /* size: 0x10 */

/* Two untyped words. */
typedef struct Pair32 {
    /* +0x00 */ u32 a;
    /* +0x04 */ u32 b;
} Pair32; /* size: 0x8 */

/* A word array with its count (`fn_800EBCF4`). */
typedef struct SndU32Table {
    /* +0x00 */ u8 pad_0x00[0x20];
    /* +0x20 */ u32* items;
    /* +0x24 */ u32 count;
} SndU32Table; /* size: 0x28 */

/* A counted header and its item array; `fn_800EEC344`/`fn_800EBD18`/`fn_800EC380`/`fn_800E95C4` share the
 * shape (the header's `count` at +0x04 bounds the index). */
typedef struct SndCounted {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ u32 count;
} SndCounted; /* size: 0x8 */

typedef struct SndLookup {
    /* +0x00 */ SndCounted* header;
    /* +0x04 */ u32* items;
} SndLookup; /* size: 0x8 */

/* The lookup hosts: `fn_800EC344` uses +0x00, `fn_800EBD18` +0x10, `fn_800EC380`/`fn_800E95C4` +0x18. */
typedef struct SndLookupHost {
    /* +0x00 */ SndLookup at_0x00;
    /* +0x08 */ u8 pad_0x08[8];
    /* +0x10 */ SndLookup at_0x10;
    /* +0x18 */ SndLookup at_0x18;
    /* +0x20 */ u8 pad_0x20[0x24];
} SndLookupHost; /* size: 0x44 */

/* A bit-mask table of items (`fn_800EBA48` allocates one, `fn_800EBAA0` frees one). */
typedef struct SndBitTable {
    /* +0x00 */ u8 pad_0x00[8];
    /* +0x08 */ u32 count;
    /* +0x0C */ u32 mask;
    /* +0x10 */ u32 field_0x10;
    /* +0x14 */ u32 items[1];
} SndBitTable; /* size: 0x18 (variable) */

/* The five-source copy `fn_800EE524` performs. */
typedef struct SndCopy28 {
    /* +0x00 */ Pair32 p[4];
    /* +0x20 */ u16 h[4];
} SndCopy28; /* size: 0x28 */

/* The four-Pair32 record `fn_800EBD58` copies. */
typedef struct SndBdRec {
    /* +0x00 */ Pair32 p_0x00;
    /* +0x08 */ Pair32 p_0x08;
    /* +0x10 */ Pair32 p_0x10;
    /* +0x18 */ Pair32 p_0x18;
    /* +0x20 */ u32 field_0x20;
    /* +0x24 */ u32 field_0x24;
} SndBdRec; /* size: 0x28 */

/* The four-relocation-state record `fn_800ED72C` constructs. */
typedef struct SndEdRec {
    /* +0x00 */ RelocState states[4];
    /* +0x20 */ u32 field_0x20;
    /* +0x24 */ u32 field_0x24;
} SndEdRec; /* size: 0x28 */

/* The host `dtor_800ED780` destroys: a record at +0 and another at +0x110. */
typedef struct SndEdHost {
    /* +0x000 */ SndEdRec base_0x000;
    /* +0x028 */ u8 pad_0x028[0xE8];
    /* +0x110 */ SndEdRec member_0x110;
} SndEdHost; /* size: 0x138 */

/* The voice-manager block `fn_800EDB24`/`fn_800EDB50` walk: a voice list at +0x110. */
typedef struct SndVoiceMgr {
    /* +0x0000 */ u8 pad_0x00[0x110];
    /* +0x0110 */ SndListNode voice_list;
} SndVoiceMgr; /* size: 0x15C */

/* The part fn_800EA7DC points at (`self + 0x1C`), with the fields `fn_800EAB18` writes. */
typedef struct SndAbVoice {
    /* +0x000 */ u8 pad_0x000[0xAB];
    /* +0x0AB */ u8 enabled_0xAB;
    /* +0x0AC */ u8 pad_0x0AC[0x28];
    /* +0x0D4 */ u16 field_0xD4;
    /* +0x0D6 */ u8 field_0xD6;
} SndAbVoice; /* size: 0xD7 */

/* The host `fn_800EA7DC` reaches through `fn_800EAB18`'s second argument. */
typedef struct SndAbVoiceHost {
    /* +0x00 */ u8 pad_0x00[0x1C];
    /* +0x1C */ SndAbVoice voice;
} SndAbVoiceHost; /* size: 0xF3 */

/* The block whose +0x2C holds a voice-host pointer (`fn_800EAB18`). */
typedef struct SndAbHost {
    /* +0x00 */ u8 pad_0x00[0x2C];
    /* +0x2C */ SndAbVoiceHost* voice;
} SndAbHost; /* size: 0x30 */

/* A block with a mode word at +0x04 (`fn_800EE868`/`fn_800EE8B4`). */
typedef struct SndModeWord {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ u32 mode;
} SndModeWord; /* size: 0x8 */

/* The three half-words `fn_800EE508` copies. */
typedef struct SndTri16 {
    /* +0x00 */ u16 a;
    /* +0x02 */ u16 b;
    /* +0x04 */ u16 c;
} SndTri16; /* size: 0x6 */

/* --- globals owned elsewhere in the sound band ------------------------------------------------ */

extern SndPool lbl_80698AF0;      /* the 0x4C pool, constructed by fn_800E9C1C/fn_800E9C10 */
extern SndPool2 lbl_8069A8E8;     /* the 0x110 pool, constructed by fn_800EDF1C/fn_800EDF10 */
extern u8 lbl_8069A810[0xD8];     /* the stream manager (fn_800E46E8.cpp's StreamWork) */
extern u16 lbl_80791468;          /* the 16-bit PRNG seed fn_800E9D00 advances */
extern u32 lbl_80794A20;          /* the expand heap fn_800EEA2C frees into */
extern f32 lbl_807964E8;          /* fn_800E8F78's clamp */
extern f32 lbl_807964EC;          /* 1.0f: the ratio denominator */
extern f32 lbl_80796514;          /* fn_800E9324's scale */
extern f32 lbl_80796518;          /* fn_800E9390's numerator */
extern f32 lbl_8079651C;          /* fn_800E9390's scale */
extern u8 lbl_80598020[0xC];      /* a node vtable (fn_800E9C74 installs it) */
extern u8 lbl_8059802C[0xC];      /* a node base vtable (fn_800E9CB0 installs it) */
extern u8 lbl_80598090[0xC];      /* the 0x110-node derived vtable (fn_800EDF74 installs it) */
extern u8 lbl_8059809C[0xC];      /* the 0x110-node base vtable (fn_800EDFC4 installs it) */
extern u8 lbl_806A1110[0x10];     /* the expand-heap allocator fn_800EE9EC allocates from */

/* --- band and SDK functions this unit calls --------------------------------------------------- */

#ifdef __cplusplus
extern "C" {
#endif

void* fn_800E4A18(void);
void* fn_800E7578(RelocState* state, u32 idx);
void* fn_800E614C(void* node);
void* fn_800E7264(void* node);
void* fn_800E7CA4(void* state);
void* dtor_800E54A8(void* obj, s32 flags);
void* fn_800E5690(void* obj, s32 flags);
void fn_804C2380(void* p, s32 kind);
void* MEMAllocFromAllocator(void* allocator, u32 size);
void MEMFreeToExpHeap(void* heap, void* ptr);
void __construct_array(void* array, void* ctor, void* dtor, int size, int count);
void __destroy_arr(void* array, void* dtor, int size, int count);

/* Owned by sound/fn_800E8E60.cpp (rule 2); its first caller outside the owner is
 * sound/fn_800EF7D8.cpp. */
void fn_800EEA44(u32 ctx, u32 chunk);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_WORK_H */
