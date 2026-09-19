import lldb,json,uuid,time
from pathlib import Path
def observe(frame,bp_loc,internal_dict):
 p=frame.GetThread().GetProcess();e=lldb.SBError();h=p.ReadMemory(0x102da0000,0x2000,e)
 if e.Fail() or uuid.UUID('19A87853-DD17-33B3-99DB-5C8AB6F0EC48').bytes not in h:return False
 pc=frame.FindRegister('pc').GetValueAsUnsigned()
 kind='public_bundle' if pc==0x190fcf274 else 'initialized_asset_manager'
 reg='x0' if kind=='public_bundle' else 'x19'
 value=frame.FindRegister(reg).GetValueAsUnsigned()
 row={'kind':kind,'pc':hex(pc),'value':hex(value),'calculator_base':'0x102da0000','unix_time':time.time()}
 with Path('/tmp/a19-calc-assets-v82.jsonl').open('a') as f:f.write(json.dumps(row)+'\n')
 print('CALCULATOR_ASSET',json.dumps(row));bp_loc.GetBreakpoint().SetEnabled(False)
 return False
