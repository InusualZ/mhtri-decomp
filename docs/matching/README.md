# Matching playbook

A playbook for making one translation unit's compiled object match the original object instruction for
instruction. Flags are the *second* thing to look at (source shape is the first), but when the target's code is
systematically "less optimized" than ours they are usually the whole story.

Two rules from `CLAUDE.md` apply throughout: a flag change is only acceptable with concrete evidence
(non-negotiable #3), and proven flags belong in `configure.py` as a **per-library** `cflags_*` override -
never by editing `cflags_base`/`cflags_runtime` for everybody.

## Layout

| path | what it is |
| --- | --- |
| `index.md` | **generated** tables of every idea, grouped by status (works / ruled-out / todo / superseded): id, title, tags, problem - start here to find an idea by symptom |
| `NNN-slug.md` | one idea per file; the three-digit **id is permanent** (700+ citations of "playbook N" / "row N" exist); the slug is 2-5 curated words and may be renamed with `git mv` |
| `NNN-slug.cpp` | optional compilable demo of a codegen idea (header format below) |
| `toolbox.md` | the unit-agnostic tools the ideas use |
| `examples/` | two worked examples of the whole loop (`rso-runtime.md`, `camellia.md`) |
| `notes/` | short unnumbered notes (`paired-single.md`) |

`docs/matching.md` is only an entry page kept so old citations resolve. To open idea N run
`python tools/agents/ideas.py where N` (or `show N`).

## Finding, adding and checking ideas (`tools/agents/ideas.py`, or `mt.py ideas ...` from the skill)

* **Find by symptom.** Read the first divergence (loop step 2 below) and name the code shape: a fused
  instruction, a register colouring, a table base, a missing `extab`, a `bl` that should be kept, a section that
  does not pair. Then `python tools/agents/ideas.py find <words>` ranks ideas by those words against the title,
  tags, slug, applies and problem (which is written as the symptom); `--tag T`, `--status S` (`ruled-out` lists
  what not to re-run, `todo` what nobody tried), `--applies Wii/1.3`, `-n N` narrow or widen it. `index.md` is the
  same data as tables. `ideas.py show N` prints an idea (and its demo's compile line); `ideas.py where N` its path.
* **Add.** `ideas.py new --title "<title>" --tags a,b [--kind codegen|process] [--slug words] [--applies X]`
  allocates the next free id atomically (an exclusive create, so two lanes racing in one tree cannot share an
  id), writes `NNN-slug.md` with the front matter and the Problem / Why it happens / How to work it / Result /
  Example skeleton (status `todo`), for `--kind codegen` a demo `NNN-slug.cpp`, and regenerates `index.md` and
  the skill's copy. Fill the body, set the `problem:` line and `status`, then check.
* **Check.** `ideas.py check` is the whole gate: front-matter schema, ids unique and agreeing with the file
  names, the H1 agreeing with the title, `index.md` and the skill's copy fresh, every `demo:` existing, no orphan
  demo file, every demo header well-formed. (`tools/selftest.py` runs it when `docs/matching/` changes.) `check
  --demos` also compiles every demo (see "Demos" below).
* **Demos.** `ideas.py demo-check [N ...|--all|--changed REF]` compiles each demo with the real MWCC and tests
  its `EXPECT:` lines; it prints PASS/FAIL per demo, and for a failure the EXPECT, why, and the part of the
  object it is about. Exit 0 when all pass, 1 on a failure, 3 when a demo was skipped (no compiler under
  `build/compilers/`); `--changed REF` selects demos whose `.cpp` or idea file changed since REF (the
  landing gate's selftest runner does this whenever `docs/matching/` changes); `--dump` prints the objdump of a
  demo instead of checking it - the way to see what to assert when writing one. All demos: about 2 s.
* For a unit that does not match, walk the ideas in id order too: the ids are the order they were learned.

## How ideas are recorded

* **One file per idea**, `docs/matching/NNN-slug.md`, with the next free id (ids are never renumbered or
  reused) and a curated slug of 2-5 lowercase words. It opens with front matter:

```
---
id: 43
title: <the idea's title>
status: works            # works | ruled-out | todo | superseded
problem: <the Problem sentence, one line>
tags: [flags, source-shape]
applies: [Wii/1.3]       # compiler versions/libs it is known to apply to; [] if unknown
demo:                    # NNN-slug.cpp when a compilable demonstration exists
---
```

  Three OPTIONAL keys follow `demo:` once an idea has been reviewed (a stage-4 pass sets them; absent means
  unreviewed): `reviewed: YYYY-MM-DD` (the day the idea was verified and refined), `related: [2, 44]` (ideas a
  reader should also open; each must exist and not be the idea itself) and `superseded_by: N` (required when
  `status: superseded`; must name an existing idea). Ids are never renumbered: a replaced idea stays and points
  forward.

  followed by a blank line, `# N. Title`, and the body (**Problem. / Why it happens. / How to work it. /
  Result. / Example.**). Record an idea in the session it works: a win that exists only in chat or a scratch
  report is lost at the next compaction.
* **A demo** (`NNN-slug.cpp`, named in `demo:`) is the smallest translation unit that shows a codegen idea. It
  opens with a `/* ... */` header comment in this format (the checker compiles it and tests the `EXPECT:` lines):

```
/* Demo for idea NNN: <title>
 * FLAGS: -O4,p -inline auto        flags appended to the unit's base cflags, one line
 * MWCC: Wii/1.3                    compiler under build/compilers/; optional, default Wii/1.3
 * EXPECT: contains fmuls           repeatable, one assertion about the compiled object per line
 * EXPECT: absent fmadds              (two or more spaces end the assertion; the rest is a note)
 */
```

  **How a demo is compiled.** With the base `cflags_base` read out of `configure.py` (never a copy), in which the
  demo's `FLAGS:` replace every flag of the same family (`-O3` replaces `-O4,p`, `-str reuse,pool` replaces
  `-str reuse`), through `sjiswrap` and the chosen `mwcceppc.exe`, into a scratch directory; the object is read
  with `objdump -d -r -t -h -s`. A **symbol** in an assertion matches exactly or as the C++ mangling prefix
  (`f` finds `f__Fi`). **The vocabulary** (`ideas_demo.py` is the one implementation):

| EXPECT | holds when |
| --- | --- |
| `contains <x>` / `absent <x>` | `<x>` is (not) an instruction mnemonic, a symbol, or a relocation target |
| `size <sym> <bytes>` | the symbol's size |
| `section <name> <bytes>` | the section's size (`0` also holds for a missing section) |
| `order <section> <A> < <B>` | symbol A is at a lower address than B inside the section (data order, ideas 80) |
| `reloc <sym>` | a relocation names `<sym>` |
| `seq <m1> <m2> ... [in <func>]` | the mnemonics occur in this order (a subsequence), in `<func>` if given |
| `count <mnemonic> <n> [in <func>]` | the mnemonic occurs exactly `n` times |
| `insn <mnemonic> <operands> [in <func>]` | an instruction with exactly these operands, e.g. `insn rlwinm r3,r0,0,24,24` |
| `bytes <section> <hex>` / `nobytes ...` | the section's contents do (not) contain the byte string |

  **How to write one.** Give the smallest file that shows the idea, with **both shapes** in it when the idea is
  "write it this way, not that way" (two functions with different names); `extern "C"` keeps the names
  unmangled. Write the header with placeholder EXPECTs, run `ideas.py demo-check N --dump` to see the
  object, then assert the difference: a count or a size per function is sturdier than a whole sequence.
  An assertion must be one that can fail - if the two spellings compile identically in isolation, say so with
  two equal `size` lines rather than pretending a difference. **When the real compiler disagrees with the idea's
  text, the demo records what the compiler did and the idea gets a "Demonstration" note** - a demo is never
  bent to fit the prose.

* **Tags** come from a fixed vocabulary: `flags`, `pragma`, `source-shape`, `allocator`, `data`, `vtable`,
  `linker`, `sections`, `symbols`, `relocations`, `measurement`, `process`, `tooling`. Use the few that are
  certain; an empty or one-tag list is fine.
* **Status**: `works` (has a result), `ruled-out` (tried, does not work - keep it so nobody re-runs it),
  `todo` (not tried yet), `superseded` (a later idea replaces it - say which in the body).
* Use `ideas.py new` to create the file (it allocates the id and runs both syncs). After hand-editing run
  `python tools/agents/sync_playbook_index.py` (regenerates `index.md`;
  it refuses a duplicate id, a bad key or tag, a slug that disagrees with the id, or a missing demo file) and
  `python .claude/skills/mwcc-unit-matching/scripts/sync_reference.py` (refreshes the skill's portable copy
  under `references/matching/`).
* **A merge conflict in a generated file** (`index.md`, or anything under the skill's `references/matching/`) is
  never resolved by hand or by union: take either side, then run the two sync commands above. Two lanes that
  each ran `ideas.py new` in their own trees can also pick the same id - the second to land takes the next free
  id for its new file before landing (`ideas.py check` names the duplicate); a cited id is never renumbered.
* Unit-specific findings (a residual diff, a known-bad flag) are not playbook material: they belong in the
  unit's own header comment.

## The loop

1. **Measure one unit** at instruction level (not the project-wide progress report).
2. **Read the first divergence** and name the *code shape* it implies (a fused instruction, a register save
   idiom, a table base, a prologue).
3. **Classify it**: if the shape is something an optimizer pass or a code-generation switch can produce, it
   is a flag lead; if it is an algorithm or signature difference, it is a source lead.
4. **Isolate**: change exactly one flag, against the *real* command line for that unit.
5. **Verify and land it**: function sizes, per-function match, relocations still resolve, and a metric
   that does not drift. A flag win goes into a per-library `cflags_*` override in `configure.py`, and a
   source win is applied to the unit's own source (`tools/flags/tryvar.py --apply <name>`) and re-measured
   there - the repository must build better, not just the probe. Prove it with the object hash.
The ideas are the individual moves; the worked examples (`examples/`) are the whole loop run once. `index.md` lists every idea with the problem it solves.


## Tools

See [toolbox.md](toolbox.md).
