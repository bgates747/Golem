#include "backends.hpp"
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
        if (argc != 3 && argc != 4) throw std::runtime_error("usage: compiler source output [reserve-prefix-last]");
        const unsigned prefix = argc == 4 ? static_cast<unsigned>(std::stoul(argv[3])) : 0;
        std::vector<portable::Reservation> external{{60000, 60031}};
        if (prefix) external.push_back({1, prefix});
        std::ifstream input(argv[1]); if (!input) throw std::runtime_error("could not read source");
        const std::string source{std::istreambuf_iterator<char>(input), {}};
        const auto program = portable::Parser(source).parse();
        for (const auto &g : program.globals)
            if (g.type != Type::U16 && g.type != Type::S16 && g.type != Type::Bool && g.type != Type::F32) throw std::runtime_error("both executable targets currently require uint16 or bool globals");
        if (program.procedures.size() > 65535) throw std::runtime_error("too many native entries");
        const auto layout = portable::layout(program, external);
        const auto assemblyLayout = portable::assembly(program, layout);
        golem::BufferAllocator allocator;
        for (size_t i = 0; i < external.size(); ++i) allocator.reserve(external[i].first, external[i].last, "external." + std::to_string(i));
        allocator.reserve(layout.buffer, layout.buffer, "application.shared_state");
        const auto buffered = BufferEmitter(program, layout, allocator).emit();
        const auto native = nativeC(program, layout);
        const unsigned arguments = allocator.allocate("dual.native.arguments");
        Bytes nativeInstall = write(arguments, le16Bytes(layout.buffer)), nativeCleanup = clear(arguments);
        std::vector<unsigned> nativeEntries, nativeOwned{arguments};
        for (size_t i = 0; i < program.procedures.size(); ++i) {
            const auto id = allocator.allocate("dual.native.wrapper");
            nativeEntries.push_back(id); nativeOwned.push_back(id);
            Bytes run{23, 0, 0xd8, 12, 0, 1, 2, 0, 0, 0, 0, 0, 0};
            emitLE16(run, static_cast<unsigned>(i)); emitLE16(run, arguments);
            if (run.size() != 17) throw std::logic_error("RUN packet size");
            append(nativeInstall, write(id, run)); append(nativeCleanup, clear(id));
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
             << ",\"error_offset\":" << layout.errorOffset << ",\"native_arguments\":" << arguments
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
        interface.replace(interface.find(oldContract), oldContract.size(), "checked_uint16_bool_dual_backend");
        writeFile(output/"interface.json", interface);
        writeFile(output/"program.json", portable::programJson(program));
        writeFile(output/"state.bin", layout.initial);
        writeFile(output/"layout.inc", assemblyLayout);
        writeFile(output/"common-install.bin", write(layout.buffer, layout.initial));
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
