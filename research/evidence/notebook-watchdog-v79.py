import lldb,struct,json
log='/tmp/a19-watchdog-v79.jsonl'
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
