/*
 * RSO/rso_notify.c - the four RSO notify stubs `RSONotifyModuleLoaded`, `RSONotifyModuleUnloaded`,
 *    `RSONotifyPreRSOLink`, `RSONotifyPostRSOLink`.
 * RANGE. .text 0x804D9B3C-0x804D9B4C (4 functions).  Cut from the OS core band 0x804C1760-0x804D9B4C.  Evidence: four
 *    4-byte `blr` bodies at 0x804D9B3C..0x804D9B4C, ending where `RSO/runtime.c` starts.
 * FLAGS. `cflags_os` (configure.py), the flags of the unit it was cut from; no data sections on either side.
 * NAMES. map names; the file name is a GUESS.
 * RESIDUALS. none.
 * SHAPES. the parameter types are forward struct tags: `RSO/runtime.h` is not included, because `RSOModule` is defined
 *    twice across the RSO headers; the prototypes agree with the declarations in `RSO/runtime.c`.
 */

#include "types.h"

struct RSOHdr;
struct RSOModule;
struct RSORelTable;

/* 0x804D9B3C (0x4): Notifies a debugger that a module was loaded; an empty hook in retail. */
void RSONotifyModuleLoaded(struct RSOHdr* p) {}

/* 0x804D9B40 (0x4): Notifies a debugger that a module was unloaded; an empty hook in retail. */
void RSONotifyModuleUnloaded(void) {}

/* 0x804D9B44 (0x4): Notifies a debugger that a module is about to be linked; an empty hook in retail. */
void RSONotifyPreRSOLink(void) {}

/* 0x804D9B48 (0x4): Notifies a debugger that a module was linked; an empty hook in retail. */
void RSONotifyPostRSOLink(struct RSOModule* pObject, struct RSORelTable* pRel) {}
