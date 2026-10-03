# GitHub Actions

**CI is not configured in this repository.** The template ships an example workflow (`.github.example/workflows/build.yml`)
plus a build-container repository (`encounter/dtk-template-build`) that stores the original game files privately so a hosted
runner can build. This project removed the example and does not use either: the original `orig/RMHE08/sys/main.dol` is
never committed (CLAUDE.md non-negotiable 2) and the verification is local.

* **The check that stands in for CI** is the landing gate, `python tools/units/land.py land --branch worker/<slug>`
  (`docs/pipeline.md` section 4), which runs the build, `ninja build/RMHE08/ok`, the tool suite
  (`python tools/selftest.py`) and the style gate before anything reaches `main`.
* **To add CI later**, follow the upstream template's instructions
  (<https://github.com/encounter/dtk-template/blob/main/docs/github_actions.md>): a private build-container repository
  holding the assets, a `.github/workflows/build.yml` that builds in it, and an optional <https://decomp.dev> entry.
  That is an owner decision, because it places the game's assets on a third party's runner.
