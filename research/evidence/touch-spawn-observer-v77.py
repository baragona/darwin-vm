import lldb,json
path='/tmp/a19-spawn-trace-v77.jsonl'
def trace(frame,bp_loc,internal_dict):
 p=frame.GetThread().GetProcess();e=lldb.SBError();mask=0x00ffffffffffffff
 def reg(n):return frame.FindRegister(n).GetValueAsUnsigned()
 name=p.ReadCStringFromMemory(reg('x1')&mask,512,e)
 if e.Fail() or name!='/Applications/TouchProbe.app/TouchProbe':return False
 def read(a,n):
  b=p.ReadMemory(a,n,e)
  if e.Fail():raise ValueError(str(e))
  return b
 def u(a):return int.from_bytes(read(a,8),'little')&mask
 r={'path':name,'pc':hex(frame.GetPC()),'lr':hex(reg('lr')),'attr_pointer':hex(reg('x3'))}
 try:
  a=u(reg('x3')&mask);r['attr']=hex(a);r['attr_bytes']=read(a,256).hex();persona=u(a+0xe0);r['persona_pointer']=hex(persona)
  if persona:r['persona_bytes']=read(persona,0x58).hex()
 except ValueError as ex:r['read_error']=str(ex)
 print('APP_SPAWN',json.dumps(r))
 with open(path,'a') as f:f.write(json.dumps(r)+'\n')
 return True
