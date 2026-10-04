"""mwlink against the real linker: the record and alignment derivations read out of the build's mwldeppc.exe.
Skipped (never faked) when capstone or the linker is absent."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
from pathlib import Path

from tools.lib import testing
from tools.lib.binary.pe import Pe
from tools.mwlink.align import derive_alignment
from tools.mwlink.anchors import _capstone
from tools.mwlink.link import default_linker
from tools.mwlink.records import derive_file_record

TIER = "smoke"


_CACHE: dict = {}


def _linker():
    """One `Pe` for the module, so both derivations walk one disassembly (the slow part is capstone's)."""
    if "pe" not in _CACHE:
        linker = default_linker()
        if _capstone() is None:
            _CACHE["pe"] = (None, "capstone is not installed")
        elif linker is None or not Path(linker).exists():
            _CACHE["pe"] = (None, "no mwldeppc.exe under build/compilers")
        else:
            _CACHE["pe"] = (Pe(linker), "")
    return _CACHE["pe"]


def test_file_record(c):
    pe, why = _linker()
    if pe is None:
        c.skip("the input-file record derivation", why)
        return
    der = derive_file_record(pe)
    c.expect("the '.comment' parser is found", der is not None)
    if der is None:
        return
    c.check("the parser's imul gives the 0x2c stride", der["stride"], 0x2C)
    c.check("the record array the parser indexes", der["table_va"], 0x533458)
    c.expect("the comment-version load/store pair is found",
             der["version_store_rva"] is not None and der["version_load_rva"] is not None)
    c.check("the three comment-kind stores", [t for _r, t in der["kind_stores"]],
            ["byte ptr [esi + 0x1e], 0", "byte ptr [esi + 0x1e], 2", "byte ptr [esi + 0x1e], 3"])
    c.check("the flag setters are 4 and 8", {t.split(", ")[-1] for _r, t in der["flag_set"]}, {"4", "8"})


def test_alignment(c):
    pe, why = _linker()
    if pe is None:
        c.skip("the alignment derivation", why)
        return
    al = derive_alignment(pe)
    c.expect("align derives the '*fill*' site", al is not None)
    if al:
        c.expect("the alignment load is a +0x20 load", any("+ 0x20]" in ld["text"] for ld in al["align_loads"]),
                 al["align_loads"])
        c.expect("the round-up is found", len(al["roundups"]) >= 1, al["roundups"])


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
