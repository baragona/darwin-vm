import json
count=0
def skip(frame,bp_loc,internal_dict):
 global count
 count+=1
 pc=frame.FindRegister('pc');entry=pc.GetValueAsUnsigned();lr=frame.FindRegister('lr').GetValueAsUnsigned()
 ok=pc.SetValueFromCString(hex(lr))
 row={'count':count,'pc':hex(entry),'method':{0x2274f14dc:'SBBacklightIdleTimer _reconfigureAttentionClientAndReset:',0x2274effe0:'SBIdleTimerGlobalCoordinator _setIdleTimerWithDescriptor:forReason:'}.get(entry,'unknown'),'lr':hex(lr),'write_success':ok}
 with open('/tmp/a19-attention-v77.jsonl','a') as f:f.write(json.dumps(row)+'\n')
 print('ATTENTION_SKIP',json.dumps(row))
 return not ok
def unavailable(frame,bp_loc,internal_dict):
 pc=frame.FindRegister('pc');entry=pc.GetValueAsUnsigned();lr=frame.FindRegister('lr').GetValueAsUnsigned()
 ok=frame.FindRegister('x0').SetValueFromCString('0') and pc.SetValueFromCString(hex(lr))
 row={'method':'AWAttentionAwarenessClient resumeWithError:','pc':hex(entry),'result':False,'lr':hex(lr),'write_success':ok}
 print('ATTENTION_UNAVAILABLE',json.dumps(row))
 with open('/tmp/a19-attention-v77.jsonl','a') as f:f.write(json.dumps(row)+'\n')
 return not ok
