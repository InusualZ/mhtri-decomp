# `declclash` - List names declared more than once with different text in one file's include closure (the `illegal function overloading` list)

<!-- generated from the module docstring of `tools/units/declclash.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

List function names declared more than once with *different text* in one source file's include closure.

## Users

profiles (`.claude/agents`) (1); CLAUDE.md (1); docs (1)

## CLI

```
python tools/units/declclash.py src/menu/fn_802E4978.cpp
python tools/units/declclash.py --only-different --json src/hud/layout.cpp
python tools/units/declclash.py include/unsplit/menu.h
python tools/units/declclash.py --selftest
```
Flags: `--fail-on-different`, `--json`, `--only-different`, `--root`, `--selftest`.
Exit codes: Exit status is 0 for a report and 1 only with `--fail-on-different` when a `DIFFERENT` name was found.
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: src + include -> SAME/DIFFERENT list.

## Invariants and rules

* Problem it solves: a unit that legitimately needs a callee owned by another unit must `#include` that unit's header. When the band headers (`include/unsplit/*.h`, `include/ef.h`, ...) still carry their own copy of the same declaration, MWCC stops with `(10197) illegal function overloading` at the *first* clashing line and `-maxerrors 1` hides the rest - so the cost of a cross-unit lane reads as one error after another instead of a finite list. This tool turns that into the list, before any source is edited.
* It is descriptive, not authoritative: it compares declaration *text* (with parameter names and `struct `/`extern ` spelling normalised away), so it is conservative in both directions.
* `DIFFERENT` names are worth inspecting but may still be the same type (`VEC3*` vs `nw4r::math::VEC3*` is one type when `VEC3` is that typedef).
* `SAME` names differ only in whitespace, parameter names or the `struct `/`extern ` keyword.
* C vs C++ *linkage* cannot be seen here (a declaration inside an `extern "C"` block and one outside it are different symbols); read the file when a pair matters.
* Exit status is 0 for a report and 1 only with `--fail-on-different` when a `DIFFERENT` name was found.

## Lib dependencies

cscan.

## Test contract

Tier: fixture.
The test (`tools/tests/units/test_declclash.py`, fixture tier; `declclash.py --selftest` runs it): The fixtures are a hand-built tree: a root file that includes a small web of headers, two of which declare the same name with the same shape (a parameter rename), two with a genuinely different shape, a `struct`/`void`-spelling pair, a name that only exists in the generated include tree, a two-line prototype that must be ignored, and an include cycle. The real acceptance case - the 30-name clash set of `src/menu/fn_802E4978.cpp`, 25 headers - is recorded in `.pi/notes/rule2-clash-pass-802e4978.md` and in the commit that added this tool.

## Known gaps

None recorded.
