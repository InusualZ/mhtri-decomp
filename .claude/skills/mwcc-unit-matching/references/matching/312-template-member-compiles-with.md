---
id: 312
title: A template member compiles with the pragma state at its instantiation point: instantiate members one by one
status: works
problem: A pragma pair around a template member's definition changes nothing, the same pragma left on to the end of the file fixes that member and regresses its siblings, and the object's function order or .sdata2 pool order is not retail's
tags: [pragma, sections, source-shape]
applies: [GC/3.0a5.2]
demo: 
related: [16, 32, 163, 278]
---

# 312. A template member compiles with the pragma state at its instantiation point: instantiate members one by one

**Problem.** A pragma pair (`#pragma opt_lifetimes off` ... `reset`) around a class template member's *definition*
has no effect. The same pragma with no reset fixes the member and regresses a sibling. Separately, a template unit's
function order and `.sdata2` pool order come out different from the retail link, so a 100 % unit still fails
`flipcheck` (relocations resolving to swapped pool words).

**Why it happens.** MWCC compiles a template member when it is instantiated, not where it is defined. A
`template class X<T>;` line at the end of the file instantiates every member there, in declaration order, so all
of them see the pragma state at that line. The functions and their pooled constants are emitted in instantiation
order. A non-template function is emitted at its definition, so it sorts by text position among the
instantiation lines.

**How to work it.**

1. Read the retail order of the unit's functions and pool words: the link keeps the referenced instantiations in
   object order and drops the rest, and the pool order shows which dropped functions were compiled first. For
   example, a constant used only by a dropped `<char>` constructor sits at the head of the pool.
2. Write the instantiations in that order at the end of the file, one explicit member instantiation per line where
   the order or the pragma has to differ (`template f32 X<wchar_t>::Get() const;`). Then put the plain function
   definitions between them, and end with the `template class` lines, which pick up anything not yet instantiated.
3. Wrap only the member that needs a pragma in the pair, around its instantiation line. Then re-measure every row.

**Result.** `nw4r/ut_TextWriterBase` (GC/3.0a5.2, `cflags_nw4r`): CalcLineRectImpl<w> went from 99.65 to 100. That
function had a frame of 0x1E0 where retail has 0x170, because lifetime splitting gave each dead store to an unused
`Rect` local its own slot. `opt_lifetimes off` for the whole file had cost PrintImpl<w> (100 -> 99.82). The unit
went from 10/11 to 11/11. Instantiating the char class, then `Font::GetCharStrmReader`, then the wchar_t members in
order fixed both the link order and the `.sdata2` order (FLT_MAX, 0.0, the int-to-float double, 0.5). `flipcheck`
went from NOT READY to READY, and the trial flip linked `main.dol: OK`.

**Example.**

```
template class TextWriterBase<char>;                 /* first: its constructor pools FLT_MAX and 0.0 */
CharStrmReader Font::GetCharStrmReader() const { ... }
template TextWriterBase<wchar_t>::TextWriterBase();
#pragma opt_lifetimes off
template bool TextWriterBase<wchar_t>::CalcLineRectImpl(Rect* rect, const wchar_t** str, int length);
#pragma opt_lifetimes reset
template class TextWriterBase<wchar_t>;
```
