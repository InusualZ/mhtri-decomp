# `recordmerge` - Merge two views of one record header (splice into filler, per-struct keys, declaration compare) and refuse while unresolved

<!-- generated from the module docstring of `tools/units/recordmerge.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

Merge two views of the same record header into one definition (docs/matching.md 56).

## Users

skills (3); CLAUDE.md (1); docs (6)

## CLI

```
python tools/units/recordmerge.py --base <base> --other <other> [--out <path>] [--take other|base]
python tools/units/recordmerge.py --selftest
python tools/units/recordmerge.py --base include/ai/ainpc.h         --other worker/802d44f4-fn-802d44f4-bd0a:include/ai/ainpc.h --out include/ai/ainpc.h
```
Flags: `--base`, `--dry-run`, `--json`, `--other`, `--out`, `--selftest`, `--take`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: two headers -> merged header.

## Invariants and rules

* A header that describes a game record (`_AINPC_W`, `_HIT_W`) is written by several lanes at once, and every lane's branch carries its own view of the same struct: the same members at the same offsets, plus the fields that lane's bodies needed. The orchestrator has to fold those views into the one header the whole tree shares, and doing it by hand has gone wrong the same way three times - so this tool *is* the procedure, with the checks the hand passes lacked.
* `<base>` is normally the working file (the live header, CRLF on this host); `<other>` is normally a revision, in `git show` spelling:
* **The three rules** (docs/matching.md 56, learned on `_AINPC_W`).
* *Splice into the filler.* The live header is the base. For every member the other view names and the base does not have at that offset, find the base **filler** covering that offset, take the new member's size from the *other* header's own layout (the distance to its next member - never from the declared type), and split the filler into `[gap][new member][gap]`.
* *Key members per struct.* A header may define several structs, and an offset-keyed member map sizes a field from the wrong one: `_AINPC_W` shares its file with a smaller struct whose `0x0` matched first, which sized the 1-byte `active` as 12 bytes and then refused every splice for lack of room. Every lookup here is per group.
* *Compare the declaration, not just `(offset, name)`.* Two views can declare one offset differently - `u8 field_0x3F8;` against `u8 field_0x3F8[4];` - and an `(offset, name)` test calls that "already there" while the newcomer's own source fails to compile against it (`illegal operands 'unsigned char' [ 'unsigned char'`). Which side is right depends on which side's *code* depends on the declaration, so this tool reports the conflict and takes `--take` (default `other`: the incoming branch is the source that has to compile), dropping or shrinking the base members the winning declaration covers.
* **It refuses to write while anything is unresolved** - a named member in the way, a filler with no room or no known end, a group only one side has, a same-offset rename, a member whose size is the struct total - and prints the per-group delta so a human can see what would change. A merge that applies is still not a proof: the test is that **both** sides' sources compile (whole-tree `ninja -k 0` at 0 FAILED) **and** that the rows hold (`ninja changes` must print no line for a unit that already owned the record). This tool cannot check either - it makes the edit reproducible and its invariants explicit.

## Lib dependencies

cscan, text.

## Test contract

Tier: fixture.
Today's selftest (`tools/units/recordmerge_selftest.py`): The fixtures are deliberately small and hand-checked: each one exercises a rule the tool implements (splice into the filler, key members per struct, compare the declaration) or a reason it must refuse (a named member in the way, no room, a same-offset rename, a size it cannot infer). The real-world acceptance case - reproducing the `_AINPC_W` merge that was done by hand - is not here, because it depends on the repo's own history; it was run against `089491a7b` and is recorded in the commit that added this tool.
Target: `tools/tests/units/test_recordmerge.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

None recorded.
