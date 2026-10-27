/* Small /mos/golemc.bin dispatcher; workers use the main user RAM below it. */
#include <agon/mos.h>
#include <stdio.h>
#include <string.h>
static char command[2048],first[48];
static int compile(int argc,char**argv){
    const char*mode=argc>1?argv[1]:"";const char*worker="raw";unsigned used;int i;
    if(argc<2||!strcmp(mode,"--help")){
        puts("Golem native compiler: source -> stock VDP buffers / native C");
        puts("golemc source.golem payload.bin");
        puts("golemc --resident [options] source.golem output-directory");
        puts("golemc --portable [options] source.golem output-directory");
        puts("golemc --stock-matrix [options] source.golem output-directory");
        puts("golemc @build.args     (options/paths from a text file)");return 0;
    }
    if(mode[0]=='@'){
        FILE*f=fopen(mode+1,"r");if(!f){puts("golemc: cannot open response file");return 1;}
        unsigned n=0;int ch;do{ch=fgetc(f);}while(ch!=EOF&&ch<=32);
        while(ch>32&&ch!=EOF&&n+1<sizeof first){first[n++]=(char)ch;ch=fgetc(f);}first[n]=0;fclose(f);mode=first;
    }
    if(!strcmp(mode,"--portable"))worker="portable";
    else if(!strcmp(mode,"--resident"))worker="resident";
    else if(!strcmp(mode,"--stock-matrix"))worker="stock-matrix";
    snprintf(command,sizeof command,"runbin /golem/compiler/%s.bin",worker);used=strlen(command);
    for(i=1;i<argc;++i){
        if(used+strlen(argv[i])+4>=sizeof command){puts("golemc: command too long; use @build.args");return 1;}
        command[used++]=' ';
        for(const char*p=argv[i];*p;++p){if(*p=='"'){puts("golemc: quote inside argument is unsupported");return 1;}command[used++]=*p;}
        command[used]=0;
    }
    return mos_oscli(command,0,0);
}

/* Batch supervision stays above worker RAM. Every job follows the normal
 * fresh MOS application load, so compilation never inherits a previous heap. */
int main(int argc,char**argv){
 if(argc!=3||strcmp(argv[1],"--batch"))return compile(argc,argv);
 FILE*f=fopen("batch.log","w");if(!f)return 1;fclose(f);
 f=fopen("batch-errors.log","w");if(f)fclose(f);
 f=fopen("batch-output.log","w");if(f)fclose(f);
 long position=0;unsigned index=0,failed=0;char line[256],response[258];
 for(;;){
  f=fopen(argv[2],"r");if(!f)return 1;
  if(fseek(f,position,SEEK_SET)){fclose(f);return 1;}
  if(!fgets(line,sizeof line,f)){fclose(f);break;}
  position=ftell(f);fclose(f);if(position<0)return 1;
  unsigned n=strlen(line);while(n&&(unsigned char)line[n-1]<=32)line[--n]=0;if(!n)continue;
  response[0]='@';strcpy(response+1,line);char*args[]={"golemc",response};
  int rc=compile(2,args);
  f=fopen("batch.log","a");if(!f)return 1;fprintf(f,"%u %d %s\n",index++,rc,line);fclose(f);
  if(rc)++failed;
 }
 return failed?1:0;
}
