#!/usr/bin/env python3
"""Source-rewrite experiments for the `Camellia` unit - data for `tools/flags/tryvar.py`.

The harness (`tryvar.py`) is generic; this file is unit-specific *data*: the rewrites that were tried
against the one remaining difference in `camellia_setup256` (an extra 4-byte stack slot) and the small
regex helpers they use.

Each entry is `(name, repls)` where `repls` is either a list of `(old, new)` string pairs or a callable
`src -> src` (returning `None` means "the pattern did not match here").

Status: **every variant in this file is a rejected candidate** - all of them either reproduce the
baseline byte-for-byte (the front end is insensitive to statement shape) or make the code worse. The
surviving hypothesis and the full analysis are in `.pi/notes/camellia-match-process.md` (finding #14).
"""
import re


DECLS = """    u32 kll,klr,krl,krr;           /* left half of key */
    u32 krll,krlr,krrl,krrr;       /* right half of key */
    u32 il, ir, t0, t1, w0, w1;    /* temporary variables */
    u32 kw4l, kw4r, dw, tl, tr;
    u32 subL[34];
    u32 subR[34];
"""


def sub256(src, pat, rep, count=1):
    """Apply a regex substitution restricted to the camellia_setup256 body.
    Returns None (and prints) when the pattern does not match."""
    start = src.index("void camellia_setup256")
    end = src.index("void camellia_setup192")
    body, n = re.subn(pat, rep, src[start:end], count=count)
    if n != count:
        print("    (pattern not found: %s)" % pat[:60])
        return None
    return src[:start] + body + src[end:]


def split_comma_stmts(src, pattern=None):
    """Split `a = x, b = y;` comma-operator statements of camellia_setup256 into two
    statements. `pattern` optionally restricts which lines are rewritten."""
    start = src.index("void camellia_setup256")
    end = src.index("void camellia_setup192")
    body = src[start:end]
    rx = re.compile(r"^(\s*)([A-Za-z_]\w*(?:\([^)]*\))? = [^,;]+), ([^,;]+);\s*$")
    out = []
    n = 0
    for line in body.split("\n"):
        m = rx.match(line)
        if m and (pattern is None or re.search(pattern, line)):
            out.append(m.group(1) + m.group(2) + ";")
            out.append(m.group(1) + m.group(3) + ";")
            n += 1
        else:
            out.append(line)
    print("    (split %d comma statements)" % n)
    return src[:start] + "\n".join(out) + src[end:]


def split_stmts(src):
    """Split every `a; b;` line of camellia_setup256 into one statement per line
    (the formatting NSS's own camellia.c uses)."""
    start = src.index("void camellia_setup256")
    end = src.index("void camellia_setup192")
    body = src[start:end]
    out = []
    for line in body.split("\n"):
        st = line.strip()
        if (st.count(";") == 2 and "; " in st and not st.startswith("CAMELLIA_")
                and st.count("(") == st.count(")")):
            a, b = line.split("; ", 1)
            out.append(a + ";")
            out.append("    " + b)
        else:
            out.append(line)
    return src[:start] + "\n".join(out) + src[end:]


VARIANTS = [
    # name, [(old, new), ...] or callable(src)->src
    ("v0_baseline", []),
    ("v1_decl_reverse", [(DECLS, """    u32 krll,krlr,krrl,krrr;       /* right half of key */
    u32 kll,klr,krl,krr;           /* left half of key */
    u32 il, ir, t0, t1, w0, w1;    /* temporary variables */
    u32 kw4l, kw4r, dw, tl, tr;
    u32 subL[34];
    u32 subR[34];
""")]),
    ("v2_decl_oneline", [(DECLS, """    u32 kll,klr,krl,krr, krll,krlr,krrl,krrr;
    u32 il, ir, t0, t1, w0, w1;
    u32 kw4l, kw4r, dw, tl, tr;
    u32 subL[34];
    u32 subR[34];
""")]),
    ("v3_arrays_first", [(DECLS, """    u32 subL[34];
    u32 subR[34];
    u32 kll,klr,krl,krr;           /* left half of key */
    u32 krll,krlr,krrl,krrr;       /* right half of key */
    u32 il, ir, t0, t1, w0, w1;    /* temporary variables */
    u32 kw4l, kw4r, dw, tl, tr;
""")]),
    ("v4_arrays_last_one_line", [(DECLS, """    u32 kll,klr,krl,krr;           /* left half of key */
    u32 krll,krlr,krrl,krrr;       /* right half of key */
    u32 il, ir, t0, t1, w0, w1;    /* temporary variables */
    u32 kw4l, kw4r, dw, tl, tr;
    u32 subL[34], subR[34];
""")]),
    ("v5_temps_merged", [(DECLS, """    u32 kll,klr,krl,krr;           /* left half of key */
    u32 krll,krlr,krrl,krrr;       /* right half of key */
    u32 il, ir, t0, t1, w0, w1, kw4l, kw4r, dw, tl, tr;
    u32 subL[34];
    u32 subR[34];
""")]),
    ("v6_temps_split", [(DECLS, """    u32 kll,klr,krl,krr;           /* left half of key */
    u32 krll,krlr,krrl,krrr;       /* right half of key */
    u32 il, ir, t0, t1, w0, w1;    /* temporary variables */
    u32 kw4l, kw4r;
    u32 dw, tl, tr;
    u32 subL[34];
    u32 subR[34];
""")]),

    # --- liveness experiments: change the IR shape without changing the algorithm ---
    ("v7_nss_format", split_stmts),
    ("v8_kb_xor_form", lambda s: sub256(
        s, r"krll \^= kll; krlr \^= klr;\n[ \t]*krrl \^= krl; krrr \^= krr;",
        "krll = krll ^ kll; krlr = krlr ^ klr;\n    krrl = krrl ^ krl; krrr = krrr ^ krr;")),
    ("v9_ka_xor_form", lambda s: sub256(
        s, r"kll \^= krll; klr \^= krlr;",
        "kll = kll ^ krll; klr = klr ^ krlr;")),
    ("v10_ka_split", lambda s: sub256(
        s, r"kll = subl\(0\) \^ krll; klr = subr\(0\) \^ krlr;\n[ \t]*krl = subl\(1\) \^ krrl; krr = subr\(1\) \^ krrr;",
        "kll = subl(0); klr = subr(0);\n"
        "    krl = subl(1); krr = subr(1);\n"
        "    kll ^= krll; klr ^= krlr;\n"
        "    krl ^= krrl; krr ^= krrr;")),
    ("v11_ka_tmp", lambda s: sub256(
        s, r"kll = subl\(0\) \^ krll; klr = subr\(0\) \^ krlr;\n[ \t]*krl = subl\(1\) \^ krrl; krr = subr\(1\) \^ krrr;",
        "w0 = krll; w1 = krlr;\n"
        "    kll = subl(0) ^ w0; klr = subr(0) ^ w1;\n"
        "    w0 = krrl; w1 = krrr;\n"
        "    krl = subl(1) ^ w0; krr = subr(1) ^ w1;")),
    ("v12_kb_split", lambda s: sub256(
        s, r"krll \^= kll; krlr \^= klr;\n[ \t]*krrl \^= krl; krrr \^= krr;",
        "krll ^= kll;\n    krlr ^= klr;\n    krrl ^= krl;\n    krrr ^= krr;")),
    ("v13_kb_temp", lambda s: sub256(
        s, r"krll \^= kll; krlr \^= klr;",
        "w0 = kll; w1 = klr;\n    krll ^= w0; krlr ^= w1;")),
    ("v14_ka_swap_lines", lambda s: sub256(
        s, r"kll = subl\(0\) \^ krll; klr = subr\(0\) \^ krlr;\n[ \t]*krl = subl\(1\) \^ krrl; krr = subr\(1\) \^ krrr;",
        "krl = subl(1) ^ krrl; krr = subr(1) ^ krrr;\n"
        "    kll = subl(0) ^ krll; klr = subr(0) ^ krlr;")),
    ("v15_dead_read", lambda s: sub256(
        s, r"(CAMELLIA_ROLDQo32\(krll, krlr, krrl, krrr, w0, w1, 34\);)\n[ \t]*\n([ \t]*/\* generate KA \*/)",
        "\\1\n    w1 = krll;\n\n\\2")),
    ("v16_roldq_34_split", lambda s: sub256(
        s, r"CAMELLIA_ROLDQo32\(krll, krlr, krrl, krrr, w0, w1, 34\);\n[ \t]*\n([ \t]*/\* generate KA \*/)",
        "w0 = krll; w1 = krlr;\n"
        "    krll = (krlr << 2) + (krrl >> 30); krlr = (krrl << 2) + (krrr >> 30);\n"
        "    krrl = (krrr << 2) + (w0 >> 30); krrr = (w0 << 2) + (w1 >> 30);\n\n\\1")),

    # --- comma-operator statements: same instructions, different IR sequence points ---
    ("v17_comma_rl1", lambda s: split_comma_stmts(s, r"CAMELLIA_RL1")),
    ("v18_comma_all", split_comma_stmts),

    # --- variable-set experiments around subL[29] (the split live range) ---
    ("v19_unused_tmp", lambda s: sub256(
        s, r"(u32 kw4l, kw4r, dw, tl, tr;)", "\\1\n    u32 tmp;")),
    ("v20_xor_tmp", lambda s: sub256(
        sub256(s, r"(u32 kw4l, kw4r, dw, tl, tr;)", "\\1\n    u32 tmp;"),
        r"subl\(29\) \^= subl\(1\); subr\(29\) \^= subr\(1\);",
        "tmp = subl(29) ^ subl(1); subl(29) = tmp;\n"
        "    tmp = subr(29) ^ subr(1); subr(29) = tmp;")),
    ("v21_xor_dw", lambda s: sub256(
        s, r"subl\(29\) \^= subl\(1\); subr\(29\) \^= subr\(1\);",
        "dw = subl(29) ^ subl(1); subl(29) = dw;\n"
        "    dw = subr(29) ^ subr(1); subr(29) = dw;")),

    # --- absorb-region `dw` / CAMELLIA_RL1 chain (the level-4 evidence points here) ---
    ("v22_rl1_temp", lambda s: sub256(
        s, r"dw = subl\(1\) & subl\(9\), subr\(1\) \^= CAMELLIA_RL1\(dw\);",
        "dw = subl(1) & subl(9);\n    w0 = CAMELLIA_RL1(dw); subr(1) ^= w0;")),
    ("v23_rl1_inline", lambda s: sub256(
        s, r"subr\(1\) \^= CAMELLIA_RL1\(dw\);\n",
        "subr(1) ^= (dw << 1) + (dw >> 31);\n")),
    ("v24_rl1_temp_all", lambda s: sub256(
        s, r"subr\(1\) \^= CAMELLIA_RL1\(dw\);",
        "w0 = CAMELLIA_RL1(dw); subr(1) ^= w0;")),
]




def pragma_scope(src, lines, restore=("optimization_level 3", "peephole off", "scheduling off",
                                      "opt_common_subs on", "opt_propagation on")):
    """Insert `#pragma ...` lines before camellia_setup256 and restore defaults before setup192.

    `-opt` levers are global, but MWCC pragmas apply from their point of appearance onward, so a pragma
    pair can scope an optimizer setting to one function - which is exactly what a per-function
    allocation difference would need.
    """
    head = "".join("#pragma %s\n" % l for l in lines)
    tail = "".join("#pragma %s\n" % l for l in restore)
    if "void camellia_setup256(" not in src or "void camellia_setup192(" not in src:
        print("    (pragma scope: function markers not found)")
        return None
    src = src.replace("void camellia_setup256(", head + "void camellia_setup256(", 1)
    src = src.replace("void camellia_setup192(", tail + "void camellia_setup192(", 1)
    return src


# --- new-idea probes (2026-09): per-function pragmas around camellia_setup256 ---
VARIANTS += [
    # sanity check that pragmas are honoured at all: re-enable the peephole for this function only.
    ("v25_pragma_peep_on", lambda s: pragma_scope(s, ["peephole on"])),
    # level 4 fixed the frame globally; scope it to this function (setup128/others stay level 3).
    ("v26_pragma_opt4", lambda s: pragma_scope(s, ["optimization_level 4"])),
    # ... and try to suppress the level-4 side effect (the hoisted absorb XORs) while keeping the frame.
    ("v27_pragma_opt4_nocse", lambda s: pragma_scope(
        s, ["optimization_level 4", "opt_common_subs off"])),
    ("v28_pragma_opt4_noprop", lambda s: pragma_scope(
        s, ["optimization_level 4", "opt_propagation off"])),
]

VARIANTS += [
    # control: scheduling is already off globally, so this alone must be a no-op.
    ("v29_pragma_sched_off", lambda s: pragma_scope(s, ["scheduling off"])),
    # hypothesis: "#pragma optimization_level 4" may pull in the whole -O4 bundle (schedule included);
    # if so, level 4 + scheduling off keeps the fixed frame and drops the hoisted XORs.
    ("v30_pragma_opt4_nosched", lambda s: pragma_scope(s, ["optimization_level 4", "scheduling off"])),
    ("v31_pragma_opt4_nosched_nocse", lambda s: pragma_scope(
        s, ["optimization_level 4", "scheduling off", "opt_common_subs off"])),
]

# level 4 fixes the frame but hoists two absorb XORs. Sweep its sub-options to find the one that owns
# the reorder, so the allocation effect can be kept without it.
for _name, _lines in [
    ("v32_p4_nodeadcode", ["optimization_level 4", "opt_dead_code off"]),
    ("v33_p4_nodeadstore", ["optimization_level 4", "opt_dead_store off"]),
    ("v34_p4_nostrength", ["optimization_level 4", "opt_strength_reduction off"]),
    ("v35_p4_noloopinv", ["optimization_level 4", "opt_loop_invariants off"]),
    ("v36_p4_nocse2", ["optimization_level 4", "opt_cse off"]),
    ("v37_p4_nolifetimes", ["optimization_level 4", "opt_lifetimes off"]),
    ("v38_p4_noglobal", ["optimization_level 4", "opt_global off"]),
    ("v39_p4_space", ["optimization_level 4", "opt_space on"]),
]:
    VARIANTS.append((_name, (lambda lines: (lambda s: pragma_scope(s, lines)))(_lines)))

def with_pragma(lines):
    """Compose a source rewrite with a per-function pragma scope (returns None if the rewrite misses)."""
    def deco(fn):
        def wrapped(src):
            out = fn(src)
            return None if out is None else pragma_scope(out, lines)
        return wrapped
    return deco


# The level-4 pragma fixes the frame; the only residual is a 14-instruction window in the kw4 absorb
# chain (register choice + where CAMELLIA_RL1 is computed). Search source forms for that window.
P4 = ["optimization_level 4"]
VARIANTS += [
    ("v40_p4_kw4_split", with_pragma(P4)(lambda s: sub256(
        s, r"dw = tl & subl\(8\), tr = subr\(10\) \^ CAMELLIA_RL1\(dw\);",
        "dw = tl & subl(8); tr = subr(10) ^ CAMELLIA_RL1(dw);"))),
    ("v41_p4_rl1_temp", with_pragma(P4)(lambda s: sub256(
        s, r"dw = tl & subl\(8\), tr = subr\(10\) \^ CAMELLIA_RL1\(dw\);",
        "dw = tl & subl(8); w0 = CAMELLIA_RL1(dw); tr = subr(10) ^ w0;"))),
    ("v42_p4_and_swap", with_pragma(P4)(lambda s: sub256(
        s, r"dw = tl & subl\(8\)", "dw = subl(8) & tl"))),
    ("v43_p4_tl_form", with_pragma(P4)(lambda s: sub256(
        s, r"tl = subl\(10\) \^ \(subr\(10\) & ~subr\(8\)\);",
        "tl = subr(10) & ~subr(8); tl ^= subl(10);"))),
    ("v44_p4_tl_temp", with_pragma(P4)(lambda s: sub256(
        s, r"tl = subl\(10\) \^ \(subr\(10\) & ~subr\(8\)\);",
        "w0 = subr(10) & ~subr(8); tl = subl(10) ^ w0;"))),
]

VARIANTS += [
    # level 4 owns the fixed frame; if the residual 12-row window is a scheduler artefact, turning the
    # scheduler ON for this function only should reproduce the target's order.
    ("v45_p4_sched_on", lambda s: pragma_scope(s, ["optimization_level 4", "scheduling on"])),
    ("v46_p3_sched_on", lambda s: pragma_scope(s, ["scheduling on"])),
]
