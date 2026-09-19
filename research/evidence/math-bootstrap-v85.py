import lldb,json,uuid
from pathlib import Path
base=0x100334000
mobile=None
expected=str(uuid.uuid5(uuid.UUID('09b988ac-d2d5-453c-bfa9-08e5c7235427'),'mobile-user')).upper().encode()
def persona(frame,bp_loc,internal_dict):
 global mobile
 p=frame.GetThread().GetProcess();e=lldb.SBError();mask=0x00ffffffffffffff
 def u(a):
  v=p.ReadUnsignedFromMemory(a,8,e)
  if e.Fail():raise ValueError(str(e))
  return v&mask
 try:
  if mobile is None:
   magic=p.ReadUnsignedFromMemory(base,4,e)
   if e.Fail() or magic!=0xfeedfacf:raise ValueError('UserManager Mach-O mismatch')
   manager=u(base+0xe7398);state=u(manager+0x20);dictionary=u(state+8);storage=u(dictionary+8)
   for i in range(16):
    v=u(storage+i*8)
    if v<0x100000000:continue
    data=p.ReadMemory(v+0x20,36,e)
    if e.Success() and data==expected:mobile=v;break
   if mobile is None:raise ValueError('Seeded mobile NSString not found')
   row={'base':hex(base),'manager':hex(manager),'mobile_nsstring':hex(mobile),'uuid':expected.decode()}
   Path('/tmp/a19-persona-resolved-v85.json').write_text(json.dumps(row,indent=2)+'\n');print('PERSONA_RESOLVED',row)
  pc=frame.FindRegister('pc').GetValueAsUnsigned();name={base+0x506c:'x0',base+0x370c:'x1'}[pc]
  return not frame.FindRegister(name).SetValueFromCString(hex(mobile))
 except Exception as exc:print('PERSONA_ERROR',str(exc));return True
def launchd(frame,bp_loc,internal_dict):
 p=frame.GetThread().GetProcess();t=p.GetTarget();e=lldb.SBError()
 candidate=(frame.FindRegister('lr').GetValueAsUnsigned()&0x7fffffffff)-0x1a21c
 if candidate<0x100000000:return False
 magic=p.ReadUnsignedFromMemory(candidate,4,e)
 if e.Fail() or magic!=0xfeedfacf:return False
 result=lldb.SBCommandReturnObject()
 t.GetDebugger().GetCommandInterpreter().HandleCommand('breakpoint set -H -a '+hex(candidate+0x29d0c),result)
 if not result.Succeeded():print('EXTENSION_BREAK_ERROR',result.GetError());return True
 bp=t.GetBreakpointAtIndex(t.GetNumBreakpoints()-1);bp.SetScriptCallbackFunction('a19_extension_stat_v85.fix')
 row={'launchd_base':hex(candidate),'stat_hook':hex(candidate+0x29d0c),'breakpoint':bp.GetID()}
 Path('/tmp/a19-launchd-resolved-v85.json').write_text(json.dumps(row,indent=2)+'\n');print('LAUNCHD_RESOLVED',row)
 bp_loc.GetBreakpoint().SetEnabled(False)
 return False
