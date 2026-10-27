/* sound/snd_bank_loader.cpp - the sound bank/file loader: the sound work record, the per-scene SE/BGM bank loads
 * (`16/whd`/`16/tsb` pairs per chunk), the stream (`srt`) loads and the voice/stream queries the SE layer drives.
 *
 * RANGE. `.text` 0x800EEAE0..0x800F2A94 (74 functions, all written, definitions in address order); `.data`
 * 0x805980A8..0x8059B170, `.sdata` 0x80791470..0x80791650, `.sdata2` 0x80796588..0x807965A0, `.sbss`
 * 0x80794A24..0x80794A40, `.bss` 0x806A1120..0x806A11B8, plus its `extab`/`extabindex`. The data is emitted by this
 * source in retail order: the tables before the first body, each body's string literals as it is compiled, and
 * three table groups between bodies (before `fn_800EFD88`, `snd_em_bank_level_load`, `snd_area_env_bank_load`
 * and `demo_bgm_load`). Module/name class 3 (GUESS): no `__FILE__` string; the range's map names are the sound
 * system's (`title_se_load__Fv`, `demo_bgm_load__FUc`, `quest_bgm_load__FUcUc`, ...). Language C++ (manglings).
 * FLAGS. `cflags_main` plus file-scope `#pragma peephole off` (retail keeps `clrlwi`+`slwi` unfused) and
 *   `#pragma pool_data off` (retail addresses every string with its own `lis`/`addi`: snd_quest_bgm_load 89.40
 *   pooled, 100 without).
 * NAMES. GUESS (from each body and its callers): snd_player_banks_load, snd_em_se_slot_release,
 *   snd_npc_voice_bank_load, lobby_bgm_load, snd_area_bank_load, snd_bank_loader_reset, snd_bank_loader_init
 *   (pad_connect's boot calls it once: work record, heap, audio stack, chunk 0), snd_heap_free_size_get,
 *   snd_heap_free_size / snd_heap_free_size_low (written once from it), snd_load_mutex, snd_wk (the work record
 *   pointer), demo_se_stream_name / demo_bgm_stream_name / demo_se_stream_path / demo_bgm_stream_path /
 *   demo_stream_slot (demo_bgm_load's stream state), chacha_dance_bank_find, chacha_dance_name_1 / _2 (the Chacha
 *   dance bank names, chunks 0x20/0x21), snd_chunk_recs_0.._5 (the six bank layouts `lbl_80598F68` points at),
 *   stage_amb_streams_NN (the ambient stream lists `lbl_8059AF0C` indexes by map; NN = the first map using it),
 *   wp_e_variant_ids_0.._2 (the variant ids `fn_800EFD88` sends to the `_e` whd).
 *   Registry names for the band callees: snd_exp_heap, snd_heap_allocator, snd_heap_alloc, snd_heap_free,
 *   snd_chunk_install (quest_snd), snd_stream_buffers_init (fn_800E46E8), snd_audio_init, snd_fx_hooks_set,
 *   snd_output_mode_set (snd_stream_reloc).
 * RESIDUALS. fn_800F0F9C (692 B): retail's shared load tail sits between the `kind == 1` and `kind == 2` bodies,
 *   case 1 falling into it and case 2 branching back - a `goto` layout; rule 8 forbids it and the conformant shapes
 *   measure switch + tail 54.77, if/else chain 52.49, for/break 52.49. This row blocks a flip.
 *   snd_quest_bgm_set: register colouring only (retail keeps the three parameters in r25..r27, ours r27..r29).
 *   pl_weapon_se_bank_load (96.18) and snd_player_banks_load (97.08): register colouring and one scheduled load.
 *   fn_800F02D4 (95.96): retail tests `count` with `bne` + `blr` where ours folds it into `beqlr`.
 *   snd_area_bank_load, fn_800F06AC, demo_bgm_ready_ck, snd_chunk_handle_get, snd_area_env_bank_load,
 *   demo_bgm_load: register choice / operand order only.
 *   `.data` (4 B), `.sdata` (1 B) and `.sbss` (5 B) are short of their claims by the alignment tail before the next
 *   unit's 8-aligned section (`demo_stream_slot` is two bytes used, sized 3); every byte up to it matches.
 * SHAPES. `#pragma pool_data off`; `for (;;)` + `break` reproduces demo_bgm_load's shared BGM tail; `x = c ? x :
 *   f(x)` reproduces the `bne` + `b` pair retail has for an unchanged value (snd_stage_bank_load, quest_bgm_load,
 *   snd_area_env_bank_load); int locals where retail keeps byte loads unextended (pl_weapon_se_bank_load).
 *
 * The record both per-actor paths walk is `pl.h`'s `_PLW`: its `equipB` / `equipB2` / `equipC` / `equipD` are the
 * four `_EQUIP` records `Get_pl_type` and `equip_variant_get` take at +0x1D0 / +0x1DC / +0x1E8 / +0x1F4, and the
 * enemy list walks it at its 0xB20 stride. `slot_active` (+0x000), `chunk_ofs` (+0x008), `se_name_set` (+0x014)
 * and `se_name_idx` (+0x01D) are named from this unit's evidence.
 */

#include "types.h"
#include "sound/snd_work.h"
#include "sound/snd_bank_loader.h"
#include "sound/snd_banks_loaded_ck.h"
#include "sound/snd_result_banks_load.h"
#include "sound/chacha_dance_bank_find.h"
#include "sound/snd_exp_heap.h"
#include "sound/snd_heap_allocator.h"
#include "sound/snd_heap_alloc.h"
#include "sound/snd_heap_free.h"
#include "sound/snd_chunk_install.h"
#include "sound/snd_stream_buffers_init.h"
#include "sound/snd_audio_init.h"
#include "sound/snd_fx_hooks_set.h"
#include "sound/snd_output_mode_set.h"
#include "MEM/mem_expheap.h"
#include "MEM/mem_allocator.h"
#include "OS/OSMutex.h"
#include "mh3_pad/system_w.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "Network/network_pat_control.h"
#include "Pl/equip_variant_get.h"
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

/* `SndSeSlot` and `SndWork` live in `sound/sound_work.h` (rule 1: the system core's HOME-menu sound fade reads the
 * work too). */

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

/* The four half-words `fn_800F2540` copies out of a voice (its pan/mix block at +0x24). */
typedef struct SndVoiceMix {
    /* +0x00 */ u16 pan;
    /* +0x02 */ u16 field_02;
    /* +0x04 */ u16 field_04;
    /* +0x06 */ u16 field_06;
} SndVoiceMix; /* size: 0x8 */

/* The per-voice record `fn_800E7FC4` / `fn_800E4F4C` hand back; the owner declares them `void*`, so
 * this is the field view the accesses below trace. */
typedef struct SndVoice {
    /* +0x00 */ u8  unused_00[0x0B];
    /* +0x0B */ s8  detune;      /* fn_800F27C4 */
    /* +0x0C */ u8  field_0C;    /* snd_chunk_ids_snapshot copies it out */
    /* +0x0D */ u8  unused_0D[0x0D];
    /* +0x1A */ u16 pitch;       /* fn_800F284C */
    /* +0x1C */ u8  unused_1C[0x04];
    /* +0x20 */ u16 loop_len;    /* fn_800F2924 (through fn_800E4F4C) */
    /* +0x22 */ u8  unused_22[0x02];
    /* +0x24 */ SndVoiceMix mix;  /* fn_800F2540 copies it out */
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

/* The `whd` sizes of one weapon type's banks (`pl_weapon_se_bank_load`): the common bank, then up to three
 * sub-banks (the bow/gun parts take all three, the other weapons only the first). */
typedef struct SndPlSizes {
    /* +0x00 */ u32 size[4];
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

u8  Get_pl_type(_EQUIP* a, _EQUIP* b);
void cnvt_eur_fname(char* dst, char* src);
extern "C" u8 quest_em_level_get(void);
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
 * Data: the work-area globals; the `.data`/`.sdata` tables are defined below in their emission order (the object's
 * `.data` is the tables before the first body, the function literals in body order, and three table groups
 * between bodies); the four tables at 0x80597554..0x80597990 are `sound/fn_800D7F54.cpp`'s.
 * --------------------------------------------------------------------------------------------- */
OSMutex snd_load_mutex;
char demo_bgm_stream_path[0x40];      /* the demo BGM stream's `ssd` path `demo_bgm_load` formats */
char demo_se_stream_path[0x40];       /* the demo SE stream's `ssd` path */
u32 snd_heap_free_size;
u32 snd_heap_free_size_low;
SndWork* snd_wk;                       /* the work record (`system_w`'s memory layout slot) */
char* demo_se_stream_name;             /* the `srt` loader's name word for the demo SE stream */
char* demo_bgm_stream_name;            /* ... and for the demo BGM stream */
u8 demo_stream_slot[3];                /* the stream each demo half went to, 0xFF when none */

extern const u8* lbl_80597554[];
extern const u8* lbl_805977FC[];
extern const u8* lbl_805978E8[];
extern const u8* lbl_80597990[];

/* One demo's stream shape: which of the two streams it carries and which name variant to use. */
typedef struct SndDemoCfg {
    /* +0x00 */ u8 bgm;          /* the demo carries the BGM stream */
    /* +0x01 */ u8 bgm_variant;  /* use the `_N` stream name */
    /* +0x02 */ u8 se;           /* the demo carries the SE stream */
    /* +0x03 */ u8 se_variant;   /* use the `_n` SE stream name */
} SndDemoCfg; /* size: 0x4 */



/* ------------------------------------------------------------------------------------------------
 * This unit's own symbols (plain map names - the remaining `fn_*` stems are the map's own placeholders and
 * `snd_bank_layout` / `scene_se_bank_load` were renamed from two of them - so they are declared `extern "C"`
 * exactly as the map spells them).
 * --------------------------------------------------------------------------------------------- */
extern "C" void snd_bank_layout(u8 mode);
extern "C" void scene_se_bank_load(u8 a, u8 b);
extern "C" void snd_player_banks_load(void);
extern "C" void pl_voice_bank_load(_PLW* work);
extern "C" s32 fn_800EFD88(s32 id, s32 kind);
extern "C" void pl_weapon_se_bank_load(_PLW* work);
extern "C" void fn_800F0230(void* unused_0, void* unused_1, void* unused_2, u32* args);
extern "C" void fn_800F02D4(void* unused_0, SndWhdHeader* header);
extern "C" void fn_800F033C(void* unused_0, void* unused_1, void* unused_2, u32* args);
extern "C" void fn_800F0410(s32 idx);
extern "C" void fn_800F0448(void* unused_0, void* ctx, void* unused_2, u32* args);
extern "C" u32 fn_800F04FC(s32 idx);
extern "C" void snd_area_bank_load(u8 kind, u8 index);
extern "C" void snd_bank_loader_reset(void);
extern "C" void fn_800F0560(u8 a, u8 b);
extern "C" void fn_800F06AC(u8 unused, u8 kind);
extern "C" u32 fn_800F08E0(void);
extern "C" void fn_800F08E8(u8 arg0);
extern "C" void fn_800F0AD0(u8 arg0);
extern "C" s8 snd_chunk_handle_get(s32 chunk);
extern "C" u8 fn_800F0C74(u8 id);
extern "C" void snd_em_bank_level_load(u8 kind);
extern "C" void fn_800F0EA0(void);
extern "C" void snd_em_se_slot_release(u8 id);
extern "C" void fn_800F0F9C(u8 id);
extern "C" s32 fn_800F1250(u8 id);
extern "C" u8 fn_800F1398(SndFlag3* work, u8 kind);
extern "C" void snd_npc_voice_bank_load(u8 arg0, u8 arg1);
extern "C" void fn_800F15B0(void);
extern "C" void fn_800F1620(s32 arg0);
extern "C" void se_slot_req(s32 id);
extern "C" void snd_area_env_bank_load(u8 arg0, u8 arg1, u8 arg2);
extern "C" s32 demo_bgm_ready_ck(void);
extern "C" void stage_bgm_reload(void);
extern "C" void snd_quest_end_stream_load(void);
extern "C" void lobby_bgm_load(void);
extern "C" void fn_800F2328(u8 arg0, u8 arg1);
extern "C" void fn_800F2468(u8 arg0);
extern "C" void fn_800F2540(u32 arg0, s32 arg1, SndVoiceMix* out);
extern "C" void snd_chunk_ids_snapshot(u32 arg0);
extern "C" u8 fn_800F2680(u32 arg0, u32 arg1);
extern "C" u8 fn_800F26B8(u32 arg0, u32 arg1, u8 arg2);
extern "C" s32 fn_800F2714(u32 arg0, s32 arg1, u8 arg2, s32 arg3);
extern "C" s32 fn_800F27C4(u32 arg0, s32 arg1, s8 detune);
extern "C" u16 fn_800F284C(u32 arg0, s32 arg1);
extern "C" s32 fn_800F2890(u32 arg0, s32 arg1);
extern "C" u32 snd_stream_dma_get(void);
extern "C" u16 fn_800F2924(s32 arg0, s32 arg1);
extern "C" u8 snd_stream_bgm_flag_get(s32 channel, u32 id);
extern "C" void fn_800F2A1C(u8 volume, u8 is_bgm);
extern "C" u8 snd_bgm_loading_get(void);

extern "C" u32 snd_heap_free_size_get(MEMiHeapHead* heap);
extern "C" void snd_bank_loader_init(void);
extern "C" s32 chacha_dance_bank_find(u8 slot, u8 id);
void title_snd_init(void);
void system_se_load(void);
void title_se_load(void);
s32 demo_bgm_load(u8 index);
void quest_bgm_load(u8 arg0, u8 arg1);
void movie_bgm_load(void);
void title_bgm_load(void);
s32 get_em_se_bank(u8 id);
u32 srt_ready_ck(s32 stream);
void set_SE_volume(u8 index);
void set_BGM_volume(u8 index);

#pragma peephole off
#pragma pool_data off

/* Picks the AX output mode from the console's sound mode: mono, stereo, surround or DPL2. */
static inline u32 snd_output_mode_get(void)
{
    switch (snd_wk->output_mode) {
    case 0:
        return 0;
    case 2:
        if (snd_wk->output_dpl2 == 0) {
            return 2;
        }
        return 3;
    default:
        return 1;
    }
}

const char* lbl_805980A8[41] = {
    NULL, "em001", "em001", "em003", "em004", "em005", "em006", "em007", "em008", "em009", "em010", "em011", "em012",
    "em013", "em014", "em015", "em016", "em017", "em018", "em019", "em020", "em021", "em022", "em023", "em024",
    "em025", "em026", "em027", "em028", "em029", "em030", "em030", "em032", "em033", "em034", "em035", "em036",
    "em036", "em036", "em036", "em040"
};

const char* lbl_80791590 = "system";

const char* lbl_80598174[3] = {"soubi_se", "quest_jingle", "questcom"};

const char* lbl_80598190[3] = {"soubi_se", "matimura_jingle", "questcom"};

const char* lbl_805981C8[3] = {"lb_town_system", "lb_town_npc", "lb_town_various"};

const char* lbl_80598200[3] = {"lb_mura_system", "lb_mura_npc", "lb_mura_various"};

const char* lbl_80598350[20] = {
    "vo_pl_m00.whd", "vo_pl_m01.whd", "vo_pl_m02.whd", "vo_pl_m03.whd", "vo_pl_m04.whd", "vo_pl_m05.whd",
    "vo_pl_m06.whd", "vo_pl_m07.whd", "vo_pl_m08.whd", "vo_pl_m09.whd", "vo_pl_m10.whd", "vo_pl_m11.whd",
    "vo_pl_m12.whd", "vo_pl_m13.whd", "vo_pl_m14.whd", "vo_pl_m15.whd", "vo_pl_m16.whd", "vo_pl_m17.whd",
    "vo_pl_m18.whd", "vo_pl_m19.whd"
};

const char* lbl_805984E0[20] = {
    "vo_pl_f00.whd", "vo_pl_f01.whd", "vo_pl_f02.whd", "vo_pl_f03.whd", "vo_pl_f04.whd", "vo_pl_f05.whd",
    "vo_pl_f06.whd", "vo_pl_f07.whd", "vo_pl_f08.whd", "vo_pl_f09.whd", "vo_pl_f10.whd", "vo_pl_f11.whd",
    "vo_pl_f12.whd", "vo_pl_f13.whd", "vo_pl_f14.whd", "vo_pl_f15.whd", "vo_pl_f16.whd", "vo_pl_f17.whd",
    "vo_pl_f18.whd", "vo_pl_f19.whd"
};

const char* lbl_80598530[3] = {"frm", "brl", "stk"};

const char* lbl_8059853C[9] = {
    "wp_two", "wp_one", "wp_ham", "wp_lan", "wp_gun", "wp_gun", "wp_gun", "wp_swo", "wp_axe"
};

const char* lbl_807915E8[2] = {"iron", "bone"};

const char* lbl_80598560[9] = {"two", "one", "ham", "lan", "gun", "gun", "gun", "swo", "axe"};

u8 lbl_8079160C[4] = {0x18, 0x1C, 0x20, 0x24};

u8 se_volume_table[8] = {0x00, 0x12, 0x24, 0x36, 0x49, 0x5B, 0x6D, 0x7F};

const char* lbl_805988E0[48] = {
    "FS00_Blank.whd", "FS01_Desert.whd", "FS02_Kusa.whd", "FS03_branch.whd", "FS04_Soil.whd", "FS05_Stone.whd",
    "FS06_Water.whd", "FS07_jari.whd", "FS08_water-jari.whd", "FS09_water-sand.whd", "FS10_water-thru.whd",
    "FS11_wood.whd", "FS12_woodcreak.whd", "FS13_bone.whd", "FS14_rock.whd", "FS15_Sands.whd", "FS16_carpet.whd",
    "FS17_cracksoil.whd", "FS18_wetwood.whd", "FS19_tuta.whd", "FS20_numa.whd", "FS21_wet-kusa.whd",
    "FS22_wet-rock.whd", "FS23_hasu.whd", "FS24_bird-nest.whd", "FS25_wara.whd", "FS26_marble.whd",
    "FS27_marble_high.whd", "FS28_snow.whd", "FS29_ice.whd", "FS30_ice_rock.whd", "FS31_carcass.whd",
    "FS32_Kareha.whd", "FS33_damage.whd", "FS34_iron_grid.whd", "FS35_iron.whd", "FS36_doro.whd",
    "FS37_molten_metal.whd", "FS38_gake.whd", "FS39_gake_snow.whd", "FS40_cloth.whd", "FS41_rope.whd",
    "FS42_donou.whd", "FS_MLT_15_Sands.whd", "FS_MLT_02_Kusa.whd", "FS_MLT_05_Stone.whd", "FS_MLT_14_rock.whd",
    "FS43_Sands2.whd"
};

const char* lbl_80598A2C[7] = {
    "RTime_jingle00.whd", "RTime_jingle01.whd", "RTime_jingle02.whd", "RTime_jingle03.whd", "RTime_jingle04.whd",
    "RTime_jingle05.whd", "RTime_jingle06.whd"
};

SndPlSizes lbl_80598A48[9] = {
    {{0x20000, 0x16000, 0x0, 0x0}},
    {{0x10000, 0x26000, 0x0, 0x0}},
    {{0x20000, 0x16000, 0x0, 0x0}},
    {{0x24000, 0x12000, 0x0, 0x0}},
    {{0x21000, 0x9000, 0x5000, 0x7000}},
    {{0x21000, 0x9000, 0x5000, 0x7000}},
    {{0x21000, 0x9000, 0x5000, 0x7000}},
    {{0x16000, 0x20000, 0x0, 0x0}},
    {{0x1C000, 0x1A000, 0x0, 0x0}}
};

u8 lbl_80598AD8[44] = {
    0x00, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x01, 0x01, 0x02, 0x01, 0x02, 0x02,
    0x02, 0x01, 0x02, 0x02, 0x02, 0x01, 0x01, 0x01, 0x02, 0x02, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00
};

const char* chacha_dance_name_1[4] = {"npc_dance06", "npc_dance02", "npc_dance04", "npc_dance00"};

const char* chacha_dance_name_2[4] = {"npc_dance06", "npc_dance03", "npc_dance05", "npc_dance01"};

SndChunkRec snd_chunk_recs_0[43] = {
    {0, 0x1400, 0x60000},
    {1, 0x1400, 0x30000},
    {2, 0x1400, 0x80000},
    {3, 0x3000, 0x130000},
    {4, 0x1400, 0x5000},
    {5, 0x1400, 0x5000},
    {6, 0x1400, 0x5000},
    {7, 0x1400, 0x5000},
    {8, 0x1400, 0x5000},
    {9, 0x1400, 0x5000},
    {10, 0x1400, 0x5000},
    {11, 0x1400, 0x5000},
    {12, 0x1400, 0x5000},
    {13, 0x1400, 0x5000},
    {14, 0x1400, 0x5000},
    {15, 0x1400, 0x5000},
    {16, 0x1400, 0x5000},
    {17, 0x1400, 0x5000},
    {18, 0x1400, 0x5000},
    {19, 0x1400, 0x5000},
    {20, 0x1400, 0x11000},
    {21, 0x1400, 0x11000},
    {22, 0x1400, 0x11000},
    {23, 0x1400, 0x11000},
    {24, 0x1400, 0x36000},
    {25, 0x1400, 0x0},
    {26, 0x1400, 0x0},
    {27, 0x1400, 0x0},
    {28, 0x1400, 0x36000},
    {29, 0x1400, 0x0},
    {30, 0x1400, 0x0},
    {31, 0x1400, 0x0},
    {32, 0x1400, 0x36000},
    {33, 0x1400, 0x0},
    {34, 0x1400, 0x0},
    {35, 0x1400, 0x0},
    {36, 0x1400, 0x36000},
    {37, 0x1400, 0x0},
    {38, 0x1400, 0x0},
    {39, 0x1400, 0x0},
    {47, 0x1400, 0x18000},
    {48, 0x1400, 0x28000},
    {-1, 0x0, 0x0}
};

SndChunkRec snd_chunk_recs_1[33] = {
    {0, 0x1400, 0x60000},
    {1, 0x1400, 0x30000},
    {2, 0x1400, 0x80000},
    {3, 0x3000, 0x130000},
    {4, 0x1400, 0x5000},
    {5, 0x1400, 0x5000},
    {6, 0x1400, 0x5000},
    {7, 0x1400, 0x5000},
    {8, 0x1400, 0x5000},
    {9, 0x1400, 0x5000},
    {10, 0x1400, 0x5000},
    {11, 0x1400, 0x5000},
    {12, 0x1400, 0x5000},
    {13, 0x1400, 0x5000},
    {14, 0x1400, 0x5000},
    {15, 0x1400, 0x5000},
    {16, 0x1400, 0x5000},
    {17, 0x1400, 0x5000},
    {18, 0x1400, 0x5000},
    {19, 0x1400, 0x5000},
    {20, 0x1400, 0x11000},
    {21, 0x1400, 0x22000},
    {24, 0x1400, 0x36000},
    {25, 0x1400, 0x0},
    {26, 0x1400, 0x0},
    {27, 0x1400, 0x0},
    {28, 0x3000, 0x60000},
    {32, 0x1400, 0x10000},
    {33, 0x1400, 0x10000},
    {36, 0x1400, 0x22000},
    {47, 0x1400, 0x18000},
    {48, 0x1400, 0x28000},
    {-1, 0x0, 0x0}
};

SndChunkRec snd_chunk_recs_2[8] = {
    {40, 0x3000, 0x100000},
    {41, 0x3000, 0x100000},
    {42, 0x3000, 0xB0000},
    {43, 0x3000, 0x64000},
    {44, 0x3000, 0x64000},
    {45, 0x3000, 0x64000},
    {46, 0x1400, 0x40000},
    {-1, 0x0, 0x0}
};

SndChunkRec snd_chunk_recs_3[4] = {
    {40, 0x3000, 0x300000},
    {45, 0x3000, 0x64000},
    {46, 0x1400, 0x80000},
    {-1, 0x0, 0x0}
};

SndChunkRec snd_chunk_recs_4[27] = {
    {0, 0x1400, 0x60000},
    {1, 0x1400, 0x30000},
    {2, 0x1400, 0x80000},
    {3, 0x3000, 0x130000},
    {4, 0x1400, 0x5000},
    {5, 0x1400, 0x5000},
    {6, 0x1400, 0x5000},
    {7, 0x1400, 0x5000},
    {8, 0x1400, 0x5000},
    {9, 0x1400, 0x5000},
    {10, 0x1400, 0x5000},
    {11, 0x1400, 0x5000},
    {12, 0x1400, 0x5000},
    {13, 0x1400, 0x5000},
    {14, 0x1400, 0x5000},
    {15, 0x1400, 0x5000},
    {16, 0x1400, 0x5000},
    {17, 0x1400, 0x5000},
    {18, 0x1400, 0x5000},
    {19, 0x1400, 0x5000},
    {20, 0x1400, 0x11000},
    {30, 0x1400, 0x80000},
    {31, 0x3000, 0x200000},
    {32, 0x3000, 0x100000},
    {46, 0x1400, 0x80000},
    {47, 0x1400, 0x18000},
    {-1, 0x0, 0x0}
};

SndChunkRec snd_chunk_recs_5[10] = {
    {21, 0x1400, 0x11000},
    {22, 0x1400, 0x11000},
    {23, 0x1400, 0x11000},
    {24, 0x1400, 0x11000},
    {25, 0x1400, 0x11000},
    {26, 0x1400, 0x11000},
    {27, 0x1400, 0x11000},
    {28, 0x1400, 0x11000},
    {29, 0x1400, 0x11000},
    {-1, 0x0, 0x0}
};

SndChunkRec* lbl_80598F68[6] = {
    snd_chunk_recs_0, snd_chunk_recs_1, snd_chunk_recs_2, snd_chunk_recs_3, snd_chunk_recs_4, snd_chunk_recs_5
};

/* 0x800EEAE0 (0x414): brings the sound system up: the work record, the sound heap, the audio stack and the bank tables. */
extern "C" void snd_bank_loader_init(void)
{
    u32 cur;
    SndChunkRec* rec;
    s32 i;

    snd_wk = (SndWork*)system_w.layout_0x958;
    memset(snd_wk, 0, sizeof(SndWork));
    if (snd_exp_heap == 0) {
        snd_exp_heap = (u32)MEMCreateExpHeapEx((void*)0x92B78000, 0x80000, 4);
    }
    OSInitMutex(&snd_load_mutex);
    MEMInitAllocatorForExpHeap((MEMAllocator*)snd_heap_allocator, (MEMiHeapHead*)snd_exp_heap, 8);
    snd_heap_free_size = snd_heap_free_size_low = snd_heap_free_size_get((MEMiHeapHead*)snd_exp_heap);
    snd_audio_init();
    snd_wk->output_mode = system_w.sound_mode_0x03;
    snd_wk->output_dpl2 = 1;
    snd_output_mode_set(snd_output_mode_get());
    snd_stream_buffers_init(0x92BF8000, 0x60000, 3);
    snd_fx_hooks_set((void*)snd_heap_alloc, (void*)snd_heap_free);
    for (i = 0; i < 0x31; i++) {
        snd_wk->chunk_handle[i] = -1;
        snd_wk->whd_ofs[i] = 0;
        snd_wk->whd_size[i] = 0;
        snd_wk->tsb_ofs[i] = 0;
        snd_wk->tsb_size[i] = 0;
    }
    rec = lbl_80598F68[0];
    cur = 0x92D26000;
    snd_wk->whd_ofs[0] = cur;
    snd_wk->whd_size[0] = rec->whd_size;
    cur += rec->whd_size;
    snd_wk->tsb_ofs[0] = cur;
    snd_wk->tsb_size[0] = rec->tsb_size;
    cur += rec->tsb_size;
    snd_wk->total_size = cur;
    snd_wk->bank_base_2 = cur;
    snd_wk->bank_base_1 = cur;
    snd_wk->mem_area[0].base = 0x92BF8000;
    snd_wk->mem_area[0].size = 0x120000;
    snd_wk->mem_area[1].base = 0x92D18000;
    snd_wk->mem_area[1].size = 0x6000;
    snd_wk->mem_area[2].base = 0x92B78000;
    snd_wk->mem_area[2].size = 0x80000;
    snd_wk->mem_area[3].base = 0;
    snd_wk->mem_area[3].size = 0;
    snd_wk->se_param_a = 0xFF;
    snd_wk->se_param_b = 0xFF;
    snd_wk->demo_id = 0xFF;
    snd_wk->bgm_loading_1 = 0;
    snd_wk->bgm_loading_2 = 0;
}

/* 0x800EEEF4 (0x8): returns the largest block the sound heap can hand out. */
extern "C" u32 snd_heap_free_size_get(MEMiHeapHead* heap)
{
    return MEMGetAllocatableSizeForExpHeapEx(heap, 4);
}

/* 0x800EEEFC (0x168): lays the quest's bank table out from the file cursor and loads the quest's enemy SE banks. */
void quest_snd_wk_init(void)
{
    SndChunkRec* rec;
    u32 cur;
    s32 i;

    if (isServerSelectState() == 1 || PlayMode_ck() == 2 || GameMode_ck() == 0) {
        rec = lbl_80598F68[0];
    } else {
        rec = lbl_80598F68[1];
    }
    cur = snd_wk->bank_base_1;
    while (rec->chunk != -1) {
        if (rec->chunk != 0) {
            snd_wk->whd_ofs[rec->chunk] = cur;
            snd_wk->whd_size[rec->chunk] = rec->whd_size;
            cur += rec->whd_size;
            snd_wk->tsb_ofs[rec->chunk] = cur;
            snd_wk->tsb_size[rec->chunk] = rec->tsb_size;
            cur += rec->tsb_size;
        }
        rec++;
    }
    snd_wk->total_size = cur;
    for (i = 1; i < 0x31; i++) {
        snd_wk->chunk_handle[i] = -1;
        fn_800F0410(i);
    }
    snd_wk->stream_dma = 0;
    snd_wk->demo_id = 0xFF;
    snd_wk->bank_base_2 = cur;
    snd_em_bank_level_load(quest_em_level_get());
}

/* 0x800EF064 (0x150): loads a stage's sound banks: the area pair, the enemy banks, the environment and the BGM. */
extern "C" void snd_stage_bank_load(u8 map, u8 area, u8 rank)
{
    char name[0x100];

    snd_wk->load_count = 0;
    snd_area_bank_load(map, area);
    fn_800F0AD0(map);
    snd_area_env_bank_load(map, area, rank);
    quest_bgm_load(map, rank);
    if (map != 21 && map != 22) {
        map = move_work_state_ck() == 1 ? map : quest_pair_table_get(map, rank);
    }
    snd_wk->stream_dma = 0x92D1E000;
    snd_wk->stream_ok = 0;
    if ((s32)snd_wk->stream_dma != 0) {
        sprintf(name, "16/envdata/se_env_m%03d.dat", map);
        if (load_file_direct(name, snd_wk->stream_dma, 0x8000) != 0) {
            load_file_req(name, snd_wk->stream_dma, 0x8000, (u32)fn_800F02D4, 0, 0);
        }
    }
}

/* 0x800EF1B4 (0xBC): whether every bank `snd_stage_bank_load` counted has both halves in. */
extern "C" s32 snd_banks_loaded_ck(void)
{
    s32 i;
    s32 loaded;

    if (snd_wk->load_count == 0) {
        return 1;
    }
    loaded = 0;
    for (i = 0; i < 0x31; i++) {
        if ((snd_wk->whd_ofs[i] != 0 || snd_wk->tsb_ofs[i] != 0) && fn_800F04FC(i) == 1) {
            loaded++;
        }
    }
    return loaded >= snd_wk->load_count;
}

/* 0x800EF270 (0x2E4): loads the quest's NPC banks: the Felyne or plain common bank, the two Chacha dances and the mask. */
extern "C" void snd_quest_bgm_set(u8 track, u8 a, u8 b)
{
    u32 args[3];
    char name[0x100];

    snd_wk->chacha_dance[0] = a;
    snd_wk->chacha_dance[1] = b;
    args[1] = 0x1C;
    args[2] = 0x1C;
    fn_800F0410(0x1C);
    args[0] = 1;
    if (track == 6) {
        sprintf(name, "16/whd/%s.whd", "npc_comon_neko");
    } else {
        sprintf(name, "16/whd/%s.whd", "npc_comon");
    }
    load_file_req(name, snd_wk->whd_ofs[0x1C], snd_wk->whd_size[0x1C], (u32)fn_800F0230, 3, args);
    args[0] = 0;
    sprintf(name, "16/tsb/%s.tsb", "npc_comon");
    load_file_req(name, snd_wk->tsb_ofs[0x1C], snd_wk->tsb_size[0x1C], (u32)fn_800F0230, 3, args);
    args[1] = 0x20;
    args[2] = 0x20;
    fn_800F0410(0x20);
    args[0] = 1;
    sprintf(name, "16/whd/%s.whd", chacha_dance_name_1[a]);
    load_file_req(name, snd_wk->whd_ofs[0x20], snd_wk->whd_size[0x20], (u32)fn_800F0230, 3, args);
    args[0] = 0;
    sprintf(name, "16/tsb/%s.tsb", chacha_dance_name_1[a]);
    load_file_req(name, snd_wk->tsb_ofs[0x20], snd_wk->tsb_size[0x20], (u32)fn_800F0230, 3, args);
    args[1] = 0x21;
    args[2] = 0x21;
    fn_800F0410(0x21);
    args[0] = 1;
    sprintf(name, "16/whd/%s.whd", chacha_dance_name_2[b]);
    load_file_req(name, snd_wk->whd_ofs[0x21], snd_wk->whd_size[0x21], (u32)fn_800F0230, 3, args);
    args[0] = 0;
    sprintf(name, "16/tsb/%s.tsb", chacha_dance_name_2[b]);
    load_file_req(name, snd_wk->tsb_ofs[0x21], snd_wk->tsb_size[0x21], (u32)fn_800F0230, 3, args);
    args[1] = 0x24;
    args[2] = 0x24;
    fn_800F0410(0x24);
    args[0] = 1;
    sprintf(name, "16/whd/%s%02d.whd", "npc_kamen", track);
    load_file_req(name, snd_wk->whd_ofs[0x24], snd_wk->whd_size[0x24], (u32)fn_800F0230, 3, args);
    args[0] = 0;
    sprintf(name, "16/tsb/%s%02d.tsb", "npc_kamen", track);
    load_file_req(name, snd_wk->tsb_ofs[0x24], snd_wk->tsb_size[0x24], (u32)fn_800F0230, 3, args);
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

/* 0x800EF56C (0x5C): the chunk dance `id` is loaded in, looking at Chacha's dance `slot` first; -1 when neither holds it. */
extern "C" s32 chacha_dance_bank_find(u8 slot, u8 id)
{
    if (id == snd_wk->chacha_dance[slot]) {
        return slot == 0 ? 0x20 : 0x21;
    }
    if (id == snd_wk->chacha_dance[slot ^ 1]) {
        return slot == 0 ? 0x21 : 0x20;
    }
    return -1;
}

/* 0x800EF5C8 (0xC8): loads the tutorial bank pair into chunk 0x15. */
extern "C" void snd_quest_bgm_load(void)
{
    u32 args[3];
    char name[0x100];

    args[1] = 0x15;
    args[2] = 0x15;
    fn_800F0410(0x15);
    args[0] = 1;
    sprintf(name, "16/whd/%s.whd", "tutorial");
    load_file_req(name, snd_wk->whd_ofs[0x15], snd_wk->whd_size[0x15], (u32)fn_800F0230, 3, args);
    args[0] = 0;
    sprintf(name, "16/tsb/%s.tsb", "tutorial");
    load_file_req(name, snd_wk->tsb_ofs[0x15], snd_wk->tsb_size[0x15], (u32)fn_800F0230, 3, args);
}

/* 0x800EF690 (0x148): loads the result screen's bank pair (chunk 0x14) and the Felyne kitchen's (chunk 0x18). */
extern "C" void snd_result_banks_load(void)
{
    u32 args[3];
    char name[0x100];

    args[1] = 0x14;
    args[2] = 0x14;
    fn_800F0410(0x14);
    args[0] = 1;
    sprintf(name, "16/whd/%s.whd", "result");
    load_file_req(name, snd_wk->whd_ofs[0x14], 0x40000, (u32)fn_800F0230, 3, args);
    args[0] = 0;
    sprintf(name, "16/tsb/%s.tsb", "result");
    load_file_req(name, snd_wk->tsb_ofs[0x14], snd_wk->tsb_size[0x14], (u32)fn_800F0230, 3, args);
    args[1] = 0x18;
    args[2] = 0x18;
    fn_800F0410(0x18);
    args[0] = 1;
    sprintf(name, "16/whd/%s.whd", "montaku");
    load_file_req(name, snd_wk->whd_ofs[0x18], 0x20000, (u32)fn_800F0230, 3, args);
    args[0] = 0;
    sprintf(name, "16/tsb/%s.tsb", "montaku");
    load_file_req(name, snd_wk->tsb_ofs[0x18], snd_wk->tsb_size[0x18], (u32)fn_800F0230, 3, args);
}

/* ------------------------------------------------------------------------------------------------
 * Bodies
 * --------------------------------------------------------------------------------------------- */

/* Lays the six bank tables out from the current file cursor.  Chunk 0 is skipped (the system SE
 * bank, `system_se_load` loads it by hand). */
extern "C" void snd_bank_layout(u8 mode)
{
    SndChunkRec* rec = lbl_80598F68[4];
    u32 cur = snd_wk->bank_base_1;

    while (rec->chunk != -1) {
        if (rec->chunk != 0) {
            snd_wk->whd_ofs[rec->chunk] = cur;
            snd_wk->whd_size[rec->chunk] = rec->whd_size;
            cur += rec->whd_size;
            snd_wk->tsb_ofs[rec->chunk] = cur;
            snd_wk->tsb_size[rec->chunk] = rec->tsb_size;
            cur += rec->tsb_size;
        }
        rec++;
    }
    snd_wk->total_size = cur;
    if (mode != 1 && (system_w.field_0x7d3 == 1 || GameMode_ck() == 3)) {
        rec = lbl_80598F68[5];
        while (rec->chunk != -1) {
            if (rec->chunk != 0) {
                snd_wk->whd_ofs[rec->chunk] = cur;
                snd_wk->whd_size[rec->chunk] = rec->whd_size;
                cur += rec->whd_size;
                snd_wk->tsb_ofs[rec->chunk] = cur;
                snd_wk->tsb_size[rec->chunk] = rec->tsb_size;
                cur += rec->tsb_size;
            }
            rec++;
        }
        snd_wk->total_size = cur;
    }
    {
        s32 i = 1;
        do {
            snd_wk->chunk_handle[i] = -1;
            fn_800F0410(i);
            i++;
        } while (i < 0x31);
    }
    snd_wk->stream_dma = 0;
    snd_wk->demo_id = 0xFF;
    snd_wk->bank_base_2 = cur;
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
    snd_wk->stream_dma = 0x92D1E000;
    snd_wk->stream_ok = 0;
    if ((s32)snd_wk->stream_dma != 0) {
        sprintf(name, "16/envdata/se_env_m%03d.dat", a);
        if (load_file_direct(name, snd_wk->stream_dma, 0x8000) != 0) {
            load_file_req(name, snd_wk->stream_dma, 0x8000, (u32)fn_800F02D4, 0, 0);
        }
    }
}

/* One enemy slot's files: one `whd` per chunk plus the shared `tsb`. */
extern "C" void snd_player_banks_load(void)
{
    char name[0x88];
    u32 args[3];
    _PLW* work = (_PLW*)get_move_work_adrs(2);
    s32 max = get_move_work_max(2);
    _PLW* p;
    s32 i;
    s32 chunk;

    if (work != NULL && max != 0) {
        p = work;
        for (i = 0; i < max; i++, p++) {
            if (p->slot_active == 0) {
                continue;
            }
            chunk = p->chunk_ofs + 0x14;
            args[1] = chunk;
            args[2] = chunk;
            fn_800F0410(chunk);
            snd_wk->load_count++;
            if (p->se_name_set == 0) {
                sprintf(name, "16/whd/%s", lbl_80598350[p->se_name_idx]);
            } else {
                sprintf(name, "16/whd/%s", lbl_805984E0[p->se_name_idx]);
            }
            args[0] = 1;
            load_file_req(name, snd_wk->whd_ofs[chunk], snd_wk->whd_size[chunk], (u32)fn_800F0230, 3,
                          args);
            args[0] = 0;
            load_file_req("16/tsb/vo_all.tsb", snd_wk->tsb_ofs[chunk], snd_wk->tsb_size[chunk],
                          (u32)fn_800F0230, 3, args);
        }
        if (GameMode_ck() == 0 || GameMode_ck() == 3) {
            s32 j;
            for (j = 0; j < max; j++, work++) {
                pl_weapon_se_bank_load(work);
            }
        }
    }
}

/* The same for one already-selected slot. */
extern "C" void pl_voice_bank_load(_PLW* work)
{
    char name[0x80];
    u32 args[3];
    s32 chunk = work->chunk_ofs + 0x14;

    args[1] = chunk;
    args[2] = chunk;
    fn_800F0410(chunk);
    snd_wk->load_count++;
    if (work->se_name_set == 0) {
        sprintf(name, "16/whd/%s", lbl_80598350[work->se_name_idx]);
    } else {
        sprintf(name, "16/whd/%s", lbl_805984E0[work->se_name_idx]);
    }
    args[0] = 1;
    load_file_req(name, snd_wk->whd_ofs[chunk], snd_wk->whd_size[chunk],
                  (u32)fn_800F0230, 3, args);
    args[0] = 0;
    load_file_req("16/tsb/vo_all.tsb", snd_wk->tsb_ofs[chunk],
                  snd_wk->tsb_size[chunk], (u32)fn_800F0230, 3, args);
}

u8 wp_e_variant_ids_0[4] = {0x19, 0xFF, 0x00, 0x00};

u8 wp_e_variant_ids_1[4] = {0x09, 0xFF, 0x00, 0x00};

u8 wp_e_variant_ids_2[8] = {0x0B, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

u8* lbl_80599038[9] = {
    wp_e_variant_ids_0, NULL, NULL, NULL, NULL, NULL, NULL, wp_e_variant_ids_1, wp_e_variant_ids_2
};

/* Whether the id is one the `kind` table carries. */
extern "C" s32 fn_800EFD88(s32 id, s32 kind)
{
    const u8* p = lbl_80599038[kind];

    if (p == NULL) {
        return 0;
    }
    while (*p != 0xFF) {
        if ((u8)id == *p) {
            return 1;
        }
        p++;
    }
    return 0;
}

/* A player slot's files: the `com` pair plus up to three per-EQUIP `whd`s. */
extern "C" void pl_weapon_se_bank_load(_PLW* work)
{
    char name[0x88];
    u32 args[3];
    s32 type;
    s32 variant;
    s32 chunk;
    s32 next;
    s32 sub;
    const u8* tbl;
    u32* size;
    s32 i;

    if (work->slot_active == 0) {
        return;
    }
    type = Get_pl_type(&work->equipB, &work->equipC);
    if (type > 8) {
        return;
    }
    variant = equip_variant_get(&work->equipB);
    chunk = lbl_8079160C[work->chunk_ofs];
    size = lbl_80598A48[type].size;
    args[1] = chunk;
    args[2] = chunk;
    fn_800F0410(chunk);
    snd_wk->load_count++;
    tbl = lbl_80597554[type];
    sub = 0;
    if (tbl != NULL) {
        sub = tbl[variant];
    }
    if (sub == 0) {
        sub = 1;
    }
    args[0] = 0;
    sprintf(name, "16/tsb/%s_com.tsb", lbl_8059853C[type]);
    load_file_req(name, snd_wk->tsb_ofs[chunk], snd_wk->tsb_size[chunk], (u32)fn_800F0230, 3, args);
    args[0] = 1;
    if ((u32)(type - 4) <= 2) {
        sprintf(name, "16/whd/%s_com.whd", lbl_8059853C[type]);
    } else {
        sprintf(name, "16/whd/%s_com_%s.whd", lbl_8059853C[type], lbl_807915E8[sub - 1]);
    }
    load_file_req(name, snd_wk->whd_ofs[chunk], *size, (u32)fn_800F0230, 3, args);
    next = chunk + 1;
    snd_wk->whd_ofs[chunk + 1] = *size + snd_wk->whd_ofs[chunk];
    size++;
    if ((u32)(type - 4) <= 2) {
        for (i = 0; i < 3; i++) {
            chunk = next;
            args[1] = next;
            args[2] = next;
            fn_800F0410(next);
            snd_wk->load_count++;
            if (i != 0) {
                variant = equip_variant_get(&(&work->equipB)[i + 1]);
            }
            args[0] = 0;
            sprintf(name, "16/tsb/wp_%s.tsb", lbl_80598530[i]);
            load_file_req(name, snd_wk->tsb_ofs[next], snd_wk->tsb_size[next], (u32)fn_800F0230, 3, args);
            args[0] = 1;
            if (fn_800EFD88(variant, type) == 0) {
                sprintf(name, "16/whd/%s%03d.whd", lbl_80598530[i], variant);
            } else {
                sprintf(name, "16/whd/%s%03d_e.whd", lbl_80598530[i], variant);
            }
            load_file_req(name, snd_wk->whd_ofs[next], *size, (u32)fn_800F0230, 3, args);
            next++;
            if (i != 2) {
                snd_wk->whd_ofs[chunk + 1] = *size + snd_wk->whd_ofs[chunk];
                size++;
            }
        }
        return;
    }
    args[1] = next;
    args[2] = next;
    fn_800F0410(next);
    snd_wk->load_count++;
    args[0] = 0;
    sprintf(name, "16/tsb/%s.tsb", lbl_8059853C[type]);
    load_file_req(name, snd_wk->tsb_ofs[next], snd_wk->tsb_size[next], (u32)fn_800F0230, 3, args);
    args[0] = 1;
    if (fn_800EFD88(variant, type) == 0) {
        sprintf(name, "16/whd/%s%03d.whd", lbl_80598560[type], variant);
    } else {
        sprintf(name, "16/whd/%s%03d_e.whd", lbl_80598560[type], variant);
    }
    load_file_req(name, snd_wk->whd_ofs[next], *size, (u32)fn_800F0230, 3, args);
}

void title_snd_init(void)
{
    snd_bank_loader_reset();
    snd_bank_layout(0);
    title_se_load();
}

/* The `whd`/`tsb` completion callback: marks the half, and when both are in releases the chunk. */
extern "C" void fn_800F0230(void* unused_0, void* unused_1, void* unused_2, u32* args)
{
    u32 ctx = args[1];
    u32 chunk = args[2];

    if (args[0] == 0) {
        snd_wk->tsb_ready[chunk] = 1;
    } else {
        snd_wk->whd_ready[chunk] = 1;
    }
    if (snd_wk->tsb_ready[chunk] == 1 && snd_wk->whd_ready[chunk] == 1) {
        snd_chunk_install(ctx, chunk);
        snd_wk->chunk_ready[chunk] = 1;
    }
}

/* The `se_env` completion callback: fixes the slot table the `whd` header carries. */
extern "C" void fn_800F02D4(void* unused_0, SndWhdHeader* header)
{
    SndWhdSlot* slot;
    s32 i;

    if (header == NULL) {
        return;
    }
    slot = &header->slots[0];
    if (header->flag == 0) {
        return;
    }
    if (header->count != 0) {
        for (i = 0; i < header->count; i++, slot++) {
            slot->field_3C = slot->field_46;
            slot->field_46 = 0;
        }
        snd_wk->stream_ok = 1;
    }
}

/* The stream completion callback: one `srt` half is in, and once both are the stream is released. */
extern "C" void fn_800F033C(void* unused_0, void* unused_1, void* unused_2, u32* args)
{
    u32 chunk = args[1];
    u32 stream = args[2];
    u32 id = args[3];

    if (args[0] == 0) {
        snd_wk->tsb_ready[chunk] = 1;
        snd_wk->chunk_ready[stream] = 1;
    } else {
        snd_wk->whd_ready[chunk] = 1;
        snd_wk->demo_id = id;
        snd_wk->chunk_ready[stream] = 1;
    }
    if (snd_wk->tsb_ready[chunk] == 1 && snd_wk->whd_ready[chunk] == 1) {
        snd_chunk_install(chunk, stream);
        fn_800E80DC(chunk, 0, 0);
    }
}

/* Clears the three ready flags of one chunk. */
extern "C" void fn_800F0410(s32 idx)
{
    snd_wk->chunk_ready[idx] = 0;
    snd_wk->tsb_ready[idx] = 0;
    snd_wk->whd_ready[idx] = 0;
}

/* The `srt` completion callback: installs the stream and flips the ready flag. */
extern "C" void fn_800F0448(void* unused_0, void* ctx, void* unused_2, u32* args)
{
    u32 a = args[0];
    u32 b = args[1];
    u32 c = args[2];
    u32 d = args[3];
    u32 e = args[4];

    fn_800E4B4C(a, ctx, b, c);
    snd_wk->stream_ready[a] = 1;
    if (d != 0) {
        fn_800E4B4C(e, ctx, b, c);
        snd_wk->stream_ready[e] = 1;
    }
}

/* Whether one of the three BGM streams has its `srt` in. */
u32 srt_ready_ck(s32 stream)
{
    return snd_wk->stream_ready[stream] - 1 == 0;
}

/* Whether one chunk has both halves in. */
extern "C" u32 fn_800F04FC(s32 idx)
{
    return snd_wk->chunk_ready[idx] - 1 == 0;
}

extern "C" void snd_area_bank_load(u8 kind, u8 index)
{
    const u8* p = lbl_805978E8[kind];

    if (p == NULL) {
        return;
    }
    p += index * 2;
    fn_800F0560(p[0], p[1]);
}

/* The "no bank" SE state: bank 0, 30 voices. */
extern "C" void snd_bank_loader_reset(void)
{
    fn_800F0560(0, 0x1E);
}

/* The SE bank selector: only tells the mixer when the pair actually moves. */
extern "C" void fn_800F0560(u8 a, u8 b)
{
    if (snd_wk->se_param_a == a && snd_wk->se_param_b == b) {
        return;
    }
    snd_stream_bank_reset();
    snd_reverb_advance(0);
    snd_reverb_advance(1);
    snd_wk->se_param_a = a;
    snd_wk->se_param_b = b;
    snd_reverb_apply(0, snd_wk->se_param_a);
    snd_reverb_apply(0, snd_wk->se_param_b);
}

void system_se_load(void)
{
    char name[0x88];

    sprintf(name, "16/whd/%s.whd", lbl_80791590);
    load_file(name, snd_wk->whd_ofs[0], snd_wk->whd_size[0]);
    sprintf(name, "16/tsb/%s.tsb", lbl_80791590);
    load_file(name, snd_wk->tsb_ofs[0], snd_wk->tsb_size[0]);
    snd_chunk_install(0, 0);
    snd_wk->chunk_ready[0] = 1;
    fn_800F0560(0, 0x1E);
}

/* The per-kind SE bank list: 16 `{id, chunk}` records starting at chunk 4. */
extern "C" void fn_800F06AC(u8 unused, u8 kind)
{
    char name[0x88];
    u32 args[3];
    s32 n;
    s32 i;
    const u8* p;
    s32 chunk;

    for (i = 0; i < 0x10; i++) {
        snd_wk->se_slot[i].id = 0;
        snd_wk->se_slot[i].chunk = 0xFF;
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
        snd_wk->se_slot[n].id = id;
        snd_wk->se_slot[n].chunk = chunk;
        args[1] = chunk;
        args[2] = chunk;
        fn_800F0410(chunk);
        snd_wk->load_count++;
        args[0] = 1;
        sprintf(name, "16/whd/%s", lbl_805988E0[bank]);
        load_file_req(name, snd_wk->whd_ofs[chunk], snd_wk->whd_size[chunk],
                      (u32)fn_800F0230, 3, args);
        args[0] = 0;
        load_file_req("16/tsb/footstep_tsb.tsb", snd_wk->tsb_ofs[chunk],
                      snd_wk->tsb_size[chunk], (u32)fn_800F0230, 3, args);
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
    char name[0x80];
    u32 args[3];
    s32 i;
    const char* const* tbl;

    for (i = 1; i < 4; i++) {
        args[1] = i;
        args[2] = i;
        fn_800F0410(i);
        args[0] = 1;
        sprintf(name, "16/whd/%s.whd", lbl_80598190[i - 1]);
        load_file_req(name, snd_wk->whd_ofs[i], snd_wk->whd_size[i],
                      (u32)fn_800F0230, 3, args);
        args[0] = 0;
        sprintf(name, "16/tsb/%s.tsb", lbl_80598190[i - 1]);
        load_file_req(name, snd_wk->tsb_ofs[i], snd_wk->tsb_size[i],
                      (u32)fn_800F0230, 3, args);
    }
    fn_800F06AC(1, arg0);
    tbl = (arg0 == 0x15) ? lbl_805981C8 : lbl_80598200;
    for (i = 0x1E; i < 0x21; tbl++, i++) {
        args[1] = i;
        args[2] = i;
        fn_800F0410(i);
        args[0] = 1;
        sprintf(name, "16/whd/%s.whd", *tbl);
        load_file_req(name, snd_wk->whd_ofs[i], snd_wk->whd_size[i],
                      (u32)fn_800F0230, 3, args);
        args[0] = 0;
        sprintf(name, "16/tsb/%s.tsb", *tbl);
        load_file_req(name, snd_wk->tsb_ofs[i], snd_wk->tsb_size[i],
                      (u32)fn_800F0230, 3, args);
    }
    fn_800F15B0();
    snd_area_env_bank_load(arg0, 0, 0);
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
        snd_wk->load_count++;
        args[0] = 1;
        if (i == 2) {
            sprintf(tmp, "16/whd/%s.whd", lbl_80598174[i - 1]);
            cnvt_eur_fname(name, tmp);
        } else {
            sprintf(name, "16/whd/%s.whd", lbl_80598174[i - 1]);
        }
        load_file_req(name, snd_wk->whd_ofs[i], snd_wk->whd_size[i],
                      (u32)fn_800F0230, 3, args);
        args[0] = 0;
        sprintf(name, "16/tsb/%s.tsb", lbl_80598174[i - 1]);
        load_file_req(name, snd_wk->tsb_ofs[i], snd_wk->tsb_size[i],
                      (u32)fn_800F0230, 3, args);
    }
    fn_800F06AC(0, arg0);
    fn_800F15B0();
}

extern "C" s8 snd_chunk_handle_get(s32 chunk)
{
    s8 handle = (s8)snd_wk->chunk_handle[chunk];

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
        if (want == snd_wk->se_slot[i].id) {
            break;
        }
        i++;
    }
    if (i >= 0x10 || snd_wk->se_slot[i].chunk == 0xFF) {
        i = 0;
    }
    return snd_wk->se_slot[i].chunk;
}

u8 lbl_805990F0[12] = {0x03, 0x03, 0x02, 0x04, 0x01, 0x05, 0x00, 0x06, 0x01, 0x05, 0x00, 0x00};

/* Lays the per-kind bank table out and records which stream slots it may use. */
extern "C" void snd_em_bank_level_load(u8 kind)
{
    SndChunkRec* rec;
    u32 cur;

    if (kind != 4) {
        rec = lbl_80598F68[2];
    } else {
        rec = lbl_80598F68[3];
    }
    if (kind > 4) {
        snd_wk->se_auto[0] = 3;
        snd_wk->se_auto[1] = 3;
    } else {
        snd_wk->se_auto[0] = lbl_805990F0[kind * 2];
        snd_wk->se_auto[1] = lbl_805990F0[kind * 2 + 1];
    }
    cur = snd_wk->bank_base_2;
    while (rec->chunk != -1) {
        if (rec->chunk != 0) {
            snd_wk->whd_ofs[rec->chunk] = cur;
            snd_wk->whd_size[rec->chunk] = rec->whd_size;
            cur += rec->whd_size;
            snd_wk->tsb_ofs[rec->chunk] = cur;
            snd_wk->tsb_size[rec->chunk] = rec->tsb_size;
            cur += rec->tsb_size;
        }
        rec++;
    }
    snd_wk->total_size = cur;
}

extern "C" void fn_800F0EA0(void)
{
    s32 i;
    for (i = 0; i < 6; i++) {
        snd_wk->se_slot_id[i] = 0;
        snd_wk->se_slot_num[i] = 0;
    }
}

/* Releases one reference of an already loaded SE id. */
extern "C" void snd_em_se_slot_release(u8 id)
{
    s32 slot = fn_800F1250(id);

    if (slot >= 0) {
        snd_wk->se_slot_num[slot]--;
        if (snd_wk->se_slot_num[slot] == 0) {
            snd_wk->se_slot_id[slot] = 0;
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
        snd_wk->se_slot_num[slot]++;
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
        if (quest_em_level_get() == 4) {
            slot = 5;
            break;
        }
        slot = snd_wk->se_auto[0];
        {
            s32 end = slot + snd_wk->se_auto[1];
            for (; slot < end; slot++) {
                if (snd_wk->se_slot_num[slot] == 0) {
                    break;
                }
            }
        }
        if (slot >= 6) {
            return;
        }
        break;
    case 2:
        if (quest_em_level_get() == 0 && (want == 0xC || want == 0xE)) {
            slot = 2;
            if (snd_wk->se_slot_num[2] != 0) {
                return;
            }
        } else {
            for (slot = 0; slot < snd_wk->se_auto[0]; slot++) {
                if (snd_wk->se_slot_num[slot] == 0) {
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
    snd_wk->se_slot_num[slot]++;
    snd_wk->se_slot_id[slot] = id;
    snd_wk->se_slot_bank[slot] = slot + 0x28;
    snd_wk->se_slot_idx[slot] = slot + 0x28;
    args[1] = snd_wk->se_slot_bank[slot];
    args[2] = snd_wk->se_slot_idx[slot];
    fn_800F0410(snd_wk->se_slot_idx[slot]);
    args[0] = 1;
    sprintf(name, "16/whd/%s.whd", nm);
    load_file_req(name, snd_wk->whd_ofs[snd_wk->se_slot_idx[slot]],
                  snd_wk->whd_size[snd_wk->se_slot_idx[slot]], (u32)fn_800F0230, 3,
                  args);
    args[0] = 0;
    sprintf(name, "16/tsb/%s.tsb", nm);
    load_file_req(name, snd_wk->tsb_ofs[snd_wk->se_slot_idx[slot]],
                  snd_wk->tsb_size[snd_wk->se_slot_idx[slot]], (u32)fn_800F0230, 3,
                  args);
}

/* The slot an SE id already occupies, or -1. */
extern "C" s32 fn_800F1250(u8 id)
{
    s32 i;

    for (i = 0; i < 6; i++) {
        if (id == snd_wk->se_slot_id[i] && snd_wk->se_slot_num[i] != 0) {
            return i;
        }
    }
    return -1;
}

s32 get_em_se_bank(u8 id)
{
    s32 i;

    for (i = 0; i < 6; i++) {
        if (id == snd_wk->se_slot_id[i]) {
            return snd_wk->se_slot_bank[i];
        }
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
    return snd_wk->se_slot_bank[5];
}

/* The NPC voice banks: `c_npc_nekotaku` when the call is demoted, `c_npc_sansai` otherwise. */
extern "C" void snd_npc_voice_bank_load(u8 arg0, u8 arg1)
{
    char name[0x80];
    u32 args[3];

    if (arg0 == 9 || arg1 == 0) {
        snd_wk->se_slot_bank[5] = 0x2D;
        snd_wk->se_slot_idx[5] = 0x2D;
        args[1] = snd_wk->se_slot_bank[5];
        args[2] = snd_wk->se_slot_idx[5];
        fn_800F0410(snd_wk->se_slot_idx[5]);
        args[0] = 1;
        sprintf(name, "16/whd/%s.whd", "c_npc_nekotaku");
        load_file_req(name, snd_wk->whd_ofs[snd_wk->se_slot_idx[5]],
                      snd_wk->whd_size[snd_wk->se_slot_idx[5]], (u32)fn_800F0230, 3, args);
        args[0] = 0;
        sprintf(name, "16/tsb/%s.tsb", "c_npc_nekotaku");
        load_file_req(name, snd_wk->tsb_ofs[snd_wk->se_slot_idx[5]],
                      snd_wk->tsb_size[snd_wk->se_slot_idx[5]], (u32)fn_800F0230, 3, args);
    }
    args[1] = 0x30;
    args[2] = 0x30;
    fn_800F0410(0x30);
    snd_wk->load_count++;
    args[0] = 1;
    sprintf(name, "16/whd/%s.whd", "c_npc_sansai");
    load_file_req(name, snd_wk->whd_ofs[0x30], snd_wk->whd_size[0x30], (u32)fn_800F0230, 3, args);
    args[0] = 0;
    sprintf(name, "16/tsb/%s.tsb", "c_npc_sansai");
    load_file_req(name, snd_wk->tsb_ofs[0x30], snd_wk->tsb_size[0x30], (u32)fn_800F0230, 3, args);
}

/* The jingle `tsb` the quest BGM hands over to. */
extern "C" void fn_800F15B0(void)
{
    u32 args[3];

    args[1] = 0x2F;
    args[2] = 0x2F;
    snd_wk->chunk_ready[0x2F] = 0;
    snd_wk->tsb_ready[0x2F] = 0;
    args[0] = 0;
    load_file_req("16/tsb/Rtime_jingle.tsb", snd_wk->tsb_ofs[0x2F],
                  snd_wk->tsb_size[0x2F], (u32)fn_800F033C, 3, args);
}

/* The demo/movie `whd` of chunk 0x2F, with the id the callback stores. */
extern "C" void fn_800F1620(s32 arg0)
{
    char name[0x88];
    u32 args[4];

    args[1] = 0x2F;
    args[2] = 0x2F;
    args[3] = arg0;
    snd_wk->chunk_ready[0x2F] = 0;
    snd_wk->whd_ready[0x2F] = 0;
    snd_wk->demo_id = 0xFF;
    if (arg0 < 7) {
        args[0] = 1;
        sprintf(name, "16/whd/%s", lbl_80598A2C[arg0]);
        load_file_req(name, snd_wk->whd_ofs[0x2F], snd_wk->whd_size[0x2F],
                      (u32)fn_800F033C, 4, args);
    }
}

extern "C" void se_slot_req(s32 id)
{
    if (snd_wk->demo_id == (u8)id) {
        fn_800E80DC(0x2F, 0, 0);
        return;
    }
    fn_800F1620(id);
}

const char* lbl_80791638[2] = {"M_TOWN_00", "M_MURA_00"};

/* The quest map's `whd`/`tsb` pair, or the two demo movie streams. */
extern "C" void snd_area_env_bank_load(u8 arg0, u8 arg1, u8 arg2)
{
    char name[0x88];
    u32 args[3];
    s32 variant;
    u8 id;
    const u8* tbl;

    if (arg0 != 0x15 && arg0 != 0x16) {
        id = move_work_state_ck() == 1 ? arg0 : quest_pair_table_get(arg0, arg2);
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
        sprintf(name, "16/whd/M%02d_%02d.whd", id, variant);
        load_file_req(name, snd_wk->whd_ofs[0x2E], snd_wk->whd_size[0x2E], (u32)fn_800F0230, 3, args);
        args[0] = 0;
        sprintf(name, "16/tsb/M%02d_%02d.tsb", id, variant);
        load_file_req(name, snd_wk->tsb_ofs[0x2E], snd_wk->tsb_size[0x2E], (u32)fn_800F0230, 3, args);
        return;
    }
    args[0] = 1;
    sprintf(name, "16/whd/%s.whd", lbl_80791638[arg0 - 0x15]);
    load_file_req(name, snd_wk->whd_ofs[0x2E], snd_wk->whd_size[0x2E], (u32)fn_800F0230, 3, args);
    args[0] = 0;
    sprintf(name, "16/tsb/%s.tsb", lbl_80791638[arg0 - 0x15]);
    load_file_req(name, snd_wk->tsb_ofs[0x2E], snd_wk->tsb_size[0x2E], (u32)fn_800F0230, 3, args);
}

void title_se_load(void)
{
    u32 args[3];

    args[1] = 0x2E;
    args[2] = 0x2E;
    fn_800F0410(0x2E);
    args[0] = 1;
    load_file_req("16/whd/title.whd", snd_wk->whd_ofs[0x2E], snd_wk->whd_size[0x2E],
                  (u32)fn_800F0230, 3, args);
    args[0] = 0;
    load_file_req("16/tsb/title.tsb", snd_wk->tsb_ofs[0x2E], snd_wk->tsb_size[0x2E],
                  (u32)fn_800F0230, 3, args);
}

const char* stage_amb_streams_00[16] = {
    "16/srt/ambient/stream/m01_a00d.ssd", "16/srt/ambient/stream/m01_a01d.ssd", "16/srt/ambient/stream/m01_a02d.ssd",
    "16/srt/ambient/stream/m01_a03d.ssd", "16/srt/ambient/stream/m01_a04d.ssd", "16/srt/ambient/stream/m01_a05d.ssd",
    "16/srt/ambient/stream/m01_a06d.ssd", "16/srt/ambient/stream/m01_a07d_in1.ssd",
    "16/srt/ambient/stream/m01_a07d_in2.ssd", "16/srt/ambient/stream/m01_a07d_out.ssd",
    "16/srt/ambient/stream/m01_a08d.ssd", "16/srt/ambient/stream/m01_a09d.ssd",
    "16/srt/ambient/stream/m01_a11d_cat.ssd", "16/srt/ambient/stream/m01_a11d.ssd",
    "16/srt/ambient/stream/m01_a00d_in.ssd", "16/srt/ambient/stream/MQ_J_SUBQE.ssd"
};

const char* stage_amb_streams_11[16] = {
    "16/srt/ambient/stream/MQ_J_SUBQE.ssd", "16/srt/ambient/stream/m01_a04d.ssd",
    "16/srt/ambient/stream/m01_a06d.ssd", "16/srt/ambient/stream/m01_a07d_in1.ssd",
    "16/srt/ambient/stream/m01_a07d_in2.ssd", "16/srt/ambient/stream/m01_a08d.ssd",
    "16/srt/ambient/stream/m01_a11d.ssd", "16/srt/ambient/stream/m01_a11d_cat.ssd",
    "16/srt/ambient/stream/m01_a00n.ssd", "16/srt/ambient/stream/m01_a01n.ssd", "16/srt/ambient/stream/m01_a02n.ssd",
    "16/srt/ambient/stream/m01_a03n.ssd", "16/srt/ambient/stream/m01_a05n.ssd",
    "16/srt/ambient/stream/m01_a07n_out.ssd", "16/srt/ambient/stream/m01_a09n.ssd",
    "16/srt/ambient/stream/m01_a00d_in.ssd"
};

const char* stage_amb_streams_01[9] = {
    "16/srt/ambient/stream/m02_a00d.ssd", "16/srt/ambient/stream/m02_a03d.ssd", "16/srt/ambient/stream/m02_a04d.ssd",
    "16/srt/ambient/stream/m02_a05d.ssd", "16/srt/ambient/stream/m02_a07d.ssd", "16/srt/ambient/stream/m02_a09d.ssd",
    "16/srt/ambient/stream/m02_a10d.ssd", "16/srt/ambient/stream/m01_a11d.ssd",
    "16/srt/ambient/stream/MQ_J_SUBQE.ssd"
};

const char* stage_amb_streams_12[11] = {
    "16/srt/ambient/stream/MQ_J_SUBQE.ssd", "16/srt/ambient/stream/m02_a05d.ssd",
    "16/srt/ambient/stream/m02_a10d.ssd", "16/srt/ambient/stream/m01_a11d.ssd", "16/srt/ambient/stream/m02_a00n.ssd",
    "16/srt/ambient/stream/m02_a03n.ssd", "16/srt/ambient/stream/m02_a04n.ssd", "16/srt/ambient/stream/m02_a06n.ssd",
    "16/srt/ambient/stream/m02_a07n.ssd", "16/srt/ambient/stream/m02_a09n.ssd", "16/srt/ambient/stream/m02_a01n.ssd"
};

const char* stage_amb_streams_02[9] = {
    "16/srt/ambient/stream/m03_a00d.ssd", "16/srt/ambient/stream/m03_a01d.ssd",
    "16/srt/ambient/stream/m03_a02_in.ssd", "16/srt/ambient/stream/m03_a05d.ssd",
    "16/srt/ambient/stream/m03_a06_in.ssd", "16/srt/ambient/stream/m03_a06.ssd",
    "16/srt/ambient/stream/m03_a10d.ssd", "16/srt/ambient/stream/m03_a09d.ssd",
    "16/srt/ambient/stream/MQ_J_SUBQE.ssd"
};

const char* stage_amb_streams_13[12] = {
    "16/srt/ambient/stream/MQ_J_SUBQE.ssd", "16/srt/ambient/stream/m03_a06.ssd",
    "16/srt/ambient/stream/m03_a06_in.ssd", "16/srt/ambient/stream/m03_a02_in.ssd",
    "16/srt/ambient/stream/m03_a00n.ssd", "16/srt/ambient/stream/m03_a01n.ssd", "16/srt/ambient/stream/m03_a02n.ssd",
    "16/srt/ambient/stream/m03_a02n_in.ssd", "16/srt/ambient/stream/m03_a03n.ssd",
    "16/srt/ambient/stream/m03_a05n.ssd", "16/srt/ambient/stream/m03_a08n.ssd", "16/srt/ambient/stream/m03_a09n.ssd"
};

const char* stage_amb_streams_03[9] = {
    "16/srt/ambient/stream/m04_a00d.ssd", "16/srt/ambient/stream/m04_a01d.ssd", "16/srt/ambient/stream/m04_a02d.ssd",
    "16/srt/ambient/stream/m04_a03d.ssd", "16/srt/ambient/stream/m04_a04d.ssd", "16/srt/ambient/stream/m04_a06d.ssd",
    "16/srt/ambient/stream/m04_a07d.ssd", "16/srt/ambient/stream/m04_a09d.ssd",
    "16/srt/ambient/stream/MQ_J_SUBQE.ssd"
};

const char* stage_amb_streams_04[13] = {
    "16/srt/ambient/stream/m05_a00d.ssd", "16/srt/ambient/stream/m05_a01d.ssd", "16/srt/ambient/stream/m05_a02d.ssd",
    "16/srt/ambient/stream/m05_a04d.ssd", "16/srt/ambient/stream/m05_a05d.ssd", "16/srt/ambient/stream/m05_a06d.ssd",
    "16/srt/ambient/stream/m05_a07d.ssd", "16/srt/ambient/stream/m05_a08d.ssd", "16/srt/ambient/stream/m05_a10d.ssd",
    "16/srt/ambient/stream/m05_a00n.ssd", "16/srt/ambient/stream/m05_a01n.ssd", "16/srt/ambient/stream/m05_a02n.ssd",
    "16/srt/ambient/stream/MQ_J_SUBQE.ssd"
};

const char* stage_amb_streams_05[4] = {
    "16/srt/ambient/stream/m06_a00.ssd", "16/srt/ambient/stream/m06_a01.ssd", "16/srt/ambient/stream/m06_a02.ssd",
    "16/srt/ambient/stream/MQ_J_SUBQE.ssd"
};

const char* stage_amb_streams_06[5] = {
    "16/srt/ambient/stream/MQ_J_SUBQE.ssd", "16/srt/ambient/stream/m07_a00d.ssd",
    "16/srt/ambient/stream/m07_a00n.ssd", "16/srt/ambient/stream/m01_a07d_in1.ssd",
    "16/srt/ambient/stream/m01_a07d_in2.ssd"
};

const char* stage_amb_streams_07[7] = {
    "16/srt/ambient/stream/m08_a00d.ssd", "16/srt/ambient/stream/m08_a01d.ssd",
    "16/srt/ambient/stream/m08_a02d_out.ssd", "16/srt/ambient/stream/m11_a01n_out.ssd",
    "16/srt/ambient/stream/m01_a07d_in1.ssd", "16/srt/ambient/stream/MQ_J_SUBQE.ssd",
    "16/srt/ambient/stream/MQ_J_MLT_1.ssd"
};

const char* stage_amb_streams_08[6] = {
    "16/srt/ambient/stream/m08_a00d.ssd", "16/srt/ambient/stream/m08_a01d.ssd",
    "16/srt/ambient/stream/m08_a02d_out.ssd", "16/srt/ambient/stream/m01_a07d_in1.ssd",
    "16/srt/ambient/stream/MQ_J_SUBQE.ssd", "16/srt/ambient/stream/MQ_J_MLT_1.ssd"
};

const char* stage_amb_streams_21[4] = {
    "16/srt/ambient/stream/mura_a00_day.ssd", "16/srt/ambient/stream/mura_a00_night.ssd",
    "16/srt/ambient/stream/mura_a02_day.ssd", "16/srt/ambient/stream/mura_a02_night.ssd"
};

const char* stage_amb_streams_20[10] = {
    "16/srt/ambient/stream/TD_a00_GATE.ssd", "16/srt/ambient/stream/TD_a03_YADO.ssd",
    "16/srt/ambient/stream/TD_a04_BUKI.ssd", "16/srt/ambient/stream/TD_a05_YADO1.ssd",
    "16/srt/ambient/stream/TD_a06_YADO2.ssd", "16/srt/ambient/stream/TD_a07_YADO3.ssd",
    "16/srt/ambient/stream/TN_a00_GATE.ssd", "16/srt/ambient/stream/TN_a03_YADO.ssd",
    "16/srt/ambient/stream/TN_a06_YADO2.ssd", "16/srt/ambient/stream/T_KUJIRA_KAZE.ssd"
};

const char* lbl_8059A554[25] = {
    "16/srt/bgm/stream/ML_V_SHINE.ssd", "16/srt/bgm/stream/ML_V_NIGHT.ssd", "16/srt/bgm/stream/ML_V_FARMS.ssd",
    "16/srt/bgm/stream/ML_V_SHIPS.ssd", "16/srt/bgm/stream/ML_T_SHINE.ssd", "16/srt/bgm/stream/ML_T_NIGHT.ssd",
    "16/srt/bgm/stream/ML_T_HANGK.ssd", "16/srt/bgm/stream/ML_T_HTL01.ssd", "16/srt/bgm/stream/ML_T_HTL02.ssd",
    "16/srt/bgm/stream/ML_T_HTL03.ssd", "16/srt/bgm/stream/ML_J_SH_NI.ssd", "16/srt/bgm/stream/ML_J_NI_SH.ssd",
    "16/srt/bgm/stream/ML_J_SAVES.ssd", "16/srt/bgm/stream/ML_T_SKB01.ssd", "16/srt/bgm/stream/ML_T_SKB02.ssd",
    "16/srt/bgm/stream/ML_T_SKB03.ssd", "16/srt/bgm/stream/ML_T_SKB04.ssd", "16/srt/bgm/stream/ML_T_SKB05.ssd",
    "16/srt/bgm/stream/ML_T_SKB06.ssd", "16/srt/bgm/stream/ML_T_SKB07.ssd", "16/srt/bgm/stream/ML_T_SKB08.ssd",
    "16/srt/bgm/stream/ML_T_WHALE.ssd", "16/srt/bgm/stream/ML_V_DKRAG.ssd", "16/srt/bgm/stream/ML_NTLOBBY.ssd",
    "16/srt/bgm/stream/MQ_J_SUBQE.ssd"
};

const char* lbl_8059AA14[31] = {
    "16/srt/bgm/stream/MQ_J_FOUND.ssd", "16/srt/bgm/stream/MQ_J_1OCHI.ssd", "16/srt/bgm/stream/MQ_J_3OCHI.ssd",
    "16/srt/bgm/stream/MQ_Q_CLEAR.ssd", "16/srt/bgm/stream/MQ_MAPST01.ssd", "16/srt/bgm/stream/MQ_FIGHT01.ssd",
    "16/srt/bgm/stream/MQ_FIGHTJJ.ssd", "16/srt/bgm/stream/MQ_MAPST02.ssd", "16/srt/bgm/stream/MQ_MAPST03.ssd",
    "16/srt/bgm/stream/MQ_MAPST04.ssd", "16/srt/bgm/stream/MQ_MAPST05.ssd", "16/srt/bgm/stream/MQ_MAPST07.ssd",
    "16/srt/bgm/stream/MQ_MAPST89.ssd", "16/srt/bgm/stream/MQ_FIGHT02.ssd", "16/srt/bgm/stream/MQ_FIGHT03.ssd",
    "16/srt/bgm/stream/MQ_FIGHT04.ssd", "16/srt/bgm/stream/MQ_FIGHT05.ssd", "16/srt/bgm/stream/MQ_FIGHT89.ssd",
    "16/srt/bgm/stream/MQ_J_MONTK.ssd", "16/srt/bgm/stream/MQ_FIGHTTI.ssd", "16/srt/bgm/stream/MQ_FIGHTK1.ssd",
    "16/srt/bgm/stream/MQ_FIGHTK2.ssd", "16/srt/bgm/stream/MQ_CSELECT.ssd", "16/srt/bgm/stream/MQ_FIGHTEY.ssd",
    "16/srt/bgm/stream/MQ_FIGHTD1.ssd", "16/srt/bgm/stream/MQ_FIGHTD2.ssd", "16/srt/bgm/stream/MQ_NTLOBBY.ssd",
    "16/srt/bgm/stream/MQ_NT_MENU.ssd", "16/srt/bgm/stream/MQ_ST_MENU.ssd", "16/srt/bgm/stream/MQ_FIGHTKR.ssd",
    "16/srt/bgm/stream/MQ_FIGHT_N.ssd"
};

const char* lbl_8059AAFC[3] = {
    "16/srt/bgm/stream/QUEST_CLEAR.ssd", "16/srt/bgm/stream/QUEST_FAILED.ssd", "16/srt/bgm/stream/QUEST_RETIRE.ssd"
};

const char* lbl_8059AC98[10] = {
    "16/srt/demo/stream/MH3_TGSDVD-New_PL2.ssd", "16/srt/demo/stream/MH3_Opening_PL2.ssd",
    "16/srt/demo/stream/MH3_V-OP_PL2.ssd", "16/srt/demo/stream/MH3_V-END_PL2.ssd",
    "16/srt/demo/stream/MH3_T-OP_PL2.ssd", "16/srt/demo/stream/MH3_T-END_PL2.ssd",
    "16/srt/demo/stream/MH3_Pelican_PL2.ssd", "16/srt/demo/stream/MH3_SabannaJoe_PL2.ssd",
    "16/srt/demo/stream/MH3_Anko_PL2.ssd", "16/srt/demo/stream/MH3_KazanSoryu_PL2.ssd"
};

const char* lbl_8059AEB0[23] = {
    NULL, "16/srt/ambient/MH3AMB_MAP01_D.srt", "16/srt/ambient/MH3AMB_MAP02_D.srt",
    "16/srt/ambient/MH3AMB_MAP03_D.srt", "16/srt/ambient/MH3AMB_MAP04_D.srt", "16/srt/ambient/MH3AMB_MAP05_D.srt",
    "16/srt/ambient/MH3AMB_MAP06_D.srt", "16/srt/ambient/MH3AMB_MAP07_D.srt", "16/srt/ambient/MH3AMB_MAP08_D.srt",
    "16/srt/ambient/MH3AMB_MAP09_D.srt", "16/srt/ambient/MH3AMB_MAP05_D.srt", "16/srt/ambient/MH3AMB_MAP08_D.srt",
    "16/srt/ambient/MH3AMB_MAP01_N.srt", "16/srt/ambient/MH3AMB_MAP02_N.srt", "16/srt/ambient/MH3AMB_MAP03_N.srt",
    "16/srt/ambient/MH3AMB_MAP04_D.srt", "16/srt/ambient/MH3AMB_MAP05_D.srt", "16/srt/ambient/MH3AMB_MAP06_D.srt",
    "16/srt/ambient/MH3AMB_MAP07_D.srt", "16/srt/ambient/MH3AMB_MAP08_D.srt", "16/srt/ambient/MH3AMB_MAP08_D.srt",
    "16/srt/ambient/MH3AMB_TOWN.srt", "16/srt/ambient/MH3AMB_MURA.srt"
};

const char** lbl_8059AF0C[23] = {
    NULL, stage_amb_streams_00, stage_amb_streams_01, stage_amb_streams_02, stage_amb_streams_03,
    stage_amb_streams_04, stage_amb_streams_05, stage_amb_streams_06, stage_amb_streams_07, stage_amb_streams_08,
    stage_amb_streams_04, stage_amb_streams_07, stage_amb_streams_11, stage_amb_streams_12, stage_amb_streams_13,
    stage_amb_streams_03, stage_amb_streams_04, stage_amb_streams_05, stage_amb_streams_06, stage_amb_streams_07,
    stage_amb_streams_07, stage_amb_streams_20, stage_amb_streams_21
};

u8 lbl_8059AF68[24] = {
    0x00, 0x10, 0x09, 0x09, 0x09, 0x0D, 0x04, 0x05, 0x07, 0x06, 0x0D, 0x07, 0x10, 0x0B, 0x0C, 0x09,
    0x0D, 0x04, 0x05, 0x07, 0x07, 0x0B, 0x04, 0x00
};

SndDemoCfg lbl_8059AF80[49] = {
    {0, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0},
    {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0},
    {1, 0, 1, 0}, {1, 0, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0},
    {0, 0, 0, 0}, {1, 0, 1, 1}, {1, 0, 1, 0}, {1, 0, 1, 0}, {1, 0, 1, 1}, {1, 0, 1, 0},
    {1, 0, 1, 0}, {1, 0, 1, 1}, {1, 0, 1, 1}, {1, 0, 1, 1}, {1, 0, 1, 1}, {1, 0, 1, 0},
    {1, 0, 1, 0}, {1, 0, 1, 0}, {1, 0, 1, 0}, {1, 0, 1, 0}, {1, 0, 1, 0}, {1, 0, 1, 1},
    {1, 0, 1, 0}, {1, 0, 1, 0}, {1, 0, 1, 0}, {0, 0, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0},
    {0, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 1, 0},
    {0, 0, 0, 0}
};

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
    alt = mapno != stage_map_kind_get(mapno);
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
        snd_wk->stream_ready[0] = 0;
    }
    if (cfg->bgm != 0) {
        fn_800E4D60(2);
        snd_wk->stream_ready[2] = 0;
    }
    if (demo_play_ck() == 1 && (index == 0xE || index == 0x2B || index == 0x2C || index == 0x2F)) {
        PlayStream(1, 1);
        PlayStream(2, 1);
        fn_800E4D60(2);
        snd_wk->stream_ready[2] = 0;
    }
    demo_stream_slot[0] = 0xFF;
    demo_stream_slot[1] = 0xFF;
    if (cfg->se != 0) {
        snd_wk->bgm_loading_2 = 1;
        sprintf(name, "16/srt/demo/MH3_DEMO_SE_%02d", index);
        if (cfg->se_variant == 1) {
            if (alt == 0) {
                strcat(name, ".srt");
                sprintf(demo_se_stream_path, "16/srt/demo/stream/demo_%02d_se.ssd", index);
            } else {
                strcat(name, "_N.srt");
                sprintf(demo_se_stream_path, "16/srt/demo/stream/demo_%02d_se_n.ssd", index);
            }
        } else {
            strcat(name, ".srt");
            sprintf(demo_se_stream_path, "16/srt/demo/stream/demo_%02d_se.ssd", index);
        }
        demo_se_stream_name = demo_se_stream_path;
        args[0] = 0;
        args[1] = (u32)&demo_se_stream_name;
        args[2] = 1;
        args[3] = 0;
        args[4] = 0;
        load_file_req(name, 0x92D18000, 0x2000, (u32)fn_800F0448, 5, args);
        demo_stream_slot[0] = 0;
    }
    for (;;) {
        if (demo_play_ck() == 1 && (index == 0xE || index == 0x2B || index == 0x2C || index == 0x2F)) {
            snd_wk->bgm_loading_1 = 1;
            sprintf(name, "16/srt/demo/MH3_DEMO_BGM_%02d", index);
            strcat(name, ".srt");
        } else if (cfg->bgm != 0) {
            snd_wk->bgm_loading_1 = 1;
            sprintf(name, "16/srt/demo/MH3_DEMO_BGM_%02d", index);
            if (cfg->bgm_variant == 1) {
                if (alt == 0) {
                    strcat(name, ".srt");
                } else {
                    strcat(name, "_N.srt");
                }
            } else {
                strcat(name, ".srt");
            }
        } else {
            break;
        }
        sprintf(demo_bgm_stream_path, "16/srt/demo/stream/demo_%02d_bgm.ssd", index);
        demo_bgm_stream_name = demo_bgm_stream_path;
        args[0] = 2;
        args[1] = (u32)&demo_bgm_stream_name;
        args[2] = 1;
        args[3] = 0;
        args[4] = 0;
        load_file_req(name, 0x92D1C000, 0x2000, (u32)fn_800F0448, 5, args);
        demo_stream_slot[0] = 2;
        break;
    }
    if (demo_play_ck() == 0) {
        if ((u8)(index + 0xF2) <= 4U || index == 0x2B) {
            snd_bgm_hold_set();
        }
    } else if ((u8)(index + 0xF1) <= 3U) {
        snd_bgm_hold_set();
    }
    return 0;
}

s32 demo_bgm_ready_ck(void)
{
    u8* p = demo_stream_slot;
    s32 i = 0;
    s32 ready = 0;

    do {
        if (*p != 0xFF) {
            if (srt_ready_ck(demo_stream_slot[0]) == 1) {
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

    snd_wk->bgm_loading_2 = 0;
    snd_wk->bgm_loading_1 = 0;
    index = move_work_state_ck() == 1 ? index : quest_pair_table_get(index, arg1);
    fn_800E4D60(0);
    fn_800E4D60(1);
    fn_800E4D60(2);
    snd_wk->stream_ready[0] = 0;
    snd_wk->stream_ready[1] = 0;
    snd_wk->stream_ready[2] = 0;
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
    load_file_req("16/srt/bgm/MH3BGM_QUEST.srt", 0x92D1A000, 0x2000, (u32)fn_800F0448, 5, args);
}

/* The field BGM: the day/night stream plus the quest `srt`. */
extern "C" void stage_bgm_reload(void)
{
    u32 args[5];
    u8 mapno;

    snd_wk->bgm_loading_2 = 0;
    snd_wk->bgm_loading_1 = 0;
    mapno = get_now_mapno();
    get_gm_daynight();
    fn_800E4D60(0);
    fn_800E4D60(2);
    snd_wk->stream_ready[0] = 0;
    snd_wk->stream_ready[2] = 0;
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
    load_file_req("16/srt/bgm/MH3BGM_QUEST.srt", 0x92D1C000, 0x2000, (u32)fn_800F0448, 5, args);
}

/* The quest-clear jingle. */
extern "C" void snd_quest_end_stream_load(void)
{
    u32 args[5];

    fn_800E4D60(2);
    snd_wk->stream_ready[0] = 0;
    snd_wk->stream_ready[1] = 0;
    snd_wk->stream_ready[2] = 0;
    args[0] = 2;
    args[1] = (u32)lbl_8059AAFC;
    args[2] = 3;
    args[3] = 2;
    args[4] = 0;
    load_file_req("16/srt/bgm/MH3_QUEST_END.srt", 0x92D1C000, 0x2000, (u32)fn_800F0448, 5, args);
}

void movie_bgm_load(void)
{
    u32 args[5];

    fn_800E4D60(0);
    snd_wk->stream_ready[0] = 0;
    args[0] = 0;
    args[1] = (u32)lbl_8059AC98;
    args[2] = 0x0A;
    args[3] = 0;
    args[4] = 0;
    load_file_req("16/srt/demo/MH3_MOVIE.srt", 0x92D18000, 0x2000, (u32)fn_800F0448, 5, args);
}

void title_bgm_load(void)
{
    u32 args[5];

    fn_800E4D60(1);
    snd_wk->stream_ready[1] = 0;
    args[0] = 1;
    args[1] = (u32)lbl_8059AA14;
    args[2] = 0x1F;
    args[3] = 0;
    args[4] = 0;
    load_file_req("16/srt/bgm/MH3BGM_QUEST.srt", 0x92D1C000, 0x2000, (u32)fn_800F0448, 5, args);
}

/* The lobby BGM. */
extern "C" void lobby_bgm_load(void)
{
    u32 args[5];

    fn_800E4D60(1);
    snd_wk->stream_ready[1] = 0;
    args[0] = 1;
    args[1] = (u32)lbl_8059A554;
    args[2] = 0x19;
    args[3] = 0;
    args[4] = 0;
    load_file_req("16/srt/bgm/MH3BGM_LOBBY.srt", 0x92D1A000, 0x2000, (u32)fn_800F0448, 5, args);
}

/* The lobby BGM with its two stream variants. */
extern "C" void fn_800F2328(u8 arg0, u8 arg1)
{
    u32 args[5];

    snd_wk->bgm_loading_2 = 0;
    snd_wk->bgm_loading_1 = 0;
    fn_800E4D60(0);
    fn_800E4D60(1);
    fn_800E4D60(2);
    snd_wk->stream_ready[0] = 0;
    snd_wk->stream_ready[1] = 0;
    snd_wk->stream_ready[2] = 0;
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
    load_file_req("16/srt/bgm/MH3BGM_LOBBY.srt", 0x92D1A000, 0x2000, (u32)fn_800F0448, 5, args);
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
    load_file_req("16/srt/bgm/MH3BGM_LOBBY.srt", 0x92D1A000, 0x2000, (u32)fn_800F0448, 5, args);
}

/* The four pan/pitch halves of one voice. */
extern "C" void fn_800F2540(u32 arg0, s32 arg1, SndVoiceMix* out)
{
    SndVoice* v;

    out->pan = 0;
    out->field_02 = 0;
    out->field_04 = 0;
    out->field_06 = 0;
    if (arg0 < 0x31 && fn_800F04FC(arg0) != 0) {
        v = (SndVoice*)fn_800E7FC4(arg0, arg1);
        if (v != NULL) {
            out->pan = v->mix.pan;
            out->field_02 = v->mix.field_02;
            out->field_04 = v->mix.field_04;
            out->field_06 = v->mix.field_06;
        }
    }
}

/* Snapshots one bank's 256 ids into the three lookup tables. */
extern "C" void snd_chunk_ids_snapshot(u32 arg0)
{
    u8* t1;
    u8* t2;
    u8* t3;
    s32 i;

    if (arg0 >= 0x31) {
        return;
    }
    t1 = &snd_wk->chunk_id_1[arg0][0];
    t2 = &snd_wk->chunk_id_2[arg0][0];
    t3 = &snd_wk->chunk_id_3[arg0][0];
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
    return snd_wk->chunk_id_1[arg0][arg1];
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
        return snd_wk->chunk_id_2[arg0][arg1];
    }
    return snd_wk->chunk_id_3[arg0][arg1];
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

extern "C" s32 fn_800F27C4(u32 arg0, s32 arg1, s8 detune)
{
    SndVoice* v;

    if (arg0 >= 0x31) {
        return 0;
    }
    if (fn_800F04FC((s32)arg0) == 0) {
        return 0;
    }
    v = (SndVoice*)fn_800E7FC4(arg0, arg1);
    if (v == NULL) {
        return 0;
    }
    v->detune = detune;
    return 1;
}

extern "C" u16 fn_800F284C(u32 arg0, s32 arg1)
{
    SndVoice* v;

    if (arg0 >= 0x31) {
        return 0;
    }
    v = (SndVoice*)fn_800E7FC4(arg0, arg1);
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

extern "C" u32 snd_stream_dma_get(void)
{
    if (snd_wk->stream_ok == 1) {
        return snd_wk->stream_dma;
    }
    return 0;
}

extern "C" u16 fn_800F2924(s32 arg0, s32 arg1)
{
    SndVoice* v;

    if (srt_ready_ck(arg0) == 0) {
        return 1;
    }
    v = (SndVoice*)fn_800E4F4C(arg0, arg1);
    if (v == NULL) {
        return 1;
    }
    return v->loop_len;
}

extern "C" u8 snd_stream_bgm_flag_get(s32 channel, u32 id)
{
    return fn_800F2924(channel, id) == 0;
}

/* Cast view: retail's prototype takes the SE level as an `s16` (the call sign-extends it); the owner declares `s32`. */
#define snd_se_level_set_c1 ((void (*)(s16))snd_se_level_set)

void set_SE_volume(u8 index)
{
    snd_se_level_set_c1(se_volume_table[index]);
    fn_800F2A1C(se_volume_table[index], 1);
}

void set_BGM_volume(u8 index)
{
    fn_800F2A1C(se_volume_table[index], 0);
}

extern "C" void fn_800F2A1C(u8 volume, u8 is_bgm)
{
    f32 v = (f32)volume / 127.0f;

    if (v < 0.0f) {
        v = 0.0f;
    }
    if (v > 1.0f) {
        v = 1.0f;
    }
    fn_800E4908(is_bgm, v);
}

extern "C" u8 snd_bgm_loading_get(void)
{
    return snd_wk->bgm_loading_2;
}
