---
id: 156
title: A redundant same-type cast on the base pointer is a codegen input: it decides whether a field address is shared with its load
status: works
problem: Retail loads a field off the base (`lwz r6,0x654c(r3)`) and materialises the field's address last (`addi r8,r3,0x654c`), while ours computes the address first and loads through it (`addi r8,r3,0x654c` ... `lwz r6,0(r8)`) - the source passes both `x->f.code` and `&x->f`.
tags: [source-shape]
applies: [Wii/1.3]
demo: 
reviewed: 2026-10-15
related: [35, 60]
---

# 156. A redundant same-type cast on the base pointer is a codegen input: it decides whether a field address is shared with its load

**Problem.** A call passes a field and the field's address (`f(self, x->err.code, &x->err)`); retail loads the field
off the base register and builds the address in the argument slot, ours builds the address once and loads the field
through it, so two instructions swap and one operand changes.

**Why it happens.** MWCC keeps a cast expression as its own node even when it casts to the pointer's own type (a
typedef of the same class). `((T*)self)->err` and `&((T*)self)->err` then share the cast subexpression, and common
subexpression elimination turns the address into the load's base; without the cast the two are separate
`self + 0x654C` nodes and each is folded into its own instruction. The effect runs both ways: removing the same kind
of cast elsewhere changed the allocation of a whole function.

**How to work it.** When `self` is already the type the cast names, try the spelling without the cast at the use
sites of the diverging function only, and measure every row of the unit: keep the cast where it measures better.
Do not strip such casts file-wide on faith.

**Result.** `Network/PatInterface` (2026-10-15): `reportPatError` 76.71 -> 100.00 with `self->pendingError_654C`
instead of `((PatInterface*)self)->pendingError_654C` (`NetworkInstance` is a typedef of `PatInterface`); stripping
the 272 `(PatInterface*)self` casts of the file moved no other row, while stripping the `(NetworkStateMachine*)self`
casts of `handleNetworkState2Binary` (the same typedef) took it 71.29 -> 61.91, so those stay.

**Example.**

```
typedef PatInterface NetworkInstance;
void reportPatError(NetworkInstance* self, u32 kind, Error e)
{
    ...
    /* retail: li r5,0 ; lwz r6,0x654c(r3) ; li r7,1 ; addi r8,r3,0x654c */
    dispatch(self, kind, NULL, self->pendingError_654C.code_00, 1, (const u8*)&self->pendingError_654C);
    /* with ((PatInterface*)self)-> on both: addi r8,r3,0x654c ; li r5,0 ; lwz r6,0(r8) ; li r7,1 */
}
```
