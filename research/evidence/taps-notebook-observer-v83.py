import lldb,json,struct,time
from pathlib import Path
EXPECTED=bytes.fromhex('a52507a3968b3b3d9c110aaefc5d2b07')
seen=set()
def appmain(frame,bp_loc,internal_dict):
 p=frame.GetThread().GetProcess();t=p.GetTarget();e=lldb.SBError()
 lr=frame.FindRegister('lr').GetValueAsUnsigned()&0x7fffffffff;base=lr-0x4828
 if not 0x100000000<=base<0x180000000:return False
 data=p.ReadMemory(base,0x1000,e)
 if e.Fail() or data[:4]!=b'\xcf\xfa\xed\xfe':return False
 offset=32;identity=None
 for _ in range(min(struct.unpack_from('<I',data,16)[0],64)):
  if offset+8>len(data):break
  cmd,size=struct.unpack_from('<II',data,offset)
  if size<8 or offset+size>len(data):break
  if cmd==0x1b:identity=data[offset+8:offset+24];break
  offset+=size
 if identity!=EXPECTED:return False
 if base not in seen:
  bp=t.BreakpointCreateByAddress(base+0x5e00);bp.SetScriptCallbackFunction('a19_notebook_v83.event');seen.add(base)
 row={'event':'notebook_main','base':hex(base),'uuid':identity.hex(),'lr':hex(lr)}
 with Path('/tmp/a19-notebook-v83.jsonl').open('a') as f:f.write(json.dumps(row)+'\n')
 print('NOTEBOOK_MAIN',row);return False

def event(frame,bp_loc,internal_dict):
 p=frame.GetThread().GetProcess();e=lldb.SBError()
 value=p.ReadCStringFromMemory(frame.FindRegister('x0').GetValueAsUnsigned()&0x00ffffffffffffff,256,e)
 row={'event':'notebook_log','message':value if e.Success() else str(e),'host_monotonic':time.monotonic()}
 with Path('/tmp/a19-notebook-v83.jsonl').open('a') as f:f.write(json.dumps(row)+'\n')
 print('NOTEBOOK_LOG',row);return False
