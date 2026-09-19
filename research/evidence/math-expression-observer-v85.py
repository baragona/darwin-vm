import lldb,json,time
from pathlib import Path
UUID=bytes.fromhex('19a87853dd1733b399db5c8ab6f0ec48')
points={0x28ee8a660+0x1cd0000:('default_environment_data','x0'),0x28ee8a684+0x1cd0000:('parsed_environment_config','x0'),0x28ee8a6c0+0x1cd0000:('created_environment','x21')}
points.update({0x20d298aa4+0x1cd0000:('typeset_image_entry','x0'),0x20d298bf8+0x1cd0000:('typeset_cgimage','x0'),0x20d245c8c+0x1cd0000:('typeset_view_body','x0')})
points.update({0x20d239370+0x1cd0000:('expression_body','x20'),0x20d2395a4+0x1cd0000:('expression_fit_return','x0'),0x20d22e860+0x1cd0000:('expression_format','x20')})
points.update({0x20eefeb00:('formatted_text','x19'),0x20eefe800:('fitted_text','x25')})
points.update({0x1844ad198+0x1cd0000:('layer_mask','x2')})
counts={}
def capture(frame,bp_loc,internal_dict):
 pc=frame.FindRegister('pc').GetValueAsUnsigned()
 if pc not in points:return False
 label,reg=points[pc]
 if counts.get(label,0)>=(20 if label=='layer_mask' else 3):return False
 path=Path('/tmp/a19-calculator-main-v85.json')
 if not path.exists():return False
 base=int(json.loads(path.read_text())['base'],16)
 p=frame.GetThread().GetProcess();e=lldb.SBError();h=p.ReadMemory(base,0x2000,e)
 if e.Fail() or h[:4]!=b'\xcf\xfa\xed\xfe' or UUID not in h:return False
 counts[label]=counts.get(label,0)+1
 value=frame.FindRegister(reg).GetValueAsUnsigned()
 row={'event':label,'value':hex(value),'nonnull':value!=0,'calculator_base':hex(base),'time':time.monotonic(),'registers':{r:frame.FindRegister(r).GetValue() for r in ['x0','x1','x2','x3','x20','lr','d0','d1']}}
 row['object_bytes']=(p.ReadMemory(value,128,e) or b'').hex();row['memory_error']=str(e)
 if label=='formatted_text':
  row['children']={}
  for off in [8,16]:
   ptr=p.ReadUnsignedFromMemory(value+off,8,e)
   row['children'][str(off)]={'address':hex(ptr),'bytes':(p.ReadMemory(ptr,256,e) or b'').hex(),'error':str(e)}
 if label=='formatted_text':
  sptr=p.ReadUnsignedFromMemory(value+8,8,e)
  buf=p.ReadUnsignedFromMemory(sptr+16,8,e);n=p.ReadUnsignedFromMemory(sptr+24,8,e)
  row['string_buffer']={'address':hex(buf),'length':n,'bytes':(p.ReadMemory(buf,min(n*2,256),e) or b'').hex(),'error':str(e)}
 with Path('/tmp/a19-math-resources-v85.jsonl').open('a') as f:f.write(json.dumps(row)+'\n')
 print('MATH_RESOURCE',row);return False
