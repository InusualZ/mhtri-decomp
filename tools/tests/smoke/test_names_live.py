"""names.py on the live tree: the repository's `.gitattributes` carries the registry's union-merge rule, the tracked
registry parses, and every winner in it names a map row's address."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

from tools.lib import testing
from tools.symbols import names as N

TIER = "smoke"


def test_attribute_rule(c):
    attrs = testing.live_root() / ".gitattributes"
    if not attrs.is_file():
        return c.skip("the registry's attribute rule", "no .gitattributes")
    lines = [" ".join(ln.split()) for ln in attrs.read_text(encoding="utf-8").splitlines()]
    c.check("the live .gitattributes merges the registry as a union", " ".join(N.ATTRIBUTE_RULE.split()) in lines, True)


def test_live_registry(c):
    root = str(testing.live_root())
    if not (testing.live_root() / N.REGISTRY_REL).is_file():
        return c.skip("the live registry", "no registry file")
    reg = N.Registry(root, ref="")
    c.check("every winner names an address the map has a row at",
            [hex(e.address) for e in reg.entries if e.status == "winner" and reg.row_at(e.address) is None], [])


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
