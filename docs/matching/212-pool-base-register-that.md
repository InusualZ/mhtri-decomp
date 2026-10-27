---
id: 212
title: A pool base register that names a mid-section object marks the TU's own section start
status: works
problem: every `addi rX,rBase,off` that reads a pooled string differs by one constant (ours higher), and retail's `lis`/`addi` base relocation names an object in the middle of the unit's `.data` where ours names the section symbol
tags: [data, sections, measurement]
applies: [Wii/1.3]
demo: 
---

# 212. A pool base register that names a mid-section object marks the TU's own section start

**Problem.** Every string or table load off the pooled base differs by the same displacement (ours `addi
r4,r30,0x1A8`, retail `addi r4,r30,0x118`), the rows sit at 99.9x %, and retail's base pair
`lis/addi rBase,X@ha/@l` names an object (`DWCi_authDataPath`) that is not the first object of the unit's claimed
`.data`, while ours names `...data.0`. The code is right; the unit is not.

**Why it happens.** Under `-pool`, MWCC addresses a TU's own `.data`/`.bss` objects off one base, and that base
is the TU's section start (the compiler's `...data.0`). A retail base pointing inside the claimed range means
the original object's section started there: everything in front of it belongs to another TU, and its bytes
shift every displacement by their size.

**How to work it.** Read the retail base relocation and the size of the claimed data in front of it. Confirm by
compiling the unit with the functions and data in front of that object removed (a scratch edit, measured and
reverted): the displacement rows should go to 100 %. Then report a seam re-draw - the functions that own the
bytes in front of the base (their `.text`, `.data`, `.sbss`) are a separate unit. Never pad or reorder data to
fake the offset.

**Result.** `DWCi/dwc_auth_interface`: retail base `DWCi_authDataPath` (0x80630020) sat 0x90 bytes into the
claim 0x8062FF90..; with `DWCi_Np_CPUCopyFast`/`DWCi_GetConsoleFriendCode` and their `.data`/`.sbss` removed,
`DWCi_initRuntime` 99.96, `DWCi_authDataTask` 99.98, `DWCi_Auth_StartRequest` 99.94 and
`DWCi_Auth_RequestCallback` 99.91 all reach 100; split into `DWCi/dwc_nonport.c` the auth unit is 14/14 at
100 % and a trial flip links the retail DOL.

**Example.**

```
retail: lis r30,DWCi_authDataPath@ha ; addi r30,r30,DWCi_authDataPath@l ; addi r4,r30,0x118
ours:   lis r30,...data.0@ha         ; addi r30,r30,...data.0@l         ; addi r4,r30,0x1a8
```
