"""ELF section dumper and the importable `sections()` reader (a view over lib.binary.elf).
Spec: docs/tools/spec/elfsect.md. CLI: elfsect.py [-u <unit>] [<obj> ...]."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

from tools.lib.binary.elf import Elf, ElfError


def sections(path):
    """(file bytes, [section header dicts]) - keys `name`, `name_off`, `typ`, `addr`, `offset`, `size`, `link`,
    `entsize`, `data` (the file slice, NOBITS included). AssertionError when `path` is not an ELF file."""
    try:
        elf = Elf.read(path)
    except ElfError as exc:
        if "not an ELF" in str(exc):
            raise AssertionError("not ELF") from None
        raise
    hdrs = [dict(name_off=s.name_offset, typ=s.type, addr=s.addr, offset=s.offset, size=s.size, link=s.link,
                 entsize=s.entsize, name=s.name, data=s.raw) for s in elf.sections]
    return elf.raw, hdrs


if __name__ == '__main__':
    args = sys.argv[1:]
    # `-u <unit>` prints the split target object next to ours, which is the usual comparison
    if args[:1] in (['-u'], ['--unit']):
        import os
        sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))  # tools/ (unitutil's own imports)
        import unitutil as uu
        unit = uu.resolve_unit(args[1] if len(args) > 1 else None)
        args = [unit.target, unit.obj]
    if not args:
        raise SystemExit("usage: elfsect.py [-u <unit>] [<obj> ...]")
    for p in args:
        d, hs = sections(p)
        print(f"== {p} ({len(d)} bytes)")
        for h in hs:
            extra = ''
            if h['name'] == '.comment':
                extra = ' bytes=' + h['data'].hex()
            print(f"   {h['name']:<16} type={h['typ']:<3} size={h['size']:<8} addr=0x{h['addr']:X}{extra}")
