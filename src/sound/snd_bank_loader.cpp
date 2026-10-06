/* sound/snd_bank_loader.cpp - the sound bank/file loader (SE and BGM banks)
 *
 * `.text` 0x800EEAE0..0x800F2A94, 66 functions written (the rest of the range is not decompiled yet).
 * Phase 4: fold of 2 registered units, built from `sound/fn_800E8E60.cpp`, `sound/fn_800EF7D8.cpp`.
 * Name is a GUESS: the range holds `snd_bank_layout`, `scene_se_bank_load`, `system_se_load`, `title_se_load` and the `*_bgm_load` family over the `SndWork` record.
 * Each function keeps the `#pragma` state it had in its retired source. The retired header's notes follow below.
 * NAMES. GUESS (from each body and its callers): snd_player_banks_load, snd_em_se_slot_release, snd_npc_voice_bank_load
 *   GUESS (from each body and its callers): lobby_bgm_load, snd_area_bank_load
 */

/* Retired header of `sound/fn_800EF7D8.cpp` (kept for its notes and residuals): */
/* sound/fn_800EF7D8.cpp - the sound-system SE/BGM loader cluster.
 *
 * .text 0x800EF7D8-0x800F2A94 (62 functions, 0x52BC bytes), registered once at its final home (brief section 2).
 *
 * Registration evidence (brief section 2, in order):
 *   1. no `__FILE__`/assert source-name string exists anywhere in the range (every data symbol the
 *      range references - checked by scanning all 62 `.text` blocks of the auto split and resolving
 *      each `lbl_*` through the split's own `*.s` data files - is a data-file path such as
 *      `16/envdata/se_env_m%03d.dat`, never a `.c`/`.cpp` name);
 *   2. `dumpmap.py lookup` gives only `zz_XXXXXXXX_` for the unnamed functions, which is not evidence;
 *   3. module = `sound`, class 3: the range sits in the sound link band directly above
 *      `sound/fn_800E46E8.cpp` (0x800E46E8-0x800E8E60) and every named symbol in it is the sound
 *      system's (`title_snd_init__Fv`, `system_se_load__Fv`, `title_se_load__Fv`,
 *      `demo_bgm_load__FUc`, `quest_bgm_load__FUcUc`, `movie_bgm_load__Fv`, `title_bgm_load__Fv`,
 *      `get_em_se_bank__FUc`, `set_SE_volume__FUc`, `set_BGM_volume__FUc`, `srt_ready_ck__Fl`) - so
 *      it goes in the `sound` lib block of configure.py, next to its neighbours;
 *   4. name = the map's own stem `fn_800EF7D8` (class 4): no evidence supports a better one and the
 *      `sound` siblings use exactly this scheme (`sound/fn_800D7F54.cpp`, `sound/fn_800E46E8.cpp`).
 *
 * Language: C++ (high). The map itself carries C++ manglings in this range
 * (`title_snd_init__Fv`, `set_BGM_volume__FUc`, ...) and the request records the range builds are
 * passed as C++ aggregates, so the C++ front-end is the one that produced the object.
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for 54 of this range's 62 symbols (checked
 * with `python tools/symbols/dumpmap.py lookup <addr>`: the runtime dump answers `zz_XXXXXXXX_`,
 * which is not a name), so the map's stems are kept as the definitions' names (two were later renamed:
 * `snd_bank_layout` 0x800EF7D8, `scene_se_bank_load` 0x800EF9C0).
 *
 * ## What the unit is
 *
 * The bank/file loader of the sound system.  One global carries all the state, `lbl_80794A2C`, a
 * pointer to the `SndWork` record (`SndWork` below, 0x97B8 bytes): four 49-entry file tables at
 * +0x10/+0xD4/+0x198/+0x25C (offset and size of the `16/whd/*.whd` and `16/tsb/*.tsb` file of each
 * chunk), the three 49x256 per-chunk id tables at +0x382/+0x3482/+0x6582, the ready flags at
 * +0x9682 / +0x96B3 / +0x9774, the 16 SE-slot records at +0x96E4, the six BGM slots at
 * +0x9704..+0x971B and the BGM/stream flags from +0x97A8 on.
 *
 * `snd_bank_layout` walks the six `SndChunk*` tables at `lbl_80598F68` and lays the cumulative file
 * offsets into those four tables; `fn_800F0230` / `fn_800F033C` / `fn_800F0448` are the
 * `load_file_req` completion callbacks that flip the per-chunk ready flags (and start the stream once
 * both halves are in); `system_se_load` / `title_se_load` / `demo_bgm_load` / `quest_bgm_load` /
 * `movie_bgm_load` / `title_bgm_load` are the per-scene entry points; `fn_800F2xxx` is the SE/BGM
 * runtime query set the SE request layer (`sound/fn_800D7F54.cpp`) drives.
 *
 * The request record handed to `load_file_req`'s last parameter is a `u32` word array whose length is
 * that call's 5th argument (`load_file_req` copies `min(mode, 0x10)` words out of it), so each site
 * declares `u32 args[mode]` - the word count is part of the ABI, not a guess.
 *
 * ## Result (measured with `python build/tmp/unitreport.py sound/fn_800EF7D8.cpp`, the report metric)
 *
 * Unit `fuzzy_match_percent` **92.89**, `matched_code` 5036 / 12988 B, **37 of the 62 functions
 * byte-identical** and **61 at or above the 80 % bar**.  The one below it is `fn_800F0F9C`
 * (692 B, 54.77 %) - see the residual below.
 *
 * Flags: `cflags_main` (`-O3 -inline noauto`) plus a file-scope `#pragma peephole off`, which is
 * required and measured: without it the unit is 59/62, because the peephole pass fuses
 * `clrlwi`+`slwi` into `clrlslwi` and `clrlwi`+`srwi` into `extrwi` where retail carries the pairs
 * unfused (`fn_800EFD88` 26.25 -> 100.00, `snd_area_bank_load` 71.00 -> 83.86, `set_SE_volume__FUc`
 * 76.53 -> 81.53, `fn_800F298C` 70.00 -> 83.33, `fn_800F0C74` 75.41 -> 100.00,
 * `get_em_se_bank__FUc` 75.63 -> 82.00, `fn_800F2A1C` 93.65 -> 100.00).  Retail is peephole-off in
 * this band, exactly as `sound/fn_800E46E8.cpp` records, so the pragma is a stand-in for the
 * per-unit `-opt nopeephole` cflags group the outbox requests.
 *
 * ## Residual: `fn_800F0F9C`
 *
 * The id -> stream-slot loader (692 B, 54.77 %).  Retail's shared tail (the `sprintf` plus the two
 * `load_file_req` calls) sits *between* the `kind == 1` and `kind == 2` bodies, with case 1 falling
 * into it and case 2 branching *backwards* into it - the layout a `goto found;` produces.  Rule 8
 * forbids the `goto`, and the three conformant shapes measured are:
 *
 * | shape | score |
 * | --- | --- |
 * | `switch` + a shared tail after it (the shape the source has now) | 54.77 |
 * | `if` / `else if` chain + a tail after it | 52.49 |
 * | `for (;;)` with `break` out of it + a tail after it | 52.49 |
 *
 * The dispatch, both slot searches and the tail's four table writes are the target's; what differs is
 * the block order (which drags the branch offsets and a few register choices with it).
 *
 * ## Shared types and shared files
 *
 * The record both per-actor paths walk is `pl.h`'s `_PLW`: its `equipB` / `equipB2` /
 * `equipC` / `equipD` are exactly the four `_EQUIP` records `Get_pl_type` and `fn_8027EE24` take at
 * +0x1D0 / +0x1DC / +0x1E8 / +0x1F4, and the enemy list walks it at its 0xB20 stride.  So this unit
 * includes that header rather than carrying a copy of either type.  Four of that record's fields had
 * no name (`unk000`, `unk08`, `unk00E[6]`, `pad_0x1C[1]`); they are named from this unit's evidence,
 * offset-preserving: `slot_active` (+0x000, the in-use flag), `chunk_ofs` (+0x008), `se_name_set`
 * (+0x014, the SE bank name-table selector) and `se_name_idx` (+0x01D, its index).
 *
 * `sound/fn_800E46E8.h` is new: the eleven sound-manager entry points this unit calls are
 * owned by `sound/fn_800E46E8.cpp`, so their declarations live in that owner's header (rule 2).
 * `sound/fn_800D7F54.cpp`'s local declarations of this unit's symbols become rule-2 findings once the
 * unit is registered; moving them here is a follow-up shared-file edit, filed in the outbox.
 */

#include "types.h"
#include "sound/sound_work.h"
#include "sound/fn_800F2A94.h"
#include "ef/fn_800CDB2C.h"
#include "nw4r/math.h"
#include "pl.h"
#include "sound/fn_800D7F54.h"
#include "sound/fn_800E46E8.h"
#include "unsplit/sound.h"

/* ------------------------------------------------------------------------------------------------
 * The work record
 * --------------------------------------------------------------------------------------------- */

/* One entry of a bank table: which chunk it fills and the size of the two files that go there. */
typedef struct SndChunkRec {
    /* +0x00 */ s16 chunk;     /* the chunk index, -1 ends the table */
    /* +0x02 */ u16 tsb_size;  /* the `16/tsb/...` half */
    /* +0x04 */ u32 whd_size;  /* the `16/whd/...` half */
} SndChunkRec; /* size: 0x8 */

/* One `{id, chunk}` slot of the SE bank list `fn_800F06AC` fills and `fn_800F0C74` looks up. */
typedef struct SndSeSlot {
    /* +0x00 */ u8 id;         /* 0xFF when the slot is free */
    /* +0x01 */ u8 chunk;      /* the chunk index its files were loaded into */
} SndSeSlot; /* size: 0x2 */

typedef struct SndWork {
    /* +0x0000 */ u8   pad_0000[0x04];
    /* +0x0004 */ u32  total_size;      /* the running file offset both bank walkers leave behind */
    /* +0x0008 */ u32  bank_base_2;     /* cursor `fn_800F0D88` walks from */
    /* +0x000C */ u32  bank_base_1;     /* cursor `snd_bank_layout` walks from */
    /* +0x0010 */ u32  whd_ofs[0x31];   /* per chunk: the `whd` half's offset */
    /* +0x00D4 */ u32  whd_size[0x31];  /*               ... and its size */
    /* +0x0198 */ u32  tsb_ofs[0x31];   /* per chunk: the `tsb` half's offset */
    /* +0x025C */ u32  tsb_size[0x31];  /*               ... and its size */
    /* +0x0320 */ u8   pad_0320[0x62];
    /* +0x0382 */ u8   chunk_id_1[0x31][0x100]; /* fn_800F25DC/2680 write and read it */
    /* +0x3482 */ u8   chunk_id_2[0x31][0x100]; /* the `fn_800F26B8` sibling (arg2 != 0) */
    /* +0x6582 */ u8   chunk_id_3[0x31][0x100]; /* the `fn_800F26B8` sibling (arg2 == 0) */
    /* +0x9682 */ u8   whd_ready[0x31]; /* set by the `fn_800F0230` callback for the `whd` half */
    /* +0x96B3 */ u8   tsb_ready[0x31]; /* set by the `fn_800F0230` callback for the `tsb` half */
    /* +0x96E4 */ SndSeSlot se_slot[0x10];
    /* +0x9704 */ u8   se_slot_id[6];   /* fn_800F1250 / snd_em_se_slot_release / fn_800F0F9C */
    /* +0x970A */ u8   se_slot_num[6];  /* its refcount */
    /* +0x9710 */ u8   se_slot_bank[6]; /* get_em_se_bank returns it */
    /* +0x9716 */ u8   se_slot_idx[6];  /* the request id fn_800F0F9C loads for it */
    /* +0x971C */ u8   se_auto[2];      /* fn_800F0D88's {start, count} */
    /* +0x971E */ u8   chunk_handle[0x31]; /* snd_bank_layout clears it to 0xFF; nothing reads it here */
    /* +0x974F */ u8   pad_974F[0x25];
    /* +0x9774 */ u8   chunk_ready[0x31]; /* both halves are in */
    /* +0x97A5 */ u8   stream_ready[3];  /* the three BGM streams (`fn_800F0448`, `srt_ready_ck`) */
    /* +0x97A8 */ u8   demo_id;
    /* +0x97A9 */ u8   se_param_a;       /* fn_800F0560 caches the pair it was last called with */
    /* +0x97AA */ u8   se_param_b;
    /* +0x97AB */ u8   load_count;
    /* +0x97AC */ u8   pad_97AC[0x02];
    /* +0x97AE */ u8   bgm_loading_1;
    /* +0x97AF */ u8   bgm_loading_2;
    /* +0x97B0 */ u32  stream_dma;       /* the address `fn_800F2900` hands out */
    /* +0x97B4 */ u32  stream_ok;
} SndWork; /* size: 0x97B8 */

/* The `whd` header `fn_800F02D4` fixes up: a flag, a slot count and 0x48-byte slots. */
typedef struct SndWhdSlot {
    /* +0x00 */ u8  unused_00[0x3C];
    /* +0x3C */ u32 field_3C;
    /* +0x40 */ u8  unused_40[0x06];
    /* +0x46 */ s16 field_46;
} SndWhdSlot; /* size: 0x48 */

typedef struct SndWhdHeader {
    /* +0x00 */ s32 flag;
    /* +0x04 */ s32 count;
    /* +0x08 */ SndWhdSlot slots[1]; /* `count` of them */
} SndWhdHeader; /* size: 0x8 + 0x48 * count */

/* The per-voice record `fn_800E7FC4` / `fn_800E4F4C` hand back; the owner declares them `void*`, so
 * this is the field view the accesses below trace. */
typedef struct SndVoice {
    /* +0x00 */ u8  unused_00[0x0B];
    /* +0x0B */ s8  detune;      /* fn_800F27C4 */
    /* +0x0C */ u8  field_0C;    /* fn_800F25DC copies it out */
    /* +0x0D */ u8  unused_0D[0x0D];
    /* +0x1A */ u16 pitch;       /* fn_800F284C */
    /* +0x1C */ u8  unused_1C[0x04];
    /* +0x20 */ u16 loop_len;    /* fn_800F2924 (through fn_800E4F4C) */
    /* +0x22 */ u8  unused_22[0x02];
    /* +0x24 */ u16 pan;         /* fn_800F2540 copies the four out */
    /* +0x26 */ u16 field_26;
    /* +0x28 */ u16 field_28;
    /* +0x2A */ u16 field_2A;
    /* +0x2C */ u8  unused_2C[0x0F];
    /* +0x3B */ u8  unused_3B[0x0D];
} SndVoice; /* size: 0x48 (approximate: only the fields below 0x2C are touched) */

/* The record `fn_800E7FC4`'s `pan`/`field_21`/`field_22` accesses belong to: the two 0x21/0x22 bytes
 * sit inside `60VoiceCfg` in the owner, which is `void*` at this boundary. */
typedef struct SndVoiceCfg {
    /* +0x00 */ u8 unused_00[0x0B];
    /* +0x0B */ u8 field_0B;
    /* +0x0C */ u8 field_0C;
    /* +0x0D */ u8 unused_0D[0x14];
    /* +0x21 */ u8 field_21;
    /* +0x22 */ u8 field_22;
} SndVoiceCfg; /* size: 0x24 (the owner's `VoiceCfg`) */

/* The five `{offset, size}` pairs `Get_pl_type` selects a player's `whd`/`tsb` files from. */
typedef struct SndPlSizes {
    /* +0x00 */ u32 whd_size;
    /* +0x04 */ u32 tsb_size;
    /* +0x08 */ u32 field_08;
    /* +0x0C */ u32 field_0C;
} SndPlSizes; /* size: 0x10 */

/* A caller record `fn_800F1398` reads one flag out of. */
typedef struct SndFlag3 {
    /* +0x00 */ u8 unused_00[0x03];
    /* +0x03 */ u8 flag;
} SndFlag3; /* size: 0x4 */

/* ------------------------------------------------------------------------------------------------
 * Foreign symbols (docs/plan.md 6.5 rule 2: every one of these resolves to an *unsplit* map address
 * whose bracketing registered units name different modules, so there is no sound header to move the
 * declaration to - the lint counts them as the documented rule-2 gap).
 * --------------------------------------------------------------------------------------------- */

/* The file loader (`sound/fn_800DCFEC.c`'s neighbour band).  Its 5th argument is the number of words
 * it copies out of the 6th, which is why each call site declares `u32 args[n]` of that size. */
void load_file_req(char* path, u32 dma, s32 size, u32 cb, s32 words, u32* args);

extern "C" u8  fn_8027EE24(void* equip);
u8  Get_pl_type(_EQUIP* a, _EQUIP* b);
void cnvt_eur_fname(char* dst, char* src);
extern "C" u8 fn_8028F288(void);
extern "C" u8  stage_map_kind_get(u8 a);
extern "C" u8  quest_pair_table_get(u8 a, u8 b);
extern "C" u32 demo_play_ck(void);
u8  get_now_mapno(void);
void get_gm_daynight(void);
u32 get_move_work_adrs(u8 kind);
u16 get_move_work_max(u8 kind);

extern "C" int sprintf(char* dst, const char* fmt, ...);
extern "C" char* strcat(char* dst, const char* src);

/* ------------------------------------------------------------------------------------------------
 * Data (all unsplit; the data pass owns the ranges, this unit only reads them)
 * --------------------------------------------------------------------------------------------- */
extern SndWork* lbl_80794A2C;
extern void* lbl_80794A30;
extern void* lbl_80794A34;
extern u8 lbl_80794A38[3];
extern char lbl_806A1138[];
extern char lbl_806A1178[];
extern const f32 lbl_80796588; /* 127.0f - the MIDI volume scale */
extern const f32 lbl_8079658C; /* 0.0f  - the clamped floor */
extern const f32 lbl_80796590; /* 1.0f  - the clamped ceiling */
extern const char* lbl_80791590;
extern const char* lbl_807915E8[2];
extern const u8 lbl_8079160C[4];
extern const u8 lbl_80791610[8];
extern const char* lbl_80791638[2];
extern const char lbl_80791640[];
extern const char lbl_80791648[];
extern u8 system_w[];

extern SndChunkRec* lbl_80598F68[6];
extern const u8* lbl_80597554[];
extern const u8* lbl_805977FC[];
extern const u8* lbl_805978E8[];
extern const u8* lbl_80597990[];
extern const u8* lbl_80599038[];
extern const char* lbl_805980A8[];
extern const char* lbl_80598174[];
extern const char* lbl_80598190[];
extern const char* lbl_805981C8[];
extern const char* lbl_80598200[];
extern const char* lbl_80598350[];
extern const char* lbl_805984E0[];
extern const char* lbl_80598530[];
extern const char* lbl_8059853C[];
extern const char* lbl_80598560[];
extern const char* lbl_805988E0[];
extern const char* lbl_80598A2C[];
extern SndPlSizes lbl_80598A48[];
extern const u8 lbl_805990F0[];
extern const char* lbl_8059A554[];
extern const char* lbl_8059AA14[];
extern const char* lbl_8059AAFC[];
extern const char* lbl_8059AC98[];
extern const char* lbl_8059AEB0[];
extern const char* lbl_8059AF0C[];
extern const u8 lbl_8059AF68[];

/* One demo's stream shape: which of the two streams it carries and which name variant to use. */
typedef struct SndDemoCfg {
    /* +0x00 */ u8 bgm;          /* the demo carries the BGM stream */
    /* +0x01 */ u8 bgm_variant;  /* use the `_N` stream name */
    /* +0x02 */ u8 se;           /* the demo carries the SE stream */
    /* +0x03 */ u8 se_variant;   /* use the `_n` SE stream name */
} SndDemoCfg; /* size: 0x4 */

extern const SndDemoCfg lbl_8059AF80[];
extern const u8 lbl_80598AD8[];

extern const char lbl_80598F80[]; /* "16/envdata/se_env_m%03d.dat" */
extern const char lbl_80598F9C[]; /* "16/whd/%s.whd" */
extern const char lbl_80598FC8[]; /* "16/tsb/%s.tsb" */
extern const char lbl_80599018[]; /* "16/whd/%s" */
extern const char lbl_80599024[]; /* "16/tsb/vo_all.tsb" */
extern const char lbl_8059905C[]; /* "16/tsb/%s_com.tsb" */
extern const char lbl_80599070[]; /* "16/whd/%s_com.whd" */
extern const char lbl_80599084[]; /* "16/whd/%s_com_%s.whd" */
extern const char lbl_8059909C[]; /* "16/tsb/wp_%s.tsb" */
extern const char lbl_805990B0[]; /* "16/whd/%s%03d.whd" */
extern const char lbl_805990C4[]; /* "16/whd/%s%03d_e.whd" */
extern const char lbl_805990D8[]; /* "16/tsb/footstep_tsb.tsb" */
extern const char lbl_805990FC[]; /* "c_npc_nekotaku" */
extern const char lbl_8059910C[]; /* "c_npc_sansai" */
extern const char lbl_80599120[]; /* "16/tsb/Rtime_jingle.tsb" */
extern const char lbl_80599150[]; /* "16/whd/M%02d_%02d.whd" */
extern const char lbl_80599168[]; /* "16/tsb/M%02d_%02d.tsb" */
extern const char lbl_80599180[]; /* "16/whd/title.whd" */
extern const char lbl_80599194[]; /* "16/tsb/title.tsb" */
extern const char lbl_8059B044[]; /* "16/srt/demo/MH3_DEMO_SE_%02d" */
extern const char lbl_8059B064[]; /* "16/srt/demo/stream/demo_%02d_se.ssd" */
extern const char lbl_8059B088[]; /* "16/srt/demo/stream/demo_%02d_se_n.ssd" */
extern const char lbl_8059B0B0[]; /* "16/srt/demo/MH3_DEMO_BGM_%02d" */
extern const char lbl_8059B0D0[]; /* "16/srt/demo/stream/demo_%02d_bgm.ssd" */
extern const char lbl_8059B0F8[]; /* "16/srt/bgm/MH3BGM_QUEST.srt" */
extern const char lbl_8059B114[]; /* "16/srt/bgm/MH3_QUEST_END.srt" */
extern const char lbl_8059B134[]; /* "16/srt/demo/MH3_MOVIE.srt" */
extern const char lbl_8059B150[]; /* "16/srt/bgm/MH3BGM_LOBBY.srt" */

/* ------------------------------------------------------------------------------------------------
 * This unit's own symbols (plain map names - the remaining `fn_*` stems are the map's own placeholders and
 * `snd_bank_layout` / `scene_se_bank_load` were renamed from two of them - so they are declared `extern "C"`
 * exactly as the map spells them).
 * --------------------------------------------------------------------------------------------- */
extern "C" void snd_bank_layout(u8 mode);
extern "C" void scene_se_bank_load(u8 a, u8 b);
extern "C" void snd_player_banks_load(u8 arg0);
extern "C" void fn_800EFC68(_PLW* work);
extern "C" s32 fn_800EFD88(u8 id, u32 kind);
extern "C" void fn_800EFDD8(_PLW* work);
extern "C" void fn_800F0230(void* unused_0, void* unused_1, void* unused_2, u32* args);
extern "C" void fn_800F02D4(void* unused_0, SndWhdHeader* header);
extern "C" void fn_800F033C(void* unused_0, void* unused_1, void* unused_2, u32* args);
extern "C" void fn_800F0410(s32 idx);
extern "C" void fn_800F0448(void* ctx, void* unused_1, void* unused_2, u32* args);
extern "C" s32 fn_800F04FC(s32 idx);
extern "C" void snd_area_bank_load(u8 kind, u8 index);
extern "C" void fn_800F0554(void);
extern "C" void fn_800F0560(u8 a, u8 b);
extern "C" void fn_800F06AC(u8 kind, u8 arg1);
extern "C" u32 fn_800F08E0(void);
extern "C" void fn_800F08E8(u8 arg0);
extern "C" void fn_800F0AD0(u8 arg0);
extern "C" s8 fn_800F0C14(s32 chunk);
extern "C" u8 fn_800F0C74(u8 id);
extern "C" void fn_800F0D88(u8 kind);
extern "C" void fn_800F0EA0(void);
extern "C" void snd_em_se_slot_release(u8 id);
extern "C" void fn_800F0F9C(u8 id);
extern "C" s32 fn_800F1250(u8 id);
extern "C" u8 fn_800F1398(SndFlag3* work, u8 kind);
extern "C" void snd_npc_voice_bank_load(u8 arg0, u8 arg1);
extern "C" void fn_800F15B0(void);
extern "C" void fn_800F1620(s32 arg0);
extern "C" void se_slot_req(u8 arg0);
extern "C" void fn_800F1700(u8 arg0, u8 arg1, u8 arg2);
extern "C" s32 fn_800F1DE0(void);
extern "C" void fn_800F1FCC(void);
extern "C" void fn_800F2114(void);
extern "C" void lobby_bgm_load(void);
extern "C" void fn_800F2328(u8 arg0, u8 arg1);
extern "C" void fn_800F2468(u8 arg0);
extern "C" void fn_800F2540(u32 arg0, s32 arg1, SndVoice* out);
extern "C" void fn_800F25DC(u32 arg0);
extern "C" u8 fn_800F2680(u32 arg0, u32 arg1);
extern "C" u8 fn_800F26B8(u32 arg0, u32 arg1, u8 arg2);
extern "C" s32 fn_800F2714(u32 arg0, s32 arg1, u8 arg2, s32 arg3);
extern "C" s32 fn_800F27C4(u32 arg0, s32 arg1, s8 arg2);
extern "C" u16 fn_800F284C(u32 arg0);
extern "C" s32 fn_800F2890(u32 arg0, s32 arg1);
extern "C" u32 fn_800F2900(void);
extern "C" u16 fn_800F2924(s32 arg0, s32 arg1);
extern "C" u8 fn_800F298C(void);
extern "C" void fn_800F2A1C(u8 volume, u8 is_bgm);
extern "C" u8 fn_800F2A84(void);

void title_snd_init(void);
void system_se_load(void);
void title_se_load(void);
s32 demo_bgm_load(u8 index);
void quest_bgm_load(u8 arg0, u8 arg1);
void movie_bgm_load(void);
void title_bgm_load(void);
u8 get_em_se_bank(u8 id);
s32 srt_ready_ck(s32 stream);
void set_SE_volume(u8 index);
void set_BGM_volume(u8 index);

#pragma peephole off

/* Releases a resource of kind 4. */
extern "C" void fn_800EEEF4(void* p)
{
    MEMGetAllocatableSizeForExpHeapEx(p, 4);
}

/* The Chacha companion's "kamen" voice bank. */
s32 get_chacha_kamen_bank(void)
{
    return 36;
}

/* The Chacha companion's first dance voice bank. */
s32 get_chacha_dance1_bank(void)
{
    return 32;
}

/* The Chacha companion's second dance voice bank. */
s32 get_chacha_dance2_bank(void)
{
    return 33;
}

/* ------------------------------------------------------------------------------------------------
 * Bodies
 * --------------------------------------------------------------------------------------------- */

/* Lays the six bank tables out from the current file cursor.  Chunk 0 is skipped (the system SE
 * bank, `system_se_load` loads it by hand). */
extern "C" void snd_bank_layout(u8 mode)
{
    SndChunkRec* rec = lbl_80598F68[4];
    u32 cur = lbl_80794A2C->bank_base_1;

    while (rec->chunk != -1) {
        if (rec->chunk != 0) {
            lbl_80794A2C->whd_ofs[rec->chunk] = cur;
            lbl_80794A2C->whd_size[rec->chunk] = rec->whd_size;
            cur += rec->whd_size;
            lbl_80794A2C->tsb_ofs[rec->chunk] = cur;
            lbl_80794A2C->tsb_size[rec->chunk] = rec->tsb_size;
            cur += rec->tsb_size;
        }
        rec++;
    }
    lbl_80794A2C->total_size = cur;
    if (mode != 1 && (system_w[0x7D3] == 1 || GameMode_ck() == 3)) {
        rec = lbl_80598F68[5];
        while (rec->chunk != -1) {
            if (rec->chunk != 0) {
                lbl_80794A2C->whd_ofs[rec->chunk] = cur;
                lbl_80794A2C->whd_size[rec->chunk] = rec->whd_size;
                cur += rec->whd_size;
                lbl_80794A2C->tsb_ofs[rec->chunk] = cur;
                lbl_80794A2C->tsb_size[rec->chunk] = rec->tsb_size;
                cur += rec->tsb_size;
            }
            rec++;
        }
        lbl_80794A2C->total_size = cur;
    }
    {
        s32 i = 1;
        do {
            lbl_80794A2C->chunk_handle[i] = 0xFF;
            fn_800F0410(i);
            i++;
        } while (i < 0x31);
    }
    lbl_80794A2C->stream_dma = 0;
    lbl_80794A2C->demo_id = 0xFF;
    lbl_80794A2C->bank_base_2 = cur;
}

/* Clears the three ready flags of one chunk. */
extern "C" void fn_800F0410(s32 idx)
{
    lbl_80794A2C->chunk_ready[idx] = 0;
    lbl_80794A2C->tsb_ready[idx] = 0;
    lbl_80794A2C->whd_ready[idx] = 0;
}

/* Whether one of the three BGM streams has its `srt` in. */
s32 srt_ready_ck(s32 stream)
{
    return lbl_80794A2C->stream_ready[stream] - 1 == 0;
}

/* Whether one chunk has both halves in. */
extern "C" s32 fn_800F04FC(s32 idx)
{
    return lbl_80794A2C->chunk_ready[idx] - 1 == 0;
}

/* The `whd`/`tsb` completion callback: marks the half, and when both are in releases the chunk. */
extern "C" void fn_800F0230(void* unused_0, void* unused_1, void* unused_2, u32* args)
{
    u32 ctx = args[1];
    u32 chunk = args[2];

    if (args[0] == 0) {
        lbl_80794A2C->tsb_ready[chunk] = 1;
    } else {
        lbl_80794A2C->whd_ready[chunk] = 1;
    }
    if (lbl_80794A2C->tsb_ready[chunk] == 1 && lbl_80794A2C->whd_ready[chunk] == 1) {
        fn_800EEA44(ctx, chunk);
        lbl_80794A2C->chunk_ready[chunk] = 1;
    }
}

/* The `se_env` completion callback: fixes the slot table the `whd` header carries. */
extern "C" void fn_800F02D4(void* unused_0, SndWhdHeader* header)
{
    if (header == NULL) {
        return;
    }
    if (header->flag == 0) {
        return;
    }
    if (header->count == 0) {
        return;
    }
    {
        SndWhdSlot* slot = &header->slots[0];
        s32 i = 0;
        for (; i < header->count; i++) {
            slot->field_3C = slot->field_46;
            slot->field_46 = 0;
            slot++;
        }
    }
    lbl_80794A2C->stream_ok = 1;
}

/* The stream completion callback: one `srt` half is in, and once both are the stream is released. */
extern "C" void fn_800F033C(void* unused_0, void* unused_1, void* unused_2, u32* args)
{
    u32 chunk = args[1];
    u32 stream = args[2];
    u32 id = args[3];

    if (args[0] == 0) {
        lbl_80794A2C->tsb_ready[chunk] = 1;
        lbl_80794A2C->chunk_ready[stream] = 1;
    } else {
        lbl_80794A2C->whd_ready[chunk] = 1;
        lbl_80794A2C->demo_id = id;
        lbl_80794A2C->chunk_ready[stream] = 1;
    }
    if (lbl_80794A2C->tsb_ready[chunk] == 1 && lbl_80794A2C->whd_ready[chunk] == 1) {
        fn_800EEA44(chunk, stream);
        fn_800E80DC(chunk, 0, 0);
    }
}

/* The `srt` completion callback: installs the stream and flips the ready flag. */
extern "C" void fn_800F0448(void* ctx, void* unused_1, void* unused_2, u32* args)
{
    u32 a = args[0];
    u32 b = args[1];
    u32 c = args[2];
    u32 d = args[3];
    u32 e = args[4];

    fn_800E4B4C(a, ctx, b, c);
    lbl_80794A2C->stream_ready[a] = 1;
    if (d != 0) {
        fn_800E4B4C(e, ctx, b, c);
        lbl_80794A2C->stream_ready[e] = 1;
    }
}

/* The "no bank" SE state: bank 0, 30 voices. */
extern "C" void fn_800F0554(void)
{
    fn_800F0560(0, 0x1E);
}

/* The SE bank selector: only tells the mixer when the pair actually moves. */
extern "C" void fn_800F0560(u8 a, u8 b)
{
    if (lbl_80794A2C->se_param_a == a && lbl_80794A2C->se_param_b == b) {
        return;
    }
    fn_800E8294();
    fn_800E85E8(0);
    fn_800E85E8(1);
    lbl_80794A2C->se_param_a = a;
    lbl_80794A2C->se_param_b = b;
    fn_800E8634(0, lbl_80794A2C->se_param_a);
    fn_800E8634(0, lbl_80794A2C->se_param_b);
}

/* The scene SE-bank loader: bank catalogue, per-kind banks, then the SE bank itself. */
extern "C" void scene_se_bank_load(u8 a, u8 b)
{
    char name[0x100];

    snd_area_bank_load(a, 0);
    fn_800F08E8(a);
    if (GameMode_ck() != 3) {
        fn_800F2328(a, b);
    }
    lbl_80794A2C->stream_dma = 0x92D1E000;
    lbl_80794A2C->stream_ok = 0;
    if ((s32)lbl_80794A2C->stream_dma != 0) {
        sprintf(name, lbl_80598F80, a);
        if (fn_800CED10(name, lbl_80794A2C->stream_dma, 0x8000) != 0) {
            load_file_req(name, lbl_80794A2C->stream_dma, 0x8000, (u32)fn_800F02D4, 0, 0);
        }
    }
}

/* One enemy slot's files: one `whd` per chunk plus the shared `tsb`. */
extern "C" void snd_player_banks_load(u8 arg0)
{
    char name[0x88];
    u32 args[3];
    _PLW* work = (_PLW*)(u32)get_move_work_adrs(2);
    u16 max = get_move_work_max(2);

    if (work != NULL && max != 0) {
        _PLW* p = work;
        s32 i;
        for (i = 0; i < max; i++) {
            if (p->slot_active != 0) {
                s32 chunk = p->chunk_ofs + 0x14;
                args[1] = chunk;
                args[2] = chunk;
                fn_800F0410(chunk);
                lbl_80794A2C->load_count++;
                if (p->se_name_set == 0) {
                    sprintf(name, lbl_80599018, lbl_80598350[p->se_name_idx]);
                } else {
                    sprintf(name, lbl_80599018, lbl_805984E0[p->se_name_idx]);
                }
                args[0] = 1;
                load_file_req(name, lbl_80794A2C->whd_ofs[chunk], lbl_80794A2C->whd_size[chunk],
                              (u32)fn_800F0230, 3, args);
                args[0] = 0;
                load_file_req((char*)lbl_80599024, lbl_80794A2C->tsb_ofs[chunk],
                              lbl_80794A2C->tsb_size[chunk], (u32)fn_800F0230, 3, args);
            }
            p++;
        }
        if (GameMode_ck() == 0 || GameMode_ck() == 3) {
            p = work;
            for (i = 0; i < max; i++) {
                fn_800EFDD8(p);
                p++;
            }
        }
    }
}

/* The same for one already-selected slot. */
extern "C" void fn_800EFC68(_PLW* work)
{
    char name[0x88];
    u32 args[3];
    s32 chunk = work->chunk_ofs + 0x14;

    args[1] = chunk;
    args[2] = chunk;
    fn_800F0410(chunk);
    lbl_80794A2C->load_count++;
    if (work->se_name_set == 0) {
        sprintf(name, lbl_80599018, lbl_80598350[work->se_name_idx]);
    } else {
        sprintf(name, lbl_80599018, lbl_805984E0[work->se_name_idx]);
    }
    args[0] = 1;
    load_file_req(name, lbl_80794A2C->whd_ofs[chunk], lbl_80794A2C->whd_size[chunk],
                  (u32)fn_800F0230, 3, args);
    args[0] = 0;
    load_file_req((char*)lbl_80599024, lbl_80794A2C->tsb_ofs[chunk],
                  lbl_80794A2C->tsb_size[chunk], (u32)fn_800F0230, 3, args);
}

/* Whether the id is one the `kind` table carries. */
extern "C" s32 fn_800EFD88(u8 id, u32 kind)
{
    const u8* p = lbl_80599038[kind];

    if (p == NULL) {
        return 0;
    }
    while (*p != 0xFF) {
        if (id == *p) {
            return 1;
        }
        p++;
    }
    return 0;
}

/* A player slot's files: the `com` pair plus up to three per-EQUIP `whd`s. */
extern "C" void fn_800EFDD8(_PLW* work)
{
    char name[0x88];
    u32 args[3];
    _EQUIP* eq;
    u8 type;
    u8 variant;
    u32 chunk;
    SndPlSizes* sizes;

    if (work->slot_active == 0) {
        return;
    }
    eq = &work->equipB;
    {
        type = Get_pl_type(eq, eq + 2);
        if ((s32)type > 8) {
            return;
        }
        variant = fn_8027EE24(eq);
    }
    chunk = lbl_8079160C[work->chunk_ofs];
    sizes = &lbl_80598A48[type];
    args[1] = chunk;
    args[2] = chunk;
    fn_800F0410(chunk);
    lbl_80794A2C->load_count++;
    {
        const u8* tbl = lbl_80597554[type];
        u8 sub = 0;
        if (tbl != NULL) {
            sub = tbl[variant];
        }
        if (sub == 0) {
            sub = 1;
        }
        args[0] = 0;
        sprintf(name, lbl_8059905C, lbl_8059853C[type]);
        load_file_req(name, lbl_80794A2C->tsb_ofs[chunk], lbl_80794A2C->tsb_size[chunk],
                      (u32)fn_800F0230, 3, args);
        args[0] = 1;
        if (type - 4 <= 2U) {
            sprintf(name, lbl_80599070, lbl_8059853C[type]);
        } else {
            sprintf(name, lbl_80599084, lbl_8059853C[type], lbl_807915E8[sub - 1]);
        }
        load_file_req(name, lbl_80794A2C->whd_ofs[chunk], sizes->whd_size, (u32)fn_800F0230, 3,
                      args);
    }
    {
        u32 next = chunk + 1;
        lbl_80794A2C->whd_size[next] = sizes->whd_size + lbl_80794A2C->whd_ofs[next];
    }
    if (type - 4 <= 2U) {
        s32 i;
        for (i = 0; i < 3; i++) {
            u8 var = variant;
            u32 idx;
            if (i != 0) {
                var = fn_8027EE24(&eq[1 + i]);
            }
            args[1] = chunk + 1 + i;
            args[2] = chunk + 1 + i;
            fn_800F0410((s32)args[2]);
            lbl_80794A2C->load_count++;
            args[0] = 0;
            sprintf(name, lbl_8059909C, lbl_80598530[i]);
            load_file_req(name, lbl_80794A2C->tsb_ofs[args[2]], lbl_80794A2C->tsb_size[args[2]],
                          (u32)fn_800F0230, 3, args);
            args[0] = 1;
            if (fn_800EFD88(var, type) == 0) {
                sprintf(name, lbl_805990B0, lbl_80598530[i], var);
            } else {
                sprintf(name, lbl_805990C4, lbl_80598530[i], var);
            }
            idx = args[2];
            load_file_req(name, lbl_80794A2C->whd_ofs[idx], sizes->tsb_size, (u32)fn_800F0230, 3,
                          args);
            if (i != 2) {
                lbl_80794A2C->whd_size[idx] = sizes->tsb_size + lbl_80794A2C->whd_ofs[idx];
            }
            sizes++;
        }
        return;
    }
    args[1] = chunk + 1;
    args[2] = chunk + 1;
    fn_800F0410((s32)args[2]);
    lbl_80794A2C->load_count++;
    args[0] = 0;
    sprintf(name, lbl_80598FC8, lbl_8059853C[chunk]);
    load_file_req(name, lbl_80794A2C->tsb_ofs[args[2]], lbl_80794A2C->tsb_size[args[2]],
                  (u32)fn_800F0230, 3, args);
    args[0] = 1;
    if (fn_800EFD88(variant, type) == 0) {
        sprintf(name, lbl_805990B0, lbl_80598560[chunk], variant);
    } else {
        sprintf(name, lbl_805990C4, lbl_80598560[chunk], variant);
    }
    load_file_req(name, lbl_80794A2C->whd_ofs[args[2]], sizes->tsb_size, (u32)fn_800F0230, 3, args);
}

void title_snd_init(void)
{
    fn_800F0554();
    snd_bank_layout(0);
    title_se_load();
}

extern "C" void snd_area_bank_load(u8 kind, u8 index)
{
    const u8* p = lbl_805978E8[kind];

    if (p == NULL) {
        return;
    }
    fn_800F0560(p[index * 2], p[index * 2 + 1]);
}

void system_se_load(void)
{
    char name[0x88];

    sprintf(name, lbl_80598F9C, lbl_80791590);
    load_file(name, lbl_80794A2C->whd_ofs[0], lbl_80794A2C->whd_size[0]);
    sprintf(name, lbl_80598FC8, lbl_80791590);
    load_file(name, lbl_80794A2C->tsb_ofs[0], lbl_80794A2C->tsb_size[0]);
    fn_800EEA44(0, 0);
    lbl_80794A2C->chunk_ready[0] = 1;
    fn_800F0560(0, 0x1E);
}

/* The per-kind SE bank list: 16 `{id, chunk}` records starting at chunk 4. */
extern "C" void fn_800F06AC(u8 kind, u8 arg1)
{
    char name[0x88];
    u32 args[3];
    const u8* p;
    s32 i;
    s32 n;
    u8 chunk;

    for (i = 0; i < 0x10; i++) {
        lbl_80794A2C->se_slot[i].id = 0;
        lbl_80794A2C->se_slot[i].chunk = 0xFF;
    }
    p = lbl_805977FC[kind];
    if (p == NULL) {
        return;
    }
    n = 0;
    chunk = 4;
    for (;;) {
        u8 id = p[0];
        u8 bank;
        if (id == 0xFF) {
            break;
        }
        bank = p[1];
        p += 2;
        lbl_80794A2C->se_slot[n].id = id;
        lbl_80794A2C->se_slot[n].chunk = chunk;
        args[1] = chunk;
        args[2] = chunk;
        fn_800F0410(chunk);
        lbl_80794A2C->load_count++;
        args[0] = 1;
        sprintf(name, lbl_80599018, lbl_805988E0[bank]);
        load_file_req(name, lbl_80794A2C->whd_ofs[chunk], lbl_80794A2C->whd_size[chunk],
                      (u32)fn_800F0230, 3, args);
        args[0] = 0;
        load_file_req((char*)lbl_805990D8, lbl_80794A2C->tsb_ofs[chunk],
                      lbl_80794A2C->tsb_size[chunk], (u32)fn_800F0230, 3, args);
        n++;
        if (n >= 0x10) {
            break;
        }
        chunk++;
        if (chunk >= 0x14) {
            break;
        }
    }
}

extern "C" u32 fn_800F08E0(void)
{
    return 0x20;
}

extern "C" void fn_800F08E8(u8 arg0)
{
    char name[0x88];
    u32 args[3];
    s32 i;
    const char* const* tbl;

    for (i = 1; i < 4; i++) {
        args[1] = i;
        args[2] = i;
        fn_800F0410(i);
        args[0] = 1;
        sprintf(name, lbl_80598F9C, lbl_80598190[i - 1]);
        load_file_req(name, lbl_80794A2C->whd_ofs[i], lbl_80794A2C->whd_size[i],
                      (u32)fn_800F0230, 3, args);
        args[0] = 0;
        sprintf(name, lbl_80598FC8, lbl_80598190[i - 1]);
        load_file_req(name, lbl_80794A2C->tsb_ofs[i], lbl_80794A2C->tsb_size[i],
                      (u32)fn_800F0230, 3, args);
    }
    fn_800F06AC(1, arg0);
    tbl = (arg0 == 0x15) ? lbl_805981C8 : lbl_80598200;
    for (i = 0x1E; i < 0x21; i++) {
        args[1] = i;
        args[2] = i;
        fn_800F0410(i);
        args[0] = 1;
        sprintf(name, lbl_80598F9C, tbl[i - 0x1E]);
        load_file_req(name, lbl_80794A2C->whd_ofs[i], lbl_80794A2C->whd_size[i],
                      (u32)fn_800F0230, 3, args);
        args[0] = 0;
        sprintf(name, lbl_80598FC8, tbl[i - 0x1E]);
        load_file_req(name, lbl_80794A2C->tsb_ofs[i], lbl_80794A2C->tsb_size[i],
                      (u32)fn_800F0230, 3, args);
    }
    fn_800F15B0();
    fn_800F1700(arg0, 0, 0);
}

/* The `se_w` bank catalogue, with the European file-name fixup on the third entry. */
extern "C" void fn_800F0AD0(u8 arg0)
{
    char name[0x80];
    char tmp[0x80];
    u32 args[3];
    s32 i;

    for (i = 1; i < 4; i++) {
        args[1] = i;
        args[2] = i;
        fn_800F0410(i);
        lbl_80794A2C->load_count++;
        args[0] = 1;
        if (i == 2) {
            sprintf(tmp, lbl_80598F9C, lbl_80598174[i - 1]);
            cnvt_eur_fname(name, tmp);
        } else {
            sprintf(name, lbl_80598F9C, lbl_80598174[i - 1]);
        }
        load_file_req(name, lbl_80794A2C->whd_ofs[i], lbl_80794A2C->whd_size[i],
                      (u32)fn_800F0230, 3, args);
        args[0] = 0;
        sprintf(name, lbl_80598FC8, lbl_80598174[i - 1]);
        load_file_req(name, lbl_80794A2C->tsb_ofs[i], lbl_80794A2C->tsb_size[i],
                      (u32)fn_800F0230, 3, args);
    }
    fn_800F06AC(0, arg0);
    fn_800F15B0();
}

extern "C" s8 fn_800F0C14(s32 chunk)
{
    s8 handle = (s8)lbl_80794A2C->chunk_handle[chunk];

    if (handle == -1) {
        return -1;
    }
    if (fn_800F04FC((s32)handle) != 0) {
        return handle;
    }
    return -1;
}

/* Looks the SE id up in the 16-slot list; slot 0 when it is absent. */
extern "C" u8 fn_800F0C74(u8 id)
{
    u8 want = id;
    s32 i = 0;

    while (i < 0x10) {
        if (want == lbl_80794A2C->se_slot[i].id) {
            break;
        }
        i++;
    }
    if (i >= 0x10 || lbl_80794A2C->se_slot[i].chunk == 0xFF) {
        i = 0;
    }
    return lbl_80794A2C->se_slot[i].chunk;
}

/* Lays the per-kind bank table out and records which stream slots it may use. */
extern "C" void fn_800F0D88(u8 kind)
{
    SndChunkRec* rec;
    u32 cur;

    if (kind != 4) {
        rec = lbl_80598F68[2];
    } else {
        rec = lbl_80598F68[3];
    }
    if (kind > 4) {
        lbl_80794A2C->se_auto[0] = 3;
        lbl_80794A2C->se_auto[1] = 3;
    } else {
        lbl_80794A2C->se_auto[0] = lbl_805990F0[kind * 2];
        lbl_80794A2C->se_auto[1] = lbl_805990F0[kind * 2 + 1];
    }
    cur = lbl_80794A2C->bank_base_2;
    while (rec->chunk != -1) {
        if (rec->chunk != 0) {
            lbl_80794A2C->whd_ofs[rec->chunk] = cur;
            lbl_80794A2C->whd_size[rec->chunk] = rec->whd_size;
            cur += rec->whd_size;
            lbl_80794A2C->tsb_ofs[rec->chunk] = cur;
            lbl_80794A2C->tsb_size[rec->chunk] = rec->tsb_size;
            cur += rec->tsb_size;
        }
        rec++;
    }
    lbl_80794A2C->total_size = cur;
}

extern "C" void fn_800F0EA0(void)
{
    s32 i;
    for (i = 0; i < 6; i++) {
        lbl_80794A2C->se_slot_id[i] = 0;
        lbl_80794A2C->se_slot_num[i] = 0;
    }
}

/* Releases one reference of an already loaded SE id. */
extern "C" void snd_em_se_slot_release(u8 id)
{
    s32 slot = fn_800F1250(id);

    if (slot >= 0) {
        lbl_80794A2C->se_slot_num[slot]--;
        if (lbl_80794A2C->se_slot_num[slot] == 0) {
            lbl_80794A2C->se_slot_id[slot] = 0;
        }
    }
}

/* Loads an SE id into one of the six stream slots. */
extern "C" void fn_800F0F9C(u8 id)
{
    char name[0x88];
    u32 args[3];
    s32 slot = fn_800F1250(id);
    u8 want = id;
    const char* nm;
    u8 kind;

    if (slot >= 0) {
        lbl_80794A2C->se_slot_num[slot]++;
        return;
    }
    nm = lbl_805980A8[want];
    kind = lbl_80598AD8[want];
    if (nm == NULL) {
        return;
    }
    if (kind == 0) {
        return;
    }
    switch (kind) {
    case 1:
        if (fn_8028F288() == 4) {
            slot = 5;
            break;
        }
        slot = lbl_80794A2C->se_auto[0];
        {
            s32 end = slot + lbl_80794A2C->se_auto[1];
            for (; slot < end; slot++) {
                if (lbl_80794A2C->se_slot_num[slot] == 0) {
                    break;
                }
            }
        }
        if (slot >= 6) {
            return;
        }
        break;
    case 2:
        if (fn_8028F288() == 0 && (want == 0xC || want == 0xE)) {
            slot = 2;
            if (lbl_80794A2C->se_slot_num[2] != 0) {
                return;
            }
        } else {
            for (slot = 0; slot < lbl_80794A2C->se_auto[0]; slot++) {
                if (lbl_80794A2C->se_slot_num[slot] == 0) {
                    break;
                }
            }
            if (slot >= 3) {
                return;
            }
        }
        break;
    default:
        return;
    }
    lbl_80794A2C->se_slot_num[slot]++;
    lbl_80794A2C->se_slot_id[slot] = id;
    lbl_80794A2C->se_slot_bank[slot] = slot + 0x28;
    lbl_80794A2C->se_slot_idx[slot] = slot + 0x28;
    args[1] = lbl_80794A2C->se_slot_bank[slot];
    args[2] = lbl_80794A2C->se_slot_idx[slot];
    fn_800F0410(lbl_80794A2C->se_slot_idx[slot]);
    args[0] = 1;
    sprintf(name, lbl_80598F9C, nm);
    load_file_req(name, lbl_80794A2C->whd_ofs[lbl_80794A2C->se_slot_idx[slot]],
                  lbl_80794A2C->whd_size[lbl_80794A2C->se_slot_idx[slot]], (u32)fn_800F0230, 3,
                  args);
    args[0] = 0;
    sprintf(name, lbl_80598FC8, nm);
    load_file_req(name, lbl_80794A2C->tsb_ofs[lbl_80794A2C->se_slot_idx[slot]],
                  lbl_80794A2C->tsb_size[lbl_80794A2C->se_slot_idx[slot]], (u32)fn_800F0230, 3,
                  args);
}
/* The slot an SE id already occupies, or -1. */
extern "C" s32 fn_800F1250(u8 id)
{
    SndWork* p = lbl_80794A2C;
    s32 i;
    for (i = 0; i < 6; i++) {
        if (id == p->se_slot_id[0] && p->se_slot_num[0] != 0) {
            return i;
        }
        p++;
    }
    return -1;
}

u8 get_em_se_bank(u8 id)
{
    SndWork* p = lbl_80794A2C;
    u8 want = id;
    s32 i;
    for (i = 0; i < 6; i++) {
        if (want == p->se_slot_id[0]) {
            return lbl_80794A2C->se_slot_bank[i];
        }
        p++;
    }
    return -1;
}

extern "C" u8 fn_800F1398(SndFlag3* work, u8 kind)
{
    if (kind == 1) {
        return 0x1F;
    }
    if (work != NULL && work->flag == 1) {
        return 0x30;
    }
    return lbl_80794A2C->se_slot_bank[5];
}

/* The NPC voice banks: `c_npc_nekotaku` when the call is demoted, `c_npc_sansai` otherwise. */
extern "C" void snd_npc_voice_bank_load(u8 arg0, u8 arg1)
{
    char name[0x88];
    u32 args[3];

    if (arg0 == 9 || arg1 == 0) {
        lbl_80794A2C->se_slot_bank[5] = 0x2D;
        lbl_80794A2C->se_slot_idx[5] = 0x2D;
        args[1] = lbl_80794A2C->se_slot_bank[5];
        args[2] = lbl_80794A2C->se_slot_idx[5];
        fn_800F0410((s32)args[2]);
        args[0] = 1;
        sprintf(name, lbl_80598F9C, lbl_805990FC);
        load_file_req(name, lbl_80794A2C->whd_ofs[args[2]], lbl_80794A2C->whd_size[args[2]],
                      (u32)fn_800F0230, 3, args);
        args[0] = 0;
        sprintf(name, lbl_80598FC8, lbl_805990FC);
        load_file_req(name, lbl_80794A2C->tsb_ofs[args[2]], lbl_80794A2C->tsb_size[args[2]],
                      (u32)fn_800F0230, 3, args);
    }
    args[1] = 0x30;
    args[2] = 0x30;
    fn_800F0410(0x30);
    lbl_80794A2C->load_count++;
    args[0] = 1;
    sprintf(name, lbl_80598F9C, lbl_8059910C);
    load_file_req(name, lbl_80794A2C->whd_ofs[0x30], lbl_80794A2C->whd_size[0x30],
                  (u32)fn_800F0230, 3, args);
    args[0] = 0;
    sprintf(name, lbl_80598FC8, lbl_8059910C);
    load_file_req(name, lbl_80794A2C->tsb_ofs[0x30], lbl_80794A2C->tsb_size[0x30],
                  (u32)fn_800F0230, 3, args);
}

/* The jingle `tsb` the quest BGM hands over to. */
extern "C" void fn_800F15B0(void)
{
    u32 args[3];

    args[1] = 0x2F;
    args[2] = 0x2F;
    lbl_80794A2C->chunk_ready[0x2F] = 0;
    lbl_80794A2C->tsb_ready[0x2F] = 0;
    args[0] = 0;
    load_file_req((char*)lbl_80599120, lbl_80794A2C->tsb_ofs[0x2F],
                  lbl_80794A2C->tsb_size[0x2F], (u32)fn_800F033C, 3, args);
}

/* The demo/movie `whd` of chunk 0x2F, with the id the callback stores. */
extern "C" void fn_800F1620(s32 arg0)
{
    char name[0x88];
    u32 args[4];

    args[1] = 0x2F;
    args[2] = 0x2F;
    args[3] = arg0;
    lbl_80794A2C->chunk_ready[0x2F] = 0;
    lbl_80794A2C->whd_ready[0x2F] = 0;
    lbl_80794A2C->demo_id = 0xFF;
    if (arg0 < 7) {
        args[0] = 1;
        sprintf(name, lbl_80599018, lbl_80598A2C[arg0]);
        load_file_req(name, lbl_80794A2C->whd_ofs[0x2F], lbl_80794A2C->whd_size[0x2F],
                      (u32)fn_800F033C, 4, args);
    }
}

extern "C" void se_slot_req(u8 arg0)
{
    if (lbl_80794A2C->demo_id == arg0) {
        fn_800E80DC(0x2F, 0, 0);
        return;
    }
    fn_800F1620(arg0);
}

/* The quest map's `whd`/`tsb` pair, or the two demo movie streams. */
extern "C" void fn_800F1700(u8 arg0, u8 arg1, u8 arg2)
{
    char name[0x88];
    u32 args[3];
    u8 id = arg0;
    u8 variant = arg1;

    if (arg0 != 0x15 && arg0 != 0x16) {
        const u8* tbl;
        if (move_work_state_ck() == 1) {
            id = arg0;
        } else {
            id = quest_pair_table_get(arg0, arg2);
        }
        tbl = lbl_80597990[id];
        if (tbl == NULL) {
            return;
        }
        variant = tbl[arg1];
    }
    args[1] = 0x2E;
    args[2] = 0x2E;
    fn_800F0410(0x2E);
    if (arg0 != 0x15 && arg0 != 0x16) {
        args[0] = 1;
        sprintf(name, lbl_80599150, id, variant);
        load_file_req(name, lbl_80794A2C->whd_ofs[0x2E], lbl_80794A2C->whd_size[0x2E],
                      (u32)fn_800F0230, 3, args);
        args[0] = 0;
        sprintf(name, lbl_80599168, id, variant);
        load_file_req(name, lbl_80794A2C->tsb_ofs[0x2E], lbl_80794A2C->tsb_size[0x2E],
                      (u32)fn_800F0230, 3, args);
        return;
    }
    args[0] = 1;
    sprintf(name, lbl_80598F9C, lbl_80791638[arg0 - 0x15]);
    load_file_req(name, lbl_80794A2C->whd_ofs[0x2E], lbl_80794A2C->whd_size[0x2E],
                  (u32)fn_800F0230, 3, args);
    args[0] = 0;
    sprintf(name, lbl_80598FC8, lbl_80791638[arg0 - 0x15]);
    load_file_req(name, lbl_80794A2C->tsb_ofs[0x2E], lbl_80794A2C->tsb_size[0x2E],
                  (u32)fn_800F0230, 3, args);
}

void title_se_load(void)
{
    u32 args[3];

    args[1] = 0x2E;
    args[2] = 0x2E;
    fn_800F0410(0x2E);
    args[0] = 1;
    load_file_req((char*)lbl_80599180, lbl_80794A2C->whd_ofs[0x2E], lbl_80794A2C->whd_size[0x2E],
                  (u32)fn_800F0230, 3, args);
    args[0] = 0;
    load_file_req((char*)lbl_80599194, lbl_80794A2C->tsb_ofs[0x2E], lbl_80794A2C->tsb_size[0x2E],
                  (u32)fn_800F0230, 3, args);
}

/* The demo's SE and BGM streams. */
s32 demo_bgm_load(u8 index)
{
    char name[0x100];
    u32 args[5];
    u8 mapno;
    s32 alt;
    const SndDemoCfg* cfg;

    if (index >= 0x31) {
        return -1;
    }
    mapno = get_now_mapno();
    alt = stage_map_kind_get(mapno) != mapno;
    cfg = &lbl_8059AF80[index];
    set_stream_main_vol_flag(0, 1);
    set_stream_main_vol_flag(1, 0);
    set_stream_main_vol_flag(2, 0);
    sound_frame_entry();
    if (cfg->se != 0) {
        PlayStream(0, 1);
    }
    if (cfg->bgm != 0) {
        PlayStream(1, 1);
        PlayStream(2, 1);
    }
    if (cfg->se != 0) {
        fn_800E4D60(0);
        lbl_80794A2C->stream_ready[0] = 0;
    }
    if (cfg->bgm != 0) {
        fn_800E4D60(2);
        lbl_80794A2C->stream_ready[2] = 0;
    }
    if (demo_play_ck() == 1 && (index == 0xE || index == 0x2B || index == 0x2C || index == 0x2F)) {
        PlayStream(1, 1);
        PlayStream(2, 1);
        fn_800E4D60(2);
        lbl_80794A2C->stream_ready[2] = 0;
    }
    lbl_80794A38[0] = 0xFF;
    lbl_80794A38[1] = 0xFF;
    if (cfg->se != 0) {
        lbl_80794A2C->bgm_loading_2 = 1;
        sprintf(name, lbl_8059B044, index);
        if (cfg->se_variant == 1) {
            if (alt == 0) {
                strcat(name, lbl_80791640);
                sprintf(lbl_806A1178, lbl_8059B064, index);
            } else {
                strcat(name, lbl_80791648);
                sprintf(lbl_806A1178, lbl_8059B088, index);
            }
        } else {
            strcat(name, lbl_80791640);
            sprintf(lbl_806A1178, lbl_8059B064, index);
        }
        lbl_80794A30 = lbl_806A1178;
        args[0] = 0;
        args[1] = (u32)&lbl_80794A30;
        args[2] = 1;
        args[3] = 0;
        args[4] = 0;
        load_file_req(name, 0x92D18000, 0x2000, (u32)fn_800F0448, 5, args);
        lbl_80794A38[0] = 0;
    }
    if (demo_play_ck() == 1 && (index == 0xE || index == 0x2B || index == 0x2C || index == 0x2F)) {
        lbl_80794A2C->bgm_loading_1 = 1;
        sprintf(name, lbl_8059B0B0, index);
        strcat(name, lbl_80791640);
        sprintf(lbl_806A1138, lbl_8059B0D0, index);
    } else if (cfg->bgm != 0) {
        lbl_80794A2C->bgm_loading_1 = 1;
        sprintf(name, lbl_8059B0B0, index);
        if (cfg->bgm_variant == 1) {
            if (alt == 0) {
                strcat(name, lbl_80791640);
            } else {
                strcat(name, lbl_80791648);
            }
        } else {
            strcat(name, lbl_80791640);
        }
        sprintf(lbl_806A1138, lbl_8059B0D0, index);
    } else {
        if (demo_play_ck() == 0) {
            if ((u8)(index + 0xF2) <= 4U || index == 0x2B) {
                snd_bgm_hold_set();
            }
        } else if ((u8)(index + 0xF1) <= 3U) {
            snd_bgm_hold_set();
        }
        return 0;
    }
    lbl_80794A34 = lbl_806A1138;
    args[0] = 2;
    args[1] = (u32)&lbl_80794A34;
    args[2] = 1;
    args[3] = 0;
    args[4] = 0;
    load_file_req(name, 0x92D1C000, 0x2000, (u32)fn_800F0448, 5, args);
    lbl_80794A38[0] = 2;
    if (demo_play_ck() == 0) {
        if ((u8)(index + 0xF2) <= 4U || index == 0x2B) {
            snd_bgm_hold_set();
        }
    } else if ((u8)(index + 0xF1) <= 3U) {
        snd_bgm_hold_set();
    }
    return 0;
}

s32 fn_800F1DE0(void)
{
    s32 ready = 0;
    s32 i = 0;
    u8* p = lbl_80794A38;

    do {
        if (*p != 0xFF) {
            if (srt_ready_ck(lbl_80794A38[0]) == 1) {
                ready++;
            }
        } else {
            ready++;
        }
        p++;
        i++;
    } while (i < 2);
    return (ready - 2) == 0;
}

/* The quest BGM: the map's stream plus the quest `srt`. */
void quest_bgm_load(u8 arg0, u8 arg1)
{
    u32 args[5];
    u8 index = arg0;

    lbl_80794A2C->bgm_loading_2 = 0;
    lbl_80794A2C->bgm_loading_1 = 0;
    if (move_work_state_ck() != 1) {
        index = quest_pair_table_get(index, arg1);
    }
    fn_800E4D60(0);
    fn_800E4D60(1);
    fn_800E4D60(2);
    lbl_80794A2C->stream_ready[0] = 0;
    lbl_80794A2C->stream_ready[1] = 0;
    lbl_80794A2C->stream_ready[2] = 0;
    if (lbl_8059AEB0[index] != NULL) {
        args[0] = 0;
        args[1] = (u32)lbl_8059AF0C[index];
        args[2] = lbl_8059AF68[index];
        args[3] = 0;
        args[4] = 0;
        load_file_req((char*)lbl_8059AEB0[index], 0x92D18000, 0x2000, (u32)fn_800F0448, 5, args);
    }
    args[0] = 1;
    args[1] = (u32)lbl_8059AA14;
    args[2] = 0x1F;
    args[3] = 1;
    args[4] = 2;
    load_file_req((char*)lbl_8059B0F8, 0x92D1A000, 0x2000, (u32)fn_800F0448, 5, args);
}

/* The field BGM: the day/night stream plus the quest `srt`. */
extern "C" void fn_800F1FCC(void)
{
    u32 args[5];
    u8 mapno;

    lbl_80794A2C->bgm_loading_2 = 0;
    lbl_80794A2C->bgm_loading_1 = 0;
    mapno = get_now_mapno();
    get_gm_daynight();
    fn_800E4D60(0);
    fn_800E4D60(2);
    lbl_80794A2C->stream_ready[0] = 0;
    lbl_80794A2C->stream_ready[2] = 0;
    if (mapno >= 0x15) {
        fn_800F2468(mapno);
        return;
    }
    if (lbl_8059AEB0[mapno] != NULL) {
        args[0] = 0;
        args[1] = (u32)lbl_8059AF0C[mapno];
        args[2] = lbl_8059AF68[mapno];
        args[3] = 0;
        args[4] = 0;
        load_file_req((char*)lbl_8059AEB0[mapno], 0x92D18000, 0x2000, (u32)fn_800F0448, 5, args);
    }
    args[0] = 2;
    args[1] = (u32)lbl_8059AA14;
    args[2] = 0x1F;
    args[3] = 0;
    args[4] = 0;
    load_file_req((char*)lbl_8059B0F8, 0x92D1C000, 0x2000, (u32)fn_800F0448, 5, args);
}

/* The quest-clear jingle. */
extern "C" void fn_800F2114(void)
{
    u32 args[5];

    fn_800E4D60(2);
    lbl_80794A2C->stream_ready[0] = 0;
    lbl_80794A2C->stream_ready[1] = 0;
    lbl_80794A2C->stream_ready[2] = 0;
    args[0] = 2;
    args[1] = (u32)lbl_8059AAFC;
    args[2] = 3;
    args[3] = 2;
    args[4] = 0;
    load_file_req((char*)lbl_8059B114, 0x92D1C000, 0x2000, (u32)fn_800F0448, 5, args);
}

void movie_bgm_load(void)
{
    u32 args[5];

    fn_800E4D60(0);
    lbl_80794A2C->stream_ready[0] = 0;
    args[0] = 0;
    args[1] = (u32)lbl_8059AC98;
    args[2] = 0x0A;
    args[3] = 0;
    args[4] = 0;
    load_file_req((char*)lbl_8059B134, 0x92D18000, 0x2000, (u32)fn_800F0448, 5, args);
}

void title_bgm_load(void)
{
    u32 args[5];

    fn_800E4D60(1);
    lbl_80794A2C->stream_ready[1] = 0;
    args[0] = 1;
    args[1] = (u32)lbl_8059AA14;
    args[2] = 0x1F;
    args[3] = 0;
    args[4] = 0;
    load_file_req((char*)lbl_8059B0F8, 0x92D1C000, 0x2000, (u32)fn_800F0448, 5, args);
}

/* The lobby BGM. */
extern "C" void lobby_bgm_load(void)
{
    u32 args[5];

    fn_800E4D60(1);
    lbl_80794A2C->stream_ready[1] = 0;
    args[0] = 1;
    args[1] = (u32)lbl_8059A554;
    args[2] = 0x19;
    args[3] = 0;
    args[4] = 0;
    load_file_req((char*)lbl_8059B150, 0x92D1A000, 0x2000, (u32)fn_800F0448, 5, args);
}

/* The lobby BGM with its two stream variants. */
extern "C" void fn_800F2328(u8 arg0, u8 arg1)
{
    u32 args[5];

    lbl_80794A2C->bgm_loading_2 = 0;
    lbl_80794A2C->bgm_loading_1 = 0;
    fn_800E4D60(0);
    fn_800E4D60(1);
    fn_800E4D60(2);
    lbl_80794A2C->stream_ready[0] = 0;
    lbl_80794A2C->stream_ready[1] = 0;
    lbl_80794A2C->stream_ready[2] = 0;
    if (lbl_8059AEB0[arg0] != NULL) {
        args[0] = 0;
        args[1] = (u32)lbl_8059AF0C[arg0];
        args[2] = lbl_8059AF68[arg0];
        args[3] = 0;
        args[4] = 0;
        load_file_req((char*)lbl_8059AEB0[arg0], 0x92D18000, 0x2000, (u32)fn_800F0448, 5, args);
    }
    args[0] = 1;
    args[1] = (u32)lbl_8059A554;
    args[2] = 0x19;
    args[3] = 1;
    args[4] = 2;
    load_file_req((char*)lbl_8059B150, 0x92D1A000, 0x2000, (u32)fn_800F0448, 5, args);
}

/* The field BGM's stream on its own. */
extern "C" void fn_800F2468(u8 arg0)
{
    u32 args[5];

    if (lbl_8059AEB0[arg0] != NULL) {
        args[0] = 0;
        args[1] = (u32)lbl_8059AF0C[arg0];
        args[2] = lbl_8059AF68[arg0];
        args[3] = 0;
        args[4] = 0;
        load_file_req((char*)lbl_8059AEB0[arg0], 0x92D18000, 0x2000, (u32)fn_800F0448, 5, args);
    }
    args[0] = 2;
    args[1] = (u32)lbl_8059A554;
    args[2] = 0x19;
    args[3] = 0;
    args[4] = 0;
    load_file_req((char*)lbl_8059B150, 0x92D1A000, 0x2000, (u32)fn_800F0448, 5, args);
}

/* The four pan/pitch halves of one voice. */
extern "C" void fn_800F2540(u32 arg0, s32 arg1, SndVoice* out)
{
    out->pan = 0;
    out->field_26 = 0;
    out->field_28 = 0;
    out->field_2A = 0;
    if (arg0 < 0x31 && fn_800F04FC((s32)arg0) != 0) {
        SndVoiceCfg* v = (SndVoiceCfg*)fn_800E7FC4(arg0, arg1);
        if (v != NULL) {
            out->pan = v->field_0C;
            out->field_26 = v->field_21;
            out->field_28 = v->field_22;
        }
    }
}

/* Snapshots one bank's 256 ids into the three lookup tables. */
extern "C" void fn_800F25DC(u32 arg0)
{
    u8* t1;
    u8* t2;
    u8* t3;
    s32 i;

    if (arg0 >= 0x31) {
        return;
    }
    t1 = &lbl_80794A2C->chunk_id_1[arg0][0];
    t2 = &lbl_80794A2C->chunk_id_2[arg0][0];
    t3 = &lbl_80794A2C->chunk_id_3[arg0][0];
    i = 0;
    do {
        SndVoiceCfg* v = (SndVoiceCfg*)fn_800E7FC4(arg0, i);
        if (v == NULL) {
            *t1 = 0;
        } else {
            *t1 = v->field_0C;
            *t2 = v->field_21;
            *t3 = v->field_22;
        }
        t1++;
        t2++;
        t3++;
        i++;
    } while (i < 0x100);
}

extern "C" u8 fn_800F2680(u32 arg0, u32 arg1)
{
    if (arg0 >= 0x31) {
        return 0;
    }
    if (arg1 >= 0x100) {
        return 0;
    }
    return lbl_80794A2C->chunk_id_1[arg0][arg1];
}

extern "C" u8 fn_800F26B8(u32 arg0, u32 arg1, u8 arg2)
{
    if (arg0 >= 0x31) {
        return 0;
    }
    if (arg1 >= 0x100) {
        return 0;
    }
    if (arg2 == 0) {
        return lbl_80794A2C->chunk_id_2[arg0][arg1];
    }
    return lbl_80794A2C->chunk_id_3[arg0][arg1];
}

extern "C" s32 fn_800F2714(u32 arg0, s32 arg1, u8 arg2, s32 arg3)
{
    SndVoiceCfg* v;

    if (arg0 >= 0x31) {
        return 0;
    }
    if (fn_800F04FC((s32)arg0) == 0) {
        return 0;
    }
    v = (SndVoiceCfg*)fn_800E7FC4(arg0, arg1);
    if (v == NULL) {
        return 0;
    }
    if (arg2 == 0) {
        v->field_21 = arg3 & 0x7F;
    } else {
        v->field_22 = arg3 & 0x7F;
    }
    return 1;
}

extern "C" s32 fn_800F27C4(u32 arg0, s32 arg1, s8 arg2)
{
    SndVoiceCfg* v;

    if (arg0 >= 0x31) {
        return 0;
    }
    if (fn_800F04FC((s32)arg0) == 0) {
        return 0;
    }
    v = (SndVoiceCfg*)fn_800E7FC4(arg0, arg1);
    if (v == NULL) {
        return 0;
    }
    v->field_0B = arg2;
    return 1;
}

extern "C" u16 fn_800F284C(u32 arg0)
{
    SndVoice* v;

    if (arg0 >= 0x31) {
        return 0;
    }
    v = (SndVoice*)fn_800E7FC4(arg0, 0);
    if (v == NULL) {
        return 0;
    }
    return v->pitch;
}

extern "C" s32 fn_800F2890(u32 arg0, s32 arg1)
{
    if (arg0 >= 0x31) {
        return 0;
    }
    if (fn_800F04FC((s32)arg0) == 0) {
        return 0;
    }
    return fn_800E7FC4(arg0, arg1) != NULL;
}

extern "C" u32 fn_800F2900(void)
{
    if (lbl_80794A2C->stream_ok == 1) {
        return lbl_80794A2C->stream_dma;
    }
    return 0;
}

extern "C" u16 fn_800F2924(s32 arg0, s32 arg1)
{
    SndVoice* v;

    if (srt_ready_ck(0) == 0) {
        return 1;
    }
    v = (SndVoice*)fn_800E4F4C(arg0, arg1);
    if (v == NULL) {
        return 1;
    }
    return v->loop_len;
}

extern "C" u8 fn_800F298C(void)
{
    return fn_800F2924(0, 0) == 0;
}

void set_SE_volume(u8 index)
{
    fn_800E84F0((s16)lbl_80791610[index]);
    fn_800F2A1C(lbl_80791610[index], 1);
}

void set_BGM_volume(u8 index)
{
    fn_800F2A1C(lbl_80791610[index], 0);
}

extern "C" void fn_800F2A1C(u8 volume, u8 is_bgm)
{
    f32 v = (f32)volume / lbl_80796588;

    if (v < lbl_8079658C) {
        v = lbl_8079658C;
    }
    if (v > lbl_80796590) {
        v = lbl_80796590;
    }
    fn_800E4908(is_bgm, v);
}

extern "C" u8 fn_800F2A84(void)
{
    return lbl_80794A2C->bgm_loading_2;
}
