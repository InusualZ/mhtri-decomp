---
id: 93
title: A Type_name(Type* self) free function is a member: define Type::name and rename the map row to the mangling
status: todo
problem: A function is spelled Type_name(Type* self, ...) and its map row is the unmangled Type_name - the compiler will mangle the real member (name__<len>Type...), so the retail symbol reads as a member the source never declared
tags: [source-shape, symbols]
applies: [Wii/1.3]
demo: 093-type-name-type-self.cpp
---

# 93. A Type_name(Type* self) free function is a member: define Type::name and rename the map row to the mangling

**Problem.** A reconstructed function is written `s32 NetworkSingleTcp_send(NetworkSingleTcp* self, const u8* data, s32 size)`
and its map row is the unmangled `NetworkSingleTcp_send`, so the unit needs `extern "C"` (or a stale map name) to pair
by name - while every call site (`NetworkSingleTcp_send(this->connection, ...)`) reads like a method call on the
object the first parameter names. The C++ project has a class whose method was spelled the C way (rule 13 of
docs/plan.md section 6.5).

**Why it happens.** Reconstruction starts from the address and the `bl` target, so the first draft is a free function
with an explicit `self`; nothing forces the class to list the method. MWCC mangles a member as
`name__<len>Type[C]F<params>` (`send__16NetworkSingleTcpFPCUcl`), so objdiff pairs the member by that name and the
map row has to carry it. `this` arrives in r3 exactly like the old `self`, so for a non-virtual method the body is
the same instructions; only the symbol (and, for a `const` self, the `C` qualifier) changes.

**How to work it.** `python tools/units/methodize.py <Type>` prints the plan: declaration and definition, every
reference, the member signature and the estimated mangling. Then, in one change: declare `name` in the class, define
`Type::name` (body unchanged, `self->` becomes `this->` or bare fields), call `obj->name(...)`, and rename the map row
with `python tools/symbols/symedit.py rename <Type_name> <mangled>` (a rename is two edits; `methodize.py --exact`
confirms the mangling with the compiler, and `tools/units/mangle.py` proves a single signature). Re-measure the unit
and its callers: the declaration set is a codegen input (row 60). A function that is genuinely C - retail C linkage
evidenced by an unmangled name in the shared dump or a caller's relocation, or a plain C struct from an SDK API - is
marked `/* free: <reason> */` on its declaration instead.

**Result.** Not yet measured on a unit: the demo below is the compiler-level claim (same body, the member emits a
mangled name and the free form the plain one), written without a compiler in reach - run
`python tools/agents/ideas.py demo-check 93` before promoting this to `works`.

**Example.**

```
extern "C" int Tcp_send(Tcp* self, int n) { return self->a + n; }   // symbol Tcp_send
int Tcp::send(int n) { return a + n; }                              // symbol send__3TcpFi
```
