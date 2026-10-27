/* The `sound`-band SE pool pointer (docs/plan.md 6.5 rule 2: a symbol no registered unit owns is declared in a band
 * header).  `se_work_init` (`sound/fn_800D7F54.cpp`) sets the pool up and `sound/se_req.cpp`'s point-SE layer reads it;
 * no registered range covers the `.sbss` word.  GUESS name.
 */
#ifndef MHTRI_UNSPLIT_SE_POOL_H
#define MHTRI_UNSPLIT_SE_POOL_H

#include "types.h"

struct SePoolView;

#ifdef __cplusplus
extern "C" {
#endif

/* .sbss 0x80794978 - the SE pool (size 0x2966C). */
extern struct SePoolView* se_pool;

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_UNSPLIT_SE_POOL_H */
