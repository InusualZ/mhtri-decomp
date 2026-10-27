"""lib.cextract (and its CLI tools/mwcc-debugger/extract.py): the one-function TU keeps the target and the preamble, reduces every other function to a
declaration (inline/template kept, out-of-class members dropped), keeps every line number and every preprocessor
line, and resolves a mangled dump target to its source identifier."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import json
import subprocess
import tempfile
from pathlib import Path

from tools.lib import testing

TIER = "fixture"
TOOL = Path(__file__).resolve().parents[2] / "mwcc-debugger"
from tools.lib import cextract as ex

UNIT = """#include "unit.h"
/* a comment with a { brace */
static int counter;

struct Box {
    int value;
    int get() const { return value; }
    void set(int v);
};

static int helper(int x)
{
    const char* s = "}";
#if DEBUG
    counter++;
#endif
    return x + 1;
}

inline int twice(int x) { return x * 2; }

template <class T> T pick(T a, T b) { return a < b ? a : b; }

void Box::set(int v)
{
    value = v;
}

int target(int x)
{
    return helper(x) + twice(x);
}

extern "C" {
void after(void)
{
    counter = 0;
}
}
"""


def test_extract(c):
    res = ex.extract(UNIT, "target")
    out = res.text
    c.check("every line keeps its number", out.count("\n"), UNIT.count("\n"))
    c.check("the target is kept", [n.split("@")[0] for n in res.kept], ["target"])
    c.contains("... with its body", out, "    return helper(x) + twice(x);")
    c.check("free functions become declarations", sorted(n.split("@")[0] for n in res.stubbed), ["after", "helper"])
    c.contains("... `static int helper(int x)` ends in `;`", " ".join(out.split()), "static int helper(int x) ;")
    c.expect("... and lose their body", "return x + 1;" not in out and "counter = 0;" not in out)
    c.check("inline and template definitions are kept whole", sorted(n.split("@")[0] for n in res.whole),
            ["pick", "twice"])
    c.check("an out-of-class member definition is dropped", [n.split("@")[0] for n in res.dropped], ["set"])
    c.expect("... entirely", "Box::set" not in out)
    c.contains("a member defined in its class stays", out, "int get() const { return value; }")
    c.expect("a preprocessor line inside a removed body stays (its #if keeps its #endif)",
             "#if DEBUG" in out and "#endif" in out)
    c.contains("the preamble stays", out, '#include "unit.h"')
    c.contains("the linkage block's braces stay", " ".join(out.split()), 'extern "C" { void after(void) ; }')
    c.raises("a target with no definition refuses", SystemExit, ex.extract, UNIT, "missing")


def test_target_identifiers(c):
    c.check("an unmangled name is itself", ex.target_identifiers("lb_arm_wrestle_step"), {"lb_arm_wrestle_step"})
    c.check("a member's stem", ex.target_identifiers("send__16NetworkSingleTcpFPCUcl"), {"send"})
    c.check("a free function's stem", ex.target_identifiers("fn__Fv"), {"fn"})
    c.check("a C name with leading underscores is itself", ex.target_identifiers("__GXCalculateVLim"),
            {"__GXCalculateVLim"})
    c.check("a constructor's class", ex.target_identifiers("__ct__3BoxFv"), {"Box"})
    c.check("a destructor's class under a Q qualifier", ex.target_identifiers("__dt__Q34nw4r3g3d6ResMatFv"), {"ResMat"})


def test_cli(c):
    with tempfile.TemporaryDirectory() as td:
        src = Path(td) / "unit.cpp"
        src.write_text(UNIT, encoding="utf-8")
        out = Path(td) / "x.cpp"
        proc = subprocess.run([sys.executable, str(TOOL / "extract.py"), str(src), "target", "-o", str(out), "--json"],
                              cwd=td, capture_output=True, text=True, encoding="utf-8", errors="replace")
        c.check("the CLI exits 0", proc.returncode, 0)
        c.check("--json names what happened", sorted(json.loads(proc.stdout)), ["dropped", "kept", "stubbed", "whole"])
        c.check("-o writes the TU", out.read_text(encoding="utf-8") == ex.extract(UNIT, "target").text, True)


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
