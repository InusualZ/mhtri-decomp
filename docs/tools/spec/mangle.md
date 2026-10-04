# `mangle` - Derive the mangled name MWCC emits for a C++ declaration (compile a stub) or estimate it textually; prints the symedit rename

<!-- generated from the module docstring of `tools/units/mangle.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

Derive the mangled name MWCC emits for a C++ declaration, so the map can be renamed to it.

## Users

skills (8); docs (8); imported by `methodize`, `stylelint`

## CLI

```
python tools/units/mangle.py 'void Pl_Skill_ck(_PLW* self, u8 x)'
python tools/units/mangle.py --unit Pl/pl_act.cpp 'void Pl_Skill_ck(_PLW*, u8)'
python tools/units/mangle.py --file decls.cpp --json
python tools/units/mangle.py --selftest
```
Flags: `--file`, `--json`, `--no-include`, `--old`, `--selftest`, `--unit`, `--verbose`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: declaration -> mangled name.

## Invariants and rules

* A C++ unit's symbols are mangled. Our symbol map spells them the way dtk does when it cannot demangle (`fn_800CCCF8`), and the original object's name is the mangled one - so a C++ definition whose name does not match the map's spelling is invisible to objdiff, and the tempting workaround is `extern "C"`.
* **It is not needed.** The map is a build input, not a description of the original's symbol table: rename the map symbol to the mangled name our source emits and objdiff pairs it by name, the relocations match, and the front-end stays the one the unit really had (docs/plan.md, "The language comes from the symbol"). This tool produces that spelling. It is the other half of a rename, so it prints the `symedit.py` command too.
* The snippet is a **declaration**; a body is appended when it has none (`{ }`), because only a *defined* symbol appears in the object's function table. A full definition may be passed instead - it is used verbatim. Whatever the declaration needs (a struct, a typedef, an include) has to be in the snippet: the tool compiles it as its own translation unit with the flags of `--unit`.
* Only the compiler's **front-end** decides the mangled spelling, so the `--unit` flags matter only in that they select the compiler version and `-lang=c++`; they are not a claim about the unit being renamed.

## Lib dependencies

units (`Unit.resolve`, `ninja_command`, `run_tokens`, `function_names`, `quiet`), repo, names (the estimates, re-exported
for `methodize`).

## Test contract

Tier: fixture.
Today's selftest: in-file `selftest()` (`--selftest`).
Target: `tools/tests/units/test_mangle.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

`--exact` needs the build tree's compiler
