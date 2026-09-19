import lldb,json,struct
def sample(frame,bp_loc,internal_dict):
 def r(n):return frame.FindRegister(n).GetValueAsUnsigned()
 row={'pc':hex(r('pc')),'registers':{n:hex(r(n)) for n in ['x0','x1','x2','x3','x8','x19','x21','x23','x28']}}
 if r('pc')==0x22d5b97c0:
  e=lldb.SBError();data=frame.GetThread().GetProcess().ReadMemory(r('x3')&0x0000ffffffffffff,32,e)
  row['destination_hex']=data.hex();row['read_success']=e.Success()
  if len(data)==32:row.update(target=struct.unpack_from('<I',data,8)[0],task=struct.unpack_from('<I',data,12)[0],connection=hex(struct.unpack_from('<Q',data,16)[0]))
 print('DESTINATION',json.dumps(row))
 with open('/tmp/a19-destination-v77.jsonl','a') as f:f.write(json.dumps(row)+'\n')
 return False
