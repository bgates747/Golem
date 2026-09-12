// Stock MOS transport for resident lookup/lifecycle qualification.
// No tested index selection or arithmetic is performed on eZ80.
#include <agon/mos.h>
#include <agon/vdp.h>
#include <stdint.h>
#include <stdio.h>
namespace {
constexpr unsigned Query = 1999, Foreign = 55555;
uint32_t ticks() {
    uint32_t a, b;
    do {
        a = sys_vars->time;
        b = sys_vars->time;
    } while (a != b);
    return a;
}
void word(uint8_t *p, unsigned n) {
    p[0] = n;
    p[1] = n >> 8;
}
void send(const uint8_t *p, unsigned n) { mos_puts((char *)p, n, 0); }
void call(unsigned id) {
    uint8_t c[] = {23, 0, 160, 0, 0, 1};
    word(c + 3, id);
    send(c, sizeof(c));
}
bool poll(uint8_t token) {
    auto &gp = reinterpret_cast<volatile uint8_t *>(sys_vars)[0x37];
    gp = token ^ 255;
    const uint8_t c[] = {23, 0, 128, token};
    send(c, sizeof(c));
    uint32_t start = ticks();
    while (gp != token)
        if (ticks() - start > 240)
            return false;
    return true;
}
void adjust(unsigned op, uint8_t value) {
    uint8_t c[] = {23, 0, 160, 0, 0, 5, 0, 3, 0, 0};
    word(c + 3, Query);
    c[6] = op;
    c[9] = value;
    send(c, sizeof(c));
}
bool query(unsigned id, unsigned offset, uint8_t &value) {
    uint8_t parts[2];
    for (unsigned nibble = 0; nibble < 2; ++nibble) {
        // If the source is missing, the failed copy leaves this 0x55 tag.
        adjust(2, 0x55);
        uint8_t c[] = {23, 0, 160, 0, 0, 5, 0xe2, 3, 0, 1, 0, 0, 0, 0, 0};
        word(c + 3, Query);
        word(c + 11, id);
        word(c + 13, offset);
        send(c, sizeof(c));
        adjust(5, nibble ? 0xf0 : 0x0f);
        adjust(6, nibble ? 0x08 : 0x10);
        auto &gp = reinterpret_cast<volatile uint8_t *>(sys_vars)[0x37];
        gp = 0xff;
        call(Query);
        uint32_t start = ticks();
        while (gp == 0xff)
            if (ticks() - start > 240)
                return false;
        parts[nibble] = gp;
        if (nibble ? ((parts[nibble] & 15) != 8) : ((parts[nibble] & 0xf0) != 0x10))
            return false;
    }
    value = (parts[0] & 15) | (parts[1] & 0xf0);
    return true;
}
bool upload() {
    FILE *f = fopen("program.bin", "rb");
    if (!f)
        return false;
    uint8_t block[1024];
    unsigned n, total = 0;
    vdp_adv_clear_buffer(0);
    while ((n = fread(block, 1, sizeof(block), f))) {
        total += n;
        if (total > 65535) {
            fclose(f);
            return false;
        }
        vdp_adv_write_block_data(0, n, (char *)block);
    }
    bool ok = !ferror(f) && total;
    fclose(f);
    if (ok)
        call(0);
    return ok;
}
bool cleanup() {
    FILE *f = fopen("program.clear", "rb");
    if (!f)
        return false;
    // Stream directly: never execute cleanup inside a buffer it clears.
    uint8_t block[1024];
    unsigned n;
    while ((n = fread(block, 1, sizeof(block), f)))
        send(block, n);
    bool ok = !ferror(f);
    fclose(f);
    vdp_adv_clear_buffer(0);
    return ok && poll(0xa5);
}
bool collect(FILE *f, unsigned id, unsigned count) {
    for (unsigned i = 0; i < count; ++i) {
        uint8_t v;
        if (!query(id, i, v) || fwrite(&v, 1, 1, f) != 1)
            return false;
    }
    return true;
}
int run() {
    if (vdp_mode(8) < 0)
        return 1;
    const uint8_t canary[] = {0x34, 0x12}, reply[] = {23, 0, 128, 0};
    vdp_adv_clear_buffer(Foreign);
    vdp_adv_write_block_data(Foreign, 2, (char *)canary);
    vdp_adv_clear_buffer(Query);
    vdp_adv_write_block_data(Query, 4, (char *)reply);
    FILE *out = fopen("results.dat", "wb"), *all = fopen("bytes.dat", "wb"),
         *life = fopen("lifecycle.dat", "wb");
    FILE *owned = fopen("owned.dat", "rb");
    if (!out || !all || !life || !owned)
        return 2;
    unsigned ids[1024], idCount = 0;
    uint8_t raw[2];
    while (fread(raw, 1, 2, owned) == 2) {
        if (idCount == 1024)
            return 3;
        ids[idCount++] = raw[0] + 256u * raw[1];
    }
    if (!idCount || ferror(owned))
        return 4;
    fclose(owned);
    const unsigned indices[] = {0, 3, 1, 2, 4, 65535, 65534, 0};
    for (unsigned cycle = 0; cycle < 3; ++cycle) {
        for (unsigned round = 0; round < 3; ++round) {
            // Reinstall over existing buffers, including after display mode reset.
            if (vdp_mode(0) < 0 || vdp_mode(8) < 0)
                return 5;
            if (!upload() || !poll(0xa5))
                return 6;
            if (!collect(life, Foreign, 2))
                return 7;
            for (unsigned index : indices) {
                uint8_t update[] = {23, 0, 160, 232, 3, 5, 0xc2, 0, 0, 2, 0, 0, 0};
                word(update + 11, index);
                send(update, sizeof(update));
                call(2000);
                if (!poll(0xa5))
                    return 8;
                if (!collect(out, 1500, 8) || !collect(out, 1102, 4) || !collect(out, 1000, 4))
                    return 9;
            }
            if (!collect(all, 4100, 256))
                return 10;
        }
        // First bytes before/after cleanup, including compiler-private IDs.
        for (unsigned i = 0; i < idCount; ++i)
            if (!collect(life, ids[i], 1))
                return 11;
        if (!cleanup())
            return 12;
        for (unsigned i = 0; i < idCount; ++i)
            if (!collect(life, ids[i], 1))
                return 13;
        if (!collect(life, Foreign, 2))
            return 14;
    }
    if (fclose(out) || fclose(all) || fclose(life))
        return 15;
    vdp_adv_clear_buffer(Query);
    vdp_adv_clear_buffer(Foreign);
    if (!poll(0xa5))
        return 16;
    return 0;
}
} // namespace
int main() {
    int result = run();
    vdp_mode(0);
    vdp_cursor_enable(true);
    FILE *f = fopen("exit.txt", "w");
    if (f) {
        fprintf(f, "%d\n", result);
        fclose(f);
    }
    return result;
}
