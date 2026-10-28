#pragma once
// Promoted from b32c596; portable CLI adaptation, unchanged emission pipeline.
#include "root.hpp"
#include "../load_plan.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>

namespace golem::portable_cli {
inline void writeFile(const std::filesystem::path &path, const std::string &bytes) {
    std::ofstream out(path, std::ios::binary);
    if (!out || !out.write(bytes.data(), static_cast<std::streamsize>(bytes.size())))
        throw std::runtime_error("could not write " + path.string());
    out.close(); if (!out) throw std::runtime_error("could not close " + path.string());
}
inline void writeFile(const std::filesystem::path &path, const dual::Bytes &bytes) {
    writeFile(path, std::string(bytes.begin(), bytes.end()));
}
inline void numbers(std::ostream &out, const std::vector<unsigned> &values) {
    out << '[';
    for (size_t i = 0; i < values.size(); ++i) { if (i) out << ','; out << values[i]; }
    out << ']';
}
inline unsigned number(const std::string &text, uint32_t maximum,
                       const char *label, uint32_t minimum=0) {
    uint64_t value=0;
    if(text.empty())throw std::runtime_error(std::string("empty ")+label);
    for(char c:text) {
        if(c<'0'||c>'9'||unsigned(c-'0')>maximum||value>(uint64_t(maximum)-unsigned(c-'0'))/10u)
            throw std::runtime_error(std::string("invalid decimal ")+label);
        value=value*10u+unsigned(c-'0');
    }
    if(value<minimum||value>maximum)throw std::runtime_error(std::string("out-of-range ")+label);
    return static_cast<unsigned>(value);
}
inline int run(int argc, char **argv) {
    using namespace dual;
    try {
        auto bindings=root_v1::unbound();
        std::set<unsigned> roles,ids;
        std::map<std::string,std::string> arrayFiles;
        std::vector<portable::Reservation> external{{60000,60031}};
        bool nativeOnly=false,bufferedOnly=false,targetSeen=false,emitLoadPlan=false;
        int option=2;
        while(option<argc && std::string(argv[option]).rfind("--",0)==0) {
            const std::string flag=argv[option++];
            if(flag=="--")break;
            if(flag=="--emit-load-plan") {
                if(emitLoadPlan)throw std::runtime_error("duplicate --emit-load-plan");
                emitLoadPlan=true;continue;
            }
            if(flag=="--target") {
                if(targetSeen||option>=argc)throw std::runtime_error("--target requires one selection: both, native or buffered");
                targetSeen=true;const std::string target=argv[option++];
                if(target!="native"&&target!="both"&&target!="buffered")throw std::runtime_error("--target requires both, native or buffered");
                nativeOnly=target=="native";bufferedOnly=target=="buffered";continue;
            }
            if(flag=="--reserve") {
                if(option>=argc)throw std::runtime_error("--reserve requires FIRST:LAST");
                const std::string range=argv[option++];const auto colon=range.find(':');
                if(colon==std::string::npos)throw std::runtime_error("--reserve requires FIRST:LAST");
                const auto first=number(range.substr(0,colon),65534,"reservation endpoint",1);
                const auto last=number(range.substr(colon+1),65534,"reservation endpoint",1);
                if(first>last)throw std::runtime_error("reversed reservation range");
                external.push_back({first,last});continue;
            }
            if(flag=="--array-data") {
                if(option>=argc)throw std::runtime_error("--array-data requires NAME=PATH");
                const std::string mapping=argv[option++];const auto equal=mapping.find('=');
                if(equal==std::string::npos || !equal || equal+1==mapping.size() ||
                   !arrayFiles.emplace(mapping.substr(0,equal),mapping.substr(equal+1)).second)
                    throw std::runtime_error("invalid or duplicate array mapping");
                continue;
            }
            if(flag!="--bind")throw std::runtime_error("unknown portable option: "+flag);
            if(option+4>argc)throw std::runtime_error("--bind requires ROLE ID OFFSET BYTES");
            const auto role=number(argv[option++],4,"binding role",1);
            const auto id=number(argv[option++],65534,"binding buffer",1);
            const auto offset=number(argv[option++],UINT32_MAX,"binding offset");
            const auto bytes=number(argv[option++],UINT32_MAX,"binding bytes",1);
            if(bytes>UINT32_MAX-offset || !roles.insert(role).second || !ids.insert(id).second)
                throw std::runtime_error("invalid or duplicate fixed binding");
            bindings[role-1]={role,id,offset,bytes,role==2?3u:1u};
        }
        if(argc-option!=2)throw std::runtime_error("portable mode requires source and output directory");
        for(const auto &b:bindings)if(b.id)external.push_back({b.id,b.id});
        std::ifstream input(argv[option]); if (!input) throw std::runtime_error("could not read source");
        const std::string source{std::istreambuf_iterator<char>(input), {}};
        auto program = portable::Parser(source).parse();
        const bool math=portable::requiresNativeMath(program);
        if(portable::containsPrint(program)) {
            if(targetSeen&&!bufferedOnly)throw std::runtime_error("print currently requires --target buffered; native output is not supported");
            bufferedOnly=true;
        }
        if(math&&!nativeOnly)throw std::runtime_error("float /, sin, cos and floor currently require --target native; stock math is not qualified");
        const std::filesystem::path output(argv[option+1]);
        if((nativeOnly&&std::filesystem::exists(output/"buffered-install.bin")) ||
           (bufferedOnly&&std::filesystem::exists(output/"native.c")) ||
           (!nativeOnly&&!bufferedOnly&&std::filesystem::exists(output/"targets.json")))
            throw std::runtime_error("target selection changed; use a fresh output directory");
        for(auto&a:program.arrays) {
            const auto supplied=arrayFiles.find(a.name);
            if(supplied!=arrayFiles.end()) {
                if(!a.initial.empty())throw std::runtime_error("array data conflicts with source initializer: "+a.name);
                std::ifstream data(supplied->second,std::ios::binary);if(!data)throw std::runtime_error("could not read array data: "+a.name);
                a.initial=Bytes(std::istreambuf_iterator<char>(data),{});arrayFiles.erase(supplied);
            }
            if(a.initial.size()!=a.bytes())throw std::runtime_error("array requires exact initialized data: "+a.name);
        }
        if(!arrayFiles.empty())throw std::runtime_error("unknown array mapping: "+arrayFiles.begin()->first);
        for (const auto &g : program.globals)
            if (g.type != Type::U16 && g.type != Type::S16 && g.type != Type::Bool && g.type != Type::F32 && !wide(g.type)) throw std::runtime_error("both executable targets currently require uint16 or bool globals");
        if (program.procedures.size() > 65535) throw std::runtime_error("too many native entries");
        const auto layout = root_v1::layoutRoot(program, external,bindings);
        const auto assemblyLayout = portable::assembly(program, layout)+root_v1::rootAssembly();
        golem::BufferAllocator allocator;
        for (size_t i = 0; i < external.size(); ++i) allocator.reserve(external[i].first, external[i].last, "external." + std::to_string(i));
        allocator.reserve(layout.buffer, layout.buffer, "application.shared_state");
        for(size_t i=0;i<program.arrays.size();++i)allocator.reserve(layout.arrayBuffers[i],layout.arrayBuffers[i],"application.array."+program.arrays[i].name);
        Buffered buffered;root_v1::Envelope envelope;
        if(!nativeOnly) {
            buffered=BufferEmitter(program, layout, allocator).emit();
            envelope=root_v1::envelope(buffered,program,layout,allocator);
        }
        std::string native;
        if(!bufferedOnly)native=root_v1::native(program, layout,bindings);
        unsigned arguments=0;
        std::map<unsigned,Bytes> nativeRecords;
        Bytes nativeInstall,nativeCleanup;
        std::vector<unsigned> nativeEntries,nativeOwned;
        if(!bufferedOnly) {
            arguments=allocator.allocate("dual.native.arguments");
            nativeRecords.emplace(arguments,le16Bytes(layout.buffer));
            nativeInstall=write(arguments,le16Bytes(layout.buffer));nativeCleanup=clear(arguments);
            nativeOwned.push_back(arguments);
        }
        for (size_t i = 0; !bufferedOnly && i < program.procedures.size(); ++i) {
            const auto id = allocator.allocate("dual.native.wrapper");
            nativeEntries.push_back(id); nativeOwned.push_back(id);
            Bytes run{23, 0, 0xd8, 12, 0, 1, 2, 0, 0, 0, 0, 0, 0};
            emitLE16(run, static_cast<unsigned>(i)); emitLE16(run, arguments);
            if (run.size() != 17) throw std::logic_error("RUN packet size");
            Bytes request;emitAdjustMultiLiteral(request,layout.buffer,kOpSet,root_v1::Requested,le16Bytes(i));
            append(request,run);
            nativeRecords.emplace(id,request);
            append(nativeInstall, write(id, request)); append(nativeCleanup, clear(id));
        }
        std::ostringstream calls;
        calls << "; Include layout.inc separately. Explicit backend wrappers send six bytes only.\n";
        for (size_t i = 0; i < program.procedures.size(); ++i) {
            std::vector<std::pair<std::string,unsigned>> choices;
            if(!nativeOnly)choices.push_back({"buffered",buffered.entries[i]});
            if(!bufferedOnly)choices.push_back({"native",nativeEntries[i]});
            for (const auto &choice : choices) {
                const auto stem = "golem_" + choice.first + "_" + program.procedures[i].name;
                if (stem.size() + 5 > 64) throw std::runtime_error("assembly wrapper name too long");
                calls << stem << ":\n ld hl," << stem << "_call\n ld bc,6\n rst.lil $18\n ret\n"
                      << stem << "_call: db 23,0,160," << (choice.second & 255) << ',' << (choice.second >> 8) << ",1\n";
            }
        }
        std::ostringstream meta;
        meta << "{\"shared_state\":" << layout.buffer << ",\"state_bytes\":" << layout.bytes
             << ",\"error_offset\":" << layout.errorOffset << ",\"root_schema\":1,\"root_header_bytes\":112,\"native_handle_offset\":24,\"buffered_dispatch\":" << envelope.dispatch << ",\"native_arguments\":" << arguments
             << ",\"buffered_entries\":"; numbers(meta, buffered.entries);
        meta << ",\"native_wrappers\":"; numbers(meta, nativeEntries);
        meta << ",\"buffered_owned\":"; numbers(meta, buffered.owned);
        meta << ",\"native_owned\":"; numbers(meta, nativeOwned);
        meta << ",\"programs\":{";
        size_t index = 0;
        for (const auto &item : buffered.programs) {
            if (index++) meta << ',';
            meta << '"' << item.first << "\":";
            numbers(meta, std::vector<unsigned>(item.second.begin(), item.second.end()));
        }
        meta << '}';
        if(bufferedOnly)meta << ",\"available_backends\":[\"buffered\"]";
        if(nativeOnly)meta << ",\"available_backends\":[\"native\"],\"required_module_libraries\":" << (math?"[\"m\",\"gcc\"]":"[]");
        meta << "}\n";
        golem::loading::Plan loadPlan;
        if(emitLoadPlan) {
            loadPlan.profile="portable";loadPlan.symbols=assemblyLayout+calls.str();loadPlan.nativeHandleOffset=24;
            for(size_t i=0;i<program.globals.size();++i)loadPlan.fields.push_back({program.globals[i].name,layout.buffer,layout.offsets[i],width(program.globals[i].type)});
            for(size_t i=0;i<program.arrays.size();++i)loadPlan.fields.push_back({program.arrays[i].name,layout.arrayBuffers[i],0,program.arrays[i].bytes()});
            for(auto span:external)loadPlan.reservations.push_back({span.first,span.last});
            loadPlan.common.emplace(layout.buffer,layout.initial);loadPlan.commonOwned.push_back(layout.buffer);
            for(size_t i=0;i<program.arrays.size();++i){loadPlan.common.emplace(layout.arrayBuffers[i],program.arrays[i].initial);loadPlan.commonOwned.push_back(layout.arrayBuffers[i]);}
            if(!nativeOnly) {
                golem::loading::Component c;c.name="buffered";c.entries=buffered.entries;c.resources=buffered.owned;
                for(const auto&p:program.procedures)c.entryNames.push_back(p.name);
                c.records=buffered.data;
                for(const auto&p:buffered.ordinaryData)if(!c.records.emplace(p).second)throw std::logic_error("duplicate buffered load data");
                for(const auto&p:buffered.programs)if(!c.records.emplace(p).second)throw std::logic_error("duplicate buffered load program");
                c.setup={23,0,248,1,0,1,0};loadPlan.components.push_back(c);
            }
            if(!bufferedOnly) {
            golem::loading::Component c;c.name="native";c.entries=nativeEntries;c.resources=nativeOwned;c.records=nativeRecords;
            for(const auto&p:program.procedures)c.entryNames.push_back(p.name);
            loadPlan.components.push_back(c);
            }
            loadPlan.finalize();
        }
        std::filesystem::create_directories(output);
        auto interface = portable::interfaceJson(program, layout);
        const std::string oldContract = "host_reference_only";
        interface.replace(interface.find(oldContract), oldContract.size(), bufferedOnly?"r25-g0-root-v1-buffered":"r25-g0-root-v1-dual-backend");
        std::ostringstream root;
        root << "{\"family\":\"R25-G0/root-v1\",\"schema\":1,\"header_bytes\":112,\"root_bytes\":" << layout.bytes
             << ",\"globals_bytes\":" << layout.bytes-112 << ",\"byte_order\":\"little\",\"argument_bytes\":2,\"fields\":{";
        bool first=true;
        for(const auto &f:std::vector<std::pair<std::string,unsigned>>{{"magic",0},{"schema",2},{"header_bytes",4},{"root_bytes",6},{"session",8},{"interface_status",12},{"language_status",14},{"runtime_result",16},{"last_success_generation",20},{"last_success_entry",24},{"descriptor_count",26},{"requested_entry",28},{"last_success_session",32},{"entry_count",36},{"globals_bytes",38},{"contract_tag",44},{"descriptors",48}}) {
            if(!first)root << ',';
            first=false;root << '\"' << f.first << "\":" << f.second;
        }
        root << "},\"bindings\":[";
        for(unsigned i=0;i<4;++i) {if(i)root << ',';const auto &b=bindings[i];root << "{\"role\":" << b.role << ",\"buffer\":" << b.id << ",\"offset\":" << b.offset << ",\"bytes\":" << b.bytes << ",\"access\":" << b.access << ",\"installed_min_bytes\":" << b.offset+b.bytes << '}';}
        root << "],\"runtime_result_owner\":\"invocation_transport_owner\",\"generation\":\"successful-invocation-modulo-2^32\",\"stock_storage_admission\":\"host-installed-manifest-invariant\",\"native_storage_admission\":\"actual-root-and-bound-range-leases\"}\n";
        writeFile(output/"root.json",root.str());
        writeFile(output/"interface.json", interface);
        writeFile(output/"program.json", portable::programJson(program));
        writeFile(output/"state.bin", layout.initial);
        writeFile(output/"layout.inc", assemblyLayout);
        Bytes arrayInstall;
        for(size_t i=0;i<program.arrays.size();++i) {
            const auto&a=program.arrays[i];writeFile(output/("array-"+a.name+".bin"),a.initial);
            append(arrayInstall,write(layout.arrayBuffers[i],a.initial));
        }
        Bytes common=write(layout.buffer,layout.initial);append(common,arrayInstall);
        writeFile(output/"common-install.bin",common);
        if(!program.arrays.empty())writeFile(output/"arrays-install.bin",arrayInstall);
        if(!nativeOnly) {
            writeFile(output/"buffered-install.bin", buffered.install);
            writeFile(output/"buffered-cleanup.bin", buffered.cleanup);
        } else writeFile(output/"targets.json",std::string("{\"selection\":\"native\",\"available_backends\":[\"native\"],\"requires_math_libraries\":")+(math?"true":"false")+"}\n");
        if(bufferedOnly)writeFile(output/"targets.json","{\"selection\":\"buffered\",\"available_backends\":[\"buffered\"]}\n");
        if(!bufferedOnly) {
            writeFile(output/"native-install.bin", nativeInstall);
            writeFile(output/"native-cleanup.bin", nativeCleanup);
            writeFile(output/"native.c", native);
        }
        writeFile(output/"calls.inc", calls.str());
        writeFile(output/"backends.json", meta.str());
        if(emitLoadPlan)loadPlan.write(output);
        std::cout << "Shared state " << layout.bytes << " bytes; ";
        if(nativeOnly)std::cout << "native-only package; ";
        else std::cout << "buffered install " << buffered.install.size() << " bytes; ";
        std::cout << program.procedures.size() << (bufferedOnly?" buffered entries.\n":" native entries.\n");
        return 0;
    } catch (const std::exception &error) { std::cerr << "compiler: " << error.what() << '\n'; return 1; }
}

} // namespace golem::portable_cli
