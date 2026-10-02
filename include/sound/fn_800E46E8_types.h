/* Types and macros the units cut from `sound/fn_800E46E8.cpp` share (hoisted at phase 4 so each is defined once). */
#ifndef MHTRI_SOUND_FN_800E46E8_TYPES_H
#define MHTRI_SOUND_FN_800E46E8_TYPES_H

#include "types.h"

/* The stream manager's per-record relocation state: the table header and its resolved entry array.
 * `entries` is `table + table->entries_offset` (a self-relative table, so the base has to be computed -
 * no field is being reached by the offset). */
typedef struct RelocTable {
    /* +0x00 */ u32 unused_0x00;
    /* +0x04 */ u32 count;
    /* +0x08 */ u32 entries_offset;
    /* +0x0C */ u32 unused_0x0C;
    /* +0x10 */ void* entries_16; /* the fixed-offset variant fn_800E7EA4/7EBC install */
} RelocTable; /* size: 0x14 (variable) */

typedef struct RelocState {
    /* +0x00 */ RelocTable* table;
    /* +0x04 */ void** entries;
} RelocState; /* size: 0x8 */

/* One stream record, stride 0x28 (`fn_800E4C90`). */
typedef struct StreamRec {
    /* +0x00 */ RelocState reloc[4]; /* fn_800E4E38/30/28/10 clear one each, fn_800E4C88/78/18/80 fill them */
    /* +0x20 */ u32 scalar_20;       /* fn_800E4E00 clears it */
    /* +0x24 */ u32 scalar_24;       /* fn_800E4E00 clears it, fn_800E4C0C writes it */
} StreamRec; /* size: 0x28 */

/* The global stream manager (`lbl_8069A810`). */
typedef struct StreamWork {
    /* +0x00 */ u32 unused_00;
    /* +0x04 */ u32 flags;          /* snd_bank_pending_ck / snd_bank_pending_clear / snd_bank_pending_set bit ops */
    /* +0x08 */ u8 unused_08[0x1C];
    /* +0x24 */ s16 cur_slot;       /* fn_800E4E70 writes it, fn_800E4E9C reads it */
    /* +0x26 */ s16 slot_tab[3];    /* fn_800E4EE8, bounded <3 by fn_800E4EA4 */
    /* +0x2C */ u8 unused_2C[0x30];
    /* +0x5C */ StreamRec recs[3];  /* fn_800E4C90 */
} StreamWork; /* size: 0xD8 */

/* The stream description `fn_800E4B4C` installs: four self-relative relocation-table offsets, each added
 * to the descriptor's own address to find the table. */
typedef struct RelocDesc {
    /* +0x00 */ u32 reloc_00;
    /* +0x04 */ u32 reloc_04;
    /* +0x08 */ u32 reloc_08;
    /* +0x0C */ u32 reloc_0C;
    /* +0x10 */ u32 reloc_10;
} RelocDesc; /* size: 0x14 */

/* One reverb unit: a callback slot, a state word, the "active" selector the shutdown path branches
 * on, and the 1000-byte effect body (`fn_800E522C` memsets exactly that much). */
typedef void (*ReverbSetFn)(void* fn, void* arg);

typedef struct ReverbHiData {
    /* +0x0000 */ u8 state_0000[0x178];
    /* +0x0178 */ f32 damping;
    /* +0x017C */ f32 mix;
    /* +0x0180 */ f32 time;
    /* +0x0184 */ f32 coloration;
    /* +0x0188 */ f32 pre_delay;
    /* +0x018C */ f32 crosstalk;
    /* +0x0190 */ u8 state_0190[0x258];
} ReverbHiData; /* size: 0x3E8 */

typedef struct ReverbFx {
    /* +0x0000 */ ReverbSetFn set_handler;
    /* +0x0004 */ u32 state;   /* 0..3, advanced by snd_reverb_state_advance */
    /* +0x0008 */ s32 active;  /* picks the shutdown routine in fn_800E523C/52A0/5304 */
    /* +0x000C */ ReverbHiData data;
} ReverbFx; /* size: 0x3F4 */

/* The manager: a mode word and three reverb units, plus the effect pointers that index them. */
typedef struct ReverbMgr {
    /* +0x0000 */ u32 mode;      /* snd_reverb_unit_set picks the effect class from it */
    /* +0x0004 */ ReverbFx fx[3];
    /* +0x0BE0 */ ReverbFx* effects[3];
    /* +0x0BEC */ u32 flags;     /* one pending bit per unit, snd_reverb_unit_set/49E0/4A00 */
} ReverbMgr; /* size: 0xBF0 */

/* The reverb parameters `snd_reverb_unit_set` copies into the live effect. */
typedef struct ReverbCfg {
    /* +0x00 */ u8 kind;        /* which of the three units */
    /* +0x01 */ u8 enable;      /* nonzero: mark the unit pending */
    /* +0x02 */ u8 unused_02[2];
    /* +0x04 */ f32 pre_delay;
    /* +0x08 */ f32 time;
    /* +0x0C */ f32 damping;
    /* +0x10 */ f32 coloration;
    /* +0x14 */ f32 mix;
    /* +0x18 */ f32 crosstalk;
} ReverbCfg; /* size: 0x1C */

/* One 8-byte part of a reverb delay line; `dtor_800E5548` walks nine of them. */
typedef struct ReverbPart {
    /* +0x00 */ u32 w[2];
} ReverbPart; /* size: 0x8 */

/* One 72-byte delay line: nine parts. */
typedef struct ReverbLine {
    /* +0x00 */ ReverbPart part[9];
} ReverbLine; /* size: 0x48 */

/* The vtable of the global sound object `lbl_80697980`; only slot 5 is reached from this unit. */
typedef struct SoundVtbl {
    /* +0x00 */ void (*method_00)(void);
    /* +0x04 */ void (*method_04)(void);
    /* +0x08 */ void (*method_08)(void);
    /* +0x0C */ void (*method_0C)(void);
    /* +0x10 */ void (*method_10)(void);
    /* +0x14 */ void (*method_14)(void* self, void* arg);
} SoundVtbl; /* size: 0x18 */

typedef struct Sound {
    /* +0x0000 */ const SoundVtbl* vtable;
    /* +0x0004 */ u32 unused_04;
    /* +0x0008 */ s16 field_08;      /* fn_800E8524 writes, fn_800E8550 reads */
    /* +0x000A */ s16 levels[49];    /* fn_800E85D8/8594 index it, 0x0A..0x6C */
    /* +0x006C */ u8 unused_6C[2];
    /* +0x006E */ u8 field_6E;
    /* +0x006F */ u8 field_6F;
    /* +0x0070 */ u8 field_70;
    /* +0x0071 */ u8 field_71;       /* fn_800E858C writes */
    /* +0x0072 */ u8 unused_72[2];
    /* +0x0074 */ u32 field_74;      /* fn_800E6764 divides by it */
    /* +0x0078 */ u32 field_78;
    /* +0x007C */ u32 levels2[3];    /* fn_800E84DC indexes it */
    /* +0x0088 */ f32 field_88;
    /* +0x008C */ f32 field_8C;
    /* +0x0090 */ ReverbLine lines[49];
} Sound; /* size: 0xE58 (the map's object is 0xE60) */

/* The vtable of the object fn_800E5F40 dispatches from; only slots 6..8 are reached here. */
typedef struct Sound2Vtbl {
    /* +0x00 */ void (*method_00)(void);
    /* +0x04 */ void (*method_04)(void);
    /* +0x08 */ void (*method_08)(void);
    /* +0x0C */ void (*method_0C)(void);
    /* +0x10 */ void (*method_10)(void);
    /* +0x14 */ void (*method_14)(void);
    /* +0x18 */ void (*method_18)(void* self, void* arg);
    /* +0x1C */ void (*method_1C)(void* self, void* arg);
    /* +0x20 */ void (*method_20)(void* self, void* arg);
} Sound2Vtbl; /* size: 0x24 */

typedef struct Sound2 {
    /* +0x00 */ const Sound2Vtbl* vtable;
} Sound2; /* size: 0x4 */

/* A group of relocation states, stride 8 (`fn_800E7E94`..`7FBC` each fill one). */
typedef struct RelocGroup {
    /* +0x00 */ RelocState state[9];
} RelocGroup; /* size: 0x48 */
#endif /* MHTRI_SOUND_FN_800E46E8_TYPES_H */
