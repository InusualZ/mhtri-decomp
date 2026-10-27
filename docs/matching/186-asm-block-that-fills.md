---
id: 186
title: An asm block that fills a local frame record: reference the parameters in the asm, peephole and optimizewithasm off
status: works
problem: Retail stores a function's parameters to stack slots at entry and keeps `addi r3,r1,N` + `psq_stx` pairs inside an asm block; ours keeps the parameters in registers, fuses each pair into `psq_st` and reorders the asm stores.
tags: [pragma, source-shape]
applies: [Wii/1.3]
demo: 
related: [127, 39]
---

# 186. An asm block that fills a local frame record: reference the parameters in the asm, peephole and optimizewithasm off

**Problem.** A C function whose body is mostly an `asm { }` block that saves registers into a local struct
(`__throw` capturing a `ThrowContext`) comes out with the parameters held in `r6`/`r0` across the block, every
`la r3, local.field` + `psq_stx fN, 0, r3, 0, 0` pair fused into one `psq_st fN, off(r1)`, and the block's last
stores reordered. Retail stores `r3`/`r4`/`r5` to their stack homes at entry (`stw r3,8(r1)` ...), reloads them
after the block, keeps every `addi`+`psq_stx` pair and ends with a `nop` after the `bl`.

**Why it happens.** Three separate passes: the scheduler treats the asm like compiled code (idea 127), the peephole
pass folds the address computation into the store's displacement, and the parameters only get memory homes when the
asm itself names them. `#pragma optimization_level 0/1`, `global_optimizer off`, `opt_propagation off`,
`optimize_asm off` and `volatileasm on` change nothing here.

**How to work it.** Scope `#pragma optimizewithasm off` and `#pragma peephole off` to the function (push/pop), and
move the parameter copies and the call into the asm block (`lwz r3, throwtype` / `stw r3, ctx.throwtype` ...,
`la r3, ctx`, `bl Handler`, `nop`). Dropping either pragma loses the function again (45 % without
`optimizewithasm off`).

**Result.** `Runtime.PPCEABI.H/ExceptionPPC` `__throw` (0x144): 53.58 -> 100 (sizes 0xE8 -> 0x144).

**Example.**

```cpp
#pragma push
#pragma optimizewithasm off
#pragma peephole off
extern "C" void __throw(char* throwtype, void* location, void* dtor)
{
    ThrowContext ctx;
    asm {
        stfd f14, ctx.FPR[14].d
        la r3, ctx.FPR[14].ps
        psq_stx f14, 0, r3, 0, 0
        lwz r3, throwtype
        stw r3, ctx.throwtype
        la r3, ctx
        bl ExPPC_ThrowHandler
        nop
    }
}
#pragma pop
```
