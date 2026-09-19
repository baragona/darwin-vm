import lldb,struct,json,time
from pathlib import Path
UUID=bytes.fromhex('d68a368df424336f885880a3ce81ee93')
seen=set()
def level(frame,bp_loc,internal_dict):
 p=frame.GetThread().GetProcess();e=lldb.SBError()
 lr=frame.FindRegister('lr').GetValueAsUnsigned()&0x7fffffffff
 if not 0x100000000<=lr<0x180000000:return False
 found=None
 for base in range(lr&~0x3fff,(lr&~0x3fff)-0x10000,-0x4000):
  data=p.ReadMemory(base,0x1000,e)
  if e.Fail() or len(data)!=0x1000 or data[:4]!=b'\xcf\xfa\xed\xfe':continue
  count=struct.unpack_from('<I',data,16)[0];offset=32
  for _ in range(min(count,64)):
   if offset+8>len(data):break
   cmd,size=struct.unpack_from('<II',data,offset)
   if size<8 or offset+size>len(data):break
   if cmd==0x1b and data[offset+8:offset+24]==UUID:found=base;break
   offset+=size
  if found is not None:break
 if found is None:return False
 before=frame.FindRegister('x4').GetValueAsUnsigned()
 if before!=1:return False
 ok=frame.FindRegister('x4').SetValueFromCString('6')
 row={'event':'agent_compression_level','uuid':UUID.hex(),'base':hex(found),'lr':hex(lr),'input_bytes':frame.FindRegister('x3').GetValueAsUnsigned(),'before':before,'after':6,'success':ok,'host_monotonic':time.monotonic()}
 with Path('/tmp/a19-compression-v83.jsonl').open('a') as f:f.write(json.dumps(row)+'\n')
 print('AGENT_COMPRESSION',row)
 return not ok
