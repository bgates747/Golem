/* Generated-C cleanup conformance: deliberately failing SDK callbacks. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sanitizer/asan_interface.h>
#include "vdplet.h"
#include "fixture.h"
#include GENERATED_SOURCE

static uint8_t state[65536], argument_data[32];
static uint8_t *array_data[ARRAY_COUNT];
static uint32_t tokens[4], owners[4], serial, mode;
static uint32_t root_calls, argument_calls, array_calls, order;
static int32_t acquire(void *context, uint16_t id, uint16_t access,
                       uint32_t offset, uint32_t bytes, vdplet_view_v1 *out) {
    (void)context;
    uint8_t *data = NULL; uint32_t size = 0, owner = 0;
    if (id == ARGS_ID) {
        if (mode & 128) return VDPL_E_BUFFER;
        data = argument_data; size = 2; owner = 2;
        if (access != VDPL_READ) return VDPL_E_ARGUMENT;
    } else if (id == STATE_ID) {
        if (mode & 8) return VDPL_E_BUFFER;
        data = state; size = STATE_BYTES - !!(mode & 16); owner = 1;
    } else {
        for (unsigned i = 0; i < ARRAY_COUNT; ++i) if (id == array_ids[i]) {
            data = array_data[i]; size = array_bytes[i]; owner = 3;
            if (access == VDPL_WRITE && array_readonly[i]) return VDPL_E_ARGUMENT;
        }
    }
    if (!data) return VDPL_E_BUFFER;
    if (offset > size) return VDPL_E_BOUNDS;
    if (bytes == VDPL_WHOLE_BUFFER) bytes = size - offset;
    if (bytes > size - offset) return VDPL_E_BOUNDS;
    for (unsigned i = 0; i < 4; ++i) if (!tokens[i]) {
        tokens[i] = ++serial; owners[i] = owner;
        *out = (vdplet_view_v1){sizeof(*out), serial, data + offset, bytes, access};
        return VDPL_OK;
    }
    return VDPL_E_LIMIT;
}
static int32_t release(void *context, uint32_t token) {
    (void)context;
    for (unsigned i = 0; i < 4; ++i) if (token && tokens[i] == token) {
        uint32_t owner = owners[i]; int32_t rc = 0;
        order = order * 10 + owner;
        if (owner == 1) {
            ++root_calls; if (mode & 1) rc = VDPL_E_HANDLE;
            /* Catch any root access after even a refused release attempt. */
            __asan_poison_memory_region(state, STATE_BYTES);
        } else if (owner == 2) {
            ++argument_calls; if (mode & 2) rc = VDPL_E_LIMIT;
            __asan_poison_memory_region(argument_data, sizeof(argument_data));
        } else {
            ++array_calls;
            if (((mode & 32) && array_calls == 1) || ((mode & 64) && array_calls == 2))
                rc = VDPL_E_BUSY;
        }
        if (!rc || !(mode & 4)) tokens[i] = 0;
        return rc;
    }
    /* Duplicate/zero/unacquired cleanup is a conformance failure. */
    fprintf(stderr, "unexpected release of %u\n", token); exit(5);
}
int main(void) {
    for (unsigned i = 0; i < ARRAY_COUNT; ++i) {
        array_data[i] = calloc(array_bytes[i], 1); if (!array_data[i]) return 7;
    }
    argument_data[0] = STATE_ID & 255; argument_data[1] = STATE_ID >> 8;
    vdplet_host_v1 host = {sizeof(host), 1, 0, 1, 0, acquire, release, 0, 0, 0};
    uint8_t head[4];
    while (fread(head, 1, sizeof(head), stdin) == sizeof(head)) {
        if (fread(state, 1, STATE_BYTES, stdin) != STATE_BYTES) return 2;
        for (unsigned i = 0; i < ARRAY_COUNT; ++i)
            if (fread(array_data[i], 1, array_bytes[i], stdin) != array_bytes[i]) return 2;
        mode = head[2] | (uint32_t)head[3] << 8;
        root_calls = argument_calls = array_calls = order = 0;
        vdplet_request_v1 request = {sizeof(request), VDPL_RUN,
            (uint16_t)(head[0] | (uint16_t)head[1] << 8), ARGS_ID, 0, (mode & 256) ? 1 : 2};
        int32_t rc = vdplet_entry(&host, &request);
        __asan_unpoison_memory_region(state, STATE_BYTES);
        __asan_unpoison_memory_region(argument_data, sizeof(argument_data));
        uint32_t remaining = 0;
        for (unsigned i = 0; i < 4; ++i) {
            if (tokens[i]) ++remaining;
            tokens[i] = 0; /* Model the runtime's normal-return lease cleanup. */
        }
        if (remaining && !(mode & 4)) return 4;
        uint32_t result[8] = {(uint32_t)rc, root_calls, argument_calls, array_calls,
                              order, remaining, 0, 0};
        if (fwrite(result, 1, sizeof(result), stdout) != sizeof(result) ||
            fwrite(state, 1, STATE_BYTES, stdout) != STATE_BYTES) return 3;
        for (unsigned i = 0; i < ARRAY_COUNT; ++i)
            if (fwrite(array_data[i], 1, array_bytes[i], stdout) != array_bytes[i]) return 3;
    }
    for (unsigned i = 0; i < ARRAY_COUNT; ++i) free(array_data[i]);
    return 0;
}
