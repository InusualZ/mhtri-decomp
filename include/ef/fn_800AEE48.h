#ifndef MHTRI_EF_FN_800AEE48_H
#define MHTRI_EF_FN_800AEE48_H

#include "types.h"
#include "nw4r/math.h"
#include "ef.h"

/* Declarations for the symbols `src/ef/fn_800AEE48.cpp` owns (docs/plan.md 6.5, rule 2).  C-visible;
 * kept minimal - only the declarations a consumer needs.
 *
 * The signatures are the owner's own definitions - `f32 fn_800B5A48(void)`,
 * `int fn_800B59E4(void* self)`, `void* fn_800B4B04(void* self, s16 flag)` - so including this header
 * from the owner cannot conflict.  `fn_800B7DB0` is not reconstructed in the owner unit yet, so it
 * keeps the `void*`/`MTX34*` view the consumers (and `unsplit/ef.h`) already share.
 *
 * The two records the particle-list walkers take are defined here (moved out of the owner's source)
 * so the walkers can be declared with the owner's own types: a generic `void(void*)` copy in
 * `unsplit/ef.h` would be an illegal overload of the typed definition in C++.
 */
#ifdef __cplusplus
extern "C" {
#endif

f32 fn_800B5A48(void);
int fn_800B59E4(void* self);
void* fn_800B4B04(void* self, s16 flag);
void fn_800B7DB0(void* em, MTX34* out);

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
void* fn_800B5A64(EfDrawList* self);
/* Walks the particle list at +0x38. */
void* fn_800B8D48(void* self, void* node);
/* Walks the auxiliary list through the per-node offset table until the callback reports 1. */
void* fn_800B5ACC(void* self, void* node);
/* Walks the list starting at the particle record's +0x3C head. */
void* fn_800B95C0(EfParticleState* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_FN_800AEE48_H */
