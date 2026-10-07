/* DB/__DBIsExceptionMarked.h - `__DBIsExceptionMarked`, which `DB/db.c` owns (docs/plan.md 6.5 rule 2, leaf header). */
#ifndef MHTRI_DB_DBISEXCEPTIONMARKED_H
#define MHTRI_DB_DBISEXCEPTIONMARKED_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Whether the debugger has marked an exception number. */
BOOL __DBIsExceptionMarked(u8 exception);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_DB_DBISEXCEPTIONMARKED_H */
