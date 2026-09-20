import sys, struct
def sections(path):
    d=open(path,'rb').read()
    assert d[:4]==b'\x7fELF', "not ELF"
    is64 = d[4]==2
    be = d[5]==2
    e = '>' if be else '<'
    if is64:
        e_shoff,=struct.unpack_from(e+'Q',d,0x28); e_shentsize,=struct.unpack_from(e+'H',d,0x3a); e_shnum,=struct.unpack_from(e+'H',d,0x3c); e_shstrndx,=struct.unpack_from(e+'H',d,0x3e)
    else:
        e_shoff,=struct.unpack_from(e+'I',d,0x20); e_shentsize,=struct.unpack_from(e+'H',d,0x2e); e_shnum,=struct.unpack_from(e+'H',d,0x30); e_shstrndx,=struct.unpack_from(e+'H',d,0x32)
    hdrs=[]
    for i in range(e_shnum):
        off=e_shoff+i*e_shentsize
        if is64:
            name,typ,flags,addr,offset,size,link,info,align,entsize=struct.unpack_from(e+'IIQQQQIIQQ',d,off)
        else:
            name,typ,flags,addr,offset,size,link,info,align,entsize=struct.unpack_from(e+'IIIIIIIIII',d,off)
        hdrs.append(dict(name_off=name,typ=typ,addr=addr,offset=offset,size=size,link=link,entsize=entsize))
    strtab=hdrs[e_shstrndx]
    def nm(o):
        end=d.index(b'\0',strtab['offset']+o); return d[strtab['offset']+o:end].decode('latin1')
    for h in hdrs:
        h['name']=nm(h['name_off']); h['data']=d[h['offset']:h['offset']+h['size']]
    return d,hdrs
if __name__=='__main__':
    args = sys.argv[1:]
    # `-u <unit>` prints the split target object next to ours, which is the usual comparison
    if args[:1] in (['-u'], ['--unit']):
        import os
        sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))  # tools/
        import unitutil as uu
        unit = uu.resolve_unit(args[1] if len(args) > 1 else None)
        args = [unit.target, unit.obj]
    if not args:
        raise SystemExit(__doc__ or "usage: elfsect.py [-u <unit>] [<obj> ...]")
    for p in args:
        d,hs=sections(p)
        print(f"== {p} ({len(d)} bytes)")
        for h in hs:
            extra=''
            if h['name']=='.comment':
                extra=' bytes='+h['data'].hex()
            print(f"   {h['name']:<16} type={h['typ']:<3} size={h['size']:<8} addr=0x{h['addr']:X}{extra}")
