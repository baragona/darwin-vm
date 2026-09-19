import json
count=0
def unlocked(frame,bp_loc,internal_dict):
 global count
 count+=1
 lr=frame.FindRegister('lr').GetValueAsUnsigned()
 ok=frame.FindRegister('x0').SetValueFromCString('0') and frame.FindRegister('pc').SetValueFromCString(hex(lr))
 row={'count':count,'method':'UIKeyboardInputModeController deviceStateIsLocked','result':False,'lr':hex(lr),'success':ok}
 with open('/tmp/a19-keyboard-lock-v81.jsonl','a') as f:f.write(json.dumps(row)+'\n')
 if count<=5:print('KEYBOARD_UNLOCKED',json.dumps(row))
 return not ok
