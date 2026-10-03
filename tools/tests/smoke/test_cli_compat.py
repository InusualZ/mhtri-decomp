"""Every invocation in docs/tools/migration.md's "CLI compatibility list" still parses: the tool answers --help (or
its usage line), every documented subcommand is offered, every documented flag appears in a help text.
Built from the list itself, so a line added there is covered here; retired tools are skipped by name.
"""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import concurrent.futures
import os
import re
import subprocess
from dataclasses import dataclass, field

from tools.lib import testing

TIER = "smoke"
LIST_DOC = "docs/tools/migration.md"
LIST_HEADING = "## CLI compatibility list"
#: Retired by the splits-program retirement (docs/tools/retired.md); their lines leave the list with that batch.
RETIRED = {"tools/splits/applysplits.py", "tools/splits/dataattach.py", "tools/splits/matchinggain.py",
           "tools/units/attribute.py"}
#: Tools that do not parse `--help`: probed with the argv that prints their usage instead.
USAGE_PROBE = {"tools/elf/elfsect.py": []}
TIMEOUT = 120


@dataclass
class Invocation:
    """One line of the list: the tool path, the subcommands and the flags it documents."""
    path: str
    subcommands: list = field(default_factory=list)
    flags: list = field(default_factory=list)
    line: str = ""


def parse_list(text: str) -> list[Invocation]:
    """The fenced block under the list's heading, one `Invocation` per line."""
    start = text.index(LIST_HEADING)
    block = text[start:].split("```", 2)[1]
    out = []
    for raw in block.splitlines():
        line = re.sub(r"\((?:no args|until [^)]*)\)", " ", raw.strip())
        if not line:
            continue
        line = line.replace("[", " ").replace("]", " ").replace("|", " | ")
        toks = line.split()
        inv = Invocation(toks[0], line=raw.strip())
        after_flag = False
        alt_start = True
        for tok in toks[1:]:
            if tok == "|":
                alt_start, after_flag = True, False
                continue
            if tok.startswith("-"):
                if re.fullmatch(r"--?[A-Za-z][\w-]*", tok) and tok not in inv.flags:
                    inv.flags.append(tok)
                after_flag, alt_start = True, False
                continue
            placeholder = "<" in tok or tok == "..." or "/" in tok
            if alt_start and not placeholder and not after_flag and tok not in inv.subcommands:
                inv.subcommands.append(tok)
            alt_start, after_flag = False, False
        out.append(inv)
    return out


def _run(root, argv):
    p = subprocess.run([sys.executable, *argv], cwd=str(root), capture_output=True, text=True, encoding="utf-8",
                       errors="replace", timeout=TIMEOUT, stdin=subprocess.DEVNULL)
    return p.returncode, (p.stdout or "") + (p.stderr or "")


def probe(root, inv: Invocation) -> list[str]:
    """The problems of one invocation (empty when it still parses as documented)."""
    probe_argv = USAGE_PROBE.get(inv.path, ["--help"])
    rc, top = _run(root, [inv.path, *probe_argv])
    problems = []
    usage = "usage" in top.lower() and "Traceback" not in top
    if not (rc == 0 or usage) or "Traceback" in top:
        problems.append("`%s %s` exit %s: %s" % (inv.path, " ".join(probe_argv), rc, top.strip()[-300:]))
        return problems
    texts = [top]
    for sub in inv.subcommands:
        src, stext = _run(root, [inv.path, sub, "--help"])
        texts.append(stext)
        offered = src == 0 or re.search(r"(?<![\w-])%s(?![\w-])" % re.escape(sub), top)
        if not offered or "Traceback" in stext:
            problems.append("subcommand `%s` is not offered (exit %s): %s" % (sub, src, stext.strip()[-200:]))
    joined = "\n".join(texts)
    for flag in inv.flags:
        if not re.search(r"(?<![\w-])%s(?![\w-])" % re.escape(flag), joined):
            problems.append("flag `%s` appears in no help text" % flag)
    return problems


def test_parse_list_on_fixture(c):
    text = ("x\n%s\n\nprose\n\n```\n"
            "tools/a.py rename|find|show\n"
            "tools/b.py --diff <ref> | --unit <X> | (no args)\n"
            "tools/c.py spawn --kind <X> | init | collect --path <X> --release\n"
            "tools/d.py [--changed [main]] [--json]\n"
            "tools/e.py build/RMHE08/obj/<X>.o -f <X>\n"
            "tools/f.py plan <X> <X>        (until question 1 is decided)\n"
            "```\nafter\n```\nnot this\n```\n") % LIST_HEADING
    got = [(i.path, i.subcommands, i.flags) for i in parse_list(text)]
    c.check("the list's grammar: subcommands, flags, placeholders, flag values, comments", got, [
        ("tools/a.py", ["rename", "find", "show"], []),
        ("tools/b.py", [], ["--diff", "--unit"]),
        ("tools/c.py", ["spawn", "init", "collect"], ["--kind", "--path", "--release"]),
        ("tools/d.py", [], ["--changed", "--json"]),
        ("tools/e.py", [], ["-f"]),
        ("tools/f.py", ["plan"], []),
    ])


def test_documented_invocations_parse(c):
    root = testing.live_root()
    doc = root / LIST_DOC
    if not doc.is_file():
        c.skip("cli compat", "%s is absent" % LIST_DOC)
        return
    invs = parse_list(doc.read_text(encoding="utf-8"))
    c.expect("the list is not empty", len(invs) > 20, "parsed %d lines" % len(invs))
    todo = []
    for inv in invs:
        if inv.path in RETIRED:
            print("skip - %s: retired (docs/tools/retired.md)" % inv.path)
        elif not (root / inv.path).is_file():
            c.skip(inv.path, "absent in this checkout (a submodule or a removed tool)")
        else:
            todo.append(inv)
    with concurrent.futures.ThreadPoolExecutor(max_workers=min(8, os.cpu_count() or 4)) as pool:
        results = list(pool.map(lambda inv: (inv, probe(root, inv)), todo))
    for inv, problems in results:
        if problems:
            c.fail(inv.path, "; ".join(problems))
        else:
            c.expect(inv.path, True)


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
