/* OS/OSDefaultExceptionHandler.h - the declaration of `OSDefaultExceptionHandler`, which `OS/OS.c` owns (docs/plan.md 6.5
 *   rule 2, leaf header). */
#ifndef MHTRI_OS_OSDEFAULTEXCEPTIONHANDLER_H
#define MHTRI_OS_OSDEFAULTEXCEPTIONHANDLER_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct OSContext;

/* 0x804CB2B0 - the exception handler a processor exception runs when no other is installed. */
void OSDefaultExceptionHandler(u8 exception, struct OSContext* context);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_OS_OSDEFAULTEXCEPTIONHANDLER_H */
