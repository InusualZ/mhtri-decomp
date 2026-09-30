---
id: 70
title: Data a unit uses that nobody owns is the unit's to claim - an `extern` for it is the defect
status: works
problem: A unit reads or writes bytes the target object carries, no `splits.txt` range covers the address, and the source only has an `extern` for it - so the data stays at zero and the flip can only fail at the link.
tags: [data, sections]
applies: []
demo:
reviewed: 2026-09-29
related: [23, 29, 53, 58, 64]
---

# 70. Data a unit uses that nobody owns is the unit's to claim - an `extern` for it is the defect

**Problem.** A unit reads or writes bytes the target object carries, the address is covered by **no**
registered `splits.txt` range, and the only thing the source has for it is an `extern` declaration. The shape
that prompted this row is in the Network band, `include/Network/network_state.h`:

```c
/* The band's request-header constants (no registered owner; declared, never defined - playbook 29).
 * `sessionTimeoutParam`/`Param2` are the bytes {1,2,3}, `requestHeaderWord0`/`Word1` the 8-byte block
 * {1,2,3,5,4,6,7,8} `sendReqOpcode1B` copies, `maskedUserName` the "******" sentinel
 * `sendReqUserObject` compares a row's short name against. */
extern const u16 sessionTimeoutParam;      /* 0x8079C7D8 */
extern const u8  sessionTimeoutParam2;     /* 0x8079C7DA */
extern const u32 requestHeaderWord0;       /* 0x8079C7E0 */
extern const u32 requestHeaderWord1;       /* 0x8079C7E4 */
extern const char maskedUserName[7];       /* 0x80793968 - the map's own size, so MWCC uses sda21 */
```

`Network/network_state.cpp` claims `.text`/`extab`/`extabindex` and **nothing else**, so every one of those
addresses is unowned. The comment names the defect itself: bytes the unit reads that no lane owns or
reproduces, parked as an `extern` so the unit measures while the data stays at zero - and the flip can only
fail, at the link. Read literally, the old advice ("declared, never defined") is what put it there.

**How it looks.** A unit's data rows are `ARG` (unpaired) or use a `@NNN` pool name, `datagap.py --unit` lists
target-extra bytes for an address no unit claims, and `flipcheck.py` cannot report the data section identical.

**Why it happens / how to work it.** **Rule 12** settles it: when a unit uses data nothing claims, the unit **claims that range**
in its own `splits.txt`, in the section the bytes live in, and **matches it as part of its own object** - the
bytes reconstructed so they byte-match the target. An `extern` for unowned data is the finding, and a header
comment that says "no registered owner" is the finding naming itself. Claiming is what turns a load row
(objdiff's `ARG` / a `@NNN` pool name, or a `lis`/`addi` pair against an undefined symbol) into a defined
symbol at the unit's (section, offset), which is the same pairing mechanism row **23** describes; the
difference is where the bytes come from. Row 23's claim is a unit's *own* data, row 53's is a table the
compiler emitted for it; here there is no claim to inherit at all - the address is unsplit, and the unit that
reads it is the only candidate owner.

Read the run before claiming it, because the playbook's caveats still bind:

* a **partial** `.sdata2`/`.sdata` claim does **not** link (`mwldeppc.exe` `ELF_gen.c` 2802) - claim the pool
  only when our object emits none of its own (rows **23**/**29**), and force a re-split when testing a claim
  (`rm build/RMHE08/config.json`), or you link the old object and see a false green;
* a unit that claims several runs of one section must own the bytes **between** them, or an
  `auto_*_data` unit lands inside its range and `dtk dol split` dies with a link-order cycle (row **53** -
  leading/trailing gaps are harmless, a gap *between* two claims is not);
* a `.data` claim can make dtk drop the target's `R_PPC_NONE` pool relocations, so measure the unit before
  *and* after (row **23**);
* **never claim what is not yours**: data a **registered** unit owns means include that owner's header
  (rule 2), and bytes the target object does not carry at all are compiler-synthesised - claimable only while
  your unit is the address's **sole referencer** (row **58**);
* and the carve-out: **declare-never-define stays right when the range is ALREADY the unit's own** (row
  **29**). There, *defining* the constants rebuilds the pool and moves the whole section, which is why the
  old advice exists. Rule 12 targets the unowned case, where nothing is claimed at all.

**When NOT to apply.** The carve-outs above are the list: a range already the unit's own (declare, never
define - idea 29), data a registered unit owns (include its header), compiler-synthesised bytes not in the
target (idea 58), and a partial `.sdata`/`.sdata2` claim (does not link). Numbers and addresses below are
evidence as measured in 2026-09; this is a process idea (splits + data), so it has no single-object demo. Re-checked 2026-09-29: the
`network_state.h` externs and the unit's three-section `splits.txt` block described above are unchanged.

**Result.** The evidence for the claim is the address, the referrers - `tools/units/callers.py <addr>`, which
is address-keyed because the asm dump is stale - the size (the target object's symbol size, or the run our
object emits) and the section boundary. The row's own outcome is measured the same way every data claim in
the playbook is: `datagap.py --unit` must show no target-extra for the claimed range, the unit's `flipcheck`
must report the data section byte-identical, and `ninja build/RMHE08/ok` must stay green with the DOL
unchanged. Two landed claims of this class are the control: row 58's `ef/fn_80101DF4` private `.sdata2` run
`0x807966E8-0x807966F0` took that unit's data rows 20/20 -> **28/28** and flipped it to `Object(Matching)`
with the sha1 unchanged, and row 53's `stage/fn_802B3270` claim (the table's own range only) made the unit's
`.data` pair at 100 %. The Network constants above are the register seed for the unclaimed case - the five
declarations sit in the campaign's local scratch note `.pi/notes/rule12-instances.md` (gitignored, so it may be
absent in a fresh clone), one per instance, with the run each one means.

**Example.** The claim, then the emission that replaces the declarations:

```
config/RMHE08/splits.txt
Network/network_state.cpp:
        .sdata2     start:0x8079C7D8 end:0x8079C7E8     # the request-header block, 16 B
        .sdata      start:0x80793968 end:0x8079396F     # maskedUserName, 7 B - the map's own size
```

```c
/* reaction in src/Network/network_state.cpp - the bytes are the unit's now, so match them rather than
 * declare them (row 29's declare-never-define was about a range that was ALREADY ours; this one was not) */
static const u8 sessionTimeoutParam[3]  = { 1, 2, 3 };                  /* 0x8079C7D8 */
static const u8 requestHeaderWord[8]    = { 1, 2, 3, 5, 4, 6, 7, 8 };   /* 0x8079C7E0 */
static const char maskedUserName[7]     = "******";                     /* 0x80793968 (.sdata) */
```

The `.sdata` half is safe (row 23); the `.sdata2` half is only safe while our object emits no pool of its
own - if it does, the pool words stay declared `extern` and the run is left to its auto unit (row 58), and
the header records why. Either way the finding is closed the same way: the declaration is no longer the
answer, the claim is.
