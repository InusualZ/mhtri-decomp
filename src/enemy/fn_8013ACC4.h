/* Declarations `enemy/fn_8013ACC4.cpp` owns: the stream reader `fn_8013BDE4` its neighbours call with a `u8**`
 * cursor, a command id and an `s16*` value slot (`fn_8013ACC4` and `fn_8013BDC8` have no cross-unit caller).
 */
#ifndef MHTRI_ENEMY_FN_8013ACC4_H
#define MHTRI_ENEMY_FN_8013ACC4_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8013BDE4 - consume one `id` record at `*in`, add its value to `*out` and advance the cursor by the
 * record's own length. */
void fn_8013BDE4(u8** in, u32 id, s16* out);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_FN_8013ACC4_H */
