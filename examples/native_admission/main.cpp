// Stock MOS transport for full state admission qualification.
// No tested index selection or arithmetic is performed on eZ80.
#include <agon/mos.h>
#include <agon/vdp.h>
#include <stdint.h>
#include <stdio.h>
namespace {
constexpr unsigned Query = 1999;
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
        if (total > 1024UL * 1024) {
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
bool collect(FILE *f, unsigned id, unsigned count) {
    for (unsigned i = 0; i < count; ++i) {
        uint8_t v;
        if (!query(id, i, v) || fwrite(&v, 1, 1, f) != 1)
            return false;
    }
    return true;
}
int run() {
    if (vdp_mode(8) < 0 || !upload() || !poll(0xa5))
        return 1;
    const uint8_t reply[] = {23, 0, 128, 0};
    vdp_adv_clear_buffer(Query);
    vdp_adv_write_block_data(Query, 4, (char *)reply);
    FILE *input = fopen("cases.dat", "rb"), *output = fopen("results.dat", "wb");
    if (!input || !output)
        return 2;
    uint8_t header[6];
    if (fread(header, 1, 6, input) != 6 || header[0] != 'G' || header[1] != '1' ||
        header[2] != '9' || header[3] != 'A')
        return 3;
    unsigned count = header[4] + 256u * header[5];
    if (!count || count > 512)
        return 4;
    for (unsigned i = 0; i < count; ++i) {
        uint8_t action[5], data[97];
        if (fread(action, 1, 5, input) != 5)
            return 5;
        unsigned length = action[1] + 256u * action[2], wait = action[3] + 256u * action[4];
        if (length > sizeof(data) || wait > 600 || action[0] > 1)
            return 6;
        if (fread(data, 1, length, input) != length)
            return 7;
        if (action[0] == 1) {
            if (length || !upload())
                return 8;
        } else if (length)
            send(data, length);
        if (wait) {
            uint32_t start = ticks();
            while (ticks() - start < wait) {
            }
        }
        if (!poll(0xa5))
            return 9;
        if (!collect(output, 1001, 80) || !collect(output, 1002, 10) || !collect(output, 1003, 2) ||
            !collect(output, 1102, 4))
            return 10;
    }
    if (fgetc(input) != EOF || ferror(input))
        return 11;
    if (fclose(input) || fclose(output))
        return 12;
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
