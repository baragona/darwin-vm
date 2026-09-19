#define main guest_agent_main
#include "live-input-agent.c"
#undef main
#include <assert.h>
static int sent,fail;
uint64_t virtual_touch_start(void){return 1;}
int virtual_touch_dispatch(void *e){(void)e;sent++;return !fail;}
void virtual_touch_stop(void){}
static uint64_t tick(void){return 1;}
static void *h(void*a,uint64_t b,unsigned c,unsigned d,unsigned e,unsigned f,unsigned g,double i,double j,double k,double l,double m,unsigned n,unsigned o,unsigned p){(void)a;(void)b;(void)c;(void)d;(void)e;(void)f;(void)g;(void)i;(void)j;(void)k;(void)l;(void)m;(void)n;(void)o;(void)p;return (void*)1;}
static void *f(void*a,uint64_t b,unsigned c,unsigned d,unsigned e,double i,double j,double k,double l,double m,unsigned n,unsigned o,unsigned p){return h(a,b,c,d,e,0,0,i,j,k,l,m,n,o,p);}
static void *k(void*a,uint64_t b,unsigned c,unsigned d,unsigned char e,unsigned f){(void)a;(void)b;(void)c;(void)d;(void)e;(void)f;return (void*)1;}
static void rel(void*a){(void)a;}
static void integer_stub(void*a,unsigned b,long c){(void)a;(void)b;(void)c;}
static void sender_stub(void*a,uint64_t b){(void)a;(void)b;}
static void append_stub(void*a,void*b,unsigned c){(void)a;(void)b;(void)c;}
static void dispatch_stub(void*a,void*b){(void)a;(void)b;sent++;}
int main(void){
 hand=h;finger=f;keyboard=k;now=tick;release=rel;integer=integer_stub;sender=sender_stub;append=append_stub;dispatch=dispatch_stub;
 assert(!command("M .1 .2"));assert(!command("U .1 .2"));assert(sent==0);
 assert(command("D .1 .2"));assert(touching);assert(!command("D .1 .2"));
 fail=1;assert(!command("U .9 .8"));assert(touching&&last_x==.1);fail=0;
 assert(command("M .8 .9"));assert(command("U .8 .9"));assert(!touching);
 const char *bad[]={"D nan .5","D inf .5","D -1 .5","D 1.1 .5","D .1 .2 extra","K -4294967292 1","K 4294967300 1","K 99999999999999999999999999 1","K 4 2","K 4 01","K 3 1","K 232 1","K 4 1 junk","K +4 1",""};
 for(unsigned i=0;i<sizeof(bad)/sizeof(bad[0]);i++){int before=sent;assert(!command((char*)bad[i]));assert(sent==before);}
 assert(command("K 4 1"));assert(keys[4]);assert(command("K 225 1"));assert(keys[225]);assert(command("D .2 .3"));assert(command("R"));assert(!touching&&!keys[4]&&!keys[225]);
 puts("PASS: invalid commands dispatch nothing; failed release retains state; reset releases touch and keys");
}
