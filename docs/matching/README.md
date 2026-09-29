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
| `index.md` | **generated** table of every idea: id, title, status, tags, problem - start here to find an idea by symptom |
| `NNN-slug.md` | one idea per file; the three-digit **id is permanent** (600+ citations of "playbook N" / "row N" exist) |
| `toolbox.md` | the unit-agnostic tools the ideas use |
| `ruled-out.md`, `todo.md` | ideas tried and dropped without a section, and ideas not tried yet |
| `examples/` | two worked examples of the whole loop (`rso-runtime.md`, `camellia.md`) |
| `notes/` | short unnumbered notes (`paired-single.md`) |

`docs/matching.md` is only an entry page kept so old citations resolve. To open idea N run
`python tools/agents/sync_playbook_index.py --where N` (or `--json` for the parsed index).

## Finding an idea by symptom

1. Read the first divergence (loop step 2 below) and name the code shape: a fused instruction, a register
   colouring, a table base, a missing `extab`, a `bl` that should be kept, a section that does not pair.
2. Scan `index.md` - the `problem` column is written as the symptom - or filter by **tag**
   (`python tools/agents/sync_playbook_index.py --json` and select on `tags`).
3. Open the idea file; each one is Problem / Why it happens / How to work it / Result / Example.
4. Walk the ideas in id order for a unit that does not match: the ids are the order they were learned.

## How ideas are recorded

* **One file per idea**, `docs/matching/NNN-slug.md`, with the next free id (ids are never renumbered or
  reused) and a slug of at most six lowercase words. It opens with front matter:

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

  followed by a blank line, `# N. Title`, and the body (**Problem. / Why it happens. / How to work it. /
  Result. / Example.**). Record an idea in the session it works: a win that exists only in chat or a scratch
  report is lost at the next compaction.
* **Tags** come from a fixed vocabulary: `flags`, `pragma`, `source-shape`, `allocator`, `data`, `vtable`,
  `linker`, `sections`, `symbols`, `relocations`, `measurement`, `process`, `tooling`. Use the few that are
  certain; an empty or one-tag list is fine.
* **Status**: `works` (has a result), `ruled-out` (tried, does not work - keep it so nobody re-runs it),
  `todo` (not tried yet), `superseded` (a later idea replaces it - say which in the body).
* After adding or editing a file run `python tools/agents/sync_playbook_index.py` (regenerates `index.md`;
  it refuses a duplicate id, a bad key or tag, a slug that disagrees with the id, or a missing demo file) and
  `python .claude/skills/mwcc-unit-matching/scripts/sync_reference.py` (refreshes the skill's portable copy
  under `references/matching/`).
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
