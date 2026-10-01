# Phase 1, band `d`: `.text` 0x802A0000 .. 0x80380000

Proposal file: `docs/splits/proposals/phase1-d.json` (format and extension keys: `docs/splits-program.md` and the band `a`
section of it; the only key added here is `kind` on a cut). Tree `15dc2cf7d`. The band holds 52 registered units
(0x8029F3C8 .. 0x80382310) and **three unowned runs, 146 functions** (0x802DDC04..0x802E0740 = 61, 0x8035FC18..0x80365C84 =
59, 0x8036A690..0x8036CF64 = 26). Everything the band decides is about pool and `__sinit` evidence; the runs carry almost no
pool evidence of their own.

## Counts

| | |
| --- | --- |
| proposal units (rendered candidate) | 17: 6 merges (`stg_w`, `camera_main`, `ai_npc`, `cockpit_quest`, `em020_prog`, `em019_ai`), 6 tails cut off a registered unit (`eft035`, `fn_802F9994`, `fn_8031DAA8`, `fn_80330194`, `fn_80348A48`, `em033_prog`: the last is a merge with the registered `fn_8035E034`), 3 recuts/extensions (`light`, `eft053`, `cockpit`, which also takes unowned run 1), 2 unowned runs as units (`fn_8035FC18`, `fn_8036A828`) |
| cuts emitted | 17: **strong 9**, **medium 8** (`sinit-closure` 10, `registered-edge` 5, `pooldup-forced` 1 (now strong: `em033_prog_tbl` slot 0), `callers` 1); 0 `guess` |
| registered cuts removed (`removes_cuts`) | 18, all **strong** (a literal read on both sides; 0x8035E034 by `em033_prog_tbl` slots on both sides) |
| registered units absorbed / recut | 13 absorbed whole under another name (2 of them `Matching`); 9 recut (`ai/fn_802D44F4`, `ef/eft035`, `ef/eft052`, `ef/eft_slot`, `enemy/em_act_step`, `enemy/fn_802F5138`, `light/light`, `menu/fn_8031A6C0`, `stage/fn_802B2AA0`) |
| unowned functions covered / left open | 146 / 0 (as medium units; the composition of the two plain runs is itself an open question) |
| candidate cuts kept as `open_questions` (never applied) | 14, all `guess`: 4 inside proposal units, 10 rows in the file (9 pool intervals of 5..31 positions, 1 sinit interval) |

Grade rubric: the one of band `a` (`strong` = an exact invariant plus an independent soft interval that holds the cut and no
other known cut; `medium` = the exact invariant alone or one interval of at most 3 functions; `guess` = the rest).
Five of the eight `medium` cuts **restate a registered edge** (kind `registered-edge`: `hud/cockpit_quest`, the two unowned
runs' starts `fn_8035FC18` and `eft053`'s, `em020_prog`, `em019_ai`): a `pooldup` interval holds the edge, but it is 15..82
functions wide and usually holds other registered edges too, so the row proves only that an edge exists near there, and
`splitcheck`'s boundary record shows no literal on both sides. They are kept because dropping a registered edge for lack of a
narrow interval is what `merge_guess` would do; a reviewer should read them as "registry position, unverified".

## Findings that drive the cuts

* **Sinit closure (R1 of band `a`)**: the 13 `__sinit`s in the band. 7 have local constructors after them: `shell`'s closure
  ends at the registered unit end (no cut), the other six cut at `L` (`stage` `0x802B5640`, `light` `0x802BF278` and
  `0x802C2700`, `ai` `0x802D9EA4`, `eft035` `0x802F2238`, `eft_slot` `0x80348A48`; `callers.py` per row). 6 have an empty
  closure: two end at a registered edge (`pl_yure`, `em_action`), three cut at the sinit end (`0x802F9994`, `0x8031DAA8`,
  `0x80330194`) and one (`0x802FF234`) fails R1's second condition (callers before `L` in the unit) and is a `guess` row. Seen in asm: `shell_pool_static_init` passes `shell_work_construct` /
  `shell_chara_heap_construct` to `__construct_array`; both sit after it.
* **One pool literal read on both sides is a strong merge**: 17 registered cuts go (stage x2, camera x2, ai x6, hud x3, em020 x2,
  em019 x2). The literals are mostly the `4330000080000000` conversion constant and `0.0`/`1.0`, read by `lfd/lfs ...@sda21`
  in each of the units (`grep lbl_8079A748 build/RMHE08/asm/ai/*.s` shows four units). Consistency check: no value is held at
  two pool addresses inside 13 of the 17 proposal units, in particular none of the six merges; the four that do
  (`cockpit` 1, `eft035` 4, `fn_8031DAA8` 2, `em020_prog` 2) are exactly the ones with a `guess` interval inside.
* **Two `Matching` units fall inside merges**: `ai/fn_802D0DCC.c` (0x168 B, inside `ai_npc`, bracketed by reads of
  `lbl_8079A748` at 0x802CDACC and 0x802D28C4) and `enemy/em020_handlers.cpp` (reads `lbl_8079B8CC`, `lbl_8079BC64`,
  `lbl_8079B848`, all also read by `em020_prog`). They match standalone because the pool is not in their object; the evidence
  says they are not TUs. Nothing in this lane flips them; a lane that registers the merged unit must fold them.
* **Gap 1 is not a unit of its own**: the 61 unowned functions continue `ai/fn_802D44F4`'s tail (sinit closure 0x802D9EA4) up
  to `hud/layout` (0.5f held at 0x8079A878 and 0x8079A8C0 forces an edge in 0x802E03D4..0x802E0490; the registered edge
  0x802E0740 is kept, so `cockpit` = 0x802D9EA4..0x802E0740).
* **Gap 3's first three functions** are called only from `eft053` (`callers.py` 0x8036A690/0x8036A814/0x8036A824) and join it.

## Units

| unit | range | cut | grade | what |
| --- | --- | --- | --- | --- |
| `stage/stg_w` | 0x802AD9C0..0x802B5640 | 0x802AD9C0 | strong | stg_w + fn_802B2978 + head of fn_802B2AA0 (two removed cuts) |
| `camera/camera_main` | 0x802B5640..0x802BF278 | 0x802B5640 | strong | fn_802B2AA0 tail + camera + head of light (`camera_main` is a guess name) |
| `light/light` | 0x802BF278..0x802C2700 | 0x802BF278 | strong | middle of `light.cpp` |
| `ai/ai_npc` | 0x802C2700..0x802D9EA4 | 0x802C2700 | strong | 0x177A4 B: light tail + five `ai/*` units (one `Matching`), six removed cuts |
| `hud/cockpit` | 0x802D9EA4..0x802E0740 | 0x802D9EA4 | strong | tail of `fn_802D44F4` + gap 1; named from its own `__FILE__` string `cockpit.cpp` (0x805D55C8, one referrer inside the unit), no longer a guess |
| `hud/cockpit_quest` | 0x802E4978..0x802F2238 | 0x802E4978 | medium | three registered units + head of `eft035` (registered edge) |
| `ef/eft035` | 0x802F2238..0x802F5138 | 0x802F2238 | strong | rest of `eft035` |
| `enemy/fn_802F9994` | 0x802F9994..0x802FA9A0 | 0x802F9994 | strong | tail of `fn_802F5138`; the end 0x802FA9A0 has private callees across it (`fn_802FA964` -> `fn_802FA9A0`, `fn_802FAFB4`): a guess |
| `menu/fn_8031DAA8` | 0x8031DAA8..0x8031EA8C | 0x8031DAA8 | strong | tail of `fn_8031A6C0`; the end 0x8031EA8C is called into from `fn_8031E7F4` (guess) |
| `enemy/fn_80330194` | 0x80330194..0x8033041C | 0x80330194 | medium | tail of `em_act_step`; the end 0x8033041C is a dispatcher tail-branch target (`fn_803303E0`): a guess |
| `menu/fn_80348A48` | 0x80348A48..0x80349DD8 | 0x80348A48 | medium | tail of `eft_slot`; module `ef` had no support, renamed `menu` (it calls into `menu_item_page`); the end 0x80349DD8 is a guess |
| `enemy/em033_prog` | 0x8035BAB4..0x8035F2B4 | 0x8035BAB4 | strong | tail of `eft052` + the registered `fn_8035E034` (removed cut 0x8035E034, strong: slots of `em033_prog_tbl` on both sides); the start is slot 0 of `em033_prog_tbl`; the end is a guess in (0x8035E580, 0x8035F060] |
| `lobby/fn_8035FC18` | 0x8035FC18..0x80365C84 | 0x8035FC18 | medium | unowned run 2 (59 functions, one unit); the end 0x80365C84 is NOT an edge: six private callees (`fn_803642B8`, `fn_803645C4`, `fn_80364BD8`, `fn_80364EE8`, `fn_803653A0`, `fn_803659E8`) are called only from `lb_menu_page_step`, the last function of `lb_menu_page.cpp`; the tail joins it (phase 2) |
| `ef/eft053` | 0x80366618..0x8036A828 | 0x80366618 | medium | extended by 3 helpers (registered edge) |
| `menu/fn_8036A828` | 0x8036A828..0x8036CF64 | 0x8036A828 | medium | unowned run 3 (23 functions) |
| `enemy/em020_prog` | 0x8036CF64..0x80378F9C | 0x8036CF64 | medium | prog + handlers (`Matching`) + all of `em020_ai` |
| `enemy/em019_ai` | 0x80378F9C..0x80382310 | 0x80378F9C | medium | em019_ai + em019_prog + em_act_mot |

`fn_<addr>` names are placeholders. Data of the registered units a proposal touches is redistributed by reader (a monotone
assignment over each data run, cf. `assign` in the lane's scratch): it is not graded evidence and phase 2 replaces it; it is
only what keeps the candidate free of new failures.

## Top open questions (all `guess`; full list in the JSON)

1. `em020_prog`: two TUs (0.5f at 0x8079B8CC and 0x8079BC78); the edge is one of 6 starts in 0x803757E0..0x803759C4. The
   merged unit is the safe superset.
2. `lobby/fn_802FA9A0`: 31 positions (0x802FAFB4..0x802FBED4) for a first cut (four dup pairs), and the sinit tail after
   0x802FF180 (12 positions, `0x802FF234` is called from inside the unit).
3. `menu/fn_8031EA8C`: three TUs (magic constants at 0x8079AEF0 / AEF8 / AF28); 6, 27 and 23 positions.
4. `cockpit`: 5 positions (0x802E03D4..0x802E0490) for where `hud/layout`'s TU starts; `eft035`: 7 positions; `fn_8031DAA8`:
   6 positions.
5. `ef/eft_slot`, `ef/eft050`, `enemy/em_pl_frame`, `ef/fn_8030681C`, `lobby/fn_8030121C`: one cut each, 5..14 positions.
6. Unowned runs 2 and 3 contain no sinit and no pool entry; their callers show clusters (`fn_80363A5C` tree vs the helpers that
   only `lb_menu_page` calls; three menu-callback triples for run 3) but no edge.
7. `em019_ai`'s real end: five of its literals are also read by `enemy/fn_80382310` (readers to 0x80383148), so the TU continues
   past the registered edge 0x80382310 into band `e`'s ground; this unit stops at the edge to keep the files disjoint.

## Remaining splitcheck failures (rendered candidate)

Candidate 300 units (baseline 306), band alone; `order`, `coverage`, `text-cut`, `extab`, `dtors`, `vtable`, `bss`, `local-static`: 0 FAIL; `ctors` 39 -> 31 (the rest are
baseline units outside the band), `pool` 129 -> 111, `data-order` 3 and `jumptable` 1 (outside the band). **New failures outside the proposal units: 0**; lint: none.
Proposal units that still FAIL (5, all `pool`): `cockpit`, `eft035`, `fn_8031DAA8`, `em020_prog` (the `guess` interval inside each: a unit that holds one value at two addresses) and `em019_ai`
(the literals shared with `fn_80382310`, band `e`). The earlier `ctors` "closure false positive" on `stg_w`, `camera_main`, `light`, `ai_npc`, `cockpit_quest` is gone: the checker reads
the closure of the sinit's callees (`splitcheck.py --baseline --only ctors --unit X` prints the `detail` lines).

* Units the proposal does not touch keep their `pool` FAIL where the interval is a `guess` (`fn_802FA9A0`, `fn_8030121C`, `fn_8030681C`, `fn_8031EA8C`, `em_pl_frame`, `eft050`, `eft_slot`).

## Tool gaps hit

* **`splitcheck` `ctors`** used the sinit end; it now reads R1's closure (plus the callers-before-`L` condition) and the own vtable slots, also past the unit end.
* **`splitcheck` `pool` order** reports intra-function scheduling as "not text order" (4 units here).
* **No "keep the registered edge" grade**: a registered edge with a wide interval can only be `medium` (kept) or `guess`
  (which `merge_guess` would fold into the previous proposal unit). A `restates` marker on a cut would express it.
* **Data of a recut registered unit** has no place to go without a per-unit range list; the lane redistributed it by reader.
  `splitcheck` would need a `data-attach` pass, or the renderer a default (today the remainder stays with the old name, and
  `jumptable`/`bss` then fail outside the proposal).
* **`tudiscover`** needs the asm dump (not in a fresh worktree); the closure, the dup-interval intersection and the forced-merge
  components were computed from the DOL decode (`splitcheck`'s reference index) and `callers.py` only.
