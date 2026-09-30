---
id: 46
title: A flipped unit's `.ctors$10` fragment is reordered by the linker
status: works
problem: A unit's object is byte-identical, `flipcheck.py` says READY, and the flip still breaks the DOL on the unit's `.ctors$10`/`.dtors$15` words, shifting the merged `.ctors`/`.dtors` tables. It reads as a linker/ordering mystery, and `flipcheck.py` cannot see it.
tags: [linker]
applies: []
demo:
reviewed: 2026-09-29
related: [36, 55, 59, 74]
---

# 46. A flipped unit's `.ctors$10` fragment is reordered by the linker

**Problem.** A unit's object is byte-identical, `flipcheck.py` says READY, and the flip still breaks the DOL on
the unit's `.ctors$10`/`.dtors$15` words, shifting the merged `.ctors`/`.dtors` tables. It reads as a
linker/ordering mystery, and `flipcheck.py` cannot see it.

**How it looks.** `flipcheck.py` prints READY, the unit is flipped to `Object(Matching, ...)`, the link succeeds
and `ninja diff` (`dtk dol diff`) then reports a data mismatch in the *linker's own* tables rather than in any
function - e.g. `_rom_copy_info` (the table describing the ranges the startup code copies) off by one word, the
entry that describes `.ctors` being one unit smaller in our link.

**Why it happens.** `.ctors`/`.dtors` (the static-constructor and destructor pointer tables) are not laid out by
link order. mwld's C++ support builds them from the input fragments in a fixed class order (`.ctors$00`,
`.ctors$10`, `.ctors`, `.ctors$99`; and the `.dtors` twins, `$00`/`$99` being the linker's own sentinels), and an
MWCC-emitted entry's **slot follows the entry's SYMBOL, not its section name** (the `__init_cpp_exceptions_reference`
family is recognised by name). A tool that compares section names or section sizes cannot see this.

**How to work it.**

1. Check freshness first: the blocker that motivated the original audit was a *stale target object, cured by a
   forced re-split*. Re-split, re-run, and only then suspect the linker.
2. Trace the unit through a real link: `python tools/mwlink_debugger.py trace <unit>` prints, per fragment, the
   section **and** the symbol, and a "Row 46" block with the fragment's fixed-order rank and its slot in the merged
   `.ctors`/`.dtors` (`order` prints the fixed class order: `python tools/mwlink_debugger.py order`). Compare the
   merged words, not only the object's section sizes.
3. Do not rename `.ctors$NN` sections or reorder link inputs hoping to move a word - both are measured no-ops for
   an MWCC-emitted entry (below). The lever, when there is one, is the source that emits the entry.

**Result.** Measured with `tools/mwlink_debugger.py` on the real link (2026-09-28), three same-length string
surgeries on the linker:

* renaming a **plain** (anonymous) `.ctors` to `.ctors$10` moves the word into the `$10` slot - for an anonymous
  fragment the section name does pick the class;
* renaming an MWCC-emitted `.ctors$10` to `.ctors`, `.ctors$55`, `.ctors$01`, `.ctors$99` or even `.dtors$10`
  leaves the output `.ctors` byte-identical - only the map's credit line changes;
* renaming the **symbols** (`__init_cpp_exceptions_reference`, `__fini_cpp_exceptions_reference`,
  `__destroy_global_chain_reference`) makes the link fail (catalogue id 205: "runtime sources ... need to be
  updated to latest version").

Ruled out by evidence: the `.comment` `CodeWarrior` block (zeroing its size changes nothing), the reloc section's
name, and link order for the `$NN` classes.

**When NOT to apply.** A byte-identical object whose flip breaks the DOL *elsewhere* is a different problem: a
missing extab/extabindex symbol name is idea 59, a trailing function trimmed by the linker is idea 36, an
odd-start section claim is idea 55. `ninja diff` names the differing symbol - read it before blaming `.ctors`.

**Demo.** None: the claim is about the linker's whole-link behaviour, and a single object cannot show it (the
class order lives in mwld, not in the compiler's output). Re-verify with `mwlink_debugger.py order` / `trace`.

**Evidence.** Four independent lane notes reached this row (kept in the campaign's local `.pi/notes/`, not durable
in git: the ctors rule investigation, the round-1 flip notes, the link-order 7.19 audit, the sysmem flip recheck).
`Runtime.PPCEABI.H/__init_cpp_exceptions` is `Object(Matching)` today and keeps its words at `.ctors[0]` and
`.dtors[1]`. The 2026-09-24 sharpening: `800CCCF8` (now `ef/ef_emform`) first failed for an unrelated declaration
bug (idea 50); with that fixed the link succeeded and the single reported difference was

```
ERROR Data mismatch for _rom_copy_info (type Object, size 0x84) at 0x80006624
ERROR Original: ...8056F2C0 8056F2C0 00000017...
ERROR Linked:   ...8056F2C0 8056F2C0 00000016...
```

i.e. every symbol matched and only the linker's own count for `.ctors` was one word short. Re-checked 2026-09-29:
`flipcheck.py ef/ef_emform` still says READY (`.ctors` 0x4, extab 0x48, extabindex 0x6C) while the unit stays
`NonMatching`, and `mwlink_debugger.py trace ef/ef_emform` shows its `.ctors` word at "fixed-order rank 2" in the
merged table - the tool now gives the starting point instead of a guess. The `_rom_copy_info`/`_eti_init_info`
tables themselves are linker-generated and cannot be claimed (dtk strips them from splits on purpose).
