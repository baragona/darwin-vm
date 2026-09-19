import lldb,json
from pathlib import Path

def arm():
 t=lldb.debugger.GetSelectedTarget();ci=lldb.debugger.GetCommandInterpreter()
 base=int(json.loads(Path('/tmp/a19-calculator-main-v79.json').read_text())['base'],16)
 ids=[]
 for offset in json.loads(Path('/tmp/a19-calculator-trap-offsets.json').read_text()):
  r=lldb.SBCommandReturnObject();ci.HandleCommand('breakpoint set -H -a '+hex(base+offset),r)
  if not r.Succeeded():raise RuntimeError(r.GetError())
  ids.append(t.GetBreakpointAtIndex(t.GetNumBreakpoints()-1).GetID())
 Path('/tmp/a19-calculator-trap-bps-v79.json').write_text(json.dumps({'base':hex(base),'ids':ids}))
 print('CALCULATOR_TRAPS_ARMED',len(ids))
