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
    const uint32_t startupBegin = ticks();
    FILE *mode = fopen("cost.only", "rb");
    const bool costOnly = mode != nullptr;
    if (mode)
        fclose(mode);
    if (vdp_mode(8) < 0)
        return 1;
    vdp_set_pixel_coordinates();
    vdp_cursor_enable(false);
    vdp_gcol(0, 15);
    if (!upload() || !poll(0xa5))
        return 2;
    const uint32_t startupTicks = ticks() - startupBegin;
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
    const unsigned ids[] = {1102, 1103, 1105, 1106, 1501, 1500, 1114, 1115, 1118};
    const unsigned sizes[] = {4, 4, 4, 4, 8, 10, 4, 4, 4}; // 46 bytes including indexed extraction.
    for (unsigned c = 0; c < count; ++c) {
        uint8_t state[24], expected[46], actual[46];
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
        for (unsigned i = 0; i < 9; ++i)
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
    // Guard and loop checks use stock result readback; no tested arithmetic is
    // performed here. Expected bytes only choose a nonmatching poll sentinel.
    FILE *extra = fopen("extra.dat", "wb");
    if (!extra)
        return 12;
    const unsigned tripsExpected[] = {65534, 65535, 6, 118};
    for (unsigned i = 0; i < 4; ++i) {
        call(2010 + i);
        if (!poll(0xa5))
            return 13;
        uint8_t expected[4], actual[4];
        word(expected, tripsExpected[i]);
        word(expected + 2, 12345);
        for (unsigned j = 0; j < 4; ++j)
            if (!query(1500, 6 + j, expected[j], actual[j]))
                return 14;
        if (fwrite(actual, 1, 4, extra) != 4)
            return 15;
    }
    const int32_t badAndGood[] = {0, -1, -2, 21, 65536, 7};
    for (unsigned i = 0; i < 6; ++i) {
        uint32_t value = uint32_t(badAndGood[i]);
        uint8_t update[] = {23, 0, 160, 232, 3, 5, 0xc2, 4, 0, 4, 0, 0, 0, 0, 0};
        word(update + 11, unsigned(value));
        word(update + 13, unsigned(value >> 16));
        send(update, sizeof(update));
        call(2020);
        if (!poll(0xa5))
            return 16;
        uint8_t expected[6] = {0, 0, 0, 0, 64, 64}, actual[6]; // flag 0, previous product 3.0
        if (i == 5) {
            expected[0] = 1;
            expected[4] = 224;
        } // flag 1, new product 7.0
        for (unsigned j = 0; j < 6; ++j)
            if (!query(j < 2 ? 1003 : 1102, j < 2 ? j : j - 2, expected[j], actual[j]))
                return 17;
        if (fwrite(actual, 1, 6, extra) != 6)
            return 18;
    }
    if (fclose(extra))
        return 19;
    // Warmed serial finite-call batches. GP is an end-of-job parser milestone;
    // these intervals include UART command transmission and the final echo.
    FILE *timing = fopen("cost.csv", "w");
    if (!timing)
        return 20;
    fprintf(timing, "kind,calls,ticks,clock_hz,host_command_bytes\n");
    fprintf(timing, "startup,1,%lu,120,0\n", (unsigned long)startupTicks);
    const unsigned kernels[] = {2010, 2011, 2012, 2013, 2000};
    const unsigned batchCalls = costOnly ? 4096 : 256;
    for (unsigned repeat = 0; repeat < 2; ++repeat)
        for (unsigned k = 0; k < 5; ++k) {
            call(kernels[k]);
            if (!poll(0xa5))
                return 21;
            uint32_t start = ticks();
            for (unsigned callIndex = 0; callIndex < batchCalls; ++callIndex)
                call(kernels[k]);
            if (!poll(0xa5))
                return 22;
            fprintf(timing, "%u,%u,%lu,120,%u\n", kernels[k], batchCalls,
                    (unsigned long)(ticks() - start), batchCalls * 6 + 4);
        }
    if (fclose(timing))
        return 23;
    // Final readback catches scratch corruption after all repeated calls.
    FILE *final = fopen("final.dat", "wb");
    if (!final)
        return 24;
    for (unsigned i = 0; i < 9; ++i)
        for (unsigned offset = 0; offset < sizes[i]; ++offset) {
            uint8_t value;
            if (!query(ids[i], offset, 0xa5, value))
                return 25;
            if (fwrite(&value, 1, 1, final) != 1)
                return 26;
        }
    if (fclose(final))
        return 27;
    mark("ready.viz");
    while (!costOnly && !(vdp_getKeyMap(14) & 1)) {
    }
    vdp_mode(0);
    vdp_set_logical_coordinates();
    vdp_cursor_enable(true);
    mark("exit.viz");
    return 0;
}
