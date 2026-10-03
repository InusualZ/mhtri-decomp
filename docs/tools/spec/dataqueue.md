# `dataqueue` - Write `data-queue.json`: every unowned data run with labels, leak, density and a pre-measurement verdict; `--request` files a data request

<!-- generated from the module docstring of `tools/units/dataqueue.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

Write `tools/units/data-queue.json`: the campaign's unowned data as a queue, not as comments.

## Users

docs (8); imported by `dataclaim`, `slots`

## CLI

```
python tools/units/dataqueue.py                 # write the queue and print a summary
python tools/units/dataqueue.py --dry-run       # report what would be written, write nothing
python tools/units/dataqueue.py --limit 20      # a preview queue (deterministic prefix)
python tools/units/dataqueue.py --json          # the queue on stdout, write nothing
python tools/units/dataqueue.py --request <unit> <addr> [--size N] [--evidence E] [--unblocks R]
python tools/units/dataqueue.py --selftest
```
Flags: `--dry-run`, `--evidence`, `--json`, `--limit`, `--no-seam-cut`, `--out`, `--request`, `--root`, `--section`, `--selftest`, `--size`, `--unblocks`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: map, splits, config.json, tudiscover cache, seams -> data-queue.json, .pi/data-requests.json.

## Invariants and rules

* docs/plan.md 7.17. the retired tiler's data pass wrote the runs it saw into `splits.txt` as `# ... claim in the measured data pass` comments, and nothing reads them back. This tool is the writer for the queue `brief.py` already reads: one entry per **unowned data run**, in the shape the plan specifies,
```
{unit, section, start, end, labels, leak, density, verdict}
```
* with `brief.py`'s `data_queue_entries()` consuming it per unit (it accepts either a bare list or `{"entries": [...]}`; this writer emits the bare list).
* Where the backlog comes from - the same repository state the other tools read, never a stored list:
* the **symbols** (`config/RMHE08/symbols.txt`, through `symbolpreflight.load_symbols`, which goes through `symedit`; the file is 4.5 MB and is never printed or pasted),
* the **owners** (`config/RMHE08/splits.txt`): a symbol is backlog when no claimed range covers its `(section, address)`,
* the **regions** (`build/RMHE08/config.json`, through `ledger.Objects`): which split object covers an unclaimed address, so a run with no referencing unit still gets a stable, honest name,
* the **references** (`build/tmp/tudiscover/graph.json`, the `tudiscover` cache): a data run whose symbols are referenced by exactly one registered unit is attributed to that unit (`Pl/pl_act`), which is what makes the queue useful to `brief.py`. The cache is read, never rebuilt - a stale stamp only drops the reference attribution, it does not trigger the 200-400 s graph walk.
* The selection rule (pure, `select`): a symbol is backlog when it is not a function, its section is a data section (not `.text`, not an `extab`/`extabindex`/`.ctors`/`.dtors` fragment - those travel with the code unit that owns them), and no `splits.txt` range covers it.
* The run grouping (pure, `group_runs`): within one section, symbols are merged while each next address is at or before the current run's end, so a gap starts a new run and a zero-size label never does.
* The verdict is the queue's own, pre-measurement verdict, and it uses the decisions the plan already made: `never` for linker-generated data (`_rom_copy_info`, `_bss_init_info`, §8.4), `owner-held` for the TRK interrupt-vector table (the escalation queue keeps it unowned), `not claimed` when the run leaks across units or a claimed symbol sits inside it (`density < 0.5`), otherwise `proposed`. `dataclaim.py` (7.8) refines `proposed` with the target-vs-ours section measurement.
* Writing is atomic (temp file + `os.replace`) and idempotent: the same repository state renders the same bytes, so re-running cannot churn the file.

## Lib dependencies

project, refs (tudiscover cache), text, seams.

## Test contract

Tier: fixture.
Today's selftest: in-file `selftest()` (`--selftest`).
Target: `tools/tests/units/test_dataqueue.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

None recorded.

## History (the incidents behind the rules - keep the rule, drop the narrative when the rule is stable)

* **The data-claim request channel.** A lane that finds a genuinely unowned range *its own rows need* used to dead-end: rule 12 refuses a bare `extern`, and the brief says do not touch `splits.txt`. `--request` files it instead - address, size, the sole-referencer evidence and the rows it unblocks - into `.pi/data-requests.json` (gitignored, deduplicated, byte-deterministic; `slots.py collect` merges a lane's filings into MAIN's). It is only the **filing channel**: the ruling is the orchestrator's and goes through the `contact_supervisor` protocol. The brief (`brief.py`, section 5d) names the command, so no lane has to guess it.
