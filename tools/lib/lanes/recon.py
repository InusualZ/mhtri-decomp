"""The recon lane's standard brief: the read-only conflict-surface survey of one lane group before a wave is launched.
Spec: docs/tools/spec/lib-lanes.md (the stage: docs/pipeline.md section 14). CLI: none (`slots.py spawn --kind recon`)."""
from __future__ import annotations

#: The questions a recon lane answers, in order (condensed from the wave-1 recon brief, 2026-10-05). `{group}` is the
#: lane group the recon is about; every answer is a table backed by a command the lane ran.
QUESTIONS = (
    ("UNIT SET and EDIT SET",
     "the registered, not-Matching units of {group}; per unit, every file a lane writing ALL its bodies would edit "
     "(its .cpp/.h, owner or leaf headers it adds declarations to, band headers `src/unsplit/*.h`, hub headers, "
     "`configure.py`, `splits.txt`/`symbols.txt` renames, docs) and how many files that is beyond its own."),
    ("CALL AND DATA GRAPH",
     "per unit, the functions of OTHER units it calls that are unwritten or undeclared and the units that own them; "
     "the data it reads that another unit owns; the HOT headers (included by >= 3 units of {group}, or by another "
     "group's units); the types defined in more than one place (rule 1, `recordmerge.py`)."),
    ("CONFLICT MATRIX",
     "every shared file a lane of {group} and a lane of another group (or module code outside the wave) could both "
     "edit, and every outside unit both would need renames or declaration moves in."),
    ("LANE PARTITION",
     "cut {group} into 1-3 lanes so no two share a unit, an owner header or a band-header region; which units MUST "
     "share a lane (a leaf header, mutual calls, one TU over two registered units, a class's methods over units); the "
     "order inside a lane (leaf first); what each lane leaves to the integrator as a request instead of editing."),
    ("PREREQUISITES",
     "what would make concurrent lanes collide or a body impossible: pending placeholder renames, stale `fn_` call "
     "sites, band declarations of symbols {group} now owns (rule 2), flipcheck's undefined references, claimed-but-"
     "unemitted `.data`, unfinished siblings, pending header folds - each with its minimal fix and whether it is a "
     "tool change, a one-time pre-pass batch or lane work."),
    ("RECOMMENDATION",
     "the lane count, boundaries and order; the PRE-PASS batches to land alone first and their order; the shared "
     "files to serialise or pre-edit; each lane's manifest (`owns`, `read_only`, `units`); the residual conflict "
     "risk (probability and impact) after that."),
)

TOOLS = ("python tools/symbols/symedit.py", "python tools/units/callers.py <addr|name>",
         "python tools/units/sweepcomments.py --unit <unit>", "python tools/units/queue.py list",
         "python tools/units/declclash.py <file>", "python tools/units/recordmerge.py",
         "python tools/units/vtableaudit.py", "git grep")


def recon_brief(group: str, units: list[str] | None = None) -> str:
    """The paste-ready task of a recon lane for `group` (the wave's lane group, e.g. `the network stack`), naming
    `units` when the orchestrator already knows the set. Read-only by construction: the brief forbids every write."""
    group = (group or "").strip()
    if not group:
        raise SystemExit("REFUSED recon brief: name the lane group (--group)")
    lines = ["# Reconnaissance (READ-ONLY): the conflict surface of %s" % group, ""]
    if units:
        lines += ["Units: %s" % ", ".join(units), ""]
    lines += [
        "A wave of decompiler lanes will write a BODY for every function of %s, concurrently with other groups' "
        "lanes (six lanes at most, one worktree each, landed one at a time by `land.py`, which refuses conflicts in "
        "shared headers and `src/` files). Your job is to MINIMISE their conflicts: measure the conflict surface of "
        "this group and its interaction with the others. Do not duplicate a sizing survey." % group,
        "",
        "Read first: CLAUDE.md, docs/pipeline.md (sections 1, 3-5, 10.10, 11, 14), docs/plan.md 5.4 and 6.5, "
        "docs/tools/spec/integrate.md, and the unit headers (RANGE/NAMES/RESIDUALS) of the group's units. Never "
        "print config/RMHE08/symbols.txt. Tools: %s." % ", ".join("`%s`" % t for t in TOOLS),
        "",
        "Measure and report, with numbers and file lists:",
        "",
    ]
    for n, (title, text) in enumerate(QUESTIONS, 1):
        lines.append("%d. %s: %s" % (n, title, text.format(group=group)))
    lines += [
        "",
        "Do not edit any file, commit, or write to the repository. Your final message is the report: compact tables, "
        "every claim backed by a command you ran, and what you could not determine.",
    ]
    return "\n".join(lines)
