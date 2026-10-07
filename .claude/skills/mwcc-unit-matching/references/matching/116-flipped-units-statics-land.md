---
id: 116
title: A flipped unit's statics land in reverse definition order, an unreferenced tail is stripped, and a short .text shifts the unflipped successor
status: works
problem: a unit at 100 % with flipcheck READY (or only a trailing-pad note) moves the DOL hash when flipped, and the zero-filled .sbss/.bss hides the cause
tags: [data, linker]
applies: [Wii/1.3]
demo: 
---

# 116. A flipped unit's statics land in reverse definition order, an unreferenced tail is stripped, and a short .text shifts the unflipped successor

**Problem.** `flipcheck` compares section bytes, and `.bss` / `.sbss` have none, so three layout defects pass it and then break the link hash
(`ninja diff` only says `Expected to find symbol @eti_8001E558`; `tools/mwlink_debugger.py trace <unit>` names the real cause).

**Why it happens.**
1. The compiler emits uninitialised statics in **reverse definition order** (the first definition lands last). Two words in the wrong order
   (`DBVerbose` / `__DBInterface`, the `FS/fs` statics) read 100 % and relocate to the wrong addresses.
2. mwld dead-strips an unreferenced variable, so a claimed `.bss` tail nothing reads (`DVD/dvdqueue`: 0x10 B after the queues) disappears from our
   object's section and shifts everything after it. A separate `static`/global filler is stripped too; sizing the array that precedes the gap to
   cover it keeps it.
3. A unit whose `.text` ends short of its claim (the 8 B alignment pad before a 16-aligned neighbour) is harmless only while the **next** object is
   also a `src/` object (alignment 16, which re-creates the pad). Flipped in front of an unflipped target object (alignment 4) the following
   `.text`, then `.data`, moves (`IPC/ipcMain` before `IPC/ipcclt`). Flip the successor first, or emit the pad.

4. An object whose map row is **larger than the part the code reads** (an 8 B `.sbss` open-state or byte-order object of which only the first
   word is touched) is dead-stripped as the shorter global the source declares, with the same effect as 2. Model the whole object: a struct
   with an `unused_0x04` word (`TRK/gdev_cc`, `TRK/nubinit`), and check the section sizes with `mwlink_debugger.py trace <unit>`.

**How to work it.** Define statics in the reverse of the target's address order; cover an unread claimed tail with the preceding array; flip a unit
only when its `.text` equals the claim or its successor is already flipped; run `trace <unit>` after every flip that moves the hash.

**Result.** `DB/db`, `DVD/dvdqueue`, `FS/fs`, `EXI/ProbeBarnacle`, `EUART/EUART` and `DVD/dvdidutils` flip with the hash
`BF4850739478CAAEDFE675949EB7C28595A7FDE9`; `IPC/ipcMain` at 100 % does not until `IPC/ipcclt` is flipped.

**Example.**

```c
/* target .sbss: __DBInterface (+0), DBVerbose (+4) */
u32 DBVerbose;
DBInterface* __DBInterface;
```
