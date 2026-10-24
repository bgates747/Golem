#pragma once
// Compiler-owned package semantics. AGNB byte framing belongs to agon-utils.
#include "scalar_backend.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <cctype>

namespace golem::loading {
using Bytes=resident_scalar::Bytes;
struct Component {
    std::string name;
    std::map<unsigned,Bytes> records;
    std::vector<unsigned> resources, entries;
    std::vector<std::string> entryNames;
    Bytes setup;
    unsigned setupEntry=0,cleanupEntry=0,moduleBuffer=0;
};
struct Field { std::string name; unsigned buffer,offset,bytes; };
struct Plan {
    std::string profile, symbols;
    std::vector<Component> components;
    std::vector<Field> fields;
    std::map<unsigned,Bytes> common;
    std::vector<unsigned> commonOwned;
    std::vector<std::pair<unsigned,unsigned>> reservations;
    unsigned nativeHandleOffset=0;
    static void append(Bytes &a,const Bytes&b){a.insert(a.end(),b.begin(),b.end());}
    static Bytes command(unsigned id,unsigned op){Bytes b{23,0,160};resident_scalar::le16(b,id);b.push_back(op);return b;}
    static void file(const std::filesystem::path&p,const Bytes&b){
        std::ofstream f(p,std::ios::binary);
        if(!f||!f.write(reinterpret_cast<const char*>(b.data()),b.size()))throw std::runtime_error("could not write load-plan payload: "+p.string());
        f.close();if(!f)throw std::runtime_error("could not close load-plan payload");
    }
    static void text(const std::filesystem::path&p,const std::string&s){file(p,Bytes(s.begin(),s.end()));}
    void finalize() {
        BufferAllocator a;
        for(size_t i=0;i<reservations.size();++i)a.reserve(reservations[i].first,reservations[i].second,"external."+std::to_string(i));
        std::set<std::string> names, labels;
        std::istringstream lines(symbols);std::string line;
        while(std::getline(lines,line)) {
            auto colon=line.find(':');if(colon==std::string::npos)continue;
            auto label=line.substr(0,colon);
            if(label.empty() || !std::isalpha(static_cast<unsigned char>(label[0])))continue;
            for(auto &ch:label)ch=std::tolower(static_cast<unsigned char>(ch));
            labels.insert(label);
        }
        for(const auto &c:components) {
            if((c.name!="stock"&&c.name!="buffered"&&c.name!="native")||!names.insert(c.name).second)
                throw std::runtime_error("invalid load component name");
            for(const auto &suffix:{"setup_buffer","cleanup_buffer","module_buffer","load","filename","setup","setup_call","cleanup","cleanup_call","bind","bind_calls"})
                if(labels.count("golem_agnb_"+c.name+"_"+suffix))throw std::runtime_error("AGNB assembly symbol collision");
        }
        if(components.empty())throw std::runtime_error("empty load plan");
        std::set<unsigned> commonIds(commonOwned.begin(),commonOwned.end());
        std::set<unsigned> claimed;
        for(unsigned id:commonOwned) { if(!claimed.insert(id).second)throw std::runtime_error("duplicate common loading ownership"); }
        for(auto &c:components)for(unsigned id:c.resources)if(!claimed.insert(id).second)throw std::runtime_error("overlapping component ownership");
        for(unsigned id:claimed)a.reserve(id,id,"compiled.resource");
        for(auto &c:components) {
            if(c.name=="native")c.moduleBuffer=a.allocate("loading.module");
            if(!c.setup.empty()) {c.setupEntry=a.allocate("loading.setup");c.records.emplace(c.setupEntry,c.setup);c.resources.push_back(c.setupEntry);}
            c.cleanupEntry=a.allocate("loading.cleanup");
            Bytes cleanup;
            for(unsigned id:commonOwned)append(cleanup,command(id,2));
            for(unsigned id:c.resources)append(cleanup,command(id,2));
            if(c.moduleBuffer)append(cleanup,command(c.moduleBuffer,2));
            if(cleanup.empty())throw std::runtime_error("empty package cleanup");
            c.records.emplace(c.cleanupEntry,cleanup);
            std::set<unsigned> allowed(c.resources.begin(),c.resources.end());allowed.insert(c.cleanupEntry);
            if(!c.entryNames.empty()&&c.entryNames.size()!=c.entries.size())throw std::runtime_error("entry name count mismatch");
            for(unsigned id:c.entries)if(!allowed.count(id))throw std::runtime_error("unowned load entry");
            std::set<unsigned> installed;
            for(const auto &p:common) {
                if(!commonIds.count(p.first)||p.second.empty()||p.second.size()>0xffffffu)throw std::runtime_error("invalid common load record");
                installed.insert(p.first);
            }
            for(const auto &p:c.records) {
                if(!allowed.count(p.first)||!p.first||p.first>=65535||!installed.insert(p.first).second||p.second.empty()||p.second.size()>0xffffffu)
                    throw std::runtime_error("invalid or duplicate load record");
            }
        }
    }
    void write(const std::filesystem::path&out)const {
        std::filesystem::create_directories(out/"load-buffers");
        std::ostringstream json,assembly,header;
        assembly<<symbols<<"\n; AGNB setup/cleanup submit calls; host owns completion and native unload.\n";
        header<<"/* Generated load-plan integration. Load AGNB successfully before setup. */\n#ifndef GOLEM_AGNB_GENERATED_H\n#define GOLEM_AGNB_GENERATED_H\n#include <stdint.h>\n#ifdef __cplusplus\nextern \"C\" {\n#endif\nvoid golem_host_call(uint16_t);\nvoid golem_host_clear(uint16_t);\nvoid golem_host_write16(uint16_t,uint16_t,uint16_t);\nint golem_host_load_agnb(const char*);\n";
        for(const auto &f:fields) {
            const auto stem="GOLEM_FIELD_"+f.name;
            header<<"#define "<<stem<<"_BUFFER "<<f.buffer<<"u\n#define "<<stem<<"_OFFSET "<<f.offset<<"u\n#define "<<stem<<"_BYTES "<<f.bytes<<"u\n";
        }
        json<<"{\"schema\":1,\"profile\":\""<<profile<<"\",\"agnb_version\":[0,3],\"native_handle_offset\":"<<nativeHandleOffset<<",\"components\":[";
        bool firstComponent=true;
        for(const auto &c:components) {
            if(!firstComponent)json<<',';
            firstComponent=false;
            json<<"{\"name\":\""<<c.name<<"\",\"setup_entry\":"<<c.setupEntry<<",\"cleanup_entry\":"<<c.cleanupEntry<<",\"module_buffer\":"<<c.moduleBuffer<<",\"entries\":[";
            for(size_t i=0;i<c.entries.size();++i){if(i)json<<',';json<<c.entries[i];}
            json<<"],\"owned\":[";
            auto owned=commonOwned;owned.insert(owned.end(),c.resources.begin(),c.resources.end());owned.push_back(c.cleanupEntry);if(c.moduleBuffer)owned.push_back(c.moduleBuffer);
            for(size_t i=0;i<owned.size();++i){if(i)json<<',';json<<owned[i];}
            json<<"],\"records\":[";
            bool first=true;
            auto emit=[&](unsigned id,const Bytes&bytes,const std::string&part){
                if(!first)json<<',';
                first=false;
                const auto path="load-buffers/"+part+"-"+std::to_string(id)+".bin";
                file(out/path,bytes);
                json<<"{\"buffer_id\":"<<id<<",\"path\":\""<<path<<"\",\"bytes\":"<<bytes.size()<<"}";
            };
            for(const auto &p:common)emit(p.first,p.second,"common");
            for(const auto &p:c.records)emit(p.first,p.second,c.name);
            json<<"]}";
            const std::string stem="golem_agnb_"+c.name;
            // These symbols are independent of old embedded bootstrap labels.
            assembly<<stem<<"_setup_buffer: equ "<<c.setupEntry<<'\n'<<stem<<"_cleanup_buffer: equ "<<c.cleanupEntry<<'\n'<<stem<<"_module_buffer: equ "<<c.moduleBuffer<<'\n';
            assembly<<stem<<"_load:\n ld de,"<<stem<<"_filename\n call agnb_load_buffers\n ret\n"<<stem<<"_filename: db \"program-"<<c.name<<".agnb\",0\n";
            header<<"static inline int "<<stem<<"_load(void){return golem_host_load_agnb(\"program-"<<c.name<<".agnb\");}\n";
            assembly<<stem<<"_setup:\n";
            if(c.setupEntry)assembly<<" ld hl,"<<stem<<"_setup_call\n ld bc,6\n xor a\n rst.lil $18\n";
            assembly<<" ret\n";
            if(c.setupEntry)assembly<<stem<<"_setup_call: db 23,0,160,"<<(c.setupEntry&255)<<','<<(c.setupEntry>>8)<<",1\n";
            assembly<<stem<<"_cleanup:\n ld hl,"<<stem<<"_cleanup_call\n ld bc,12\n xor a\n rst.lil $18\n ret\n"<<stem<<"_cleanup_call: db 23,0,160,"<<(c.cleanupEntry&255)<<','<<(c.cleanupEntry>>8)<<",1,23,0,160,"<<(c.cleanupEntry&255)<<','<<(c.cleanupEntry>>8)<<",2\n";
            for(size_t i=0;i<c.entryNames.size();++i) {
                const auto entry="golem_"+c.name+"_"+c.entryNames[i];
                header<<"#define GOLEM_ENTRY_"<<c.name<<"_"<<c.entryNames[i]<<" "<<c.entries[i]<<"u\n";
                header<<"static inline void "<<entry<<"(void){golem_host_call("<<c.entries[i]<<"u); }\n";
            }
            if(c.moduleBuffer) {
                header<<"static inline void "<<stem<<"_bind(uint16_t handle){";
                assembly<<stem<<"_bind:\n";
                for(size_t i=0;i<c.entries.size();++i) {
                    header<<"golem_host_write16("<<c.entries[i]<<"u,"<<nativeHandleOffset<<"u,handle);";
                    assembly<<" ld a,l\n ld ("<<stem<<"_bind_value_"<<i<<"),a\n ld a,h\n ld ("<<stem<<"_bind_value_"<<i<<"+1),a\n";
                }
                header<<"}\n";
                assembly<<" ld hl,"<<stem<<"_bind_calls\n ld bc,"<<13*c.entries.size()<<"\n xor a\n rst.lil $18\n ret\n"<<stem<<"_bind_calls:\n";
                for(size_t i=0;i<c.entries.size();++i)
                    assembly<<" db 23,0,160,"<<(c.entries[i]&255)<<','<<(c.entries[i]>>8)<<",5,194,"<<(nativeHandleOffset&255)<<','<<(nativeHandleOffset>>8)<<",2,0\n"<<stem<<"_bind_value_"<<i<<": dw 0\n";
            }
            const auto macro="GOLEM_AGNB_"+c.name;
            header<<"#define "<<macro<<"_SETUP "<<c.setupEntry<<"u\n#define "<<macro<<"_CLEANUP "<<c.cleanupEntry<<"u\n#define "<<macro<<"_MODULE "<<c.moduleBuffer<<"u\n";
            header<<"static inline void "<<stem<<"_setup(void){";
            if(c.setupEntry)header<<"golem_host_call("<<c.setupEntry<<"u);";
            header<<"}\nstatic inline void "<<stem<<"_cleanup(void){golem_host_call("<<c.cleanupEntry<<"u);golem_host_clear("<<c.cleanupEntry<<"u);}\n";
        }
        json<<"]}\n";
        header<<"#ifdef __cplusplus\n}\n#endif\n#endif\n";
        text(out/"load-plan.json",json.str());text(out/"agnb.inc",assembly.str());text(out/"agnb.h",header.str());
    }
};
} // namespace golem::loading
