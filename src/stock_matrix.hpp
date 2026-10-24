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
    enum Kind { Control, Matrix, Vertices } kind;
    std::string name;
    unsigned id=0, offset=0, count=0;
    uint32_t initial=0;
};
struct Operand { int field=-1; uint32_t literal=0; };
struct Operation { std::string name; std::vector<unsigned> refs; std::vector<Operand> args; };
struct Procedure { std::string name; unsigned id=0; std::vector<Operation> ops; };
struct Result {
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
        if (r.takeKeyword("float32")) {
            v.kind=Resource::Control; v.name=name(); r.need('='); v.initial=bits(r.number());
            v.offset=out.values.size(); dword(out.values,v.initial);
            if (out.values.size()>1024) r.error("at most 256 float32 controls are supported");
        } else if (r.takeKeyword("Matrix")) {
            v.kind=Resource::Matrix; r.need('(');
            if (r.uint16()!=4) r.error("only Matrix(4,4) is supported");
            r.need(',');
            if (r.uint16()!=4) r.error("only Matrix(4,4) is supported");
            r.need(')');
            v.name=name(); r.need('='); r.needKeyword("Identity");
            if (++matrixCount>64) r.error("at most 64 matrices are supported");
        } else if (r.takeKeyword("Borrowed")) {
            v.kind=Resource::Vertices; r.needKeyword("XYZ16"); v.name=name(); r.need('='); r.needKeyword("Buffer");
            r.need('('); v.id=r.uint16(); r.need(','); v.count=r.uint16(); r.need(')');
            if (!v.id || v.id==65535 || !v.count || v.count>10922) r.error("XYZ16 needs ID 1..65534 and count 1..10922");
            bool covered=false;
            for (auto span:reservations) if (span.first<=v.id && v.id<=span.last) covered=true;
            if (!covered) r.error("borrowed vertex ID must be covered by --reserve");
            for (auto &old:out.resources) if (old.kind==Resource::Vertices && old.id==v.id)
                r.error("borrowed vertex aliases are not supported");
        } else r.error("expected initialized float32, Matrix(4,4), Borrowed XYZ16 or Sub");
        lookup.emplace(v.name,out.resources.size()); out.resources.push_back(v);
    }
    auto ref=[&](Resource::Kind kind) {
        auto t=r.identifier(); auto i=lookup.find(t.text);
        if (i==lookup.end() || out.resources[i->second].kind!=kind)
            syntax::fail(t.begin,"unknown name or wrong matrix-profile operand type");
        return i->second;
    };
    while (!r.end()) {
        r.needKeyword("Sub"); Procedure p; p.name=name();
        while (!r.keyword("EndSub")) {
            if (r.end()) r.error("expected EndSub");
            Operation op; op.name=syntax::lower(r.identifier().text); r.need('(');
            if (op.name=="transform_vertices") {
                op.refs.push_back(ref(Resource::Vertices)); r.need(',');
                op.refs.push_back(ref(Resource::Matrix)); r.need(','); op.refs.push_back(ref(Resource::Vertices));
                const auto &dest=out.resources[op.refs[0]], &src=out.resources[op.refs[2]];
                if (dest.id==src.id || dest.count!=src.count) r.error("transform needs distinct XYZ16 buffers with equal counts");
            } else if (op.name=="matrix_multiply") {
                for (unsigned i=0;i<3;++i) { if(i)r.need(','); op.refs.push_back(ref(Resource::Matrix)); }
            } else if (op.name=="matrix_identity" || op.name=="matrix_rotate" ||
                       op.name=="matrix_scale" || op.name=="matrix_translate") {
                op.refs.push_back(ref(Resource::Matrix));
                if (op.name!="matrix_identity") for (unsigned i=0;i<3;++i) {
                    r.need(','); Operand arg;
                    if (r.peek().kind==syntax::Kind::Identifier) arg.field=ref(Resource::Control);
                    else arg.literal=bits(r.number());
                    op.args.push_back(arg);
                }
            } else r.error("unsupported stock matrix operation");
            r.need(')'); p.ops.push_back(op);
            if (p.ops.size()>1024) r.error("at most 1024 operations per entry");
        }
        r.pop(); if (p.ops.empty()) r.error("stock matrix entry must not be empty");
        out.entries.push_back(p);
        if (out.entries.size()>64) r.error("at most 64 entries");
    }
    if (!matrixCount || out.entries.empty()) throw std::runtime_error("stock matrix profile needs a matrix and a finite Sub");
    BufferAllocator allocator;
    for (size_t i=0;i<reservations.size();++i)
        allocator.reserve(reservations[i].first,reservations[i].last,"external."+std::to_string(i));
    auto allocate=[&](const std::string &role) { unsigned id=allocator.allocate(role); out.owned.insert(id); return id; };
    if (!out.values.empty()) out.input=allocate("matrix.controls");
    for (auto &v:out.resources) {
        if (v.kind==Resource::Control) v.id=out.input;
        if (v.kind==Resource::Matrix) v.id=allocate("matrix."+v.name);
    }
    for (auto &p:out.entries) p.id=allocate("matrix.entry."+p.name);
    for (auto &p:out.entries) {
        Bytes code;
        for (auto &op:p.ops) {
            auto id=[&](unsigned at) { return out.resources[op.refs[at]].id; };
            if (op.name=="matrix_multiply") {
                append(code,command(id(0),34)); append(code,Bytes{6,4,4}); word(code,id(1)); word(code,id(2));
            } else if (op.name=="transform_vertices") {
                append(code,command(id(0),41)); append(code,Bytes{8,0xc0}); word(code,id(1)); word(code,id(2));
                word(code,out.resources[op.refs[0]].count);
            } else if (op.name=="matrix_identity") {
                append(code,command(id(0),33)); code.push_back(0);
            } else {
                const unsigned kind=op.name=="matrix_rotate" ? 2 : op.name=="matrix_scale" ? 5 : 6;
                Bytes instruction=command(id(0),33); instruction.push_back(kind); instruction.push_back(0);
                const unsigned temp=allocate("matrix.arguments");
                for (unsigned i=0;i<3;++i) {
                    const auto &arg=op.args[i]; dword(instruction,arg.literal);
                    if (arg.field>=0) {
                        const auto &field=out.resources[arg.field];
                        append(code,command(temp,5)); code.push_back(0xe2);
                        word(code,8+4*i); word(code,4); word(code,out.input); word(code,field.offset);
                    }
                }
                out.blocks.emplace(temp,instruction); append(code,call(temp));
            }
        }
        if (code.size()>65535) throw std::runtime_error("matrix entry exceeds a single block");
        out.blocks.emplace(p.id,code);
    }
    // Matrix IDs have no command blocks: command33 creates their special typed storage.
    out.setup={23,0,248,1,0,1,0};
    out.bootstrap=out.setup;
    if (out.input) append(out.bootstrap,upload(out.input,out.values));
    for (const auto &v:out.resources) if(v.kind==Resource::Matrix) {
        append(out.bootstrap,command(v.id,2)); append(out.bootstrap,command(v.id,33)); out.bootstrap.push_back(0);
        append(out.setup,command(v.id,2)); append(out.setup,command(v.id,33)); out.setup.push_back(0);
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
    json<<"],\"matrices\":["; first=true;
    for(auto &v:out.resources) if(v.kind==Resource::Matrix) {
        if(!first)json<<',';
        first=false;
        json<<"{\"name\":\""<<v.name<<"\",\"id\":"<<v.id<<",\"rows\":4,\"columns\":4,\"bytes\":64}";
    }
    json<<"],\"borrowed\":["; first=true;
    for(auto &v:out.resources) if(v.kind==Resource::Vertices) {
        if(!first)json<<',';
        first=false;
        json<<"{\"name\":\""<<v.name<<"\",\"id\":"<<v.id<<",\"type\":\"XYZ16\",\"offset\":0,\"count\":"<<v.count<<",\"bytes\":"<<v.count*6<<"}";
    }
    json<<"],\"entries\":["; first=true;
    std::vector<assembly::Entry> exports;
    for(auto &p:out.entries) {
        if(!first)json<<',';
        first=false;
        json<<"{\"name\":\""<<p.name<<"\",\"id\":"<<p.id<<",\"bytes\":"<<out.blocks.at(p.id).size()<<"}";
        exports.push_back({p.name,p.id,call(p.id)});
    }
    json<<"],\"owned\":["; first=true;
    for(unsigned id:out.owned) { if(!first)json<<',';first=false;json<<id; }
    json<<"],\"rotation_units\":\"degrees\",\"completion\":\"submission only; host owns fences\",\"runtime_checks\":false}\n";
    out.json=json.str();
    std::vector<assembly::Export> fields;
    for(auto &v:out.resources) fields.push_back({v.name,v.id,v.kind==Resource::Control?4u:v.kind==Resource::Matrix?64u:v.count*6,
        v.kind==Resource::Control?3u:v.kind==Resource::Matrix?6u:7u,v.offset});
    out.thinAssembly=assembly::renderPackage(fields,{{"golem_inputs_buffer",out.input},{"golem_inputs_bytes",unsigned(out.values.size())}},
        exports,out.owned.size(),out.bootstrap,out.cleanup,out.update,false);
    out.assembly=assembly::renderPackage(fields,{{"golem_inputs_buffer",out.input},{"golem_inputs_bytes",unsigned(out.values.size())}},
        exports,out.owned.size(),out.bootstrap,out.cleanup,out.update);
    auto at=out.assembly.find("golemc --resident --emit-asm");
    out.assembly.replace(at,std::string("golemc --resident --emit-asm").size(),"golemc --stock-matrix");
    out.assembly+="; Additional type tags: 6=4x4 float32 matrix, 7=packed XYZ16 vertices.\n";
    return out;
}
} // namespace golem::stock_matrix
