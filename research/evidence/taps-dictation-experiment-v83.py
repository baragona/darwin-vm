import json,lldb,time
BASE=0x102bcc000
UUID=bytes.fromhex('a52507a3968b3b3d9c110aaefc5d2b07')
count=0
def restricted(frame,bp_loc,internal_dict):
 global count
 p=frame.GetThread().GetProcess();e=lldb.SBError();h=p.ReadMemory(BASE,0x1000,e)
 if e.Fail() or not h or h[:4]!=b'\xcf\xfa\xed\xfe' or UUID not in h:return False
 count+=1;lr=frame.FindRegister('lr').GetValueAsUnsigned()
 ok=frame.FindRegister('x0').SetValueFromCString('1') and frame.FindRegister('pc').SetValueFromCString(hex(lr))
 row={'event':'notebook_dictation_restricted','count':count,'base':hex(BASE),'lr':hex(lr),'success':ok,'time':time.monotonic()}
 with open('/tmp/a19-dictation-v83.jsonl','a') as f:f.write(json.dumps(row)+'\n')
 if count<=5:print('DICTATION_RESTRICTED',row)
 return not ok
