from pathlib import Path
import json, struct, zipfile
import pefile, capstone
from scripts.prepare_ipc_c32 import identity

package=Path('work/steam-only-delivery/l4d2-bridge-steam-only-dev-2da41c976cf19b26-r38334235685.zip')
words=[int(identity()[i*8:i*8+8],16) for i in range(4)]
evidence={}
with zipfile.ZipFile(package) as z:
    for name in json.loads(z.read('BUILD-INFO.json'))['files']:
        raw=z.read(name);pe=pefile.PE(data=raw)
        # These optimized MSVC constructors store separate immediates rather
        # than keeping a single contiguous 16-byte constant in .rdata.
        word_offset=raw.find(struct.pack('<I',words[0]));assert word_offset>=3
        rva=pe.get_rva_from_offset(word_offset-3)
        mode=capstone.CS_MODE_64 if pe.FILE_HEADER.Machine==0x8664 else capstone.CS_MODE_32
        decoder=capstone.Cs(capstone.CS_ARCH_X86,mode);decoder.detail=True
        instructions=list(decoder.disasm(pe.get_data(rva,40),pe.OPTIONAL_HEADER.ImageBase+rva))[:4]
        assert len(instructions)==4
        registers=set();displacements=[];stores=[]
        for index,ins in enumerate(instructions):
            assert ins.mnemonic=='mov'
            target,value=ins.operands
            assert target.type==capstone.x86.X86_OP_MEM and value.type==capstone.x86.X86_OP_IMM
            assert value.imm & 0xffffffff == words[index]
            assert target.size==4 or index==3 and target.size==8
            registers.add((target.mem.base,target.mem.index));displacements.append(target.mem.disp)
            stores.append({'rva':hex(ins.address-pe.OPTIONAL_HEADER.ImageBase),'instruction':ins.mnemonic+' '+ins.op_str})
        assert len(registers)==1 and displacements==[displacements[0]+4*i for i in range(4)]
        evidence[name]={'actual_build_words':[hex(w) for w in words],'compiled_stores':stores}
Path('work/steam-only-delivery/compiled-protocol-verification.json').write_text(json.dumps(evidence,indent=2)+'\n')
print('PASS actual four-word compiled protocol identity in all three optimized binaries')
