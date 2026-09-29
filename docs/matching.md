# Matching playbook (moved)

The playbook is now **one file per idea** under [`docs/matching/`](matching/). This page is kept only so every
old citation of `docs/matching.md`, "playbook N" and "row N" still resolves.

* **Start here:** [`matching/README.md`](matching/README.md) - what the playbook is, the loop, how to find an
  idea by symptom, how ideas are recorded.
* **The index:** [`matching/index.md`](matching/index.md) (generated) - id, title, status, tags and the problem
  each idea solves.
* **Idea N is `docs/matching/NNN-<slug>.md`** (three-digit zero-padded id; ids are permanent and never
  renumbered). `python tools/agents/sync_playbook_index.py --where N` prints the path.
* The loop is in the README; the tools are in [`matching/toolbox.md`](matching/toolbox.md); ideas ruled out or
  not yet tried are in [`matching/ruled-out.md`](matching/ruled-out.md) and [`matching/todo.md`](matching/todo.md);
  the two worked examples are in [`matching/examples/`](matching/examples/).
