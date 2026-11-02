---
id: 288
title: A block-local reference to a global numbers the inlined copy above the @ temporaries and colours it first
status: works
problem: An inline helper called on a global (`Scroll(sConsole)`) keeps its copy of the global in r30 and a field it reads in r31 where retail has them swapped, and the same helper matches at another call site of the same unit
tags: [source-shape, allocator]
applies: [GC/3.0a5.2, Wii/1.3]
demo: 
related: [243, 244]
---

# 288. A block-local reference to a global numbers the inlined copy above the @ temporaries and colours it first

**Problem.** `Console_ShowLatestLine(sAssertionConsole)` inlined into `Warning` gives `lwz r30, sAssertionConsole;
lhz r31, 0x20(r30); mr r3, r30; bl; subf. r0, r31, r3; stw r0, 0x18(r30)` where retail has r31 for the console
and r30 for the height (6 bytes on 5 instructions). The same inline at VPanic's site matches with our registers, so
the inline's body is right and no declaration order, helper inline, static (auto-inlined) helper, accessor argument,
hoisted store/argument or `opt_lifetimes off` moves the pair.

**Why it happens.** The allocator dump (idea 243) shows the three nodes: the height is a hoist-phase temporary
(`@438`, the field read moved ahead of the call), the `if (sAssertionConsole)` load is a CSE temporary (`@444`) and
the copy of the global the inlined body uses is the last CSE temporary (`@446`). `@` temporaries are numbered in
descending id and coloured in descending number: the height takes r31 (new), the `if` load takes r30 (new, r31 is held
by the va_list address), and the copy, coloured last, takes the lowest handed-out register free of its neighbours -
r30. Retail's order needs the copy coloured before the `if` load, which no CSE-phase temporary can be. The
va_list's address temporaries have no `@` name and are numbered *above* every `@` temporary; a reference bound to the
global (`ConsoleHandle& console = sAssertionConsole;`) makes the copy an address temporary of that class, so it is
coloured first and takes r31, and the height then takes the new r30. At VPanic's site the `if` load is volatile, so the
plain spelling already colours as retail does.

**How to work it.** When an inlined helper's copy of a global and a value it loads are swapped between r30/r31 and
another call site of the same helper matches: dump the function, confirm the copy is the last `@` temporary (`regsim
verify` reproduces the dump), then bind the global through a block-local reference at the call site and pass the
reference. Leave the other sites as they are. The reference compiles to the same `lwz` (no address is materialised).

**Result.** `nw4r/db_assert` (GC/3.0a5.2, -O4,p, `-ipa file`): `Warning` 99.70874 -> 100, unit 5/6 -> 6/6 rows,
flipcheck READY and a trial `Object(Matching)` link byte-identical. Measured with the Wii/1.3 proxy dump (the GC
compiler has no debugger tables; it emits the same object for every spelling tried).

**Example.**

```
        Console_Printf(sAssertionConsole, "\n");
        {
            ConsoleHandle& console = sAssertionConsole;   /* was Console_ShowLatestLine(sAssertionConsole); */
            Console_ShowLatestLine(console);
        }
```
