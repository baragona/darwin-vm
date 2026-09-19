import json,time
count=0
def skip(frame,bp_loc,internal_dict):
 global count
 count+=1
 pc=frame.FindRegister('pc').GetValueAsUnsigned();lr=frame.FindRegister('lr').GetValueAsUnsigned()
 ok=frame.FindRegister('pc').SetValueFromCString(hex(lr))
 row={'event':'skip_absent_sleep_service_retry','entry':hex(pc),'return':hex(lr),'count':count,'ok':ok,'host_monotonic':time.monotonic()}
 with open('/tmp/a19-sleep-retry-v81.jsonl','a') as f:f.write(json.dumps(row)+'\n')
 if count<6:print('SLEEP_RETRY_SKIP',row)
 return not ok
