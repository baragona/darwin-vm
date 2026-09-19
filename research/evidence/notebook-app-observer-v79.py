import lldb,json
log='/tmp/a19-notebook-events-v79.jsonl'
def event(frame,bp_loc,internal_dict):
 p=frame.GetThread().GetProcess();e=lldb.SBError()
 message=p.ReadCStringFromMemory(frame.FindRegister('x0').GetValueAsUnsigned()&0x00ffffffffffffff,256,e)
 data={'event':'app_log','message':message if e.Success() else str(e)}
 print('NOTEBOOK_EVENT',json.dumps(data))
 with open(log,'a') as f:f.write(json.dumps(data)+'\n')
 return False
