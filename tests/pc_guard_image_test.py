"""Verify hardware executable windows against every function in a linked ELF32 image."""
import struct,sys
from pathlib import Path
b=Path(sys.argv[1]).read_bytes();assert b[:6]==b'\x7fELF\x01\x01'
shoff=struct.unpack_from('<I',b,32)[0];shentsize,shnum=struct.unpack_from('<HH',b,46)
sections=[struct.unpack_from('<10I',b,shoff+i*shentsize) for i in range(shnum)]
syms={};funcs=[]
for sec in sections:
 if sec[1]!=2:continue
 strings=sections[sec[6]];st=b[strings[4]:strings[4]+strings[5]]
 for o in range(sec[4],sec[4]+sec[5],sec[9]):
  n,addr,size,info,other,index=struct.unpack_from('<IIIBBH',b,o)
  name=st[n:st.find(b'\0',n)].decode();syms[name]=addr
  if info&15==2 and index and addr:funcs.append((name,addr,size))
xlo,xhi=0x02000120,syms['_etext'];rlo,rhi=syms['_rt_start'],syms.get('_guard_ram_end',syms['_rt_end'])
excluded=[(n,a,z) for n,a,z in funcs if not(xlo<=a and a+z<=xhi+1 or rlo<=a and a+z<=rhi)]
print('PC windows:',hex(xlo),hex(xhi),'and',hex(rlo),hex(rhi-1))
print('Functions tested:',len(funcs),'excluded:',[(n,hex(a),z) for n,a,z in excluded])
if '--expect-old-failure' in sys.argv:
 assert {n for n,a,z in excluded}=={'p5_samples','p5_mod_samples','p5_dc_samples'}
 print('REPRODUCED: the previous firmware excludes all three native Prophet RAM kernels.')
else:
 assert not excluded
 for n in ['p5_samples','p5_mod_samples','p5_dc_samples']:
  assert syms['_data_start']<=syms[n]<syms['_data_end']
  assert rlo<=syms[n]<rhi
 assert syms['_guard_ram_end']==max(syms['_rt_end'],syms['_p5_text_end'])
 print('PASS: every linked function and native Prophet kernel is covered by the hardware PC guards.')
