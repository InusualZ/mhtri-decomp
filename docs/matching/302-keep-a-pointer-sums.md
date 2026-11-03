---
id: 302
title: Keep a pointer sum's constant in place with += steps: the frontend folds one expression's constant to the chain's end
status: works
problem: Retail adds a struct's size in the middle of a byte-offset sum (`add r4,r3,r0; addi r0,r4,0x20; lwz r4,0x10(r3); add ...`) where ours has the `addi` after the last field, with the loads reading the other register
tags: [source-shape, allocator]
applies: [Wii/1.3]
demo: 
related: [289, 285]
---

# 302. Keep a pointer sum's constant in place with += steps: the frontend folds one expression's constant to the chain's end

**Problem.** A byte-offset sum over several fields and a `sizeof` (`(u8*)track + track->keyBytes + sizeof(Track) +
track->randomBytes + track->randomTableBytes`): retail emits `lwz r0,0xc(r3); add r4,r3,r0; addi r0,r4,0x20; lwz
r4,0x10(r3); add r0,r0,r4; lwz r4,0x14(r3); add r17,r0,r4` - the constant added second, where it is written - while
ours emits the three loads and adds first and `addi r17,r4,0x20` last, two instructions out of place and the
registers of the chain swapped.

**Why it happens.** The first PCode dump (`tools/mwcc-debugger`, `after-initpcode`/`after-unreachable-block-deletion`)
already carries the folded chain, so the fold is the frontend's: in one expression it moves the constant addend of a
pointer sum to the end of the chain, through a typed `+ 1`, an integer round trip (`(u8*)((u32)p + K)`), `(s32)`/`(u32)`
casts of the operands, parentheses, and the copy propagation of a named intermediate or a redefinition of the same
variable (`names = ...; names = names + a + b;`). Only an in-place update is not an expression it reassociates: `names
+= a;` keeps the constant where the previous statement left it. A second effect of the same fold: when the constant
is written *last* the chain's loads come out one ahead with load-first adds (`add r204,r201,r202`), where written
second they are sequential (`add r247,r245,r246`) - retail's shape for a sum that ends in the constant.

**How to work it.** Write the constant where retail adds it, then add each later field with its own `+=`; and write a
sum whose constant retail adds last with the constant *second* (the fold puts it last and the chain stays
sequential). The `+=` form makes the partial sums one web (the variable's register), which is right only when retail's
intermediates are not temporaries; measure the register operands after the shape lands.

**Result.** `ef/ef_particlemanager` `Calc` (Wii/1.3, -O3, peephole off): the `memcpy` source sum with `sizeof` second
98.45 -> 98.70 %; `names` with `+=` steps 99.73 -> 99.94 % (the six remaining operands are retail's expression
temporaries r4/r0 against the `+=` web r17).

**Example.**

```c
names = (u8*)track + track->keyBytes + sizeof(EfPmTrack);   /* add r4,r3,r0 ; addi r?,r4,0x20 */
names += track->randomBytes;                                /* lwz ; add */
names += track->randomTableBytes;                           /* lwz ; add r17 */
memcpy(&field, (u8*)track + track->keyBytes + sizeof(EfPmTrack) + track->randomBytes + track->randomTableBytes +
                   track->nameTableBytes, sizeof(field));   /* sequential loads and adds, addi 0x20 last */
```
