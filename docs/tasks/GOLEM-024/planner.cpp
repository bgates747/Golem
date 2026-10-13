#include "portable.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>

namespace {
unsigned endpoint(const std::string &text) {
    if (text.empty() || text.find_first_not_of("0123456789") != std::string::npos)
        throw std::runtime_error("invalid reservation endpoint");
    const auto number = std::stoul(text);
    if (!number || number >= 65535) throw std::runtime_error("reservation endpoint must be in 1..65534");
    return static_cast<unsigned>(number);
}
void write(const std::filesystem::path &path, const std::string &bytes) {
    std::ofstream out(path, std::ios::binary);
    if (!out || !out.write(bytes.data(), static_cast<std::streamsize>(bytes.size())))
        throw std::runtime_error("could not write " + path.string());
    out.close();
    if (!out) throw std::runtime_error("could not close " + path.string());
}
}
int main(int argc, char **argv) {
    try {
        int position = 1;
        std::string target;
        std::vector<portable::Reservation> reservations;
        while (position < argc && std::string(argv[position]).rfind("--", 0) == 0) {
            const std::string option = argv[position++];
            if (position == argc) throw std::runtime_error("missing option value");
            const std::string value = argv[position++];
            if (option == "--target") {
                if (!target.empty() || (value != "buffered" && value != "vdplet"))
                    throw std::runtime_error("target must be buffered or vdplet, once");
                target = value;
            } else if (option == "--reserve") {
                const auto colon = value.find(':');
                if (colon == std::string::npos) throw std::runtime_error("reservation must be FIRST:LAST");
                reservations.push_back({endpoint(value.substr(0, colon)), endpoint(value.substr(colon + 1))});
            } else throw std::runtime_error("unknown option: " + option);
        }
        if (target.empty() || argc - position != 2)
            throw std::runtime_error("usage: planner --target buffered|vdplet [--reserve FIRST:LAST ...] source output");
        std::ifstream input(argv[position], std::ios::binary);
        if (!input) throw std::runtime_error("could not read source");
        const std::string source{std::istreambuf_iterator<char>(input), {}};
        const auto program = portable::Parser(source).parse();
        const auto layout = portable::layout(program, reservations);
        // Render/validate everything before creating or changing output files.
        const auto interface = portable::interfaceJson(program, layout);
        const auto ir = portable::programJson(program);
        const auto assembly = portable::assembly(program, layout);
        const std::string state(layout.initial.begin(), layout.initial.end());
        const std::string requirements = "{\n  \"target\": \"" + target +
            "\",\n  \"requires_vdplet_runtime\": " + (target == "vdplet" ? "true" : "false") +
            ",\n  \"executable_output\": false,\n  \"phase\": \"shared_frontend_only\",\n"
            "  \"interface\": \"interface.json\",\n  \"target_execution_qualified\": false\n}\n";
        const std::filesystem::path output = argv[position + 1];
        std::filesystem::create_directories(output);
        write(output / "interface.json", interface);
        write(output / "program.json", ir);
        write(output / "state.bin", state);
        write(output / "layout.inc", assembly);
        write(output / "target.json", requirements);
        std::cout << "Shared frontend plan: " << layout.bytes << " state bytes, "
                  << program.procedures.size() << " entries; no executable backend output.\n";
    } catch (const std::exception &error) {
        std::cerr << "planner: " << error.what() << '\n';
        return 1;
    }
}
