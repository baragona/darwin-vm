import lldb,json,uuid
from pathlib import Path

def inspect(frame,bp_loc,internal_dict):
 p=frame.GetThread().GetProcess();e=lldb.SBError();fp=frame.FindRegister('fp').GetValueAsUnsigned()&0x7fffffffff
 frames=[]
 for i in range(40):
  b=p.ReadMemory(fp,16,e)
  if e.Fail() or len(b)!=16:break
  nxt=int.from_bytes(b[:8],'little')&0x7fffffffff;lr=int.from_bytes(b[8:],'little')&0x7fffffffff
  frames.append(hex(lr));base=lr-0x1ffc
  if 0x100000000<=base<0x110000000 and base%0x4000==0:
   h=p.ReadMemory(base,0x2000,e)
   if e.Success() and h[:4]==bytes.fromhex('cffaedfe') and uuid.UUID('19A87853-DD17-33B3-99DB-5C8AB6F0EC48').bytes in h:
    row={'base':hex(base),'frames':frames,'uuid':'19A87853-DD17-33B3-99DB-5C8AB6F0EC48'}
    Path('/tmp/a19-calculator-main-v83.json').write_text(json.dumps(row,indent=2)+'\n');print('CALCULATOR_MAIN',row);return False
  if nxt<=fp or nxt-fp>0x100000:break
  fp=nxt
 return False
