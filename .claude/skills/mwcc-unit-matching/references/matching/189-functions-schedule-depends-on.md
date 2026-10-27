---
id: 189
title: A function's schedule depends on the previously compiled function: anchor it with a dead-stripped predecessor
status: works
problem: The first function of a unit has two independent instructions in the wrong order (same size, same relocations) and no source shape, flag, pragma or compiler version moves them.
tags: [source-shape, linker, process]
applies: [Wii/1.3]
demo: 
related: [36, 147]
---

# 189. A function's schedule depends on the previously compiled function: anchor it with a dead-stripped predecessor

**Problem.** A unit's first function differs from retail only by two independent, adjacent instructions in the
other order (a preheader `addi rX,rX,sym@l` and `li rY,0`), the size and relocations are identical, and every
shape of the function, every `-opt`/`#pragma opt_*` setting and every Wii/GC compiler version reproduces the same
swap.

**Why it happens.** MWCC's first `scheduling` pass (backend dump `after-scheduling`, the earlier of the two) keeps
per-physical-register state across the functions of one compile. A dead copy of the incoming `r3` (`mr r32,r3`,
removed only after scheduling) is placed by that state: after a function that read an argument register the
scheduler emits retail's order, as the first function of a fresh compile it does not. Retail compiled *something*
before the function - a function the link later dead-stripped, or the previous file in the same compiler process.
`-inline deferred` "fixes" it only because it compiles the functions in reverse order.

**How to work it.** Confirm with a probe: put `int f(int a) { return a + 1; }` before the function and recompile
on the unit's real command line - if the order flips to retail's, the cause is this. An empty function, an `asm`
function, `return a;` or `return 0;` do not change the state; any predecessor that schedules a read of an argument
register does (`*p = 0`, `return *p`, `a + b`). In the unit, write a never-called `static` function before the
first function, mark its body as a GUESS in the unit header, flip, and prove the DOL hash: mwld dead-strips the
unreferenced function, so the linked bytes are retail's.

**Result.** `Runtime.PPCEABI.H/Gecko_ExceptionPPC`: `__register_fragment` 93.68 % -> 100 %, unit 14/14 rows, flipped
to `Matching` with `main.dol` SHA-1 `bf4850739478caaedfe675949eb7c28595a7fde9`.

**Example.**

```
/* Never called (dead-stripped); compiled first, it leaves the scheduler the state retail's order needs. */
static int ExPPC_SchedulerAnchor(int value)
{
    return value + 1;
}

extern "C" int __register_fragment(struct __eti_init_info* info, char* TOC) { ... }
```
