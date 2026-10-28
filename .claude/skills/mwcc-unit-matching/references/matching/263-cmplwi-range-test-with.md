---
id: 263
title: A cmplwi range test with cmpwi case tests and no clrlwi is a switch on a u16 parameter under peephole on
status: works
problem: a switch dispatch tests its 0..N case run with one `cmplwi` and its single cases with `cmpwi`, on the raw parameter register; a u32 switch gives `cmplwi` everywhere and an s32 one `cmpwi 0; blt; cmpwi N; ble`
tags: [source-shape, pragma]
applies: [Wii/1.3]
related: [209, 207, 108]
demo: 
---

# 263. A cmplwi range test with cmpwi case tests and no clrlwi is a switch on a u16 parameter under peephole on

**Problem.** Retail's dispatch is `cmplwi r3,1; ble A; cmpwi r3,2; beq B; cmpwi r3,3; beq C` straight on the
parameter. A `u32` selector gives `cmplwi` for every test, an `s32`/`s16`/`s8`/enum selector a two-sided signed range
(`cmpwi r3,0; blt; cmpwi r3,1; ble`), and `#pragma optimization_level 4` (idea 108) changes neither.

**Why it happens.** The mixed signedness is a promoted `u8`/`u16` selector (idea 209): the value is an `int`, so the
case tests are signed, but the compiler knows it is non-negative, so the `0..1` run needs only its upper bound,
tested unsigned. Under `peephole off` the parameter is re-extended first (`clrlwi r0,r3,16`); the peephole pass drops
that `clrlwi` because the ABI already hands a `u16` argument zero-extended.

**How to work it.** Declare the parameter `u16` (or `u8`) and compile the function with the peephole pass on
(`#pragma peephole on` ... `#pragma peephole off` around it in a peephole-off file). Fix the owner's header to the
same type and prove the callers' objects unchanged (a constant argument needs no extension either way).

**Result.** `sound/snd_stream_reloc`: `snd_output_mode_set` 95.86 -> 100 %; `snd_bank_loader.o` (the caller,
through the leaf header) sha1-identical before and after the header change.

**Example.**

```
#pragma peephole on
void snd_output_mode_set(u16 mode)
{
    switch (mode) {                          /* cmplwi r3,1 ; ble / cmpwi r3,2 ; beq / cmpwi r3,3 ; beq */
    case 0: case 1: AXSetMode(0); break;
    case 2: AXSetMode(1); break;
    case 3: AXSetMode(2); break;
    }
    MIXSetSoundMode(mode);
}
#pragma peephole off
```
