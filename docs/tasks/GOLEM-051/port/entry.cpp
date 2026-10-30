#include "recovery.hpp"
int golemc_entry(int,char**);
static int response(const char*path,char*program){
    FILE*f=fopen(path,"rb");if(!f){fprintf(stderr,"golemc: cannot open %s\n",path);return 1;}
    long length=-1;if(!fseek(f,0,SEEK_END))length=ftell(f);
    if(length<0||length>8192||fseek(f,0,SEEK_SET)){
        fclose(f);fputs("golemc: response file exceeds 8192 bytes\n",stderr);return 1;
    }
    // Keep the existing input limit without retaining its entire maximum
    // allocation throughout compilation of a short response file.
    char*text=(char*)malloc((size_t)length+1);if(!text){fclose(f);ag::panic("out of compiler memory");}
    size_t n=fread(text,1,(size_t)length,f);bool bad=ferror(f)||n!=(size_t)length;fclose(f);
    if(bad){free(text);fputs("golemc: response file exceeds 8192 bytes\n",stderr);return 1;}
    text[n]=0;char*args[128];unsigned count=1;args[0]=program;char*p=text;
    while(*p){while(*p&&isspace((unsigned char)*p))++p;if(!*p)break;
        if(count==128){free(text);fputs("golemc: too many response arguments\n",stderr);return 1;}
        bool quote=*p=='"';if(quote)++p;args[count++]=p;
        while(*p&&(quote?*p!='"':!isspace((unsigned char)*p)))++p;
        if(quote&&!*p){free(text);fputs("golemc: unclosed response quote\n",stderr);return 1;}
        if(*p)*p++=0;
    }
    int rc=golemc_entry((int)count,args);free(text);return rc;
}
int main(int argc,char**argv){
    char stack;ag::stack_high=(uintptr_t)&stack+256;
    if(argc==2&&argv[1][0]=='@')return response(argv[1]+1,argv[0]);
    // Agent qualification and optional unattended compilation. Each line names
    // one response file; errors are logged and do not skip subsequent jobs.
    if(argc==3&&!strcmp(argv[1],"--batch")){
        FILE*f=fopen(argv[2],"r");if(!f)return 1;
        FILE*log=fopen("batch.log","w");if(!log){fclose(f);return 1;}
        freopen("batch-output.log","w",stdout);freopen("batch-errors.log","w",stderr);
        char line[256];unsigned index=0,failed=0;
        while(fgets(line,sizeof line,f)){
            size_t n=strlen(line);while(n&&isspace((unsigned char)line[n-1]))line[--n]=0;if(!n)continue;
            fprintf(stderr,"[case %u %s]\n",index,line);fflush(stderr);
            int rc=response(line,argv[0]);fprintf(log,"%u %d %s\n",index++,rc,line);fflush(log);if(rc)++failed;
        }
        fflush(stdout);fflush(stderr);fclose(log);fclose(f);return failed?1:0;
    }
    return golemc_entry(argc,argv);
}
