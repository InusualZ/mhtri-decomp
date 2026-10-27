/* sound/snd_listener_polar.h - leaf: the 3D positioning helpers `sound/fn_800E8E60.cpp` defines (docs/plan.md 6.5
 * rule 2), with the signatures `sound/sound_obj.cpp` calls them through. */
#ifndef MHTRI_SOUND_SND_LISTENER_POLAR_H
#define MHTRI_SOUND_SND_LISTENER_POLAR_H

#include "types.h"
#include "MTX/mtx.h"

#ifdef __cplusplus
struct SndRefTable;

extern "C" {
#endif

void snd_listener_polar(const Mtx listener, const Vec* pos, Vec* rel, f32* angle, f32* dist);
void snd_distance_atten(f32* max_dist, f32* dist, f32* atten);
s32 snd_pan_calc(Vec* rel, f32 dist, f32 angle, f32 pan_dist, f32 pan_angle, f32 atten);
s32 snd_span_calc(Vec* rel, f32 dist, f32 pan_dist, f32 atten);
s32 snd_distance_volume(s16 volume, f32 dist, f32 scale);
s32 snd_distance_priority(f32 dist, f32 scale);
u32 snd_ref_table_count(SndRefTable* table, s32 index);

#ifdef __cplusplus
}
#endif

#endif
