/* The enemy user-data command interpreter unit `enemy/fn_8013ACC4.cpp` (0x8013ACC4..0x8013BE60): the
 * 256-entry program interpreter, its 8-byte record copier and the stream reader its neighbours call.
 *
 * Declarations moved here from the consumer units' `src/` files (docs/plan.md 6.5 rule 2: an extern
 * lives with the TU that owns the symbol).  The signature set is what the consumers used
 * (`enemy/fn_8013BE60.c` and `enemy/fn_8013F764.cpp` pass a `u8**` cursor, a command id and a `s16*`
 * value slot; `unsplit/enemy.h` carries the same signature today and can drop it once those
 * two units include this header).
 *
 * `fn_8013ACC4` itself is this unit's entry point and has no cross-unit caller in the recovered code,
 * and `fn_8013BDC8`'s record type is this unit's own view, so neither is declared here.
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
