---
id: 36
title: A flipped unit's unreferenced trailing function is trimmed by the linker
status: works
problem: The unit's object is byte-identical, `flipcheck.py` is happy and `linkorder.py` says `LINK OK` - and the flip still breaks the DOL, by exactly the size of the object's *last* function, with every later section shifted. It reads as an unreachable linker mystery, because nothing about the code differs.
tags: [linker, sections]
applies: []
demo:
---

# 36. A flipped unit's unreferenced trailing function is trimmed by the linker

**Problem.** The unit's object is byte-identical, `flipcheck.py` is happy and `linkorder.py` says `LINK OK` - and the
flip still breaks the DOL, by exactly the size of the object's *last* function, with every later section shifted. It
reads as an unreachable linker mystery, because nothing about the code differs.

**Why it happens.** `dol split` writes the target objects with `export_all: true`, which stamps `active_flags=0x08`
on every entry of the `.comment` symbol table; MWCC-compiled objects write `0x00`. The linker honours that flag, so
our object's *trailing* function - the one nothing inside the object references - is dropped, along with its `extab`
record and `extabindex` entry. Measured on `sys_mem.cpp`: `ninja diff` reported `_eti_init_info` at 0x8003F17C where
the target has it at 0x8003F1C8, and the 0x4C difference is exactly `fn_8004054C`'s size.

**Result.** Marking the function `__declspec(export)` sets the flag. `.text` is unchanged and the link reproduces the
original DOL byte for byte - `sys_mem.cpp`, flip 12. Proven three ways: adding the symbol to the script's
`FORCEACTIVE` restores the target layout, and swapping the two objects' `.comment` sections in each direction swaps
the behaviour. Ruled out by measurement: the `.comment` version byte alone, `SHF_INFO_LINK` on `.rela.*`, section
order, `.note.split`, local symbol names, and all 21 Wii/GC compilers.

**Example.** Only a *trailing* unreferenced function needs it, and only in a unit that flips. The generalised check
belongs in `flipcheck.py`: compare the `.comment` per-symbol `active_flags` (offset 0x2C + 8*index, byte 5) between
`obj/<unit>.o` and `src/<unit>.o` - a mismatch means the link will trim.

**Refined (2026-09-28) - the flag is settled, and `flipcheck.py`'s census is a SUPERSET.**
`tools/mwlink_debugger.py` proved the mechanism with **four controlled relinks**, each replacing
`build/RMHE08/obj/main.o` in the response file with a copy (all four redirect into `build/scratch/`, so
`main.elf` is untouched):

| copy of `main.o` | `.comment` magic | the 8 risk symbols' flag byte | `.text` | risk symbols still in the map |
| --- | --- | --- | --- | --- |
| `keep` (control) | valid | 0x08 on all 8 | `0x1278` | all 8 |
| `single` | valid | 0x08 except `fn_8003F200` = 0x00 | `0x126c` | 7 (`fn_8003F200`, 0xc bytes, gone) |
| `clear` | valid | 0x00 on all 8 | `0x1238` | 2 (`fn_8003FC64`, `fn_80040360`) |
| `badmagic` | broken | 0x00 on all 8 | `0x1278` | all 8 |

The `single` run drops **exactly one** symbol and its `.text` shrinks by exactly that symbol's size - so the
flag is **per-symbol**, not per object; and `badmagic` clears the same bytes but breaks the `CodeWarrior`
magic and drops **nothing**, so the decision is driven by the parsed `.comment`, not by the ELF symbol table.
The flag is **byte 5 of each 8-byte entry** (`0x2c + 8*index + 5`) and the bit is **0x08** (force
active/export, the same bit `docs/comment_section.md` documents). Two symbols (`fn_8003FC64`,
`fn_80040360`) survive even with it clear, so the census `flipcheck.py` reports is a **superset**: it names 8
candidates and the link really trims 6 - a `trim risk` row is a candidate to check, not a verdict. **Which
phase** does the strip is still not derived and is a filed gap: no message is printed for that step, so no
phase anchor can name it, and a read watchpoint on the flag byte lands on the `rep movsd` that copies
`.comment` into a heap buffer whose address moves between runs.
