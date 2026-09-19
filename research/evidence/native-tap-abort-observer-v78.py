import json,lldb
from pathlib import Path
def capture(frame,bp_loc,internal_dict):
 p=frame.GetThread().GetProcess()
 def r(n):return frame.FindRegister(n).GetValueAsUnsigned()
 row={'registers':{n:hex(r(n)) for n in ['pc','lr','sp','fp','x0','x1','x2','x3','x19','x20','x21']},'frames':[]}
 fp=r('fp'); seen=set()
 for i in range(48):
  if not fp or fp in seen:break
  seen.add(fp); e=lldb.SBError(); data=p.ReadMemory(fp,16,e)
  if not e.Success() or len(data)!=16:break
  nxt=int.from_bytes(data[:8],'little'); lr=int.from_bytes(data[8:],'little')
  row['frames'].append({'fp':hex(fp),'lr_raw':hex(lr),'lr_stripped':hex(lr&0x7fffffffff)})
  if nxt<=fp or nxt-fp>0x100000:break
  fp=nxt
 with Path('/tmp/a19-abort-v78.jsonl').open('a') as f:f.write(json.dumps(row)+'\n')
 print('ABORT_CAPTURE',json.dumps(row))
 return True
