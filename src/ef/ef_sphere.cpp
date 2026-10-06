/*
 * ef/ef_sphere.cpp - the sphere emitter form's spawn function `ef_sphere_spawn` and its tail thunk.
 * RANGE. .text 0x800CDB2C-0x800CE5A8 (2 functions); extab 0x8000A5DC-0x8000A5E4, extabindex 0x80023C70-0x80023C7C,
 *   .data 0x80595058-0x80595118 (the `__FILE__` string "ef_sphere.cpp" and the assert strings), .sdata2
 *   0x80796328-0x80796360.  Left edge: `ef/ef_point.cpp` ends there.  Right edge: `fn_800CE5A4` is called only
 *   from `ef_sphere_spawn`, while `fn_800CE5A8` (the 0x84D0 `system_w` side-table size) is called only from
 *   `ef/system_core.cpp`, which starts there.
 * FLAGS. `cflags_main`.
 * NAMES. The file name is the `__FILE__` string's.
 * RESIDUALS. 2 rows unwritten: 0x800CDB2C-0x800CE5A8 (`ef_sphere_spawn`, which asserts on its `em`/`pm`/`params`
 *   pointers, and the thunk); the object emits no section, so flipcheck finds every claimed section missing.
 */

#include "types.h"
