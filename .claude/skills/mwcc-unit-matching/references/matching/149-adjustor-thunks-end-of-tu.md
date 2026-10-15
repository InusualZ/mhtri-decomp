---
id: 149
title: Adjustor thunks sit at the end of their TU, so thunks ahead of other code mark a TU boundary
status: todo
problem: A unit's adjustor thunks (`addi r3,r3,-N ; b <method>`) sit in the middle of its claimed `.text`, ahead of functions ours emits before them, and the unit cannot be laid out as one object.
tags: [sections, vtable]
applies: [GC/3.0a5.2]
demo: 
related: [80, 98]
---

# 149. Adjustor thunks sit at the end of their TU, so thunks ahead of other code mark a TU boundary

**Problem.** A class with a secondary base needs adjustor thunks (`addi r3,r3,-4 ; b <method>`) for the
secondary vtable part. In retail a group of thunks sits before more functions of what was registered as the same
unit, while ours always emits the thunks last.

**Why it happens.** MWCC (seen under the homebutton band's flags, `-ipa file`) emits a TU's adjustor thunks after
all of its functions. A thunk group followed by more code therefore ends one TU; the code after it belongs to the
next.

**How to work it.** Treat the end of a thunk group as a TU seam candidate, alongside the `.data` order seams (idea
80): cut the registered range after the thunks and check the second part's data order agrees.

**Result.** Observed, not yet measured as a split: `homebutton/gui`'s thunks sit before `hbmGuiDrawQuad` in retail,
so 0x80569E64..0x8056A478 is a second TU (`homebutton/hbm_gui_draw`); `homebutton/hbm_kb_list` and
`hbm_kb_cursor` end in their thunks.

**Example.**

```
80569DAC create / init / ... ; three thunks `addi r3,r3,-4 ; b ...`   <- end of TU
80569E64 hbmGuiDrawQuad ...                                           <- next TU
```
