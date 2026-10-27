#include "recovery.hpp"
int main(){
    char stack;ag::stack_high=(uintptr_t)&stack+256;uint64_t state=0x719afe125be30103ull;unsigned checks=0;
    const char*edges[]={"0","-0","4294967295","-2147483648","16777217","1.000000059604644775390625",
        "0.00006103515625","2.2250738585072014e-308","4.9406564584124654e-324",
        "1.7976931348623157e308","1e309","1e-999","3.4028234663852886e38","1.1754943508222875e-38"};
    for(const char*s:edges){char*a,*b;double ref=::strtod(s,&a);auto value=ag::parse_real(s,&b);uint64_t bits;memcpy(&bits,&ref,8);if(bits!=value.raw.v||a-s!=b-s){fprintf(stderr,"parse mismatch %s\n",s);return 1;}++checks;}
    for(unsigned i=0;i<2000;++i){
        state^=state<<13;state^=state>>7;state^=state<<17;if((state&0x7ff0000000000000ull)==0x7ff0000000000000ull)continue;
        double ref;memcpy(&ref,&state,8);auto value=ag::Real::bits(state);
        for(unsigned precision:{6u,9u,17u}){
            char expected[100],actual[100];snprintf(expected,sizeof expected,"%.*g",(int)precision,ref);
            ag::format_real(value,precision,actual,sizeof actual);
            if(strcmp(expected,actual)){fprintf(stderr,"format mismatch %s != %s p%u\n",expected,actual,precision);return 1;}++checks;
            char*end;auto parsed=ag::parse_real(expected,&end);double number=::strtod(expected,nullptr);uint64_t bits;memcpy(&bits,&number,8);
            if(*end||parsed.raw.v!=bits){fprintf(stderr,"roundtrip mismatch %s\n",expected);return 1;}++checks;
        }
    }
    for(unsigned i=0;i<2000;++i){
        state^=state<<13;state^=state>>7;state^=state<<17;
        uint32_t bits=(uint32_t)state;float a;memcpy(&a,&bits,4);
        if(!__builtin_isfinite(a))continue;
        for(float toward:{__builtin_inff(),-__builtin_inff(),0.0f}){
            float ref=::nextafterf(a,toward),actual=std::nextafter(a,toward);uint32_t x,y;
            memcpy(&x,&ref,4);memcpy(&y,&actual,4);if(x!=y){fprintf(stderr,"nextafter mismatch\n");return 1;}++checks;
        }
    }
    printf("Exact numeric parsing/formatting: %u checks passed.\n",checks);return 0;
}
