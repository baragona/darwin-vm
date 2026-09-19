import json
counts={}
def event(frame,bp_loc,internal_dict):
 def reg(n):return frame.FindRegister(n).GetValueAsUnsigned()
 pc=reg('pc');n=counts.get(pc,0)+1;counts[pc]=n
 if n<=100:
  row={'pc':hex(pc),'count':n,'receiver':hex(reg('x0')),'event':hex(reg('x2')),'lr':hex(reg('lr'))}
  print('INPUT_ROUTE',json.dumps(row))
  with open('/tmp/a19-route-v77.jsonl','a') as f:f.write(json.dumps(row)+'\n')
 return False
def geometry(frame,bp_loc,internal_dict):
 import lldb,struct
 sp=frame.FindRegister('sp').GetValueAsUnsigned();err=lldb.SBError()
 data=frame.GetThread().GetProcess().ReadMemory(sp+0x40,64,err)
 row={'stage':'geometry','success':err.Success(),'values':list(struct.unpack('<8d',data)) if len(data)==64 else []}
 print('GEOMETRY',row)
 with open('/tmp/a19-route-geometry-v77.jsonl','a') as f:f.write(json.dumps(row)+'\n')
 return False
