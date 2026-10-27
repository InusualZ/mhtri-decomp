---
id: 249
title: A struct returned in r3:r4 and stored twice is a named copy of a temporary
status: works
problem: retail stores a small struct's r3/r4 return to two stack slots and reads only one; ours stores it once, so the frame and every later stack offset shift by 8
tags: [source-shape, allocator]
applies: []
demo: 
---

# 249. A struct returned in r3:r4 and stored twice is a named copy of a temporary

**Problem.** After a call returning an 8-byte struct in r3:r4 (GC/3.0a5.2 returns `Vec2f` this way), retail writes
`stw r3/r4` to two different stack slots - one is never read again - and loads the fields from the other. Ours
(`Vec2f v = obj->get();`) builds the value in place: one slot, a frame 8 bytes smaller, and every later stack
offset off by 8, which reads as dozens of argument mismatches.

**Why it happens.** The original spelled the value as a temporary plus a named copy; MWCC materialises the call
result into the temporary, copies the registers into the named local (dead stores, kept), then forwards the
temporary's fields to the uses.

**How to work it.** Name the temporary and copy it: `Vec2f vTemp = obj->get(); Vec2f v = vTemp;`. Declaring the
named local apart and assigning it (`Vec2f v; v = obj->get();`) measures worse. Sweep every site with
`mt.py tryvar <unit> --sites '^(\s*)Vec2f (\w+) = (self->v_0x18[8C]\(\));' '\1Vec2f \2Temp = \3;\n\1Vec2f \2 = \2Temp;'`
- it keeps only the sites that improve.

**Result.** homebutton/hbm_text_panel: +27 B hbmPanelHitTest, +32 B hbmPanelLayoutText, +9 B
hbmPanelFindLineOverflow, then +83 B over four more sites (hbmPanelHandleMessage's frame went 0x250 -> 0x270, equal
to retail); unit 97.9985 -> 98.8827 % with the other lane changes. Sites whose result is read once directly
(`self->v_0x18C().x`) and the position read of hbmPanelLayoutText measure worse and stay as they are.

**Example.**

```
Vec2f scaleTemp = self->v_0x18C();
Vec2f scale = scaleTemp;          /* retail: stw r3,0x30; stw r4,0x34; stw r4,0x24; stw r3,0x20 */
```
