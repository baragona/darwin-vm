# Convert this diagnostic probe's prebuilt swipe samples into a stationary touch.
# No app state, hit-testing result, callback, or captured pixels are modified.
import lldb,json
counts={}
def tap(frame,bp_loc,internal_dict):
 def reg(n):return frame.FindRegister(n)
 if reg('x0').GetValueAsUnsigned()!=0 or reg('x1').GetValueAsUnsigned()!=0:return False
 try:x=float(reg('d0').GetValue());y=float(reg('d1').GetValue())
 except (TypeError,ValueError):return False
 if abs(x-.5)>1e-8 or not .19<=y<=.97:return False
 bid=bp_loc.GetBreakpoint().GetID();n=counts.get(bid,0)+1
 if n>28:print('TAP_LIMIT',bid,n);return True
 ok=reg('d1').SetValueFromCString('0.55')
 after=float(reg('d1').GetValue());counts[bid]=n
 r={'breakpoint':bid,'sample':n,'x':x,'y_before':y,'y_after':after,'write_success':ok}
 print('TAP_SAMPLE',json.dumps(r))
 with open('/tmp/a19-tap-v77.jsonl','a') as f:f.write(json.dumps(r)+'\n')
 return not ok or abs(after-.55)>1e-8
