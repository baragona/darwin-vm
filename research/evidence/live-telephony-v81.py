import json,time
count=0
def skip(frame,bp_loc,internal_dict):
 global count
 count+=1
 lr=frame.FindRegister('lr').GetValueAsUnsigned()
 ok=frame.FindRegister('pc').SetValueFromCString(hex(lr))
 row={'method':'SBTelephonyManager queue_setFastDormancySuspended:withConnection:','count':count,'lr':hex(lr),'success':ok,'host_monotonic':time.monotonic()}
 with open('/tmp/a19-telephony-v81.jsonl','a') as f:f.write(json.dumps(row)+'\n')
 if count<=5:print('TELEPHONY_SKIP',json.dumps(row))
 return not ok
