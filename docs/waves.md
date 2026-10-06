# Waves: the record

One entry per wave, newest first; the process is `docs/pipeline.md` section 14. An entry is written after the wave's
last landing, from the recon reports, `python tools/units/landlog.py` and the review findings. Keep it short: numbers
and commit ids, one line per fact.

## Template

```md
## Wave N (YYYY-MM-DD): <groups>

* **Recon** (<n> lanes): predicted <n> shared files, hot headers <list>, <n> pre-pass batches.
* **Pre-pass**: <commit> <subject>, one line per batch, in landing order.
* **Lanes**: <lane>: <units>, owns <globs>, read-only <globs>; one line per lane.
* **Landings**: <n> attempts, <n> landed, <n> refused (rows: <row> x<n>), <n> conflicts (paths: <list>).
* **Predicted vs happened**: collisions recon predicted / that happened / that it missed (file, lanes, cost).
* **Merger and fixer rounds**: <n>, each with its cause.
* **Review findings by defect class** (`docs/plan.md` 6.5, "Before reporting"): <class> x<n>.
* **Requests produced**: the rule, tool or recon question each surprise turned into.
```

## Wave 1 (2026-10-06): the network stack, the quest system, the nw4r library - in progress

* **Recon**: four read-only lanes (one per group, plus a sizing lane), brief `.pi/tasks/recon-lanes.md`.
* **Pre-pass**, landed alone before any lane, in this order:
  * `7e9ddd8c3` game/font: land font/flfnt and 14 more (font and nw4r::db recuts)
  * `f934ba65c` repo/comments: sweep src/ef (nw4r::ef library units) comments to the unit header template
  * `050438106` repo/comments: sweep src/ef (game effect units, hub headers) comments to the unit header template
  * `9db8fbe62` game/network: land Network/NetworkStreamSink and 5 more (network recuts and folds)
  * `b59bb5041` game/pl: land Pl/pl_coll and 52 more (nw4r declaration moves)
  * `65b341058` game/network: land Network/net_session_close and 25 more (quest renames and moves)
* **Lanes**: launched from `65b341058` with per-lane briefs (`.pi/tasks/decomp-lane-common.md` plus one prompt per lane).
* **Landings, predicted vs happened, rounds, findings**: to be written when the wave's last branch lands.
