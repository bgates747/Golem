// Stock MOS loader/readback for Golem's native arithmetic qualification.
// It uploads opaque compiler output and inputs; it does no tested arithmetic.
#include <agon/mos.h>
#include <agon/vdp.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
namespace {
constexpr unsigned QueryBuffer = 1999;
uint32_t ticks() {
    uint32_t a, b;
    do {
        a = sys_vars->time;
        b = sys_vars->time;
    } while (a != b);
    return a;
}
void word(uint8_t *p, unsigned value) {
    p[0] = value;
    p[1] = value >> 8;
}
void send(const uint8_t *p, unsigned n) { mos_puts((char *)p, n, 0); }
void call(unsigned id) {
    uint8_t c[] = {23, 0, 160, 0, 0, 1};
    word(c + 3, id);
    send(c, sizeof(c));
}
void mark(const char *name) {
    FILE *f = fopen(name, "wb");
    if (f)
        fclose(f);
}
bool poll(uint8_t token) {
    auto &gp = reinterpret_cast<volatile uint8_t *>(sys_vars)[0x37];
    gp = token ^ 255;
    const uint8_t c[] = {23, 0, 128, token};
    send(c, sizeof(c));
    uint32_t start = ticks();
    while (gp != token) {
        if (ticks() - start > 240)
            return false;
    }
    return true;
}
bool query(unsigned id, unsigned offset, uint8_t expected, uint8_t &value) {
    auto &gp = reinterpret_cast<volatile uint8_t *>(sys_vars)[0x37];
    const uint8_t sentinel = expected ^ 255;
    gp = sentinel;
    uint8_t c[] = {23, 0, 160, 0, 0, 5, 0xe2, 3, 0, 1, 0, 0, 0, 0, 0};
    word(c + 3, QueryBuffer);
    word(c + 11, id);
    word(c + 13, offset);
    send(c, sizeof(c));
    call(QueryBuffer);
    uint32_t start = ticks();
    while (gp == sentinel) {
        if (ticks() - start > 240)
            return false;
    }
    value = gp;
    return true;
}
bool upload() {
    FILE *f = fopen("program.bin", "rb");
    if (!f)
        return false;
    vdp_adv_clear_buffer(0);
    uint8_t block[1024];
    unsigned n, total = 0;
    while ((n = fread(block, 1, sizeof(block), f)) != 0) {
        total += n;
        if (total > 65535) {
            fclose(f);
            return false;
        }
        vdp_adv_write_block_data(0, n, (char *)block);
    }
    bool ok = !ferror(f) && total > 0;
    fclose(f);
    if (ok)
        call(0);
    return ok;
}
} // namespace
int main() {
    if (vdp_mode(8) < 0)
        return 1;
    vdp_set_pixel_coordinates();
    vdp_cursor_enable(false);
    vdp_gcol(0, 15);
    if (!upload() || !poll(0xa5))
        return 2;
    const uint8_t reply[] = {23, 0, 128, 0};
    vdp_adv_clear_buffer(QueryBuffer);
    vdp_adv_write_block_data(QueryBuffer, sizeof(reply), (char *)reply);
    FILE *input = fopen("cases.dat", "rb");
    FILE *output = fopen("results.dat", "wb");
    if (!input || !output)
        return 3;
    uint8_t header[6];
    if (fread(header, 1, 6, input) != 6 || memcmp(header, "G19P", 4))
        return 4;
    unsigned count = header[4] | unsigned(header[5]) << 8;
    if (!count || count > 128)
        return 5;
    const unsigned ids[] = {1102, 1103, 1105, 1106, 1501, 1500};
    const unsigned sizes[] = {4, 4, 4, 4, 8, 10}; // 34 bytes, including loop neighbours.
    for (unsigned c = 0; c < count; ++c) {
        uint8_t state[24], expected[34], actual[34];
        if (fread(state, 1, sizeof(state), input) != sizeof(state) ||
            fread(expected, 1, sizeof(expected), input) != sizeof(expected))
            return 6;
        uint8_t update[] = {23, 0, 160, 232, 3, 5, 0xc2, 0, 0, 24, 0};
        send(update, sizeof(update));
        send(state, sizeof(state));
        call(2000);
        if (!poll(0xa5))
            return 7;
        unsigned index = 0;
        for (unsigned i = 0; i < 6; ++i)
            for (unsigned offset = 0; offset < sizes[i]; ++offset) {
                if (!query(ids[i], offset, expected[index], actual[index]))
                    return 8;
                ++index;
            }
        if (fwrite(actual, 1, sizeof(actual), output) != sizeof(actual))
            return 9;
    }
    if (fgetc(input) != EOF)
        return 10;
    fclose(input);
    if (fclose(output))
        return 11;
    mark("ready.viz");
    while (!(vdp_getKeyMap(14) & 1)) {
    }
    vdp_mode(0);
    vdp_set_logical_coordinates();
    vdp_cursor_enable(true);
    mark("exit.viz");
    return 0;
}
