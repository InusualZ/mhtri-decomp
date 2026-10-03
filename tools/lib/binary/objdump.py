"""Disassembler text: locate and run powerpc-eabi-objdump / dtk, and one tokenizer for their line shapes.
Spec: docs/tools/spec/lib-binary.md. CLI: none (library)."""
from __future__ import annotations

import os
import re
import shutil
import subprocess
import tempfile
from dataclasses import dataclass

#: Where the build puts binutils' objdump, relative to a tree root (Windows first).
OBJDUMP_RELS = (os.path.join("build", "binutils", "powerpc-eabi-objdump.exe"),
                os.path.join("build", "binutils", "powerpc-eabi-objdump"))
DTK_REL = os.path.join("build", "tools", "dtk.exe")

_SECTION_RE = re.compile(r"^Disassembly of section (?P<name>.+):$")
_LABEL_RE = re.compile(r"^(?P<addr>[0-9a-f]+) <(?P<name>.+)>:\s*$")
_RELOC_RE = re.compile(r"^\s+(?P<addr>[0-9a-f]+):\s+(?P<type>R_PPC_\S+)\s*(?P<symbol>.*?)\s*$")
_INSN_RE = re.compile(r"^(?P<indent>\s*)(?P<addr>[0-9a-f]+):\t(?P<body>.*)$")
# dtk (`dtk elf disasm` and the asm dump): `/* 80004000 00000100  94 21 FF F0 */\tstwu r1, -0x10(r1)`
_DTK_INSN_RE = re.compile(r"^/\*\s+(?P<addr>[0-9A-Fa-f]{8})\s+(?P<raw>[0-9A-Fa-f ]+?)\s*\*/\s*(?P<text>.+?)\s*$")
_DTK_HEADER_RE = re.compile(r"^#\s+(?P<sec>[.\w]+):0x(?P<off>[0-9A-Fa-f]+)\s+\|\s+0x(?P<addr>[0-9A-Fa-f]+)\s+\|")
_DTK_FN_RE = re.compile(r"^\.fn\s+(?P<name>[^,\s]+)")


@dataclass(frozen=True)
class Line:
    """One tokenized disassembler line.

    `kind` is `insn` (address, mnemonic, operands, text, raw), `label` (address, name), `section` (name),
    `reloc` (address, reloc = type, name = symbol), `header` (dtk's `# sec:0xOFF | 0xADDR |`: name = section,
    address = absolute, offset), `fn` / `endfn` (dtk's block markers: name). `mnemonic` keeps a record form's
    trailing `.`; `raw` is the raw-bytes column when the shape prints one; `body` is an objdump line's whole
    text after `addr:<TAB>`, stripped (raw column included).
    """
    kind: str
    address: int | None = None
    mnemonic: str = ""
    operands: str = ""
    text: str = ""
    raw: str = ""
    name: str = ""
    reloc: str = ""
    offset: int | None = None
    indented: bool = False
    body: str = ""


def split_text(text: str) -> tuple[str, str]:
    """`lis     r3,-32666` -> (`lis`, `r3,-32666`); the record-form dot is kept."""
    bits = text.split(None, 1)
    if not bits:
        return "", ""
    return bits[0], (bits[1].strip() if len(bits) > 1 else "")


def tokenize(line: str) -> Line | None:
    """The one tokenizer for objdump `-d[r]` lines and dtk's disassembly lines; None for anything else.

    It dispatches on the first character, so a line costs one or two regex attempts (a whole-DOL listing is
    millions of lines)."""
    if not line:
        return None
    first = line[0]
    if first == "/":
        m = _DTK_INSN_RE.match(line)
        if not m:
            return None
        text = m.group("text")
        mnemonic, operands = split_text(text)
        return Line("insn", int(m.group("addr"), 16), mnemonic, operands, text, m.group("raw"))
    if first in "#.":
        m = _DTK_HEADER_RE.match(line)
        if m:
            return Line("header", int(m.group("addr"), 16), name=m.group("sec"), offset=int(m.group("off"), 16))
        m = _DTK_FN_RE.match(line)
        if m:
            return Line("fn", name=m.group("name"))
        if line.startswith(".endfn"):
            return Line("endfn")
        return None
    if first == "D":
        m = _SECTION_RE.match(line)
        return Line("section", name=m.group("name")) if m else None
    if first not in " \t":
        m = _LABEL_RE.match(line)
        if m:
            return Line("label", int(m.group("addr"), 16), name=m.group("name"))
    elif "R_" in line:
        m = _RELOC_RE.match(line)
        if m:
            return Line("reloc", int(m.group("addr"), 16), reloc=m.group("type"), name=m.group("symbol"))
    m = _INSN_RE.match(line)
    if m:
        whole = body = m.group("body")
        raw = ""
        if "\t" in body:
            raw, body = body.rsplit("\t", 1)
        text = body.strip()
        mnemonic, operands = split_text(text)
        if not mnemonic:
            return None
        return Line("insn", int(m.group("addr"), 16), mnemonic, operands, text, raw.strip(),
                    indented=bool(m.group("indent")), body=whole.strip())
    return None


def locate(root: str | os.PathLike | None = None) -> str | None:
    """The tree's `build/binutils` objdump, else one on PATH (`powerpc-eabi-objdump`, `objdump`), else None."""
    if root is not None:
        for rel in OBJDUMP_RELS:
            path = os.path.join(os.fspath(root), rel)
            if os.path.exists(path):
                return path
    for name in ("powerpc-eabi-objdump", "objdump"):
        hit = shutil.which(name)
        if hit:
            return hit
    return None


def run_objdump(objdump: str | os.PathLike, args: list[str], path: str | os.PathLike,
                check: bool = False) -> subprocess.CompletedProcess:
    """`objdump <args> <path>` with the codec rule (UTF-8, errors replaced); raises on a non-zero exit if
    `check`."""
    return subprocess.run([os.fspath(objdump), *args, os.fspath(path)], capture_output=True, text=True,
                          encoding="utf-8", errors="replace", check=check)


def disassemble(objdump: str | os.PathLike, path: str | os.PathLike, sections: list[str] | None = None,
                relocs: bool = False, raw: bool = True, cpu: str | None = None) -> str:
    """The `objdump -d` text of `path` (an object or a linked ELF); raises RuntimeError on failure."""
    args = ["-d"]
    if cpu:
        args = ["-M", cpu] + args
    if relocs:
        args.append("-r")
    if not raw:
        args.append("--no-show-raw-insn")
    for name in sections or ():
        args += ["-j", name]
    proc = run_objdump(objdump, args, path)
    if proc.returncode != 0:
        raise RuntimeError("objdump failed (%d): %s" % (proc.returncode, (proc.stderr or "").strip()[:400]))
    return proc.stdout


def dtk_disasm(obj: str | os.PathLike, dtk: str | os.PathLike, cwd: str | os.PathLike | None = None) -> str:
    """`dtk elf disasm <obj>` as text (through a temporary file); raises RuntimeError on failure."""
    fd, tmp = tempfile.mkstemp(prefix="dtk-disasm-", suffix=".s")
    os.close(fd)
    os.remove(tmp)
    try:
        proc = subprocess.run([os.fspath(dtk), "elf", "disasm", os.fspath(obj), tmp], cwd=cwd,
                              capture_output=True, text=True, encoding="utf-8", errors="replace")
        if proc.returncode != 0 or not os.path.exists(tmp):
            raise RuntimeError("dtk elf disasm failed: " + ((proc.stdout or "") + (proc.stderr or "")).strip()[:200])
        with open(tmp, "r", encoding="utf-8", errors="replace") as handle:
            return handle.read()
    finally:
        if os.path.exists(tmp):
            os.remove(tmp)
