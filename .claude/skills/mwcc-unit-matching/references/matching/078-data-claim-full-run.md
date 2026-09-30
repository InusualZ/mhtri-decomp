---
id: 78
title: A data claim must cover the run the unit actually TOUCHES, not the extent the symbol happens to name
status: works
problem: `NHTTP/d_nhttp`'s `.sbss` claim covered **4 B** (`0x80795880`, the list head) while the unit's own rows relocate three words of a **16-byte run** at `0x80795878-0x80795888`: two lazy-init flags (`li r0,1` / `stw`, inside `NHTTPi_RegisterCallbacks` and `fn_8051A8A4`) and the list head, with `NHTTPi_systemInfoP` as the fourth word - and `powerpc-eabi-objdump -t` shows all four as **one object** in `auto_10_80795878_sbss.o`. So the 4 B claim left rows this unit owns still unowned, the blocked total was 796 B rather than the 524 B recorded, and the same pass separately recorded `.bss 0x80762C20` at its *used* extent (0x24) where the map, the split and the target symbol all say 0x40.
tags: [data, sections]
applies: [NHTTP]
demo:
---

# 78. A data claim must cover the run the unit actually TOUCHES, not the extent the symbol happens to name

**Problem.** `NHTTP/d_nhttp`'s `.sbss` claim covered **4 B** (`0x80795880`, the list head) while the unit's own rows
relocate three words of a **16-byte run** at `0x80795878-0x80795888`: two lazy-init flags (`li r0,1` / `stw`,
inside `NHTTPi_RegisterCallbacks` and `fn_8051A8A4`) and the list head, with `NHTTPi_systemInfoP` as the fourth
word - and `powerpc-eabi-objdump -t` shows all four as **one object** in `auto_10_80795878_sbss.o`. So the 4 B claim
left rows this unit owns still unowned, the blocked total was 796 B rather than the 524 B recorded, and the same
pass separately recorded `.bss 0x80762C20` at its *used* extent (0x24) where the map, the split and the target
symbol all say 0x40.

**Why try it.** A claim is a statement about **what our object emits**, so its boundaries must match the object that
owns the bytes - the map row and the target symbol - not the part of them this unit currently reads. The *used*
extent is an observation about our progress; the *object* extent is the fact. Getting it wrong has a specific cost
that is easy to underestimate: rule 12 refuses an `extern` for the range from anywhere (band header included), so
every row that touches the unowned words stays blocked, and the next pass re-derives the same claim from scratch.

**Result.** The amendment to the 16-byte run (authorised under the claim-amendment protocol, with the `splits.txt`
lines in the file's own format and `NHTTPi_systemInfoP` moved out of the band header into the unit, per rule 2's
inversion) makes writable the two rows that only need `.sbss` - `NHTTPi_RegisterCallbacks` (140 B) and
`fn_8051A8A4` (132 B) - and, with `.bss 0x80762C20` at its symbol's own 0x40, the three list rows (272 + 32 + 96).
Two habits follow. **Take claims in increments**: the `.sdata`/`.data` half of that run is a separate measured step
because a `.data` claim can drop the target's `R_PPC_NONE` pool relocations (row 23). And when a claim is amended,
**move the map rows and band declarations inside it in the same change** - rule 7's names and rule 2's inversion are
part of the claim, not a follow-up.
