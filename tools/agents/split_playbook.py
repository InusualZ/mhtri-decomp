#!/usr/bin/env python3
"""split_playbook.py - the ONE-TIME split of docs/matching.md into docs/matching/ (one file per idea).

Input: the monolithic playbook (`## N. Title` sections, plus the unnumbered `The loop`, `Toolbox`,
`Ruled out`, two `Worked example` sections and one unnumbered paired-single note) and the skill's hand-kept
`working-the-list.md`. Output under docs/matching/:

    NNN-slug.md            one per numbered idea: front matter, a blank line, `# N. Title`, then the
                           ORIGINAL section body byte-for-byte
    README.md toolbox.md ruled-out.md todo.md examples/*.md notes/paired-single.md
    index.md               generated afterwards by sync_playbook_index.py

`verify()` re-reads the written files and proves every idea body (and each other section body) is
byte-identical to the old section text once the front matter and the heading line are removed.

    python tools/agents/split_playbook.py [--source PATH --todo PATH] [--out DIR] [--verify-only]
    python tools/agents/split_playbook.py --selftest

Tag heuristic (TAG_KEYWORDS): a tag is proposed when one of its keywords occurs in the idea's title or problem
sentence (case-insensitive substring); tags are ranked by hit count and at most MAX_TAGS are kept. It is
deliberately crude - an empty or 1-2 tag list is fine, and a human refines tags by editing front matter.
"""
import argparse
import os
import re
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import sync_playbook_index as spi  # noqa: E402

HEAD_RE = re.compile(r"^## (.*?)\s*$")
NUM_RE = re.compile(r"^(\d+)\.\s+(.*)$")
MAX_TAGS = 4
MAX_SLUG_WORDS = 6

TAG_KEYWORDS = {
    "flags": ["flag", "-opt", "-o4", "-o3", "cflags", "-pool", "-str ", "-fp_contract", "-inline", "-func_align",
              "-sdata", "-cpp_exceptions", "mw_version"],
    "pragma": ["pragma"],
    "source-shape": ["source shape", "rewrite", "loop shape", "polarity", "declaration order", "temporar",
                     "switch", "ternary", "operand order", "declared shape", "cast", "source spelling", "`new`"],
    "allocator": ["allocator", "register", "colour", "color", "web priority", "live range"],
    "data": [".data", ".sdata", ".sbss", ".bss", ".rodata", "literal pool", "string pool", "jump table",
             "data range", "data claim", "pool entry", "string literal", "unknown-size"],
    "vtable": ["vtable", "virtual", "class hierarchy", "key function", "vptr", "polymorphic"],
    "linker": ["linker", "ctors", "dtors", "link padding", "ldscript", "flipped", "flip "],
    "sections": ["section", "extab", "splits.txt", "claim"],
    "symbols": ["symbol", "mangl", "name ", "names ", "naming"],
    "relocations": ["reloc"],
    "measurement": ["measure", "metric", "objdiff", "percent", "score", "stale object", "instrument",
                    "first divergence", "match_percent", "divergence"],
    "process": ["lane", "merge", "merging", "record", "deferred", "tiling", "campaign", "one owner"],
    "tooling": ["tool", "script", "automate", "harness", "generate, compile"],
}


def slugify(title, taken):
    """Lowercase ascii words from the title, at most MAX_SLUG_WORDS, unique against `taken`."""
    t = title.lower().replace("'", "").replace("`", "")
    words = re.findall(r"[a-z0-9]+", t)
    while len(words) > 1 and words[0] in ("a", "an", "the"):
        words = words[1:]
    slug = "-".join(words[:MAX_SLUG_WORDS]) or "idea"
    base, n = slug, 2
    while slug in taken:
        slug = "%s-%d" % (base, n)
        n += 1
    return slug


def derive_tags(title, problem):
    text = (title + " " + problem).lower()
    hits = []
    for tag, kws in TAG_KEYWORDS.items():
        n = sum(text.count(k) for k in kws)
        if n:
            hits.append((-n, spi.TAGS.index(tag), tag))
    return [t for _n, _o, t in sorted(hits)[:MAX_TAGS]]


def parse_sections(text):
    """(intro lines, [(heading, [body lines])]) - a `## ` line inside a code fence is not a heading.

    Lines keep no terminator; the text is split on LF only (the playbook is LF).
    """
    lines = text.split("\n")
    intro, secs, cur, fence = [], [], None, False
    for line in lines:
        if line.startswith("```"):
            fence = not fence
        m = HEAD_RE.match(line) if not fence else None
        if m:
            cur = (m.group(1), [])
            secs.append(cur)
        elif cur is None:
            intro.append(line)
        else:
            cur[1].append(line)
    return intro, secs


def section_text(body):
    return "\n".join(body)


def front_matter(n, title, problem, tags):
    return "\n".join(["---", "id: %d" % n, "title: %s" % title, "status: works", "problem: %s" % problem,
                      "tags: [%s]" % ", ".join(tags), "applies: []", "demo:", "---"]) + "\n"


def idea_file(n, title, problem, tags, body):
    return front_matter(n, title, problem, tags) + "\n# %d. %s\n" % (n, title) + section_text(body)


def other_file(heading, body):
    return "# %s\n" % heading + section_text(body)


README = """# Matching playbook

A playbook for making one translation unit's compiled object match the original object instruction for
instruction. Flags are the *second* thing to look at (source shape is the first), but when the target's code is
systematically "less optimized" than ours they are usually the whole story.

Two rules from `CLAUDE.md` apply throughout: a flag change is only acceptable with concrete evidence
(non-negotiable #3), and proven flags belong in `configure.py` as a **per-library** `cflags_*` override -
never by editing `cflags_base`/`cflags_runtime` for everybody.

## Layout

| path | what it is |
| --- | --- |
| `index.md` | **generated** table of every idea: id, title, status, tags, problem - start here to find an idea by symptom |
| `NNN-slug.md` | one idea per file; the three-digit **id is permanent** (600+ citations of "playbook N" / "row N" exist) |
| `toolbox.md` | the unit-agnostic tools the ideas use |
| `ruled-out.md`, `todo.md` | ideas tried and dropped without a section, and ideas not tried yet |
| `examples/` | two worked examples of the whole loop (`rso-runtime.md`, `camellia.md`) |
| `notes/` | short unnumbered notes (`paired-single.md`) |

`docs/matching.md` is only an entry page kept so old citations resolve. To open idea N run
`python tools/agents/sync_playbook_index.py --where N` (or `--json` for the parsed index).

## Finding an idea by symptom

1. Read the first divergence (loop step 2 below) and name the code shape: a fused instruction, a register
   colouring, a table base, a missing `extab`, a `bl` that should be kept, a section that does not pair.
2. Scan `index.md` - the `problem` column is written as the symptom - or filter by **tag**
   (`python tools/agents/sync_playbook_index.py --json` and select on `tags`).
3. Open the idea file; each one is Problem / Why it happens / How to work it / Result / Example.
4. Walk the ideas in id order for a unit that does not match: the ids are the order they were learned.

## How ideas are recorded

* **One file per idea**, `docs/matching/NNN-slug.md`, with the next free id (ids are never renumbered or
  reused) and a slug of at most six lowercase words. It opens with front matter:

```
---
id: 43
title: <the idea's title>
status: works            # works | ruled-out | todo | superseded
problem: <the Problem sentence, one line>
tags: [flags, source-shape]
applies: [Wii/1.3]       # compiler versions/libs it is known to apply to; [] if unknown
demo:                    # NNN-slug.cpp when a compilable demonstration exists
---
```

  followed by a blank line, `# N. Title`, and the body (**Problem. / Why it happens. / How to work it. /
  Result. / Example.**). Record an idea in the session it works: a win that exists only in chat or a scratch
  report is lost at the next compaction.
* **Tags** come from a fixed vocabulary: `flags`, `pragma`, `source-shape`, `allocator`, `data`, `vtable`,
  `linker`, `sections`, `symbols`, `relocations`, `measurement`, `process`, `tooling`. Use the few that are
  certain; an empty or one-tag list is fine.
* **Status**: `works` (has a result), `ruled-out` (tried, does not work - keep it so nobody re-runs it),
  `todo` (not tried yet), `superseded` (a later idea replaces it - say which in the body).
* After adding or editing a file run `python tools/agents/sync_playbook_index.py` (regenerates `index.md`;
  it refuses a duplicate id, a bad key or tag, a slug that disagrees with the id, or a missing demo file) and
  `python .claude/skills/mwcc-unit-matching/scripts/sync_reference.py` (refreshes the skill's portable copy
  under `references/matching/`).
* Unit-specific findings (a residual diff, a known-bad flag) are not playbook material: they belong in the
  unit's own header comment.

## The loop
%s
## Tools

See [toolbox.md](toolbox.md).
"""

TODO_LEAD = """# Ideas without a section of their own

Hand-kept (a later stage turns these rows into `status:` values on idea files). The tables below hold ideas with
**no section of their own**: tried in one unit's context and failed, or not tried at all. `no` means tried and it
did not work (or does not apply) - it stays listed so nobody re-runs it; `todo` means not tried yet. The moment
an idea works it earns an `NNN-slug.md` file. A few `todo` rows were raised by the `Camellia` flag hunt
(`camellia_setup256`), which is closed - re-queue one only if the same shape reappears.

"""


def extract_tables(todo_text):
    """The two tables of working-the-list.md verbatim: (ruled-out block, new-ideas block), each from its lead
    sentence through the end of its table."""
    lines = todo_text.split("\n")
    out = []
    for lead in ("Ruled out so far", "New ideas with no section yet"):
        i = next(k for k, ln in enumerate(lines) if ln.startswith(lead))
        j = i + 1
        while j < len(lines) and not lines[j].startswith("|"):
            j += 1
        while j < len(lines) and lines[j].startswith("|"):
            j += 1
        out.append("\n".join(lines[i:j]))
    return out


def build(source_text, todo_text):
    """{relative path: content} for everything except index.md, plus a manifest for verification."""
    intro, secs = parse_sections(source_text)
    files, manifest, taken = {}, [], set()
    loop_body = None
    for heading, body in secs:
        m = NUM_RE.match(heading)
        low = heading.lower()
        if m:
            n, title = int(m.group(1)), m.group(2).strip()
            problem = spi.problem_sentence(body)
            if problem is None:
                raise SystemExit("section %d (%s) has no Problem sentence" % (n, title))
            slug = slugify(title, taken)
            taken.add(slug)
            rel = "%03d-%s.md" % (n, slug)
            if rel in files or any(f.startswith("%03d-" % n) for f in files):
                raise SystemExit("duplicate id %d" % n)
            files[rel] = idea_file(n, title, " ".join(problem.split()), derive_tags(title, problem), body)
            manifest.append((rel, heading, body, "idea"))
        elif heading == "The loop":
            loop_body = body
        elif heading == "Toolbox":
            files["toolbox.md"] = other_file(heading, body)
            manifest.append(("toolbox.md", heading, body, "other"))
        elif low.startswith("ruled out"):
            files["ruled-out.md"] = other_file(heading, body)
            manifest.append(("ruled-out.md", heading, body, "other"))
        elif low.startswith("worked example: the `rso/runtime`"):
            files["examples/rso-runtime.md"] = other_file(heading, body)
            manifest.append(("examples/rso-runtime.md", heading, body, "other"))
        elif low.startswith("worked example: the `camellia`"):
            files["examples/camellia.md"] = other_file(heading, body)
            manifest.append(("examples/camellia.md", heading, body, "other"))
        elif low.startswith("a paired-single"):
            files["notes/paired-single.md"] = other_file(heading, body)
            manifest.append(("notes/paired-single.md", heading, body, "other"))
        else:
            raise SystemExit("unclassified section: %r" % heading)
    if loop_body is None:
        raise SystemExit("no `## The loop` section")
    # The loop's closing paragraph pointed at CLAUDE.md's retired idea table; README supplies the pointer.
    loop_lines = list(loop_body)
    cut = next((k for k, ln in enumerate(loop_lines) if ln.startswith("The tricks below are the individual moves")),
               len(loop_lines))
    loop_txt = "\n".join(loop_lines[:cut]) + (
        "The ideas are the individual moves; the worked examples (`examples/`) are the whole loop run once. "
        "`index.md` lists every idea with the problem it solves.\n\n")
    files["README.md"] = README % loop_txt
    manifest.append(("README.md#loop", "The loop", loop_lines[:cut], "contained"))
    ruled_tab, new_tab = extract_tables(todo_text)
    files["todo.md"] = TODO_LEAD + ruled_tab + "\n\n" + new_tab + "\n"
    manifest.append(("todo.md#tables", "tables", (ruled_tab + "\n\n" + new_tab).split("\n"), "contained"))
    return files, manifest


def verify(out_dir, manifest):
    """[defects]: each body must be byte-identical to the old section text."""
    bad = []
    for rel, heading, body, kind in manifest:
        want = section_text(body)
        path, _, _frag = rel.partition("#")
        got_text = spi.read(os.path.join(out_dir, *path.split("/")))
        if kind == "contained":
            if want.rstrip("\n") not in got_text:
                bad.append("%s: original text is not contained" % rel)
            continue
        if kind == "idea":
            _fm, rest = spi.parse_front_matter(got_text)
            n, title = NUM_RE.match(heading).groups()
            head = "\n# %s. %s\n" % (n, title.strip())
            if not rest.startswith(head):
                bad.append("%s: heading line differs" % rel)
                continue
            got = rest[len(head):]
        else:
            head = "# %s\n" % heading
            if not got_text.startswith(head):
                bad.append("%s: heading line differs" % rel)
                continue
            got = got_text[len(head):]
        if got != want:
            bad.append("%s: body differs (%d vs %d chars)" % (rel, len(got), len(want)))
    return bad


def run(source, todo, out_dir, verify_only=False):
    files, manifest = build(spi.read(source), spi.read(todo))
    if not verify_only:
        for rel, content in files.items():
            p = os.path.join(out_dir, *rel.split("/"))
            os.makedirs(os.path.dirname(p), exist_ok=True)
            spi.write(p, content)
    bad = verify(out_dir, manifest)
    n_ideas = sum(1 for m in manifest if m[3] == "idea")
    if bad:
        for b in bad:
            print("IDENTITY FAIL: %s" % b, file=sys.stderr)
        return 1
    print("split ok: %d idea files + %d other sections byte-identical to the source (README loop / todo tables "
          "contained)" % (n_ideas, sum(1 for m in manifest if m[3] == "other")))
    return 0


# ---- selftest ---------------------------------------------------------------------------------------------

FIXTURE_SRC = """# Title

intro

## The loop

1. step

The tricks below are the individual moves; go.

## Toolbox

| tool | gives |
| --- | --- |
| a | b |

## 2. Second `idea`: with a colon

**Problem.** Second problem with a `-O4` flag: keep it.

```
## not a heading
```

## 1. First idea's title is a very long title with many words

Problem: first problem about a vtable.

## Ruled out - do not re-run these

* nothing

## Worked example: the `RSO/runtime` unit (x)

body a

## Worked example: the `Camellia` unit

body b

## A paired-single thing

body c
"""

FIXTURE_TODO = """# x

Ruled out so far - tried:

| idea | problem | status |
| --- | --- | --- |
| a | b | no |

New ideas with no section yet:

| idea | problem | status |
| --- | --- | --- |
| c | d | todo |

How to work the list:
"""


def selftest():
    fails = []

    def check(name, got, want):
        if got != want:
            fails.append("%s: got %r want %r" % (name, got, want))

    check("slug: apostrophes dropped, 6 words max", slugify("First idea's title is a very long title", set()),
          "first-ideas-title-is-a-very")
    check("slug: duplicate gets a suffix", slugify("Same title", {"same-title"}), "same-title-2")
    check("slug: leading article dropped", slugify("A vtable we own", set()), "vtable-we-own")
    check("tags: pragma + flags", derive_tags("Scope with pragmas", "a flag changes"), ["flags", "pragma"])
    check("tags: unsure -> empty", derive_tags("Hello", "world"), [])
    out = tempfile.mkdtemp(prefix="split-selftest-")
    src, todo = os.path.join(out, "m.md"), os.path.join(out, "t.md")
    spi.write(src, FIXTURE_SRC)
    spi.write(todo, FIXTURE_TODO)
    dst = os.path.join(out, "docs", "matching")
    check("run: ok", run(src, todo, dst), 0)
    names = sorted(os.listdir(dst))
    check("files", names, ["001-first-ideas-title-is-a-very.md", "002-second-idea-with-a-colon.md", "README.md",
                            "examples", "notes", "ruled-out.md", "todo.md", "toolbox.md"])
    idea, defects = spi.load_idea(os.path.join(dst, "002-second-idea-with-a-colon.md"), "002-second-idea-with-a-colon.md")
    check("idea 2 parses clean", defects, [])
    check("idea 2 problem is one line with the colon kept", idea["problem"],
          "Second problem with a `-O4` flag: keep it.")
    check("fence-protected heading stayed in the body",
          "## not a heading" in spi.read(os.path.join(dst, "002-second-idea-with-a-colon.md")), True)
    # a tampered body must fail verification
    p = os.path.join(dst, "toolbox.md")
    spi.write(p, spi.read(p) + "extra\n")
    _f, manifest = build(FIXTURE_SRC, FIXTURE_TODO)
    check("verify: a tampered body is refused", len(verify(dst, manifest)), 1)
    # a duplicate id is refused
    try:
        build(FIXTURE_SRC + "\n## 2. Again\n\n**Problem.** x\n", FIXTURE_TODO)
        got = "no refusal"
    except SystemExit as e:
        got = str(e)
    check("build: duplicate id refused", got, "duplicate id 2")
    for f in fails:
        print("FAIL " + f)
    print("split_playbook selftest: %d failure(s)" % len(fails))
    return 1 if fails else 0


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    root = spi.find_root()
    ap.add_argument("--source", default=os.path.join(root, "docs", "matching.md"))
    ap.add_argument("--todo", default=os.path.join(root, ".claude", "skills", "mwcc-unit-matching",
                                                    "working-the-list.md"))
    ap.add_argument("--out", default=os.path.join(root, "docs", "matching"))
    ap.add_argument("--verify-only", action="store_true")
    ap.add_argument("--selftest", action="store_true")
    a = ap.parse_args()
    if a.selftest:
        return selftest()
    return run(a.source, a.todo, a.out, a.verify_only)


if __name__ == "__main__":
    sys.exit(main())
