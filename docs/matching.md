# Matching playbook (moved)

The playbook is now **one file per idea** under [`docs/matching/`](matching/). This page is kept only so every
old citation of `docs/matching.md`, "playbook N" and "row N" still resolves.

* **Start here:** [`matching/README.md`](matching/README.md) - what the playbook is, the loop, how to find an
  idea by symptom, how ideas are recorded.
* **The index:** [`matching/index.md`](matching/index.md) (generated) - id, title, status, tags and the problem
  each idea solves.
* **Idea N is `docs/matching/NNN-<slug>.md`** (three-digit zero-padded id; ids are permanent and never
  renumbered). `python tools/agents/ideas.py where N` prints the path (`ideas.py find <words>` searches).
* The loop is in the README; the tools are in [`matching/toolbox.md`](matching/toolbox.md); ideas ruled out or
  not yet tried are ordinary idea files with `status: ruled-out` / `todo`, in their own sections of the index;
  the two worked examples are in [`matching/examples/`](matching/examples/).
