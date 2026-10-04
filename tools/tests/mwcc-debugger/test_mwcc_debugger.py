"""mwcc-debugger: the port is reproducible from upstream by make_port.py, and versions.py identifies a compiler
build (and refuses a mismatched one) through lib.binary.pe, on PeBuilder images."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import importlib.util
import subprocess
import tempfile
from pathlib import Path

from tools.lib import testing
from tools.lib.binary.build import PeBuilder

TIER = "fixture"
TOOL = Path(__file__).resolve().parents[2] / "mwcc-debugger"


def load_versions():
    spec = importlib.util.spec_from_file_location("mwcc_versions", TOOL / "versions.py")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module  # dataclasses resolve their string annotations through sys.modules
    spec.loader.exec_module(module)
    return module


def test_port_is_reproducible(c):
    with tempfile.TemporaryDirectory() as td:
        out = Path(td) / "mwcc_debugger.py"
        proc = subprocess.run([sys.executable, str(TOOL / "make_port.py"), str(TOOL / "upstream" / "mwcc_debugger.py"),
                               str(out)], cwd=td, capture_output=True, text=True, encoding="utf-8",
                              errors="replace")
        c.check("make_port.py runs", proc.returncode, 0)
        committed = (TOOL / "mwcc_debugger.py").read_text(encoding="utf-8")
        regenerated = out.read_text(encoding="utf-8") if out.exists() else ""
        c.expect("the committed port is make_port.py's output, byte for byte", committed == regenerated,
                 "mwcc_debugger.py differs from `make_port.py upstream/mwcc_debugger.py` - edit make_port.py, "
                 "then regenerate")
        c.contains("the port carries the repository prologue (versions.py needs tools/lib inside gdb)",
                   committed.splitlines(), 'import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path('
                   '__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))')


def wii13_image(versions, symbols=True, shift=0):
    """A PE carrying the Wii/1.3 probe signature, and (optionally) a CodeView blob naming the row's data symbols."""
    name, rva, sig = next(p for p in versions._PROBES if p[0] == "Wii/1.3")
    row = versions.row("Wii/1.3")
    b = PeBuilder()
    text_va = rva & ~0xFFF
    b.section(".text", bytes(rva - text_va) + sig, va=text_va)
    want = {sym: row[field] for field, sym in versions._WII13_SYMBOLS.items()}
    lo = min(want.values()) & ~0xFFF
    hi = max(want.values()) + 0x100
    b.section(".data", b"", vsize=hi - lo, va=lo)
    if symbols:
        b.codeview([(".data", addr - lo + (shift if i == 0 else 0), sym) for i, (sym, addr) in enumerate(want.items())])
    return b


def test_detect(c):
    versions = load_versions()
    with tempfile.TemporaryDirectory() as td:
        td = Path(td)
        exe = wii13_image(versions).write(td / "wii13.exe")
        v = versions.detect(exe)
        c.check("the probe identifies Wii/1.3", v.name if v else None, "Wii/1.3")
        row = versions.row("Wii/1.3")
        c.check("the row comes back as absolute VAs", v.opcodeinfo_addr if v else None,
                row["opcodeinfo_addr"] + 0x400000)
        c.check("without a symbol blob the row is taken as is", (versions.detect(
            wii13_image(versions, symbols=False).write(td / "bare.exe")) or None).name, "Wii/1.3")
        bad = wii13_image(versions, shift=4).write(td / "moved.exe")
        err = c.raises("a symbol the blob puts elsewhere refuses the row, loudly", SystemExit, versions.detect, bad)
        c.contains("... naming the symbol", str(err), "is at")
        other = PeBuilder().section(".text", b"\x90" * 16).write(td / "other.exe")
        c.check("an unknown build is None", versions.detect(other), None)
        arm = PeBuilder(machine=0x01C0).section(".text", b"\x90").write(td / "arm.exe")
        c.check("a non-i386 image is None", versions.detect(arm), None)
        c.check("a missing file is None", versions.detect(td / "nope.exe"), None)


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
