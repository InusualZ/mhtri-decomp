/* ef/ef_emitter.h - the cross-unit declarations of `ef/ef_emitter.cpp` (C linkage), in the consumers' spellings; the
 *   owner does not include it and defines each with its own record types. */
#ifndef MHTRI_EF_EF_EMITTER_H
#define MHTRI_EF_EF_EMITTER_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800A7750 - the per-emitter creation entry the creation queue calls: the emitter form, the effect handle, the
 * setting record, the manager, a life and an optional VEC3 position. */
void fn_800A7750(void* form, void* eh, const void* setting, void* manager, u16 life, const void* pos);
/* 0x800A8998 - stores the word `v` through `dst` and hands `dst` back. */
void* ef_store_word(void* dst, s32 v); /* untyped: byte range - one word stored into the caller's slot */
/* 0x800A8A04 - the spawner's truncation (the angle -> byte rounding helper). */
f32 ef_truncate_float(f32 x);
/* 0x800A8A08 - steps the random block and returns it normalised to 0..1. */
f32 ef_random_float(u32* random);
/* 0x800A8C24 - the body of a length-prefixed resource block (past its header). */
void* ef_res_block_body(void* res);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EF_EMITTER_H */
