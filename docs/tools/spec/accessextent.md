# `accessextent` - How far anything reads a data block: decode every reference site (callers census) and abstractly interpret offsets and loop walks to bound the extent

<!-- generated from the module docstring of `tools/units/accessextent.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

How far does anything read this block: the offset interpretation a data-extent claim turns on.

## Users

no caller in the tracked tree

## CLI

```
python tools/units/accessextent.py <address|name> [--json] [--limit N] [--min-offset 0xN]
[--elf PATH] [--objdump PATH] [--accessor NAME]
[--block ACCESSOR] [--rebuild] [--selftest]
```
Flags: `--accessor`, `--block`, `--elf`, `--json`, `--limit`, `--min-offset`, `--objdump`, `--rebuild`, `--selftest`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: callers index, main.elf, objdump -> verdict.

## Invariants and rules

* **The question.** "How big is this data block?" is answered today by the furthest offset anything reaches into it, and that number goes into `splits.txt` as a size. Until now every lane answered it by hand: the quest recon lane wrote a ~150-line PPC abstract interpreter over the DOL for exactly this, at ~40 minutes of its budget. It is asked again on every data block, so it should be one command.
* **What it does not rebuild.** `tools/units/callers.py` already enumerates every reference to an address - built on the dump's *addresses* (never its stale labels), cached, and with the split objects' relocations as a fallback - so the census is reused here verbatim (`callers.query`, `callers.load_index`). What that tool does not do is *interpret* a site: `lwz r3, <sym>` says the symbol is read, not how far past it the code then walks. That interpretation is this tool.
* **The shape of the answer.** For each reference site the instruction is decoded out of the target's own disassembly (`build/<game>/main.elf`, through `build/binutils/powerpc-eabi-objdump.exe` - nothing is on `PATH`; `ninja tools` fetches the binutils if it is absent), and a light abstract interpretation follows the value the site puts in a register:
* the block pointer - the value loaded from the queried symbol - is `blk`; and for a symbol that *is* the block rather than a pointer to it (no `sym@sda21` load anywhere), `lis sym@ha` + `addi rD, rA, sym@l` materialises the block's own address, which the two halves make recognisable by value;
* `addi rD, rS, k` / `add rD, rA, rB` / `mr` move and add to it, so `blk + const` and `blk + reg` are values too;
* a load or store whose base is `blk + const + reg…` is an **access**: its offset is `const`, plus the displacement, plus an upper bound for every register in the expression;
* a register that is incremented by a constant inside an enclosing loop is bounded by the loop nest's own walk - `(trips - 1) * step`, multiplied through the outer levels, with `trips` read from the guard (`cmpwi rC, N` + `blt`, or a `bdnz` whose `mtctr` is a constant, which sits *before* the loop head) and the starting value from the last `li` before the access;
* control flow is respected enough that no state is invented: every direct branch records the register state that reaches its target, so the region an unconditional `b` jumps over is walked with an unknown state and a `bctr` switch keeps the state its cases were dispatched with.
* **Whichever census answered, the instructions decide.** The dump graph and `callers.py`'s object fallback are not equally detailed: the fallback has no instruction text, so it calls every non-call reference `addr`, and it records an `R_PPC_ADDR16_HA`/`_LO` site as the immediate *halfword* it patches - two bytes into the instruction. Both are recovered from the instruction this tool decodes anyway (the kind from the mnemonic, the site from the halfword), so a query answers the same thing whichever census the tree has; `census.source` still says which one answered, and `rep["census"]` is that census's own counts. The fallback's two rows for a `lis`+`addi` pair are not coalesced the way the dump's are, so its `unresolved` list is one row per relocation rather than one per site; the extent is the same.
* **What it refuses to do.** A base that cannot be resolved is reported *per site* with the reason - a register with no static bound, a pointer that is passed to a callee (`bl`) or stored away, a load that is never dereferenced, an offset term the size of an address - never guessed. A tool that invents an offset is worse than no tool, because the claim it justifies becomes real in `splits.txt`, and a size that is 2 bytes short silently claims bytes belonging to the next object in the link order.
* **The strongest evidence is reported separately, and a clear is not a copy.** A `memset`/`bzero` call whose buffer is the block and whose size argument is a constant *states* the extent: `memset(get_userdata(), 0, 0x6000)` is a 0x6000-byte record, whatever the accesses reach. A `memcpy`/`memmove` whose buffer is the block is a *partial* operation - save data copied into the record, one field copied out - so its constant is a lower bound, never the record's size; and a call of either kind whose buffer is the block *plus an offset* says nothing about the whole block. The verdict therefore names `zeroing size` and `copy size` separately, prefers a whole-block clear as the extent, and refuses to escalate over a partial copy: a `memcpy(blk, ..., 0x6000)` into a 0x6AB8-byte record is not evidence that the record is only 0x6000 bytes. Those sites are listed with their function, address, class and buffer offset; a disagreement between the extent and the inferred reach is printed loudly, the dangerous direction (an access past the stated size) most loudly of all.
* **A block with no symbol is seeded through its accessor.** Some records have no map row - `Q_MoveWork` is heap, reached as `*(u32*)(0x806685E0 + 0xA4)` then `+0x10 + index*4` - so a census keyed on a symbol answers *0 accesses*. `--block <accessor>` treats `r3 = bl <accessor>` as the block base: the accessor's call sites are the census, their callers are the functions interpreted, and one heap record reached through one accessor is one command (`--block get_move_work_adrs`).
* **The real case, as the acceptance run.** `Q_UserData` is reached through a cached global: `get_userdata` (0x8004D120) loads `*(void**)(&system_w + 0x95C)` and stores it into the `.sbss` word at 0x80794880, from which 346 functions read it back (789 loads + 3 stores). The literal size comes from the init path - `memset(get_userdata(), 0, 0x6000)` in `fn_80047398.cpp`'s `fn_800497B4` (0x800497D0) and `fn_800498EC` (0x80049908). The furthest static access is **0x53A8** (`lwz r0, 21416(r31)` at 0x8004F1F4 in `set_mydata2vs__FUcUc`, r31 = `get_userdata()`'s return and never rewritten); the loop-carried one the data-extent brief measured, `sth r29, 0x5366(r3)` at 0x803C138C inside a 16-iteration loop in `menu/get_pop_dat_ptr.cpp`'s `fn_803C0F3C`, is 0x5366 + 15*4 = **0x53A2** (tied with the same loop's 0x803C10C0), and the same function also reads 0x53A4 after that loop. `--selftest` asserts all three, so the loop bound, the plain displacement and the post-loop load are each pinned, and it also cross-checks a symbol whose size the map *does* state - `system_w` is declared 0xA5C and the tool infers 0xA5C from a furthest access of 0xA58, which is what says the offsets are not invented.
* **Limits, stated so no number is over-read.**
* The census is the number of *sites*, and a site whose block pointer is passed to a callee is not followed into it: the block may be read further by the callee, and that is said per site rather than folded into the maximum.
* The scan is one pass in address order with a state per branch target, not a full dataflow fixpoint: a register written by a `bctr` switch's case bodies is followed only where the case re-establishes it, and a value that reaches an access through two *different* paths is kept only when both agree. Both directions are conservative - a missed access is reported as an unresolved site, not as a smaller size - but the count of `unresolved` is what to read for how much the pass could not follow.
* An interior reference the dump prints as `sym+0xNN@ha` is not indexed under `sym` by the census's symbol regex; separately, that gap is read here out of the census's own rows (never a second scan).
* The object form is used only when nothing loads the symbol directly (`sym@sda21`). A pointer variable whose *address* is materialised and then dereferenced (`lis/addi sym` + `lwz rX, 0(rB)`) is a site the pass cannot yet follow, and it is reported unresolved rather than guessed.
* The dump's state is the census's to report; when the dump is stale the instruction *texts* are too (never the addresses).
* It is a reader. No `src/` edits, and it writes nothing at all - the census it reuses owns the only cache (`build/tmp/callers/graph.json`).

## Lib dependencies

`lib.refs` (`RefIndex`: the census of sites, the dump or the object fallback -
WP3c, was `callers`), `lib.ppc` (`decode_rw`, `CALL_MNEMONICS` - was through `callees`), `lib.binary.objdump`, `lib.report`
(`rel_path`), `lib.repo` (`VERSION`).

## Test contract

Tier: fixture (synthetic instruction sequences); smoke: the Q_UserData acceptance run when main.elf and objdump exist.
Today's selftest (`tools/units/accessextent_selftest.py`): `accessextent.selftest()` holds the checks, so this entry point and the tool's flag cannot drift. The synthetic half is instruction sequences only - no dump, no ELF and no objdump - and the real Q_UserData acceptance run is made only when this tree has `build/<game>/main.elf` and an objdump, so the check count is identical in MAIN and in a fresh worktree.
Target: `tools/tests/units/test_accessextent.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

None recorded.

## History (the incidents behind the rules - keep the rule, drop the narrative when the rule is stable)

* **The three numbers are labelled, always.** The default output spells out `furthest static access` (the maximum offset the DOL contains - what a bare "furthest" means), `furthest loop-carried` (a displacement plus a bound read out of a loop guard: a *derived* figure, and the one easy to paraphrase as the maximum), and `literal size`, and the access table is headed "furthest static accesses". The data-extent brief that asked for this tool had transcribed the loop-carried figure as the furthest static access - it cost a round trip to unpick - so the output cannot be read that way, and `--json` carries `furthest_static_*`/`furthest_loop_carried_*` as separate fields with a per-access `loop_carried` flag.
