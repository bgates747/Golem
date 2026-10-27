/* Bounded direct-to-buffer compiler; no AST interpreter or host subprocesses. */
#include "golem.h"
#include <ctype.h>
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

enum { T_END, T_NAME, T_NUMBER, T_STRING, T_PUNCT };
enum { U16, I16, U8 };
typedef struct { char text[256]; unsigned kind, line, column; } Token;
typedef struct { char name[32]; unsigned type, id; int32_t low, high; } Global;
typedef struct { char name[32]; unsigned id; } Sub;
typedef struct { char name[32]; unsigned record, offset, caller, line, column; } Call;
typedef struct {
    const char *source;
    unsigned pos, line, column, global_count, sub_count, call_count;
    unsigned depth, active[8], seen_code, in_sub, current_sub, executable;
    unsigned acc, status, carry, zero, overflow, conversion, bounds;
    uint32_t payload;
    Token token;
    Global globals[G_GLOBAL_MAX];
    Sub subs[G_SUB_MAX];
    Call calls[128];
    GPackage *package;
    char *error;
    unsigned error_size;
    jmp_buf escape;
} Compiler;
/* Static working state avoids a large ADL stack frame. One compilation at a time. */
static Compiler c;

unsigned g_word(const uint8_t *p) { return (unsigned)p[0] | ((unsigned)p[1] << 8); }
void g_putword(uint8_t *p, unsigned v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
static void fail(const char *message) {
    snprintf(c.error, c.error_size, "line %u, column %u: %s",
             c.token.line, c.token.column, message);
    longjmp(c.escape, 1);
}
static int same(const char *a, const char *b) {
    while (*a && *b && tolower((unsigned char)*a) == tolower((unsigned char)*b)) { ++a; ++b; }
    return !*a && !*b;
}
static int reserved(const char *s) {
    static const char *const words[] = {"print","uint16","sint16","uint8","float32","bool",
        "bounds","range","for","in","to","step","next","sub","endsub","if","then",
        "else","elseif","endif","do","while","until","loop","switch","case","default",
        "endswitch","array","table","record","endrecord","function","endfunction",
        "return","draw_point","draw_line","fill_quad","draw_bitmap",0};
    unsigned i; for (i = 0; words[i]; ++i) if (same(s, words[i])) return 1;
    return 0;
}
static int letter(int ch) { return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_'; }
static int digit(int ch) { return ch >= '0' && ch <= '9'; }
static void advance(void) {
    if (c.source[c.pos++] == '\n') { ++c.line; c.column = 1; } else ++c.column;
}
static void next(void) {
    unsigned n = 0; int ch;
    for (;;) {
        while (c.source[c.pos] && isspace((unsigned char)c.source[c.pos])) advance();
        if (c.source[c.pos] != '/' || c.source[c.pos + 1] != '/') break;
        while (c.source[c.pos] && c.source[c.pos] != '\n') advance();
    }
    c.token.line = c.line; c.token.column = c.column; ch = (unsigned char)c.source[c.pos];
    c.token.text[0] = 0;
    if (!ch) { c.token.kind = T_END; return; }
    if (ch == ';') fail("remove ';': statements have no terminators");
    if (letter(ch)) {
        c.token.kind = T_NAME;
        while (letter(c.source[c.pos]) || digit(c.source[c.pos])) {
            if (n == 31) fail("identifier exceeds 31 bytes");
            c.token.text[n++] = c.source[c.pos]; advance();
        }
    } else if (digit(ch)) {
        c.token.kind = T_NUMBER;
        while (digit(c.source[c.pos])) {
            if (n == 10) fail("integer literal too long");
            c.token.text[n++] = c.source[c.pos]; advance();
        }
        if (letter(c.source[c.pos]) || c.source[c.pos] == '.') fail("expected decimal integer literal");
    } else if (ch == '"') {
        c.token.kind = T_STRING; advance();
        while (c.source[c.pos] != '"') {
            ch = (unsigned char)c.source[c.pos];
            if (!ch) fail("unterminated string");
            if (ch == '\\' && (c.source[c.pos+1] == '\\' || c.source[c.pos+1] == '"' || c.source[c.pos+1] == 'n')) {
                advance(); ch = c.source[c.pos] == 'n' ? '\n' : c.source[c.pos];
            }
            if (n == 255) fail("string exceeds 255 bytes");
            if (ch < 32 && ch != '\n' && ch != '\r' && ch != '\t') fail("unsupported control byte in string");
            c.token.text[n++] = (char)ch; advance();
        }
        advance();
    } else {
        c.token.kind = T_PUNCT; c.token.text[n++] = (char)ch; advance();
    }
    c.token.text[n] = 0;
}
static int keyword(const char *s) { return c.token.kind == T_NAME && same(c.token.text, s); }
static int take(const char *s) {
    if ((c.token.kind == T_NAME && same(c.token.text,s)) ||
        (c.token.kind == T_PUNCT && !strcmp(c.token.text,s))) { next(); return 1; }
    return 0;
}
static void need(const char *s) {
    char message[90]; if (take(s)) return;
    snprintf(message,sizeof message,"expected '%s'",s); fail(message);
}
static void name(char out[32]) {
    if (c.token.kind != T_NAME || reserved(c.token.text)) fail("expected non-reserved identifier");
    strcpy(out,c.token.text); next();
}
static unsigned number(void) {
    uint32_t value = 0; unsigned i;
    if (c.token.kind != T_NUMBER) fail("expected unsigned decimal integer");
    for (i=0;c.token.text[i];++i) {
        value = value*10u+(unsigned)(c.token.text[i]-'0');
        if (value>65535ul) fail("integer literal outside 0..65535");
    }
    next(); return (unsigned)value;
}
static int32_t signed_number(void) {
    int negative = take("-"); int32_t v;
    if (!negative) take("+");
    v = (int32_t)number(); return negative ? -v : v;
}
static GBuffer *buffer(unsigned id) { return &c.package->records[id-c.package->base]; }
static void byte(unsigned id, unsigned v) {
    GBuffer *b = buffer(id);
    if (b->size == G_BUFFER_MAX || c.payload == G_PAYLOAD_MAX) fail("compiled buffer/payload limit exceeded");
    if (b->size == b->capacity) {
        unsigned capacity = b->capacity ? b->capacity*2u : 32u;
        uint8_t *p = (uint8_t *)realloc(b->data,capacity);
        if (!p) fail("out of compiler memory");
        b->data=p; b->capacity=capacity;
    }
    b->data[b->size++]=(uint8_t)v; ++c.payload;
}
static void word(unsigned id, unsigned v) { byte(id,v); byte(id,v>>8); }
static void header(unsigned out,unsigned id,unsigned op) { byte(out,23);byte(out,0);byte(out,160);word(out,id);byte(out,op); }
static unsigned resource(unsigned size) {
    unsigned id,i;
    if(c.package->count==G_RECORD_MAX || c.package->base+c.package->count>65534u) fail("buffer ID/resource limit exceeded");
    id=c.package->base+c.package->count++;
    for(i=0;i<size;++i) byte(id,0);
    return id;
}
static void set(unsigned out,unsigned dest,unsigned off,unsigned width,unsigned value) {
    unsigned i; header(out,dest,5);byte(out,0xc2);word(out,off);word(out,width);
    for(i=0;i<width;++i) { byte(out,value);value>>=8; }
}
static void copy(unsigned out,unsigned dest,unsigned off,unsigned width,unsigned source,unsigned soff) {
    header(out,dest,5);byte(out,0xe2);word(out,off);word(out,width);word(out,source);word(out,soff);
}
static void add(unsigned out,unsigned dest,unsigned width,unsigned value,int fetched) {
    header(out,dest,5);byte(out,fetched?0xe4:0xc4);word(out,0);word(out,width);
    if(fetched) { word(out,value);word(out,0); } else word(out,value);
}
static void compare(unsigned out,unsigned target,unsigned op,unsigned cond,unsigned left,unsigned loff,unsigned right,unsigned roff) {
    header(out,target,op);byte(out,cond|0xa0);word(out,left);word(out,loff);word(out,right);word(out,roff);
}
static void propagate(unsigned out) { compare(out,65535u,8,3,c.status,0,c.zero,0); }
static Global *lookup(const char *s) {
    unsigned i;
    for(i=0;i<c.global_count;++i) if(!strcmp(s,c.globals[i].name)) return &c.globals[i];
    fail("undeclared variable (identifiers are case sensitive)");return 0;
}
static void literal_check(Global *g,int32_t v) {
    if(v<g->low||v>g->high) fail("literal/constant result is outside destination type or Bounds");
}
/* Compare words as zero-extended bytes: never pass FFFF to stock conditional. */
static void byte_probe(unsigned out,unsigned source,unsigned offset) { set(out,c.carry,0,2,0);copy(out,c.carry,0,1,source,offset); }
static void check_range(unsigned out,unsigned source,Global *g) {
    unsigned hi,lo,bound_hi,bound_lo,scratch;
    if(g->type!=U16 || (g->low==0 && g->high==65535)) return;
    /* Finite helper per check. Boundary tests use high first, then low on equality. */
    for(hi=0;hi<2;++hi) {
        unsigned bound=(unsigned)(hi?g->high:g->low);
        unsigned cond=hi?5u:4u; /* greater / less */
        bound_hi=resource(2);bound_lo=resource(2);scratch=resource(0);
        g_putword(buffer(bound_hi)->data,bound>>8);g_putword(buffer(bound_lo)->data,bound&255u);
        byte_probe(out,source,1);
        compare(out,c.bounds,8,cond,c.carry,0,bound_hi,0);
        /* Call low-byte check only when high byte equals its bound. */
        byte_probe(scratch,source,0);compare(scratch,c.bounds,8,cond,c.carry,0,bound_lo,0);
        byte_probe(out,source,1);compare(out,scratch,6,2,c.carry,0,bound_hi,0);propagate(out);
        lo=0;(void)lo;
    }
}
static void handler(unsigned out,unsigned code,const char *message) {
    unsigned i;set(out,c.status,0,2,code);
    for(i=0;message[i];++i) byte(out,(uint8_t)message[i]);
}
typedef struct { unsigned literal,value,id; } Operand;
static Operand u16_operand(void) {
    Operand a={0,0,0};
    if(c.token.kind==T_NUMBER) { a.literal=1;a.value=number(); }
    else {
        Global *g;char s[32];name(s);g=lookup(s);
        if(g->type!=U16) fail("expected uint16 operand; no implicit conversion");
        a.id=g->id;
    }
    return a;
}
static void uint_expression(unsigned out,Global *dest,int initial) {
    Operand a=u16_operand(); unsigned constant=a.literal;uint32_t sum=a.value;
    if(a.literal) set(out,c.acc,0,2,a.value);else copy(out,c.acc,0,2,a.id,0);
    while(take("+")) {
        a=u16_operand();constant=constant&&a.literal;
        if(initial&&!constant) fail("initializer addition must be constant");
        if(constant) { sum+=a.value;if(sum>65535ul) fail("constant uint16 addition overflows"); }
        add(out,c.acc,2,a.literal?a.value:a.id,!a.literal);
        byte_probe(out,c.acc,2);compare(out,c.overflow,8,3,c.carry,0,c.zero,0);
    }
    if(constant) literal_check(dest,(int32_t)sum);
    check_range(out,c.acc,dest);
    copy(out,dest->id,0,2,c.acc,0);
}
static void converted(unsigned out,unsigned dest,unsigned offset) {
    Global *g;char s[32];unsigned limit;
    need("sint16");need("(");name(s);g=lookup(s);need(")");
    if(g->type!=U16) fail("sint16() here requires a uint16 variable");
    limit=resource(2);g_putword(buffer(limit)->data,127);
    byte_probe(out,g->id,1);compare(out,c.conversion,8,5,c.carry,0,limit,0);
    copy(out,dest,offset,2,g->id,0);
}
static void assign(unsigned out,Global *g,int initial) {
    unsigned i;
    for(i=0;i<c.depth;++i) if(c.active[i]==g->id) fail("cannot assign an active For counter");
    if(g->type==U16) { uint_expression(out,g,initial);return; }
    if(g->type==I16 && keyword("sint16")) {
        if(g->low!=-32768l||g->high!=32767l) fail("bounded sint16 conversion is not supported in this release");
        converted(out,g->id,0);return;
    }
    if(c.token.kind==T_NAME) {
        Global *source;char s[32];name(s);source=lookup(s);
        if(source->type!=g->type) fail("assignment type mismatch; explicit conversion required");
        if(source->low<g->low||source->high>g->high) fail("source Bounds are wider than destination Bounds");
        copy(out,g->id,0,g->type==U8?1:2,source->id,0);
    } else {
        int32_t v=signed_number();literal_check(g,v);set(out,g->id,0,g->type==U8?1:2,(unsigned)v);
    }
}
static void declaration(unsigned out) {
    Global *g;char s[32];unsigned type,i;
    if(c.seen_code) fail("global declarations must precede executable code and Subs");
    type=keyword("uint16")?U16:keyword("sint16")?I16:U8;next();name(s);
    for(i=0;i<c.global_count;++i) if(!strcmp(c.globals[i].name,s)) fail("duplicate global name");
    if(c.global_count==G_GLOBAL_MAX) fail("more than 64 globals");
    g=&c.globals[c.global_count++];strcpy(g->name,s);g->type=type;
    g->low=type==I16?-32768l:0;g->high=type==I16?32767l:type==U8?255:65535;
    if(take("Bounds")) {
        int32_t low,high;need("(");low=signed_number();need(",");high=signed_number();need(")");
        if(low<g->low||high>g->high||low>high) fail("invalid inclusive Bounds for type");
        g->low=low;g->high=high;
    }
    g->id=resource(type==U16?3:type==I16?2:1);
    if(take("=")) assign(out,g,1);else literal_check(g,0);
}
static void statements(unsigned out,int closing);
static void loop(unsigned out) {
    char s[32],closing[32];Global *g;unsigned first,last,body,state,scratch,bound_high,bound_low,patch,advance_at,i;
    next();name(s);g=lookup(s);
    if(g->type!=U16) fail("For counter must be declared uint16");
    if(c.depth==8) fail("loop nesting exceeds eight");
    for(i=0;i<c.depth;++i) if(c.active[i]==g->id) fail("nested loops need distinct counters");
    if(take("In")) {
        need("Range");need("(");first=number();need(":");last=number();
        if(take(":")) { if(number()!=1) fail("only unit Step is supported"); }need(")");
    } else {
        need("=");first=number();need("To");last=number();
        if(take("Step")) { if(number()!=1) fail("only unit Step is supported"); }
    }
    if(first>last) fail("Range must be nonempty and ascending");
    literal_check(g,(int32_t)first);literal_check(g,(int32_t)last);
    body=resource(0);state=resource(6);scratch=state;bound_high=state;bound_low=state;
    g_putword(buffer(state)->data+2,last>>8);g_putword(buffer(state)->data+4,last&255u);
    set(out,g->id,0,3,first);set(out,scratch,0,2,0);
    c.active[c.depth++]=g->id;statements(body,1);--c.depth;
    need("Next");name(closing);if(strcmp(s,closing)) fail("Next name does not match For counter");
    /* If high differs, continue; if both zero-extended bytes match, return. */
    header(body,body,10);patch=buffer(body)->size;word(body,0);byte(body,0);
    byte(body,0xa3);word(body,g->id);word(body,1);word(body,bound_high);word(body,2);
    copy(body,scratch,0,1,g->id,0);
    compare(body,65535u,8,2,scratch,0,bound_low,4);
    advance_at=buffer(body)->size;g_putword(buffer(body)->data+patch,advance_at);
    add(body,g->id,2,1,0);header(body,body,7);
    header(out,body,1);propagate(out);
}
static void coordinate(unsigned out,unsigned target,unsigned offset) {
    if(keyword("sint16")) converted(out,target,offset);
    else if(c.token.kind==T_NAME) {
        char s[32];Global *g;name(s);g=lookup(s);
        if(g->type!=I16) fail("drawing coordinates require sint16; use explicit sint16(uint16Variable)");
        copy(out,target,offset,2,g->id,0);
    } else {
        int32_t v=signed_number();if(v<-32768l||v>32767l) fail("coordinate outside sint16 range");
        g_putword(buffer(target)->data+offset,(unsigned)v);
    }
}
static void drawing(unsigned out) {
    unsigned count=keyword("draw_point")?1:keyword("draw_line")?2:4;
    unsigned command=resource(0),colour=0,colour_id=0,i,offsets[4];
    next();need("(");
    if(c.token.kind==T_NUMBER) { colour=number();if(colour>63) fail("colour outside 0..63"); }
    else {
        char s[32];Global *g;name(s);g=lookup(s);
        if(g->type!=U8||g->low<0||g->high>63) fail("colour variable needs uint8 Bounds within 0..63");
        colour_id=g->id;
    }
    byte(command,18);byte(command,0);byte(command,colour);
    if(colour_id) copy(out,command,2,1,colour_id,0);
    for(i=0;i<count;++i) {
        /* Quad emits A,B,D,C to match the existing two-triangle implementation. */
        byte(command,25);byte(command,count==1?69:count==2?(i?5:4):(i<2?4:85));
        offsets[i]=buffer(command)->size;word(command,0);word(command,0);
    }
    for(i=0;i<count;++i) {
        unsigned slot=count==4?(i==2?3:i==3?2:i):i;
        need(",");coordinate(out,command,offsets[slot]);need(",");coordinate(out,command,offsets[slot]+2);
    }
    need(")");header(out,command,1);c.package->flags|=1;
}
static void call(unsigned out,char s[32]) {
    Call *p;
    if(c.call_count==128) fail("more than 128 call sites");
    need("(");need(")");p=&c.calls[c.call_count++];strcpy(p->name,s);
    p->record=out;p->caller=c.in_sub?c.current_sub:G_SUB_MAX;
    p->line=c.token.line;p->column=c.token.column;
    header(out,0,1);p->offset=buffer(out)->size-3;propagate(out);
}
static void statements(unsigned out,int closing) {
    while(c.token.kind!=T_END) {
        if(keyword("Next")||keyword("EndSub")) break;
        if(keyword("Sub")) { if(closing) fail("Subs cannot be nested");break; }
        if(keyword("uint16")||keyword("sint16")||keyword("uint8")) { declaration(out);continue; }
        c.seen_code=1;++c.executable;
        if(take("print")) {
            unsigned i;if(c.token.kind!=T_STRING) fail("print requires a string literal");
            for(i=0;c.token.text[i];++i) byte(out,(uint8_t)c.token.text[i]);next();
        } else if(keyword("For")) loop(out);
        else if(keyword("draw_point")||keyword("draw_line")||keyword("fill_quad")) drawing(out);
        else if(c.token.kind==T_NAME&&!reserved(c.token.text)) {
            char s[32];name(s);
            if(c.token.kind==T_PUNCT&&!strcmp(c.token.text,"(")) call(out,s);
            else { Global *g=lookup(s);need("=");assign(out,g,0); }
        } else fail("unsupported statement in on-Agon profile");
    }
}
static unsigned find_sub(const char *s) {
    unsigned i;for(i=0;i<c.sub_count;++i) if(!strcmp(s,c.subs[i].name)) return i;
    fail("unknown Sub name");return 0;
}
static unsigned visit(unsigned at,unsigned mask,unsigned depth) {
    unsigned i,max=depth;
    if(depth>4) fail("Sub call depth exceeds four");
    if(mask&(1u<<at)) fail("recursive Sub calls are not supported");
    mask|=1u<<at;
    for(i=0;i<c.call_count;++i) if(c.calls[i].caller==at) {
        unsigned value=visit(find_sub(c.calls[i].name),mask,depth+1);
        if(value>max) max=value;
    }
    return max;
}
void g_free(GPackage *p) {
    unsigned i;for(i=0;i<p->count;++i) free(p->records[i].data);
    memset(p,0,sizeof *p);
}
int g_compile(const char *source,unsigned base,GPackage *out,char *error,unsigned error_size) {
    unsigned root,i;
    memset(&c,0,sizeof c);memset(out,0,sizeof *out);
    c.source=source;c.line=c.column=1;c.package=out;c.error=error;c.error_size=error_size;
    out->base=base;
    if(setjmp(c.escape)) return 0;
    c.token.line=c.token.column=1;
    if(strlen(source)>G_SOURCE_MAX) fail("source exceeds 16384 bytes");
    if(!base||base>65534u) fail("first buffer ID outside 1..65534");
    root=resource(0);out->entry=root;
    c.acc=resource(3);c.status=resource(2);c.carry=resource(2);c.zero=resource(2);
    c.overflow=resource(0);c.conversion=resource(0);c.bounds=resource(0);out->status=c.status;
    handler(c.overflow,1,"\r\nGolem error: uint16 addition overflow\r\n");
    handler(c.conversion,6,"\r\nGolem error: sint16 conversion out of range\r\n");
    handler(c.bounds,7,"\r\nGolem error: assignment outside Bounds\r\n");
    set(root,c.status,0,2,0);next();statements(root,0);
    if(keyword("Sub")) {
        unsigned top_executable=c.executable;
        if(top_executable) fail("choose top-level statements or named Subs, not both");
        c.seen_code=1;
        while(take("Sub")) {
            Sub *s;char nm[32];unsigned j;name(nm);
            for(j=0;j<c.global_count;++j) if(!strcmp(nm,c.globals[j].name)) fail("Sub collides with global name");
            for(j=0;j<c.sub_count;++j) if(!strcmp(nm,c.subs[j].name)) fail("duplicate Sub name");
            if(c.sub_count==G_SUB_MAX) fail("more than 16 Subs");
            c.current_sub=c.sub_count;s=&c.subs[c.sub_count++];strcpy(s->name,nm);s->id=resource(0);
            c.in_sub=1;statements(s->id,2);c.in_sub=0;need("EndSub");
        }
        header(root,c.subs[0].id,1);propagate(root);
    }
    if(c.token.kind!=T_END) fail("unexpected block delimiter or trailing source");
    for(i=0;i<c.call_count;++i) {
        unsigned sub=find_sub(c.calls[i].name);
        g_putword(buffer(c.calls[i].record)->data+c.calls[i].offset,c.subs[sub].id);
    }
    for(i=0;i<c.sub_count;++i) visit(i,0,1);
    /* Zero-length records cannot be written by the stock WRITE command. */
    for(i=0;i<out->count;++i) if(!out->records[i].size) header(base+i,65535u,7);
    error[0]=0;return 1;
}
int g_write(FILE *f,const GPackage *p) {
    unsigned i;uint8_t h[16]={'G','O','B','1',0};
    g_putword(h+4,p->count);g_putword(h+6,p->base);g_putword(h+8,p->entry);
    g_putword(h+10,p->status);g_putword(h+12,p->flags);
    if(fwrite(h,1,16,f)!=16) return 0;
    for(i=0;i<p->count;++i) {
        g_putword(h,p->base+i);g_putword(h+2,p->records[i].size);
        if(fwrite(h,1,4,f)!=4 || fwrite(p->records[i].data,1,p->records[i].size,f)!=p->records[i].size) return 0;
    }
    return !ferror(f);
}
int g_read(FILE *f,GPackage *p,char *error,unsigned error_size) {
    unsigned i;uint32_t total=0;uint8_t h[16];
    memset(p,0,sizeof *p);
    if(fread(h,1,16,f)!=16||memcmp(h,"GOB1",4)||g_word(h+14)||g_word(h+12)>1) goto invalid;
    p->count=g_word(h+4);p->base=g_word(h+6);p->entry=g_word(h+8);p->status=g_word(h+10);p->flags=g_word(h+12);
    if(!p->count||p->count>G_RECORD_MAX) { p->count=0;goto invalid; }
    if(!p->base || (uint32_t)p->base+p->count>65535ul || p->entry<p->base ||
       p->entry>=p->base+p->count || p->status<p->base || p->status>=p->base+p->count) goto invalid;
    for(i=0;i<p->count;++i) {
        unsigned n;
        if(fread(h,1,4,f)!=4||g_word(h)!=p->base+i) goto invalid;
        n=g_word(h+2);total+=n;
        if(!n||n>G_BUFFER_MAX||total>G_PAYLOAD_MAX) goto invalid;
        p->records[i].data=(uint8_t *)malloc(n);
        if(!p->records[i].data) { snprintf(error,error_size,"out of loader memory");return 0; }
        p->records[i].size=p->records[i].capacity=n;
        if(fread(p->records[i].data,1,n,f)!=n) goto invalid;
    }
    if(fgetc(f)!=EOF||ferror(f)||p->records[p->status-p->base].size<2) goto invalid;
    error[0]=0;return 1;
invalid:
    snprintf(error,error_size,"invalid, unsupported or truncated GOB1 package");return 0;
}
int g_write_map(FILE *f,const GPackage *p) {
    unsigned i;
    fprintf(f,"GOB1 entry=%u status=%u first=%u count=%u flags=%u\n",p->entry,p->status,p->base,p->count,p->flags);
    for(i=0;i<c.global_count;++i) {
        const Global *g=&c.globals[i];
        fprintf(f,"global %s %s buffer=%u Bounds(%ld,%ld)\n",g->type==U16?"uint16":g->type==I16?"sint16":"uint8",
                g->name,g->id,(long)g->low,(long)g->high);
    }
    for(i=0;i<c.sub_count;++i) fprintf(f,"Sub %s buffer=%u\n",c.subs[i].name,c.subs[i].id);
    for(i=0;i<p->count;++i) fprintf(f,"buffer %u bytes=%u\n",p->base+i,p->records[i].size);
    return !ferror(f);
}
