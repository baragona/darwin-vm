// Exercise the actual browser handlers without a guest or host HID access.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const source = fs.readFileSync(__dirname + '/live-view.py', 'utf8').split('<script>')[1].split('</script>')[0];
function setup(version) {
  const calls=[], timers=new Map(),listeners={}; let next=0,clock=0;
  const screen={focus(){},setPointerCapture(){},addEventListener(type,fn){listeners[type]=fn},getBoundingClientRect(){return {left:0,top:0,width:100,height:100}}};
  const elements={'#screen':screen,'#status':{},'#home':{}};
  const context=vm.createContext({document:{querySelector:id=>elements[id]},window:{},
    performance:{now:()=>clock},
    setTimeout(fn,ms){timers.set(++next,{fn,ms});return next},clearTimeout(id){timers.delete(id)},
    async fetch(url,options){if(url==='/input'){calls.push(...JSON.parse(options.body));return {ok:true}}
      return {async json(){return {version,status:'Connected',frame:0,age:null,rejected:0}}}}});
  vm.runInContext(source,context);
  vm.runInContext('version='+version,context);
  const event=(x=20,y=30)=>({button:0,pointerId:1,clientX:x,clientY:y,preventDefault(){}});
  return {screen,calls,timers,event,context,listeners,advance:ms=>{clock+=ms}};
}
const settle=()=>new Promise(resolve=>setImmediate(resolve));
(async()=>{
  let s=setup(3);s.screen.onpointerdown(s.event());assert.deepEqual(s.calls,[]);
  s.screen.onpointerup(s.event());await settle();assert.deepEqual(s.calls,['T 0.20000 0.30000']);
  s=setup(3);s.screen.onpointerdown(s.event());s.screen.onpointermove(s.event(40,30));
  s.screen.onpointerup(s.event(50,30));await settle();
  assert.deepEqual(s.calls,['D 0.20000 0.30000','M 0.40000 0.30000','U 0.50000 0.30000']);
  s=setup(3);s.screen.onpointerdown(s.event());
  [...s.timers.values()].find(t=>t.ms===200).fn();s.screen.onpointerup(s.event());await settle();
  assert.deepEqual(s.calls,['D 0.20000 0.30000','U 0.20000 0.30000']);
  for(const cancel of ['onpointercancel','onlostpointercapture']){
    s=setup(3);s.screen.onpointerdown(s.event());s.screen[cancel]();s.screen.onpointerup(s.event());await settle();
    assert.deepEqual(s.calls,['R']);assert(![...s.timers.values()].some(t=>t.ms===200));
  }
  s=setup(2);s.screen.onpointerdown(s.event());s.screen.onpointerup(s.event());await settle();
  assert.deepEqual(s.calls,['D 0.20000 0.30000','U 0.20000 0.30000']);
  s=setup(4);const wheel=(x,y,shift=false)=>({deltaX:x,deltaY:y,shiftKey:shift,preventDefault(){}});
  s.listeners.wheel(wheel(100,0));s.listeners.wheel(wheel(100,0));await settle();
  assert.deepEqual(s.calls,['S 0.8 0.5 0.2 0.5']);
  s.advance(700);s.listeners.wheel(wheel(0,100));await settle();
  assert.equal(s.calls.at(-1),'S 0.5 0.8 0.5 0.2');
  s.advance(700);s.listeners.wheel(wheel(0,-100,true));await settle();
  assert.equal(s.calls.at(-1),'S 0.2 0.5 0.8 0.5');
  s.advance(700);s.listeners.wheel(wheel(100,0,true));await settle();
  assert.equal(s.calls.at(-1),'S 0.8 0.5 0.2 0.5');
  s=setup(3);s.listeners.wheel(wheel(100,0));await settle();assert.deepEqual(s.calls,[]);
  s=setup(4);s.screen.onpointerdown(s.event());s.listeners.wheel(wheel(0,100));s.screen.onpointercancel();await settle();assert.deepEqual(s.calls,['R']);
  console.log('PASS: taps, drags, holds, cancellation, wheel direction/throttling, and legacy guests');
})().catch(error=>{console.error(error);process.exitCode=1});
