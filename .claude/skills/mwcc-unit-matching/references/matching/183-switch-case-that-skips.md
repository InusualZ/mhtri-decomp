---
id: 183
title: A switch case that skips the shared tail, or a case that runs another case's tail: continue inside do { } while (0)
status: works
problem: Retail's switch has one copy of a tail that several cases share (or that some cases skip), reached by a `b` from a case body; duplicating the tail in each case adds a copy (MWCC does not tail-merge) and `break` cannot leave the enclosing block from inside the switch.
tags: [source-shape]
applies: [Wii/1.3]
demo: 
related: [145]
---

# 183. A switch case that skips the shared tail, or a case that runs another case's tail: continue inside do { } while (0)

**Problem.** Retail lays a switch out with one shared block after it: the value cases set a local and branch to it,
the default falls into it, and some cases jump past it (`switch (act) { case 4: ...: <skip>; case 0: ...: q = 64; default:
q = 0; } <tail>`); or a nested case (`case 0` with `kind == 1`) branches straight into another case's body. Written
goto-free, the tail is either duplicated into every case (each copy is emitted - MWCC does not cross-jump identical
tails, so the function grows by the tail's size) or needs a flag and a compare retail does not have. Inside a
`switch`, `break` only leaves the switch, so idea 145's `do { ... break; } while (0)` cannot express the skip.

**Why it happens.** `continue` in a `do { } while (0)` jumps to the loop condition, which is the constant 0, so MWCC
emits it as a plain branch past the loop - and `continue` is not captured by the inner `switch`. The shared tail
goes after the switch inside the loop: a `break` out of the switch reaches it, a `continue` skips it. A case that
retail sends into another case's code becomes an empty case (`case 3: break;`) whose body is the after-switch tail.

**How to work it.** Wrap the switch and its tail in `do { ... } while (0)`; turn the skipping cases into `continue`,
the cases that end in the tail into `break`, and give every case that must not run the tail a `continue` (its
own `break` would run it). Keep the cases in retail's body order. Read the jump table out of the DOL first when the
switch is table-driven: entries that point past the tail are the skip cases.

**Result.** `Pl/fn_802840DC` (Wii/1.3, -O3): `pl_weapon2_hit_react` 75.30 -> 100 % (the act-number switch with six
skip cases, eight 64-quake cases and a 0-quake default sharing one hit-latch tail; 52.28 % when the tail was
duplicated); `pl_weapon2_alt_act_charge` 91.19 -> 100 % (640 B; the duplicated hold tail was 688 B).
Also when retail's single copy sits *mid-switch* (reached by branches into a sibling case, idea 213): `lobby/lb_pane_ui`
`lobby_enter_seq_step` 85.20 -> 89.54 and `lobby_scene_setup_step` 84.88 -> 89.31 with the block moved after the switch.
`lobby/lb_npc` (Wii/1.3, -O4 by pragma): `lb_player_control` 93.76 -> 99.03 % (1884 -> 1840 B, the target's size;
the do-while wraps a whole `if`/`else if` chain - the success paths `continue` past the shared
`lb_act_armed_reenter` call, the failing spot cases `idle = 0; break;` into the post-switch idle test, and the
`else` arms fall through to the one reenter call; a 3-state flag measured 92.15); `lb_player_remote_follow`
82.26 -> 98.82 % (a nested case - family 1's act 3 - `break`s into the walk block written once after both
switches, the other cases `continue`; idea 218 is the rest).

**Example.**

```
do {
    switch (self->act_no) {
    case 4: case 16:            continue;   /* past the tail */
    case 0: case 22: quake = 64; break;      /* b tail        */
    default:         quake = 0;  break;      /* falls into it */
    }
    hit_latch(self, quake);                  /* one copy      */
} while (0);
```
