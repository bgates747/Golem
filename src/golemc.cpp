// Host compiler CLI: explicit raw payload or resident package output.
#include "assembly_output.hpp"
#include "portable/driver.hpp"
#include "stock_matrix.hpp"
#include "load_plan.hpp"
#include <filesystem>

namespace {
using namespace golem::ordinary;
}
namespace golem::cli {
unsigned reservationNumber(const std::string &text) {
    if (text.empty()) throw golem::AllocationError("empty reservation endpoint");
    unsigned value = 0;
    for (char c : text) {
        if (c < '0' || c > '9' || value > (65534u - static_cast<unsigned>(c - '0')) / 10u)
            throw golem::AllocationError("reservation endpoints must be decimal IDs in 1..65534");
        value = value * 10u + static_cast<unsigned>(c - '0');
    }
    if (value == 0) throw golem::AllocationError("buffer ID 0 is reserved for the loader root");
    return value;
}
golem::resident::Reservation reservation(const std::string &text) {
    auto colon = text.find(':');
    if (colon == std::string::npos) throw golem::AllocationError("reservation must be FIRST:LAST (inclusive)");
    return {reservationNumber(text.substr(0, colon)), reservationNumber(text.substr(colon + 1))};
}
void write(const std::filesystem::path &path, const golem::resident_scalar::Bytes &bytes) {
    std::ofstream out(path, std::ios::binary);
    if (!out || !out.write(reinterpret_cast<const char *>(bytes.data()), bytes.size()))
        throw std::runtime_error("could not write " + path.string());
    out.close();
    if (!out) throw std::runtime_error("could not close " + path.string());
}
}
int main(int argc, char **argv) {
    using namespace golem::cli;
    try {
        if (argc >= 2 && std::string(argv[1]) == "--stock-matrix") {
            int pos=2; bool assemblySeen=false,emitLoadPlan=false;
            std::vector<golem::resident::Reservation> reservations;
            while(pos<argc && (std::string(argv[pos])=="--reserve" || std::string(argv[pos])=="--emit-asm" || std::string(argv[pos])=="--emit-load-plan")) {
                if(std::string(argv[pos])=="--emit-load-plan") {
                    if(emitLoadPlan)throw std::runtime_error("duplicate --emit-load-plan");
                    emitLoadPlan=true;++pos;continue;
                }
                if(std::string(argv[pos])=="--emit-asm") {
                    if(assemblySeen) throw std::runtime_error("duplicate --emit-asm");
                    assemblySeen=true; ++pos; continue;
                }
                if(++pos==argc) throw std::runtime_error("--reserve requires FIRST:LAST");
                reservations.push_back(reservation(argv[pos++]));
            }
            if(argc-pos!=2) throw std::runtime_error("--stock-matrix requires source and output directory");
            auto result=golem::stock_matrix::compile(readFile(argv[pos]),reservations);
            golem::loading::Plan loadPlan;
            if(emitLoadPlan) {
                loadPlan.profile="stock-matrix";loadPlan.symbols=result.thinAssembly;
                for(auto span:reservations)loadPlan.reservations.push_back({span.first,span.last});
                golem::loading::Component c;c.name="stock";c.records=result.blocks;c.setup=result.setup;
                if(result.input)c.records.emplace(result.input,result.values);
                c.resources.assign(result.owned.begin(),result.owned.end());
                for(auto &e:result.entries){c.entries.push_back(e.id);c.entryNames.push_back(e.name);}
                for(const auto&f:result.resources)loadPlan.fields.push_back({f.name,f.id,f.offset,f.byteSize()});
                loadPlan.components.push_back(c);loadPlan.finalize();
            }
            const std::filesystem::path output(argv[pos+1]);
            std::filesystem::create_directories(output);
            write(output/"bootstrap.bin",result.bootstrap); write(output/"cleanup.bin",result.cleanup);
            write(output/"invoke.bin",result.invoke); write(output/"update-call.bin",result.update);
            write(output/"interface.json",golem::stock_matrix::Bytes(result.json.begin(),result.json.end()));
            write(output/"golem.inc",golem::stock_matrix::Bytes(result.assembly.begin(),result.assembly.end()));
            if(emitLoadPlan)loadPlan.write(output);
            std::cout << "stock matrix installation " << result.bootstrap.size() << " bytes; " << result.entries.size() << " finite entries\n";
            return 0;
        }
        if (argc >= 2 && std::string(argv[1]) == "--portable") return golem::portable_cli::run(argc, argv);
        if (argc >= 2 && std::string(argv[1]) == "--resident") {
            int pos = 2;
            bool emitAssembly = false,emitLoadPlan=false;
            std::string tableName, tablePath;
            std::vector<golem::resident::Reservation> reservations;
            while (pos < argc && (std::string(argv[pos]) == "--reserve" || std::string(argv[pos]) == "--emit-asm" || std::string(argv[pos]) == "--table-data" || std::string(argv[pos]) == "--emit-load-plan")) {
                if(std::string(argv[pos])=="--emit-load-plan") {
                    if(emitLoadPlan)throw std::runtime_error("duplicate --emit-load-plan");
                    emitLoadPlan=true;++pos;continue;
                }
                if (std::string(argv[pos]) == "--emit-asm") {
                    if (emitAssembly) throw std::runtime_error("duplicate --emit-asm option");
                    emitAssembly = true; ++pos; continue;
                }
                if (std::string(argv[pos]) == "--table-data") {
                    if (!tablePath.empty()) throw std::runtime_error("only one --table-data mapping is supported");
                    if (++pos == argc) throw std::runtime_error("--table-data requires NAME=PATH");
                    const std::string mapping = argv[pos++];
                    const auto equals = mapping.find('=');
                    if (equals == std::string::npos || equals == 0 || equals + 1 == mapping.size())
                        throw std::runtime_error("--table-data requires NAME=PATH");
                    tableName = mapping.substr(0,equals); tablePath = mapping.substr(equals+1);
                    continue;
                }
                if (++pos == argc) throw std::runtime_error("--reserve requires FIRST:LAST");
                reservations.push_back(reservation(argv[pos++]));
            }
            if (argc - pos != 2 && argc - pos != 3) throw std::runtime_error("resident mode requires source, output directory and optional example input");
            const auto source = readFile(argv[pos]);
            golem::syntax::Reader mode(source);
            golem::resident_scalar::Bytes bootstrap, cleanup, invoke, update;
            std::string json, assembly;
            size_t ownedCount = 0;
            golem::loading::Plan loadPlan;loadPlan.profile="resident";
            for(auto span:reservations)loadPlan.reservations.push_back({span.first,span.last});
            auto records=[&](const auto&r,const golem::loading::Bytes&setup,const std::string&symbols,const std::vector<unsigned>&entries) {
                golem::loading::Component c;c.name="stock";c.setup=setup;c.entries=entries;
                c.records=r.initial;
                for(const auto&p:r.programs)if(!c.records.emplace(p).second)throw std::logic_error("duplicate resident record");
                for(const auto&p:r.owned)c.resources.push_back(p.first);
                loadPlan.components.push_back(c);loadPlan.symbols=symbols;
            };
            if (mode.keyword("Record")) {
                if (tablePath.empty()) throw std::runtime_error("table profile requires --table-data NAME=PATH at build time");
                const auto raw = readFile(tablePath);
                auto result = golem::resident_table::compile(source, tableName,
                    golem::resident_scalar::Bytes(raw.begin(),raw.end()), reservations);
                unsigned value = result.spec.initialIndex;
                if (argc - pos == 3) {
                    golem::syntax::Reader input(argv[pos+2]); value = input.uint16();
                    if (!input.end()) input.error("unexpected text after uint16 index");
                }
                update = golem::resident_table::inputUpdate(result,value);
                update.insert(update.end(),result.invoke.begin(),result.invoke.end());
                json = golem::resident_table::manifest(result,reservations);
                if(emitLoadPlan) {
                    records(result,{23,0,248,1,0,1,0},golem::assembly::render(result,update,false),{result.entry});
                    loadPlan.components.back().entryNames={result.spec.entry};
                    loadPlan.fields.push_back({result.spec.index,result.index,0,2});
                    loadPlan.fields.push_back({result.spec.output,result.output,0,result.spec.rowBytes()});
                }
                if (emitAssembly) assembly = golem::assembly::render(result,update);
                bootstrap = std::move(result.bootstrap); cleanup = std::move(result.cleanup);
                invoke = std::move(result.invoke); ownedCount = result.owned.size();
            } else if (golem::resident_drawing::profile(source)) {
                if (!tablePath.empty()) throw std::runtime_error("--table-data requires the resident table profile");
                if (argc - pos != 2) throw std::runtime_error("drawing profile does not accept a scalar example input");
                auto result = golem::resident_drawing::compile(source, reservations);
                update = golem::resident_drawing::inputUpdate(result);
                update.insert(update.end(), result.invoke.begin(), result.invoke.end());
                json = golem::resident_drawing::manifest(result, reservations);
                if(emitLoadPlan) {
                    golem::loading::Bytes setup;
                    if(!result.initialBitmapIDs.empty())setup={23,0,248,1,0,1,0};
                    std::vector<unsigned> entries;for(const auto&e:result.entries)entries.push_back(e.id);
                    records(result,setup,golem::assembly::render(result,update,false),entries);
                    for(const auto&e:result.entries)loadPlan.components.back().entryNames.push_back(e.name);
                    for(const auto&g:result.spec.globals)loadPlan.fields.push_back({g.name,result.input,g.offset,g.width()});
                }
                if (emitAssembly) assembly = golem::assembly::render(result, update);
                bootstrap = std::move(result.bootstrap); cleanup = std::move(result.cleanup);
                invoke = std::move(result.invoke); ownedCount = result.owned.size();
            } else {
                if (!tablePath.empty()) throw std::runtime_error("--table-data requires the resident table profile");
                auto result = golem::resident::compileResident(source, reservations);
                double value = argc - pos == 3 ? golem::resident::updateValue(argv[pos + 2]) : result.spec.initial;
                update = golem::resident_scalar::inputUpdate(result, value);
                update.insert(update.end(), result.invoke.begin(), result.invoke.end());
                json = golem::resident::manifest(result, reservations);
                if(emitLoadPlan) {
                    golem::loading::Bytes setup{23,0,248,1,0,1,0};
                    for(const auto &p:result.extras.data)golem::loading::Plan::append(setup,golem::loading::Plan::command(p.first,2));
                    golem::loading::Plan::append(setup,result.extras.initialize);
                    golem::loading::Plan::append(setup,golem::resident_scalar::inputUpdate(result,result.spec.initial));
                    records(result,setup,golem::assembly::render(result,update,false),{result.layout.entry});
                    loadPlan.components.back().entryNames={result.spec.entry};
                    loadPlan.fields.push_back({result.spec.input,result.layout.input,0,4});
                    loadPlan.fields.push_back({result.spec.output,result.layout.output,0,2});
                    for(const auto&g:result.extras.globals)loadPlan.fields.push_back({g.first,static_cast<unsigned>(g.second),0,2});
                }
                if (emitAssembly) assembly = golem::assembly::render(result, update);
                bootstrap = std::move(result.bootstrap); cleanup = std::move(result.cleanup);
                invoke = std::move(result.invoke); ownedCount = result.owned.size();
            }
            if(emitLoadPlan)loadPlan.finalize();
            // Semantic/reservation validation completes before output mutation.
            // File publication itself is not atomic if an I/O operation fails.
            const std::filesystem::path output(argv[pos + 1]);
            std::filesystem::create_directories(output);
            write(output / "bootstrap.bin", bootstrap);
            write(output / "cleanup.bin", cleanup);
            write(output / "invoke.bin", invoke);
            write(output / "update-call.bin", update);
            write(output / "interface.json", golem::resident_scalar::Bytes(json.begin(), json.end()));
            if (emitAssembly) write(output / "golem.inc", golem::resident_scalar::Bytes(assembly.begin(), assembly.end()));
            if(emitLoadPlan)loadPlan.write(output);
            std::cout << "resident installation " << bootstrap.size() << " bytes; update/call "
                      << update.size() << " bytes; owned buffers " << ownedCount << '\n';
            return 0;
        }
        if (argc != 3 || std::string(argv[1]).compare(0, 2, "--") == 0) {
            std::cerr << "usage: golemc <source> <payload>\n"
                      << "       golemc --resident [--emit-asm] [--emit-load-plan] [--table-data NAME=PATH] [--reserve FIRST:LAST ...] <source> <directory> [example-input]\n";
            std::cerr << "       golemc --stock-matrix [--reserve FIRST:LAST ...] [--emit-asm] [--emit-load-plan] <source> <directory>\n";
            std::cerr << "       golemc --portable [--emit-load-plan] [--target both|native] [--reserve FIRST:LAST ...] [--bind ROLE ID OFFSET BYTES ...] [--array-data NAME=PATH ...] <source> <directory>\n";
            return 2;
        }
        auto bytes = compile(readFile(argv[1]));
        write(argv[2], bytes);
        std::cout << "wrote " << bytes.size() << " payload bytes\n";
    } catch (const std::exception &error) {
        std::cerr << "golemc: error: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
