#pragma once
// Bounded stock VDP matrix profile. No portable/native arithmetic semantics.
#include "assembly_output.hpp"
#include <cstring>
#include <limits>

namespace golem::stock_matrix {
using Bytes = resident_scalar::Bytes;
inline void word(Bytes &b, unsigned x) { b.push_back(x & 255); b.push_back((x >> 8) & 255); }
inline void append(Bytes &a, const Bytes &b) { a.insert(a.end(), b.begin(), b.end()); }
inline Bytes command(unsigned id, unsigned op) { Bytes b{23,0,160}; word(b,id); b.push_back(op); return b; }
inline Bytes call(unsigned id) { return command(id,1); }
inline Bytes upload(unsigned id, const Bytes &data) {
    if (data.size() > 65535) throw std::runtime_error("stock matrix block exceeds 65535 bytes");
    Bytes b=command(id,2); append(b,command(id,0)); word(b,data.size()); append(b,data); return b;
}
inline uint32_t bits(double number) {
    float value=static_cast<float>(number);
    if (!std::isfinite(value) || std::fpclassify(value)==FP_SUBNORMAL || (value==0 && number!=0))
        throw std::runtime_error("matrix literal must fit finite normal float32 or zero");
    uint32_t out; std::memcpy(&out,&value,4); return out;
}
inline void dword(Bytes &b, uint32_t n) { word(b,n & 65535); word(b,n >> 16); }
struct Resource {
    enum Kind { Control, Matrix, Vertices, Counter } kind;
    std::string name;
    unsigned id=0, offset=0, count=0;
    uint32_t initial=0;
    unsigned dimension=4, recordBytes=6, format=0xc0;
    std::string record="XYZ16";
    Bytes matrixValues;
    unsigned byteSize() const { return kind==Control ? 4 : kind==Counter ? 2 : kind==Matrix ? dimension*dimension*4 : count*recordBytes; }
    unsigned typeTag() const { return kind==Control ? 3 : kind==Counter ? 1 : kind==Matrix ? (dimension==4 ? 6 : 8) : record=="XYZ16" ? 7 : record=="XYZF32" ? 9 : 10; }
};
struct Offset { unsigned value=0, last=0, stride=0; };
struct Operand { int field=-1; uint32_t literal=0; Offset offset; };
struct Operation {
    std::string name, callee;
    std::vector<unsigned> refs, indices;
    std::vector<Operand> args;
    std::vector<Offset> offsets;
    std::vector<Operation> body;
    unsigned target=0;
};
struct Procedure { std::string name; unsigned id=0; std::vector<Operation> ops; };
struct LoopLayout {
    std::string entry;
    unsigned counter=0, body=0, state=0, start=0, end=0, iterations=0, operations=0, cursors=0, bodyBytes=0, stateBytes=0;
};
struct Result {
    std::vector<LoopLayout> loops;
    std::vector<Resource> resources;
    std::vector<Procedure> entries;
    std::map<unsigned,Bytes> blocks;
    std::set<unsigned> owned;
    unsigned input=0;
    Bytes values, bootstrap, cleanup, invoke, update, setup;
    std::string json, assembly, thinAssembly;
};
inline Result compile(const std::string &source, const std::vector<resident::Reservation> &reservations={}) {
    syntax::Reader r(source);
    Result out;
    std::set<std::string> foldedNames;
    std::map<std::string,unsigned> lookup;
    unsigned matrixCount=0;
    auto name=[&]() {
        auto t=r.name();
        const auto folded=syntax::lower(t.text);
        if (t.text.size()>40 || folded=="matrix" || folded=="borrowed" || folded=="xyz16" ||
            folded=="buffer" || folded=="identity" || !foldedNames.insert(folded).second)
            syntax::fail(t.begin,"duplicate, reserved or overlong matrix-profile name");
        return t.text;
    };
    while (!r.end() && !r.keyword("Sub")) {
        Resource v{};
        if (r.takeKeyword("uint16")) {
            v.kind=Resource::Counter;v.name=name();r.need('=');v.initial=r.uint16();
        } else if (r.takeKeyword("float32")) {
            v.kind=Resource::Control; v.name=name(); r.need('='); v.initial=bits(r.number());
            v.offset=out.values.size(); dword(out.values,v.initial);
            if (out.values.size()>1024) r.error("at most 256 float32 controls are supported");
        } else if (r.takeKeyword("Matrix")) {
            v.kind=Resource::Matrix; r.need('(');
            v.dimension=r.uint16(); r.need(',');
            if (r.uint16()!=v.dimension || (v.dimension!=3 && v.dimension!=4))
                r.error("only Matrix(3,3) and Matrix(4,4) are supported");
            r.need(')'); v.name=name(); r.need('=');
            if (!r.takeKeyword("Identity")) {
                r.needKeyword("matrix_values"); r.need('(');
                for (unsigned i=0;i<v.dimension*v.dimension;++i) {
                    if(i)r.need(',');
                    dword(v.matrixValues,bits(r.number()));
                }
                r.need(')');
            }
            if (++matrixCount>64) r.error("at most 64 matrices are supported");
        } else if (r.takeKeyword("Borrowed")) {
            v.kind=Resource::Vertices;
            if (r.takeKeyword("XYZ16")) {}
            else if (r.takeKeyword("XYZF32")) { v.record="XYZF32";v.recordBytes=12;v.format=0; }
            else if (r.takeKeyword("XY16")) { v.record="XY16";v.recordBytes=4;v.dimension=3; }
            else r.error("expected XYZ16, XYZF32 or XY16");
            v.name=name(); r.need('='); r.needKeyword("Buffer");
            r.need('('); v.id=r.uint16(); r.need(','); v.count=r.uint16(); r.need(')');
            if (!v.id || v.id==65535 || !v.count || v.count>65535/v.recordBytes)
                r.error("borrowed records need ID 1..65534 and a positive count fitting 65535 bytes");
            bool covered=false;
            for (auto span:reservations) if (span.first<=v.id && v.id<=span.last) covered=true;
            if (!covered) r.error("borrowed vertex ID must be covered by --reserve");
            for (auto &old:out.resources) if (old.kind==Resource::Vertices && old.id==v.id)
                r.error("borrowed vertex aliases are not supported");
        } else r.error("expected initialized float32, 3x3/4x4 Matrix, typed Borrowed buffer or Sub");
        lookup.emplace(v.name,out.resources.size()); out.resources.push_back(v);
    }
    auto ref=[&](Resource::Kind kind) {
        auto t=r.identifier(); auto i=lookup.find(t.text);
        if (i==lookup.end() || out.resources[i->second].kind!=kind)
            syntax::fail(t.begin,"unknown name or wrong matrix-profile operand type");
        return i->second;
    };
    int activeCounter=-1;unsigned loopStart=0,loopEnd=0;
    auto offset=[&]() {
        Offset value;value.value=value.last=r.uint16();
        if(r.take('*')) {
            const unsigned stride=value.value;auto counter=r.identifier();
            if(!stride || activeCounter<0 || counter.text!=out.resources[activeCounter].name)
                syntax::fail(counter.begin,"indexed offset requires positive stride and active loop counter");
            unsigned bias=0;if(r.take('+'))bias=r.uint16();
            const uint64_t first=uint64_t(stride)*loopStart+bias,last=uint64_t(stride)*loopEnd+bias;
            if(last>65535)r.error("indexed offset exceeds 16-bit addressing");
            value={unsigned(first),unsigned(last),stride};
        }
        return value;
    };
    auto operand=[&]() {
        Operand arg;
        if (r.peek().kind!=syntax::Kind::Identifier) { arg.literal=bits(r.number());return arg; }
        auto t=r.identifier();
        if (syntax::lower(t.text)=="read_float32" && r.take('(')) {
            auto field=r.identifier();auto i=lookup.find(field.text);
            if(i==lookup.end())syntax::fail(field.begin,"unknown float32 source");
            arg.field=i->second;r.need(',');arg.offset=offset();r.need(')');
            const auto &v=out.resources[arg.field];
            if(v.kind==Resource::Counter || (v.kind==Resource::Vertices && v.record!="XYZF32") ||
                arg.offset.value%4 || (arg.offset.last!=arg.offset.value && arg.offset.stride%4) ||
                arg.offset.last+4>v.byteSize())
                syntax::fail(t.begin,"read_float32 requires an aligned in-bounds float32 field");
        } else {
            auto i=lookup.find(t.text);
            if(i==lookup.end() || out.resources[i->second].kind!=Resource::Control)
                syntax::fail(t.begin,"expected float32 control, literal or read_float32 field");
            arg.field=i->second;
        }
        return arg;
    };
    auto parseOperation=[&](auto &&self)->Operation {
        if(r.takeKeyword("For")) {
            if(activeCounter>=0)r.error("nested stock loops are not supported");
            Operation op;op.name="for";op.refs.push_back(ref(Resource::Counter));
            r.needKeyword("In");r.needKeyword("Range");r.need('(');
            const unsigned begin=r.uint16();r.need(':');const unsigned end=r.uint16();
            if(r.take(':'))if(r.uint16()!=1)r.error("stock loops support only positive unit step");
            r.need(')');if(begin>end)r.error("stock loop range must be ascending and nonempty");
            op.indices={begin,end};activeCounter=op.refs[0];loopStart=begin;loopEnd=end;
            while(!r.keyword("Next")) {
                if(r.end() || r.keyword("EndSub"))r.error("expected Next for stock loop");
                op.body.push_back(self(self));
                if(op.body.size()>1024)r.error("at most 1024 static loop body operations");
            }
            r.pop();
            // Next's optional name must not consume the following function call.
            if(r.peek().kind==syntax::Kind::Identifier && !r.keyword("EndSub") && !r.keyword("For") && !r.keyword("Next")) {
                auto look=r;look.pop();
                if(!look.take('(')) {
                    auto endName=r.identifier();
                    if(endName.text!=out.resources[activeCounter].name)syntax::fail(endName.begin,"Next counter does not match For");
                }
            }
            activeCounter=-1;
            if(uint64_t(end-begin+1)*op.body.size()>65536)r.error("stock loop exceeds 65536 primitive executions");
            return op;
        }
            Operation op;auto operation=r.identifier();op.name=syntax::lower(operation.text);r.need('(');
            if (op.name=="transform_vertices") {
                op.refs.push_back(ref(Resource::Vertices)); r.need(',');
                op.refs.push_back(ref(Resource::Matrix)); r.need(','); op.refs.push_back(ref(Resource::Vertices));
                const auto &dest=out.resources[op.refs[0]], &src=out.resources[op.refs[2]];
                if (dest.id==src.id || dest.count!=src.count || dest.record!=src.record ||
                    out.resources[op.refs[1]].dimension!=src.dimension)
                    r.error("transform needs distinct buffers with matching record types/counts and matrix dimension");
            } else if (op.name=="matrix_multiply") {
                for (unsigned i=0;i<3;++i) { if(i)r.need(','); op.refs.push_back(ref(Resource::Matrix)); }
                for(unsigned at:op.refs)if(out.resources[at].dimension!=out.resources[op.refs[0]].dimension)
                    r.error("matrix multiplication needs matching dimensions");
            } else if (op.name=="matrix_set_value") {
                op.refs.push_back(ref(Resource::Matrix));r.need(',');op.refs.push_back(ref(Resource::Matrix));
                const unsigned dimension=out.resources[op.refs[0]].dimension;
                if(out.resources[op.refs[1]].dimension!=dimension)r.error("matrix set needs matching dimensions");
                for(unsigned i=0;i<2;++i) {r.need(',');auto index=r.uint16();if(index>=dimension)r.error("matrix index outside dimensions");op.indices.push_back(index);}
                r.need(',');op.args.push_back(operand());
            } else if (op.name=="copy_bytes") {
                op.refs.push_back(ref(Resource::Vertices));r.need(',');op.offsets.push_back(offset());r.need(',');
                op.refs.push_back(ref(Resource::Vertices));r.need(',');op.offsets.push_back(offset());r.need(',');op.indices.push_back(r.uint16());
                const auto &dest=out.resources[op.refs[0]],&src=out.resources[op.refs[1]];
                const auto &d=op.offsets[0],&from=op.offsets[1];const unsigned n=op.indices[0];
                if(!n || d.last+n>dest.byteSize() || from.last+n>src.byteSize())r.error("copy span outside declared borrowed storage");
                if(dest.id==src.id && (d.stride || from.stride))r.error("indexed self-copy is not supported");
                if(dest.id==src.id && d.value!=from.value && d.value<from.value+n && from.value<d.value+n)
                    r.error("overlapping self-copy is not supported");
            } else if (op.name=="matrix_identity" || op.name=="matrix_rotate" ||
                       op.name=="matrix_scale" || op.name=="matrix_translate" || op.name=="matrix_invert") {
                op.refs.push_back(ref(Resource::Matrix));
                const unsigned dimension=out.resources[op.refs[0]].dimension;
                if(op.name=="matrix_invert" && dimension!=3)r.error("only 3x3 inversion is supported");
                unsigned arguments=op.name=="matrix_identity" || op.name=="matrix_invert" ? 0 :
                    op.name=="matrix_rotate" && dimension==3 ? 1 : dimension-1;
                for (unsigned i=0;i<arguments;++i) {r.need(',');op.args.push_back(operand());}
            } else {op.name="call";op.callee=operation.text;}

        if(op.name=="call" && activeCounter>=0)r.error("Sub calls inside stock loops are not supported");
        r.need(')');return op;
    };
    while (!r.end()) {
        r.needKeyword("Sub"); Procedure p;p.name=name();unsigned sourceOperations=0;
        while(!r.keyword("EndSub")) {
            if(r.end())r.error("expected EndSub");
            auto op=parseOperation(parseOperation);
            sourceOperations+=op.name=="for" ? op.body.size() : 1;
            p.ops.push_back(std::move(op));
            if(sourceOperations>1024 || p.ops.size()>1024)r.error("at most 1024 static operations per entry");
        }
        r.pop();if(p.ops.empty())r.error("stock matrix entry must not be empty");
        out.entries.push_back(std::move(p));
        if(out.entries.size()>64)r.error("at most 64 entries");
    }
    if (!matrixCount || out.entries.empty()) throw std::runtime_error("stock matrix profile needs a matrix and a finite Sub");
    std::map<std::string,unsigned> procedures;
    for(unsigned i=0;i<out.entries.size();++i)procedures.emplace(out.entries[i].name,i);
    for(auto &p:out.entries)for(auto &op:p.ops)if(op.name=="call") {
        auto target=procedures.find(op.callee);
        if(target==procedures.end())throw std::runtime_error("unknown stock Sub: "+op.callee);
        op.target=target->second;
    }
    std::vector<unsigned> visited(out.entries.size()),depths(out.entries.size());
    auto visit=[&](auto &&self,unsigned i)->unsigned {
        if(visited[i]==1)throw std::runtime_error("recursive stock Subs are not supported");
        if(visited[i]==2)return depths[i];
        visited[i]=1;unsigned depth=1;
        for(const auto &op:out.entries[i].ops)if(op.name=="call")depth=std::max(depth,1+self(self,op.target));
        if(depth>4)throw std::runtime_error("stock Sub call depth exceeds four");
        visited[i]=2;return depths[i]=depth;
    };
    for(unsigned i=0;i<out.entries.size();++i)visit(visit,i);
    BufferAllocator allocator;
    for (size_t i=0;i<reservations.size();++i)
        allocator.reserve(reservations[i].first,reservations[i].last,"external."+std::to_string(i));
    auto allocate=[&](const std::string &role) { unsigned id=allocator.allocate(role); out.owned.insert(id); return id; };
    if (!out.values.empty()) out.input=allocate("matrix.controls");
    for (auto &v:out.resources) {
        if (v.kind==Resource::Control) v.id=out.input;
        if (v.kind==Resource::Matrix) v.id=allocate("matrix."+v.name);
        if (v.kind==Resource::Counter) {
            v.id=allocate("matrix.counter."+v.name);Bytes data;word(data,v.initial);data.push_back(0);
            out.blocks.emplace(v.id,data); // The third byte is add-with-carry output.
        }
    }
    for (auto &p:out.entries) p.id=allocate("matrix.entry."+p.name);
    struct Patch { unsigned at=0,base=0;Offset offset; };
    struct LoopEmission { unsigned body=0,state=0;Bytes initial;std::vector<std::pair<unsigned,unsigned>> cursors; };
    auto emitOps=[&](auto &&self,const std::vector<Operation> &operations,LoopEmission *loop,const std::string &entry)->Bytes {
        Bytes code;
        auto instruction=[&](const Bytes &bytes,const std::vector<Patch> &patches=std::vector<Patch>{}) {
            const unsigned start=unsigned(code.size()+15*patches.size());
            for(const auto &patch:patches) {
                if(!loop)throw std::runtime_error("indexed operand outside emitted loop");
                const unsigned cursor=loop->initial.size();
                word(loop->initial,patch.base+patch.offset.value);loop->initial.push_back(0);
                loop->cursors.push_back({cursor,patch.offset.stride});
                append(code,command(loop->body,5));code.push_back(0xe2);
                word(code,start+patch.at);word(code,2);word(code,loop->state);word(code,cursor);
            }
            append(code,bytes);
        };
        for(const auto &op:operations) {
            auto id=[&](unsigned at) {return out.resources[op.refs[at]].id;};
            auto fetched=[&](Bytes &bytes,const Operand &arg,std::vector<Patch> &patches) {
                const auto &field=out.resources[arg.field];bytes.push_back(0);word(bytes,field.id);
                if(arg.offset.stride)patches.push_back({unsigned(bytes.size()),field.offset,arg.offset});
                word(bytes,field.offset+arg.offset.value);
            };
            if(op.name=="for") {
                LoopEmission nested;nested.body=allocate("matrix.loop.body");nested.state=allocate("matrix.loop.state");
                word(nested.initial,op.indices[1]&255);word(nested.initial,op.indices[1]>>8);word(nested.initial,0);
                Bytes body=self(self,op.body,&nested,entry);
                // Stock bufferConditional initializes an int32 to -1, then reads
                // only two bytes. A 0xffff word remains the error sentinel.
                // Compare ZERO-EXTENDED bytes instead, including at index65535.
                const unsigned skip=body.size();append(body,command(nested.body,10));
                body.insert(body.end(),{0,0,0});body.push_back(0xa3);
                word(body,id(0));word(body,1);word(body,nested.state);word(body,2);
                resident_scalar::copy(body,nested.state,4,id(0),0,1);
                append(body,command(65535,8));body.push_back(0xa2);
                word(body,nested.state);word(body,4);word(body,nested.state);word(body,0);
                const unsigned advance=body.size();
                for(unsigned byte=0;byte<3;++byte)body[skip+6+byte]=(advance>>(8*byte))&255;
                auto increment=[&](unsigned target,unsigned at,unsigned amount) {
                    append(body,command(target,5));body.push_back(0xc4);word(body,at);word(body,2);word(body,amount);
                };
                increment(id(0),0,1);
                for(auto cursor:nested.cursors)increment(nested.state,cursor.first,cursor.second);
                // The pre-increment byte probes already exclude the endpoint.
                // This conditional backedge also preserves the admitted high bound.
                append(body,command(nested.body,8));body.push_back(0xa6);
                word(body,id(0));word(body,1);word(body,nested.state);word(body,2);
                if(body.size()>65535)throw std::runtime_error("stock loop body exceeds a single block");
                out.blocks.emplace(nested.body,body);out.blocks.emplace(nested.state,nested.initial);
                Bytes counter;word(counter,op.indices[0]);counter.push_back(0);
                resident_scalar::literal(code,id(0),2,0,counter);
                resident_scalar::literal(code,nested.state,2,0,nested.initial);
                append(code,call(nested.body));
                LoopLayout layout;layout.entry=entry;layout.counter=id(0);layout.body=nested.body;layout.state=nested.state;
                layout.start=op.indices[0];layout.end=op.indices[1];layout.iterations=layout.end-layout.start+1;
                layout.operations=op.body.size();layout.cursors=nested.cursors.size();layout.bodyBytes=body.size();layout.stateBytes=nested.initial.size();out.loops.push_back(layout);
            } else if(op.name=="call") {
                instruction(call(out.entries[op.target].id));
            } else if(op.name=="copy_bytes") {
                Bytes bytes=command(id(0),5);bytes.push_back(0xe2);std::vector<Patch> patches;
                if(op.offsets[0].stride)patches.push_back({unsigned(bytes.size()),0,op.offsets[0]});
                word(bytes,op.offsets[0].value);word(bytes,op.indices[0]);word(bytes,id(1));
                if(op.offsets[1].stride)patches.push_back({unsigned(bytes.size()),0,op.offsets[1]});
                word(bytes,op.offsets[1].value);instruction(bytes,patches);
            } else if(op.name=="matrix_set_value") {
                const auto &arg=op.args[0];const unsigned dimension=out.resources[op.refs[0]].dimension;
                Bytes bytes=command(id(0),34);bytes.push_back(arg.field<0 ? 1 : 0x21);
                bytes.push_back(dimension);bytes.push_back(dimension);word(bytes,id(1));
                bytes.push_back(op.indices[0]);bytes.push_back(op.indices[1]);std::vector<Patch> patches;
                if(arg.field<0) {bytes.push_back(0);dword(bytes,arg.literal);}else fetched(bytes,arg,patches);
                instruction(bytes,patches);
            } else if(op.name=="matrix_multiply") {
                const unsigned dimension=out.resources[op.refs[0]].dimension;
                Bytes bytes=command(id(0),34);append(bytes,Bytes{6,static_cast<unsigned char>(dimension),static_cast<unsigned char>(dimension)});
                word(bytes,id(1));word(bytes,id(2));instruction(bytes);
            } else if(op.name=="transform_vertices") {
                Bytes bytes=command(id(0),41);bytes.push_back(8);bytes.push_back(out.resources[op.refs[2]].format);
                word(bytes,id(1));word(bytes,id(2));word(bytes,out.resources[op.refs[0]].count);instruction(bytes);
            } else if(op.name=="matrix_identity" || op.name=="matrix_invert") {
                Bytes bytes=command(id(0),out.resources[op.refs[0]].dimension==3 ? 32 : 33);
                bytes.push_back(op.name=="matrix_identity" ? 0 : 1);instruction(bytes);
            } else {
                const unsigned kind=op.name=="matrix_rotate" ? 2 : op.name=="matrix_scale" ? 5 : 6;
                Bytes affine=command(id(0),out.resources[op.refs[0]].dimension==3 ? 32 : 33);affine.push_back(kind);affine.push_back(0);
                const unsigned temp=allocate("matrix.arguments");
                for(unsigned i=0;i<op.args.size();++i) {
                    const auto &arg=op.args[i];dword(affine,arg.literal);
                    if(arg.field>=0) {
                        const auto &field=out.resources[arg.field];Bytes bytes=command(temp,5);bytes.push_back(0xe2);
                        word(bytes,8+4*i);word(bytes,4);word(bytes,field.id);std::vector<Patch> patches;
                        if(arg.offset.stride)patches.push_back({unsigned(bytes.size()),field.offset,arg.offset});
                        word(bytes,field.offset+arg.offset.value);instruction(bytes,patches);
                    }
                }
                out.blocks.emplace(temp,affine);instruction(call(temp));
            }
        }
        if(code.size()>65535)throw std::runtime_error("matrix entry exceeds a single block");
        return code;
    };
    for(auto &p:out.entries)out.blocks.emplace(p.id,emitOps(emitOps,p.ops,nullptr,p.name));
    // Matrix IDs have no command blocks: matrix operations create typed storage.
    out.setup={23,0,248,1,0,1,0};
    out.bootstrap=out.setup;
    if (out.input) append(out.bootstrap,upload(out.input,out.values));
    for (const auto &v:out.resources) if(v.kind==Resource::Matrix) {
        Bytes initial=command(v.id,2);
        if(v.matrixValues.empty()) {append(initial,command(v.id,v.dimension==3 ? 32 : 33));initial.push_back(0);}
        else {
            append(initial,command(v.id,34));initial.push_back(0);initial.push_back(v.dimension);initial.push_back(v.dimension);initial.push_back(0);
            append(initial,v.matrixValues);
        }
        append(out.bootstrap,initial);append(out.setup,initial);
    }
    for (const auto &block:out.blocks) append(out.bootstrap,upload(block.first,block.second));
    for (unsigned id:out.owned) append(out.cleanup,command(id,2));
    out.invoke=call(out.entries.front().id);
    if (out.input) {
        out.update=command(out.input,5); out.update.push_back(0xc2); word(out.update,0); word(out.update,out.values.size());
        append(out.update,out.values);
    }
    append(out.update,out.invoke);
    std::ostringstream json;
    json << "{\"schema\":1,\"profile\":\"stock-matrix-v1\",\"target\":\"stock-buffered\",\"input\":{\"id\":"
         << out.input << ",\"bytes\":" << out.values.size() << "},\"globals\":[";
    bool first=true;
    for (auto &v:out.resources) if(v.kind==Resource::Control) {
        if(!first)json<<',';
        first=false;
        json<<"{\"name\":\""<<v.name<<"\",\"type\":\"float32\",\"buffer\":"<<v.id<<",\"offset\":"<<v.offset<<",\"bytes\":4}";
    }
    if(std::any_of(out.resources.begin(),out.resources.end(),[](const Resource &v){return v.kind==Resource::Counter;})) {
        json<<"],\"counters\":[";first=true;
        for(const auto &v:out.resources)if(v.kind==Resource::Counter) {
            if(!first)json<<',';
            first=false;
            json<<"{\"name\":\""<<v.name<<"\",\"type\":\"uint16\",\"buffer\":"<<v.id<<",\"offset\":0,\"bytes\":2,\"storage_bytes\":3}";
        }
    }
    json<<"],\"matrices\":["; first=true;
    for(auto &v:out.resources) if(v.kind==Resource::Matrix) {
        if(!first)json<<',';
        first=false;
        json<<"{\"name\":\""<<v.name<<"\",\"id\":"<<v.id<<",\"rows\":"<<v.dimension<<",\"columns\":"<<v.dimension<<",\"bytes\":"<<v.byteSize()<<"}";
    }
    json<<"],\"borrowed\":["; first=true;
    for(auto &v:out.resources) if(v.kind==Resource::Vertices) {
        if(!first)json<<',';
        first=false;
        json<<"{\"name\":\""<<v.name<<"\",\"id\":"<<v.id<<",\"type\":\""<<v.record<<"\",\"offset\":0,\"count\":"<<v.count<<",\"bytes\":"<<v.byteSize()<<"}";
    }
    json<<"],\"entries\":["; first=true;
    std::vector<assembly::Entry> exports;
    for(auto &p:out.entries) {
        if(!first)json<<',';
        first=false;
        json<<"{\"name\":\""<<p.name<<"\",\"id\":"<<p.id<<",\"bytes\":"<<out.blocks.at(p.id).size()<<"}";
        exports.push_back({p.name,p.id,call(p.id)});
    }
    if(!out.loops.empty()) {
        json<<"],\"loops\":[";first=true;
        for(const auto &loop:out.loops) {
            if(!first)json<<',';
            first=false;
            json<<"{\"entry\":\""<<loop.entry<<"\",\"counter_buffer\":"<<loop.counter<<",\"body_buffer\":"<<loop.body
                <<",\"state_buffer\":"<<loop.state<<",\"start\":"<<loop.start<<",\"end\":"<<loop.end<<",\"iterations\":"<<loop.iterations
                <<",\"body_operations\":"<<loop.operations<<",\"cursors\":"<<loop.cursors<<",\"body_bytes\":"<<loop.bodyBytes<<",\"state_bytes\":"<<loop.stateBytes<<"}";
        }
    }
    json<<"],\"owned\":["; first=true;
    for(unsigned id:out.owned) { if(!first)json<<',';
            first=false;json<<id; }
    json<<"],\"rotation_units\":\"degrees\",\"completion\":\"submission only; host owns fences\",\"runtime_checks\":false}\n";
    out.json=json.str();
    std::vector<assembly::Export> fields;
    for(auto &v:out.resources) fields.push_back({v.name,v.id,v.byteSize(),v.typeTag(),v.offset});
    out.thinAssembly=assembly::renderPackage(fields,{{"golem_inputs_buffer",out.input},{"golem_inputs_bytes",unsigned(out.values.size())}},
        exports,out.owned.size(),out.bootstrap,out.cleanup,out.update,false);
    out.assembly=assembly::renderPackage(fields,{{"golem_inputs_buffer",out.input},{"golem_inputs_bytes",unsigned(out.values.size())}},
        exports,out.owned.size(),out.bootstrap,out.cleanup,out.update);
    auto at=out.assembly.find("golemc --resident --emit-asm");
    out.assembly.replace(at,std::string("golemc --resident --emit-asm").size(),"golemc --stock-matrix");
    out.assembly+="; Additional type tags: 6=4x4 float32 matrix, 7=packed XYZ16 vertices.\n";
    for(const auto &v:out.resources)if(v.typeTag()>=8) {
        out.assembly+="; Additional stock type tags: 8=3x3 float32 matrix, 9=packed XYZF32, 10=packed XY16.\n";
        break;
    }
    return out;
}
} // namespace golem::stock_matrix
