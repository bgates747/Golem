#include "mini.hpp"
// Exact rational decimal conversion. Rounding is nearest, ties to even.
// The small bigint is used only during compilation/metadata formatting.
namespace ag {
namespace {
struct Big {
    uint32_t*p=nullptr;unsigned n=0,cap=0;Cleanup cleanup;
    static void destroy(void*p){static_cast<Big*>(p)->~Big();}
    Big(){cleanup.attach(this,destroy);}explicit Big(uint64_t v):Big(){if(v){room(2);p[0]=(uint32_t)v;p[1]=(uint32_t)(v>>32);n=p[1]?2:1;}}
    Big(const Big&b):Big(){room(b.n);n=b.n;if(n)memcpy(p,b.p,n*4u);}
    Big&operator=(const Big&b){if(this!=&b){room(b.n);n=b.n;if(n)memcpy(p,b.p,n*4u);}return *this;}
    ~Big(){cleanup.detach();free(p);}
    void room(unsigned need){if(need<=cap)return;unsigned c=need*2+4;if(c>32768)panic("numeric literal exceeds compiler memory");auto*q=(uint32_t*)malloc(c*4u);if(!q)panic("out of numeric compiler memory");if(n)memcpy(q,p,n*4u);free(p);p=q;cap=c;}
    void trim(){while(n&&!p[n-1])--n;}
    unsigned bits()const{if(!n)return 0;uint32_t x=p[n-1];unsigned b=0;while(x){++b;x>>=1;}return (n-1)*32u+b;}
    void small(unsigned m,unsigned add=0){uint64_t carry=add;room(n+1);for(unsigned i=0;i<n;++i){uint64_t v=(uint64_t)p[i]*m+carry;p[i]=(uint32_t)v;carry=v>>32;}if(carry)p[n++]=(uint32_t)carry;}
    void shift(unsigned count){if(!n||!count)return;unsigned words=count/32,b=count%32;room(n+words+1);if(words){memmove(p+words,p,n*4u);memset(p,0,words*4u);n+=words;}if(b){uint32_t carry=0;for(unsigned i=words;i<n;++i){uint64_t v=((uint64_t)p[i]<<b)|carry;p[i]=(uint32_t)v;carry=(uint32_t)(v>>32);}if(carry)p[n++]=carry;}}
    int cmp(const Big&b)const{if(n!=b.n)return n<b.n?-1:1;for(unsigned i=n;i;--i)if(p[i-1]!=b.p[i-1])return p[i-1]<b.p[i-1]?-1:1;return 0;}
    void sub(const Big&b){uint64_t borrow=0;for(unsigned i=0;i<n;++i){uint64_t v=(uint64_t)(i<b.n?b.p[i]:0)+borrow;uint32_t old=p[i];p[i]=(uint32_t)((uint64_t)old-v);borrow=(uint64_t)old<v;}trim();}
    void ten(unsigned exponent){for(unsigned i=0;i<exponent;++i)small(10);}
};
uint64_t quotient(Big num,const Big&den,unsigned maxbit){uint64_t q=0;for(unsigned k=maxbit+1;k;--k){Big part(den);part.shift(k-1);if(num.cmp(part)>=0){num.sub(part);q|=1ull<<(k-1);}}num.shift(1);int order=num.cmp(den);if(order>0||(order==0&&(q&1)))++q;return q;}
int order2(const Big&num,const Big&den,int exponent){if(exponent>=0){Big b(den);b.shift((unsigned)exponent);return num.cmp(b);}Big a(num);a.shift((unsigned)-exponent);return a.cmp(den);}
int order10(const Big&num,const Big&den,int exponent){if(exponent>=0){Big b(den);b.ten((unsigned)exponent);return num.cmp(b);}Big a(num);a.ten((unsigned)-exponent);return a.cmp(den);}
}
Real parse_real(const char*text,char**end){
    const char*p=text;bool negative=false;Big num;int decimal=0,exponent=0;bool seen=false,dot=false;
    if(*p=='-'||*p=='+'){negative=*p=='-';++p;}
    while((*p>='0'&&*p<='9')||(*p=='.'&&!dot)){
        if(*p=='.'){dot=true;++p;continue;}
        seen=true;num.small(10,(unsigned)(*p-'0'));if(dot)--decimal;++p;
    }
    if(!seen){if(end)*end=(char*)text;return Real();}
    if(*p=='e'||*p=='E'){
        const char*before=p++;bool neg=false;if(*p=='+'||*p=='-'){neg=*p=='-';++p;}
        if(*p<'0'||*p>'9')p=before;
        else{while(*p>='0'&&*p<='9'){if(exponent<100000)exponent=exponent*10+(*p-'0');++p;}decimal+=neg?-exponent:exponent;}
    }
    if(end)*end=(char*)p;
    uint64_t sign=negative?0x8000000000000000ull:0;
    if(!num.n)return Real::bits(sign);
    // Decimal integers up to 2^53 are exactly representable in binary64.
    // Avoid rational long division for ubiquitous drawing/array literals.
    if(decimal==0&&num.bits()<=53){
        uint64_t integer=num.p[0];if(num.n>1)integer|=(uint64_t)num.p[1]<<32;
        Real value(integer);return negative?-value:value;
    }
    /* Bound enormous exponents before allocating powers of ten. */
    int approximate=(int)((int32_t)num.bits()*30103/100000)+decimal;
    if(approximate>310){errno=ERANGE;return Real::bits(sign|0x7ff0000000000000ull);}
    if(approximate< -330){errno=ERANGE;return Real::bits(sign);}
    Big den(1);if(decimal>=0)num.ten((unsigned)decimal);else den.ten((unsigned)-decimal);
    int e=(int)num.bits()-(int)den.bits();if(order2(num,den,e)<0)--e;
    if(e>1023){errno=ERANGE;return Real::bits(sign|0x7ff0000000000000ull);}
    if(e< -1075){errno=ERANGE;return Real::bits(sign);}
    int shift=e< -1022?1074:52-e;
    if(shift>=0)num.shift((unsigned)shift);else den.shift((unsigned)-shift);
    uint64_t q=quotient(num,den,53);
    if(e< -1022){if(!q)errno=ERANGE;return Real::bits(sign|q);}
    if(q>=0x20000000000000ull){q>>=1;++e;}
    if(e>1023){errno=ERANGE;return Real::bits(sign|0x7ff0000000000000ull);}
    return Real::bits(sign|((uint64_t)(e+1023)<<52)|(q&0xfffffffffffffull));
}
unsigned format_real(Real value,unsigned precision,char*out,unsigned capacity){
    char digits[24],tmp[24];unsigned used=0;
    auto put=[&](char ch){if(used+1>=capacity)panic("numeric formatting buffer");out[used++]=ch;};
    uint64_t bits=value.raw.v,mant=bits&0xfffffffffffffull;unsigned binary=(unsigned)((bits>>52)&2047);
    if(bits>>63)put('-');
    if(binary==2047){const char*s=mant?"nan":"inf";while(*s)put(*s++);out[used]=0;return used;}
    if(!binary&&!mant){put('0');out[used]=0;return used;}
    if(!precision)precision=1;if(precision>17)precision=17;
    int e2=binary?(int)binary-1023-52:-1074;if(binary)mant|=0x10000000000000ull;
    Big num(mant),den(1);if(e2>=0)num.shift((unsigned)e2);else den.shift((unsigned)-e2);
    int e=(int)(((int32_t)num.bits()-(int32_t)den.bits())*30103/100000);
    while(order10(num,den,e)<0)--e;while(order10(num,den,e+1)>=0)++e;
    int scale=(int)precision-1-e;if(scale>=0)num.ten((unsigned)scale);else den.ten((unsigned)-scale);
    uint64_t q=quotient(num,den,59),power=1;for(unsigned i=0;i<precision;++i)power*=10;
    if(q>=power){q/=10;++e;}
    unsigned n=0;do{tmp[n++]=(char)('0'+q%10);q/=10;}while(q);
    for(unsigned i=0;i<n;++i)digits[i]=tmp[n-i-1];
    while(n>1&&digits[n-1]=='0')--n;
    if(e< -4||e>=(int)precision){
        put(digits[0]);if(n>1){put('.');for(unsigned i=1;i<n;++i)put(digits[i]);}
        put('e');put(e<0?'-':'+');unsigned a=(unsigned)(e<0?-e:e);unsigned t=0;do{tmp[t++]=(char)('0'+a%10);a/=10;}while(a);if(t<2)tmp[t++]='0';while(t)put(tmp[--t]);
    }else if(e<0){put('0');put('.');for(int i=-1;i>e;--i)put('0');for(unsigned i=0;i<n;++i)put(digits[i]);}
    else{unsigned before=(unsigned)e+1;for(unsigned i=0;i<before||i<n;++i){if(i==before)put('.');put(i<n?digits[i]:'0');}}
    out[used]=0;return used;
}
}
