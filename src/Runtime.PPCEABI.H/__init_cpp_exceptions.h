/*
 * Runtime.PPCEABI.H/__init_cpp_exceptions.h - the linker labels at the heads of the constructor and destructor
 * tables, whose first words are the `.ctors$10`/`.dtors$10` rows `Runtime.PPCEABI.H/__init_cpp_exceptions.cpp`
 * owns (`_ctors` = `.ctors:0x8056F2C0`, `_dtors` = `.dtors:0x8056F440`, both `type:label` in the map).
 */
#ifndef RUNTIME_INIT_CPP_EXCEPTIONS_H
#define RUNTIME_INIT_CPP_EXCEPTIONS_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* A static constructor or destructor entry: the SDK's `__ppc_eabi_linker.h` spelling. */
typedef void (*voidfunctionptr)(void);

/* The null-terminated static constructor table `__init_cpp` runs at startup. */
extern voidfunctionptr _ctors[];
/* The null-terminated static destructor table `exit` runs at shutdown. */
extern voidfunctionptr _dtors[];

#ifdef __cplusplus
}
#endif

#endif
