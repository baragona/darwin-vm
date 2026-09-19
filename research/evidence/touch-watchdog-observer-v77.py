import lldb,struct,json
log='/tmp/a19-watchdog-v77.jsonl'
def scale(frame,bp_loc,internal_dict):
 r=frame.FindRegister('d0');before=r.GetValue();ok=r.SetValueFromCString('60.0')
 data={'event':'watchdog_scale','before':before,'after':r.GetValue(),'write_success':ok}
 print('WATCHDOG_SCALE',json.dumps(data))
 with open(log,'a') as f:f.write(json.dumps(data)+'\n')
 return not ok
def appmain(frame,bp_loc,internal_dict):
 data={'event':'UIApplicationMain','pc':frame.FindRegister('pc').GetValue(),'delegate':frame.FindRegister('x3').GetValue(),'lr':frame.FindRegister('lr').GetValue()}
 print('APP_MAIN',json.dumps(data))
 with open(log,'a') as f:f.write(json.dumps(data)+'\n')
 return False
def factor(frame,bp_loc,internal_dict):
 r=frame.FindRegister('d8');before=r.GetValue();ok=r.SetValueFromCString('60.0')
 data={'event':'watchdog_factor','before':before,'after':r.GetValue(),'write_success':ok}
 print('WATCHDOG_FACTOR',json.dumps(data))
 with open(log,'a') as f:f.write(json.dumps(data)+'\n')
 return not ok
event_bp=None
def event(frame,bp_loc,internal_dict):
 p=frame.GetThread().GetProcess();e=lldb.SBError()
 message=p.ReadCStringFromMemory(frame.FindRegister('x0').GetValueAsUnsigned()&0x00ffffffffffffff,256,e)
 data={'event':'app_log','message':message if e.Success() else str(e)}
 print('TOUCH_EVENT',json.dumps(data))
 with open(log,'a') as f:f.write(json.dumps(data)+'\n')
 return False
def appmain(frame,bp_loc,internal_dict):
 global event_bp
 p=frame.GetThread().GetProcess();t=p.GetTarget();e=lldb.SBError()
 base=(frame.FindRegister('lr').GetValueAsUnsigned()&0x7fffffffff)-0x4568
 magic=p.ReadUnsignedFromMemory(base,4,e)
 data={'event':'UIApplicationMain','base':hex(base),'magic':hex(magic),'read_ok':e.Success()}
 if e.Success() and magic==0xfeedfacf:
  if event_bp:t.BreakpointDelete(event_bp)
  result=lldb.SBCommandReturnObject()
  t.GetDebugger().GetCommandInterpreter().HandleCommand('breakpoint set -H -a '+hex(base+0x4584),result)
  data['breakpoint_result']=result.GetOutput()
  if result.Succeeded():
   bp=t.GetBreakpointAtIndex(t.GetNumBreakpoints()-1);event_bp=bp.GetID();bp.SetScriptCallbackFunction('a19_watchdog_v77.event')
 print('APP_MAIN',json.dumps(data))
 with open(log,'a') as f:f.write(json.dumps(data)+'\n')
 return False
