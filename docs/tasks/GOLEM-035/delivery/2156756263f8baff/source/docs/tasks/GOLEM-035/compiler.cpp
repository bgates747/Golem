#include "root.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>

namespace {
void writeFile(const std::filesystem::path &path, const std::string &bytes) {
    std::ofstream out(path, std::ios::binary);
    if (!out || !out.write(bytes.data(), static_cast<std::streamsize>(bytes.size())))
        throw std::runtime_error("could not write " + path.string());
    out.close(); if (!out) throw std::runtime_error("could not close " + path.string());
}
void writeFile(const std::filesystem::path &path, const dual::Bytes &bytes) {
    writeFile(path, std::string(bytes.begin(), bytes.end()));
}
void numbers(std::ostream &out, const std::vector<unsigned> &values) {
    out << '[';
    for (size_t i = 0; i < values.size(); ++i) { if (i) out << ','; out << values[i]; }
    out << ']';
}
}
int main(int argc, char **argv) {
    using namespace dual;
    try {
        if(argc<3)throw std::runtime_error("usage: compiler source output [reserve-prefix-last] [--bind role id offset bytes ...]");
        auto bindings=root_v1::unbound();unsigned prefix=0;int option=3;
        if(option<argc && std::string(argv[option])!="--bind" && std::string(argv[option])!="--array-data")prefix=std::stoul(argv[option++]);
        if(prefix>=65535)throw std::runtime_error("invalid reservation prefix");
        std::set<unsigned> roles,ids;
        std::map<std::string,std::string> arrayFiles;
        while(option<argc) {
            if(std::string(argv[option])=="--array-data") {
                if(++option>=argc)throw std::runtime_error("--array-data requires NAME=PATH");
                const std::string mapping=argv[option++];const auto equal=mapping.find('=');
                if(equal==std::string::npos || !equal || equal+1==mapping.size() || !arrayFiles.emplace(mapping.substr(0,equal),mapping.substr(equal+1)).second)
                    throw std::runtime_error("invalid or duplicate array mapping");
                continue;
            }
            if(std::string(argv[option++])!="--bind" || option+4>argc)throw std::runtime_error("invalid --bind arguments");
            const auto role=std::stoul(argv[option++]),id=std::stoul(argv[option++]),offset=std::stoul(argv[option++]),bytes=std::stoul(argv[option++]);
            if(role<1||role>4||!id||id>=65535||!bytes||offset>UINT32_MAX||bytes>UINT32_MAX||offset+bytes>UINT32_MAX||!roles.insert(role).second||!ids.insert(id).second)
                throw std::runtime_error("invalid or duplicate fixed binding");
            bindings[role-1]={static_cast<unsigned>(role),static_cast<unsigned>(id),static_cast<unsigned>(offset),static_cast<unsigned>(bytes),role==2?3u:1u};
        }
        std::vector<portable::Reservation> external{{60000, 60031}};
        if (prefix) external.push_back({1, prefix});
        for(const auto &b:bindings)if(b.id)external.push_back({b.id,b.id});
        std::ifstream input(argv[1]); if (!input) throw std::runtime_error("could not read source");
        const std::string source{std::istreambuf_iterator<char>(input), {}};
        auto program = portable::Parser(source).parse();
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
            if (g.type != Type::U16 && g.type != Type::S16 && g.type != Type::Bool && g.type != Type::F32) throw std::runtime_error("both executable targets currently require uint16 or bool globals");
        if (program.procedures.size() > 65535) throw std::runtime_error("too many native entries");
        const auto layout = root_v1::layoutRoot(program, external,bindings);
        const auto assemblyLayout = portable::assembly(program, layout)+root_v1::rootAssembly();
        golem::BufferAllocator allocator;
        for (size_t i = 0; i < external.size(); ++i) allocator.reserve(external[i].first, external[i].last, "external." + std::to_string(i));
        allocator.reserve(layout.buffer, layout.buffer, "application.shared_state");
        for(size_t i=0;i<program.arrays.size();++i)allocator.reserve(layout.arrayBuffers[i],layout.arrayBuffers[i],"application.array."+program.arrays[i].name);
        auto buffered = BufferEmitter(program, layout, allocator).emit();
        const auto envelope=root_v1::envelope(buffered,program,layout,allocator);
        const auto native = root_v1::native(program, layout,bindings);
        const unsigned arguments = allocator.allocate("dual.native.arguments");
        Bytes nativeInstall = write(arguments, le16Bytes(layout.buffer)), nativeCleanup = clear(arguments);
        std::vector<unsigned> nativeEntries, nativeOwned{arguments};
        for (size_t i = 0; i < program.procedures.size(); ++i) {
            const auto id = allocator.allocate("dual.native.wrapper");
            nativeEntries.push_back(id); nativeOwned.push_back(id);
            Bytes run{23, 0, 0xd8, 12, 0, 1, 2, 0, 0, 0, 0, 0, 0};
            emitLE16(run, static_cast<unsigned>(i)); emitLE16(run, arguments);
            if (run.size() != 17) throw std::logic_error("RUN packet size");
            Bytes request;emitAdjustMultiLiteral(request,layout.buffer,kOpSet,root_v1::Requested,le16Bytes(i));
            append(request,run);
            append(nativeInstall, write(id, request)); append(nativeCleanup, clear(id));
        }
        std::ostringstream calls;
        calls << "; Include layout.inc separately. Explicit backend wrappers send six bytes only.\n";
        for (size_t i = 0; i < program.procedures.size(); ++i) {
            for (const auto &choice : std::vector<std::pair<std::string, unsigned>>{{"buffered", buffered.entries[i]}, {"native", nativeEntries[i]}}) {
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
        meta << "}}\n";
        const std::filesystem::path output(argv[2]);
        std::filesystem::create_directories(output);
        auto interface = portable::interfaceJson(program, layout);
        const std::string oldContract = "host_reference_only";
        interface.replace(interface.find(oldContract), oldContract.size(), "r25-g0-root-v1-dual-backend");
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
        writeFile(output/"buffered-install.bin", buffered.install);
        writeFile(output/"buffered-cleanup.bin", buffered.cleanup);
        writeFile(output/"native-install.bin", nativeInstall);
        writeFile(output/"native-cleanup.bin", nativeCleanup);
        writeFile(output/"native.c", native);
        writeFile(output/"calls.inc", calls.str());
        writeFile(output/"backends.json", meta.str());
        std::cout << "Shared state " << layout.bytes << " bytes; buffered install " << buffered.install.size()
                  << " bytes; " << program.procedures.size() << " native entries.\n";
    } catch (const std::exception &error) { std::cerr << "compiler: " << error.what() << '\n'; return 1; }
}
