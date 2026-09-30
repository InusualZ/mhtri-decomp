---
id: 46
title: A flipped unit's `.ctors$10` fragment is reordered by the linker
status: works
problem: A unit's object is byte-identical, `flipcheck.py` says READY, and the flip still breaks the DOL on the unit's `.ctors$10`/`.dtors$15` words, shifting the merged `.ctors`/`.dtors` tables. It reads as a linker/ordering mystery, and `flipcheck.py` cannot see it.
tags: [linker]
applies: []
demo:
---

# 46. A flipped unit's `.ctors$10` fragment is reordered by the linker

**Problem.** A unit's object is byte-identical, `flipcheck.py` says READY, and the flip still breaks the DOL on the unit's `.ctors$10`/`.dtors$15` words, shifting the merged `.ctors`/`.dtors` tables. It reads as a linker/ordering mystery, and `flipcheck.py` cannot see it.

**Why try it.** Check the target object's freshness first: the blocker that motivated the 7.19 audit was a *stale target object, cured by a re-split*. If a fresh re-split does not cure it, the linker's built-in `.ctors`/`.dtors` path is the remaining suspect: it collects `$NN` fragments in a fixed name order (`.ctors$00, .ctors$10, .ctors, .ctors$99`), not link order, so compare the merged `.ctors`/`.dtors` words, not only the object's section sizes.

**Result.** Measured independently by 4 worker(s):

* `ctors-rule.md` - # The `.ctors`/`.dtors` flip blocker (roadmap 7.19) - read-only investigation Read-only pass (no ninja/configure/link, no repo file touched except this note). Goal: decide why flipping
* `flip-round-1.md` - ## The open one: a flipped object's .ctors/.dtors fragments do not land where the original's did `Runtime.PPCEABI.H/__init_cpp_exceptions` passes all three checks and still fails:
* `linkorder-7.19.md` - * The `.ctors`/`_reference` blocker that motivated 7.19 was a **stale target object cured by a re-split**: `__init_cpp_exceptions` is `Object(Matching, ...)` today and the green link keeps all three of its words (`__init_cpp_exceptions_reference` at `.ctors[0]`, `__fini_cpp_exceptions_reference` at `.dtors[1]`).
* `sysmem-flip-recheck.md` - Same shape as the `.ctors` blocker: the measurement predates the forced re-split, and the re-split cured it. ## 1. Object equivalence (current objects, all regenerated at 12:56)

**Sharpened 2026-09-24 - the blocker is `_rom_copy_info`, and it is off by one word.** `800CCCF8` (now
`ef/ef_emform`) had this recorded as "the `.ctors` class", and the real first cause was a *different* bug
(row 50) that hid it: the link failed outright with an undefined `Panic__Q24nw4r2dbFPCciPCce__FPCciPCce`.
With that fixed the link **succeeds**, and `ninja diff` then reports exactly one thing:

```
ERROR Data mismatch for _rom_copy_info (type Object, size 0x84) at 0x80006624
ERROR Original: ...8056F2C0 8056F2C0 00000017...
ERROR Linked:   ...8056F2C0 8056F2C0 00000016...
```

That is worth more than "suspect the ordering": **every symbol matches** - the only difference anywhere in
the linked image is `_rom_copy_info`, the *linker's own* table describing the `.ctors` range it copies at
startup, and the entry that describes `.ctors` (`from == to == 0x8056F2C0`, i.e. the whole table) is one unit
smaller in our link. So the merged `.ctors` **content** is right and the count is wrong. The next step is
concrete: read `_rom_copy_info` entry by entry from the original DOL and from our link and find the entry
whose size differs, then work out which `.ctors$NN` input fragment the linker did not see. Useful sizes on
this tree: the linked ELF's `.ctors` is 0x16C at 0x8056F2C0, `.dtors` is 0xC; six `.ctors` words are claimed
by registered units (`ef/ef_emform` 0x8056F2E8-0x8056F2EC, `sound/fn_800E46E8` 0x8056F2F4-0x8056F300,
`ef/fn_80114E34` 0x8056F310-0x8056F314, `Runtime.PPCEABI.H/__init_cpp_exceptions` 0x8056F2C0-0x8056F2C4).

**Refined (2026-09-28) - the slot follows the entry SYMBOL, not the section name.** `tools/mwlink_debugger.py`
can now trace one object through a real link (`trace`, with the build's own link line via `trace --link`,
verified byte-identical to `main.elf`) and derive the linker's phase table from its PE resource catalogue
(`phases`; the anchor is the `call [LoadStringA]` whose `uID` argument is not a constant, RVA 0x3d0b0, 1248 of
them). Three same-length string-surgery experiments on the real link settled this row:

* renaming a **plain** `.ctors` to `.ctors$10` **moves the word** into the `$10` slot - for an *anonymous*
  fragment the section name really does pick the class;
* renaming an MWCC-emitted `.ctors$10` to `.ctors`, `.ctors$55`, `.ctors$01`, `.ctors$99`, or even `.dtors$10`
  leaves the output `.ctors` **byte-identical** - the word stays in the `$10` slot and only the map's credit
  line changes;
* renaming the **symbols** (`__init_cpp_exceptions_reference`, `__fini_cpp_exceptions_reference`,
  `__destroy_global_chain_reference`) makes the link **fail**, with catalogue id 205: *"runtime sources
  'global_destructor_chain.c' and '__init_cpp_exceptions.cpp' both need to be updated to latest version."*

So the fixed class *order* is real (`.ctors$00`, `.ctors$10`, `.ctors`, `.ctors$99`, the `$00`/`$99` ends being
the linker's own sentinels) but an MWCC entry's **slot is chosen from its symbol, not its section**. Ruled
out by evidence: the `.comment` `CodeWarrior` block (zeroing its size changes nothing), the reloc section's
name, and link order for the `$NN` classes. **A section-name comparison cannot see any of it** - which is the
trap for a flip-check tool that compares sections; `trace` prints the section *and* the symbol per fragment.