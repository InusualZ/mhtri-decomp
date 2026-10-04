"""The subprocess codec rule on the live tree: no `tools/**` call decodes text without pinning `encoding=` (F34)."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

from tools.lib import proc, testing

TIER = "smoke"


def test_no_trap_sites(c):
    sites = proc.trap_sites(str(testing.live_root()))
    c.check("no text-mode subprocess call under tools/ leaves the codec to the host locale", sites, [])


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
