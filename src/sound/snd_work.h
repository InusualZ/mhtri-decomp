/* sound/snd_work.h - the sound bank loader's work record (`sound/snd_bank_loader.cpp`'s `snd_wk`), shared with
 * `ef/system_core.cpp`, whose memory layout places it at `system_w.layout_0x958` (docs/plan.md 6.5 rule 1). */
#ifndef MHTRI_SOUND_SND_WORK_H
#define MHTRI_SOUND_SND_WORK_H

#include "types.h"

/* One `{id, chunk}` slot of the SE bank list `fn_800F06AC` fills and `fn_800F0C74` looks up. */
typedef struct SndSeSlot {
    /* +0x00 */ u8 id;         /* 0xFF when the slot is free */
    /* +0x01 */ u8 chunk;      /* the chunk index its files were loaded into */
} SndSeSlot; /* size: 0x2 */

/* One `{base, size}` memory area `snd_bank_loader_init` records (GUESS: the stream, FX and heap areas). */
typedef struct SndMemArea {
    /* +0x00 */ u32 base;
    /* +0x04 */ u32 size;
} SndMemArea; /* size: 0x8 */

typedef struct SndWork {
    /* +0x0000 */ u8   output_mode;     /* `system_w`'s sound mode byte: 0 mono, 2 surround, else stereo */
    /* +0x0001 */ u8   output_dpl2;     /* with mode 2: non-zero selects output mode 3 over 2 (GUESS name) */
    /* +0x0002 */ u8   pad_0002[0x02];
    /* +0x0004 */ u32  total_size;      /* the running file offset both bank walkers leave behind */
    /* +0x0008 */ u32  bank_base_2;     /* cursor `snd_em_bank_level_load` walks from */
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
    /* +0x971C */ u8   se_auto[2];      /* snd_em_bank_level_load's {start, count} */
    /* +0x971E */ s8   chunk_handle[0x31]; /* the stream handle `snd_chunk_install` records, -1 when none */
    /* +0x974F */ u8   pad_974F[0x01];
    /* +0x9750 */ SndMemArea mem_area[4]; /* set once by `snd_bank_loader_init` */
    /* +0x9770 */ u8   pad_9770[0x04];
    /* +0x9774 */ u8   chunk_ready[0x31]; /* both halves are in */
    /* +0x97A5 */ u8   stream_ready[3];  /* the three BGM streams (`fn_800F0448`, `srt_ready_ck`) */
    /* +0x97A8 */ u8   demo_id;
    /* +0x97A9 */ u8   se_param_a;       /* fn_800F0560 caches the pair it was last called with */
    /* +0x97AA */ u8   se_param_b;
    /* +0x97AB */ u8   load_count;
    /* +0x97AC */ u8   chacha_dance[2];  /* the dance banks `snd_quest_bgm_set` loads into chunks 0x20/0x21 */
    /* +0x97AE */ u8   bgm_loading_1;
    /* +0x97AF */ u8   bgm_loading_2;
    /* +0x97B0 */ u32  stream_dma;       /* the address `snd_stream_dma_get` hands out */
    /* +0x97B4 */ u32  stream_ok;
} SndWork; /* size: 0x97B8 */

#endif /* MHTRI_SOUND_SND_WORK_H */
