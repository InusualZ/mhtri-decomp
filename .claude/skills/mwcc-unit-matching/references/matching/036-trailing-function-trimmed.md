---
id: 36
title: A flipped unit's unreferenced trailing function is trimmed by the linker
status: works
problem: The unit's object is byte-identical, `flipcheck.py` is happy and `linkorder.py` says `LINK OK` - and the flip still breaks the DOL, by exactly the size of the object's *last* function, with every later section shifted.
tags: [linker]
applies: [Wii/1.0]
demo: 036-trailing-function-trimmed.cpp
reviewed: 2026-09-29
related: [46, 55, 59]
---

# 36. A flipped unit's unreferenced trailing function is trimmed by the linker

**Problem.** The unit's object is byte-identical, `flipcheck.py` is happy and `linkorder.py` says `LINK OK` - and
the flip (`Object(NonMatching, ...)` to `Object(Matching, ...)`, so the object is really linked) still breaks the
DOL, by exactly the size of the object's *last* function, with every later section shifted. It reads as an
unreachable linker mystery, because nothing about the code differs.

**How it looks.** `ninja build/RMHE08/ok` fails; `ninja diff` reports the first differing symbol *after* the unit
(e.g. `_eti_init_info` at 0x8003F17C where the target has it at 0x8003F1C8) and the offset equals a whole
function's size - the one at the end of the unit that nothing inside the object calls.

**Why it happens.** `dol split` writes the target objects with `export_all: true`, which stamps `active_flags=0x08`
on every entry of the `.comment` symbol table (`docs/comment_section.md`: `08` = force active / export, "prevents
the symbol from being deadstripped"); MWCC-compiled objects write `0x00`. The linker honours that flag, so our
object's *trailing* function - the one nothing inside the object references - is dropped, along with its `extab`
record (exception-unwind table) and `extabindex` entry. The target object keeps everything because every symbol
is flagged.

**How to work it.** Mark the function `__declspec(export)` (or `#pragma force_active on`), which sets the flag. The
code is unchanged and the link reproduces the original layout.

```c
extern "C" __declspec(export) void fn_8004054C(void* ptr) throw();   /* the object's last function */
```

Only a *trailing* unreferenced function needs it, and only in a unit that flips. The generalised check belongs in
`flipcheck.py`: compare the `.comment` per-symbol `active_flags` (offset 0x2C + 8*index, byte 5) between
`obj/<unit>.o` and `src/<unit>.o` - a mismatch means the link may trim.

**When NOT to apply.** A function that *is* referenced from inside the object survives without the flag, and a
unit that is not linked (`NonMatching`) cannot show the effect at all. `flipcheck.py`'s census of flag mismatches
is a **superset** of what the linker really trims (see below): a `trim risk` row is a candidate to check, not a
verdict, so confirm with the DOL hash before exporting every candidate.

**Result.** Measured on `sys_mem.cpp`: marking `fn_8004054C` `__declspec(export)` left `.text` unchanged and the
link reproduced the original DOL byte for byte (flip 12). Proven three ways at the time: adding the symbol to the
linker script's `FORCEACTIVE` restores the target layout, and swapping the two objects' `.comment` sections in each
direction swaps the behaviour. Ruled out by measurement: the `.comment` version byte alone, `SHF_INFO_LINK` on
`.rela.*`, section order, `.note.split`, local symbol names, and all 21 Wii/GC compilers. The current
`src/sys_mem.cpp` header still records this.

**Demonstration.** `036-trailing-function-trimmed.cpp` (`ideas.py demo-check 36`) shows the compiler half in one
object: `__declspec(export)` writes `0x08` at byte 5 of that symbol's `.comment` entry and a plain function writes
`0x00`; the trimming itself is a link-time effect one object cannot show.

**Evidence (refined 2026-09-28).** `tools/mwlink_debugger.py trace` and `phases` proved the mechanism with four
controlled relinks, each replacing `build/RMHE08/obj/main.o` in the response file with a copy (all four redirect
into `build/scratch/`, so `main.elf` is untouched):

| copy of `main.o` | `.comment` magic | the 8 risk symbols' flag byte | `.text` | risk symbols still in the map |
| --- | --- | --- | --- | --- |
| `keep` (control) | valid | 0x08 on all 8 | `0x1278` | all 8 |
| `single` | valid | 0x08 except `fn_8003F200` = 0x00 | `0x126c` | 7 (`fn_8003F200`, 0xc bytes, gone) |
| `clear` | valid | 0x00 on all 8 | `0x1238` | 2 (`fn_8003FC64`, `fn_80040360`) |
| `badmagic` | broken | 0x00 on all 8 | `0x1278` | all 8 |

The `single` run drops **exactly one** symbol and its `.text` shrinks by exactly that symbol's size - so the flag
is **per-symbol**, not per object; and `badmagic` clears the same bytes but breaks the `CodeWarrior` magic and
drops **nothing**, so the decision is driven by the parsed `.comment`, not by the ELF symbol table. The flag is
**byte 5 of each 8-byte entry** (`0x2c + 8*index + 5`) and the bit is **0x08**. Two symbols (`fn_8003FC64`,
`fn_80040360`) survive even with it clear, so the census `flipcheck.py` reports is a **superset**: it names 8
candidates and the link really trims 6. **Which phase** does the strip is still not derived and is a filed gap: no
message is printed for that step, so no phase anchor can name it.
