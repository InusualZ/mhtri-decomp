#!/usr/bin/env python3
"""Merge two views of the same record header into one definition (docs/matching.md 56).

A header that describes a game record (`_AINPC_W`, `_HIT_W`) is written by several lanes at once, and
every lane's branch carries its own view of the same struct: the same members at the same offsets, plus
the fields that lane's bodies needed.  The orchestrator has to fold those views into the one header the
whole tree shares, and doing it by hand has gone wrong the same way three times - so this tool *is* the
procedure, with the checks the hand passes lacked.

    python tools/units/recordmerge.py --base <base> --other <other> [--out <path>] [--take other|base]
    python tools/units/recordmerge.py --selftest

`<base>` is normally the working file (the live header, CRLF on this host); `<other>` is normally a
revision, in `git show` spelling:

    python tools/units/recordmerge.py --base include/ai/ainpc.h \
        --other worker/802d44f4-fn-802d44f4-bd0a:include/ai/ainpc.h --out include/ai/ainpc.h

**The three rules** (docs/matching.md 56, learned on `_AINPC_W`).

1. *Splice into the filler.*  The live header is the base.  For every member the other view names and
   the base does not have at that offset, find the base **filler** covering that offset, take the new
   member's size from the *other* header's own layout (the distance to its next member - never from the
   declared type), and split the filler into `[gap][new member][gap]`.
2. *Key members per struct.*  A header may define several structs, and an offset-keyed member map sizes
   a field from the wrong one: `_AINPC_W` shares its file with a smaller struct whose `0x0` matched
   first, which sized the 1-byte `active` as 12 bytes and then refused every splice for lack of room.
   Every lookup here is per group.
3. *Compare the declaration, not just `(offset, name)`.*  Two views can declare one offset differently -
   `u8 field_0x3F8;` against `u8 field_0x3F8[4];` - and an `(offset, name)` test calls that "already
   there" while the newcomer's own source fails to compile against it (`illegal operands 'unsigned char'
   [ 'unsigned char'`).  Which side is right depends on which side's *code* depends on the declaration,
   so this tool reports the conflict and takes `--take` (default `other`: the incoming branch is the
   source that has to compile), dropping or shrinking the base members the winning declaration covers.

**It refuses to write while anything is unresolved** - a named member in the way, a filler with no room
or no known end, a group only one side has, a same-offset rename, a member whose size is the struct
total - and prints the per-group delta so a human can see what would change.  A merge that applies is
still not a proof: the test is that **both** sides' sources compile (whole-tree `ninja -k 0` at
0 FAILED) **and** that the rows hold (`ninja changes` must print no line for a unit that already owned
the record).  This tool cannot check either - it makes the edit reproducible and its invariants explicit.
"""

from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
from dataclasses import dataclass, field
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
sys.path.insert(0, str(ROOT / "tools" / "units"))

import sharedfiles as sf  # noqa: E402  the one writer of shared files

MEMBER_RE = re.compile(r"^(?P<indent>[ \t]*)/\* \+(?P<off>0x[0-9A-Fa-f]+) \*/ (?P<body>.*?)[ \t]*$")
GROUP_RE = re.compile(r"^[ \t]*(?:typedef[ \t]+)?(?:struct|union|class)[ \t]+([A-Za-z_]\w*)")
END_RE = re.compile(r"^[ \t]*};")
RANGE_RE = re.compile(r"\[\s*(0x[0-9A-Fa-f]+)\s*-\s*(0x[0-9A-Fa-f]+)\s*\]")
COUNT_RE = re.compile(r"\[\s*(0x[0-9A-Fa-f]+|\d+)\s*\]\s*;\s*$")
NAME_RE = re.compile(r"^.*?([A-Za-z_][A-Za-z_0-9]*)\s*(?:\[[^\]]*\])?\s*;\s*$")
SCALAR = {
    "u8": 1, "s8": 1, "char": 1, "BOOL": 1, "bool": 1,
    "u16": 2, "s16": 2, "short": 2, "wchar_t": 2,
    "int": 4, "long": 4, "u32": 4, "s32": 4, "f32": 4, "float": 4, "size_t": 4,
    "u64": 8, "s64": 8, "f64": 8, "double": 8,
}
MAX_DETAIL = 12


# --------------------------------------------------------------------------------------------------
# the model: one header file is a list of groups of members
# --------------------------------------------------------------------------------------------------
@dataclass
class Member:
    off: int
    code: str                 # the declaration, trailing comment removed
    raw: str                  # the line as written - a splice keeps it verbatim
    line: int                 # index into the parsed text's line list
    name: str | None

    @property
    def filler(self) -> bool:
        return self.name is not None and self.name.startswith("unused_0x")

    @property
    def size(self) -> int | None:
        return declared_size(self.code)

    @property
    def end(self) -> int | None:
        size = self.size
        return None if size is None else self.off + size

    def label(self) -> str:
        return f"+0x{self.off:03X} {self.name}"


@dataclass
class Group:
    name: str
    start: int = -1        # the line that opens the group
    stop: int = -1         # its `};` line
    members: list[Member] = field(default_factory=list)


def declared_size(code: str) -> int | None:
    """The byte count a declaration states, or None when it does not state one.

    A filler says `[0xEND - 0xSTART]`; an array says `[N]`; a scalar's count comes from its type
    token.  An array of a named type (`AINPCFormation x[2];`) is deliberately unknown - the type's own
    size is not this tool's business.
    """
    m = RANGE_RE.search(code)
    if m:
        # the house form is `[0xEND - 0xSTART]`, so group 1 is the end
        return int(m.group(1), 16) - int(m.group(2), 16)
    m = COUNT_RE.search(code)
    if m:
        v = m.group(1)
        return int(v, 16) if v.lower().startswith("0x") else int(v)
    if "*" in code:
        return 4
    for tok in re.findall(r"[A-Za-z_]\w*", code):
        if tok in SCALAR:
            return SCALAR[tok]
        if tok in ("const", "volatile", "unsigned", "signed", "static", "typedef"):
            continue
        break
    return None


def parse(text: str) -> list[Group]:
    groups: list[Group] = []
    cur: Group | None = None
    for i, line in enumerate(text.split("\n")):
        if GROUP_RE.match(line) and line.rstrip().endswith("{"):
            cur = Group(GROUP_RE.match(line).group(1), start=i)
            groups.append(cur)
            continue
        if END_RE.match(line):
            if cur is not None:
                cur.stop = i
            cur = None
            continue
        if cur is None:
            continue
        m = MEMBER_RE.match(line)
        if not m:
            continue
        code = m.group("body").split("/*")[0].strip()
        nm = NAME_RE.match(code)
        cur.members.append(Member(int(m.group("off"), 16), code, line.rstrip(), i,
                                  nm.group(1) if nm else None))
    return groups


def group_map(text: str) -> dict[str, Group]:
    return {g.name: g for g in parse(text.replace("\r\n", "\n"))}


# --------------------------------------------------------------------------------------------------
# the plan
# --------------------------------------------------------------------------------------------------
@dataclass
class GroupDelta:
    name: str
    added: list[str] = field(default_factory=list)
    conflicts: list[dict] = field(default_factory=list)
    superseded: list[str] = field(default_factory=list)
    dropped: list[str] = field(default_factory=list)


@dataclass
class Plan:
    groups: list[GroupDelta] = field(default_factory=list)
    unresolved: list[str] = field(default_factory=list)
    violations: list[str] = field(default_factory=list)
    carried: list[str] = field(default_factory=list)
    skipped_lines: list[str] = field(default_factory=list)
    text: str | None = None

    @property
    def ok(self) -> bool:
        return not self.unresolved and not self.violations


def _fmt_filler(off: int, end: int) -> str:
    return "    /* +0x%03X */ u8 unused_0x%03X[0x%03X - 0x%03X];" % (off, off, end, off)


def _next_off(members: list[Member], off: int) -> int | None:
    after = [m.off for m in members if m.off > off]
    return min(after) if after else None


def _rewrite(work: list[str], replace: dict[int, list[str]], drop: set[int]) -> list[str]:
    out: list[str] = []
    for i, line in enumerate(work):
        if i in replace:
            out.extend(replace[i])
        elif i in drop:
            continue
        else:
            out.append(line)
    return out


def merge(base_text: str, other_text: str, take: str = "other") -> Plan:
    base_nl = base_text.replace("\r\n", "\n")
    other_nl = other_text.replace("\r\n", "\n")
    work = base_nl.split("\n")
    plan = Plan()
    base_count: dict[str, int] = {}
    for g in parse(base_nl):
        base_count[g.name] = base_count.get(g.name, 0) + 1

    for og in parse(other_nl):
        delta = GroupDelta(name=og.name)
        plan.groups.append(delta)
        if base_count.get(og.name, 0) != 1:
            plan.unresolved.append(
                f"{og.name}: the base has {base_count.get(og.name, 0)} groups with that name, need exactly 1")
            continue

        for m in og.members:
            if m.filler or not m.name:
                continue
            # Re-parse after every edit: line numbers and covering fillers move under the next member.
            bg = group_map("\n".join(work)).get(og.name)
            if bg is None:
                plan.unresolved.append(f"{og.name}: the group left the base after an earlier edit")
                break

            at_offset = [x for x in bg.members if x.off == m.off]
            if at_offset and not at_offset[0].filler:
                _resolve_conflict(bg, og, m, at_offset[0], take, delta, plan, work)
                continue

            covering = [x for x in bg.members if x.off <= m.off]
            c = covering[-1] if covering else None
            if c is None:
                plan.unresolved.append(
                    f"{m.label()} ({og.name}): the base has no member at or before that offset")
                continue
            if not c.filler:
                plan.unresolved.append(
                    f"{m.label()} ({og.name}): it falls inside the base's member '{c.name}' at "
                    f"+0x{c.off:03X} - split that member by hand first")
                continue

            end = _next_off(og.members, m.off)
            if end is None:
                plan.unresolved.append(
                    f"{m.label()} ({og.name}): last member of the other's layout, so its size is the "
                    f"struct total and this tool cannot infer it")
                continue
            size = end - m.off
            if c.end is None:
                plan.unresolved.append(
                    f"{m.label()} ({og.name}): the covering base filler {c.name} has no size this tool "
                    f"can read, so there is no room to check")
                continue
            if m.off + size > c.end:
                plan.unresolved.append(
                    f"{m.label()} ({og.name}): needs {size} B but the base's filler {c.name} ends at "
                    f"+0x{c.end:03X}")
                continue

            rows: list[str] = []
            if c.off < m.off:
                rows.append(_fmt_filler(c.off, m.off))
            rows.append(m.raw)
            if m.off + size < c.end:
                rows.append(_fmt_filler(m.off + size, c.end))
            work = _rewrite(work, {c.line: rows}, set())
            delta.added.append(f"{m.label()} ({size} B)")
            if len(rows) > 1:
                delta.superseded.append(f"{c.name} split at +0x{m.off:03X}")

    # Top-level lines the other side has and the base lacks - an `extern` beside the record, a typedef
    # its members use, an explanatory comment.  Nothing about their placement is mechanical, so a
    # *declaration* is carried verbatim after the last group's closing brace (reported), and anything
    # else is reported and not carried: dropping a comment loses information but changes no code, and
    # the whole-tree build is the arbiter of whether the placement was right.
    other_lines = other_nl.split("\n")
    base_lines = {l.strip() for l in base_nl.split("\n")}
    carried: list[str] = []
    skipped: list[str] = []
    for line in other_lines:
        s = line.strip()
        if not s or s in base_lines or MEMBER_RE.match(line) or END_RE.match(line) \
                or (GROUP_RE.match(line) and line.rstrip().endswith("{")):
            continue
        (carried if s.endswith(";") else skipped).append(s)
    if carried:
        closes = [i for i, l in enumerate(work) if END_RE.match(l)]
        at = closes[-1] + 1 if closes else len(work)
        work[at:at] = carried
        plan.carried = carried
    plan.skipped_lines = skipped

    merged = "\n".join(work)
    plan.violations = verify(base_nl, other_nl, merged)
    plan.text = None if not plan.ok else merged
    return plan


def _resolve_conflict(bg: Group, og: Group, m: Member, b: Member, take: str,
                      delta: GroupDelta, plan: Plan, work: list[str]) -> None:
    """One offset declared by both sides.  Same declaration -> nothing to do; different -> report."""
    if b.code == m.code:
        return
    label = m.label()
    if b.name != m.name:
        plan.unresolved.append(
            f"{label} ({og.name}): the base calls it '{b.name}' and the other '{m.name}' - a "
            f"same-offset rename this tool will not guess")
        return

    end = _next_off(og.members, m.off)
    if end is None:
        plan.unresolved.append(
            f"{label} ({og.name}): a declaration conflict on the last member of the other's layout - "
            f"its size is the struct total, so which declaration wins cannot be checked")
        return
    size = end - m.off
    covered = [x for x in bg.members if m.off < x.off < m.off + size]
    unknown = [x for x in covered if not x.filler or x.size is None]
    if unknown:
        plan.unresolved.append(
            f"{label} ({og.name}): taking the other's declaration would cover "
            f"{', '.join(x.label() for x in unknown)}, which is not a plain filler - decide by hand")
        return

    delta.conflicts.append({
        "member": label,
        "group": og.name,
        "base": b.code,
        "other": m.code,
        "size": size,
        "covers": [x.label() for x in covered],
        "resolution": "taken from the other" if take == "other" else "the base's kept",
    })
    if take != "other":
        return

    drop = {x.line for x in covered}
    replace = {b.line: [m.raw]}
    for x in covered:
        assert x.size is not None
        if x.off + x.size > m.off + size:
            # a filler that reaches past the winner is shrunk, not dropped: the bytes above it belong
            # to members the base already has
            replace[x.line] = [_fmt_filler(m.off + size, x.off + x.size)]
            drop.discard(x.line)
            delta.dropped.append(f"{x.name} shrunk to +0x{m.off + size:03X}")
        else:
            delta.dropped.append(x.name)
    work[:] = _rewrite(work, replace, drop)


# --------------------------------------------------------------------------------------------------
# verify: the invariants a merge must keep, and nothing about the base's pre-existing shape
# --------------------------------------------------------------------------------------------------
def tiling_violations(groups: dict[str, Group]) -> set[str]:
    """Members whose declared size disagrees with the next member's offset (last member exempt)."""
    out: set[str] = set()
    for name, g in groups.items():
        for a, b in zip(g.members, g.members[1:]):
            if a.size is not None and a.end != b.off:
                out.add(f"{name}: {a.label()} declares +0x{a.end:03X}, the next member is +0x{b.off:03X}")
    return out


def _spans(g: Group) -> list[tuple[int, int | None]]:
    return [(m.off, m.end) for m in g.members]


def _covered(g: Group, off: int) -> bool:
    """True when some member of `g` describes the byte at `off` (a member with no known end is open)."""
    return any(m.off <= off and (m.end is None or off < m.end) for m in g.members)


def verify(base_text: str, other_text: str, merged_text: str) -> list[str]:
    base = group_map(base_text)
    other = group_map(other_text)
    merged = group_map(merged_text)
    out: list[str] = []
    pre = tiling_violations(base) | tiling_violations(other)

    for name, g in merged.items():
        offs = [m.off for m in g.members]
        if offs != sorted(offs):
            out.append(f"{name}: members are not in offset order")
        if len(offs) != len(set(offs)):
            out.append(f"{name}: duplicate member offsets")
        names = [m.name for m in g.members if m.name]
        dupes = sorted({n for n in names if names.count(n) > 1})
        if dupes:
            out.append(f"{name}: duplicate member name(s): {', '.join(dupes)}")
        here = {(m.off, m.name) for m in g.members}
        # every field the other view named must have arrived; every member of the base must still be
        # described - either by name, or (a conflict decision, a filler split) by a member that spans
        # its offset
        if name in other:
            for m in other[name].members:
                if not m.filler and (m.off, m.name) not in here:
                    out.append(f"{name}: {m.label()} is in the other and missing from the merge")
        if name in base:
            for m in base[name].members:
                if (m.off, m.name) in here:
                    continue
                if not _covered(g, m.off):
                    out.append(f"{name}: {m.label()} is in the base and nothing in the merge describes "
                               f"that offset")
        for v in tiling_violations({name: g}) - pre:
            out.append(v)

    for name in other:
        if name not in merged:
            out.append(f"{name}: the other side's group is missing from the merge")
    for name in base:
        if name not in merged:
            out.append(f"{name}: the base's group is missing from the merge")
    return out


# --------------------------------------------------------------------------------------------------
# io / report
# --------------------------------------------------------------------------------------------------
def read_source(spec: str) -> tuple[str, str]:
    """A file path, or `git show` spelling `<rev>:<path>`; returns (text, what to call it)."""
    if ":" in spec and not Path(spec).exists():
        rev, path = spec.split(":", 1)
        r = subprocess.run(["git", "show", f"{rev}:{path}"], cwd=ROOT,
                           capture_output=True, text=True, encoding="utf-8", errors="replace")
        if r.returncode != 0:
            raise SystemExit(f"git show {spec} failed: {r.stderr.strip()}")
        return r.stdout, spec
    p = Path(spec)
    if not p.exists():
        raise SystemExit(f"no such file: {spec}")
    return p.read_text(encoding="utf-8"), spec


def report(plan: Plan, base_label: str, other_label: str, out_label: str | None,
           written: bool) -> None:
    print(f"base  {base_label}")
    print(f"other {other_label}")
    for d in plan.groups:
        print(f"  {d.name:<16} +{len(d.added)} added, {len(d.conflicts)} conflict(s), "
              f"{len(d.superseded)} filler split(s), {len(d.dropped)} dropped")
        for row in d.added[:MAX_DETAIL]:
            print(f"      + {row}")
        if len(d.added) > MAX_DETAIL:
            print(f"      + ... and {len(d.added) - MAX_DETAIL} more")
        for c in d.conflicts:
            print(f"      ! {c['member']}: base '{c['base']}' / other '{c['other']}' -> {c['resolution']}")
            if c["covers"]:
                print(f"        covers {', '.join(c['covers'])}")
        for row in d.dropped[:MAX_DETAIL]:
            print(f"      - {row}")
    if plan.carried:
        print(f"carried {len(plan.carried)} top-level declaration(s) only the other side has:")
        for row in plan.carried[:MAX_DETAIL]:
            print(f"      > {row}")
    if plan.skipped_lines:
        print(f"{len(plan.skipped_lines)} other-side line(s) neither carried nor in the base:")
        for row in plan.skipped_lines[:5]:
            print(f"      ~ {row}")
    if plan.violations:
        print(f"verification: {len(plan.violations)} violation(s)")
        for v in plan.violations:
            print(f"      x {v}")
    else:
        print("verification: tiling held, every member of both sides is present")
    if plan.unresolved:
        print(f"unresolved ({len(plan.unresolved)}) - nothing written:")
        for u in plan.unresolved:
            print(f"      ? {u}")
    elif written:
        print(f"WRITTEN {out_label}")
    elif out_label:
        print("(dry run - nothing written)")
    else:
        print("(no --out - nothing written)")


def build_parser() -> argparse.ArgumentParser:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--base", help="the live header: a path, or <rev>:<path>")
    ap.add_argument("--other", help="the incoming view: a path, or <rev>:<path>")
    ap.add_argument("--out", help="write the merge here (default: the base's path)")
    ap.add_argument("--take", choices=("other", "base"), default="other",
                    help="which side's declaration wins a same-offset conflict (default: other)")
    ap.add_argument("--dry-run", action="store_true")
    ap.add_argument("--json", dest="json_out")
    ap.add_argument("--selftest", action="store_true")
    return ap


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    if args.selftest:
        import recordmerge_selftest
        return recordmerge_selftest.selftest()
    if not args.base or not args.other:
        build_parser().error("--base and --other are required (or --selftest)")

    base_text, base_label = read_source(args.base)
    other_text, other_label = read_source(args.other)
    plan = merge(base_text, other_text, take=args.take)

    out: Path | None = None
    if args.out:
        out = Path(args.out)
    elif ":" in args.base and not Path(args.base).exists():
        out = Path(args.base.split(":", 1)[1])
    written = False
    if out is not None and plan.ok and not args.dry_run:
        nl = sf.line_ending(sf.read_text(out)) if out.exists() else sf.line_ending(base_text)
        if not nl:
            nl = "\n"
        tx = sf.Transaction()
        try:
            text = plan.text if plan.text.endswith("\n") else plan.text + "\n"
            tx.write(out, sf.with_ending(text, nl))
            written = True
        finally:
            tx.cleanup()

    report(plan, base_label, other_label, str(out) if out else None, written)
    if args.json_out:
        Path(args.json_out).write_text(json.dumps({
            "base": base_label, "other": other_label, "out": str(out) if out else None,
            "written": written, "ok": plan.ok, "take": args.take,
            "groups": [vars(d) for d in plan.groups],
            "unresolved": plan.unresolved, "violations": plan.violations,
        }, indent=2), encoding="utf-8")
    return 0 if plan.ok else 1


if __name__ == "__main__":
    sys.exit(main())
