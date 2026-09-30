#!/usr/bin/env python3
"""ideas.py - find, read, add and check the matching playbook's ideas (docs/matching/NNN-slug.md).

    python tools/agents/ideas.py find <words...> [--tag T] [--status S] [--applies V] [-n N]
    python tools/agents/ideas.py show N
    python tools/agents/ideas.py where N
    python tools/agents/ideas.py new --title T --tags a,b [--kind codegen|process] [--slug s] [--applies a,b]
    python tools/agents/ideas.py check [--demos]
    python tools/agents/ideas.py demo-check [N ...|--all|--changed REF]
    python tools/agents/ideas.py --selftest

`find` ranks ideas by the query words against the title, tags, slug and problem sentence (the problem is written
as the symptom, so search by what you see). `show` prints the file and, for an idea with a demo, the demo's
path and the compile line. `new` allocates the next free id ATOMICALLY (an exclusive create of a per-id lock,
so two lanes racing in one tree cannot take one id), scaffolds the idea (a `codegen` idea also gets a demo
`.cpp` with the header below) and regenerates the index and the skill's copy. `check` is the whole gate: front
matter schema, unique ids, file names agreeing with ids, the H1 agreeing with the title, index and skill copy
fresh, every `demo:` existing, no orphan demo file, every demo header well-formed (`--demos` also compiles them,
as `demo-check --all`). `demo-check` compiles each demo with the real MWCC (the base cflags read from
configure.py, the demo's FLAGS replacing same-family flags) and tests its EXPECT lines against
`objdump -d -r -t -h`; the vocabulary is `ideas_demo.py`'s.

The parser is `sync_playbook_index.py`'s - this tool imports it and never forks it.

DEMO HEADER (`NNN-slug.cpp`, one per codegen idea; stage 3 compiles it and checks the EXPECT lines):

    /* Demo for idea NNN.
     * FLAGS: -O4,p -inline auto       flags appended to the unit's base cflags (one line)
     * MWCC: Wii/1.3                   compiler; optional, default Wii/1.3
     * EXPECT: contains fmuls          repeatable; one assertion about the compiled object per line
     * EXPECT: absent fmadds             (vocabulary: ideas_demo.py / docs/matching/README.md)
     */
"""
import argparse
import os
import re
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import sync_playbook_index as spi  # noqa: E402
import ideas_demo  # noqa: E402

DEFAULT_MWCC = "Wii/1.3"
SLUG_RE = re.compile(r"^[a-z0-9]+(?:-[a-z0-9]+)*$")
DEMO_KEYS = ("FLAGS", "MWCC", "EXPECT")
SKILL_SYNC = ".claude/skills/mwcc-unit-matching/scripts/sync_reference.py"

SCAFFOLD = """
**Problem.** <the symptom, one sentence: what the diff shows and why it is misleading>

**Why it happens.** <what the compiler/linker/tooling does that produces it>

**How to work it.** <the steps; the one lever to pull, against the real command line for the unit>

**Result.** <measured evidence: unit, before/after numbers, the sha1 or the score - or "not yet measured">

**Example.**

```
<the smallest source or flag spelling that shows it>
```
"""

DEMO_SKELETON = """/* Demo for idea {n}: {title}
 * FLAGS: -O4,p -inline auto
 * MWCC: Wii/1.3
 * EXPECT: contains TODO_symbol    what the fix must produce (a mnemonic or symbol)
 * EXPECT: absent TODO_symbol      what the wrong shape produces
 */

/* The smallest translation unit that shows the idea. */
int demo_{under}(int x)
{{
    return x;
}}
"""


# ---- loading -------------------------------------------------------------------------------------------------

def load(root):
    ideas, defects = spi.load_ideas(root)
    return ideas, defects


def idea_path(root, idea):
    return os.path.join(root, *spi.DIR_REL.split("/"), idea["file"])


def demo_header(text):
    """({FLAGS, MWCC, EXPECT: [..]}, [defects]) from the first comment block of a demo file."""
    m = re.match(r"\s*/\*(.*?)\*/", text, re.S)
    if not m:
        return None, ["no opening `/* ... */` header comment"]
    info, defects = {"EXPECT": []}, []
    for line in m.group(1).splitlines():
        km = re.match(r"^[\s*]*(FLAGS|MWCC|EXPECT):\s*(.*?)\s*$", line)
        if not km:
            continue
        k, v = km.groups()
        if k == "EXPECT":
            # a trailing "   explanation" after two or more spaces is prose; the assertion is what precedes it
            info["EXPECT"].append(re.split(r"\s{2,}", v)[0])
        elif k in info:
            defects.append("`%s:` appears twice" % k)
        else:
            info[k] = re.split(r"\s{2,}", v)[0]
    if "FLAGS" not in info:
        defects.append("no `FLAGS:` line")
    if not info["EXPECT"]:
        defects.append("no `EXPECT:` line")
    for e in info["EXPECT"]:
        if not ideas_demo.expect_ok(e):
            defects.append("EXPECT `%s` is not one of: %s" % (e, ideas_demo.EXPECT_HELP))
    return info, defects


# ---- find / show / where -------------------------------------------------------------------------------------

def words(s):
    return [w for w in re.findall(r"[a-z0-9_.$@+-]+", s.lower()) if w]


def score(idea, qwords):
    """Weighted word hits: title 3, tag 3, slug 2, applies 2, problem 1 (substring, so `reloc` finds `relocations`)."""
    fields = ((" ".join(words(idea["title"])), 3), (" ".join(idea["tags"]), 3), (idea["slug"].replace("-", " "), 2),
              (" ".join(idea["applies"]).lower(), 2), (" ".join(words(idea["problem"])), 1))
    total = 0
    for w in qwords:
        hit = False
        for text, weight in fields:
            n = text.count(w)
            if n:
                total += weight * min(n, 2)
                hit = True
        if not hit:
            total -= 1
    return total


def cmd_find(root, a, out=None):
    out = out or sys.stdout
    ideas, defects = load(root)
    if defects:
        for d in defects:
            print("refusing: %s" % d, file=sys.stderr)
        return 1
    qwords = [w for q in a.words for w in words(q)]
    rows = []
    for i in ideas:
        if a.tag and a.tag not in i["tags"]:
            continue
        if a.status and i["status"] != a.status:
            continue
        if a.applies and not any(a.applies.lower() in x.lower() for x in i["applies"]):
            continue
        s = score(i, qwords) if qwords else 0
        if qwords and s <= 0:
            continue
        rows.append((-s, i["id"], i))
    rows.sort(key=lambda r: (r[0], r[1]))
    if not rows:
        print("no idea matches", file=sys.stderr)
        return 1
    for _s, _n, i in rows[:a.limit]:
        out.write("%3d  %-9s %s  [%s]\n     %s/%s\n" % (i["id"], i["status"], i["title"], ", ".join(i["tags"]),
                                                         spi.DIR_REL, i["file"]))
    return 0


def find_idea(ideas, n):
    for i in ideas:
        if i["id"] == n:
            return i
    return None


def cmd_where(root, a, out=None):
    out = out or sys.stdout
    ideas, defects = load(root)
    if defects:
        for d in defects:
            print("refusing: %s" % d, file=sys.stderr)
        return 1
    i = find_idea(ideas, a.n)
    if not i:
        print("no idea %d in %s (ids: %d-%d)" % (a.n, spi.DIR_REL, ideas[0]["id"], ideas[-1]["id"]), file=sys.stderr)
        return 1
    out.write("%s/%s\n" % (spi.DIR_REL, i["file"]))
    return 0


def compile_line(info, demo_rel):
    mw = info.get("MWCC", DEFAULT_MWCC)
    return ("build/compilers/%s/mwcceppc.exe <the unit's base cflags: `python tools/unitutil.py info -u <unit>`> %s "
            "-c %s -o build/tmp/demo.o" % (mw, info.get("FLAGS", ""), demo_rel)).replace("  ", " ")


def cmd_show(root, a, out=None):
    out = out or sys.stdout
    ideas, defects = load(root)
    if defects:
        for d in defects:
            print("refusing: %s" % d, file=sys.stderr)
        return 1
    i = find_idea(ideas, a.n)
    if not i:
        print("no idea %d" % a.n, file=sys.stderr)
        return 1
    out.write(spi.read(idea_path(root, i)))
    if i["demo"]:
        demo_rel = "%s/%s" % (spi.DIR_REL, i["demo"])
        info, dd = demo_header(spi.read(os.path.join(root, *demo_rel.split("/"))))
        out.write("\n---\ndemo: %s\n" % demo_rel)
        if info:
            out.write("flags: %s   mwcc: %s\n" % (info.get("FLAGS", ""), info.get("MWCC", DEFAULT_MWCC)))
            for e in info["EXPECT"]:
                out.write("expect: %s\n" % e)
            out.write("compile: %s\n" % compile_line(info, demo_rel))
        for d in dd:
            out.write("demo header defect: %s\n" % d)
    return 0


# ---- new -----------------------------------------------------------------------------------------------------

def taken_ids(d):
    """Every id in use: an idea file `NNN-*` or a reservation `.NNN.lock`."""
    ids = set()
    for name in os.listdir(d):
        m = re.match(r"^(\d{3})-", name) or re.match(r"^\.(\d{3})\.lock$", name)
        if m:
            ids.add(int(m.group(1)))
    return ids


def reserve_id(d):
    """The next free id, reserved by an exclusive create of `.NNN.lock` (returns (id, lock path)).

    `open(..., "x")` fails for the second racer, who rescans and takes the next id; a name-only exclusive
    create of `NNN-slug.md` would not do, because two racers with different slugs create different files.
    """
    while True:
        n = max(taken_ids(d) or {0}) + 1
        lock = os.path.join(d, ".%03d.lock" % n)
        try:
            fd = os.open(lock, os.O_CREAT | os.O_EXCL | os.O_WRONLY)
        except FileExistsError:
            continue
        os.close(fd)
        return n, lock


def slug_of(title, taken_slugs=()):
    t = title.lower().replace("'", "").replace("`", "")
    ws = re.findall(r"[a-z0-9]+", t)
    while len(ws) > 1 and ws[0] in ("a", "an", "the"):
        ws = ws[1:]
    slug = "-".join(ws[:4]) or "idea"
    base, k = slug, 2
    while slug in taken_slugs:
        slug = "%s-%d" % (base, k)
        k += 1
    return slug


def new_idea(root, title, tags, kind="process", slug=None, applies=(), sync=True):
    """Scaffold a new idea; returns (id, path). Raises SystemExit naming the refusal."""
    for t in tags:
        if t not in spi.TAGS:
            raise SystemExit("refusing: unknown tag `%s` (vocabulary: %s)" % (t, ", ".join(spi.TAGS)))
    if kind not in ("codegen", "process"):
        raise SystemExit("refusing: --kind must be codegen or process")
    if not title.strip() or "\n" in title:
        raise SystemExit("refusing: --title must be one non-empty line")
    d = os.path.join(root, *spi.DIR_REL.split("/"))
    if not os.path.isdir(d):
        raise SystemExit("refusing: %s does not exist" % spi.DIR_REL)
    slug = slug or slug_of(title)
    if not SLUG_RE.match(slug):
        raise SystemExit("refusing: slug `%s` is not lowercase-words-with-hyphens" % slug)
    n, lock = reserve_id(d)
    md = os.path.join(d, "%03d-%s.md" % (n, slug))
    demo_name = "%03d-%s.cpp" % (n, slug)
    try:
        fm = ["---", "id: %d" % n, "title: %s" % title, "status: todo",
              "problem: <one line: the symptom, as the diff shows it>", "tags: [%s]" % ", ".join(tags),
              "applies: [%s]" % ", ".join(applies), "demo: %s" % (demo_name if kind == "codegen" else ""), "---",
              "", "# %d. %s" % (n, title)]
        text = spi.NL.join(fm) + spi.NL + SCAFFOLD
        with open(md, "x", encoding="utf-8", newline="") as f:
            f.write(text)
        if kind == "codegen":
            with open(os.path.join(d, demo_name), "x", encoding="utf-8", newline="") as f:
                f.write(DEMO_SKELETON.format(n=n, title=title, under=slug.replace("-", "_")))
    finally:
        os.remove(lock)
    if sync:
        sync_generated(root)
    return n, md


def sync_generated(root):
    """Regenerate index.md and (when the skill is present) its copy."""
    ideas, defects = load(root)
    if not defects:
        spi.write(os.path.join(root, *spi.TARGET_REL.split("/")), spi.build_index(ideas))
    script = os.path.join(root, *SKILL_SYNC.split("/"))
    if os.path.isfile(script):
        subprocess.call([sys.executable, script], cwd=root)


def cmd_new(root, a, out=None):
    out = out or sys.stdout
    tags = [t.strip() for t in a.tags.split(",") if t.strip()] if a.tags else spi.derive_tags(a.title, "")
    applies = [t.strip() for t in a.applies.split(",") if t.strip()] if a.applies else []
    n, md = new_idea(root, a.title, tags, kind=a.kind, slug=a.slug, applies=applies)
    out.write("created %s (id %d, tags: %s)\n" % (os.path.relpath(md, root).replace(os.sep, "/"), n,
                                                   ", ".join(tags) or "none"))
    out.write("fill in the front matter `problem:` line and the body; `python tools/agents/ideas.py check` "
              "before you commit\n")
    return 0


# ---- check ---------------------------------------------------------------------------------------------------

def check_root(root, skill=True):
    """[defects] for the whole playbook: everything `check` promises."""
    ideas, defects = load(root)
    out = list(defects)
    d = os.path.join(root, *spi.DIR_REL.split("/"))
    demos = set()
    for i in ideas:
        text = spi.read(idea_path(root, i))
        _fm, body = spi.parse_front_matter(text)
        want = "# %d. %s" % (i["id"], i["title"])
        h1 = next((ln.rstrip("\r") for ln in body.split("\n") if ln.startswith("# ")), None)
        if h1 != want:
            out.append("%s: the H1 is %r, want %r" % (i["file"], h1, want))
        if i["demo"]:
            demos.add(i["demo"])
            path = os.path.join(d, i["demo"])
            if os.path.isfile(path):
                _info, dd = demo_header(spi.read(path))
                out.extend("%s: demo %s: %s" % (i["file"], i["demo"], x) for x in dd)
    if os.path.isdir(d):
        for name in sorted(os.listdir(d)):
            if re.match(r"^\d{3}-.*\.(cpp|c|h)$", name) and name not in demos:
                out.append("%s: a demo file no idea's `demo:` names" % name)
            if re.match(r"^\.\d{3}\.lock$", name):
                out.append("%s: a stale id reservation (a crashed `ideas.py new`); delete it" % name)
    if not defects:
        target = os.path.join(root, *spi.TARGET_REL.split("/"))
        have = spi.read(target) if os.path.isfile(target) else None
        if have != spi.build_index(ideas):
            out.append("%s is stale - run python tools/agents/sync_playbook_index.py" % spi.TARGET_REL)
    if skill:
        script = os.path.join(root, *SKILL_SYNC.split("/"))
        if os.path.isfile(script):
            p = subprocess.run([sys.executable, script, "--check"], cwd=root, capture_output=True, text=True, encoding="utf-8", errors="replace")
            if p.returncode != 0:
                out.append("the skill's references/matching copy is stale - run python %s" % SKILL_SYNC)
    return out, ideas


def demo_files(root, ideas):
    """[(idea, absolute demo path)] for every idea with a demo, in id order."""
    d = os.path.join(root, *spi.DIR_REL.split("/"))
    return [(i, os.path.join(d, i["demo"])) for i in ideas if i["demo"]]


def changed_paths(root, ref):
    """Repo-relative paths changed since `ref` (committed diff, working tree, and untracked)."""
    out = set()
    for argv in (["git", "diff", "--name-only", ref], ["git", "ls-files", "--others", "--exclude-standard"]):
        p = subprocess.run(argv, cwd=root, capture_output=True, text=True, encoding="utf-8", errors="replace")
        if p.returncode != 0:
            raise SystemExit("`%s` failed: %s" % (" ".join(argv), (p.stderr or "").strip()))
        out.update(x.strip().replace("\\", "/") for x in p.stdout.splitlines() if x.strip())
    return out


def run_demos(root, picked, out, compile_fn=ideas_demo.compile_demo):
    """Check every (idea, path); returns (pass, fail, skip)."""
    counts = {"PASS": 0, "FAIL": 0, "SKIP": 0}
    for i, path in picked:
        info, dd = demo_header(spi.read(path))
        if dd:
            status, rep = "FAIL", ["  header: " + x for x in dd]
        else:
            status, rep = ideas_demo.check_demo(root, info, path, compile_fn)
        counts[status] += 1
        out.write("%s  %03d  %s\n" % (status, i["id"], i["demo"]))
        for r in rep:
            out.write(r + "\n")
    return counts["PASS"], counts["FAIL"], counts["SKIP"]


def cmd_demo_check(root, a, out=None, compile_fn=ideas_demo.compile_demo):
    out = out or sys.stdout
    ideas, defects = load(root)
    if defects:
        for d in defects:
            print("refusing: %s" % d, file=sys.stderr)
        return 1
    every = demo_files(root, ideas)
    if a.changed:
        ch = changed_paths(root, a.changed)
        prefix = spi.DIR_REL + "/"
        picked = [(i, p) for i, p in every
                  if prefix + i["demo"] in ch or prefix + i["file"] in ch]
        if not picked:
            out.write("demo-check: no demo changed since %s\n" % a.changed)
            return 0
    elif a.ids:
        by = {i["id"]: (i, p) for i, p in every}
        missing = [n for n in a.ids if n not in by]
        if missing:
            print("demo-check: no demo for idea(s) %s" % ", ".join(str(n) for n in missing), file=sys.stderr)
            return 1
        picked = [by[n] for n in a.ids]
    elif a.all:
        picked = every
    else:
        print("demo-check: name idea ids, or pass --all or --changed REF", file=sys.stderr)
        return 2
    if a.dump:
        for i, path in picked:
            info, dd = demo_header(spi.read(path))
            obj, log = compile_fn(root, info, path)
            out.write("==== %03d %s\n%s\n" % (i["id"], i["demo"], obj.text if obj else log))
        return 0
    t0 = time.time()
    p, f, s = run_demos(root, picked, out, compile_fn)
    out.write("demo-check: %d pass, %d fail, %d skip (%.1f s)\n" % (p, f, s, time.time() - t0))
    if f:
        return 1
    return 3 if s else 0


def cmd_check(root, a, out=None):
    out = out or sys.stdout
    defects, ideas = check_root(root)
    if defects:
        for x in defects:
            print("FAIL: %s" % x, file=sys.stderr)
        return 1
    demos = [i for i in ideas if i["demo"]]
    if getattr(a, "demos", False):
        p, f, s = run_demos(root, demo_files(root, ideas), out)
        out.write("demos: %d pass, %d fail, %d skip\n" % (p, f, s))
        if f or s:
            return 1 if f else 3
    by = {}
    for i in ideas:
        by[i["status"]] = by.get(i["status"], 0) + 1
    out.write("ok: %d ideas (%s), ids %d-%d; %d demo(s)%s\n"
              % (len(ideas), ", ".join("%d %s" % (v, k) for k, v in sorted(by.items())), ideas[0]["id"],
                 ideas[-1]["id"], len(demos), " - `check --demos` (or demo-check --all) compiles them"
                 if demos else ""))
    return 0


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--repo", help="repository root (default: walk up for configure.py)")
    ap.add_argument("--selftest", action="store_true", help="run tools/agents/ideas_selftest.py")
    sub = ap.add_subparsers(dest="cmd")
    f = sub.add_parser("find", help="rank ideas by words")
    f.add_argument("words", nargs="*")
    f.add_argument("--tag")
    f.add_argument("--status")
    f.add_argument("--applies")
    f.add_argument("-n", "--limit", type=int, default=8)
    s = sub.add_parser("show", help="print an idea (and its demo)")
    s.add_argument("n", type=int)
    w = sub.add_parser("where", help="print an idea's path")
    w.add_argument("n", type=int)
    nw = sub.add_parser("new", help="allocate the next id and scaffold an idea")
    nw.add_argument("--title", required=True)
    nw.add_argument("--tags", default="")
    nw.add_argument("--kind", default="process", choices=("codegen", "process"))
    nw.add_argument("--slug")
    nw.add_argument("--applies", default="")
    c = sub.add_parser("check", help="the whole playbook gate")
    c.add_argument("--demos", action="store_true", help="also compile every demo and test its EXPECT lines")
    d = sub.add_parser("demo-check", help="compile demos with the real MWCC and test their EXPECT lines")
    d.add_argument("ids", nargs="*", type=int)
    d.add_argument("--all", action="store_true")
    d.add_argument("--dump", action="store_true", help="print each demo's objdump instead of checking it (for writing EXPECTs)")
    d.add_argument("--changed", metavar="REF", help="demos (or their idea files) changed since REF")
    a = ap.parse_args(argv)
    if a.selftest:
        return subprocess.call([sys.executable, os.path.join(HERE, "ideas_selftest.py")])
    if not a.cmd:
        ap.print_help()
        return 2
    root = spi.find_root(a.repo)
    return {"find": cmd_find, "show": cmd_show, "where": cmd_where, "new": cmd_new, "check": cmd_check, "demo-check": cmd_demo_check}[a.cmd](root, a)


if __name__ == "__main__":
    sys.exit(main())
