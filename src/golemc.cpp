// Host compiler CLI: explicit raw payload or resident package output.
#include "assembly_output.hpp"
#include "portable/driver.hpp"
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
        if (argc >= 2 && std::string(argv[1]) == "--portable") return golem::portable_cli::run(argc, argv);
        if (argc >= 2 && std::string(argv[1]) == "--resident") {
            int pos = 2;
            bool emitAssembly = false;
            std::string tableName, tablePath;
            std::vector<golem::resident::Reservation> reservations;
            while (pos < argc && (std::string(argv[pos]) == "--reserve" || std::string(argv[pos]) == "--emit-asm" || std::string(argv[pos]) == "--table-data")) {
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
                if (emitAssembly) assembly = golem::assembly::render(result, update);
                bootstrap = std::move(result.bootstrap); cleanup = std::move(result.cleanup);
                invoke = std::move(result.invoke); ownedCount = result.owned.size();
            }
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
            std::cout << "resident installation " << bootstrap.size() << " bytes; update/call "
                      << update.size() << " bytes; owned buffers " << ownedCount << '\n';
            return 0;
        }
        if (argc != 3 || std::string(argv[1]).compare(0, 2, "--") == 0) {
            std::cerr << "usage: golemc <source> <payload>\n"
                      << "       golemc --resident [--emit-asm] [--table-data NAME=PATH] [--reserve FIRST:LAST ...] <source> <directory> [example-input]\n";
            std::cerr << "       golemc --portable [--reserve FIRST:LAST ...] [--bind ROLE ID OFFSET BYTES ...] [--array-data NAME=PATH ...] <source> <directory>\n";
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
