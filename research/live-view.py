#!/usr/bin/env python3
"""Local-only guest viewer. Requires exclusive UART ownership and the live agent."""
import argparse, base64, binascii, collections, http.server, json, re, secrets
import socket, struct, threading, time, zlib

MAX_RAW = 16 * 1024 * 1024

def png(raw, width, height):
    rgba = bytearray(raw)
    rgba[0::4], rgba[2::4] = raw[2::4], raw[0::4]
    stride = width * 4
    rows = b''.join(b'\0' + rgba[y:y+stride] for y in range(0, len(raw), stride))
    def chunk(kind, data):
        return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind+data))
    return b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', width,height,8,6,0,0,0)) + chunk(b'IDAT',zlib.compress(rows,1)) + chunk(b'IEND',b'')

class Frames:
    def __init__(self):
        self.current = None
        self.rejected = 0
    def line(self, line):
        if line.startswith(b'LIVE_FRAME_BEGIN '):
            self.current = None
            try:
                _, seq, w, h, raw, packed, crc = line.split()
                seq,w,h,raw,packed = map(int,(seq,w,h,raw,packed))
                if not (0<w<=4096 and 0<h<=4096 and raw==w*h*4<=MAX_RAW and 0<packed<=MAX_RAW+65536):
                    raise ValueError('frame bounds')
                self.current = [seq,w,h,raw,packed,int(crc,16),bytearray()]
            except ValueError:
                self.rejected += 1
        elif line.startswith(b'LIVE_FRAME_DATA ') and self.current:
            try:
                _,seq,offset,encoded = line.split()
                c=self.current
                data=base64.b64decode(encoded,validate=True)
                if int(seq)!=c[0] or int(offset)!=len(c[6]) or not data or len(data)>768 or len(c[6])+len(data)>c[4]:
                    raise ValueError('frame sequence')
                c[6].extend(data)
            except (ValueError,binascii.Error):
                self.current=None;self.rejected+=1
        elif line.startswith(b'LIVE_FRAME_END ') and self.current:
            c=self.current;self.current=None
            try:
                if int(line.split()[1])!=c[0] or len(c[6])!=c[4] or zlib.crc32(c[6])!=c[5]:
                    raise ValueError('frame checksum')
                d=zlib.decompressobj();raw=d.decompress(c[6],c[3]+1)
                if len(raw)!=c[3] or not d.eof or d.unused_data or d.unconsumed_tail:
                    raise ValueError('frame size')
                return c[0],png(raw,c[1],c[2])
            except (ValueError,zlib.error,IndexError):
                self.rejected+=1
        return None

COMMAND = re.compile(r'(?:[DMU] (?:0(?:\.\d+)?|1(?:\.0+)?) (?:0(?:\.\d+)?|1(?:\.0+)?)|K \d{1,3} [01]|[HRP])\Z')
class Bridge:
    def __init__(self, path, transcript, start, rate):
        self.socket=socket.socket(socket.AF_UNIX);self.socket.connect(path)
        self.log=open(transcript,'xb');self.rate=rate
        self.lock=threading.Lock();self.queue=collections.deque();self.ack=threading.Event();self.ready=threading.Event()
        self.parser=Frames();self.picture=None;self.frame=0;self.frame_at=0;self.status='Waiting for guest agent';self.stopped=False
        self.reader_thread=threading.Thread(target=self.reader,daemon=True);self.reader_thread.start()
        if start:self.send("/bin/stty -echo; /bin/live-input-agent; /bin/stty echo\n",rate=25)
        threading.Thread(target=self.writer,daemon=True).start()
    def send(self, text, rate=None):
        for b in text.encode():
            self.socket.sendall(bytes([b]));time.sleep(1/(rate or self.rate))
    def reader(self):
        buf=bytearray()
        try:
            while True:
                data=self.socket.recv(65536)
                if not data:raise EOFError('Guest UART closed')
                self.log.write(data);self.log.flush();buf.extend(data)
                while b'\n' in buf:
                    line,_,rest=buf.partition(b'\n');buf=bytearray(rest);line=bytes(line).replace(b'\r',b'')
                    if line==b'LIVE_INPUT_READY 1':self.ready.set();self.status='Connected'
                    elif line.startswith(b'LIVE_INPUT_RESULT '):
                        self.status='Connected' if line==b'LIVE_INPUT_RESULT 1' else 'Guest rejected a command'
                        self.ack.set()
                    elif line==b'LIVE_INPUT_CLOSED':raise EOFError('Guest agent exited')
                    result=self.parser.line(line)
                    if result:
                        with self.lock:self.frame,self.picture=result;self.frame_at=time.monotonic()
                if len(buf)>8192:buf.clear()
        except (OSError,EOFError) as e:self.status=str(e);self.stopped=True;self.ack.set()
    def writer(self):
        last_frame=0;touch_active=False;held_keys=set();last_input=0
        while not self.ready.wait(.25):
            if self.stopped:return
        while not self.stopped:
            with self.lock:cmd=self.queue.popleft() if self.queue else None
            if cmd is None and (touch_active or held_keys) and time.monotonic()-last_input>=5:
                cmd='R'
            if cmd is None and not touch_active and not held_keys and time.monotonic()-last_frame>=1:
                cmd='F';last_frame=time.monotonic()
            if cmd is None:time.sleep(.01);continue
            if cmd=='R':touch_active=False;held_keys.clear()
            elif cmd.startswith('D '):touch_active=True
            elif cmd.startswith('U '):touch_active=False
            elif cmd.startswith('K '):
                _,usage,down=cmd.split()
                if down=='1':held_keys.add(usage)
                else:held_keys.discard(usage)
            if cmd!='F':last_input=time.monotonic()
            self.ack.clear()
            try:self.send(cmd+'\n')
            except OSError as e:self.status=str(e);self.stopped=True;return
            if not self.ack.wait(120):
                self.status='Guest acknowledgement timed out; input paused';self.stopped=True;return
    def close(self):
        self.stopped=True;self.ack.set()
        try:self.socket.shutdown(socket.SHUT_RDWR)
        except OSError:pass
        self.socket.close();self.reader_thread.join(timeout=2)
        if not self.reader_thread.is_alive():self.log.close()
    def enqueue(self, commands):
        if not self.ready.is_set() or self.stopped:raise ValueError('Guest is not ready')
        if not isinstance(commands,list) or not 1<=len(commands)<=32:raise ValueError('Invalid command batch')
        for cmd in commands:
            if not isinstance(cmd,str) or len(cmd)>80 or not COMMAND.fullmatch(cmd):raise ValueError('Invalid input command')
            if cmd.startswith('K ') and not 4<=int(cmd.split()[1])<=231:raise ValueError('Invalid key usage')
        with self.lock:
            if len(self.queue)+len(commands)>128:
                self.queue.clear();self.queue.append('R');raise ValueError('Input queue full; releasing input')
            for cmd in commands:
                if cmd.startswith('M ') and self.queue and self.queue[-1].startswith('M '):self.queue[-1]=cmd
                else:self.queue.append(cmd)

HTML='''<!doctype html><meta charset="utf-8"><title>iOS guest</title>
<style>body{background:#17191d;color:#eee;font:15px system-ui;margin:16px}header{display:flex;gap:16px;align-items:center;margin-bottom:12px}button{padding:8px 16px}img{max-height:86vh;max-width:95vw;touch-action:none;user-select:none;background:#000}#status{color:#bbb}</style>
<header><strong>iOS guest</strong><button id="home">Home</button><span id="status">Connecting…</span></header><img id="screen" draggable="false" tabindex="0" alt="Waiting for a guest frame">
<script>
const screen=document.querySelector('#screen'),status=document.querySelector('#status');let down=false,lastFrame=-1,outbox=[],sending=false;
function send(cmds){
  for(const cmd of cmds){if(cmd.startsWith('M ')&&outbox.length&&outbox[outbox.length-1].startsWith('M '))outbox[outbox.length-1]=cmd;else outbox.push(cmd)}
  if(outbox.length>128){outbox=['R'];down=false;status.textContent='Input queue full; releasing input'}
  drain();
}
async function drain(){
  if(sending)return;sending=true;
  try{while(outbox.length){let cmds=outbox.splice(0,32);let r=await fetch('/input',{method:'POST',headers:{'Content-Type':'application/json','X-VM-Token':'TOKEN'},body:JSON.stringify(cmds)});if(!r.ok)throw Error('Input unavailable')}}
  catch(e){outbox=[];down=false;status.textContent=e.message}
  finally{sending=false}
}
function point(e){let r=screen.getBoundingClientRect();return [Math.max(0,Math.min(1,(e.clientX-r.left)/r.width)),Math.max(0,Math.min(1,(e.clientY-r.top)/r.height))].map(n=>n.toFixed(5)).join(' ')}
screen.onpointerdown=e=>{if(e.button!==0)return;e.preventDefault();screen.focus();screen.setPointerCapture(e.pointerId);down=true;send(['D '+point(e)])};
screen.onpointermove=e=>{if(down)send(['M '+point(e)])};screen.onpointerup=e=>{if(down){down=false;send(['U '+point(e)])}};
screen.onpointercancel=()=>{down=false;send(['R'])};screen.oncontextmenu=e=>e.preventDefault();
document.querySelector('#home').onclick=()=>send(['H']);window.onblur=()=>{down=false;send(['R'])};
const keys={Enter:40,Escape:41,Backspace:42,Tab:43,Space:44,Minus:45,Equal:46,BracketLeft:47,BracketRight:48,Backslash:49,Semicolon:51,Quote:52,Backquote:53,Comma:54,Period:55,Slash:56,CapsLock:57,Delete:76,ArrowRight:79,ArrowLeft:80,ArrowDown:81,ArrowUp:82,ControlLeft:224,ShiftLeft:225,AltLeft:226,MetaLeft:227,ControlRight:228,ShiftRight:229,AltRight:230,MetaRight:231};
for(let i=0;i<26;i++)keys['Key'+String.fromCharCode(65+i)]=4+i;for(let i=1;i<=9;i++)keys['Digit'+i]=29+i;keys.Digit0=39;
for(const type of ['keydown','keyup'])screen.addEventListener(type,e=>{if(keys[e.code]){e.preventDefault();if(!e.repeat)send(['K '+keys[e.code]+' '+(type==='keydown'?1:0)])}});
async function poll(){try{let s=await(await fetch('/state')).json();status.textContent=s.status+' · '+(s.age===null?'no frame':s.age.toFixed(1)+'s since frame')+' · '+s.rejected+' rejected frames';if(s.frame!==lastFrame&&s.frame){lastFrame=s.frame;screen.src='/frame?'+s.frame}}catch(e){status.textContent=e.message}setTimeout(poll,500)}poll();
</script>'''

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--serial',required=True);p.add_argument('--transcript',required=True)
    p.add_argument('--port',type=int,default=8989);p.add_argument('--start',action='store_true',help='launch agent from an idle guest shell')
    p.add_argument('--rate',type=int,default=200,help='UART transmit bytes per second');a=p.parse_args()
    if not 1<=a.rate<=10000:p.error('rate must be 1..10000')
    bridge=Bridge(a.serial,a.transcript,a.start,a.rate);token=secrets.token_hex(16)
    class Handler(http.server.BaseHTTPRequestHandler):
        def log_message(self,*args):pass
        def reply(self,code,data,kind):
            self.send_response(code);self.send_header('Content-Type',kind);self.send_header('Cache-Control','no-store');self.send_header('Content-Length',str(len(data)));self.end_headers();self.wfile.write(data)
        def do_GET(self):
            if self.path=='/':return self.reply(200,HTML.replace('TOKEN',token).encode(),'text/html; charset=utf-8')
            if self.path=='/state':
                with bridge.lock:data={'status':bridge.status,'frame':bridge.frame,'age':time.monotonic()-bridge.frame_at if bridge.frame_at else None,'rejected':bridge.parser.rejected}
                return self.reply(200,json.dumps(data).encode(),'application/json')
            if self.path.startswith('/frame'):
                with bridge.lock:picture=bridge.picture
                return self.reply(200 if picture else 503,picture or b'No frame','image/png' if picture else 'text/plain')
            self.reply(404,b'Not found','text/plain')
        def do_POST(self):
            if self.path!='/input' or self.headers.get('X-VM-Token')!=token:return self.reply(403,b'Forbidden','text/plain')
            try:
                length=int(self.headers.get('Content-Length','0'))
                if not 0<length<=4096:raise ValueError('Invalid request size')
                bridge.enqueue(json.loads(self.rfile.read(length)));self.reply(200,b'{}','application/json')
            except (ValueError,TypeError):self.reply(400,b'Invalid or unavailable input','text/plain')
    try:server=http.server.ThreadingHTTPServer(('127.0.0.1',a.port),Handler)
    except OSError:bridge.close();raise
    print(f'Viewer: http://127.0.0.1:{a.port}',flush=True)
    try:server.serve_forever()
    finally:bridge.close();server.server_close()
if __name__=='__main__':main()
