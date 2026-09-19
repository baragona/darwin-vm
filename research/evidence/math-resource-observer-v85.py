import lldb,json,time
from pathlib import Path
UUID=bytes.fromhex('19a87853dd1733b399db5c8ab6f0ec48')
points={0x28ee8a660+0x1cd0000:('default_environment_data','x0'),0x28ee8a684+0x1cd0000:('parsed_environment_config','x0'),0x28ee8a6c0+0x1cd0000:('created_environment','x21')}
counts={}
def capture(frame,bp_loc,internal_dict):
 pc=frame.FindRegister('pc').GetValueAsUnsigned()
 if pc not in points:return False
 label,reg=points[pc]
 if counts.get(label,0)>=3:return False
 path=Path('/tmp/a19-calculator-main-v85.json')
 if not path.exists():return False
 base=int(json.loads(path.read_text())['base'],16)
 p=frame.GetThread().GetProcess();e=lldb.SBError();h=p.ReadMemory(base,0x2000,e)
 if e.Fail() or h[:4]!=b'\xcf\xfa\xed\xfe' or UUID not in h:return False
 counts[label]=counts.get(label,0)+1
 value=frame.FindRegister(reg).GetValueAsUnsigned()
 row={'event':label,'value':hex(value),'nonnull':value!=0,'calculator_base':hex(base),'time':time.monotonic()}
 with Path('/tmp/a19-math-resources-v85.jsonl').open('a') as f:f.write(json.dumps(row)+'\n')
 print('MATH_RESOURCE',row);return False
