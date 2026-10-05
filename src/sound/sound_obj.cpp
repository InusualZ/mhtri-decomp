/* sound/sound_obj.cpp - the global sound object and its reverb delay lines
 *
 * `.text` 0x800E5430..0x800E7D34, 65 functions written (the rest of the range is not decompiled yet).
 * Name is a GUESS: the unit owns the 0xE60 B global sound object `lbl_80697980` and the line destructors `dtor_800E54A8`/`dtor_800E5548`.
 * Each function keeps the `#pragma` state it had in its retired source.
 */

#include "sound/fn_800E8E60.h" /* fn_800E8E60 (rule 2: the owner's header) */
#include "sound/snd_reverb_mgr.h" /* snd_reverb_mgr (rule 2: the owner's header) */
#include "sound/snd_stream_reloc.h" /* snd_stream_reloc (rule 2: the owner's header) */
#include "sound/snd_voice_pool.h" /* snd_voice_pool (rule 2: the owner's header) */
#include "sound/snd_level_tbl.h" /* snd_level_tbl (rule 2: the owner's header) */
#include "sound/snd_stream_mgr.h" /* snd_stream_mgr (rule 2: the owner's header) */
#include "sound/quest_snd.h" /* quest_snd (rule 2: the owner's header) */
#include "sound/snd_bank_table_get.h" /* snd_bank_table_get (rule 2: the owner's header) */
#include "sound/snd_reverb_unit_set.h" /* snd_reverb_unit_set (rule 2: the owner's header) */
#include "types.h"
#include "sound/fn_800E46E8_types.h"
/* signatures the calls below use, when they differ from the owner header's: a cast call is the same direct call. */
#define fn_800EDB24_c1 ((void* (*)(void*))fn_800EDB24)
#define fn_800EDA88_c1 ((void* (*)(void*, void*))fn_800EDA88)

/* The 49-line bank `fn_800E5430` destroys at +0x90. */
typedef struct ReverbBank {
    /* +0x000 */ u8 unused_000[0x90];
    /* +0x090 */ ReverbLine lines[49];
} ReverbBank; /* size: 0xE58 */

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

/* Two untyped words. */
typedef struct Pair32 {
    /* +0x00 */ u32 a;
    /* +0x04 */ u32 b;
} Pair32; /* size: 0x8 */

/* A vtable pointer. */
typedef struct VtblHolder {
    /* +0x00 */ const void* vtable;
} VtblHolder; /* size: 0x4 */

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

extern f32 lbl_80796490;            /* 127.0f: the level normaliser */

extern f32 lbl_807964B8;            /* fn_800E7CD4's scale factor */
extern u8 lbl_80597DF8[0x28];       /* a vtable fn_800E7D24 installs */

extern u8 lbl_80597DA8[0x28];       /* the sound object's vtable (fn_800E7ABC) */
extern u8 lbl_80597DD0[0x28];       /* the sound object's base vtable (fn_800E7CE0) */
extern f32 lbl_807964B0;            /* fn_800E7ABC's initial field_88 */
extern f32 lbl_807964B4;            /* fn_800E7ABC's field_8C source */

extern u8 lbl_80697980[0xE60];      /* the global sound object (fn_800E5714) */

extern "C" {
ReverbBank* fn_800E5430(ReverbBank* bank, s32 flags);
void* dtor_800E54A8(void* obj, s32 flags);
void* fn_800E5504(void* obj, s32 flags);
void* dtor_800E5548(void* obj, s32 flags);
void* fn_800E5608(void* obj, s32 flags);
void* fn_800E564C(void* obj, s32 flags);
void* fn_800E5690(void* obj, s32 flags);
void fn_800E56D4(void* arg);
Sound* fn_800E5714(void);

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
void* fn_800E9B5C(u8* list, void* entry);
void* fn_800E9BE4(u8* list);
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


void* fn_800E9960(u8* list);
void fn_804C2840(AxVoice* voice, u32 a, s32 b, s32 c, s32 d, s32 e, s16 f, s16 g, s32 h);
void fn_80471610(AxVoice* voice, u32 a);

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


void fn_800E8E60(AxVoice* voice, u32 a, s16 b);

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

void fn_800E6D8C(Sound2* self, void* a, VoiceCfg* cfg, u32 c, u32 d);

u32 fn_800E6C60(Sound* snd, u32 a, u32 b, u32 c);

}


#pragma peephole off

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
        VoiceEntry* voice = (VoiceEntry*)fn_800EDB24_c1(snd_bank_table_get());

        while (fn_800E6100((u8*)snd_bank_table_get(), (u8*)voice) == 1) {
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
        owner->slot = (SoundSlot*)fn_800EDA88_c1(snd_bank_table_get(), owner->slot);
        return 1;
    }
    if (fn_800E6238(slot, 512) != 1) {
        if (fn_800E6238(slot, 256) != 0) {
            AxVoice* voice = ax->voice;
            if (voice->field_38 == 0) {
                AXFreeVoice(voice);
                owner->slot = (SoundSlot*)fn_800EDA88_c1(snd_bank_table_get(), owner->slot);
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
    return &snd_reverb_mgr;
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
    VoiceEntry* voice = (VoiceEntry*)fn_800EDB24_c1(snd_bank_table_get());
    u32 flag = cfg->field_06 & 1;
    u32 key = ((flag == 0 ? arg : 255) << 24) | (flag << 16) | ((u32)cfg->field_0B << 8) | cfg->field_0A;

    while (fn_800E6100((u8*)snd_bank_table_get(), (u8*)voice) == 1) {
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
    VoiceEntry* voice = (VoiceEntry*)fn_800EDB24_c1(snd_bank_table_get());
    u32 flag = cfg->field_06 & 1;
    u32 key = ((flag == 0 ? arg : 255) << 24) | (flag << 16) | ((u32)cfg->field_0B << 8) | cfg->field_0A;

    while (fn_800E6100((u8*)snd_bank_table_get(), (u8*)voice) == 1) {
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
    VoiceEntry* voice = (VoiceEntry*)fn_800EDB24_c1(snd_bank_table_get());
    u32 count = 0;

    while (fn_800E6100((u8*)snd_bank_table_get(), (u8*)voice) == 1) {
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
        snd_reverb_unit_set(fn_800E66D0(), cfg);
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
    VoiceEntry* voice = (VoiceEntry*)fn_800EDB24_c1(snd_bank_table_get());
    s16 pitch = 0;

    while (fn_800E6100((u8*)snd_bank_table_get(), (u8*)voice) == 1) {
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
    VoiceEntry* voice = (VoiceEntry*)fn_800EDB24_c1(snd_bank_table_get());

    while (fn_800E6100((u8*)snd_bank_table_get(), (u8*)voice) == 1) {
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
