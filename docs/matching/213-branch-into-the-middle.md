---
id: 213
title: A branch into the middle of another case is not an optimisation: MWCC Wii/1.3 has no tail-merge or cross-jump pass
status: ruled-out
problem: retail's switch branches into the middle of a sibling case's code (forwards or backwards) and holds one copy of a shared tail; every goto-free spelling either repeats the tail or moves it, and no flag, pragma or pass merges the copies
tags: [source-shape, tooling]
applies: [Wii/1.3]
demo: 
related: [183, 203, 145, 214]
---

# 213. A branch into the middle of another case is not an optimisation: MWCC Wii/1.3 has no tail-merge or cross-jump pass

**Problem.** A step-machine `switch` in retail has one copy of a block that several cases run - "system file done"
(`865 = 0; state = 3`) reached from case 0, from case 1's inner case and backwards from `case 0x64`, an enter-city
tail reached backwards from state 5, a page-count tail reached from seven places. The hypothesis was that the source
was goto-free and an MWCC pass (tail merging, cross-jumping, jump threading) folded identical copies. It is not:
the compiler has no such pass, so the retail layout is the source's own control flow.

**Why it happens.** Evidence, all from the Wii/1.3 `mwcceppc.exe` the build runs:

* *The pass inventory.* The compiler's symbol names hold the frontend (IRO) control-flow passes
  `IRO_DoJumpChaining`, `IRO_RemoveRedundantJumps`, `IRO_OptimizeSwitches`, `IRO_RemoveTailRecursions`,
  `IRO_MergeFlowGraphNodes`, and the PCode-level `mergeblocks`/`canmergeblocks`/`peepholemergeblocks`/
  `optimizefinalbranches` - every one of them retargets a jump to a jump or joins a block to its *single*
  successor. Nothing compares two blocks for equality (there is no cross-jump/tail-merge routine).
* *The debugger timeline.* `tools/mwcc-debugger` on a one-function reproduction of `lobby_enter_seq_step`
  (the full unit dies with gdb `MemoryError ... 0x34`, see `.pi/notes/mwcc-debugger-gaps.md`): the goto-free source
  has 73 blocks / 28 `bl` from `backend-03-after-unreachable-block-deletion` through `backend-24-after-code-labels`
  (213 instructions, `verify_pcode` MATCH against the object); the same function with retail's three `goto`s has
  67 blocks / 27 `bl` (186 instructions = retail's 744 B) at every dump. No pass changes the block count or the
  number of copies - the difference is already in the first backend dump, i.e. it is the source's flow graph.
* *The levers.* `#pragma peephole on` around the function (848 B, no merge) and `#pragma optimization_level 4`
  (unchanged) do not fold the copies either.
* *Goto-free C cannot express it.* Without `goto`, a jump target is a loop head or condition, a loop or switch exit,
  an if/else join or a case label, and jump chaining only retargets a jump whose target is an unconditional jump to
  one of those. A target in the *middle* of another case's code is none of them (only a Duff's-device case label
  inside another case's block reaches it, which rule 8 forbids as a label used for control flow).

**How to work it.** Stop searching for the merging shape. Measure the goto spelling once in a scratch copy (never
committed) to know the ceiling, then pick the best goto-free spelling and record both numbers in the unit header:

1. when every entry ends in the *same* exit (falls to the code after the switch), idea 183's `do { switch ... }
   while (0)` with the shared block after the switch and `continue` on every other path keeps one copy - the block
   moves to the end of the switch, but the function shrinks to near retail's size and usually scores higher than
   the duplicated form;
2. when the shared block ends in its own exit (`return` past the after-switch code) or sits at the start of a case,
   moving it to the end costs more than the duplicate (measure; keep the duplicate or idea 203's re-dispatch);
3. a static helper standing in for the tail keeps the size close but adds a function and a `bl` per site.

**Result.** Wii/1.3, `cflags_lobby` (-O3):

| row | current | goto ceiling (scratch) | best goto-free |
| --- | --- | --- | --- |
| `lobby/lb_pane_ui` `lobby_enter_seq_step` | 85.20 (852 B) | 98.92 (744 B) | **89.54** (772 B; 183 + `done = 1` before the call) |
| `lobby/lb_pane_ui` `lobby_scene_setup_step` | 84.88 (756 B) | - | **89.31** (756 B; same shape) |
| `menu/menu_placeinfo` `lb_city_select_screen_step` | 98.26 (3768 B) | 100.0 (3704 B) | 98.26 kept; 183 tail-after-switch 96.30 (3704 B) |
| `menu/menu_placeinfo` `lb_community_select_step` | 96.31 (helper) | 99.24 (3336 B) | 96.31 kept; 183 tail-after-switch 3360 B, worse |

The same diagnosis on four more rows (scratch TUs with `cflags_main`: a constant flag before a `break`, a loop
variable re-tested after its own loop and an inline function's constant `return` all keep their compare, also under
`optimization_level 4`; identical `if` and `case` tails both stay): `enemy/em018_prog` `em018_motion_events` (case 0x55
into case 0x56's water tail, 98.70; Duff 99.32), `Network/network_pat_control` `updateMessagePool` (states 3/4/6
backwards into states 1/2, 75.96) and `quest/quest_entry` `quest_main_step` (fail and clear tails entered backwards,
83.72) stay; `camera/camera_main` `camera_follow_collide`, whose jumps are all forward, has a goto-free spelling
(idea 214).

The second shared block of the lobby pair (the `game_system_file_create_start` test reached from case 0 and case 1)
stays duplicated: with the "system file done" block after the switch, a third exit kind (skip both) is not
expressible without a flag.

**Example.**

```
do {
    switch (self->state) {
    case 0:  ...; if (started) { self->state = 1; continue; }  break;   /* break -> shared block */
    case 2:  switch (wait()) { case 1: break; case -1: self->state = 0x64; continue; default: continue; } break;
    case 0x64: if (pressed) break; ...; continue;                         /* retail: backward b */
    default: continue;
    }
    system_w.save_pending = 0;      /* the one copy retail branches into */
    self->state = 3;
} while (0);
```
