# `methodize` - Rule-13 planner: `Type_name(Type*)` free functions -> members, with declaration/definition/call-site edits, estimated or verified mangling, and a symedit rename-batch

<!-- generated from the module docstring of `tools/units/methodize.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

Plan the migration of `Type_name(Type* self, ...)` free functions to `Type::name` members (rule 13).

## Users

the landing gate (1); profiles (`.claude/agents`) (6); skills (3); docs (4)

## CLI

```
python tools/units/methodize.py NetworkSingleTcp             # the plan for one type
python tools/units/methodize.py --all                        # every type that has a finding
python tools/units/methodize.py NetworkSingleTcp --batch out.txt   # a `symedit.py rename-batch` input
python tools/units/methodize.py NetworkSingleTcp --exact     # confirm each mangling with the real compiler
python tools/units/methodize.py --class NetworkTcp --fn networkPeer_send=send --fn networkPeer_recv=recv         --unit Network/network_socket_streams.cpp --batch out.txt      # explicit mapping (below)
python tools/units/methodize.py --map networkPeer_send=NetworkTcp::send --map-file map.json
python tools/units/methodize.py --selftest
```
Flags: `--all`, `--batch`, `--class`, `--exact`, `--fn`, `--json`, `--map`, `--map-file`, `--obj`, `--root`, `--selftest`, `--unit`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: src, include, map, object -> plan, batch file.

## Invariants and rules

* Explicit mapping: when the free functions do not share the class's prefix (`networkPeerStream_*` -> `NetworkByteStream`, `networkPeer_*` -> Tcp or Udp by address order), name each `old=Class::member` (`--map`, repeatable; `--map-file` is a JSON object of the same pairs; `--class C --fn old=member` is the short form). Each function is found by its exact name (no prefix or `Type* self` naming needed): a first parameter of type `Class*`/`const Class*`/`Class&` is the `this` (kind member), anything else makes it a static member. The plan adds, per function, the exact edits it can determine (`edits`: the definition header rewritten to `Class::member(...)`, a declaration to move into the class, every call site rewritten to `obj->member(...)` / `Class::member(...)`) and writes the rename-batch. The mangling is estimated by `mangle.py`; with `--unit <src path>` (or `--obj <file.o>`) it is VERIFIED against the function symbols of that built object: a name the object defines is `verified`, and a Class::member the object defines under a *different* mangling refuses the plan (exit 2, no batch written). An object built before the migration only carries the old names and leaves the estimate unconfirmed (say so, rebuild, rerun). Two mappings that mangle to one name are refused.
* It **never edits source or the map**. A lane does the source half (declare in the class, define `Type::name`, sweep the call sites) and measures it (playbook 60: `this` arrives in r3 like the old `self`, so a non-virtual method should be codegen-neutral, but the declaration set is a codegen input); the orchestrator applies the map half with `python tools/symbols/symedit.py rename-batch <file>`.
* How the findings are found: it calls `stylelint.rule13_findings` over `src/` and `include/` (one rule, one implementation), so a function the lint exempts with `/* free: <reason> */` is not planned. The mangled name is `mangle.estimate_member_mangling` (a pure-text estimate, marked `~`) unless `--exact` compiles the member through `mangle.mangle` (needs the build tree's compiler). **In the batch file only an exact name is an active rename**; an estimate is written as a comment, because a wrong map name un-pairs the symbol in objdiff. A constructor/destructor-shaped name (`construct`, `ctor`, `dtor`, `destruct`, `init`-less) is flagged: its mangling is `__ct__`/`__dt__`, not the method form, so no rename is proposed for it.

## Lib dependencies

cscan (`calls`, `split_params`, `function_declarations`, `mask_preproc`), project (`SymbolMap`), names (the mangling
estimate), units (`function_names`), repo; the rule-13 findings from `stylelint` and the compiled spelling from
`mangle.mangle` (its two tool imports).

## Test contract

Tier: fixture.
Today's selftest: in-file `selftest()` (`--selftest`).
Target: `tools/tests/units/test_methodize.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

None recorded.

## History (the incidents behind the rules - keep the rule, drop the narrative when the rule is stable)

* docs/plan.md section 6.5 rule 13: a free function named `<Type>_<name>` whose first parameter is the type's own `self` is a member function spelled the C way; the **static form** (owner ruling 2026-09-29) is the same name with no `Type* self` (`GameSpyInterfaceThread_getInstance(void)`), a **static member**: the plan then reads `static R name(args);` in the class, `R Type::name(args)` for the definition, callers `Type::name(...)`, and the compiler's mangling is the plain member one with no `this` and no `C` (`getInstance__22GameSpyInterfaceThreadFv`). Each plan entry says `kind: member` or `kind: static`. This tool is the read-only planner for fixing one: it lists, per function, where it is declared and defined, every reference in `src/` and `include/` (comment-aware), the member signature to write, the mangled name the compiler will then emit, and the map row it must rename.
