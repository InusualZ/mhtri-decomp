/* Runtime.PPCEABI.H/__va_arg.h - the declaration of `__va_arg`, which `Runtime.PPCEABI.H/__va_arg.cpp` owns: the PPC EABI
 * fetch of the next variadic argument of class `type` (1: a general-purpose word); the address of its slot. */
#ifndef MHTRI_RUNTIME_VA_ARG_H
#define MHTRI_RUNTIME_VA_ARG_H

#include "types.h"
#include "stdarg.h"

#ifdef __cplusplus
extern "C" {
#endif
u32* __va_arg(struct __va_list_struct* ap, s32 type);
#ifdef __cplusplus
}
#endif

#endif
