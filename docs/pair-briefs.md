# Pair-run briefs

Paste-ready briefs for the two read-only legs of a pair run (`docs/pipeline.md` section 15): the dependency scout
(step 2) and the scout review (step 2b), and the deps JSON shape both write. Replace `<A>`, `<B>`, `<pair>` and
`<base>`; everything else is used as written.


## Every brief carries these two rules

* **Nothing outside the checkout is ever deleted, moved or overwritten** (owner, 2026-10-14; `docs/pipeline.md` 11). Your writes stay inside your checkout (your slot, or MAIN's `.pi/` where a brief says so). A path outside it - a stray directory, a temp tree, anything under `C:\` that is not the checkout - is **listed and reported**, never removed, even when it looks like your own scratch. Scratch goes in `<checkout>\.pi\tmp\<slug>\`, written as a Windows path (`C:\Users\...\mhtri-dtk.slotN\.pi\tmp\...`); never `/c/Users/...`, `/tmp` or `$TEMP` from Python or a script - Windows Python resolves `/c/Users/x` to `C:\c\Users\x`, a directory outside the repo (the 2026-10-14 incident that deleted `C:\c`).
* **Ghidra is a hint, never the source of truth** (owner, 2026-10-14). Names, field names, struct layouts, types and symbols in the shared Ghidra project come from the owner's own reverse engineering; use them as leads and prove every fact from the retail bytes, the SDK or a measurement (`docs/memory-dump.md`).

## The dependency scout (step 2)

Launch: a read-only `scout` (or `Explore`) lane in MAIN.

```
READ-ONLY dependency scout in <repo> on main (<base>). Read CLAUDE.md, docs/plan.md 6.5, docs/pipeline.md 4, 14, 15.
Do NOT edit tracked files, commit, or run ninja/configure.py/land.py; never print config/RMHE08/symbols.txt. You MAY
compile scratch probes into .pi/tmp/scout-<pair>/ with a unit's own compile command
(`python .claude/skills/mwcc-unit-matching/scripts/mt.py units <unit>`). Run `date +%T` at start and end.

Pair: <A> and <B>. Two lanes will each write a bounded SLICE (~20 functions) of one unit, confined by a manifest that
`land.py land --manifest` enforces; every edit outside a unit's own files is applied first by a pre-pass that lands
alone. Your list is what the pre-pass applies. Work ONE unit at a time.

Per unit:
0. TU check (tools/splits/tudiscover.py --no-refresh, tools/units/poolseams.py --unit, data order, map name runs).
   Not one TU -> the recut is item 1.
1. The SLICE: ~20 unwritten/partial functions, best value (size x tractability): address, size, name, current %.
2. Per slice function, from the target asm (`mt.py diff -u <unit> <sym> --all`, build/RMHE08/obj/<unit>.o with
   build/binutils objdump -dr): every callee, datum and struct field offset, then:
   a. declared in an owner header the unit can include? COMPILE-PROBE unit source + each needed owner header;
   b. every rename paired with an owner-header declaration;
   c. return types from every retail caller's next use of r3 (cmplwi -> unsigned, extsb -> s8, clrlwi 16/24 ->
      u16/u8; cmpwi 0 says nothing), parameter widths from the callee's OWN body;
   d. every struct displacement maps to a named field in the defining header (rule 7 refuses unkNN and address-named
      tokens); every struct defined once in the include closure;
   e. declarations of the unit's own slice symbols anywhere in its include closure and src/unsplit/*.h;
   f. new rule-7 tokens a body would add (the ratchet is per rule, file, token);
   g. the data claim covers what the bodies read (tools/units/datagap.py --census --unit); foreign R_PPC_ADDR32
      pointers into the unit's ranges (tools/units/callers.py <row> --pointers);
   h. the unit's leaf headers go in its own list.
3. Items: id, kind (tools/units/backlog.py NEW_KINDS: shared-file, range, seam, flag, tooling, naming, band-header,
   untyped, method), file, what, fix, evidence, prepass. FLIP likely? (configure.py in owns only then.)
4. The shared-header map: every src/ file both slices need edited or read; which items edit it; the pre-pass batching
   so no lane edits a shared file; both manifests (owns disjoint, shared headers read_only to both).

Final message: per-unit tables with evidence, the slices, step timings, and LAST a ```json``` block in the deps shape.
```

## The scout review (step 2b)

Launch: a read-only `codereviewer` lane in MAIN, with fresh context, after the deps file is saved.

```
Read-only SCOUT REVIEW in <repo> on main (<base>). Do not edit tracked files, build the repo, commit or switch
branches; never print config/RMHE08/symbols.txt. Scratch probes go in .pi/tmp/review-<pair>/. Run `date +%T` at start
and end.

Review .pi/outbox/deps-<pair>.json. Independently re-check, per slice function, from the target asm:
1. every callee/datum has a declaration in an owner header the unit can include (compile probes);
2. every proposed rename is paired with an owner-header declaration;
3. return types from every retail caller's next use of r3; parameter widths from the callee's own body;
4. every struct field the slice touches is named; no struct defined twice in the include closure;
5. declarations of the units' own symbols in their closures and src/unsplit/*.h (incl. addresses in the unit's ranges);
6. new rule-7 tokens; 7. the data claim and foreign pointers into the unit's ranges; 8. leaf headers owned;
9. the manifests: owns disjoint, every file both closures read is read_only to both, and no lane needs to edit a
   shared file after the pre-pass;
10. the pre-pass batching: items of both units that touch one src/ file go in one batch; which batches can run in
    parallel; the landing order.

Output: a verdict per item (correct / wrong / incomplete + the correction), ADDED items (same shape), DROPPED items,
slice functions to remove, the final pre-pass plan, and a ```json``` block
{"verdicts":{},"added":[],"dropped":[],"slice_drop":{},"prepass_plan":{}}.
```

The orchestrator appends the review to the deps file under `review` and briefs the pre-pass from the corrected items.

## The deps JSON shape

As written in trials 4 to 6 (`.pi/outbox/deps-ainpc-menuinfo.json`, `deps-plact-plmotion.json`):

```json
{"pair": ["ai/ai_npc.cpp", "menu/menu_infomation.cpp"],
 "base": "32cc5a1e4",
 "scout": {"started": "07:09:23", "ended": "07:28:12", "probes": ".pi/tmp/scout4/"},
 "units": {
  "ai/ai_npc.cpp": {
   "own": ["src/ai/ai_npc.cpp", "src/ai/ai_npc.h", "src/ai/ainpc_move.h"],
   "flip_likely": false,
   "slice": [{"addr": "0x802D8550", "size": 316, "name": "fn_802D8550", "pct": 0.0}],
   "items": [
    {"id": "AI-4", "kind": "shared-file", "file": "src/ai/ainpc.h",
     "what": "unnamed offsets the slice reads: +0x184 VEC3, +0x344 u16, ...",
     "fix": "split the padding into named fields; rebuild every includer",
     "evidence": "objdump -dr build/RMHE08/obj/ai/ai_npc.o",
     "prepass": true}],
   "manifest": {"lane": "ai_npc-slice1",
                "owns": ["src/ai/ai_npc.cpp", "src/ai/ai_npc.h"],
                "read_only": ["src/ai/ainpc.h", "src/Pl/", "config/RMHE08/", "configure.py"],
                "units": ["ai/ai_npc.cpp"]}}},
 "shared": ["config/RMHE08/symbols.txt"],
 "review": {"started": "07:31:48", "ended": "07:51:32",
            "dropped": ["AI-6", "AI-9", "AI-11", "MI-6"],
            "corrections": {"AI-8": "use IdValue, not _SLOTENT"},
            "prepass_plan": {"shared_batch": ["AI-3", "MI-1", "AI-2", "AI-1"], "ai": ["AI-4", "AI-5"], "menu": []}}}
```

The `manifest` object is the one `land.py land --manifest` reads (`docs/tools/spec/lane-manifest.md`); save it as its own
file (`{"manifest": {...}}`) when the lane's base is known.
