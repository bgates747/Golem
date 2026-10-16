#ifndef FLOWER_PARAMETERS_HPP
#define FLOWER_PARAMETERS_HPP
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

namespace flower {
static const unsigned Fields=7, TextLimit=31;
static const char *const names[Fields]={"Petals","Vectors","Depth","Periods","Shrink","Radius","Angle (degrees)"};
struct Parameters {
    char text[Fields][TextLimit+1];
    float value[Fields];
    void defaults() {
        const char *const strings[Fields]={"3.03","1.98","0.6","66","0.8","512","0"};
        const float values[Fields]={3.03f,1.98f,0.6f,66.0f,0.8f,512.0f,0.0f};
        for(unsigned i=0;i<Fields;++i){strcpy(text[i],strings[i]);value[i]=values[i];}
    }
};
inline uint32_t bits(float n){uint32_t b;memcpy(&b,&n,4);return b;}
inline bool parse(unsigned field,const char *text,float &out) {
    if(field>=Fields||!text[0]||strlen(text)>TextLimit)return false;
    const char *p=text;if(*p=='+'||*p=='-')++p;
    bool digit=false,nonzero=false;
    while(*p>='0'&&*p<='9'){digit=true;nonzero|=*p!='0';++p;}
    if(*p=='.'){++p;while(*p>='0'&&*p<='9'){digit=true;nonzero|=*p!='0';++p;}}
    if(!digit)return false;
    unsigned exponent=0;
    if(*p=='e'||*p=='E'){
        ++p;if(*p=='+'||*p=='-')++p;
        if(*p<'0'||*p>'9')return false;
        while(*p>='0'&&*p<='9'){if(exponent<1001)exponent=exponent*10+unsigned(*p-'0');++p;}
    }
    if(*p)return false;
    // Bound the vendor parser's signed 24-bit exponent. Zero needs no scaling.
    if(nonzero&&exponent>1000)return false;
    char *end=0;
    const float n=nonzero?strtof(text,&end):0.0f;
    const uint32_t magnitude=bits(n)&UINT32_C(0x7fffffff);
    if((nonzero&&(!end||*end))||magnitude>=UINT32_C(0x7f800000))return false;
    if(nonzero&&magnitude<UINT32_C(0x00800000))return false;
    if((field==0||field==1||field==3)&&!(n>0.0f))return false;
    if(field==2&&!(n>-1.0f))return false;
    if(field==5&&n<0.0f)return false;
    if(field==6&&(n<0.0f||n>360.0f))return false;
    out=n;return true;
}
inline bool accept(Parameters &params,unsigned field,const char *replacement) {
    if(!replacement[0])return true;
    float n;if(!parse(field,replacement,n))return false;
    strcpy(params.text[field],replacement);params.value[field]=n;return true;
}
struct Editor {
    char replacement[TextLimit+1];unsigned size;
    void reset(){size=0;replacement[0]=0;}
    bool key(uint8_t ascii){
        if(ascii==8||ascii==127){if(size)replacement[--size]=0;return true;}
        if(ascii>=32&&ascii<=126&&size<TextLimit){replacement[size++]=char(ascii);replacement[size]=0;return true;}
        return false;
    }
};
}
#endif
