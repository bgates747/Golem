#include "scalar_frontend.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>

namespace {
void write(const std::filesystem::path &path, const scalar::Bytes &bytes) {
    std::ofstream out(path, std::ios::binary);
    if (!out || !out.write(reinterpret_cast<const char *>(bytes.data()), bytes.size()))
        throw scalar::Error("could not write " + path.string());
    out.close();
    if (!out) throw scalar::Error("could not close " + path.string());
}
}
int main(int argc, char **argv) {
    if (argc != 3 && argc != 4) {
        std::cerr << "usage: golemc-scalar <source> <output-directory> [example-update-value]\n";
        return 2;
    }
    try {
        std::ifstream in(argv[1], std::ios::binary);
        if (!in) throw scalar::Error("could not read source");
        std::ostringstream text; text << in.rdbuf();
        if (in.bad()) throw scalar::Error("source read failed");
        auto result = readable::compile(text.str());
        const std::string valueText = argc == 4 ? argv[3] : "";
        double value = argc == 4 ? readable::Parser(valueText).updateValue() : result.spec.initial;
        auto update = scalar::inputUpdate(result, value);
        update.insert(update.end(), result.invoke.begin(), result.invoke.end());
        auto json = readable::manifest(result);
        // Parse, type/range check and generate everything before touching outputs.
        // File I/O errors can still leave a partial output set; no atomic publisher yet.
        const std::filesystem::path output(argv[2]);
        std::filesystem::create_directories(output);
        write(output / "bootstrap.bin", result.bootstrap);
        write(output / "cleanup.bin", result.cleanup);
        write(output / "invoke.bin", result.invoke);
        write(output / "update-call.bin", update);
        write(output / "interface.json", scalar::Bytes(json.begin(), json.end()));
        std::cout << "installed stream " << result.bootstrap.size() << " bytes; update/call "
                  << update.size() << " bytes\n";
    } catch (const std::exception &error) {
        std::cerr << "golemc-scalar: " << error.what() << '\n';
        return 1;
    }
}
