import lldb,json,struct
counts={}
def sample(frame,bp_loc,internal_dict):
 p=frame.GetThread().GetProcess()
 def r(n):return frame.FindRegister(n).GetValueAsUnsigned()
 def mem(a,n):
  e=lldb.SBError();b=p.ReadMemory(a&0x0000ffffffffffff,n,e)
  return b if e.Success() and len(b)==n else b''
 pc=r('pc');counts[pc]=counts.get(pc,0)+1
 row={'pc':hex(pc),'count':counts[pc],'registers':{n:hex(r(n)) for n in ['x0','x1','x2','x3','x8','x9','x19','x20','lr']}}
 if pc==0x22d4f1154:
  b=mem(r('x19')+0x20,4);row['receiver_pid']=struct.unpack('<I',b)[0] if b else None
 if pc==0x191991cf4:
  b=mem(r('x3')+0xc0,32);row['callbacks_raw']=b.hex()
 print('RECEIVER',json.dumps(row))
 with open('/tmp/a19-receiver-v77.jsonl','a') as f:f.write(json.dumps(row)+'\n')
 return False
