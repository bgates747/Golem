// Host compiler CLI: explicit raw payload or resident package output.
#include "resident.hpp"
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
        if (argc >= 2 && std::string(argv[1]) == "--resident") {
            int pos = 2;
            std::vector<golem::resident::Reservation> reservations;
            while (pos < argc && std::string(argv[pos]) == "--reserve") {
                if (++pos == argc) throw std::runtime_error("--reserve requires FIRST:LAST");
                reservations.push_back(reservation(argv[pos++]));
            }
            if (argc - pos != 2 && argc - pos != 3) throw std::runtime_error("resident mode requires source, output directory and optional example input");
            auto result = golem::resident::compileResident(readFile(argv[pos]), reservations);
            double value = argc - pos == 3 ? golem::resident::updateValue(argv[pos + 2]) : result.spec.initial;
            auto update = golem::resident_scalar::inputUpdate(result, value);
            update.insert(update.end(), result.invoke.begin(), result.invoke.end());
            auto json = golem::resident::manifest(result, reservations);
            // Semantic/reservation validation completes before output mutation.
            // File publication itself is not atomic if an I/O operation fails.
            const std::filesystem::path output(argv[pos + 1]);
            std::filesystem::create_directories(output);
            write(output / "bootstrap.bin", result.bootstrap);
            write(output / "cleanup.bin", result.cleanup);
            write(output / "invoke.bin", result.invoke);
            write(output / "update-call.bin", update);
            write(output / "interface.json", golem::resident_scalar::Bytes(json.begin(), json.end()));
            std::cout << "resident installation " << result.bootstrap.size() << " bytes; update/call "
                      << update.size() << " bytes; owned buffers " << result.owned.size() << '\n';
            return 0;
        }
        if (argc != 3 || std::string(argv[1]).compare(0, 2, "--") == 0) {
            std::cerr << "usage: golemc <source> <payload>\n"
                      << "       golemc --resident [--reserve FIRST:LAST ...] <source> <directory> [example-input]\n";
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
