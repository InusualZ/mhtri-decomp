---
name: agents-md-local-only
description: Pull and push the LOCAL-ONLY working-state section of AGENTS.md - take it out of the file into a state file before a commit that touches AGENTS.md, and put it back exactly where it was afterwards - through tools/agents/localonly.py. Use before any `git add AGENTS.md` (non-negotiables rule 8 forbids committing that section), after the commit to restore it, and to check a revision for leftover markers.
license: MIT
compatibility: an AGENTS.md with the LOCAL-ONLY-BEGIN / LOCAL-ONLY-END marker lines
metadata:
  author: mhtri-dtk
  tool: tools/agents/localonly.py
---

# Pulling and pushing AGENTS.md's local-only section

The `## Current task / plan` section between the two LOCAL-ONLY markers is live agent working state, and
non-negotiables rule 8 forbids committing it: it would publish scratch state and hand the next session a
stale plan. So a commit that touches `AGENTS.md` needs the section out of the file first, and back in
afterwards.

```sh
python tools/agents/localonly.py pull     # section out of the file, into .pi/local-only.state.json
git add AGENTS.md && git commit ...
python tools/agents/localonly.py push     # section back exactly where it was
```

## Commands

| goal | command |
| --- | --- |
| take the section out | `python tools/agents/localonly.py pull [--dry-run] [--force]` |
| put it back | `python tools/agents/localonly.py push [--dry-run]` |
| what is where | `python tools/agents/localonly.py status` |
| rule 8 check on a revision | `python tools/agents/localonly.py verify [--rev HEAD]` |
| another file / state path | add `--file <path>` / `--state <path>` |

## Why a tool and not sed

The markers are **quoted as text inside rule 8 itself**, further down the same file, so any pattern that is
not anchored at the start of a line matches four times - and a range-based edit (`sed -i '/BEGIN/,/END/d'`)
then silently removes the wrong region. `pull` matches whole marker *lines*, and `push` does not guess: it
stores the removed bytes plus ~240 characters of the text on each side of the cut and refuses to write if
the surrounding text has moved.

## The state file

* Default `.pi/local-only.state.json` - inside the gitignored scratch tree, so it can never be committed.
  It holds the section's text, the two anchors, and the file's hash before the pull.
* `pull` refuses to overwrite an existing state file (`--force` overrides); `push` deletes it after a
  successful restore. A leftover state file means a pull that never got its push.
* After a push the tool reports whether the file is byte-identical to the pre-pull version. "The rest of
  the file changed while pulled" is normal when you kept editing it; the section itself is still restored
  at the recorded anchor.

## Failure modes and what they mean

| message | meaning |
| --- | --- |
| `already holds a pulled section` | you pulled twice, or never pushed the first one |
| `already contains a LOCAL-ONLY section` | the file has one and you are pushing a second - check `status` |
| `the anchor ... matches N times` / `the text after the cut does not match` | the file changed too much since the pull; insert the stored block by hand (`status` prints where the state file is) |
| `no LOCAL-ONLY section - nothing to pull` | the file is already stripped; `push` is what you want |

## Verification

`verify` runs rule 8's own check against a committed revision - `git show <rev>:AGENTS.md | grep -c
'^<!-- LOCAL-ONLY'` must print `0`. Run it after committing AGENTS.md, and before claiming a commit is
clean.
