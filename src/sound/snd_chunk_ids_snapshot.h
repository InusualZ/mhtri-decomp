/* sound/snd_chunk_ids_snapshot.h - leaf header (docs/plan.md 6.5 rule 2): `snd_chunk_ids_snapshot`, which `sound/snd_bank_loader.cpp` owns. */
#ifndef MHTRI_SOUND_SND_CHUNK_IDS_SNAPSHOT_H
#define MHTRI_SOUND_SND_CHUNK_IDS_SNAPSHOT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800F25DC (0xA4): snapshots one bank's 256 ids into the three lookup tables. */
void snd_chunk_ids_snapshot(u32 arg0);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_SND_CHUNK_IDS_SNAPSHOT_H */
