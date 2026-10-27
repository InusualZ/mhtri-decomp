/* ef/eft051.h - `ef/eft051.cpp`'s header: the eft051 lobby decoration family and the record its work block holds. */
#ifndef MHTRI_EF_EFT051_H
#define MHTRI_EF_EFT051_H

#include "types.h"
#include "ef.h"
#include "pl.h"

/* The family `eft051_dispatch` drives (`eft051_spawn` pools 0x0C bytes): the pooled models, the spot and kind the source
 * picked and its countdown.  size: 0x0C */
typedef struct Eft051Work {
    /* +0x00 */ s32 count_0x00;         /* the pooled handle list's used length */
    /* +0x04 */ MHchar* models_0x04[1]; /* the pooled handle list `eft051_release` releases */
    /* +0x08 */ u8 spot_0x08;           /* the spot record of the map's table */
    /* +0x09 */ u8 kind_0x09;           /* the joint the model shows (kind + 1) */
    /* +0x0A */ s16 timer_0x0A;         /* the countdown `eft051_alive` runs to 0xB4 */
} Eft051Work;

/* One decoration spot of the eft051 tables: the position and the yaw. size: 0x10 */
typedef struct EftSpot {
    /* +0x00 */ Vec pos;
    /* +0x0C */ u16 yaw;
    /* +0x0E */ u16 pad_0x0E;
} EftSpot;

#ifdef __cplusplus
extern "C" {
#endif

void eft051_spawn(_PLW* src, u8 kind);
void eft051_release(_EFT* self);
void eft051_dispatch(_EFT* self);
void eft051_create(_EFT* self);
void eft051_alive(_EFT* self);
void eft051_state_next(_EFT* self);
void eft051_retire(_EFT* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EFT051_H */
