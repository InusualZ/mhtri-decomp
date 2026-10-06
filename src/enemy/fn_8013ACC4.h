/* Declarations `enemy/fn_8013ACC4.cpp` owns: the stream reader `fn_8013BDE4` its neighbours call with a `u8**`
 * cursor, a command id and an `s16*` value slot (`fn_8013BDC8` has no cross-unit caller).
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

/* 0x8013ACC4 - runs the enemy's user-data command program (caller: enemy/fn_80138074.cpp); `EmWork` is the owner's
 * view of the enemy work block. */
struct EmWork;
u32 em_userdata_script_run(struct EmWork* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_FN_8013ACC4_H */
