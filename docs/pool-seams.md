# Literal pools as TU-seam evidence

MWCC emits **one literal pool per translation unit**, one entry per distinct value, and `mwldeppc` does not merge
pools across objects. So a pooled literal read by two registered units means the two are **one original TU that the
registry cut into pieces** - and, the converse, one value held at two pool addresses means **two TUs**. This is the
pool sibling of the `.data` V->S seams in `docs/data-order-seams.md`; it is playbook idea 94
(`docs/matching/094-pool-per-tu.md`). Measured 2026-09-30 on the tree at `2a7f633aa`.

## 1. The premise, verified

Compiler (`mwcceppc` Wii/1.3 with `cflags_base`; the core of it is the demo of idea 94):

* **One entry per distinct value, per TU.** `x*1.5f+2.5f`, `x*1.5f+3.5f`, `x*1.5f` and `x+1.5` (double) pool `2.5f 1.5f 3.5f`
  and `2.5 1.5`: each value once, however many functions read it. Floats and doubles are separate entries (different
  size); a double is 8-aligned (4 B of padding after three floats). `(float)int` adds `0x4330000080000000`, `(float)unsigned`
  adds `0x4330000000000000` - **in the TU's own pool**, not synthesised by the linker. Strings follow `-str reuse`
  (`"hello"` twice -> one `.sdata` entry).
* **Order is creation order, function by function in source order**, not textual order inside an expression
  (`x*1.5f+2.5f` pools `2.5f` before `1.5f`). Text order of the functions is source order, so a TU's pool runs in text
  order.
* **Where it lands** is the size threshold: default `-sdata2 8` puts floats and doubles in `.sdata2`; `-sdata2 4` keeps
  floats there and sends doubles to `.rodata`; `-sdata2 0` (REL flags) sends all to `.rodata`, strings to `.data`.
  Content and order do not change.

Linker: two TUs `fx: v*1.5f+2.5f` / `fy: v*1.5f+3.5f` (8 B `.sdata2` each) link to **16 B with two copies of `1.5f`**
(`402000003fc00000406000003fc00000`); the same two functions in one TU are **12 B**. Two TUs that both convert an int
link to two copies of the `0x4330000080000000` magic (24 B with a float between). mwld never merges a pool.

dtk side: a split object holds exactly the pool entries of the `.sdata2` range `splits.txt` claims for the unit;
everything else stays an **undefined `lbl_8079xxxx`** relocation target. `ef/fn_80101DF4` (claims its run) has
`.sdata2` 8 B and 2 `.text` relocations into it; `hud/cockpit_quest` and `hud/fn_802EBED8` (run unclaimed) have no
`.sdata2` and 103 / 125 relocations to undefined `lbl_8079xxxx`.

Real DOL (`orig/RMHE08/sys/main.dol`, map rows typed `float`/`double` in `.sdata2`):

* 7,081 literals, 1,862 distinct values; **567 values sit at more than one address (5,219 extra copies)** - the
  per-TU pooling the premise predicts (`0.0f` 232 copies, `1.0f` 200, the int->double magic 170).
* **Independent true-TU check.** 95 `__FILE__`-anchored TUs (one `.c`/`.cpp` each, `tudiscover` source anchors); 48 of
  them read pooled `.sdata2` literals (285 literals). **No value is held twice inside one TU**, although 270 of those 285
  values recur in some other TU's pool (so the check has power). No shared-literal label (221 unique-value ones, 5,217 with
  a copy elsewhere) spans two anchored files, and no `pooldup` interval lies wholly inside one anchored file.
* Units that hold one value at two pool addresses (so are already several TUs): **75 of 232** units that read a pooled
  literal. The rate rises with size: 1 of 14 units under 0x200 B of `.text`, 0 of 22 at 0x200-0x800, 7 of 40 at
  0x800-0x2000, 67 of 156 above. (`poolseams.py` prints the list.)
* The units whose compiled pool is byte-identical to the target's (`ef/fn_80101DF4`, with matching `.text` too, and
  `Network/NetworkSessionManagerPat`) are in no group: no contradiction among the verified matches (two is too few to be
  evidence alone).

## 2. Exceptions and what they cost

A literal address read by two registered units is not always one TU. Measured causes on the 745 shared addresses:

| class | count | handling |
| --- | --- | --- |
| named global/table/variable (`.sdata2` not 4/8 B, `.sdata` not a string, every `.data`/`.bss`/`.sbss`/`.rodata` object) | 233 multi-unit addresses (`.data` 92, `.bss` 68, `.sbss` 42, `.sdata` 18, `.rodata` 13) | never an edge: several TUs reference a global legitimately |
| a `const` global scalar (`extern const float k`) | not separable in the map (no scope on `lbl_`); only 6 `.sdata2` rows carry `scope:` | the group's **confidence** (text adjacency + first-use order + `.data` contiguity) is the guard; `--span-max` in `tudiscover` |
| int->float magic `0x4330...` | 23 shared edges | **not** an exception: the compiler pools it per TU (section 1), kept as an edge and counted |
| `.sdata` string (`lbl_807927C0/C4`, group 10) | the 18 non-string `.sdata` objects are the excluded ones above | a shared `data:string` is an edge; an initialised `char[]` is never pooled, so a repeated string is never a dedupe witness |
| untyped / `4byte` `.sdata2` word | - | can be half of an 8-byte object the map cut in two (two `0xFFFFFFFF` words read by one `lfd`: `g3d_light.cpp`, `ef_particle.cpp`): excluded from every dedupe rule |
| a unit that is itself several TUs (`coarse`) | 75 units, 16 of the 20 groups | chains neighbouring TUs into one component: demotes the group to `medium` |
| data-only member units (no `.text`) | 146 shared edges owned by 3 data-only units (`Pl/pl_*_data`) | the TU's own pool/data claim cut off the code: a member, not an exception |
| inline deferral / `-inline auto` copies | not observed | pool order follows emission order; 19 of 20 groups keep first-use order (the 20th, `Network`, has 1 inversion) |

## 3. Census (whole tree, `python tools/units/poolseams.py`)

* **20 groups over 100 of 302 registered units**, 745 shared literal addresses; largest 17, 10, 8, 7, 6, 6, 6 units; 16
  groups are text-adjacent, 4 are **interleaved** (a registered unit lies inside the span and shares no literal: a TU's
  `.text` is contiguous, so it is part of the fold too - `ai/fn_802D0DCC`, `Pl/pl_master`, `sound/fn_800DCFEC`, which
  have matched `.text`, and the three `Network` units of group 11, which do not).
* Confidence: **high 4, medium 15, low 1** (133 / 584 / 28 shared literals). Medium is almost always "a coarse unit is in it".
* **The model and the `.data` model disagree in exactly one group**: `Network/NetworkPeerMcs + NetworkSessionStable +
  fn_803D3CE8 + network_shared_data` (28 shared literals, 1 first-use inversion, its `.data` claims interleave three other
  units' `.data`). Everything else has a contiguous `.data` (or none claimed).
* **The cockpit group reproduces** (group 10): `menu/fn_802E4978 + hud/cockpit_quest + hud/fn_802EBED8 + ef/eft035`,
  0x802E4978-0x802F5138, 18 shared literals (`.sdata2` and the `.sdata` strings `lbl_807927C0/C4`), text adjacent, first-use
  order holds. `ef/eft035` is itself *coarse* (it holds a value at two pool addresses): its tail is a different TU,
  so the fold is `fn_802E4978` .. the start of `eft035`'s own TU, not necessarily all of `eft035`.
  `tudiscover at 802E4978` puts the TU start in `0x802E4978..0x802E5764` (`pooldup`: `1.0f` at `lbl_8079A8D8` then again at
  `lbl_8079A8FC`).
* Other folds worth a lane: `Pl/fn_802430E8 .. Pl/pl_act_data` (8 units, high), `enemy/fn_801251D0 .. fn_80137604` (6,
  high), the 17-unit `enemy/fn_80170600 .. fn_801A9540` chain (a coarse-unit chain: fold in pieces), `Pl/fn_802693C4 ..
  fn_8027D684` (idea 58's `0x8079A008` case; `Pl/pl_master` lies inside it).

## 4. What each tool says now

* `poolseams.py [--unit U] [--json F]` / `datagap.py --pool-seams`: the census above; `--unit` prints one group.
* `tudiscover.py at <addr>`: a `pool model` block (registered units the range overlaps + their fold candidate) and a
  `pool dedupe` block (narrow intervals where a value repeats = a new TU starts there). `--pool-model off|on|strong`:
  `on` (default) makes a shared pooled literal a must-link even with a unique value and adds `pooldup` as a soft vote;
  `strong` makes `pooldup` a strong pin. Bench tier 4: `pooldup` precision 0.240 (29 hit / 92 miss vs `splits.txt`; a miss
  is inside a unit that is itself several TUs), claimed unit starts pinned 157 -> 162. The older `pool` (adjacent-label run
  jump) kind is 0.104 - **noise**, left as is (an ordinary TU has disjoint ordered referrers for adjacent literals).
* `datagap.py` strict deferrals (`pool-synth`, `isolated-run`, `span-blocked`, `ambiguous-owner`): the reason now ends with
  `candidate fold: ... - the claim is blocked by a seam, not by the tool` when the unit is in a group (verdict and class
  unchanged).
* `flipcheck.py` / `sectiongap.py`: a differing `.sdata2`/`.sdata` of a unit in a group reads "our object's pool is a
  partial pool of a TU that spans several registered units (candidate fold: ...)".
* `brief.py`: a pool unit that reads a literal a registered unit already touches carries
  `pool_seams` and the brief opens with a **Pool evidence** note naming the units.

## 5. Status and open items

* Done: census, `tudiscover` pool model, deferral/flipcheck/sectiongap/brief notes, idea 94, fixtures for each.
* Not done: applying a fold (registering the four cockpit units as one); a function-level `must-link` from a unit-level
  group (`tudiscover` only links readers within `--span-max`, so `fn_802E4978`'s MATCH SET stays one function - the group
  line carries the answer instead); cross-checking `pooldup` intervals against `Matching` objects once more pools match.
* Caveat: the map gives one `lbl_` per dtk-cut word, so a `4byte`/untyped pair can be one object; typed
  `float`/`double` rows are the only dedupe witnesses.
