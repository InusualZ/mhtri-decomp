# `typeregistry` - Registry of shared types/helpers under include/ and src/: where defined, who uses, duplicated debt; `relevant_headers` feeds the brief

<!-- generated from the module docstring of `tools/units/typeregistry.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

The registry of shared types and helpers - what already exists, and who copied it.

## Users

imported by `brief`

## CLI

```
python tools/units/typeregistry.py                     # the duplication report, human-readable
python tools/units/typeregistry.py --json              # the same, machine-readable
python tools/units/typeregistry.py --unit Pl/pl_act    # the shared headers that unit should use
python tools/units/typeregistry.py --report <path.md>  # write the report to a file
python tools/units/typeregistry.py --selftest
```
Flags: `--json`, `--no-map`, `--report`, `--root`, `--selftest`, `--unit`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: include, src, map -> report.

## Invariants and rules

* The project's rule (CLAUDE.md -> Conventions; docs/plan.md section 6.5 rule 1) is that a type more than one unit needs is declared **once**, under `include/`, and included where needed - a declaration moves there the *second* time a unit needs it. Nothing told a worker what already existed, so two units created `include/ef.h` and `include/nw4r/math.h` independently while three others each defined the same `VEC3`/`EfWork` locally and deferred the consolidation to nobody. This tool is the missing lookup: it scans `include/**` and every `src/**` file, and answers, per declaration:
* **where it is defined** - every file (a header, a unit, or both);
* **which units use it** - a unit uses a declaration when it includes the defining header, names the declaration in its source, or owns a map symbol whose (mangled) name encodes it;
* **whether it is duplicated debt** - a shared header's name that a unit re-defines (rule 1), a name two units both define with no header owning it, or a name one unit defines that another names without including it (rule 1, before there is a header);
* **whether a helper prototype is repeated** - the same `extern` carried by several files (rule 2).
* `brief.py` consumes `registry()` + `relevant_headers()` directly, so a worker's brief names the shared headers its unit should reuse instead of re-create. The scan is textual by design - it is a *lookup* for the worker, not a compiler - and every finding names the file and line a human can open.
* What is scanned, and what a "declaration" is:
| kind | how it is found |
| --- | --- |
| `struct` / `union` / `enum` / `class` | the keyword plus its body; the tag and any trailing `typedef` alias are both recorded |
| `typedef` | a non-aggregate `typedef`; the declarator (including `(*fn)` function pointers and arrays) |
| `using` | a C++ alias (`using Foo = ...;`) |
| `inline` | an `inline`/`__inline` function definition (a helper meant to be shared) |
| `macro` | a `#define` (object- or function-like) |
| `extern` | a file-scope `extern` declaration (a helper prototype - rule 2's duplicate when two files both carry it) |
* `include/types.h` is the project's scalar base: every unit including it is normal, so it is only reported when a unit *redefines* one of its names, and `src/Camellia/camellia.c` is the documented vendor exception (EXCEPTIONS below).

## Lib dependencies

cscan, project (`SymbolMap`, `Splits`), names (`peel_tokens`), repo (the default `--root` is the invocation's tree).

## Test contract

Tier: fixture.
Today's selftest (`tools/units/typeregistry_selftest.py`): The checks run against a temp fixture tree, so no repository state, no map and no compiler are touched. What they pin: comment/string stripping that preserves length and line numbers, the declarator extractor for every shape the tree carries (named/anonymous aggregates, forward and function-pointer and array typedefs, `using`, macros, `inline` helpers, `extern` prototypes), the field-type fingerprint, the `<digits><that many chars>` mangled-name peeling, shared-header scanning, the two debt kinds plus the rule-2 repeated prototype, the vendor exception, the relevance ranking the brief renders, and that the report text names the header and the copying unit. `typeregistry.selftest()` holds the checks so `typeregistry.py --selftest` and this entry point cannot drift.
Target: `tools/tests/units/test_typeregistry.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

None recorded.
