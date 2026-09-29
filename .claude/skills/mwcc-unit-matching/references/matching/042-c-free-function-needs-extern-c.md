---
id: 42
title: A C++ free function needs `extern "C"` so objdiff can pair it by name
status: works
problem: A `fn_*` function defined in a `.cpp` file measures 0 % while its bytes are right: MWCC mangles the free function (`fn_80073398__FP9ResHandle`) and objdiff pairs symbols by name, so neither side pairs and the function contributes nothing.
tags: [symbols, measurement]
applies: []
demo:
---

# 42. A C++ free function needs `extern "C"` so objdiff can pair it by name

**Problem.** A `fn_*` function defined in a `.cpp` file measures 0 % while its bytes are right: MWCC mangles the free function (`fn_80073398__FP9ResHandle`) and objdiff pairs symbols by name, so neither side pairs and the function contributes nothing.

**Why try it.** Wrap the `fn_*` definitions in `extern "C"`: the emitted symbol becomes the plain map name and every symbol pairs. This is the source half of playbook 31 (the map rename is the other half).

**Result.** 1 unit(s) measured the same lever independently, so the evidence is grouped here rather than written once per outbox:

* `auto/80073398_fn_80073398` - extern "C" on the fn_* definitions - every fn_* symbol: 0 -> 100 (MWCC mangles a C++ free function as fn_80073398__FP9ResHandle, which objdiff cannot pair)

**Example.**

```cpp
extern "C" void fn_80073398(ResHandle* self) { ... }
```