---
id: 66
title: A narrow RETURN TYPE is visible at the caller
status: works
problem: The caller is one instruction off at the `bl`: the target stores the result with a raw `sth`, ours masks it first - a `clrlwi`/`extsh` retail does not have - and every later instruction shifts with it. It reads as caller-side scheduling, where a mask cannot come from.
tags: [source-shape]
applies: []
demo: 066-narrow-return-type.cpp
---

# 66. A narrow RETURN TYPE is visible at the caller

**Problem.** The caller is one instruction off at the `bl`: the target stores the result with a raw `sth`,
ours masks it first - a `clrlwi`/`extsh` retail does not have - and every later instruction shifts with it.
It reads as caller-side scheduling, where a mask cannot come from.

**Why try it.** MWCC converts a value to the **callee's declared** width, so a mask at the call site is
evidence about the *callee's signature*, not about the caller's source. This is row 57 in the return
position: there the callee's parameter was declared wider than the value; here its return type is declared
narrower than the value the caller wants.

**Result.** `NetworkLogger::flag_48` is `u16`, not `s32`: with the `u16` return the target's raw `sth` comes
back (`receive` **92.30 -> 94.99**, `send` +0.03). Same class in the other direction: `registerReceiver` is
declared `s32` while its call sites cast to `u8` - retail's early returns are `li r3, -1` (a wide -1, not
`li r3, 0xff`) and the tail masks the `u8` it returns.

**Example.**

```c
u16 flag_48(void);                  /* the caller stores it raw:  sth r3, ... */
s32 flag_48(void);                  /* ... and this spelling masks at the store */
```

**Demonstration.** `066-narrow-return-type.cpp` (`ideas.py demo-check 66`) reproduces the return position: forwarding an
`s32`-returning callee as `u16` costs a `clrlwi`, a `u16`-returning callee is a plain tail call. The *store* form (`sth`
of a wide result) was not reproduced: storing `s32` or `u16` results into a `u16` field is a raw `sth` either way.
