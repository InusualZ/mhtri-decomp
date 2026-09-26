#!/usr/bin/env python3
"""profileprobe.py - smoke-test subagent profiles and print the checklist to judge them by.

A profile is a prompt, so the closest thing to a test has two halves:

  T1 discovery - the file must appear as a project agent (`subagent list`).
  T2 recall    - a reader child that runs nothing and answers, from its own prompt
                 alone, what its job/limits/rules/verification/report are.

T2 has caught a real defect already: the first `decompiler` draft mis-numbered
section 6.5's rule table (rule 1 described as the vtable rule, which is rule 10),
and the probe's "rules 5 and 8 are not in my context" is what exposed it.

    python .pi/bin/profileprobe.py <agent> [<agent> ...]

writes .pi/workflows/probe-<agents>.js and prints the per-agent checklist, so a
profile change can be re-tested by re-running this and reading the replies
against it. Behaviour on a real task (T3) is separate and is logged in
.agents/agents/TESTS.md.
"""
import io
import json
import os
import sys

COMMON = """PROBE - do no work: run no tools, write no files, no builds, no git. Answer from your own system prompt
only, in under 20 lines total. Being told what is MISSING from your context is the most useful answer you can
give, so do not paper over a gap.

1. One line: what is your job?
2. Where may you write, and which tree must you never write in? How do you tell that you were launched in the
   wrong tree, and what is the correct action if you were?
3. List the numbered rules you are subject to - the numbers and a few words each. Say explicitly which numbers
   you are confident about and which you are unsure of or do not have.
4. What must you run before you report, and what must each result be? Which single result is the PRIMARY signal
   if they disagree?
5. What sections must your final report contain?
6. Name anything a worker doing this job needs that you were NOT told.
"""

EXTRA = {
    "decompiler": """
7. For this role specifically: what is the matching policy when a row cannot reach 100 %, and where does a
   residual belong? Name three codegen levers that usually pay, and what a `rule 7 deferred` comment does and
   does not defer.
""",
    "merger": """
7. For this role specifically: which two files may be unioned automatically and which must NEVER be handled
   that way, and why? What exactly must you prove before committing, and what is the struct-shrink hazard?
""",
    "fixer": """
7. For this role specifically: how do you decide a reconstructed struct's size (name the evidence kinds)? How do
   you fix a rule-6 pointer-arithmetic finding? What is the hard constraint on scores, and what happens to a
   measured improvement?
""",
}


def main(argv):
    agents = argv[1:] or ["decompiler", "merger", "fixer"]
    main_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
    lanes = []
    for a in agents:
        task = (COMMON + EXTRA.get(a, "")) if a in EXTRA else COMMON
        t = task.replace("\\", "\\\\").replace("`", "\\`").replace("${", "\\${")
        lanes.append('  { key: "probe-%s", agent: "%s", task: `%s` },' % (a, a, t))
    js = "\n".join([
        "// T2 recall probe for: %s - the child must run nothing and answer from its prompt." % ", ".join(agents),
        "const lanes = ["] + lanes + [
        "];",
        "const ran = await runs.all(lanes);",
        "emit(ran.map(function (r) { return { key: r.key, status: r.status, out: String(r.output || '') }; }));",
        "return ran.map(function (r) { return { key: r.key, status: r.status }; });",
    ])
    out = os.path.join(main_dir, ".pi", "workflows", "probe-%s.js" % "-".join(agents))
    io.open(out, "w", encoding="utf-8", newline="\n").write(js + "\n")
    print("wrote %s" % os.path.relpath(out, main_dir))
    print("launch: subagent({ async: true, worktree: false, workflowScriptPath: %s })"
          % os.path.relpath(out, main_dir))
    print()
    print("T1 checklist (run `subagent list`): each of these must show as a project agent")
    for a in agents:
        print("  - %s" % a)
    print()
    print("T2 checklist (judge each reply against these; a missing item is a PROFILE DEFECT, record it):")
    for a in agents:
        print("  [%s]" % a)
        for item in ("its job in one line",
                     "writes only in its worktree, never MAIN, and names the tell (repo root = MAIN)",
                     "the numbered rules it is subject to, with the right numbers",
                     "the pre-report verification: FAILED==0 first, then DOL OK, then stylelint clean",
                     "the primary signal when they disagree = the FAILED count (the ok target lies)",
                     "its report's required sections"):
            print("      - %s" % item)
        if a == "decompiler":
            print("      - best-scoring variant policy; residual in the UNIT header")
            print("      - three paying codegen levers (peephole off / fp_contract / typed params / pool off)")
            print("      - rule 7 deferral defers the fn_ half only")
        if a == "merger":
            print("      - union only configure.py + splits.txt; a HEADER gets a hand union")
            print("      - the zero-rows-moved proof")
            print("      - the re-pad hazard (a split without a pad shrinks the struct)")
        if a == "fixer":
            print("      - size evidence kinds (member sum, allocation, memset length, .data/.rel, dump)")
            print("      - rule 6 fixed by naming the record, not by a cast")
            print("      - no row may end lower; an improvement is kept")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
