/* OS/OSSwitchFiberEx.h - the declaration of `OSSwitchFiberEx`, which `OS/OSContext.c` owns (docs/plan.md 6.5 rule 2,
 *   leaf header). */
#ifndef MHTRI_OS_OSSWITCHFIBEREX_H
#define MHTRI_OS_OSSWITCHFIBEREX_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804CD1C0 - runs `entry` on the stack whose top is `stack`, without saving the caller's context. */
void OSSwitchFiberEx(u32 arg0, u32 arg1, u32 arg2, u32 arg3, void (*entry)(void), u8* stack);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_OS_OSSWITCHFIBEREX_H */
