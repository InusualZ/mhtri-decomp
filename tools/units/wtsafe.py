"""Remove a worktree without letting a junction reach outside it, and prove `orig/` survived (`lib.lanes.teardown`).
Spec: docs/tools/spec/wtsafe.md. CLI: python tools/units/wtsafe.py [--check] [--unlink DIR] [--selftest];
tests: tools/tests/units/test_wtsafe.py."""
from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import os
import tempfile

from tools.lib import cli
from tools.lib import repo as librepo
from tools.lib.lanes import teardown
from tools.lib.lanes.teardown import is_reparse_point, sha1, unlink_reparse_points  # noqa: F401  (the old names)

TOOL = cli.Tool("wtsafe", "docs/tools/spec/wtsafe.md", tests="tools/tests/units/test_wtsafe.py",
                description=(__doc__ or "").splitlines()[0], common=())
REPO = str(librepo.PACKAGED_ROOT)
CONFIG = os.path.join("config", "RMHE08", "config.yml")
_make_junction = teardown.make_junction


def main_worktree(start: str | None = None) -> str:
    """MAIN as git sees it from `start` (default: this file's tree) - the tree whose `orig/` is pinned."""
    return teardown.main_worktree(start or os.path.dirname(os.path.abspath(__file__)), fallback=REPO)


def remove_worktree(path: str, repo: str | None = None) -> list[str]:
    return teardown.remove_worktree(path, repo)


def ground_truth(repo: str | None = None) -> dict[str, str]:
    return librepo.ground_truth(repo or main_worktree())


def verify_orig(repo: str | None = None, want: dict[str, str] | None = None) -> list[tuple[str, str, str]]:
    return teardown.verify_orig(repo or main_worktree(), want)


def snapshot(repo: str | None = None) -> str:
    return teardown.snapshot(repo or main_worktree())


def main() -> int:
    ap = TOOL.parser()
    ap.add_argument("--check", action="store_true", help="verify orig/ against its pinned hashes")
    ap.add_argument("--unlink", metavar="DIR", help="unlink the reparse points under DIR")
    a = ap.parse_args()
    if a.selftest:
        return TOOL.selftest(cwd=tempfile.gettempdir())
    if a.unlink:
        for p in unlink_reparse_points(a.unlink):
            print("unlinked %s" % p)
        return 0
    main_wt = main_worktree()
    bad = verify_orig(main_wt)
    for rel, want, got in bad:
        print("CHANGED %s want %s got %s" % (rel, want, got))
    print("orig/ (%s): %d pinned file(s), %s" % (main_wt, len(ground_truth(main_wt)), "intact" if not bad else "CHANGED"))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
