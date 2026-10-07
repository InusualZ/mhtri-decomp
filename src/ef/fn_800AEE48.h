/* ef/fn_800AEE48.h - the `ef/ef_drawstripestrategy.cpp` symbols its own header does not carry (`fn_800B4B04`..
 * `ef_pm_last_alive`), with the two records the particle-list walkers take; it includes the post-field and resource
 * units' own headers. */
#ifndef MHTRI_EF_FN_800AEE48_H
#define MHTRI_EF_FN_800AEE48_H

#include "types.h"
#include "nw4r/math.h"
#include "ef.h"
#include "ef/ef_postfield.h" /* vec3_sub_assign (its owner's header) */
#include "ef/ef_resource.h"  /* the resource singleton's entry points (their owner's header) */

#ifdef __cplusplus
extern "C" {
#endif

f32 ef_float_epsilon(void);
int ef_vec3_normalize(void* self);

/* The particle record the list walkers yield.  +0x38/+0x3C are the two list heads. size: 0xB3 */
typedef struct EfParticleState {
    /* +0x00 */ u16 flags_0x00;
    /* +0x02 */ u8 pad_0x02[0x36];
    /* +0x38 */ u32 field_0x38;
    /* +0x3C */ u32 field_0x3C;
    /* +0x40 */ u8 pad_0x40[0x58];
    /* +0x98 */ Vec field_0x98;
    /* +0xA4 */ Vec field_0xA4;
    /* +0xB0 */ u8 field_0xB0;
    /* +0xB1 */ u8 pad_0xB1;
    /* +0xB2 */ u8 field_0xB2;
} EfParticleState; /* size: 0xB3 */

/* The draw strategy's two particle list heads. */
typedef struct EfDrawList {
    /* +0x00 */ u8 pad_0x00[0x38];
    /* +0x38 */ u32 field_0x38; /* an auxiliary head (the smooth-stripe sibling) */
    /* +0x3C */ u32 field_0x3C; /* the particle list head */
} EfDrawList; /* size: 0x40 */

/* Walks the particle list at +0x3C until the callback reports 1 (or the end). */
void* ef_pm_first_alive(EfDrawList* self);
/* Walks the particle list at +0x38. */
void* ef_pm_prev_alive(void* self, void* node);
/* Walks the auxiliary list through the per-node offset table until the callback reports 1. */
void* ef_pm_next_alive(void* self, void* node);
/* Walks the list starting at the particle record's +0x3C head. */
void* ef_pm_last_alive(EfParticleState* self);



#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_FN_800AEE48_H */
