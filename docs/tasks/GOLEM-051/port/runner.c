/* Standalone stock-VDP runner. MOS owns files and transport; VDP runs the code. */
#include <agon/mos.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint8_t tag=0x51;
#ifdef GOLEM_QUALIFY
extern void g_observe(void);
#define OBSERVE() g_observe()
#else
#define OBSERVE() ((void)0)
#endif
static void word(uint8_t*p,unsigned v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static unsigned getword(const uint8_t*p){return (unsigned)p[0]|(unsigned)p[1]<<8;}
static void send(const void*p,unsigned n){mos_puts((const char*)p,n,0);}
static void command(unsigned id,unsigned op){uint8_t b[6]={23,0,160,0,0,0};word(b+3,id);b[5]=(uint8_t)op;send(b,6);}
static int echo(void){
    volatile uint8_t*gp=mos_sysvars()+sysvar_gp;tag^=0xff;*gp=(uint8_t)~tag;
    uint8_t b[4]={23,0,128,tag};send(b,4);uint32_t start=getsysvar_time();
    while(*gp!=tag)if((uint32_t)(getsysvar_time()-start)>7200ul){puts("VDP timeout: reset before reusing installed buffers.");return 0;}
    return 1;
}
static int drawing_complete(void){
    volatile uint8_t*flags=mos_sysvars()+sysvar_vdp_pflags;*flags&=(uint8_t)~vdp_pflag_point;
    const uint8_t b[7]={23,0,132,0,0,0,0};send(b,7);uint32_t start=getsysvar_time();
    while(!(*flags&vdp_pflag_point))if((uint32_t)(getsysvar_time()-start)>7200ul)return 0;
    return 1;
}
static int join(char*out,unsigned cap,const char*dir,const char*file){
    if(strlen(dir)+strlen(file)+2>=cap)return 0;
    strcpy(out,dir);strcat(out,"/");strcat(out,file);return 1;
}
/* Buffer 0 is the existing loader-root reservation in every compiled profile. */
static int replay(const char*path){
    FILE*f=fopen(path,"rb");if(!f){printf("Cannot read %s\n",path);return 0;}
    uint8_t data[96],head[8]={23,0,160,0,0,0,0,0};size_t n;unsigned chunks=0;
    command(0,2);
    while((n=fread(data,1,sizeof data,f))!=0){
        word(head+6,(unsigned)n);send(head,8);send(data,(unsigned)n);
        if(!echo()){fclose(f);return 0;}++chunks;
    }
    int bad=ferror(f);fclose(f);if(bad){puts("Read failed; no program call sent.");command(0,2);return 0;}
    if(chunks){command(0,14);command(0,1);if(!echo())return 0;}
    command(0,2);return echo();
}
static int json_number(const char*path,const char*key,unsigned*out,int array){
    FILE*f=fopen(path,"rb");if(!f)return 0;char needle[80];snprintf(needle,sizeof needle,"\"%s\":",key);
    unsigned match=0;int ch;while((ch=fgetc(f))!=EOF){
        if(ch==needle[match]){if(!needle[++match])break;}else match=ch==needle[0]?1:0;
    }
    if(ch==EOF){fclose(f);return 0;}
    do{ch=fgetc(f);}while(ch==' '||ch=='\t'||ch=='\r'||ch=='\n');
    if(array){if(ch!='['){fclose(f);return 0;}do{ch=fgetc(f);}while(ch<=32&&ch!=EOF);}
    unsigned value=0,n=0;while(ch>='0'&&ch<='9'){
        if(value>(65535u-(unsigned)(ch-'0'))/10u){fclose(f);return 0;}
        value=value*10u+(unsigned)(ch-'0');++n;ch=fgetc(f);
    }
    fclose(f);if(!n)return 0;*out=value;return 1;
}
static int common_records(const char*path,int cleanup){
    FILE*f=fopen(path,"rb");if(!f)return 0;uint8_t h[8];size_t count;
    while((count=fread(h,1,6,f))==6){
        if(h[0]!=23||h[1]||h[2]!=160){fclose(f);return 0;}
        unsigned id=getword(h+3),op=h[5];
        if(!id||id==65535){fclose(f);return 0;}
        if(op==2){if(cleanup)command(id,2);}
        else if(op==0){if(fread(h,1,2,f)!=2||fseek(f,(long)getword(h),SEEK_CUR)){fclose(f);return 0;}}
        else{fclose(f);return 0;}
    }
    int good=!count&&!ferror(f);fclose(f);return good;
}
static int raw_cleanup(const char*payload){
    char path[256];if(strlen(payload)+13>=sizeof path)return 0;
    strcpy(path,payload);strcat(path,".cleanup.bin");FILE*f=fopen(path,"rb");
    if(!f){puts("Raw cleanup sidecar missing; compiled helpers remain installed.");return 1;}
    fclose(f);return replay(path);
}
int main(int argc,char**argv){
    int review=0,pos=1,portable=0,resident=0;const char*path;char file[256];
    if(pos<argc&&!strcmp(argv[pos],"--review")){review=1;++pos;}
    if(pos<argc&&!strcmp(argv[pos],"--resident")){resident=1;++pos;}
    else if(pos<argc&&!strcmp(argv[pos],"--portable")){portable=1;++pos;}
    if(argc-pos!=1){puts("golemrun payload.bin | --resident directory | --portable directory");return 1;}
    path=argv[pos];printf("Running Golem %s%s\n",path,portable?" (first buffered Sub)":resident?" (first resident Sub)":" (raw)");
    const uint8_t pixels[]={23,0,192,0,29,0,0,0,0};send(pixels,sizeof pixels);
    if(portable){
        unsigned entry;
        if(!join(file,sizeof file,path,"backends.json")||!json_number(file,"buffered_entries",&entry,1)){
            puts("No buffered entry. Native C must first be built into a VDPlet.");return 1;}
        if(!join(file,sizeof file,path,"common-install.bin")||!common_records(file,0)||!replay(file))return 1;
        if(!join(file,sizeof file,path,"buffered-install.bin")||!replay(file))return 1;
        command(entry,1);if(!echo()||!drawing_complete())return 1;
        OBSERVE();
        if(!join(file,sizeof file,path,"buffered-cleanup.bin")||!replay(file))return 1;
        if(!join(file,sizeof file,path,"common-install.bin")||!common_records(file,1))return 1;
    }else if(resident){
        if(!join(file,sizeof file,path,"bootstrap.bin")||!replay(file))return 1;
        if(!join(file,sizeof file,path,"invoke.bin")||!replay(file)||!drawing_complete())return 1;
        OBSERVE();
        if(!join(file,sizeof file,path,"cleanup.bin")||!replay(file))return 1;
    }else{
        if(!replay(path)||!drawing_complete())return 1;OBSERVE();if(!raw_cleanup(path))return 1;
    }
    if(!echo())return 1;
    puts("");
    if(review)puts("Type dir at the MOS prompt and report to the agent that it works.");
    return 0;
}
