"""Bounded read-only observation; no event or app state changes."""
import json
counts={}
methods={0x18775f6b8:'UIApplication sendEvent:',0x187265140:'UIWindow sendEvent:',0x2277d9b70:'CoverSheet dismiss gesture began',0x2274e9c9c:'SBLockScreenManager unlockUIFromSource:withOptions:'}
def sample(frame,bp_loc,internal_dict):
 def r(n):return frame.FindRegister(n).GetValueAsUnsigned()
 pc=r('pc');n=counts.get(pc,0)+1;counts[pc]=n
 row={'pc':hex(pc),'method':methods.get(pc,'unknown'),'count':n,'registers':{k:hex(r(k)) for k in ['x0','x1','x2','x3','lr']}}
 if n>=96:
  bp_loc.GetBreakpoint().SetEnabled(False);row['observer_disabled_at_limit']=True
 with open('/tmp/a19-ui-delivery-v77.jsonl','a') as f:f.write(json.dumps(row)+'\n')
 return False
