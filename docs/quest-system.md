# The quest system: the load-to-spawn path, and the match-type filter

**What this is.** The promoted conclusions of the quest recon (`quest/recon`, lane `worker/recon-d068`). It is a
*recon* document: it registers nothing, changes no source, and every claim here carries an address so the next
lane can check it in one read. The evidence-classed source - 688 lines, every claim tagged with its class - is
**`.pi/notes/quest-inventory.md`**; this file is its conclusions, and where the two disagree the note wins.

**Evidence classes, as the note defines them.** `[bytes]` = the split disassembly (`build/RMHE08/asm/**`) and/or
the target object; `[xref]` = the whole-DOL caller index; `[tool]` = `tudiscover.py` / `dumpmap.py` /
`symedit.py`; `[map]` = `config/RMHE08/symbols.txt`; `[splits]` = `config/RMHE08/splits.txt`; `[dump-map]` = the
read-only Dolphin map next to the Ghidra dump. **`GUESS`** marks a name or a meaning the lane derived; nothing
else here is a guess.

**Two traps this band sets, both measured.**

* **The `asm/` dump is stale relative to the map.** A `bl` whose callee has since been renamed still prints the
  *old* label (`grep "bl quest_element_build"` finds nothing; the dump says `fn_803AD008`). Resolve a callee by
  **address** (asm label -> address -> the current `[map]` row), never by grepping the dump for a new name, and
  use **`tools/units/callers.py <address|name>`** for "who calls / who reads this" - it is address-keyed and
  builds its graph from the current map. (The recon lane hand-rolled the same index in 30 lines before
  `callers.py` existed; the tool is the family answer now - roadmap 7.34.)
* **`.sdata2` is shared across objects by the linker, so a shared `.sdata2` label is not evidence of a TU.**
  See §5.3 - it invalidates one of `tudiscover.py`'s own "strong" evidence kinds for this project.

---

## 1. The load-to-spawn path, in order

### 1.1 The file table (data, not code)

`lbl_8058AFC8`, `.data` **0x8058AFC8..0x8058AFE8** (0x20 B: four rows of `{u32 size; char *name}`) `[bytes]`:

| row | +0x00 size | +0x04 name | what it is |
| --- | --- | --- | --- |
| 0 | 0x0003A540 | 0x8058AF98 = `"05/quest00.bin"` | hunt list, low tier |
| 1 | 0x00064180 | 0x8058AFA8 = `"05/quest01.bin"` | hunt list, high tier |
| 2 | 0x00000000 | 0x807910F8 (an 8-byte `.sdata` object) | the "no file" variant (size 0) |
| 3 | 0x00003BC0 | 0x8058AFB8 = `"05/quest03.bin"` | the other list (VS/arena - §4.1) |

`[xref]` the table is referenced from exactly two functions, `fn_803AD200` and `fn_803AD6E0` - it is **private to
the quest loader**. Row 1's size (0x64180 = 410,496 B) is exactly the loader's file buffer (0x64400 alloc minus
its 0x280 header), which is how the buffer's internal split was identified.

### 1.2 `quest_init(u8 kind)` - 0x803AD47C, 0x264 B `[bytes]`

The band's only map-named entry point (`[map]` `quest_init__FUc`; `[dump-map]` `quest_init(unsigned`). Body:

1. `kind == 0` -> `bl fn_803AD200` (0x803AD49C), else `bl fn_803AD6E0` (0x803AD4A4).
2. `quest_work_ptr = &quest_work` (0x806C5858) (0x803AD4B0), then `memset(quest_work, 0, 0x6AB8)` (0x803AD4BC).
3. `quest_work->0x6758 = 0x19`, then eight `quest_work->0x6698[i] = work_mem_alloc(0x19 * 2)` - eight stacks of
   0x19 `u16` quest ids (0x803AD4C8..0x803AD50C).
4. `quest_work->0x66B8[0..7] = 0` - the eight counts.
5. `quest_work->0x03C = work_mem_alloc(0x3000)` - the current quest result record.
6. Walk `quest_list_items[0..quest_list_count)`: per entry read `u8 kind = item+0x8A` and `u16 id = item+0x2C`,
   scan the 2x8 `u16` threshold table `lbl_805F2A98` (private; `[xref]` sole referrer `quest_init`), then
   `quest_work->0x6698[kind][count[kind]++] = id`.
   **Residual:** the scan's row counter is dead in the shipped build (the store does not depend on it) - measured,
   recorded, not guessed.
7. `memset(quest_work + 0x34C, 0, 6 * 0x30)`.
8. `memcpy(quest_work + 0x698, get_userdata(), 0x6000)` - the save-data mirror.
9. `fn_8004E710(quest_work + 0x6AA4, quest_work + 0x45C0)`, the three `f64` timer stores to `+0x6A48`/`+0x6A50`/
   `+0x6A58` (from `lbl_8079C510`/`lbl_8079C518`), and `0xFF` to `+0x309`/`+0x30A`/`+0x30B`. Returns
   `quest_work_ptr`.

**Call sites.** The recon dossier's prose says **five**; its own `[xref]` table lists **six**, and this document
does not resolve the difference - it quotes the table:

| call site | caller | argument | gate around the call |
| --- | --- | --- | --- |
| 0x803A140C | `fn_803A13B4` (`menu/multi_result.cpp`) | 0 | none - the result->next-quest machine's first state |
| 0x803A1528 | `fn_803A13B4` | 0 | `lobby_w->0xB0 == 3` |
| 0x80447300 | `fn_80446EE8` (unregistered band) | 0 | `fn_803BE8B8(0)` before it |
| 0x8021F6F4 | `fn_8021F5C8` (`lobby/fn_8021E1EC.cpp`) | 0 | `GameMode_set(2)`, `all_reset()` |
| 0x8028B994 | `fn_8028B524` (`Pl/fn_80288CEC.cpp`) | 0 | `PlayMode_ck() == 3 \|\| == 6 \|\| == 2` |
| 0x8028BC68 | `fn_8028BAA8` (`Pl/fn_80288CEC.cpp`) | 1 | `PlayMode_ck() == 3 \|\| == 6 \|\| == 2` |

Only the two `Pl/fn_80288CEC.cpp` sites take the result (`stw r3, 0xDC(r31)` - the `QuestWork *` into the
move-work root at `+0xDC`), and only they gate on `PlayMode` (§3.1).

### 1.3 The two loaders

**`fn_803AD200`, 0x803AD200, 0x27C B** - the `quest_init(0)` half `[bytes]`:

```
p = work_mem_alloc(0x64400);                       // 0x803AD218
quest_list_base   (0x80794C20) = p;                // the allocation start
quest_list_values (0x80794C24) = p + 0x1A0;        // u16 values[]
quest_list_file   (0x80794C1C) = p + 0x280;        // the file image
memset(p, 0, 0x64400);
if (fn_8044FB98() == 1)                            // a quest is already resident
    row = (system_w->0x7D8 >= 0x2710) ? &tbl[1] : &tbl[0];              // THE ID TIER GATE
else
    row = (PlayMode_ck() == 4 || fn_8042CB9C() == 1) ? &tbl[1] : &tbl[0];  // PlayMode / NET GATE
cnvt_eur_fname(buf, row->name);
load_file(buf, quest_list_file, row->size);
count = *((u32 *)quest_list_file + 1);             // header {u32, u32 count}
for (i = 0; i < count; i++) {                      // 8-byte records from file + 0x8
    quest_list_items[i]  = (u8 *)quest_list_file + rec->u32_00;
    quest_list_values[i] = rec->u16_06;
}
quest_list_items_ptr (0x80794C3C) = quest_list_base;
quest_list_count     (0x80794C44) = count;
```

**The tier gate is on the quest ID, not on hunter rank.** `system_w->0x7D8` is the ID last written by
`fn_803A13B4` at 0x803A15F8 (zeroed by `fn_803A12D4` 0x803A1324 and by `GalleryDemo_task` 0x8044FE1C), and the
only comparison of it is at 0x803AD254. The ID itself is a `u16` read from `lb_param_w + 0x00`; the evidence
that it *is* the quest ID is the `[0xEA60,0xEA6B]` test on the same field (§3.2) plus the arena's own
`arena_time_table[12]`.

**`fn_803AD6E0`, 0x803AD6E0, 0x214 B** - the `quest_init(kind != 0)` half: byte-for-byte the same parse, but the
table row is fixed at **index 3** (`05/quest03.bin`, 0x3BC0 B) and there is **no gate at all**.

`[xref]` both loaders have exactly one caller (`quest_init`) and `lbl_8058AFC8` exactly two.

**The quest file format, proved by the parse loop** `[bytes]`:

```
struct QuestFileHeader { /* +0x00 */ u32 magic_or_zero; /* +0x04 */ u32 count; };
struct QuestFileRecord { /* +0x00 */ u32 ofs_into_file; /* +0x04 */ u16 unused; /* +0x06 */ u16 id; };
// `quest_list_items[i] = file_image + rec.ofs`; the row at that offset carries
//   u8 kind at +0x8A and u16 id at +0x2C - so a quest row is at least 0x8C bytes.
```

`GUESS`: `kind` is a category index into the eight stacks at `+0x6698`; only its *use* as an index is proved.

### 1.4 The handoff - `GameModeExec()`, 0x80041978, 0x94 B

Owner `mh3_pad.cpp` (`[splits]` `.text` 0x800408A8..0x80047398 `[bytes]`):

```
switch (GameMode_get()) {                          // fn_800CF208 -> system_w+0x24
case 1:  system_w->0x868 = 0;
         Tsk_Change(fn_8028BF1C, 4);               // 0x800419BC   <-- THE HANDOFF
         break;
case 2:  Tsk_Change(system_w->0x7D3 == 1 ? fn_803A13B4 : fn_8021F3A8, 4);
         break;
default: break;
}
```

`Tsk_Change__FPvs` is 0x800418CC (0x60 B, same unit). Its neighbours are the complete "which match" fan-out:

| entry | address | task installed |
| --- | --- | --- |
| `GameModeExec()` | 0x80041978 | GameMode 1 -> `fn_8028BF1C` (the field); GameMode 2 -> `fn_803A13B4` / `fn_8021F3A8` |
| `fn_80041A0C` | 0x80041A0C | `fn_8028DDCC` (`Pl`) |
| `VsGameModeExec()` | 0x80041A1C | `fn_8028E528` -> `fn_8028BAA8` -> `quest_init(1)` |
| `TestModeExec()` | 0x80041A2C | nothing (a 4-byte `blr`) |
| `ArenaSelExec()` | 0x80041A34 | `fn_80040CA8()`, then the arena task `fn_804463C4` (§5) |

From the `Tsk_Change(fn_8028BF1C, 4)` instruction on, the field task **owns the game and calls `quest_init(0)`
itself**: `fn_8028BF1C` (0x8028BF1C, 0x24C B) reads `get_move_work_adrs(0)`, calls `fn_800D2F1C()` and
`nwMoveStart__Fv()`, branches on `fn_8042CB9C()` and `move_work->0x00`/`+0x01`, and calls `fn_8028B524` /
`fn_8028C6CC` - and `fn_8028B524` is the function that calls `quest_init(0)` and stores the `QuestWork *` at
`move_work->0xDC` (0x8028B998). `[xref]` `fn_8028BF1C` has no direct caller besides the task switch, so it is
a jump-table task entry. `GameModeExec`/`VsGameModeExec`/`ArenaSelExec`/`TestModeExec`/`Tsk_Change` are
`[dump-map]` names (real); the `fn_` entries are not yet named (rule 7 debt, no rename was made).

---

## 2. The shape of the answer

Three independent filters sit on this path, in execution order:

1. **`PlayMode`** (`system_w+0x25`) - `{2,3,6}` selects the one shared `QuestWork`; any other value gives a
   **per-player 0x6AB8 block**. This is the single-vs-multi filter, with `fn_8042CB9C()` (`net_ctrl_wk->0x11
   == 7`) as the second input. (§3.1)
2. **The quest-ID range `[0xEA60, 0xEA6B]`** - twelve arena quests - picks `ArenaSelExec()` over
   `GameModeExec()`. (§3.2)
3. **The session-kind index, 0..8, passed to `create_move_work(long)`** - selects one of nine rows of the 9x7
   `u16` table at `.data 0x80595118`, i.e. which object pools the match configures. (§3.3)

---

## 3. The match-type filter, in detail

### 3.1 Layer 1 - `PlayMode` (`system_w+0x25`)

`PlayMode_ck()` is 0x800CF218 (`lbz r3, 0x25(system_w)`); `PlayMode_set()` is 0x800CF254 and rejects `>= 7`.
`system_w` is `.bss` 0x806585E0, 0xA5C B `[map]`. The readers and their constants `[bytes]`: `==2` 39 sites
(mostly `sound/fn_800F2A94.cpp`, the whole `Pl/fn_80288CEC.cpp` scene half, `stage/stg_w.s`); `==3` 21 sites
(the same two files plus `lobby`, `menu/menu_result.cpp`); `==4` three sites, one of them the **file-selection
gate** in `fn_803AD200` (0x803AD278); `==6` 11 sites, all `Pl/fn_80288CEC.cpp` and `fn_8044FF18`; `==1` one site
in `ef/fn_800CDB2C.cpp`.

The decisive reader is `fn_8028B524` (0x8028B524, 0x584 B, `Pl/fn_80288CEC.cpp`), with `fn_8028BAA8`
(0x8028BAA8, 0x24C B) the same shape ending in `quest_init(1)`:

```
/* fn_8028B524, on its own u8 argument */
if (arg == 0 || arg == 2) {
    if (fn_8042CB9C() == 1)          kind = 0;      /* net_ctrl_wk->0x11 == 7 */
    else if (PlayMode_ck() == 6)     kind = 7;
    else                             kind = 8;
} else {
    if (PlayMode_ck() == 3 || PlayMode_ck() == 6)  kind = 4;
    else if (fn_8042CB9C() == 1)                   kind = 0;
    else if (PlayMode_ck() == 6)                   kind = 7;
    else                                           kind = 8;
}
create_move_work(kind);                            // 0x8028B608
...
if (PlayMode_ck() == 3 || PlayMode_ck() == 6 || PlayMode_ck() == 2)
    move_work->0xDC = quest_init(0);               // 0x8028B994 - ALWAYS the global QuestWork
else {                                             // 0x8028B9A0..0x8028B9B8
    move_work->0xDC = work_mem_alloc(0x6AB8);      // a PRIVATE per-player QuestWork
    memset(move_work->0xDC, 0, 0x6AB8);
}
```

**So the rule is: `PlayMode ∈ {2,3,6}` -> one shared, globally visible `QuestWork` (the single-instance case);
any other `PlayMode` -> a per-player 0x6AB8 block.** `fn_8042CB9C()` is 0x18 B inside `fn_80429B94.cpp` and
reads a `.sdata` global (`net_ctrl_wk`); `fn_8028BAA8` additionally does `create_move_work(0)` (0x8028BADC) and
`set_move_work_max(2, system_w->0x8AF == 0 ? 1 : 2)` / `set_max_player__(that)`.

*What is **not** proved:* what the game calls PlayMode 2/3/6. The note says so explicitly - the writers list and
the `lbl_80595118` table are the strongest available triangulation, and "single vs multi" is the reading they
support, not a name from a string or a dump.

Writers of `PlayMode`, `[xref]`, in full: `lobby/fn_801F9CD4.cpp` `fn_801FB80C` 0x801FB8D4 = **4** (right after
`create_move_work(1)`); `lobby/fn_8021E1EC.cpp` 0x8021F504 = **3**, 0x8021F5A0 = **6**; `menu/multi_result.cpp`
`fn_803A13B4` 0x803A162C = **6** (arena) and 0x803A1648 = **6** (hunt); `quest/quest_entry.cpp` `fn_803ADA70`
0x803ADC20 = **3** (only when `PlayMode_ck() == 6`); the arena task `fn_804463C4` 0x80446680 = **6**,
0x8044677C = **2**, 0x8044696C = **4**.

### 3.2 Layer 2 - the quest-ID range, in `fn_803A13B4` (0x803A13B4, 0x2CC B, `menu/multi_result.cpp`)

This is the arena-vs-normal-hunt decision, and it is a plain ID range `[bytes]`:

```
q = lb_param_w->u16_0x00;                          /* THE SELECTED QUEST ID */
system_w->0x7D8 = q;                               /* 0x803A15F8 - sticky, read by fn_803AD200 */
if ((u16)(q + 0x15A0) <= 0xB) {                    /* 0x803A1618 : q in [0xEA60, 0xEA6B] */
    system_w->0x90F = 1;
    PlayMode_set(6); GameMode_set(3);
    ArenaSelExec__Fv();                            /* 0x803A1638  <-- ARENA */
} else {
    PlayMode_set(6); GameMode_set(1);
    GameModeExec__Fv();                            /* 0x803A1654  <-- NORMAL HUNT */
}
```

The twelve IDs **0xEA60..0xEA6B (60000..60011) are the arena quests**, cross-confirmed by `menu.h`'s own
`arena_time_table[]` (`.data` 0x805F7AF8, twelve pointers to `u16` time tables).

### 3.3 Layer 3 - `create_move_work(long)`, 0x800CFB2C, 0x74 B (`ef/fn_800CDB2C.cpp`)

`[bytes]`: for `kind < 9`, `fn_800CF8EC()` allocates/resets a 0x2C-byte block at `system_w+0xA4`; then for kind
`k`, every non-zero `u16 tbl[k*7 + i]` read from `.data` **0x80595118** becomes
`fn_800CF948(i, value)` (0x800CF948, 0x148 B), which allocates `value * slot_size[i]` bytes into
`move_work->0x10 + i*4` and records the count at `move_work->0x00 + i*2`.

Slot (`[bytes]`, `fn_800CF948`'s switch): 0 = 0x22E8 (the player/move-work root, `get_move_work_adrs(0)`;
matches `move_work->0x22E4`), 1 = 0x24, 2 = special (`fn_80267548()` allocates it; the value is stored as the
count - the PL scene object), 3 = 0xB18, 4 = 0x48, 5 = 0x168, 6 = 0x41.

The 9x7 table `lbl_80595118` (`.data` 0x80595118..0x80595198, 0x80 B reserved) and its complete `[xref]` set:

| kind | slot0 | slot1 | slot2 | slot3 | slot4 | slot5 | slot6 | caller |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 0 | 1 | 18 | 4 | 32 | 128 | 256 | 512 | `fn_8028B524` (offline/net) |
| 1 | 0 | 18 | 4 | 0 | 128 | 256 | 512 | `lobby/fn_801F9CD4.cpp` 0x801FB8DC |
| 2 | 0 | 18 | 1 | 0 | 96 | 60 | 512 | `lobby/fn_801F9CD4.cpp` 0x801FB5CC |
| 3 | 0 | 18 | 4 | 32 | 32 | 60 | 512 | (not called in this build) |
| 4 | 1 | 18 | 1 | 32 | 128 | 256 | 512 | `fn_8028B524` (PlayMode 3/6) |
| 5 | 0 | 18 | 8 | 0 | 128 | 256 | 512 | arena task `fn_804463C4` 0x80446518 |
| 6 | 0 | 0 | 3 | 0 | 0 | 0 | 0 | `charmake_init` 0x80443EAC |
| 7 | 1 | 18 | 1 | 32 | 128 | 256 | 512 | `fn_8028B524` (PlayMode 6 fallback) |
| 8 | 1 | 18 | 2 | 32 | 128 | 256 | 512 | `fn_8028B524` (last fallback) |

`GUESS` (the note's own marker): kinds 0/4/7/8 are the *field* kinds (slot 0 = 1 player root) and 1/2/5/6 the
*menu/lobby* kinds (slot 0 = 0) - that is the only reading the table supports, and no string or dump name
confirms it. The six call sites above are the whole `create_move_work` `[xref]` set.

### 3.4 Secondary, on the same band (not match-type filters)

* **`quest_flag_*_ck(QuestRecord *)`** - eight functions, 0x78-0x84 B, at **0x803B4BEC..0x803B5120** (inside
  `enemy/em_pop.cpp`'s registered range, but quest predicates). Each is: `fn_800CF280() == 1 -> 0` (that is
  `move_work->0x113 == 1`), `rec == NULL -> quest_record_get()`, then `(rec->0x310 & <one bit>) != 0`. Bits
  `[bytes]` (MSB-numbered): `0x80000`, `0x100`, `0x10`, `0x800000`, `0x4000000`, `0x2000000`, `0x80000000`,
  `0x100000`. `quest_record_get()` (0x803B3394, 0x1C B) is `quest_work_ptr->0x03C`. These are the "is condition
  X of the accepted quest satisfied" gates; biggest consumers are the quest board
  (`lobby/lb_quest_screen.cpp`), the results (`menu/menu_result.cpp`, `menu/arena_result.cpp`) and enemy logic.
* **The arena element gates** - `quest_arena_need_get` (0x803B6800, 0xF0 B) and `quest_arena_count_get`
  (0x803B68F0, 0xA8 B), both map-named though the stale dump still labels them `fn_*`. They read
  `quest_work + index*0x60` (`QuestElement`): `+0x94` flags, `+0x98`/`+0x9A` a `u16` pair, and a byte at
  `quest_work+0x6803`.

---

## 4. The data structures the path lives in

### 4.1 `QuestWork` - `.bss` 0x806C5858, size **0x6AB8** `[bytes: quest_init's memset length]`

`quest_work_ptr` (`.sbss` 0x80794C40) holds its address. `include/unsplit/menu.h` already documents most of it;
these are the additions from this pass (every one read out of `quest_init` / `fn_803ADA70` / `fn_803AF4B4`):

| offset | type | name (proposed) | evidence |
| --- | --- | --- | --- |
| +0x0308 | u8 | result kind | written 1/2 by `fn_803ADA70` 0x803ADB20/48/7C |
| +0x0309..+0x030B | u8 x3 | per-player slot states | `quest_init` 0x803AD6B0..0x803AD6C4 sets 0xFF |
| +0x034C | 6 x 0x30 | result row scratch | `quest_init` memset 0x803AD64C |
| +0x03FC? | 0x40 | quest element payload | `fn_803AD008` memsets `+0x3F4` (0x40 B) - base differs by 8 from the 0x34C block; **unreconciled** |
| +0x045C0 | ? | quest-condition sub-block | `fn_8004E710(&0x6AA4, &0x45C0)` |
| +0x0698 | u8[0x6000] | save-data mirror | `memcpy(..., get_userdata(), 0x6000)` 0x803AD680 |
| +0x6698 | u16 *[8] | per-kind quest-id stacks | eight `work_mem_alloc(0x19*2)`, 0x803AD4E4 |
| +0x66B8 | s32[8] | those stacks' counts | zeroed 0x803AD514..0x803AD550, incremented 0x803AD630 |
| +0x6758 | s32 | stack capacity = 0x19 | `li r0,0x19; stw` 0x803AD4C0 |
| +0x675C | s32 | "players have entered" flag | `fn_803ADA70` 0x803ADAC4 |
| +0x6760 | u8[8] | per-player enter flags | `fn_803ADA70` 0x803ADBB4 |
| +0x6803 | u8 | arena element sub-flag | `fn_803B6800` reads `0x6803(r3)` |
| +0x6977 | u8 | screen active | `fn_803ADA70` 0x803ADB18 |
| +0x69A5 | u8 | player count for the entry send | `fn_803ADA70` 0x803ADBE8 |
| +0x6A48/+0x6A50/+0x6A58 | f64 x3 | timers | `lfd` from `lbl_8079C510`/`lbl_8079C518`, 0x803AD694..A8 |
| +0x6A3A..+0x6A40 | u8 x7 | entry-send header bytes | `lb_entry_start_send` 0x80339B54..0x80339B9C |
| +0x6A68 | u8 | last sent entry id | `lb_entry_start_send` 0x80339BB8 |
| +0x6A8C/+0x6A90? | u32 x2 | handover payload | `lb_entry_handover_send` reads `+0x684`/`+0x688` of `*((void **)(move_work+0xDC))`; **unreconciled base** |
| +0x6AA4 | 0x10 | arena-select record | `fn_803AF4B4`: byte 0 = "selected", `+0x6AB0` = the chosen quest id (compared with `lb_param_w->0x00` at 0x803AF548) |

**`quest_work+0x6AB0` caches the accepted quest ID** - `fn_803AF4B4` 0x803AF548 compares it with
`lb_param_w->0x00` and memsets the 0x10-byte `+0x6AA4` block when they differ.

### 4.2 The quest-list allocation - one anonymous 0x64400 B block behind four `.sbss` words

`[bytes]` `fn_803AD200` 0x803AD218..2C and `fn_803AD6E0` 0x803AD6F8..0C:

| address | name | what it points at |
| --- | --- | --- |
| base + 0x000 -> 0x80794C20 | (unnamed) | the allocation start; also `quest_list_items_ptr` |
| base + 0x000 -> 0x80794C3C | `quest_list_items` (menu.h) | `u32 *items[104]` (0x1A0 B) |
| base + 0x1A0 -> 0x80794C24 | `quest_list_values` (menu.h) | `u16 values[112]` (0xE0 B) |
| base + 0x280 -> 0x80794C1C | (unnamed) | the loaded file image (0x64180 B = quest01.bin's size) |
| 0x80794C44 | `quest_list_count` (menu.h) | the header's count |

`[xref]` 0x80794C1C has 24 references, **all** inside the two loaders; 0x80794C20 has 6, the same two. Both
words are private to the loader and are **rule-7 naming debt** (`lbl_80794C1C` -> `quest_list_file_image`,
`lbl_80794C20` -> `quest_list_alloc_base` are the note's proposed names, marked `GUESS`).

### 4.3 `MoveWork` - `system_w+0xA4`, 0x2C B `[bytes: fn_800CF8EC allocs 0x2C]`

`+0x00` is `u16[7]` (read by `get_move_work_max` as `lhzx`), `+0x10` is `void *[7]` (read by
`get_move_work_adrs` as `lwz 0x10(base + i*4)`). `get_move_work_adrs(i)` returns 0 for `i >= 7` or when
`system_w+0xA4 == 0`. Slot 0's record (0x22E8 B) is the player root; offsets on this path:

| offset | evidence |
| --- | --- |
| +0x00, +0x01 | `fn_8028BF1C` 0x8028BF68/84 (task-entry state) |
| +0xDC | the `QuestWork *` - written by `fn_8028B524` 0x8028B998, read by `lb_entry_handover_send` 0x80338E3C |
| +0xE9 | the **quest phase byte** - `fn_803AD8F4` returns it, 20+ call sites switch on it (sound BGM at 0x800F3498/0x800F36C4, `draw_shape.cpp` 0x800554B0) |
| +0x113 | `move_work_state_ck` (0x800CF280) reads `lbz 0x113` and returns `== 1` |
| +0x22E4 | `fn_8044FB98` reads `lbz 0x22E4` = "a quest is resident" - the input to §1.3's tier gate |

### 4.4 `system_w` (`.bss` 0x806585E0, 0xA5C B) and `lb_param_w` (`.bss` 0x806590B4, 0x9C B, `data:2byte`)

`system_w`:`+0x24` u8 GameMode (`fn_800CF208`; `GameMode_set` rejects `>= 4`); `+0x25` u8 PlayMode (§3.1);
`+0x7D3` u8 (`GameModeExec` 0x800419CC); `+0x7D8` u32 **the quest ID** (written 0x803A15F8, read 0x803AD250);
`+0x868` u8 (`GameModeExec` 0x800419AC); `+0x8AF` u8 (`fn_8028BAA8` 0x8028BAE0 - selects 1 vs 2 players);
`+0x90F` u8 (the arena marker, `fn_803A13B4` 0x803A1624). `lb_param_w`:`+0x00` u16 **the selected quest ID**
(0x803A15EC `lhz`); `+0x09` u8 (`fn_803A13B4` 0x803A160C, copied from `lobby_w+0x162`). `fn_803A12D4` memsets
`lb_param_w` (0x9C) and `lobby_w` (0x17C) to zero before the result screen runs.

### 4.5 A naming trap worth recording

`[xref]` `fn_803AD8F4` (0x803AD8F4, 0x38 B) is called from `sound/fn_800F2A94.cpp` (ten sites) and
`draw_shape.cpp` (0x800554B0): it is a *global game-state getter* (`get_move_work_adrs(0) ? move_work->0xE9 :
0`), **not** a quest-local helper. A `fn_` name there misleads a future reader into thinking the quest band owns
it. The note proposes `quest_phase_get` (`GUESS`).

---

## 5. The arena task - the registered unit, and its open cut

### 5.1 The registration (unlanded at the time of writing)

`quest/arenatask.cpp` is registered on branch **`worker/arena-task-92fd`** (tree `mhtri-dtk.slot3`, claim
`quest/arena_task`), **not in `main`'s `splits.txt`** - verified 2026-09-28: `grep arenatask
config/RMHE08/splits.txt` is empty in `main`, in this lane's tree and in slots 1/2/4/5/6, and present in slot 3.
Its registration: `.text` **0x804459E4..0x80448404** (19 functions, 10,784 B), extab 0x8001DE9C..0x8001DF24,
extabindex 0x8003EB20..0x8003EBEC, `.ctors` 0x8056F3D0..0x8056F3D4. Durable evidence:
`.pi/notes/arena-task-92fd.md` and `.pi/outbox/arena-task-92fd.json`.

**Class-1 evidence for the TU.** `.data` 0x80607390 is the bare `__FILE__` string `"arenatask.cpp"`; it has
exactly one copy in the DOL and its **only referrer is 0x80445B3C, inside this range's head
`arena_resource_load` (0x804459E4)** - so that function and this band are one translation unit. That is the
`__FILE__`-with-one-copy test (playbook 54) the *quest dossier itself* could not pass anywhere in
0x8038E8E8..0x803C4BA0 (`[bytes]`, 110 candidate strings scanned, none referenced from that region) - which is
why every name in §1-§4 is class 3/4 and marked.

**The seam at both edges, and the open internal cut.** The left edge 0x804459E4 starts the band's own `.data`
(0x80607210) and `.sdata` (0x80793B28) runs, and the 8-byte accessor `fn_804459DC` before it reads
`arena_lsp_data_adrs`, whose neighbour `que_info` this band's `arena_quest_info_build` (0x80445B80) fills - so
0x804459DC is a candidate *first* function and 0x804459E4 the candidate second. The right edge 0x80448404 begins
the save-file module (nine functions sharing the private `.bss` path buffer `lbl_806E40C0`), and the function
before it (`arena_camera_light_vec_init`, 0x80448358) writes this band's `.bss` vectors - so the cut is exactly
there. **The internal cut at 0x80446990 is unproven**: under it, the band's `.data` per-function run
(0x806073B8 `arena_player_init`'s float array, 0x806073E0 `arena_light_init`'s GXColor quad) and its `.bss` run
(0x806E4078 camera pair, 0x806E4090 light quad) tile a *second* object exactly; under the registered (wider)
extent they are this unit's tail. The data alone cannot separate the two readings, and the lane registered the
maximal run - which is also what the flanking registrations do. A re-draw is a `splits.txt` edit plus the
extab/extabindex split points.

### 5.2 The arena task's own id space vs the selection gate

The arena lane measured, in `arena_quest_info_build` (0x80445B80), a walk of quest ids **0x2328..0x2331 - ten
rows**, with `arena_work+0x0C` selecting one - and its outbox records "not the 0xEA60..0xEA6B range the recon
dossier guessed for this path". Both readings stand and they are about different things: **`[0xEA60,0xEA6B]` is
the *selection* gate** (`fn_803A13B4`'s test on `lb_param_w+0x00`, cross-confirmed by `arena_time_table[12]`),
while the arena task's `que_info` rows are built from a **second, ten-entry id range**. The discrepancy is
**open** - it needs one lane to read `fn_803B1EAC` (0x803B1EAC, owner `lobby/lb_quest_board.cpp`, the callee in
`arena_quest_info_build`) and the private 0x24E0-byte arena config table at 0x80604D30 (ten 0x3B0-byte records,
per that lane's note) before either range can be called "the" arena id set.

### 5.3 The `.sdata2` merge caveat (a playbook correction, measured)

**MWLD merges identical `.sdata2` constants across objects in this build.** Measured over the whole DOL: of
**7245** `.sdata2` labels, **640 are cited by more than one registered unit** - e.g. 0x8079C520 (`50.0f`),
0x8079C524 (`60.0f`) and 0x8079C528 (the int->double magic `0x4330000080000000`) are each cited by
`menu/arena_result` **and** `quest/quest_entry` **and** `enemy/em_pop`; 0x8079A3B8 (the same magic) by
`Pl/fn_80295EF4` and `menu/menu_item`. **Consequence: a `.sdata2` label pair is not evidence of a common TU**,
so `tudiscover.py`'s `.sdata2` "strong x1" kind is invalid here - it is exactly the evidence its only "strong"
cuts in the arena band rest on (`lbl_8079C930 -> lbl_8079C934`, `lbl_8079C96C -> lbl_8079C970`, and its
`extended` range 0x804437FC..0x80446AE8). The playbook's "referrer-run" test for `.sdata2` therefore needs the
caveat, or the tool's `.sdata2` class has to go.

**`.data`/`.sdata` sharing is *not* affected**: 12 of 2193 `.sdata` labels and 82 of 11208 `.data` labels are
multi-cited, and all of those are plausible genuine globals. And **Dolphin `.map` local-symbol prefixes are
noise, not an owner signal** - `_80444a34s_a_hou_back1_80607220`'s prefix is neither the referrer nor the owner
(`_802a22a4s_menu_item.cpp_805cdfc8` is referenced from `fn_802A5444`/`fn_802A579C`/`fn_802A64B0`).

The **extabindex staircase** is monotone across the whole 0x80441108..0x804485D4 run (every entry 8 B,
function addresses strictly rising), so it yields the exact per-function inventory and **no cut**.

---

## 6. What is unknown, honestly

1. **No `__FILE__` string exists anywhere in 0x8038E8E8..0x803C4BA0**, so no name in §1-§4 is class 1 or 2.
   Every file, function and field name proposed there is a marked `GUESS` or a `[map]`/`[dump-map]` name quoted
   as it stands. The arena band is the exception (§5.1) and it is a different band.
2. **`PlayMode` 2/3/6 semantics are inferred, not proved** - which values are tested where is proved, and that
   `{2,3,6}` selects the shared `quest_work` is proved; what the game calls those modes is not.
3. **The tier threshold 0x2710 is on the quest ID, not on hunter rank** - provable only as far as
   `lb_param_w+0x00` being the quest ID is provable (§1.3); if that field is something else, this flips.
4. **`quest_init`'s threshold scan at 0x803AD594 is dead code in the shipped build** (its row counter is
   overwritten before use) - either genuine dead code or a misread register lifetime. Flagged, not glossed.
5. **Both candidate quest-band seams (S1 at 0x803AFF34 in `quest/quest_entry.cpp`, S2 at 0x803B177C in
   `menu/arena_result.cpp`) are single-signal candidates**; the note did not register either, and the
   data-claim list it left (`.data` 0x8058AFC8, 0x805F2A98, 0x805F7AB0..0x805F7B78, `.sdata2`
   0x8079C510..0x8079C520 and 0x8079C4C0..0x8079C508, `.sbss` 0x80794C1C/0x80794C20) is unclaimed.
6. **Two `QuestWork` offsets are unreconciled** (`+0x03F4` vs `+0x03FC`; `+0x6A88` vs `+0x6A8C`) - noted there
   rather than guessed here.
7. **The arena id-space discrepancy (§5.2) is open.**
8. **Cost is unmeasured.** This document quotes no timing, no score and no byte count of its own; the only
   numbers in it are the addresses, sizes and counts the two notes measured, quoted as theirs.

## 7. Where the evidence lives

| what | where |
| --- | --- |
| the quest band, evidence-classed, 688 lines | `.pi/notes/quest-inventory.md` |
| the arena task unit, its registration and its seam argument | `.pi/notes/arena-task-92fd.md`, `.pi/outbox/arena-task-92fd.json` |
| the arena task's phase-1 record and reviewer checklist | `.pi/notes/loop-arena-phase1.md` |
| the `__FILE__`-with-one-copy seam test (playbook 54) | `docs/matching.md` |
| who calls / who reads an address, address-keyed | `python tools/units/callers.py <address\|name>` |
| the seam tool, and the `.sdata2` caveat above | `python tools/splits/tudiscover.py at <address>` |
