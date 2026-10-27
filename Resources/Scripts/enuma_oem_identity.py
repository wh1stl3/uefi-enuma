"""Generate a relocation-normalized text fingerprint for an exact staged PE."""
import struct
from pathlib import Path
from enuma_oem_profile import ORIGINAL, PATCHED, sha


def generate(pe, output, runtime=None):
    b=Path(pe).read_bytes();digest=sha(b)
    if digest not in (ORIGINAL,PATCHED):raise ValueError('unknown Display PE')
    p=struct.unpack_from('<I',b,60)[0];optional=p+24
    image_base=struct.unpack_from('<Q',b,optional+24)[0]
    entry=struct.unpack_from('<I',b,optional+16)[0]
    size=struct.unpack_from('<I',b,optional+56)[0]
    table=optional+struct.unpack_from('<H',b,p+20)[0];sections=[]
    for i in range(struct.unpack_from('<H',b,p+6)[0]):
        off=table+40*i;name=b[off:off+8].rstrip(b'\0');virtual,rva,raw_size,raw=struct.unpack_from('<IIII',b,off+8)
        sections.append((name,rva,raw_size,raw))
    text=next(s for s in sections if s[0]==b'.text')
    def offset(rva):
        hits=[s for s in sections if s[1]<=rva<s[1]+s[2]]
        if len(hits)!=1:raise ValueError('invalid RVA')
        return hits[0][3]+rva-hits[0][1]
    rr,rn=struct.unpack_from('<II',b,optional+112+5*8);cursor=offset(rr);end=cursor+rn;relocs=[]
    while cursor<end:
        page,n=struct.unpack_from('<II',b,cursor)
        if not n:break
        if n<8 or n%2 or cursor+n>end:raise ValueError('bad relocation block')
        for x in struct.unpack_from('<'+'H'*((n-8)//2),b,cursor+8):
            kind=x>>12;rva=page+(x&4095)
            if kind not in (0,10):raise ValueError('unsupported relocation')
            if kind==10 and text[1]<=rva<text[1]+text[2]:relocs.append(rva-text[1])
        cursor+=n
    relocs.sort()
    if any(x+8>text[2] or (i and x<relocs[i-1]+8) for i,x in enumerate(relocs)):raise ValueError('overlapping text relocations')
    h=14695981039346656037
    for v in b[text[3]:text[3]+text[2]]:h=((h^v)*1099511628211)&((1<<64)-1)
    metadata=dict(pe_sha256=digest,image_size=size,entry_rva=entry,text_rva=text[1],text_size=text[2],
                  preferred_base=image_base,text_fnv64=hex(h),text_relocations=len(relocs),cryptographic_authentication=False)
    header='// Generated from the hash-verified build-private OEM PE. Not a signature.\n'
    session=runtime['session'] if runtime else 1
    header+=f'#define ENUMA_OEM_RUNTIME {1 if runtime else 0}\n#define ENUMA_OEM_SESSION 0x{session:x}ULL\n#define ENUMA_OEM_SELECTED_CALL 0ULL\n'
    header+=f'#define ENUMA_OEM_PROGRESS {1 if runtime and runtime["experiment"]=="oem-progress-v2" else 0}\n'
    for name,value in [('SIZE',size),('ENTRY',entry),('TEXT_RVA',text[1]),('TEXT_SIZE',text[2]),('BASE',image_base),('HASH',h)]:
        header+=f'#define ENUMA_OEM_ID_{name} 0x{value:x}ULL\n'
    header+='STATIC CONST UINT32 EnumaOemTextRelocs[]={'+','.join(hex(x) for x in relocs)+'};\n'
    Path(output).write_text(header)
    metadata['header_sha256']=sha(header.encode());metadata['runtime']=runtime;return metadata
