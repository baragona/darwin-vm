# V78-only controlled syscall experiment, invoked while stopped at Touch Probe spawn.
import lldb,struct,json
saved=None
results=[]
def current():
 p=lldb.debugger.GetSelectedTarget().GetProcess()
 return p,p.GetSelectedThread().GetFrameAtIndex(0)
def read(p,a,n):
 e=lldb.SBError();b=p.ReadMemory(a,n,e)
 if e.Fail() or b is None or len(b)!=n:raise RuntimeError(str(e))
 return b
def write(p,a,b):
 e=lldb.SBError();n=p.WriteMemory(a,b,e)
 if e.Fail() or n!=len(b):raise RuntimeError(str(e))
def reg(f,n,v):
 if not f.FindRegister(n).SetValueFromCString(hex(v)):raise RuntimeError('register '+n)
def prepare():
 global saved
 assert saved is None
 p,f=current();assert f.FindRegister('pc').GetValueAsUnsigned()==0x24cb93eb0
 e=lldb.SBError();name=p.ReadCStringFromMemory(f.FindRegister('x1').GetValueAsUnsigned()&0x00ffffffffffffff,512,e)
 assert e.Success() and name=='/Applications/TouchProbe.app/TouchProbe'
 names=['x'+str(i) for i in range(29)]+['fp','lr','sp','pc','cpsr']
 values={n:f.FindRegister(n).GetValueAsUnsigned() for n in names}
 assert values['lr']==0x10437e1f4
 start=values['sp']-0x600
 memory=read(p,start,0x600)
 saved={'regs':values,'start':start,'memory':memory}
 # Exact arm64 persona-info ABI: 348 bytes, name at88 and UID at344.
 info=bytearray(348);struct.pack_into('<IIII',info,0,2,1003,2,501)
 nickname=b'darwinvm-501-0';info[88:88+len(nickname)]=nickname
 struct.pack_into('<I',info,344,501)
 saved['info']=start+0x100;saved['id']=start+0x280;saved['count']=start+0x288
 write(p,saved['info'],info);write(p,saved['id'],struct.pack('<I',1003));write(p,saved['count'],struct.pack('<Q',1))
 print('PERSONA_SYSCALL_PREPARED',hex(start),hex(values['sp']))
def call(op):
 p,f=current();assert saved is not None
 # Same six syscall arguments as the firmware's kpersona_alloc/info wrappers.
 for n,v in {'x0':op,'x1':0,'x2':saved['info'],'x3':saved['id'],'x4':saved['count'],'x5':0,'x16':0x1ee}.items():reg(f,n,v)
 reg(f,'pc',0x24cb8eeb8)
 print('PERSONA_SYSCALL_READY',op)
def report(label):
 p,f=current();assert f.FindRegister('pc').GetValueAsUnsigned()==0x24cb8eebc
 info=read(p,saved['info'],348)
 r={'step':label,'return':f.FindRegister('x0').GetValueAsUnsigned(),'cpsr':hex(f.FindRegister('cpsr').GetValueAsUnsigned()),'id':int.from_bytes(read(p,saved['id'],4),'little'),'info_version':int.from_bytes(info[:4],'little'),'info_id':int.from_bytes(info[4:8],'little'),'info_type':int.from_bytes(info[8:12],'little'),'info_uid':int.from_bytes(info[344:348],'little'),'info_name':info[88:344].split(b'\0')[0].decode()}
 results.append(r)
 with open('/tmp/a19-persona-syscall-v78.json','w') as out:json.dump(results,out,indent=2)
 print('PERSONA_SYSCALL_RESULT',json.dumps(r))
def restore():
 global saved
 p,f=current();assert saved is not None
 write(p,saved['start'],saved['memory'])
 assert read(p,saved['start'],len(saved['memory']))==saved['memory']
 for n,v in saved['regs'].items():reg(f,n,v)
 for n,v in saved['regs'].items():assert f.FindRegister(n).GetValueAsUnsigned()==v,n
 print('PERSONA_CALL_STATE_RESTORED',hex(f.FindRegister('pc').GetValueAsUnsigned()))
 saved=None

def touch_spawn(frame,bp_loc,internal_dict):
 p=frame.GetThread().GetProcess();e=lldb.SBError()
 name=p.ReadCStringFromMemory(frame.FindRegister('x1').GetValueAsUnsigned()&0x00ffffffffffffff,512,e)
 if e.Success() and name=='/Applications/TouchProbe.app/TouchProbe':
  print('TOUCH_SPAWN_READY');return True
 return False
