/* Diagnostic client: Golem computes; C installs/calls and displays readback. */
#define main unused_runner_main
#include "/Agon/mystuff/golem/docs/tasks/GOLEM-051/port/runner.c"
#undef main
#include "fixture.h"
static uint8_t state[STATE_BYTES],expected[STATE_BYTES],observed[OBSERVATIONS],array[ARRAY_BYTES];
static unsigned cursor,errors;
static int setbuffer(unsigned id,const uint8_t*data,unsigned bytes){
 command(id,2);
 while(bytes){unsigned n=bytes>96?96:bytes;uint8_t h[8]={23,0,160,0,0,0,0,0};word(h+3,id);word(h+6,n);
  send(h,8);send(data,n);if(!echo())return 0;data+=n;bytes-=n;
 }return 1;
}
static unsigned observe(unsigned id,unsigned offset,uint8_t want){
 uint8_t h[8]={23,0,160,0,0,0,4,0},q[4]={23,0,128,0};q[3]=(uint8_t)~want;
 command(0,2);send(h,8);send(q,4);
 uint8_t patch[15]={23,0,160,0,0,5,0xe2,3,0,1,0,0,0,0,0};word(patch+11,id);word(patch+13,offset);send(patch,15);
 volatile uint8_t*gp=mos_sysvars()+sysvar_gp;*gp=(uint8_t)~want;command(0,1);
 uint32_t start=getsysvar_time();while(*gp!=want&&(uint32_t)(getsysvar_time()-start)<600ul){}
 unsigned value=*gp;if(value!=want)++errors;return value;
}
static int package(const char*directory,const char*cases,unsigned id,const unsigned*entries){
 char file[128];snprintf(file,sizeof file,"%s/common-install.bin",directory);if(!replay(file))return 0;
 snprintf(file,sizeof file,"%s/buffered-install.bin",directory);if(!replay(file))return 0;
 FILE*f=fopen(cases,"rb");if(!f)return 0;
 for(unsigned c=0;c<CASES;++c){uint8_t e[2];
  if(fread(e,1,2,f)!=2||fread(state,1,STATE_BYTES,f)!=STATE_BYTES||fread(array,1,ARRAY_BYTES,f)!=ARRAY_BYTES||
     fread(expected,1,STATE_BYTES,f)!=STATE_BYTES){fclose(f);return 0;}
  if(!setbuffer(id,state,STATE_BYTES)||!setbuffer(id==STATE_DEFAULT?ARRAY_DEFAULT:ARRAY_RELOCATED,array,ARRAY_BYTES)){fclose(f);return 0;}
  command(entries[getword(e)],1);if(!echo()){fclose(f);return 0;}
  unsigned start=cursor;
  for(unsigned i=0;i<OFFSET_COUNT;++i){unsigned off=offsets[i];observed[cursor++]=(uint8_t)observe(id,off,expected[off]);}
  if(c==6){
   unsigned values[4]={0,0,0,0};
   for(unsigned i=0;i<OFFSET_COUNT;++i)for(unsigned j=0;j<4;++j)
    if(offsets[i]==display_offsets[j])values[j]=observed[start+i]|(unsigned)observed[start+i+1]<<8;
   printf("uint16: 5 & 3 = %u; 5 | 3 = %u; 5 ^ 3 = %u; ~5 = %u\n",values[0],values[1],values[2],values[3]);
  }
 }
 fclose(f);
 snprintf(file,sizeof file,"%s/buffered-cleanup.bin",directory);if(!replay(file))return 0;
 snprintf(file,sizeof file,"%s/common-install.bin",directory);if(!common_records(file,1)||!echo())return 0;
 command(0,2);return echo();
}
int main(void){
 puts("Golem bitwise operator review");
 puts("Source: review.golem, Sub compute (stock buffered commands)");
 puts("C test client supplies inputs and displays diagnostic readback.");
 puts("Checking all 4 integer types, signed boundaries, precedence and failures.");
 if(!package("default","default.dat",STATE_DEFAULT,entries_default)||
    !package("relocated","relocated.dat",STATE_RELOCATED,entries_relocated))return 2;
 FILE*f=fopen("results.dat","wb");if(!f)return 3;fwrite(observed,1,cursor,f);fclose(f);
 printf("%s: %u byte observations across %u calls.\n",errors?"FAIL":"PASS",cursor,CASES*2);
 puts("Type dir at the MOS prompt to check the keyboard still works.");
 f=fopen("done.dat","wb");if(!f)return 3;uint8_t result=(uint8_t)(errors!=0);fwrite(&result,1,1,f);fclose(f);
 return errors!=0;
}
