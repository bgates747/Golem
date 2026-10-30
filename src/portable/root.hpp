#pragma once
#include "backends.hpp"
#include <array>
namespace root_v1 {
using namespace dual;
constexpr unsigned Header=112, Interface=12, Language=14, Runtime=16, Generation=20,
                   LastEntry=24, Requested=28, LastSession=32;
struct Binding {unsigned role=0,id=0,offset=0,bytes=0,access=0;};
using Bindings=std::array<Binding,4>;
inline void word(Bytes &v,unsigned offset,uint32_t value,unsigned bytes=2) {
    for(unsigned n=0;n<bytes;++n)v.at(offset+n)=static_cast<unsigned char>(value>>(8*n));
}
inline Bindings unbound() {Bindings b{};for(unsigned i=0;i<4;++i)b[i].role=i+1;return b;}
inline Layout layoutRoot(const Program &p,const std::vector<Reservation>&external,const Bindings &bindings) {
    auto l=portable::layout(p,external);const unsigned globals=l.bytes-2;
    if(globals+Header>65535)throw std::runtime_error("root exceeds stock block limit");
    Bytes initial(Header+globals,0);std::copy(l.initial.begin(),l.initial.end()-2,initial.begin()+Header);
    for(auto &offset:l.offsets)offset+=Header;
    l.bytes=Header+globals;l.errorOffset=Language;l.initial=initial;
    word(l.initial,0,0x4752);word(l.initial,2,1);word(l.initial,4,Header);word(l.initial,6,l.bytes);
    word(l.initial,8,1,4);word(l.initial,LastEntry,65535);word(l.initial,26,4);
    word(l.initial,36,p.procedures.size());word(l.initial,38,globals);word(l.initial,44,0x52323530,4);
    for(unsigned i=0;i<4;++i) {
        const auto &b=bindings[i];const unsigned at=48+i*16;
        word(l.initial,at,b.role);word(l.initial,at+2,b.id);word(l.initial,at+4,b.offset,4);
        word(l.initial,at+8,b.bytes,4);word(l.initial,at+12,b.access);
    }
    return l;
}
inline unsigned checkCode(unsigned offset) {
    if(offset<2)return 1;
    if(offset<4)return 2;
    if(offset>=48)return 5;
    if((offset>=30&&offset<32)||(offset>=40&&offset<48))return 6;
    return 3;
}
inline std::vector<unsigned> fixedOffsets() {
    std::vector<unsigned> offsets;
    for(unsigned i=0;i<Header;++i)
        if(i<8||(i>=26&&i<28)||(i>=30&&i<32)||i>=36)offsets.push_back(i);
    return offsets;
}
struct Envelope {
    unsigned dispatch=0;
    std::vector<unsigned> bufferedEntries;
};
inline Envelope envelope(Buffered &b,const Program &p,const Layout &l,golem::BufferAllocator &a) {
    // Retain each record once and serialize directly: the root guard can be
    // several KiB, so encoded temporary copies waste scarce on-Agon RAM.
    auto install=[&](unsigned id,const Bytes &bytes){
        if(bytes.empty()||bytes.size()>65535)throw std::runtime_error("invalid block length");
        append(b.install,clear(id));emitWriteBlock(b.install,id,bytes);};
    auto data=[&](Bytes bytes){const auto id=a.allocate("d0.private.data");b.owned.push_back(id);b.data[id]=std::move(bytes);
        install(id,b.data[id]);append(b.cleanup,clear(id));return id;};
    auto program=[&](Bytes code){const auto id=a.allocate("d0.private.code");b.owned.push_back(id);b.programs[id]=std::move(code);
        install(id,b.programs[id]);append(b.cleanup,clear(id));return id;};
    const auto scratch=data({0,0}),operand=data({0,0}),zero=data({0,0});
    std::array<unsigned,9> failure{};
    for(unsigned n=1;n<=8;++n) {Bytes code;emitAdjustMultiLiteral(code,l.buffer,kOpSet,Interface,le16Bytes(n));failure[n]=program(std::move(code));}
    Bytes guard;emitAdjustMultiLiteral(guard,l.buffer,kOpSet,Interface,{0,0});
    for(auto offset:fixedOffsets()) {
        copy(guard,{scratch,0},{l.buffer,offset},1);
        emitAdjustMultiLiteral(guard,operand,kOpSet,0,{l.initial[offset],0});
        emitCondJumpBufferValue(guard,failure[checkCode(offset)],3,scratch,0,operand,0);
    }
    copy(guard,{scratch,0},{l.buffer,8},1);
    for(unsigned n=1;n<4;++n)emitAdjustMultiBufferFetched(guard,scratch,6,0,1,l.buffer,8+n);
    emitCondJumpBufferValue(guard,failure[4],2,scratch,0,zero,0);
    copy(guard,{scratch,0},{l.buffer,Requested+1},1);
    emitAdjustMultiLiteral(guard,operand,kOpSet,0,{static_cast<unsigned char>(p.procedures.size()>>8),0});
    emitCondJumpBufferValue(guard,failure[7],5,scratch,0,operand,0);
    Bytes low;copy(low,{scratch,0},{l.buffer,Requested},1);
    emitAdjustMultiLiteral(low,operand,kOpSet,0,{static_cast<unsigned char>(p.procedures.size()),0});
    emitCondJumpBufferValue(low,failure[7],7,scratch,0,operand,0);
    emitCondCallBufferValue(guard,program(std::move(low)),2,scratch,0,operand,0);
    const auto check=program(std::move(guard)),counter=data({0,0,0,0,0});
    Bytes finished;emitBufferHeader(finished,65535);finished.push_back(7);const auto done=program(std::move(finished));
    std::vector<unsigned> success;
    for(auto entry:b.entries) {
        Bytes code=call(entry);emitCondJumpBufferValue(code,done,3,l.buffer,Language,zero,0);
        copy(code,{counter,0},{l.buffer,Generation},4);
        emitAdjustMultiLiteral(code,counter,kOpAddWithCarry,0,{1,0,0,0});
        copy(code,{l.buffer,Generation},{counter,0},4);
        copy(code,{l.buffer,LastEntry},{l.buffer,Requested});copy(code,{l.buffer,LastSession},{l.buffer,8},4);
        success.push_back(program(std::move(code)));
    }
    Bytes dispatch=call(check);emitCondJumpBufferValue(dispatch,done,3,l.buffer,Interface,zero,0);
    for(unsigned i=0;i<success.size();++i) {
        emitAdjustMultiLiteral(dispatch,operand,kOpSet,0,le16Bytes(i));
        emitCondJumpBufferValue(dispatch,success[i],2,l.buffer,Requested,operand,0);
    }
    Envelope output;output.dispatch=program(std::move(dispatch));
    for(unsigned i=0;i<p.procedures.size();++i) {
        Bytes code;emitAdjustMultiLiteral(code,l.buffer,kOpSet,Requested,le16Bytes(i));
        emitBufferHeader(code,output.dispatch);code.push_back(7);output.bufferedEntries.push_back(program(std::move(code)));
    }
    b.entries=output.bufferedEntries;return output;
}
inline void replaceOne(std::string &text,const std::string&from,const std::string&to) {
    const auto at=text.find(from);
    if(at==std::string::npos||text.find(from,at+from.size())!=std::string::npos)
        throw std::runtime_error("pinned native emitter anchor mismatch");
    text.replace(at,from.size(),to);
}
inline std::string native(const Program&p,const Layout&l,const Bindings&bindings) {
    auto text=dual::nativeC(p,l);
    replaceOne(text,"#define FAIL(n) do {put(state+14,n);return VDPL_E_ARGUMENT;}while(0)",
        "static int32_t finish_views(const vdplet_host_v1*h,vdplet_view_v1*root,vdplet_view_v1*args,int32_t result){\n"
        "int32_t cleanup=0;vdplet_view_v1*views[]={root,args};\n"
        "for(unsigned i=0;i<2;++i){uint32_t token=views[i]->lease;if(!token)continue;views[i]->lease=0;\n"
        "int32_t rc=h->buffer_release(h->context,token);if(rc&&!cleanup)cleanup=rc;}\n"
        "return cleanup?cleanup:result;}\n"
        "#define FAIL(n) do {put(state+14,n);return finish_views(h,&view,&args,VDPL_E_ARGUMENT);}while(0)\n"
        "#define ROOT_FAIL(n,rc) do{put(state+12,n);return finish_views(h,&view,&args,(rc));}while(0)");
    replaceOne(text,"if(view.bytes!="+std::to_string(l.bytes)+")return VDPL_E_BOUNDS;",
        "if(view.bytes!="+std::to_string(l.bytes)+"){return finish_views(h,&view,&args,VDPL_E_BOUNDS);}");
    replaceOne(text,"VDPL_WHOLE_BUFFER,&view);if(rc)return rc;",
        "VDPL_WHOLE_BUFFER,&view);if(rc){return finish_views(h,&view,&args,rc);}");
    std::ostringstream guard;guard << "uint8_t*state=view.data;put(state+12,0);\n";
    const auto offsets=fixedOffsets();
    auto array=[&](const char*name,auto get){guard << "static const uint8_t " << name << "[] __attribute__((aligned(4)))={";for(auto o:offsets)guard << get(o) << ',';guard << "};\n";};
    array("offsets",[](unsigned o){return o;});array("values",[&](unsigned o){return unsigned(l.initial[o]);});array("errors",[](unsigned o){return checkCode(o);});
    guard << "for(unsigned i=0;i<sizeof(offsets);++i)if(state[offsets[i]]!=values[i])ROOT_FAIL(errors[i],VDPL_E_ARGUMENT);\n"
          << "if(!get32(state+8))ROOT_FAIL(4,VDPL_E_ARGUMENT);\n"
          << "if(get(state+28)>=" << p.procedures.size() << "u||get(state+28)!=r->entry_id)ROOT_FAIL(7,VDPL_E_ARGUMENT);\n";
    for(const auto &b:bindings)if(b.id) {
        guard << "{vdplet_view_v1 bound={sizeof(bound),0,0,0,0};rc=h->buffer_acquire(h->context," << b.id << "," << (b.access&2?2:1) << ',' << b.offset << "u," << b.bytes << "u,&bound);\n"
              << "if(rc){ROOT_FAIL(8,rc);}\nrc=h->buffer_release(h->context,bound.lease);if(rc){ROOT_FAIL(8,rc);}}\n";
    }
    for(size_t i=0;i<p.arrays.size();++i) {
        const auto&a=p.arrays[i];
        guard << "{vdplet_view_v1 bound={sizeof(bound),0,0,0,0};rc=h->buffer_acquire(h->context," << l.arrayBuffers[i]
              << ',' << (a.readOnly?1:2) << ",0,VDPL_WHOLE_BUFFER,&bound);\n"
              << "if(rc){ROOT_FAIL(8,rc);}\nuint32_t shape=bound.bytes;rc=h->buffer_release(h->context,bound.lease);\n"
              << "if(rc){ROOT_FAIL(8,rc);}\nif(shape!=" << a.bytes() << "u){ROOT_FAIL(8,VDPL_E_BOUNDS);}}\n";
    }
    guard << "put(state+14,0);\n";
    replaceOne(text,"uint8_t*state=view.data;put(state+14,0);\n",guard.str());
    replaceOne(text,"h->buffer_release(h->context,view.lease);h->buffer_release(h->context,args.lease);return 0;",
        "put32(state+20,get32(state+20)+1u);put(state+24,r->entry_id);put32(state+32,get32(state+8));\n"
        "return finish_views(h,&view,&args,0);");
    return text;
}
inline std::string rootAssembly() {
    std::ostringstream o;
    for(const auto &pair:std::vector<std::pair<std::string,unsigned>>{{"magic",0},{"schema",2},{"header_bytes",4},{"root_bytes",6},{"session",8},{"interface_status",12},{"language_status",14},{"runtime_result",16},{"last_success_generation",20},{"last_success_entry",24},{"descriptor_count",26},{"requested_entry",28},{"last_success_session",32},{"entry_count",36},{"globals_bytes",38},{"contract_tag",44},{"descriptors",48}})
        o << "golem_root_" << pair.first << "_offset: equ " << pair.second << '\n';
    o << "golem_root_header_bytes: equ 112\ngolem_root_schema: equ 1\ngolem_root_descriptor_bytes: equ 16\n";
    return o.str();
}
} // namespace root_v1
