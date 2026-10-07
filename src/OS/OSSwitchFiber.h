/* OS/OSSwitchFiber.h - the declaration of `OSSwitchFiber`, which `OS/OSContext.c` owns (docs/plan.md 6.5 rule 2,
 *   leaf header). */
#ifndef MHTRI_OS_OSSWITCHFIBER_H
#define MHTRI_OS_OSSWITCHFIBER_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804CD190 - runs `func` on the stack whose top is `stack`, without saving the caller's context. */
void OSSwitchFiber(void (*func)(void), u8* stack);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_OS_OSSWITCHFIBER_H */
