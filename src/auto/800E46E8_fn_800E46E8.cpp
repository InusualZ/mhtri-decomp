/* auto/800E46E8_fn_800E46E8.cpp - the sound/stream runtime: `fn_800E46E8`, `.text` 0x800E46E8-0x800E8E60.
 *
 * 196 functions in one C++ TU.  Two globals carry the state:
 *   * the stream manager `lbl_8069A810` (0xD8 B, `.bss`): a flag word at +0x04, the current slot index at
 *     +0x24 with its three-entry table at +0x26, and three per-stream records at +0x5C, stride 0x28
 *     (`fn_800E4C90` is `base + i*0x28 + 0x5C`, `fn_800E4EA4` bounds the index at 3).  Each record holds
 *     four relocation-table states (`table` + resolved `entries`) at +0x00/+0x08/+0x10/+0x18 and two
 *     scalars at +0x20/+0x24; `fn_800E4C1C` walks one state's table and adds the table base to every entry
 *     whose top bit is clear (`fn_800E7578` is the read side, `fn_800E7E94`..`7FBC` the per-offset
 *     installers).
 *   * the sound object `lbl_80697980` (0xE60 B): a vtable, the level table `levels[49]` at +0x0A, three
 *     level words at +0x7C, two floats at +0x88/+0x8C and the 49 72-byte reverb delay lines at +0x90.
 *     `fn_800E7ABC` is its constructor, `fn_800E7C34`/`dtor_800E5548` the line constructor/destructor.
 *     The reverb units themselves live in `lbl_80696D90` (`ReverbMgr`): three `ReverbFx` (0x3F4 B each)
 *     with a 1000-byte effect body, their `effects[3]` pointers at +0xBE0 and a pending-bit word at +0xBEC.
 *
 * The volume block is a separate `.sbss` run (0x807949B0..0x807949D8): per-slot "is bgm" flags, a
 * `{cur, prev}` pair for bgm and one for se, and the set-point each is driven from.
 *
 * Flags: `auto` / `Wii/1.3` / `cflags_main` (`-O3 -inline noauto`), but retail's codegen is
 * **peephole-off** throughout - the tell is the unfused `clrlwi r0,r3,24` before a `u8` use
 * (`set_stream_main_vol_flag`, `fn_800E48E4`, `fn_800E4908`, the `fn_800E46E8` selector) and the
 * unfused `slwi`/`addi r0` elsewhere, which the peephole removes.  The whole-file
 * `#pragma peephole off` below carries that; the outbox asks for a per-region `-opt nopeephole` cflags
 * group so the pragma can go.
 *
 * Linkage: this is a C++ file whose map names are a mix - the named entries (`set_stream_main_vol_flag__
 * FUcUc`, `PlayStream__FUlUl`, ...) are the *mangled* names of C++ functions and are written as such,
 * while every `fn_XXXXXXXX` entry has an unmangled map name, so those definitions and declarations carry
 * `extern "C"` (main.cpp's header records the same convention).
 *
 * Residuals (measured with `python build/tmp/unitreport.py`, the report metric):
 *   * 11 functions are still unwritten (0 %): `fn_800E5720` (1608 B), `fn_800E6314` (728), `fn_800E6A58`
 *     (520), `fn_800E6818` (512), `fn_800E6D8C` (496), `fn_800E7078` (492), `fn_800E8B1C` (428),
 *     `fn_800E8888` (336), `fn_800E89F8` (292), `fn_800E6764` (180) and `fn_800E7EA4` (24 B, 66 %).
 *   * `fn_800E7EA4`/`fn_800E7EBC`/`fn_800E77DC`/`fn_800E6A18`/`fn_800E6FB8`/`fn_800E6FF8`/`fn_800E7038`/
 *     `fn_800E4FB4` are the "materialise the state pointer, then a redundant branch" shape: retail emits
 *     `addi r3,r3,OFF` / `b` where ours folds `OFF` into the displacement (24-64 B, 66-93 %).  Neither
 *     `&block->state[i]`, `block->state + i`, a `for(;;) { ...; break; }`, a `while` nor a `do/while(0)`
 *     reproduces it, and marking `fn_800E7578` `inline` (so the wrappers inline it) does not either - it
 *     only loses `fn_800E4FB4`'s 87 %.  The logic itself is byte-identical.
 *   * `fn_800E46E8`'s jump table is `jumptable_80597D88`, which lives in the unit's **unclaimed** `.data`
 *     range (0x80597D88), so our object emits its own local `@NN` table; the score is unaffected
 *     (reloc-only) and the claim belongs to the measured data pass.
 *
 * Inventory, addresses and sizes: `python tools/units/ledger.py unit auto/800E46E8_fn_800E46E8.cpp`.
 * The name is provisional (`auto/` plus the first symbol's address) because nothing in the object names
 * the original source file.
 */

#include "types.h"

#pragma peephole off

/* A 4x4 matrix, 0x40 B, the form the GX projection path exchanges.  Only copied here, so it is an
 * opaque word block. */
typedef struct Mtx44 {
    /* +0x00 */ u32 w[16];
} Mtx44; /* size: 0x40 */

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

/* The flag word fn_800E4A00/49E0/4ABC reach at +4 (both the stream manager and the sound object
 * carry it there). */
typedef struct FlagWord {
    /* +0x00 */ u32 unused_00;
    /* +0x04 */ u32 flags;
} FlagWord; /* size: 0x8 */

/* The global stream manager (`lbl_8069A810`). */
typedef struct StreamWork {
    /* +0x00 */ u32 unused_00;
    /* +0x04 */ u32 flags;          /* fn_800E4A00 / fn_800E49E0 / fn_800E4ABC bit ops */
    /* +0x08 */ u8 unused_08[0x1C];
    /* +0x24 */ s16 cur_slot;       /* fn_800E4E70 writes it, fn_800E4E9C reads it */
    /* +0x26 */ s16 slot_tab[3];    /* fn_800E4EE8, bounded <3 by fn_800E4EA4 */
    /* +0x2C */ u8 unused_2C[0x30];
    /* +0x5C */ StreamRec recs[3];  /* fn_800E4C90 */
} StreamWork; /* size: 0xD8 */

/* A volume that is smoothed from `prev` to `cur`. */
typedef struct StreamVol {
    /* +0x00 */ f32 cur;
    /* +0x04 */ f32 prev;
} StreamVol; /* size: 0x8 */

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

/* The same body for the second effect class: its live parameters sit 0x30 lower. */
typedef struct ReverbStdData {
    /* +0x0000 */ u8 state_0000[0x148];
    /* +0x0148 */ f32 damping;
    /* +0x014C */ f32 mix;
    /* +0x0150 */ f32 time;
    /* +0x0154 */ f32 coloration;
    /* +0x0158 */ f32 pre_delay;
    /* +0x015C */ f32 crosstalk;
    /* +0x0160 */ u8 state_0160[0x288];
} ReverbStdData; /* size: 0x3E8 */

typedef struct ReverbFx {
    /* +0x0000 */ ReverbSetFn set_handler;
    /* +0x0004 */ u32 state;   /* 0..3, advanced by fn_800E51AC */
    /* +0x0008 */ s32 active;  /* picks the shutdown routine in fn_800E523C/52A0/5304 */
    /* +0x000C */ ReverbHiData data;
} ReverbFx; /* size: 0x3F4 */

/* The manager: a mode word and three reverb units, plus the effect pointers that index them. */
typedef struct ReverbMgr {
    /* +0x0000 */ u32 mode;      /* fn_800E4FF4 picks the effect class from it */
    /* +0x0004 */ ReverbFx fx[3];
    /* +0x0BE0 */ ReverbFx* effects[3];
    /* +0x0BEC */ u32 flags;     /* one pending bit per unit, fn_800E4FF4/49E0/4A00 */
} ReverbMgr; /* size: 0xBF0 */

/* The reverb parameters `fn_800E4FF4` copies into the live effect. */
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

/* The 49-line bank `fn_800E5430` destroys at +0x90. */
typedef struct ReverbBank {
    /* +0x000 */ u8 unused_000[0x90];
    /* +0x090 */ ReverbLine lines[49];
} ReverbBank; /* size: 0xE58 */

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

/* One slot-list entry's info block (fn_800E6154 returns entry+12). */
typedef struct SlotEntry {
    /* +0x00 */ u32 kind;      /* fn_800E5F40 dispatches the voice handler on it */
    /* +0x04 */ u32 owner;     /* compared against the voice's owner */
    /* +0x08 */ u8 field_08;
    /* +0x09 */ u8 field_09;
    /* +0x0A */ u8 field_0A;
    /* +0x0B */ u8 field_0B;
    /* +0x0C */ s16 field_0C;  /* fn_800E781C's third argument */
    /* +0x0E */ s16 field_0E;  /* fn_800E7878's third argument */
} SlotEntry; /* size: 0x10 */

/* One voice of the manager's voice list (fn_800EDB24 walks it, fn_800E614C returns the next). */
typedef struct VoiceEntry {
    /* +0x00 */ u32 kind;         /* 0..4, fn_800E5F40 */
    /* +0x04 */ struct VoiceEntry* next; /* fn_800E614C */
    /* +0x08 */ u8 unused_08[8];
    /* +0x10 */ u32 owner;        /* compared against the slot's owner */
    /* +0x14 */ u32 key;          /* compared against the config's key */
    /* +0x18 */ u8 unused_18[1];
    /* +0x19 */ u8 active;        /* must be 1 for the entry to be handled */
    /* +0x1A */ u8 unused_1A[6];
} VoiceEntry; /* size: 0x20 (approximate) */

/* The AX voice a slot holds; only the one status half-word is read here. */
typedef struct AxVoice {
    /* +0x00 */ u8 unused_00[0x38];
    /* +0x38 */ u16 field_38;
} AxVoice; /* size: 0x3A (approximate) */

/* The AX handle `fn_800E8E60` reads a word from. */
typedef struct AxHandle {
    /* +0x00 */ u32 unused_00;
    /* +0x04 */ u32 field_04;
} AxHandle; /* size: 0x8 */

/* The AX voice block of a sound slot (fn_800E5F38 returns it). */
typedef struct AxVoiceBlock {
    /* +0x00 */ u8 unused_00[2];
    /* +0x02 */ s16 field_02;   /* pan, fn_800E781C */
    /* +0x04 */ u8 unused_04[2];
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
    /* +0x30 */ u8 unused_30[0x0A];
    /* +0x3A */ u8 field_3A;    /* loop state, set to 5 when it wraps */
    /* +0x3B */ u8 unused_3B[5];
    /* +0x40 */ AxHandle* field_40; /* fn_800E5EC4 clears it, fn_800E7878 reads its +4 */
    /* +0x44 */ AxVoice* voice; /* fn_800E5EC4 clears it, fn_800E6168 frees it */
    /* +0x48 */ u8 unused_48[2];
    /* +0x4A */ s16 field_4A;
    /* +0x4C */ s16 field_4C;   /* pitch upper limit */
    /* +0x4E */ s16 field_4E;   /* pitch lower limit (negated) */
    /* +0x50 */ u8 unused_50[0x0A];
    /* +0x5A */ s16 field_5A;
} AxVoiceBlock; /* size: 0x5C */

/* A block whose AX voice state sits at +0x1C. */
typedef struct AxHolder {
    /* +0x00 */ u8 unused_00[0x1C];
    /* +0x1C */ struct AxVoiceBlock ax;
} AxHolder; /* size: 0x1C + sizeof(AxVoiceBlock) */

/* A block with one word at +0x08. */
typedef struct WordAt8 {
    /* +0x00 */ u8 unused_00[8];
    /* +0x08 */ u32 word;
} WordAt8; /* size: 0xC */

/* One sound slot: a flag word, the owner pointer and the AX voice block. */
typedef struct SoundSlot {
    /* +0x00 */ u32 unused_00;
    /* +0x04 */ u32 unused_04;
    /* +0x08 */ u32 unused_08;
    /* +0x0C */ u32 flags;      /* fn_800E5F18/5F28/6238 */
    /* +0x10 */ u32 unused_10;
    /* +0x14 */ void* unused_14;
    /* +0x18 */ u8 unused_18[4];
    /* +0x1C */ AxVoiceBlock ax; /* fn_800E5F38 returns it */
} SoundSlot; /* size: 0x64 */

/* The owner record fn_800E5EC4/6168 update. */
typedef struct SoundOwner {
    /* +0x00 */ u8 unused_00[0x14];
    /* +0x14 */ SoundSlot* slot;
} SoundOwner; /* size: 0x18 */

/* The config fn_800E5D68 scales into a voice volume. */
typedef struct SoundCfg {
    /* +0x00 */ s16 level;     /* per-slot level */
    /* +0x02 */ s16 level2;
    /* +0x04 */ s16 level3;
    /* +0x06 */ u8 unused_06[0x0A];
    /* +0x10 */ u16 voice_id;  /* fn_800E8594's argument */
} SoundCfg; /* size: 0x12 (approximate) */

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


/* Two untyped words. */
typedef struct Pair32 {
    /* +0x00 */ u32 a;
    /* +0x04 */ u32 b;
} Pair32; /* size: 0x8 */

/* A vtable pointer. */
typedef struct VtblHolder {
    /* +0x00 */ const void* vtable;
} VtblHolder; /* size: 0x4 */

/* A group of relocation states, stride 8 (`fn_800E7E94`..`7FBC` each fill one). */
typedef struct RelocGroup {
    /* +0x00 */ RelocState state[9];
} RelocGroup; /* size: 0x48 */

/* A sound object's level array. */
typedef struct LevelHolder {
    /* +0x000 */ u8 unused_000[124];
    /* +0x07C */ u32 levels[1];
} LevelHolder; /* size: 0x80 (approximate) */

/* The global level setting (`fn_800E8690`). */
typedef struct LevelSetting {
    /* +0x00 */ s16 level;
    /* +0x02 */ u16 unused_02;
} LevelSetting; /* size: 0x4 */

/* One entry of a slot table: a slot id and its weight. */
typedef struct SlotTableEntry {
    /* +0x00 */ u8 id;
    /* +0x01 */ u8 weight;
} SlotTableEntry; /* size: 0x2 */

/* A weighted slot table (`fn_800E745C` rolls against it). */
typedef struct SlotTable {
    /* +0x00 */ u32 count;
    /* +0x04 */ SlotTableEntry entries[1];
} SlotTable; /* size: 0x8 (variable) */

/* The voice parameters `fn_800E72B0`/`737C`/`745C` read. */
typedef struct VoiceCfg {
    /* +0x00 */ u8 kind;
    /* +0x01 */ u8 unused_01;
    /* +0x02 */ u8 field_02;
    /* +0x03 */ u8 unused_03[3];
    /* +0x06 */ u16 field_06;
    /* +0x08 */ u8 unused_08[2];
    /* +0x0A */ u8 field_0A;
    /* +0x0B */ u8 field_0B;
    /* +0x0C */ u8 unused_0C[0x11];
    /* +0x1D */ u8 field_1D;    /* next slot id in the chain, 255 ends it */
    /* +0x1E */ u8 unused_1E[3];
    /* +0x21 */ u8 field_21;
    /* +0x22 */ u8 field_22;
    /* +0x23 */ u8 field_23;
} VoiceCfg; /* size: 0x24 */


/* A delay line's relocation state, at +0x20 (`fn_800E77DC`). */
typedef struct LineSlot {
    /* +0x00 */ u8 unused_00[0x20];
    /* +0x20 */ RelocState state;
} LineSlot; /* size: 0x28 */

/* The pitch arguments `fn_804717C0` takes. */
typedef struct AxPitchArgs {
    /* +0x00 */ u16 field_00;
    /* +0x02 */ u16 field_02;
    /* +0x04 */ u16 field_04;
    /* +0x06 */ u16 field_06;
} AxPitchArgs; /* size: 0x8 */

/* The unit's `.sbss` volume block.  Declared with its map names - the target object relocates against
 * exactly these. */
extern u8 lbl_807949B0[8];          /* per-slot "this stream is bgm" flags */
extern StreamVol lbl_807949B8;      /* bgm smoothed volume */
extern StreamVol lbl_807949C0;      /* bgm set-point (set_str_bgm_main_vol) */
extern StreamVol lbl_807949C8;      /* se smoothed volume */
extern StreamVol lbl_807949D0;      /* se set-point (set_str_se_main_vol) */

extern f32 lbl_80796478;            /* 1.0f: the initial volume */
extern f32 lbl_80796490;            /* 127.0f: the level normaliser */
extern f64 lbl_807964A0;            /* the int->float conversion magic */
extern u8 lbl_80698AF0[0x1D20];     /* the global slot list (fn_800E615C) */
extern f32 lbl_807964B8;            /* fn_800E7CD4's scale factor */
extern u8 lbl_80597DF8[0x28];       /* a vtable fn_800E7D24 installs */
extern u32 lbl_80597E20[128];       /* the level table fn_800E8E48 indexes */
extern u8 lbl_80597DA8[0x28];       /* the sound object's vtable (fn_800E7ABC) */
extern u8 lbl_80597DD0[0x28];       /* the sound object's base vtable (fn_800E7CE0) */
extern f32 lbl_807964B0;            /* fn_800E7ABC's initial field_88 */
extern f32 lbl_807964B4;            /* fn_800E7ABC's field_8C source */
extern LevelSetting lbl_80794A04;   /* the global level setting */
extern u8 lbl_80794A08[8];          /* the "state off" flag */
extern u32 lbl_807949E4;            /* reverb work-area size */
extern u32 lbl_807949E8;            /* reverb work-area base */
extern u32 lbl_807949EC;
extern u32 lbl_807949F0;
extern u32 lbl_807949F4;
extern u32 lbl_80794A00;            /* reverb time counter */
extern void* mpMediator__15sNetworkLibrary;
extern u32 lbl_807949F8;
extern u32 lbl_807949FC;
extern u32 lbl_80794868;
extern u8 lbl_80698AE0[0x10];
extern void* lbl_807949D8;

extern StreamWork lbl_8069A810;     /* the stream manager */
extern u8 lbl_8069A8E8[0x6828];     /* the sound table `fn_800EDC50`/`fn_800EDD00` install into */
extern ReverbMgr lbl_80696D90;      /* the global reverb manager (fn_800E5398) */
extern u8 lbl_80697980[0xE60];      /* the global sound object (fn_800E5714) */

/* Callees still unsplit; the spellings are the map's, which is what the target relocates against. */
extern "C" {
void fn_80075390(Mtx44* mtx);
void fn_802BDC90(Mtx44* mtx);
void* fn_80075E98(Mtx44* mtx);
void GXSetProjection(void* mtx, u32 type);
u32 OSDisableInterrupts(void);
void OSRestoreInterrupts(u32 level);

/* This unit's own entries (defined below; declared up here so the address order below can be read in
 * call order). */
Mtx44* fn_800E479C(Mtx44* dst, const Mtx44* src);
void fn_800E47CC(Mtx44* dst, const Mtx44* src);
void fn_800E4E00(StreamRec* rec);
void fn_800E4E10(StreamRec* rec);
void fn_800E4E18(RelocState* state);
void fn_800E4E28(StreamRec* rec);
void fn_800E4E30(StreamRec* rec);
void fn_800E4E38(StreamRec* rec);
void fn_800E4E70(StreamWork* work, s16 slot);
s16 fn_800E4E9C(StreamWork* work);
s16 fn_800E4EE8(StreamWork* work, u32 idx);
void fn_800E4C0C(StreamRec* rec, u32 a, u32 b);
void fn_800E4C18(StreamRec* rec, RelocTable* table);
void fn_800E4C1C(RelocState* state, RelocTable* table);
void fn_800E4C78(StreamRec* rec, RelocTable* table);
void fn_800E4C80(StreamRec* rec, RelocTable* table);
void fn_800E4C88(StreamRec* rec, RelocTable* table);
void* fn_800E4FB4(StreamRec* rec, u32 entry_idx);

void fn_800E51AC(ReverbMgr* mgr, u32 idx);
ReverbHiData* fn_800E5184(ReverbFx* fx);
ReverbStdData* fn_800E517C(ReverbFx* fx);
ReverbStdData* fn_800E518C(ReverbFx* fx);
ReverbHiData* fn_800E5194(ReverbFx* fx);
ReverbStdData* fn_800E519C(ReverbFx* fx);
ReverbHiData* fn_800E51A4(ReverbFx* fx);
void fn_800E522C(ReverbFx* fx);
void fn_800E523C(ReverbFx* fx);
void fn_800E52A0(ReverbFx* fx);
void fn_800E5304(ReverbFx* fx);
ReverbMgr* fn_800E53A4(ReverbMgr* mgr);
void fn_800E5420(ReverbFx* fx, ReverbSetFn handler);
ReverbBank* fn_800E5430(ReverbBank* bank, s32 flags);
void* dtor_800E54A8(void* obj, s32 flags);
void* fn_800E5504(void* obj, s32 flags);
void* dtor_800E5548(void* obj, s32 flags);
void* fn_800E5608(void* obj, s32 flags);
void* fn_800E564C(void* obj, s32 flags);
void* fn_800E5690(void* obj, s32 flags);
void fn_800E56D4(void* arg);
Sound* fn_800E5714(void);
u32 fn_800E8858(void);
void fn_80474DD0(void);
void fn_80474EB0(ReverbHiData* data);
void fn_80474F20(void);
void fn_80474D50(ReverbStdData* data);
void fn_80474E80(ReverbHiData* data);
void fn_80476DF0(ReverbStdData* data);
void fn_80476E80(ReverbHiData* data);
void fn_80476F10(ReverbStdData* data);
void fn_80476F40(ReverbHiData* data);
void AXFXReverbHiInit(ReverbStdData* data);
void AXFXReverbHiShutdown(ReverbStdData* data);
void AXFXReverbHiCallback(void);
void AXRegisterAuxACallback(void);
void fn_8046F210(void);
void fn_8046F280(void);
void* memset(void* dst, int val, u32 n);
void __destroy_arr(void* array, void* dtor, int size, int count);
void __dl__FPv(void* p);

void fn_800E5EC4(void* unused, SoundOwner* owner);
void fn_800E5F18(SoundSlot* slot, u32 mask);
void fn_800E5F28(SoundSlot* slot, u32 mask);
AxVoiceBlock* fn_800E5F38(void* p);
void fn_800E5F40(Sound2* self);
u32 fn_800E60B4(u8* list, u8* entry);
u32 fn_800E60E8(u8* a, u8* b);
u8* fn_800E60F8(u8* list);
u32 fn_800E6100(u8* list, u8* entry);
u32 fn_800E6134(u8* a, u8* b);
u8* fn_800E6144(u8* list);
VoiceEntry* fn_800E614C(VoiceEntry* voice);
SlotEntry* fn_800E6154(u8* entry);
u8* fn_800E615C(void);
u32 fn_800E6168(void* unused, SoundOwner* owner);
u32 fn_800E6238(SoundSlot* slot, u32 mask);
void fn_800E6250(Sound2* self, SoundSlot* slot);
void fn_800E6314(Sound2* self, AxVoiceBlock* ax);
void fn_800E96BC(Sound2* self, SoundSlot* slot, AxVoiceBlock* ax);
s32 fn_800E885C(void);
s16 fn_800E852C(void);
void* fn_800E9B5C(u8* list, void* entry);
void* fn_800E9BE4(u8* list);
void* fn_800EDB24(void* table);
void* fn_800EDA88(void* table, void* slot);
void AXFreeVoice(AxVoice* voice);

void fn_800E65EC(void* unused, SoundSlot* slot);
u32 fn_800E66C8(ReverbMgr* mgr);
ReverbMgr* fn_800E66D0(void);
void fn_800E66DC(void* unused, u32 owner, u32 kind, u8 a, u8 b, u8 c, u8 d, s16 e, s16 f);
u32 fn_800E7264(WordAt8* p);
u32 fn_800E726C(u8* list, u8* entry);
u32 fn_800E72A0(u8* a, u8* b);
void fn_800E72B0(void* unused, VoiceCfg* cfg, u32 arg);
void fn_800E737C(Sound2* self, VoiceCfg* cfg, u32 arg);
void fn_800E745C(Sound2* self, void* p1, VoiceCfg* cfg, u32 p3, u32 p4);
void* fn_800E7570(void* p, u32 idx);
void* fn_800E7578(RelocState* state, u32 idx);
Sound* fn_800E7AB0(void);
void fn_800E7CA4(Pair32* p);
void fn_800E7CB4(Pair32* p);
void fn_800E7CC4(Pair32* p);
f32 fn_800E7CD4(f32 v);
void fn_800E7D24(VtblHolder* p);
void fn_800E7E94(void* group, RelocTable* table);
void fn_800E7E9C(void* group, RelocTable* table);
void fn_800E7EA4(void* group, RelocTable* table);
void fn_800E7EBC(void* group, RelocTable* table);
ReverbLine* fn_800E7ED0(Sound* snd, u32 idx);
void fn_800E7F9C(void* group, RelocTable* table);
void fn_800E7FA4(void* group, RelocTable* table);
void fn_800E7FAC(void* group, RelocTable* table);
void fn_800E7FB4(void* group, RelocTable* table);
void fn_800E7FBC(void* group, RelocTable* table);
void fn_800E84DC(Sound* p, u32 v, u8 idx);
void fn_800E84F0(s32 v);
void fn_800E8524(Sound* snd, s16 v);
s16 fn_800E8550(Sound* snd);
void fn_800E8558(u8 v);
void fn_800E858C(Sound* snd, u8 v);
s16 fn_800E85D8(Sound* snd, u32 idx);
LevelSetting* fn_800E8690(void);
void fn_800E87E0(void* alloc, void* free);
s32 fn_800E885C(void);
s16 fn_800E8880(LevelSetting* setting);
void* fn_800E89D8(void);
void fn_800E89E0(u32 v);
void fn_800E8D40(void);
void fn_800E8D74(void);
void fn_800E8E3C(s16* p);
u32 fn_800E8E48(s16 idx);

u32 fn_800E9D00(void);
void* fn_800E9960(u8* list);
void fn_800EE014(AxHandle* handle, AxVoice* voice, s16 b, u16 c);
void fn_804C2840(AxVoice* voice, u32 a, s32 b, s32 c, s32 d, s32 e, s16 f, s16 g, s32 h);
void fn_80471610(AxVoice* voice, u32 a);
u32 fn_804C2830(void);
void AXFXSetHooks(void* alloc, void* free);

void fn_800E75B0(void* unused, SoundSlot* slot);
void fn_800E7620(void* unused, SoundSlot* slot);
void fn_800E7688(void* unused, SoundSlot* slot);
u32 fn_800E7700(void* unused, u32 key);
void fn_800E778C(Sound* snd, u32 idx, u32 entry_idx);
void* fn_800E77DC(void* p, u32 idx);
void fn_800E781C(void* unused, void* slot, s16 pan);
void fn_800E7878(void* unused, void* slot, s16 pitch);
s16 fn_800E78F8(void* unused, u32 key);
void fn_800E7990(void* unused, u32 key, u16 pitch);
Sound* fn_800E7ABC(Sound* snd);
ReverbLine* fn_800E7C34(ReverbLine* line);
Sound* fn_800E7CE0(Sound* snd);
void fn_800E7D34(void);
void fn_800E7DE4(u32 idx, RelocDesc* desc);
void fn_800E7EE0(u32 idx, RelocDesc* desc);
void* fn_800E7FC4(u32 idx, u32 key);
u32 fn_800E802C(u32 key);
void fn_800E8080(u32 key, u16 pitch);
void* fn_800E80DC(u32 a, u32 b, u32 c);
void* fn_800E8150(u32 a, u32 b, u32 c, u32 d);
void* fn_800E81BC(u32 a, u32 b, u32 c, u32 d);
void fn_800E8228(u32 owner);
void fn_800E8294(void);
void fn_800E82D4(void);
void fn_800E8314(void);
void fn_800E8354(u32 owner, s16 arg);
void fn_800E83CC(u32 owner, s16 arg);
s16 fn_800E8444(u32 key);
void fn_800E8498(u32 v, u8 idx);
s16 fn_800E8594(u32 idx);
void fn_800E85E8(u32 idx);
void fn_800E8E60(AxVoice* voice, u32 a, s16 b);
void fn_800E93E0(ReverbLine* line);
void* fn_800E6764(Sound* snd, u32 a, u32 b, u32 c);
void* fn_800E6818(Sound* snd, u32 a, u32 b, u32 c, u32 d);
void* fn_800E6A58(Sound* snd, u32 a, u32 b, u32 c, u32 d);
void fn_80471890(u16 pitch, u16* a, u16* b);
void fn_804717C0(AxVoice* voice, AxPitchArgs* args);
void fn_80471830(AxVoice* voice, u16 a, u16 b);
void __construct_array(void* array, void* ctor, void* dtor, int size, int count);

void* fn_800E6A18(void* p, u32 idx);
void fn_800E6D44(Sound2* self, void* a, VoiceCfg* cfg, u32 c, u32 d);
void* fn_800E6F7C(void* p, u32 idx);
void* fn_800E6FB8(void* p, u32 idx);
void* fn_800E6FF8(void* p, u32 idx);
void* fn_800E7038(void* p, u32 idx);
void fn_800E8634(u32 idx, u32 entry_idx);
void fn_800E8698(void);
void fn_800E86E8(u32 id);
void fn_800E8CC8(s16* src, s32 count);
void fn_800E8DA4(void);
void* fn_800E8DE0(void* obj, s32 flags);
void fn_800E6D8C(Sound2* self, void* a, VoiceCfg* cfg, u32 c, u32 d);
void fn_804DD460(void);
void fn_804DF140(void);
void fn_804C40D0(void);
void fn_800EDB74(void* table);
void fn_8046FDD0(u16 id);
u32 OSEnableInterrupts(void);
void fn_8046E490(void* obj);
void fn_804C2800(void);
void __register_global_object(void* obj, void* dtor, void* ref);

void* fn_800E8888(void);
void fn_800E8730(void);
void fn_800E87E4(u32 mode);
u32 fn_800E6C60(Sound* snd, u32 a, u32 b, u32 c);
void AIInit(u32 mode);
void fn_8046E430(u32 v);
void fn_804C26E0(void);
void fn_804DF0A0(void);
void fn_804DD430(void);
void AXRegisterCallback(void* cb);
void fn_8046FD90(u32 v);
void* fn_8046D420(void* cb);
void fn_8046FD70(u32 v);
void fn_804C2820(u32 mode);

StreamWork* fn_800E4A18(void);
u32 fn_800E4A00(void* work, u32 mask);
void fn_800E49E0(void* work, s32 mask);
void fn_800E4ABC(void* work, u32 mask);
StreamWork* fn_800E49F4(void);
StreamRec* fn_800E4C90(StreamWork* work, u32 idx);
void fn_800E48B0(void);
void fn_800EDC50(void* table, u32 id);
void fn_800EDDB0(void* table, u32 id);
void fn_800EDE60(void* table, u32 id);
void fn_800EDD00(void* table, u32 id, u32 idx);
void fn_800EBAE8(StreamWork* work, u32 a, u32 b, u32 c);
void fn_800EBC24(StreamWork* work, u32 a, u32 b, u32 c);
void fn_800EBDDC(StreamWork* work, u32 a, u32 b, u32 c);
u32 fn_800ED5B4(StreamWork* work, u32 id);
}

/* Maps a sound-kind selector to the volume-slot it plays through. */
extern "C" s32 fn_800E46E8(u32 kind)
{
    switch ((u8)kind) {
    case 0: return 0;
    case 1: return 1;
    case 2: return 3;
    case 3: return 2;
    case 4: return 5;
    case 5: return 6;
    case 6: return 4;
    case 7: return 7;
    default: return 3;
    }
}

/* Builds a projection matrix from the current screen setup and hands it to GX. */
extern "C" void fn_800E4754(void)
{
    Mtx44 mtx;
    Mtx44 src;

    fn_80075390(&mtx);
    fn_802BDC90(&src);
    fn_800E479C(&mtx, &src);
    GXSetProjection(fn_80075E98(&mtx), 0);
}

/* Assigns one matrix over another and returns the destination. */
extern "C" Mtx44* fn_800E479C(Mtx44* dst, const Mtx44* src)
{
    fn_800E47CC(dst, src);
    return dst;
}

/* Copies a 4x4 matrix. */
extern "C" void fn_800E47CC(Mtx44* dst, const Mtx44* src)
{
    *dst = *src;
}

/* Resets every volume to 1.0 and clears the per-slot flags. */
extern "C" void fn_800E4850(void)
{
    lbl_807949B0[0] = 0;
    lbl_807949B0[1] = 0;
    lbl_807949B0[2] = 0;
    lbl_807949B0[3] = 0;
    lbl_807949B8.prev = lbl_80796478;
    lbl_807949B8.cur = lbl_80796478;
    lbl_807949C0.cur = lbl_80796478;
    lbl_807949C8.prev = lbl_80796478;
    lbl_807949C8.cur = lbl_80796478;
    lbl_807949D0.cur = lbl_80796478;
}

/* Sets one slot's "this stream is bgm" flag. */
void set_stream_main_vol_flag(u8 slot, u8 is_bgm)
{
    lbl_807949B0[slot] = is_bgm;
}

/* Sets the bgm set-point volume. */
void set_str_bgm_main_vol(f32 vol)
{
    lbl_807949C0.cur = vol;
}

/* Sets the se set-point volume. */
void set_str_se_main_vol(f32 vol)
{
    lbl_807949D0.cur = vol;
}

/* Snapshots both set-points into the smoothed volumes (the previous value keeps the interpolation start). */
extern "C" void fn_800E48B0(void)
{
    lbl_807949B8.prev = lbl_807949B8.cur;
    lbl_807949B8.cur = lbl_807949C0.cur;
    lbl_807949C8.prev = lbl_807949C8.cur;
    lbl_807949C8.cur = lbl_807949D0.cur;
}

/* The stream subsystem is always ready. */
extern "C" s32 fn_800E48DC(void)
{
    return 1;
}

/* Picks the smoothed volume of the bus one slot is assigned to. */
extern "C" f32 fn_800E48E4(u8 slot)
{
    if (lbl_807949B0[slot] == 0) {
        return lbl_807949B8.cur;
    }
    return lbl_807949C8.cur;
}

/* Sets the set-point of the bus selected by `is_bgm`. */
extern "C" void fn_800E4908(u8 is_bgm, f32 vol)
{
    if (is_bgm == 0) {
        lbl_807949C0.cur = vol;
    }
    if (is_bgm != 0) {
        lbl_807949D0.cur = vol;
    }
}

/* The current bgm volume. */
extern "C" f32 fn_800E492C(void)
{
    return lbl_807949B8.cur;
}

/* Snapshots the volumes, then selects whichever of the three sound banks is flagged as pending. */
extern "C" void fn_800E4934(void)
{
    fn_800E48B0();
    if (fn_800E4A00(fn_800E4A18(), 1) != 0) {
        fn_800EDC50(fn_800E49F4(), 2);
        fn_800E49E0(fn_800E4A18(), 1);
        return;
    }
    if (fn_800E4A00(fn_800E4A18(), 2) != 0) {
        fn_800EDDB0(fn_800E49F4(), 2);
        fn_800E49E0(fn_800E4A18(), 2);
        return;
    }
    if (fn_800E4A00(fn_800E4A18(), 4) != 0) {
        fn_800EDE60(fn_800E49F4(), 2);
        fn_800E49E0(fn_800E4A18(), 4);
    }
}

/* Clears the given bank's pending bits. */
extern "C" void fn_800E49E0(void* work, s32 mask)
{
    ((FlagWord*)work)->flags = ((FlagWord*)work)->flags & ~mask;
}

/* The sound table the bank installers write into. */
extern "C" StreamWork* fn_800E49F4(void)
{
    return (StreamWork*)lbl_8069A8E8;
}

/* Tests whether any of the given bank's pending bits is set. */
extern "C" u32 fn_800E4A00(void* work, u32 mask)
{
    return (((FlagWord*)work)->flags & mask) != 0;
}

/* The global stream manager. */
extern "C" StreamWork* fn_800E4A18(void)
{
    return &lbl_8069A810;
}

/* Resets the volume block and hands the three sound-bank parameters to the bank installer. */
extern "C" void fn_800E4A24(u32 a, u32 b, u32 c)
{
    fn_800E4850();
    fn_800EBAE8(fn_800E4A18(), a, b, c);
}

/* Marks bank 1 pending, with interrupts off. */
extern "C" void fn_800E4A7C(void)
{
    u32 level = OSDisableInterrupts();
    fn_800E4ABC(fn_800E4A18(), 1);
    OSRestoreInterrupts(level);
}

/* Sets the given bank's pending bits. */
extern "C" void fn_800E4ABC(void* work, u32 mask)
{
    ((FlagWord*)work)->flags |= mask;
}

/* Marks bank 2 pending, with interrupts off. */
extern "C" void fn_800E4ACC(void)
{
    u32 level = OSDisableInterrupts();
    fn_800E4ABC(fn_800E4A18(), 2);
    OSRestoreInterrupts(level);
}

/* Marks bank 4 pending, with interrupts off. */
extern "C" void fn_800E4B0C(void)
{
    u32 level = OSDisableInterrupts();
    fn_800E4ABC(fn_800E4A18(), 4);
    OSRestoreInterrupts(level);
}

/* Installs one stream's four relocation tables and its two scalar words. */
extern "C" void fn_800E4B4C(u32 idx, RelocDesc* desc, u32 a, u32 b)
{
    RelocTable* t4 = (RelocTable*)(desc->reloc_04 + (u32)desc);
    RelocTable* t8 = (RelocTable*)(desc->reloc_08 + (u32)desc);
    RelocTable* tC = (RelocTable*)(desc->reloc_0C + (u32)desc);
    fn_800E4C88(fn_800E4C90(fn_800E4A18(), idx), (RelocTable*)(desc->reloc_00 + (u32)desc));
    fn_800E4C80(fn_800E4C90(fn_800E4A18(), idx), t4);
    fn_800E4C78(fn_800E4C90(fn_800E4A18(), idx), t8);
    fn_800E4C18(fn_800E4C90(fn_800E4A18(), idx), tC);
    fn_800E4C0C(fn_800E4C90(fn_800E4A18(), idx), a, b);
}

/* Stores a stream's two scalar words. */
extern "C" void fn_800E4C0C(StreamRec* rec, u32 a, u32 b)
{
    rec->scalar_20 = a;
    rec->scalar_24 = b;
}

/* Installs a relocation table into a stream's first state. */
extern "C" void fn_800E4C18(StreamRec* rec, RelocTable* table)
{
    fn_800E4C1C(&rec->reloc[0], table);
}

/* Installs a relocation table into the given state and resolves every entry that is not yet fixed. */
extern "C" void fn_800E4C1C(RelocState* state, RelocTable* table)
{
    u32 i;

    state->table = table;
    state->entries = (void**)((u8*)table + table->entries_offset);
    for (i = 0; i < state->table->count; i++) {
        u32 entry = (u32)state->entries[i];
        if ((entry & 0x80000000) == 0 && entry != 0) {
            state->entries[i] = (void*)((u32)state->entries + entry);
        }
    }
}

/* Installs a relocation table into a stream's third state. */
extern "C" void fn_800E4C78(StreamRec* rec, RelocTable* table)
{
    fn_800E4C1C(&rec->reloc[2], table);
}

/* Installs a relocation table into a stream's fourth state. */
extern "C" void fn_800E4C80(StreamRec* rec, RelocTable* table)
{
    fn_800E4C1C(&rec->reloc[3], table);
}

/* Installs a relocation table into a stream's second state. */
extern "C" void fn_800E4C88(StreamRec* rec, RelocTable* table)
{
    fn_800E4C1C(&rec->reloc[1], table);
}

/* The stream record of one slot. */
extern "C" StreamRec* fn_800E4C90(StreamWork* work, u32 idx)
{
    return &work->recs[idx];
}

/* Starts a stream on the given bank, with interrupts off. */
void PlayStream(u32 a, u32 b)
{
    u32 level = OSDisableInterrupts();
    fn_800EBC24(fn_800E4A18(), a, b, 0);
    OSRestoreInterrupts(level);
}

/* Stops the stream on the given bank, with interrupts off. */
extern "C" void fn_800E4D00(u32 a, u32 b)
{
    u32 level = OSDisableInterrupts();
    fn_800EBDDC(fn_800E4A18(), a, b, 0);
    OSRestoreInterrupts(level);
}

/* Releases one slot's sound table and clears its record. */
extern "C" void fn_800E4D60(u32 idx)
{
    u32 level = OSDisableInterrupts();

    fn_800EDD00(fn_800E49F4(), 2, idx);
    fn_800E4E38(fn_800E4C90(fn_800E4A18(), idx));
    fn_800E4E30(fn_800E4C90(fn_800E4A18(), idx));
    fn_800E4E28(fn_800E4C90(fn_800E4A18(), idx));
    fn_800E4E10(fn_800E4C90(fn_800E4A18(), idx));
    fn_800E4E00(fn_800E4C90(fn_800E4A18(), idx));
    OSRestoreInterrupts(level);
}

/* Clears a stream record's two scalar words. */
extern "C" void fn_800E4E00(StreamRec* rec)
{
    rec->scalar_20 = 0;
    rec->scalar_24 = 0;
}

/* Clears a stream record's fourth relocation state. */
extern "C" void fn_800E4E10(StreamRec* rec)
{
    fn_800E4E18(&rec->reloc[3]);
}

/* Clears a relocation state. */
extern "C" void fn_800E4E18(RelocState* state)
{
    state->table = 0;
    state->entries = 0;
}

/* Clears a stream record's third relocation state. */
extern "C" void fn_800E4E28(StreamRec* rec)
{
    fn_800E4E18(&rec->reloc[2]);
}

/* Clears a stream record's second relocation state. */
extern "C" void fn_800E4E30(StreamRec* rec)
{
    fn_800E4E18(&rec->reloc[1]);
}

/* Clears a stream record's first relocation state. */
extern "C" void fn_800E4E38(StreamRec* rec)
{
    fn_800E4E18(&rec->reloc[0]);
}

/* Selects the volume slot a stream plays through. */
extern "C" void fn_800E4E3C(s32 slot)
{
    fn_800E4E70(fn_800E4A18(), (s16)slot);
}

/* Stores the manager's current slot index. */
extern "C" void fn_800E4E70(StreamWork* work, s16 slot)
{
    work->cur_slot = slot;
}

/* The manager's current slot index. */
extern "C" s16 fn_800E4E78(void)
{
    return fn_800E4E9C(fn_800E4A18());
}

/* The manager's current slot index. */
extern "C" s16 fn_800E4E9C(StreamWork* work)
{
    return work->cur_slot;
}

/* The volume slot of one of the three banks, or 0 for an out-of-range bank. */
extern "C" s16 fn_800E4EA4(u32 idx)
{
    if (idx >= 3) {
        return 0;
    }
    return fn_800E4EE8(fn_800E4A18(), idx);
}

/* The volume slot stored for one bank. */
extern "C" s16 fn_800E4EE8(StreamWork* work, u32 idx)
{
    return work->slot_tab[idx];
}

/* The status of one stream, with interrupts off. */
u32 GetStreamStatus(u32 id)
{
    u32 level = OSDisableInterrupts();
    u32 status = fn_800ED5B4(fn_800E4A18(), id);

    OSRestoreInterrupts(level);
    return status;
}

/* One stream's entry in a relocation table, with interrupts off. */
extern "C" void* fn_800E4F4C(u32 idx, u32 entry_idx)
{
    u32 level = OSDisableInterrupts();
    void* entry = fn_800E4FB4(fn_800E4C90(fn_800E4A18(), idx), entry_idx);

    OSRestoreInterrupts(level);
    return entry;
}

/* Looks up one entry of a stream's third relocation table. */
extern "C" void* fn_800E4FB4(StreamRec* rec, u32 entry_idx)
{
    RelocState* state = rec->reloc + 2;

    if (state->table == 0) {
        return 0;
    }
    if (state->table->count <= entry_idx) {
        return 0;
    }
    return state->entries[entry_idx];
}

/* Installs one reverb unit's parameters and marks it pending. */
extern "C" void fn_800E4FF4(ReverbMgr* mgr, ReverbCfg* cfg)
{
    u32 kind = cfg->kind;

    if (kind >= 3) {
        return;
    }
    fn_800E51AC(mgr, kind);
    if (cfg->enable != 0) {
        mgr->flags |= 1 << cfg->kind;
    } else {
        mgr->flags &= ~(1 << cfg->kind);
    }
    ReverbFx* fx = mgr->effects[cfg->kind];
    if (mgr->mode == 3) {
        ReverbHiData* data = fn_800E5184(fx);
        data->damping = cfg->damping;
        data->mix = cfg->mix;
        data->time = cfg->time;
        data->coloration = cfg->coloration;
        data->pre_delay = cfg->pre_delay;
        data->crosstalk = cfg->crosstalk;
        fn_80474DD0();
        fn_80474EB0(data);
        fx->set_handler((ReverbSetFn)(void*)fn_80474F20, data);
        fx->active = 1;
    } else {
        ReverbStdData* data = fn_800E517C(fx);
        data->damping = cfg->damping;
        data->mix = cfg->mix;
        data->time = cfg->time;
        data->coloration = cfg->coloration;
        data->pre_delay = cfg->pre_delay;
        data->crosstalk = cfg->crosstalk;
        AXFXReverbHiInit(data);
        fn_80474D50(data);
        fx->set_handler((ReverbSetFn)(void*)AXFXReverbHiCallback, data);
        fx->active = 0;
    }
    fx->state = 0;
}

/* The effect body of the second class. */
extern "C" ReverbStdData* fn_800E517C(ReverbFx* fx)
{
    return (ReverbStdData*)&fx->data;
}

/* The effect body of the first class. */
extern "C" ReverbHiData* fn_800E5184(ReverbFx* fx)
{
    return (ReverbHiData*)&fx->data;
}

/* The effect body of the second class. */
extern "C" ReverbStdData* fn_800E518C(ReverbFx* fx)
{
    return (ReverbStdData*)&fx->data;
}

/* The effect body of the first class. */
extern "C" ReverbHiData* fn_800E5194(ReverbFx* fx)
{
    return (ReverbHiData*)&fx->data;
}

/* The effect body of the second class. */
extern "C" ReverbStdData* fn_800E519C(ReverbFx* fx)
{
    return (ReverbStdData*)&fx->data;
}

/* The effect body of the first class. */
extern "C" ReverbHiData* fn_800E51A4(ReverbFx* fx)
{
    return (ReverbHiData*)&fx->data;
}

/* Releases one reverb unit: its shutdown routine for the current state, then its callback and body. */
extern "C" void fn_800E51AC(ReverbMgr* mgr, u32 idx)
{
    ReverbFx* fx = mgr->effects[idx];
    s32 state = fx->state;

    if (state == 3) {
        return;
    }
    switch (state) {
    case 0: fn_800E523C(fx); break;
    case 1: fn_800E52A0(fx); break;
    case 2: fn_800E5304(fx); break;
    }
    fx->state = 3;
}

/* Clears an effect body. */
extern "C" void fn_800E522C(ReverbFx* fx)
{
    memset(&fx->data, 0, sizeof(fx->data));
}

/* Releases the first effect class: its AXFX shutdown, then the callback and the body. */
extern "C" void fn_800E523C(ReverbFx* fx)
{
    if (fx->active == 1) {
        fn_80474E80(fn_800E5184(fx));
    } else {
        AXFXReverbHiShutdown(fn_800E517C(fx));
    }
    fx->set_handler(0, 0);
    fn_800E522C(fx);
}

/* Releases the second effect class. */
extern "C" void fn_800E52A0(ReverbFx* fx)
{
    if (fx->active == 3) {
        fn_80476E80(fn_800E5194(fx));
    } else {
        fn_80476DF0(fn_800E518C(fx));
    }
    fx->set_handler(0, 0);
    fn_800E522C(fx);
}

/* Releases the third effect class. */
extern "C" void fn_800E5304(ReverbFx* fx)
{
    if (fx->active == 3) {
        fn_80476F40(fn_800E51A4(fx));
    } else {
        fn_80476F10(fn_800E519C(fx));
    }
    fx->set_handler(0, 0);
    fn_800E522C(fx);
}

/* Stores the current sound handle in the caller's word. */
extern "C" void fn_800E5368(u32* out)
{
    *out = fn_800E8858();
}

/* Constructs the global reverb manager. */
extern "C" ReverbMgr* fn_800E5398(void)
{
    return fn_800E53A4(&lbl_80696D90);
}

/* Wires up the three reverb units and their effect pointers. */
extern "C" ReverbMgr* fn_800E53A4(ReverbMgr* mgr)
{
    fn_800E5420(&mgr->fx[0], (ReverbSetFn)(void*)AXRegisterAuxACallback);
    fn_800E5420(&mgr->fx[1], (ReverbSetFn)(void*)fn_8046F210);
    fn_800E5420(&mgr->fx[2], (ReverbSetFn)(void*)fn_8046F280);
    mgr->flags = 0;
    mgr->effects[0] = &mgr->fx[0];
    mgr->effects[1] = &mgr->fx[1];
    mgr->effects[2] = &mgr->fx[2];
    return mgr;
}

/* Sets one unit's callback and marks it constructed. */
extern "C" void fn_800E5420(ReverbFx* fx, ReverbSetFn handler)
{
    fx->set_handler = handler;
    fx->state = 3;
}

/* Destroys a reverb bank's 49 lines, then the bank itself. */
extern "C" ReverbBank* fn_800E5430(ReverbBank* bank, s32 flags)
{
    if (bank != 0) {
        __destroy_arr(&bank->lines, (void*)dtor_800E5548, sizeof(ReverbLine), 49);
        dtor_800E54A8(bank, 0);
        if ((s16)flags > 0) {
            __dl__FPv(bank);
        }
    }
    return bank;
}

/* Destroys one delay line's own members, then frees it. */
extern "C" void* dtor_800E54A8(void* obj, s32 flags)
{
    if (obj != 0) {
        fn_800E5504(obj, 0);
        if ((s16)flags > 0) {
            __dl__FPv(obj);
        }
    }
    return obj;
}

/* Frees an object if the delete flag asks for it. */
extern "C" void* fn_800E5504(void* obj, s32 flags)
{
    if (obj != 0 && (s16)flags > 0) {
        __dl__FPv(obj);
    }
    return obj;
}

/* Destroys the nine parts of one delay line, then frees the line. */
extern "C" void* dtor_800E5548(void* obj, s32 flags)
{
    ReverbLine* line = (ReverbLine*)obj;

    if (line != 0) {
        fn_800E5690(&line->part[8], -1);
        fn_800E5690(&line->part[7], -1);
        fn_800E5690(&line->part[6], -1);
        fn_800E5690(&line->part[5], -1);
        fn_800E5690(&line->part[4], -1);
        fn_800E5690(&line->part[3], -1);
        fn_800E5690(&line->part[2], -1);
        fn_800E564C(&line->part[1], -1);
        fn_800E5608(&line->part[0], -1);
        if ((s16)flags > 0) {
            __dl__FPv(line);
        }
    }
    return line;
}

/* Frees an object if the delete flag asks for it. */
extern "C" void* fn_800E5608(void* obj, s32 flags)
{
    if (obj != 0 && (s16)flags > 0) {
        __dl__FPv(obj);
    }
    return obj;
}

/* Frees an object if the delete flag asks for it. */
extern "C" void* fn_800E564C(void* obj, s32 flags)
{
    if (obj != 0 && (s16)flags > 0) {
        __dl__FPv(obj);
    }
    return obj;
}

/* Frees an object if the delete flag asks for it. */
extern "C" void* fn_800E5690(void* obj, s32 flags)
{
    if (obj != 0 && (s16)flags > 0) {
        __dl__FPv(obj);
    }
    return obj;
}

/* Passes an argument to the global sound object's fifth virtual method. */
extern "C" void fn_800E56D4(void* arg)
{
    Sound* snd = fn_800E5714();

    snd->vtable->method_14(snd, arg);
}

/* The global sound object. */
extern "C" Sound* fn_800E5714(void)
{
    return (Sound*)lbl_80697980;
}

/* Scales the three global level factors and the config's own levels into a 0..127 voice volume. */
extern "C" s32 fn_800E5D68(SoundCfg* cfg)
{
    f32 a = (f32)(s16)fn_800E885C() / lbl_80796490;
    f32 b = (f32)(s16)fn_800E8594(cfg->voice_id) / lbl_80796490;
    f32 c = (f32)(s16)fn_800E852C() / lbl_80796490;
    f32 d = (f32)cfg->level / lbl_80796490;
    f32 e = (f32)cfg->level3 / lbl_80796490;
    f32 f = (f32)cfg->level2 / lbl_80796490;
    s16 v = (s16)(s32)(a * c * b * f * e * d * lbl_80796490);

    if (v > 127) {
        v = 127;
    }
    if (v < 0) {
        v = 0;
    }
    return v;
}

/* Clears one sound slot's voice block and marks it unowned. */
extern "C" void fn_800E5EC4(void* unused, SoundOwner* owner)
{
    SoundSlot* slot = owner->slot;
    AxVoiceBlock* ax = fn_800E5F38(slot);

    ax->voice = 0;
    ax->field_40 = 0;
    fn_800E5F28(slot, 1);
    fn_800E5F18(slot, 1024);
}

/* Sets bits in a sound slot's flag word. */
extern "C" void fn_800E5F18(SoundSlot* slot, u32 mask)
{
    slot->flags |= mask;
}

/* Clears bits in a sound slot's flag word. */
extern "C" void fn_800E5F28(SoundSlot* slot, u32 mask)
{
    slot->flags &= ~mask;
}

/* The AX voice block of a sound slot. */
extern "C" AxVoiceBlock* fn_800E5F38(void* p)
{
    return &((AxHolder*)p)->ax;
}

/* Dispatches every active voice of a slot to the handler its kind selects. */
extern "C" void fn_800E5F40(Sound2* self)
{
    VoiceEntry* iter = (VoiceEntry*)fn_800E9BE4(fn_800E615C());

    while (fn_800E60B4((u8*)fn_800E615C(), (u8*)iter) == 1) {
        SlotEntry* info = fn_800E6154((u8*)iter);
        VoiceEntry* voice = (VoiceEntry*)fn_800EDB24(fn_800E49F4());

        while (fn_800E6100((u8*)fn_800E49F4(), (u8*)voice) == 1) {
            if (voice->active == 1 && voice->owner == info->owner) {
                switch (voice->kind) {
                case 0: self->vtable->method_18(self, voice); break;
                case 1: self->vtable->method_1C(self, voice); break;
                case 2: self->vtable->method_20(self, voice); break;
                case 3: fn_800E781C(self, voice, info->field_0C); break;
                case 4: fn_800E7878(self, voice, info->field_0E); break;
                }
            }
            voice = fn_800E614C(voice);
        }
        iter = (VoiceEntry*)fn_800E9B5C(fn_800E615C(), iter);
    }
}

/* Whether a slot list has reached its end marker. */
extern "C" u32 fn_800E60B4(u8* list, u8* entry)
{
    return fn_800E60E8(fn_800E60F8(list), entry);
}

/* Whether two addresses are equal. */
extern "C" u32 fn_800E60E8(u8* a, u8* b)
{
    return b == a;
}

/* The end marker of a slot list. */
extern "C" u8* fn_800E60F8(u8* list)
{
    return list + 76;
}

/* Whether a voice list has reached its end marker. */
extern "C" u32 fn_800E6100(u8* list, u8* entry)
{
    return fn_800E6134(fn_800E6144(list), entry);
}

/* Whether two addresses are equal. */
extern "C" u32 fn_800E6134(u8* a, u8* b)
{
    return b == a;
}

/* The end marker of a voice list. */
extern "C" u8* fn_800E6144(u8* list)
{
    return list + 272;
}

/* The next voice in a list. */
extern "C" VoiceEntry* fn_800E614C(VoiceEntry* voice)
{
    return voice->next;
}

/* The slot info block of one slot list entry. */
extern "C" SlotEntry* fn_800E6154(u8* entry)
{
    return (SlotEntry*)(entry + 12);
}

/* The global slot list. */
extern "C" u8* fn_800E615C(void)
{
    return lbl_80698AF0;
}

/* Frees the voice a slot still holds when its flags say it is finished. */
extern "C" u32 fn_800E6168(void* unused, SoundOwner* owner)
{
    SoundSlot* slot = owner->slot;
    AxVoiceBlock* ax = fn_800E5F38(slot);

    if (fn_800E6238(slot, 1024)) {
        if (ax->voice != 0) {
            AXFreeVoice(ax->voice);
        }
        owner->slot = (SoundSlot*)fn_800EDA88(fn_800E49F4(), owner->slot);
        return 1;
    }
    if (fn_800E6238(slot, 512) != 1) {
        if (fn_800E6238(slot, 256) != 0) {
            AxVoice* voice = ax->voice;
            if (voice->field_38 == 0) {
                AXFreeVoice(voice);
                owner->slot = (SoundSlot*)fn_800EDA88(fn_800E49F4(), owner->slot);
                return 1;
            }
        }
    }
    return 0;
}

/* Tests whether any of a sound slot's flag bits is set. */
extern "C" u32 fn_800E6238(SoundSlot* slot, u32 mask)
{
    return (slot->flags & mask) != 0;
}

/* Advances a slot's voice timers and re-dispatches it. */
extern "C" void fn_800E6250(Sound2* self, SoundSlot* slot)
{
    AxVoiceBlock* ax = fn_800E5F38(slot);

    if (ax->field_24 != (u32)-1 && ax->field_24 != 0) {
        ax->field_24--;
    }
    if (ax->field_20 != (u32)-1) {
        if (++ax->field_2C >= ax->field_20 && ax->field_3A != 0) {
            ax->field_3A = 5;
            ax->field_20 = (u32)-1;
        }
    }
    fn_800E96BC(self, slot, ax);
    fn_800E6314(self, ax);
}

/* Starts a slot's voice from its AX block. */
extern "C" void fn_800E65EC(void* unused, SoundSlot* slot)
{
    AxVoiceBlock* ax = fn_800E5F38(slot);
    s32 a;
    s32 b;
    s32 c;
    s32 d;
    s32 e;

    fn_800EE014(ax->field_40, ax->voice, ax->field_10, ax->field_1A);
    a = fn_800E8E48(ax->field_5A);
    b = fn_800E8E48(ax->field_18);
    c = fn_800E8E48(ax->field_16);
    d = fn_800E8E48(ax->field_14);
    e = fn_800E8E48(ax->field_06);
    fn_804C2840(ax->voice, fn_800E66C8(fn_800E66D0()), e, d, c, b, ax->field_12, ax->field_4A, a);
    fn_80471610(ax->voice, 1);
    fn_800E5F18(slot, 256);
    ax->field_3A = 1;
}

/* The manager's pending-bit word. */
extern "C" u32 fn_800E66C8(ReverbMgr* mgr)
{
    return mgr->flags;
}

/* The global reverb manager. */
extern "C" ReverbMgr* fn_800E66D0(void)
{
    return &lbl_80696D90;
}

/* Fills one slot-list entry's info block. */
extern "C" void fn_800E66DC(void* unused, u32 owner, u32 kind, u8 a, u8 b, u8 c, u8 d, s16 e, s16 f)
{
    SlotEntry* info;

    if (owner == 0) {
        return;
    }
    info = fn_800E6154((u8*)fn_800E9960(fn_800E615C()));
    if (info == 0) {
        return;
    }
    info->kind = kind;
    info->owner = owner;
    info->field_08 = a;
    info->field_09 = b;
    info->field_0A = c;
    info->field_0B = d;
    info->field_0C = e;
    info->field_0E = f;
}

/* One word of a sound object. */
extern "C" u32 fn_800E7264(WordAt8* p)
{
    return p->word;
}

/* Whether a voice list has reached its end marker. */
extern "C" u32 fn_800E726C(u8* list, u8* entry)
{
    return fn_800E72A0(fn_800E6144(list), entry);
}

/* Whether two addresses are equal. */
extern "C" u32 fn_800E72A0(u8* a, u8* b)
{
    return a == b;
}

/* Silences every voice whose key matches the one the config selects. */
extern "C" void fn_800E72B0(void* unused, VoiceCfg* cfg, u32 arg)
{
    VoiceEntry* voice = (VoiceEntry*)fn_800EDB24(fn_800E49F4());
    u32 flag = cfg->field_06 & 1;
    u32 key = ((flag == 0 ? arg : 255) << 24) | (flag << 16) | ((u32)cfg->field_0B << 8) | cfg->field_0A;

    while (fn_800E6100((u8*)fn_800E49F4(), (u8*)voice) == 1) {
        if (voice->active == 1 && voice->key == key) {
            AxVoiceBlock* ax = fn_800E5F38(voice);
            ax->field_3A = 5;
        }
        voice = fn_800E614C(voice);
    }
}

/* Dispatches every voice whose key matches the one the config selects. */
extern "C" void fn_800E737C(Sound2* self, VoiceCfg* cfg, u32 arg)
{
    VoiceEntry* voice = (VoiceEntry*)fn_800EDB24(fn_800E49F4());
    u32 flag = cfg->field_06 & 1;
    u32 key = ((flag == 0 ? arg : 255) << 24) | (flag << 16) | ((u32)cfg->field_0B << 8) | cfg->field_0A;

    while (fn_800E6100((u8*)fn_800E49F4(), (u8*)voice) == 1) {
        if (voice->active == 1 && voice->key == key) {
            self->vtable->method_18(self, voice);
        }
        voice = fn_800E614C(voice);
    }
}

/* Plays one of a bank's slots, picked by a 0..99 roll. */
extern "C" void fn_800E745C(Sound2* self, void* p1, VoiceCfg* p2, u32 p3, u32 p4)
{
    SlotTable* table = (SlotTable*)fn_800E7570(p1, p2->field_02);
    s32 roll;
    u32 total;
    u32 i;

    if (table == 0 || table->count == 0) {
        return;
    }
    roll = (s32)(u8)fn_800E9D00() % 100;
    total = 0;
    for (i = 0; i < table->count; i++) {
        total += table->entries[i].weight;
        if (roll < (s32)total) {
            u8* slot = (u8*)fn_800E6A18(p1, table->entries[i].id);
            if (slot == 0) {
                return;
            }
            slot[33] = ((u8*)p2)[33];
            slot[34] = ((u8*)p2)[34];
            slot[35] = ((u8*)p2)[35];
            fn_800E6D44(self, p1, (VoiceCfg*)slot, p3, p4);
            return;
        }
    }
}

/* A relocation-table state inside a group. */
extern "C" void* fn_800E7570(void* p, u32 idx)
{
    return fn_800E7578(&((RelocGroup*)p)->state[7], idx);
}

/* Looks up one entry of a relocation table. */
extern "C" void* fn_800E7578(RelocState* state, u32 idx)
{
    if (state->table == 0) {
        return 0;
    }
    if (state->table->count <= idx) {
        return 0;
    }
    return state->entries[idx];
}

/* Constructs the global sound object. */
extern "C" Sound* fn_800E7AB0(void)
{
    return fn_800E7ABC((Sound*)lbl_80697980);
}

/* Clears a two-word block. */
extern "C" void fn_800E7CA4(Pair32* p)
{
    p->a = 0;
    p->b = 0;
}

/* Clears a two-word block. */
extern "C" void fn_800E7CB4(Pair32* p)
{
    p->a = 0;
    p->b = 0;
}

/* Clears a two-word block. */
extern "C" void fn_800E7CC4(Pair32* p)
{
    p->a = 0;
    p->b = 0;
}

/* Scales a level by the unit's factor. */
extern "C" f32 fn_800E7CD4(f32 v)
{
    return v * lbl_807964B8;
}

/* Installs a vtable. */
extern "C" void fn_800E7D24(VtblHolder* p)
{
    p->vtable = lbl_80597DF8;
}

/* Installs a relocation table into one state of the group. */
extern "C" void fn_800E7E94(void* group, RelocTable* table)
{
    fn_800E4C1C(&((RelocGroup*)group)->state[3], table);
}

/* Installs a relocation table into one state of the group. */
extern "C" void fn_800E7E9C(void* group, RelocTable* table)
{
    fn_800E4C1C(&((RelocGroup*)group)->state[2], table);
}

/* Installs a table and its fixed entry array into one state of the group. */
extern "C" void fn_800E7EA4(void* group, RelocTable* table)
{
    RelocState* state = &((RelocGroup*)group)->state[1];

    state->table = table;
    state->entries = (void**)&table->entries_16;
}

/* Installs a table and its fixed entry array into one state of the group. */
extern "C" void fn_800E7EBC(void* group, RelocTable* table)
{
    RelocState* state = &((RelocGroup*)group)->state[0];

    state->table = table;
    state->entries = (void**)&table->entries_16;
}

/* One delay line of the bank. */
extern "C" ReverbLine* fn_800E7ED0(Sound* snd, u32 idx)
{
    return &snd->lines[idx];
}

/* Installs a relocation table into one state of the group. */
extern "C" void fn_800E7F9C(void* group, RelocTable* table)
{
    fn_800E4C1C(&((RelocGroup*)group)->state[8], table);
}

/* Installs a relocation table into one state of the group. */
extern "C" void fn_800E7FA4(void* group, RelocTable* table)
{
    fn_800E4C1C(&((RelocGroup*)group)->state[7], table);
}

/* Installs a relocation table into one state of the group. */
extern "C" void fn_800E7FAC(void* group, RelocTable* table)
{
    fn_800E4C1C(&((RelocGroup*)group)->state[6], table);
}

/* Installs a relocation table into one state of the group. */
extern "C" void fn_800E7FB4(void* group, RelocTable* table)
{
    fn_800E4C1C(&((RelocGroup*)group)->state[5], table);
}

/* Installs a relocation table into one state of the group. */
extern "C" void fn_800E7FBC(void* group, RelocTable* table)
{
    fn_800E4C1C(&((RelocGroup*)group)->state[4], table);
}

/* Stores one level in a sound object's level array. */
extern "C" void fn_800E84DC(Sound* p, u32 v, u8 idx)
{
    p->levels2[idx] = v;
}

/* Sets the global sound object's level word. */
extern "C" void fn_800E84F0(s32 v)
{
    fn_800E8524(fn_800E5714(), (s16)v);
}

/* Stores the sound object's level word. */
extern "C" void fn_800E8524(Sound* snd, s16 v)
{
    snd->field_08 = v;
}

/* The global sound object's level word. */
extern "C" s16 fn_800E852C(void)
{
    return fn_800E8550(fn_800E5714());
}

/* The sound object's level word. */
extern "C" s16 fn_800E8550(Sound* snd)
{
    return snd->field_08;
}

/* Stores the global sound object's mode byte. */
extern "C" void fn_800E8558(u8 v)
{
    fn_800E858C(fn_800E5714(), v);
}

/* Stores the sound object's mode byte. */
extern "C" void fn_800E858C(Sound* snd, u8 v)
{
    snd->field_71 = v;
}

/* One element of the sound object's level table. */
extern "C" s16 fn_800E85D8(Sound* snd, u32 idx)
{
    return snd->levels[idx];
}

/* The global level setting. */
extern "C" LevelSetting* fn_800E8690(void)
{
    return &lbl_80794A04;
}

/* Installs the AX reverb hooks. */
extern "C" void fn_800E87E0(void* alloc, void* free)
{
    AXFXSetHooks(alloc, free);
}

/* The current sound handle. */
extern "C" u32 fn_800E8858(void)
{
    return fn_804C2830();
}

/* The global level setting's level word. */
extern "C" s32 fn_800E885C(void)
{
    return fn_800E8880(fn_800E8690());
}

/* The level word of a setting. */
extern "C" s16 fn_800E8880(LevelSetting* setting)
{
    return setting->level;
}

/* The network library mediator. */
extern "C" void* fn_800E89D8(void)
{
    return mpMediator__15sNetworkLibrary;
}

/* Records whether the given state is the "off" one. */
extern "C" void fn_800E89E0(u32 v)
{
    lbl_80794A08[0] = (u8)(v == 1);
}

/* Clears the reverb work area. */
extern "C" void fn_800E8D40(void)
{
    u32 size = 0x8000;
    void* base = (void*)0x90003F60;

    lbl_807949E4 = size;
    lbl_807949E8 = (u32)base;
    lbl_807949EC = 0;
    lbl_807949F0 = 0;
    lbl_807949F4 = size;
    lbl_80794A00 = 0;
    memset(base, 0, size);
}

/* Advances the reverb time counter with interrupts off. */
extern "C" void fn_800E8D74(void)
{
    u32 level = OSDisableInterrupts();

    lbl_80794A00 += 11;
    OSRestoreInterrupts(level);
}

/* Stores the maximum voice volume. */
extern "C" void fn_800E8E3C(s16* p)
{
    *p = 127;
}

/* One entry of the level table. */
extern "C" u32 fn_800E8E48(s16 idx)
{
    return lbl_80597E20[idx];
}

/* Stops a slot's voice and clears its flags. */
extern "C" void fn_800E75B0(void* unused, SoundSlot* slot)
{
    AxVoiceBlock* ax;

    if (slot == 0) {
        return;
    }
    fn_800E5F28(slot, 512);
    fn_800E5F28(slot, 1);
    fn_800E5F18(slot, 1024);
    ax = fn_800E5F38(slot);
    if (ax->voice != 0) {
        fn_80471610(ax->voice, 0);
    }
}

/* Stops a slot's voice. */
extern "C" void fn_800E7620(void* unused, SoundSlot* slot)
{
    AxVoiceBlock* ax;

    if (slot == 0) {
        return;
    }
    ax = fn_800E5F38(slot);
    if (ax->voice == 0) {
        return;
    }
    fn_800E5F18(slot, 512);
    fn_80471610(ax->voice, 0);
}

/* Releases a slot's voice once its release flag is set. */
extern "C" void fn_800E7688(void* unused, SoundSlot* slot)
{
    AxVoiceBlock* ax;

    if (slot == 0) {
        return;
    }
    if (fn_800E6238(slot, 512) != 1) {
        return;
    }
    fn_800E5F28(slot, 512);
    ax = fn_800E5F38(slot);
    if (ax->voice == 0) {
        return;
    }
    if (ax->voice->field_38 != 0) {
        return;
    }
    fn_80471610(ax->voice, 1);
}

/* Counts the voices one owner holds. */
extern "C" u32 fn_800E7700(void* unused, u32 key)
{
    VoiceEntry* voice = (VoiceEntry*)fn_800EDB24(fn_800E49F4());
    u32 count = 0;

    while (fn_800E6100((u8*)fn_800E49F4(), (u8*)voice) == 1) {
        if (voice->active == 1 && key == voice->owner) {
            count++;
        }
        voice = fn_800E614C(voice);
    }
    return count;
}

/* Applies the reverb config stored in one of the bank's lines. */
extern "C" void fn_800E778C(Sound* snd, u32 idx, u32 entry_idx)
{
    ReverbCfg* cfg = (ReverbCfg*)fn_800E77DC(&snd->lines[idx], entry_idx);

    if (cfg != 0) {
        fn_800E4FF4(fn_800E66D0(), cfg);
    }
}

/* Looks up one entry of a line's relocation table. */
extern "C" void* fn_800E77DC(void* p, u32 idx)
{
    RelocState* state = &((LineSlot*)p)->state;

    if (state->table == 0) {
        return 0;
    }
    if (state->table->count <= idx) {
        return 0;
    }
    return state->entries[idx];
}

/* Clamps and stores a slot's pan. */
extern "C" void fn_800E781C(void* unused, void* slot, s16 pan)
{
    AxVoiceBlock* ax;
    s16 v;

    if (slot == 0) {
        return;
    }
    ax = fn_800E5F38(slot);
    v = pan;
    if (v > 127) {
        v = 127;
    } else if (v < 0) {
        v = 0;
    }
    ax->field_02 = v;
}

/* Clamps a slot's pitch to its limits and applies it to the voice. */
extern "C" void fn_800E7878(void* unused, void* slot, s16 pitch)
{
    AxVoiceBlock* ax;
    s16 v;

    if (slot == 0) {
        return;
    }
    ax = fn_800E5F38(slot);
    v = pitch;
    if (v > ax->field_4C) {
        v = ax->field_4C;
    } else if (v < -ax->field_4E) {
        v = -ax->field_4E;
    }
    ax->field_10 = v;
    fn_800E8E60(ax->voice, ax->field_40->field_04, ax->field_10);
}

/* The pitch of the first voice one owner holds. */
extern "C" s16 fn_800E78F8(void* unused, u32 key)
{
    VoiceEntry* voice = (VoiceEntry*)fn_800EDB24(fn_800E49F4());
    s16 pitch = 0;

    while (fn_800E6100((u8*)fn_800E49F4(), (u8*)voice) == 1) {
        if (voice->active == 1 && key == voice->owner) {
            pitch = fn_800E5F38(voice)->field_10;
            break;
        }
        voice = fn_800E614C(voice);
    }
    return pitch;
}

/* Retunes every voice one owner holds. */
extern "C" void fn_800E7990(void* unused, u32 key, u16 pitch)
{
    VoiceEntry* voice = (VoiceEntry*)fn_800EDB24(fn_800E49F4());

    while (fn_800E6100((u8*)fn_800E49F4(), (u8*)voice) == 1) {
        if (voice->active == 1 && key == voice->owner) {
            AxVoiceBlock* ax;
            u16 p = pitch;

            if (p > 16000) {
                p = 16000;
            }
            ax = fn_800E5F38(voice);
            if (p != ax->field_1A) {
                u16 a;
                u16 b;

                if (ax->field_1E == 0) {
                    AxPitchArgs args;

                    fn_80471890(p, &a, &b);
                    args.field_00 = 1;
                    args.field_02 = 0;
                    args.field_04 = a;
                    args.field_06 = b;
                    fn_804717C0(ax->voice, &args);
                    ax->field_1E = 1;
                    ax->field_1A = p;
                } else {
                    fn_80471890(p, &a, &b);
                    fn_80471830(ax->voice, a, b);
                    ax->field_1A = p;
                }
            }
        }
        voice = fn_800E614C(voice);
    }
}

/* Constructs the global sound object. */
extern "C" Sound* fn_800E7ABC(Sound* snd)
{
    u32 i;

    fn_800E7CE0(snd);
    snd->vtable = (const SoundVtbl*)lbl_80597DA8;
    snd->field_08 = 127;
    snd->field_6E = 127;
    snd->field_6F = 127;
    snd->field_70 = 127;
    snd->field_71 = 127;
    snd->field_74 = 512;
    snd->field_78 = 0;
    snd->field_88 = lbl_807964B0;
    snd->field_8C = fn_800E7CD4(lbl_807964B4);
    __construct_array(&snd->lines[0], (void*)fn_800E7C34, (void*)dtor_800E5548, sizeof(ReverbLine), 49);
    for (i = 0; i < 49; i++) {
        snd->levels[i] = 127;
    }
    snd->levels2[0] = 0;
    snd->levels2[1] = 0;
    snd->levels2[2] = 0;
    return snd;
}

/* Constructs one delay line. */
extern "C" ReverbLine* fn_800E7C34(ReverbLine* line)
{
    fn_800E7CC4((Pair32*)&line->part[0]);
    fn_800E7CB4((Pair32*)&line->part[1]);
    fn_800E7CA4((Pair32*)&line->part[2]);
    fn_800E7CA4((Pair32*)&line->part[3]);
    fn_800E7CA4((Pair32*)&line->part[4]);
    fn_800E7CA4((Pair32*)&line->part[5]);
    fn_800E7CA4((Pair32*)&line->part[6]);
    fn_800E7CA4((Pair32*)&line->part[7]);
    fn_800E7CA4((Pair32*)&line->part[8]);
    return line;
}

/* The base constructor of the sound object. */
extern "C" Sound* fn_800E7CE0(Sound* snd)
{
    fn_800E7D24((VtblHolder*)snd);
    snd->vtable = (const SoundVtbl*)lbl_80597DD0;
    snd->unused_04 = 0;
    return snd;
}

/* Snapshots the voice state and installs whichever sound bank is pending. */
extern "C" void fn_800E7D34(void)
{
    fn_800E5F40((Sound2*)fn_800E5714());
    if (fn_800E4A00(fn_800E5714(), 1) != 0) {
        fn_800EDC50(fn_800E49F4(), 1);
        fn_800E49E0(fn_800E5714(), 1);
        return;
    }
    if (fn_800E4A00(fn_800E5714(), 2) != 0) {
        fn_800EDDB0(fn_800E49F4(), 1);
        fn_800E49E0(fn_800E5714(), 2);
        return;
    }
    if (fn_800E4A00(fn_800E5714(), 4) != 0) {
        fn_800EDE60(fn_800E49F4(), 1);
        fn_800E49E0(fn_800E5714(), 4);
    }
}

/* Installs one line's four relocation tables. */
extern "C" void fn_800E7DE4(u32 idx, RelocDesc* desc)
{
    RelocTable* t4 = (RelocTable*)(desc->reloc_04 + (u32)desc);
    RelocTable* t8 = (RelocTable*)(desc->reloc_08 + (u32)desc);
    RelocTable* tC = (RelocTable*)(desc->reloc_0C + (u32)desc);

    fn_800E7EBC(fn_800E7ED0(fn_800E5714(), idx), (RelocTable*)(desc->reloc_00 + (u32)desc));
    fn_800E7EA4(fn_800E7ED0(fn_800E5714(), idx), t4);
    fn_800E7E9C(fn_800E7ED0(fn_800E5714(), idx), t8);
    fn_800E7E94(fn_800E7ED0(fn_800E5714(), idx), tC);
    fn_800E93E0(fn_800E7ED0(fn_800E5714(), idx));
}

/* Installs one line's five relocation tables. */
extern "C" void fn_800E7EE0(u32 idx, RelocDesc* desc)
{
    RelocTable* t4 = (RelocTable*)(desc->reloc_04 + (u32)desc);
    RelocTable* t8 = (RelocTable*)(desc->reloc_08 + (u32)desc);
    RelocTable* tC = (RelocTable*)(desc->reloc_0C + (u32)desc);
    RelocTable* t10 = (RelocTable*)(desc->reloc_10 + (u32)desc);

    fn_800E7FBC(fn_800E7ED0(fn_800E5714(), idx), (RelocTable*)(desc->reloc_00 + (u32)desc));
    fn_800E7FB4(fn_800E7ED0(fn_800E5714(), idx), t4);
    fn_800E7FAC(fn_800E7ED0(fn_800E5714(), idx), t8);
    fn_800E7FA4(fn_800E7ED0(fn_800E5714(), idx), tC);
    fn_800E7F9C(fn_800E7ED0(fn_800E5714(), idx), t10);
}

/* One line's slot for a voice id, with interrupts off. */
extern "C" void* fn_800E7FC4(u32 idx, u32 key)
{
    u32 level = OSDisableInterrupts();
    void* slot = fn_800E6A18(fn_800E7ED0(fn_800E5714(), idx), key);

    OSRestoreInterrupts(level);
    return slot;
}

/* How many voices one owner holds, with interrupts off. */
extern "C" u32 fn_800E802C(u32 key)
{
    u32 level = OSDisableInterrupts();
    u32 count = fn_800E7700(fn_800E5714(), key);

    OSRestoreInterrupts(level);
    return count;
}

/* Retunes one owner's voices, with interrupts off. */
extern "C" void fn_800E8080(u32 key, u16 pitch)
{
    u32 level = OSDisableInterrupts();

    fn_800E7990(fn_800E5714(), key, pitch);
    OSRestoreInterrupts(level);
}

/* Plays a sound through the global object, with interrupts off. */
extern "C" void* fn_800E80DC(u32 a, u32 b, u32 c)
{
    u32 level = OSDisableInterrupts();
    void* r = fn_800E6764(fn_800E5714(), a, b, c);

    OSRestoreInterrupts(level);
    return r;
}

/* Plays a sound through the global object, with interrupts off. */
extern "C" void* fn_800E8150(u32 a, u32 b, u32 c, u32 d)
{
    u32 level = OSDisableInterrupts();
    void* r = fn_800E6818(fn_800E5714(), a, b, c, d);

    OSRestoreInterrupts(level);
    return r;
}

/* Plays a sound through the global object, with interrupts off. */
extern "C" void* fn_800E81BC(u32 a, u32 b, u32 c, u32 d)
{
    u32 level = OSDisableInterrupts();
    void* r = fn_800E6A58(fn_800E5714(), a, b, c, d);

    OSRestoreInterrupts(level);
    return r;
}

/* Registers one slot-list entry with no extra data, with interrupts off. */
extern "C" void fn_800E8228(u32 owner)
{
    u32 level = OSDisableInterrupts();

    fn_800E66DC(fn_800E5714(), owner, 0, 0, 0, 0, 0, 0, 0);
    OSRestoreInterrupts(level);
}

/* Marks sound bank 1 pending, with interrupts off. */
extern "C" void fn_800E8294(void)
{
    u32 level = OSDisableInterrupts();

    fn_800E4ABC(fn_800E5714(), 1);
    OSRestoreInterrupts(level);
}

/* Marks sound bank 2 pending, with interrupts off. */
extern "C" void fn_800E82D4(void)
{
    u32 level = OSDisableInterrupts();

    fn_800E4ABC(fn_800E5714(), 2);
    OSRestoreInterrupts(level);
}

/* Marks sound bank 4 pending, with interrupts off. */
extern "C" void fn_800E8314(void)
{
    u32 level = OSDisableInterrupts();

    fn_800E4ABC(fn_800E5714(), 4);
    OSRestoreInterrupts(level);
}

/* Registers a slot-list entry of kind 3, with interrupts off. */
extern "C" void fn_800E8354(u32 owner, s16 arg)
{
    u32 level = OSDisableInterrupts();

    fn_800E66DC(fn_800E5714(), owner, 3, 0, 0, 0, 0, arg, 0);
    OSRestoreInterrupts(level);
}

/* Registers a slot-list entry of kind 4, with interrupts off. */
extern "C" void fn_800E83CC(u32 owner, s16 arg)
{
    u32 level = OSDisableInterrupts();

    fn_800E66DC(fn_800E5714(), owner, 4, 0, 0, 0, 0, 0, arg);
    OSRestoreInterrupts(level);
}

/* The pitch of one owner's first voice, with interrupts off. */
extern "C" s16 fn_800E8444(u32 key)
{
    u32 level = OSDisableInterrupts();
    s16 pitch = fn_800E78F8(fn_800E5714(), key);

    OSRestoreInterrupts(level);
    return pitch;
}

/* Stores one of the global object's level words. */
extern "C" void fn_800E8498(u32 v, u8 idx)
{
    fn_800E84DC(fn_800E5714(), v, idx);
}

/* One of the global object's level words, or 0 for an out-of-range index. */
extern "C" s16 fn_800E8594(u32 idx)
{
    if (idx >= 49) {
        return 0;
    }
    return fn_800E85D8(fn_800E5714(), idx);
}

/* Releases one reverb unit, with interrupts off. */
extern "C" void fn_800E85E8(u32 idx)
{
    u32 level = OSDisableInterrupts();

    fn_800E51AC(fn_800E66D0(), idx);
    OSRestoreInterrupts(level);
}

/* Looks up one entry of a block's table at +0x40. */
extern "C" void* fn_800E6A18(void* p, u32 idx)
{
    RelocState* state = &((RelocGroup*)p)->state[8];

    if (state->table == 0) {
        return 0;
    }
    if (state->table->count <= idx) {
        return 0;
    }
    return state->entries[idx];
}

/* Dispatches a voice to the handler its kind selects. */
extern "C" void fn_800E6D44(Sound2* self, void* a, VoiceCfg* cfg, u32 c, u32 d)
{
    u8 kind = cfg->kind;

    if (kind == 1) {
        fn_800E6D8C(self, a, cfg, c, d);
        return;
    }
    if (kind == 2) {
        fn_800E72B0(self, cfg, c);
        return;
    }
    if (kind == 3) {
        fn_800E737C(self, cfg, c);
        return;
    }
    if (kind == 4) {
        fn_800E745C(self, a, cfg, c, d);
    }
}

/* Looks up one 8-byte entry of a block's table at +0x00. */
extern "C" void* fn_800E6F7C(void* p, u32 idx)
{
    RelocState* state = &((RelocGroup*)p)->state[0];

    if (state->table == 0) {
        return 0;
    }
    if (state->table->count <= idx) {
        return 0;
    }
    return (u8*)state->entries + idx * 8;
}

/* Looks up one entry of a block's table at +0x18. */
extern "C" void* fn_800E6FB8(void* p, u32 idx)
{
    RelocState* state = &((RelocGroup*)p)->state[3];

    if (state->table == 0) {
        return 0;
    }
    if (state->table->count <= idx) {
        return 0;
    }
    return state->entries[idx];
}

/* Looks up one entry of a block's table at +0x10. */
extern "C" void* fn_800E6FF8(void* p, u32 idx)
{
    RelocState* state = &((RelocGroup*)p)->state[2];

    if (state->table == 0) {
        return 0;
    }
    if (state->table->count <= idx) {
        return 0;
    }
    return state->entries[idx];
}

/* Looks up one 20-byte entry of a block's table at +0x08. */
extern "C" void* fn_800E7038(void* p, u32 idx)
{
    RelocState* state = &((RelocGroup*)p)->state[1];

    if (state->table == 0) {
        return 0;
    }
    if (state->table->count <= idx) {
        return 0;
    }
    return (u8*)state->entries + idx * 20;
}

/* Applies one reverb config from the global object's bank, with interrupts off. */
extern "C" void fn_800E8634(u32 idx, u32 entry_idx)
{
    u32 level = OSDisableInterrupts();

    fn_800E778C(fn_800E5714(), idx, entry_idx);
    OSRestoreInterrupts(level);
}

/* The per-frame sound update. */
extern "C" void fn_800E8698(void)
{
    u32 level = OSEnableInterrupts();

    fn_804DD460();
    fn_804DF140();
    fn_804C40D0();
    fn_800EDB74(fn_800E49F4());
    fn_800E7D34();
    fn_800E4934();
    OSRestoreInterrupts(level);
}

/* Sets the global voice count with interrupts off. */
extern "C" void fn_800E86E8(u32 id)
{
    u32 level = OSDisableInterrupts();

    fn_8046FDD0((u16)id);
    OSRestoreInterrupts(level);
}

/* Copies one buffer into the ring the reverb time counter walks. */
extern "C" void fn_800E8CC8(s16* src, s32 count)
{
    u32 pos = lbl_807949EC;
    s16* dst;
    s32 n;

    lbl_807949F8 = pos;
    lbl_807949FC = lbl_80794868;
    dst = (s16*)(pos + lbl_807949E8);
    n = count / 2;
    while (n-- > 0) {
        *dst++ = *src++;
        pos = lbl_807949EC + 2;
        lbl_807949EC = pos;
        if (pos % lbl_807949F4 == 0) {
            lbl_807949EC = 0;
            dst = (s16*)lbl_807949E8;
        }
    }
}

/* Initialises the global level setting and registers its destructor. */
extern "C" void fn_800E8DA4(void)
{
    fn_800E8E3C(&lbl_80794A04.level);
    __register_global_object(&lbl_80794A04, (void*)fn_800E8DE0, lbl_80698AE0);
}

/* Destroys the global level setting. */
extern "C" void* fn_800E8DE0(void* obj, s32 flags)
{
    if (obj != 0) {
        fn_8046E490(obj);
        fn_804C2800();
        if ((s16)flags > 0) {
            __dl__FPv(obj);
        }
    }
    return obj;
}

/* Brings up the audio subsystem. */
extern "C" void fn_800E8730(void)
{
    u32 level;

    AIInit(0);
    fn_8046E430(1);
    fn_804C26E0();
    fn_804DF0A0();
    fn_804DD430();
    AXRegisterCallback((void*)fn_800E8698);
    fn_8046FD90(0);
    fn_800E87E4(1);
    level = OSDisableInterrupts();
    lbl_807949D8 = fn_8046D420((void*)fn_800E8888);
    OSRestoreInterrupts(level);
    /* The same reverb work-area clear `fn_800E8D40` performs; retail has the body here too. */
    lbl_807949E4 = 0x8000;
    lbl_807949E8 = 0x90003F60;
    lbl_807949EC = 0;
    lbl_807949F0 = 0;
    lbl_807949F4 = 0x8000;
    lbl_80794A00 = 0;
    memset((void*)0x90003F60, 0, 0x8000);
}

/* Selects the output mode. */
extern "C" void fn_800E87E4(u32 mode)
{
    switch (mode) {
    case 0:
    case 1: fn_8046FD70(0); break;
    case 2: fn_8046FD70(1); break;
    case 3: fn_8046FD70(2); break;
    }
    fn_804C2820(mode);
    fn_800E5368((u32*)fn_800E66D0());
}

/* Plays a slot chain through one of the object's delay lines. */
extern "C" u32 fn_800E6C60(Sound* snd, u32 a, u32 b, u32 c)
{
    ReverbLine* line = &snd->lines[a];
    VoiceCfg* cfg = (VoiceCfg*)fn_800E6A18(line, b);

    if (cfg == 0) {
        return 0;
    }
    snd->field_78++;
    fn_800E6D44((Sound2*)snd, line, cfg, a, c);
    while (cfg->field_1D != 255) {
        cfg = (VoiceCfg*)fn_800E6A18(line, cfg->field_1D);
        if (cfg == 0) {
            break;
        }
        fn_800E6D44((Sound2*)snd, line, cfg, a, c);
    }
    return snd->field_78;
}
