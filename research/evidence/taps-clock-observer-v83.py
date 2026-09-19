import json,lldb,time
from pathlib import Path

def capture(frame,bp_loc,internal_dict):
 p=frame.GetThread().GetProcess();e=lldb.SBError()
 regs={r:frame.FindRegister(r).GetValueAsUnsigned() for r in ['pc','cpsr','x8','x22','x24','x25','sp','lr']}
 data={'event':'scheduler_nonmonotonic_comparison','host_time':time.time(),'registers':{k:hex(v) for k,v in regs.items()}}
 ptr=p.ReadUnsignedFromMemory(regs['x25']+0x1b8,8,e)
 if e.Success():
  offset=p.ReadUnsignedFromMemory(ptr+0x58,8,e)
  if e.Success():data.update(cpu_data=hex(ptr),timebase_offset=hex(offset))
 Path('/tmp/a19-clock-panic-v83.json').write_text(json.dumps(data,indent=2)+'\n')
 print('SCHEDULER_CLOCK_FAILURE',data)
 return True
