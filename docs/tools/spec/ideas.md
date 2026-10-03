# `ideas` - Find, read, add and check the matching playbook's ideas; `check`/`demo-check` are the docs-batch gate rows selftest.py maps

<!-- generated from the module docstring of `tools/agents/ideas.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

ideas.py - find, read, add and check the matching playbook's ideas (docs/matching/NNN-slug.md).

## Users

the landing gate (2); the selftest runner (14); skills (61); CLAUDE.md (2); docs (63)

## CLI

```
python tools/agents/ideas.py find <words...> [--tag T] [--status S] [--applies V] [-n N]
python tools/agents/ideas.py show N
python tools/agents/ideas.py where N
python tools/agents/ideas.py new --title T --tags a,b [--kind codegen|process] [--slug s] [--applies a,b]
python tools/agents/ideas.py check [--demos]
python tools/agents/ideas.py demo-check [N ...|--all|--changed REF]
python tools/agents/ideas.py --selftest
/* Demo for idea NNN.
* FLAGS: -O4,p -inline auto       flags appended to the unit's base cflags (one line)
* MWCC: Wii/1.3                   compiler; optional, default Wii/1.3
* EXPECT: contains fmuls          repeatable; one assertion about the compiled object per line
* EXPECT: absent fmadds             (vocabulary: ideas_demo.py / docs/matching/README.md)
*/
```
Subcommands: `find`, `show`, `where`, `new`, `check`, `demo-check`.
Flags: `--all`, `--applies`, `--changed`, `--demos`, `--dump`, `--kind`, `--repo`, `--selftest`, `--slug`, `--status`, `--tag`, `--tags`, `--title`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: docs/matching/*.md -> stdout, new idea files.

## Invariants and rules

* `find` ranks ideas by the query words against the title, tags, slug and problem sentence (the problem is written as the symptom, so search by what you see). `show` prints the file and, for an idea with a demo, the demo's path and the compile line. `new` allocates the next free id ATOMICALLY (an exclusive create of a per-id lock, so two lanes racing in one tree cannot take one id), scaffolds the idea (a `codegen` idea also gets a demo `.cpp` with the header below) and regenerates the index and the skill's copy. `check` is the whole gate: front matter schema, unique ids, file names agreeing with ids, the H1 agreeing with the title, index and skill copy fresh, every `demo:` existing, no orphan demo file, every demo header well-formed (`--demos` also compiles them, as `demo-check --all`). `demo-check` compiles each demo with the real MWCC (the base cflags read from configure.py, the demo's FLAGS replacing same-family flags) and tests its EXPECT lines against `objdump -d -r -t -h`; the vocabulary is `ideas_demo.py`'s.
* The parser is `sync_playbook_index.py`'s - this tool imports it and never forks it.
* DEMO HEADER (`NNN-slug.cpp`, one per codegen idea; stage 3 compiles it and checks the EXPECT lines):

## Lib dependencies

text, proc, binary (demo evaluate), repo.

## Test contract

Tier: fixture; smoke: the real tree's index and demos.
Today's selftest (`tools/agents/ideas_selftest.py`): Covers: `find` ranking and filters on fixtures, `new` scaffolding a valid idea (and a codegen demo) that passes `check`, id allocation under a thread race (distinct ids, no lost file), the refusals of `new`, every class `check` must fail on, the demo header parser, and the real tree.
Target: `tools/tests/agents/test_ideas.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

None recorded.
