# `lane-manifest` - a lane's declared file ownership, and the gate row that holds a batch to it

## Purpose

A lane with no queue claim (a multi-unit lane, a tooling lane, a sweep lane) declares the files it may edit in a
manifest; `land.py land --manifest <path|slug>` adds the gate row "the batch touches only the lane's manifest", which
refuses any changed path outside the manifest's `owns` or inside its `read_only`. Without `--manifest` the gate is
unchanged.

## Users

The orchestrator (it writes or approves the manifest when it launches a wave's lanes, `docs/pipeline.md` section 14,
and names it at landing); the lane (it copies the manifest into its outbox and stays inside it); the gate
(`tools/units/landing/rows/manifest.py`); `tools/units/landlog.py` (the id on the log line).

## CLI

```
python tools/units/land.py land --branch B --manifest <path|slug> [...]
python tools/units/land.py land --units a,b --manifest <path|slug> [...]
python tools/units/land.py verify --dry-run --manifest <path|slug> [...]
```
`<path>`: a JSON file (absolute, or relative to MAIN). `<slug>`: `MAIN/.pi/outbox/<slug>.json`, the lane's outbox as
`slots.py collect` copied it (a `worker/` prefix is dropped, so the branch name works). A file with a top-level
`manifest` key yields that object; any other JSON object is the manifest itself. Exit codes are the gate's.

## Inputs and outputs

The manifest, read-only:

```json
{"manifest": {"lane": "net-l2", "base": "65b341058",
              "owns": ["src/Network/NetworkSession*", "src/unsplit/network.h", "configure.py", "config/RMHE08/*.txt"],
              "read_only": ["src/types.h", "src/nw4r/", "src/Network/network_state.h"],
              "units": ["Network/NetworkSessionManager", "Network/NetworkSessionStable"]}}
```

* `lane` (required): the lane's name, printed by the row. `base`: the commit the lane was cut from (recorded, not
  compared). `units`: the units the lane works (recorded). `owns` (required, non-empty) and `read_only`: lists of
  repo-relative entries.
* An entry with `*`, `?` or `[` is an `fnmatch` glob (`*` crosses `/`); any other entry is a directory (everything
  below it, trailing `/` optional) or the exact path.
* Output: one row in the gate table; the landing log line's `manifest` key (`lib.lanes.landlog.Attempt.manifest`); a
  `manifest: <id>` line in the commit body.

## Invariants and rules

* A path inside a `read_only` entry refuses even when an `owns` entry also matches it: read-only wins.
* A path no `owns` entry matches refuses. The paths judged are the gate's changed paths (both sides of a rename),
  minus the tolerated scratch; gitignored paths (`.pi/`, `build/`) are never in a batch.
* The out-of-manifest refusal is GATE: the batch edits a file its lane does not own. The remedy is to move the edit
  into an integrator request for the file's owner (`docs/tools/spec/integrate.md`), or to extend the manifest with
  the orchestrator's approval and re-run.
* A manifest that is missing, not JSON, or malformed (`lane` empty, `owns` empty or not a list of strings) is
  BOOKKEEPING: the batch may be fine, the landing's own input is not.
* No `--manifest`, no row: a claim lane keeps today's gate, and the golden table of every older scenario is unchanged.
* The manifest grants nothing: it only narrows what the other rows already allow (`landing.common.outside_batch`).

## Lib dependencies

None beyond the gate's (`landing.common`, `landing.state`); `fnmatch` and `json` from the standard library.

## Test contract

Tier: fixture. `tools/tests/units/landing/test_gate_golden.py`: `manifest-inside-pass` (a unit batch inside `owns`
passes) and `manifest-read-only-hot-header-refusal` (the same batch with its hot header `src/Net/net.h` read-only
refuses, naming the header and the remedy) in the golden table; `test_manifest_decision` (directory/glob/exact
matching, read-only over owns, an unowned path, the shape problems, slug/outbox/bare-file loading, a missing file);
`test_landlog_hook.py::test_manifest_reaches_the_log_and_the_body`. Mutation-checked (2026-10-06): deleting the row
from `PRE_BUILD` fails 6 checks, ignoring `read_only` fails 5, ignoring `owns` fails 1.

## Known gaps

The manifest is not cross-checked against other live lanes' manifests (two lanes owning one file is caught at the
second landing as a conflict, not at launch); `base` is recorded, not compared; `slots.py spawn` does not write a
manifest yet - the orchestrator writes it into the lane's brief and the lane copies it into its outbox.
