#include "vdplet_image.h"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <vector>

static void *allocate(uint32_t bytes, bool) { return std::malloc(bytes); }
static void release(void *data) { std::free(data); }

int main(int argc, char **argv) {
    if (argc != 2) return 2;
    std::ifstream input(argv[1], std::ios::binary);
    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(input)), {});
    vdpl::Image image;
    const int status = vdpl::load(bytes.data(), bytes.size(), image, allocate, release, 3);
    std::printf("{\"status\":%d,\"text_bytes\":%u,\"data_bytes\":%u,\"relocations\":%u}\n",
                status, image.text_bytes, image.data_bytes, image.relocation_count);
    vdpl::dispose(image, release);
    return 0;
}
