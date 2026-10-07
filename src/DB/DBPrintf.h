/* DB/DBPrintf.h - `DBPrintf`, which `DB/db.c` owns (docs/plan.md 6.5 rule 2, leaf header). */
#ifndef MHTRI_DB_DBPRINTF_H
#define MHTRI_DB_DBPRINTF_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804A4B70 - prints a formatted message to the debugger channel. */
void DBPrintf(const char* format, ...);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_DB_DBPRINTF_H */
