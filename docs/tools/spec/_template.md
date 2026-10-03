# `<name>` - <one-line purpose>

## Purpose

What the tool does, in two sentences, present tense.

## Users

Who calls it: profiles, skills, docs, the gate, other tools (by name).

## CLI

Usage lines; subcommands; flags; exit codes (0 ok, 1 findings/refusal, 2 could not run); the `--json` schema.

## Inputs and outputs

What it reads and what it writes (files, stdout); nothing written outside the named paths.

## Invariants and rules

One bullet per rule: what is refused, what is never done, what a verdict means. A date in parentheses only where it explains
the rule.

## Lib dependencies

The `lib.*` modules it stands on (and the one tool API it may import, if any).

## Test contract

Tier (fixture / smoke), what the fixture pins, what the smoke check tolerates.

## Known gaps

What it does not do yet, and the question or package that will decide it.
