import lldb

def fix(frame,bp_loc,internal_dict):
    p=frame.GetThread().GetProcess();e=lldb.SBError();mask=0x00ffffffffffffff
    path=p.ReadCStringFromMemory(frame.FindRegister('x20').GetValueAsUnsigned()&mask,512,e)
    if e.Fail() or path!='/System/Library/Frameworks/ExtensionFoundation.framework/XPCServices/extensionkitservice.xpc': return False
    b=frame.FindRegister('x23').GetValueAsUnsigned()&mask
    uid=p.ReadUnsignedFromMemory(b+0x10,4,e)
    if e.Fail(): return True
    mode=p.ReadUnsignedFromMemory(b+4,2,e)
    print('EXTENSION_STAT',path,hex(b),'uid',uid,'mode',oct(mode),'result',frame.FindRegister('x0').GetValue())
    if e.Success() and uid==0:return False
    if e.Fail() or uid!=99 or mode!=0o40755 or frame.FindRegister('x0').GetValueAsUnsigned()!=1: return True
    n=p.WriteMemory(b+0x10,bytes(4),e)
    print('EXTENSION_STAT_UID_WRITE',n,str(e))
    return e.Fail() or n!=4
