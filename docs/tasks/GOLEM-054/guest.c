/* Test client. Golem prints the numerical text; C supplies inputs and labels. */
#define main unused_runner_main
#include "/Agon/mystuff/golem/docs/tasks/GOLEM-051/port/runner.c"
#undef main
#include "fixture.h"
static uint8_t state[STATE_BYTES],expected[OFFSET_COUNT],observed[CASE_COUNT*OFFSET_COUNT];
static char text[128];static unsigned errors,cursor;
static int setbuffer(unsigned id,const uint8_t*p,unsigned bytes){
 command(id,2);while(bytes){unsigned n=bytes>96?96:bytes;uint8_t h[8]={23,0,160,0,0,0,0,0};word(h+3,id);word(h+6,n);
 send(h,8);send(p,n);if(!echo())return 0;p+=n;bytes-=n;}return 1;
}
static uint8_t observe(unsigned offset,uint8_t want){
 uint8_t h[8]={23,0,160,0,0,0,4,0},q[4]={23,0,128,0};q[3]=(uint8_t)~want;
 command(0,2);send(h,8);send(q,4);
 uint8_t patch[15]={23,0,160,0,0,5,0xe2,3,0,1,0,0,0,0,0};word(patch+11,STATE_ID);word(patch+13,offset);send(patch,15);
 volatile uint8_t*gp=mos_sysvars()+sysvar_gp;*gp=(uint8_t)~want;command(0,1);
 uint32_t start=getsysvar_time();while(*gp!=want&&(uint32_t)(getsysvar_time()-start)<600ul){}
 uint8_t actual=*gp;if(actual!=want)++errors;return actual;
}
static int scene(unsigned number){
 uint8_t value[2];word(value,number);FILE*f=fopen("scene.dat","wb");if(!f)return 0;fwrite(value,1,2,f);fclose(f);
 uint32_t start=getsysvar_time();
 for(;;){
  f=fopen("advance.dat","rb");if(f){unsigned n=fread(value,1,2,f);fclose(f);if(n==2&&getword(value)==number)return 1;}
  uint32_t tick=getsysvar_time();if(tick-start>14400ul)return 0;while(getsysvar_time()==tick){}
 }
}
static int install(void){return replay("package/common-install.bin")&&replay("package/buffered-install.bin");}
static int cleanup(void){return replay("package/buffered-cleanup.bin")&&common_records("package/common-install.bin",1)&&echo();}
int main(int argc,char**argv){
 int review=argc>1&&!strcmp(argv[1],"--review");
 int serial=argc>1&&!strcmp(argv[1],"--serial");
 if(!install())return 2;
 if(serial){const uint8_t printer=2;send(&printer,1);}
 const uint8_t cursor_off[]={23,1,0};send(cursor_off,sizeof cursor_off);
 if(review){
  const uint8_t clear=12;send(&clear,1);
  puts("Golem float32 printing on stock VDP");
  puts("Source: numbers.golem / Sub show. Golem formats every number.");
  puts("C only installs, supplies inputs, calls entries and separates rows.");
  puts("Nine significant digits; fixed/scientific notation; no VDPlet.");
 }
 FILE*f=fopen("cases.dat","rb");if(!f)return 3;
 for(unsigned c=0;c<CASE_COUNT;++c){
  uint8_t h[4];if(fread(h,1,4,f)!=4||fread(state,1,STATE_BYTES,f)!=STATE_BYTES||fread(expected,1,OFFSET_COUNT,f)!=OFFSET_COUNT){fclose(f);return 3;}
  unsigned length=h[3];if(length>=sizeof text||fread(text,1,length,f)!=length){fclose(f);return 3;}
  unsigned entry=getword(h),reset=h[2];
  if(serial)send("{#",2);
  if(!review&&!serial){const uint8_t clear[]={12};send(clear,1);if(length)send(text,length);if(!echo()||!drawing_complete())return 4;if(!scene(c*2))return 5;send(clear,1);}
  if(reset&&!setbuffer(STATE_ID,state,STATE_BYTES))return 4;
  command(entries[entry],1);if(!echo()||!drawing_complete())return 4;
  for(unsigned i=0;i<OFFSET_COUNT;++i)observed[cursor++]=observe(offsets[i],expected[i]);
  if(serial)send("#}\r\n",4);
  else if(review)send("\r\n",2);else if(!scene(c*2+1))return 5;
 }
 fclose(f);
 if(serial){const uint8_t printer=3;send(&printer,1);}
 if(!cleanup())return 4;
 f=fopen("results.dat","wb");if(!f)return 3;fwrite(observed,1,cursor,f);fclose(f);
 if(review){puts(errors?"FAIL":"PASS: numerical output and retained state checks complete.");puts("Type dir at the MOS prompt to check the keyboard still works.");}
 f=fopen("done.dat","wb");if(!f)return 3;uint8_t done=(uint8_t)(errors!=0);fwrite(&done,1,1,f);fclose(f);
 return errors!=0;
}
