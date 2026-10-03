# `profileprobe` - Write a recall probe for a subagent profile and print the launch line and checklist

<!-- generated from the module docstring of `tools/agents/profileprobe.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

profileprobe.py - smoke-test subagent profiles and print the checklist to judge them by.

## Users

docs (6)

## CLI

```
python tools/agents/profileprobe.py <agent> [<agent> ...]
```
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: profile -> .pi/probes/*.md.

## Invariants and rules

* A profile is a prompt, so the closest thing to a test has two halves:
* T1 discovery - the file must appear as a project agent (`claude agents`). T2 recall - a reader child that runs nothing and answers, from its own prompt alone, what its job/limits/rules/verification/report are.
* T2 has caught a real defect already: the first `decompiler` draft mis-numbered section 6.5's rule table (rule 1 described as the vtable rule, which is rule 10), and the probe's "rules 5 and 8 are not in my context" is what exposed it.
* writes .pi/probes/probe-<agent>.md, prints the launch command for each, and prints the per-agent checklist, so a profile change can be re-tested by re-running this and reading the replies against it. Behaviour on a real task (T3) is separate and is logged in docs/agent-profile-tests.md.

## Lib dependencies

repo.

## Test contract

Tier: none (manual probe).
No selftest today.
Target: `tools/tests/agents/test_profileprobe.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

None recorded.
