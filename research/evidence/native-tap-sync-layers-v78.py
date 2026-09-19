import json
from pathlib import Path
def synchronous(frame, bp_loc, internal_dict):
    r=frame.FindRegister('x2')
    before=r.GetValueAsUnsigned()
    ok=r.SetValueFromCString('0')
    row={'method':'CALayerHost setRendersAsynchronously:', 'requested':before, 'applied':r.GetValueAsUnsigned(), 'write_success':ok, 'caller':frame.FindRegister('lr').GetValue()}
    with Path('/tmp/a19-sync-layers-v78.jsonl').open('a') as f: f.write(json.dumps(row)+'\n')
    print('SYNC_LAYER',json.dumps(row))
    return not ok
