/*
 * ef/ef_sphere.cpp - the sphere emitter form's spawn function: `.text` 0x800CDB2C..0x800CE5A8 (`ef_sphere_spawn`, 0xA78 B, and its
 * tail thunk `fn_800CE5A4`), extab 0x8000A5DC..0x8000A5E4, extabindex 0x80023C70..0x80023C7C, `.data` 0x80595058..0x80595108 (its
 * `__FILE__` string "ef_sphere.cpp" and assert strings).  Registered 2026-09-30, carved out of `ef/fn_800CDB2C.cpp`.
 *
 * Seams: the left edge is `ef/ef_point.cpp`'s end.  The right edge is 0x800CE5A8: `fn_800CE5A4` is called only from
 * `ef_sphere_spawn`, while `fn_800CE5A8` (the 0x84D0 `system_w` side-table size) is called only from the game-system TU that
 * starts there (`.pi/notes/ef-nwres-seam.md`).  Name from the `__FILE__` string (class 1).
 *
 * Unwritten: `ef_sphere_spawn` (the emitter builder: reads the `.sdata2` pool 0x80796328..0x80796358 and asserts on its `em`/`pm`/`params`
 * pointers) and the thunk; `.data` is claimed and not emitted yet.
 *
 * Inventory and evidence: `python tools/units/ledger.py unit ef/ef_sphere.cpp`.
 */

#include "types.h"
