# `langcheck` - Decide a unit's language (C/C++) from evidence: mangled definitions, `.cpp` `__FILE__` strings, extab presence under no-exceptions libs; the gate and brief read it

<!-- generated from the module docstring of `tools/units/langcheck.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

Decide a translation unit's *language* (C or C++) from evidence, not from our convenience.

## Users

the landing gate (14); skills (3); docs (4); imported by `attribute`, `brief`, `promote_batch`, `relocaudit`, `vtableaudit`

## CLI

```
python tools/units/langcheck.py                 # every registered unit, verdict + evidence
python tools/units/langcheck.py --disagree      # the sweep report: units that disagree
python tools/units/langcheck.py --unit auto/800CCFB0_fn_800CCFB0
python tools/units/langcheck.py --json
python tools/units/langcheck.py --selftest
```
Flags: `--disagree`, `--json`, `--main`, `--selftest`, `--unit`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: target .o, DOL, map, configure.py -> verdicts.

## Invariants and rules

* **its own symbol is mangled** - a *definition* carrying MWCC's `__F`/`__Q` argument-list mangling (`SetPosition__Q34nw4r3g3d6CameraFRCQ34nw4r4math4VEC3`, `fn_800CD584__FP9ResHandle`);
* **its panic/log string names a `.cpp`** - the `__FILE__` assert strings are original source names (`ef_line.cpp`, `g3d_resanm.cpp`, `menu_message.cpp`). A unit whose *object* references one is a C++ file even when its `.text` reads like C, and a unit whose object references a `.c` name is C.
* A mangled name on the *referenced* side (`get_now_areano__Fv`, `Panic__Q24nw4r2dbFPCciPCce`) is **suggestive, not conclusive**: it is still reported, with its reason, but it must not raise confidence to `high` nor drive an extension change by itself. A C translation unit can call a mangled function - it declares it with the map's spelling - and `auto/800FD520_fn_800FD520` does exactly that: it calls mangled `SetRootMtxTrans__FPQ34nw4r2ef6EffectPQ34nw4r4math4VEC3` and was reconstructed as `.c`, matching at 100.00 % with all 26 relocations identical (`auto/803066F0_fn_803066F0.c` is the same shape).
* A third signal is **suggestive, not conclusive** too, and it is cheap and mechanical: an object that carries the `extab`/`extabindex` sections was compiled as C++ - **C has no exceptions**, so a C translation unit has no `__eh` records to emit. The confound is a lib whose `cflags` set `-Cpp_exceptions on` (`cflags_pl`, `cflags_main`, `cflags_g3d`, `cflags_camellia`): that makes a **C** unit emit `extab` as well, so the signal is only usable when the lib leaves the flag off, which `cflags_exceptions` resolves from the unit's cflags variable. Even then it is **one-directional** - a C++ file with no `try`/`catch`/`throw` emits no `extab`, so an object *without* the section evidences nothing (`Runtime.PPCEABI.H/__init_cpp_exceptions.cpp` is C++ and has none). It therefore joins the mangled-callee signal as `suggested`: reported with its reason, keeps the extension, and never renames on its own. On today's tree its decisive hit count is **0** - the `.c` units that carry `extab` all sit in libs that enable exceptions (`auto`/`main`), where the hint is silent - but it is decisive for the no-exceptions libs and catches a wrong extension the moment a new unit is registered there.
* That decides three things and none of them is stylistic: the **file extension**, the **`-lang`** the front-end is run with, and the **name objdiff pairs by** (a C++ definition is mangled unless it is `extern "C"` - playbook row 42 seen from the other side).
* **Why this is a tool and not a judgement call.** The attribution batches picked `.c`/`.cpp` per unit from a guess, so registered units are the wrong language today: `auto/800CCFB0_fn_800CCFB0` is `ef_line.cpp` and is registered as `.c`. `auto/800CCFB0` closed at 99.96 % with its last rows attributed to "C-vs-C++ front-end, not source shape" - that residual is not source-reachable. The fix belongs in the promotion pass (rename + extension + one re-split), which is what the sweep report here feeds.
* The extension is not just a hint: `dtk` derives the front-end flag from it and passes `-lang=c` for a `.c` object, `-lang=c++` for a `.cpp` one (visible in the real compile command). So a wrong extension is a wrong compiler invocation, and a lib-level `-lang` in `cflags` would be the only way to disagree with it - which is why `unit_verdict` resolves the cflags variable's text and reports that separately.
* **The trap this tool exists to avoid.** Every target object carries a `STT_FILE` symbol whose name is the *configured* source path (`800CCFB0_fn_800CCFB0.c`, `pl_master.cpp`). dtk's split synthesises it from `configure.py`, so it is circular: it echoes the extension we chose and is evidence of nothing. `elf_symbols` returns it, and every consumer here filters `type == 4`/`SHN_ABS` out. (The other near-miss is `-lang`: it lives in a cflags list, so `unit_verdict` resolves the cflags variable's text and reports a flag/verdict disagreement separately from an extension one.)
* Read-only: nothing here writes a file, and nothing here re-splits, builds or links.

## Lib dependencies

binary, names, project.configure, report.

## Test contract

Tier: fixture.
Today's selftest: in-file `selftest()` (`--selftest`).
Target: `tools/tests/units/test_langcheck.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

extab signal is one-directional and silent in libs with exceptions on

## History (the incidents behind the rules - keep the rule, drop the narrative when the rule is stable)

* `docs/plan.md`, "The language comes from the symbol, not from our convenience" (owner's rule, 2026-09-23). A unit is **C++** only on **conclusive** evidence:
