# Runtime band recut: MSL C, libm, MetroTRK, AI

The old `MSL_C/alloc.cpp` (0x804578FC..0x804642C8, 128 functions) and `TRK/TRK_flush_cache.cpp` (0x80467EEC..0x8046D9F0,
166 functions) were merged guess blocks. They are cut into per-file units; every unit keeps the lib block and cflags
group of the unit it came from (`Runtime.PPCEABI.H` / `cflags_ppceabi` for the first, `OS` / `cflags_os` for the
second). Nothing is matched yet: every unit is a stub whose header carries RANGE/FLAGS/NAMES/EVIDENCE/RESIDUALS.

## Evidence classes

* **A** - a data object, pool, string or table that only the unit reads (or that sits in the section order between the
  neighbours' objects), a `.rodata` `__FILE__`-style string, an extab record, or a pooled value held again at another
  pool address (one TU pools a value once).
* **B** - a dump-name family plus call-graph closure (callees only inside the unit).
* **C (not cut)** - nothing but kind; merged into a coarse unit and marked COARSE in the header.

## Facts the cut rests on

* The first half of the old alloc span is not Gecko: the Gecko unit's five functions already match with their own extab
  and the jump table 0x8060E8A0; 0x804578FC.. is a second runtime file (`Runtime.PPCEABI.H/ExceptionPPC.cp`, GUESS name)
  holding all five extab records of the old span.
* `.rodata` 0x805724B8 reads `GCN_Mem_Alloc.c : InitDefaultHeap...`: the allocator hook is its own file.
* `__files` (0x8060E958) points at three 0x100 buffers (`.bss` 0x806F4D00..0x806F5000) and at the console callbacks of
  `MSL_C/uart_console_io.c` and `TRK/mslsupp.c`.
* Pool dedupe: `0.0` is pooled at 0x8079C9A8, 0x8079C9F8, 0x8079CA10, 0x8079CA28, 0x8079CF28 and 0x8079CF30, so the
  number conversion, printf, string-to-float, wide printf, `s_sin` and `s_tan` are separate TUs.
* Data-only units (no `.text`): `MSL_C/errno.c` (`.sbss` 0x80794E08), `MSL_C/locale.c` (tables and records no code of the
  band reads by name), `MSL_C/float.c` (`.sdata` 0x80793CF0..D00), `MSL_C/wctype.c` (`.rodata` 0x80572B88..0x80573188).
* `TRK/targimpl.c` is one unit on purpose: `gTRKState` and `gTRKCPUState` are read on both sides of every internal
  candidate boundary of the back half, so `.bss` order forbids a cut inside it.
* The AI library is the 16-aligned run 0x8046D420..0x8046D9F0 inside the 4-packed TRK band; its directory is `AI_SDK`
  because `src/ai/` is the game's monster AI and `AI` collides on a case-insensitive checkout.

## The 22 MSL math units

Verified: each holds exactly one function and no value twice in its pool. 16 of the 22 adjacent pairs share a pooled
value (two pool addresses = two TUs); the unproven pairs have no pool on one side (`e_fmod`, `s_copysign`, `s_modf`) or
share nothing (`k_tan`, `s_frexp`, `s_cos`, `s_sin`, `e_log`). A merge needs same-TU evidence and none exists, so they
stay as registered. `MSL/e_acos.cpp` was missing and sat at the end of the alloc span.

## Left as it was

`Runtime.PPCEABI.H/Gecko_ExceptionPPC.cp` (its header's statement that no `__throw` exists in this DOL is stale: the map
names `__throw` at 0x80458A60), `MSL_C/alloc.h`, `MSL_C/strstr.h`, `TRK/TRK_flush_cache.h` (declarations of symbols now
owned by other units of the band; the gate counts them as pre-existing rule-2 findings; the lanes that write the bodies
move them into per-owner headers).
