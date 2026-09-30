---
id: 57
title: A call-site mask means the callee's parameter is declared wider than the value
status: works
problem: A wrapper (or any caller) sits at 50-90 % and the first divergence is one instruction at the `bl`: retail masks or sign-extends the argument (`clrlwi r4,r4,16`, `extsh r5,r5`, a `slwi`/`srawi` pair) and ours passes it straight through. It reads as a scheduling or inlining residual, nothing in the caller's own source hints at a mask, and the search goes to flags and to the caller's statement order - where the mask cannot exist.
tags: [source-shape]
applies: []
demo:
---

# 57. A call-site mask means the callee's parameter is declared wider than the value

**Problem.** A wrapper (or any caller) sits at 50-90 % and the first divergence is one instruction at the `bl`:
retail masks or sign-extends the argument (`clrlwi r4,r4,16`, `extsh r5,r5`, a `slwi`/`srawi` pair) and ours
passes it straight through. It reads as a scheduling or inlining residual, nothing in the caller's own source
hints at a mask, and the search goes to flags and to the caller's statement order - where the mask cannot exist.

**Why try it.** The mask is the *caller's* cost of the *callee's* prototype: MWCC converts an argument to the
callee's declared parameter width, so a parameter declared `s32`/`u32` forces a mask on a value whose own type
is narrower (`s16`, `u8`, a bitfield), while a parameter declared with the narrow type does not. The mask is
therefore evidence about a declaration that is not in the function you are looking at.

**Result.** When retail's call site carries a mask ours does not, widen the **callee's parameter** to
`s32`/`u32` and narrow explicitly at the use inside the callee (which keeps the callee's own codegen) - or, for
a local, widen the local's declared type and mask at the use. Measured across one `ai` band without any flag
change: `fn_802D2ABC` 56.7 -> 90, `fn_802D287C` 85.9 -> 93.8, `fn_802D2264` 77.9 -> 81.7, and ten thin wrappers
went to **100 %**. The mirror case is the same lever: `fn_802D3984` (76.14 %) has our mask *too* wide, i.e. a
parameter declared wider than retail's, so the presence *or absence* of the mask is a statement about the
callee's declared width rather than about the caller.

**Example (the layout calculator that found the band's other class).** These records are written as explicit pad
arrays (`u8 pad_0xNNN[0xMMM - 0xNNN];`), which makes the layout mechanically checkable: a 30-line checker
asserts that the offsets tile (every filler's declared end equals the next member's offset, no duplicate
offsets). It caught `field_0x216` declared `s32` where the code does a `lbz` - the field is **1** byte, so every
later field sat 2 or 8 bytes out and ~20 functions each lost ~20 points. Take a field's size from the **access
width in the code** (`lbz` = 1, `lhz` = 2, `lwz` = 4, `stb`/`sth` likewise), never from the type you guessed,
and assert the tiling before you measure.

**Refinement, same session (a second and third view of the same record).** Two traps the first merge walked into,
both of them invisible to the tiling assert:

* **A header file can define more than one struct.** Keying members by offset alone matched `_AINPC_W`'s offsets
  against a smaller struct that sits above it in the same file (`0x0`, `0xC`), so the "next member offset" that
  sizes a field came from the wrong struct - `active` was sized 12 B instead of 1 B, and every splice was then
  refused for lack of room. Parse **per struct**: name each group, keep its own member list, and compute every
  size and every covering filler *within* the group. (A byte-wide filler is also written `u8 unused_0xNNN;` with
  no `[0xE - 0xN]` bracket, so a tiling check that reads only the bracket form should not flag it - the field
  ends at the next member either way.)
* **Compare the declaration, not just `(offset, name)`.** Fourteen differences: six were fillers the splices
  themselves superseded (a filler shrinks as its sub-fields arrive - the expected shape), and one was a genuine
  conflict the `(offset, name)` rule skipped silently - main had `u8 field_0x3F8;`, the newcomer
  `u8 field_0x3F8[4];`, and the newcomer's code indexes it. The merged header therefore compiled every *landed*
  consumer and not the newcomer's own source (`illegal operands 'unsigned char' [ 'unsigned char'`). **The test
  of a record merge is that both sides' sources compile** - whole-tree `ninja -k 0` at 0 FAILED, not just the
  consumer's object - **plus the rows** (`ninja changes` must print no line for a unit that already owned the
  record). Only the same-name-different-size class needs a decision, and the side whose *code* depends on the
  declaration has the better claim: here the array won, the scalar's neighbouring `unused_0x3F9` filler (which
  the array now covers) was dropped, and the whole-tree build plus an empty `ninja changes` proved both
  consumers and the newcomer.

**The tool.** `tools/units/recordmerge.py` implements the three rules above:

    python tools/units/recordmerge.py --base include/ai/ainpc.h         --other worker/<slug>:include/ai/ainpc.h --out include/ai/ainpc.h

It refuses to write while anything is unresolved - a named member in the way, no room in the covering
filler, a same-offset rename, a member whose size is the struct total - and prints the per-group delta so
the decision is visible. Top-level lines only the other view has are carried verbatim after the last group
when they are declarations (reported), and reported but not carried when they are not. Run against the
merge that produced this section (`089491a7b`'s header and the `802d44f4` view) it reproduces the 36
splices, the 33 filler splits and the `0x3F8` decision exactly, and keeps one trailing comment the hand
pass typed away. It is still only the edit: **the proof remains the whole-tree build plus `ninja changes`.**
