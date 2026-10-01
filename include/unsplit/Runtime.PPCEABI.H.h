/*
 * Runtime.PPCEABI.H / MSL band header (docs/plan.md 6.5 rule 2): the declarations of this band moved to their owners'
 * headers when the phase 4 split registered the units that own them - `__construct_array` in
 * `Runtime.PPCEABI.H/CPlusLibPPC.h`, `strlen` in `MSL/strlen.h` and the MSL C string / stdio / conversion helpers
 * (`strcmp`, `sprintf`, `strchr`, `atoi`, `strcpy`, `strncpy`, `strcat`, `printf`, `memmove`, `memcmp`, `snprintf`,
 * `wcsncpy`) in `MSL_C/alloc.h`.  The owner headers are included here so every consumer of this band header still sees
 * them; a unit that is next touched can include the owner's header directly.
 */
#ifndef MHTRI_UNSPLIT_RUNTIME_PPCEABI_H
#define MHTRI_UNSPLIT_RUNTIME_PPCEABI_H

#include "types.h"
#include "Runtime.PPCEABI.H/CPlusLibPPC.h"
#include "MSL/strlen.h"
#include "MSL_C/alloc.h"

#endif /* MHTRI_UNSPLIT_RUNTIME_PPCEABI_H */
