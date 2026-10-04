"""mwlink.catalogue: RT_STRING decoding, the message ids, the phase timeline and the diagnostic classifier."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import struct

from tools.lib import testing
from tools.mwlink.catalogue import (classify_diagnostics, classify_phase, decode_string_block, match_message,
                                    message_catalogue, parse_timeline, phase_kinds)

TIER = "fixture"


class FakePe:
    """The two calls the catalogue makes of a PE: `read_rva` and `string_blocks`."""

    def __init__(self, blob, blocks=()):
        self.blob, self.blocks = blob, list(blocks)

    def read_rva(self, rva, n):
        return self.blob[rva:rva + n]

    def string_blocks(self):
        return self.blocks


def block(*texts):
    return b"".join(struct.pack("<H", len(t)) + t.encode("utf-16le") for t in texts)


def test_rt_string_decode(c):
    raw = block("Link", "", "Linking: '%c'")
    c.check("length-prefixed UTF-16LE with an empty slot", decode_string_block(FakePe(raw), 0, len(raw)),
            [(0, "Link"), (2, "Linking: '%c'")])


def test_message_ids(c):
    # the first block is named 1 and holds ids 0..15: msgid = (block_name - 1) * 16 + slot (phases --prove: 27/29/41)
    first = block("zero")
    second = block(*[""] * 11, "Linking: '%c'")
    blob = first + second
    pe = FakePe(blob, [(1, 0, len(first)), (2, len(first), len(second))])
    cat = message_catalogue(pe)
    c.check("block 1 slot 0 is id 0", cat.get(0), "zero")
    c.check("block 2 slot 11 is id 27, the id the loader asks for", cat.get(27), "Linking: '%c'")
    c.check("nothing else", sorted(cat), [0, 27])


def test_timeline(c):
    tl = parse_timeline("#   Compiling: 'a.o'\n#   Linking: 'x.elf'\n#   Optimizing: 'x.elf'\n"
                        "#   Layout: 'x.elf' (.text)\nnot a phase\n")
    c.check("timeline kinds", [k for k, _ in tl], ["Compiling", "Linking", "Optimizing", "Layout"])
    cat = {43: "Linking: '%c'", 58: "Layout: '%c' (%c)"}
    c.check("classify_phase maps a Layout line to its msgid", classify_phase("#   Layout: 'x.elf' (.text)", cat)[0], 58)
    c.check("phase kinds are read off the catalogue", phase_kinds(cat), {"Linking", "Layout"})
    c.check("a catalogue with no phase message falls back to the five names", phase_kinds({1: "x"}),
            {"Linking", "Optimizing", "Writing", "Layout", "Compiling"})
    only = parse_timeline("#   Linking: 'x'\n#   undefined: 'foo'\n", {"Linking"})
    c.check("with kinds, a body line shaped like a phase is not one", only, [("Linking", "#   Linking: 'x'")])


def test_diagnostics(c):
    cat = {27: "Linking: '%c'", 41: "Optimizing: '%c'",
           189: "runtime sources 'global_destructor_chain.c' and '__init_cpp_exceptions.cpp' both need to be "
                "updated to latest version.  Please contact Freescale support."}
    c.check("match_message: a phase line", match_message("  Linking: 'x.elf'", cat)[0], 27)
    c.check("match_message: unknown text stays unknown", match_message("#   something the catalogue does not have", cat),
            None)
    text = ("#   Linking: 'x.elf'\n#   Optimizing: 'x.elf'\n### mwldeppc.exe Linker Error:\n"
            "#   runtime sources 'global_destructor_chain.c' and '__init_cpp_exceptions.cpp' both need to be updated "
            "to latest version.  Please contact Freescale support.\n")
    diags = classify_diagnostics(text, cat)
    c.check("one banner + body is one diagnostic", len(diags), 1)
    c.check("its id comes from the catalogue", diags[0]["msgid"] if diags else None, 189)
    c.check("it is attributed to the last phase before it", diags[0]["phase"] if diags else None, "Optimizing")
    c.check("a run with no banner reports none", classify_diagnostics("#   nothing to see here", cat), [])
    odd = classify_diagnostics("### mwldeppc.exe Linker Error:\n#   a message nobody catalogued\n", cat)
    c.check("an uncatalogued diagnostic keeps no id (never invented)", [(d["msgid"], d["phase"]) for d in odd],
            [(None, None)])


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
