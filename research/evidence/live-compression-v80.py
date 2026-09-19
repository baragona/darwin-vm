import lldb,struct,uuid,time,json
from pathlib import Path
expected=uuid.UUID('DC30B50A-41D4-3289-9EF8-F7DB4A8094D5').bytes
base=None
last=None
def log(row):
 print('LIVE_TIMING',row)
 with open('/tmp/a19-live-timing-v80.jsonl','a') as f:f.write(json.dumps(row)+'\n')
def compress(frame,bp_loc,internal_dict):
 global base,last
 p=frame.GetThread().GetProcess();e=lldb.SBError();lr=frame.FindRegister('lr').GetValueAsUnsigned()&0x7fffffffff
 candidate=lr-0x5878
 if base is None:
  b=p.ReadMemory(candidate,4096,e)
  if e.Fail() or len(b)!=4096 or b[:4]!=bytes.fromhex('cffaedfe'):return False
  n=struct.unpack_from('<I',b,16)[0];off=32;found=False
  for _ in range(n):
   cmd,size=struct.unpack_from('<II',b,off)
   if cmd==0x1b and b[off+8:off+24]==expected:found=True
   if size<8:break
   off+=size
   if off+8>len(b):break
  if not found:return False
  base=candidate
  for offset,fn in [(0x5878,'compressed'),(0x5710,'rendered')]:
   bp=p.GetTarget().BreakpointCreateByAddress(base+offset);bp.SetScriptCallbackFunction('a19_live_compress_v80.'+fn)
  log({'event':'verified_agent','base':hex(base)})
 if candidate!=base:return False
 before=frame.FindRegister('w4').GetValueAsUnsigned();ok=frame.FindRegister('w4').SetValueFromCString('6')
 last=time.monotonic();log({'event':'compress_start','before':before,'level':6,'ok':ok,'time':last})
 return not ok
def compressed(frame,bp_loc,internal_dict):
 p=frame.GetThread().GetProcess();e=lldb.SBError();sp=frame.FindRegister('sp').GetValueAsUnsigned()
 count=p.ReadUnsignedFromMemory(sp+0x98,8,e)
 log({'event':'compress_end','bytes':count,'duration':time.monotonic()-last if last else None,'result':frame.FindRegister('w0').GetValueAsUnsigned()})
 return False
def rendered(frame,bp_loc,internal_dict):
 log({'event':'render_return','time':time.monotonic(),'result':frame.FindRegister('w0').GetValueAsUnsigned()});return False
