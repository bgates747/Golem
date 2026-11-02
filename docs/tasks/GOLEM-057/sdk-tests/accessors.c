#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* Include the maintained SDK through -I; do not copy its declarations here. */
#include "vdplet.h"

static unsigned checks;

/* Accessor admission only: a callback invocation is an immediate test failure. */
static void unexpected_callback(void) {
    fputs("FAIL: accessor invoked a service callback\n", stderr);
    abort();
}

static int32_t surface_info(void *context, vdplet_surface_v1 *surface) {
    (void)context; (void)surface;
    unexpected_callback();
    return VDPL_E_UNSUPPORTED;
}
static int32_t pixels_write(void *context, const vdplet_pixels_v1 *pixels) {
    (void)context; (void)pixels;
    unexpected_callback();
    return VDPL_E_UNSUPPORTED;
}
static int32_t pcm_open(void *context, const vdplet_pcm_config_v1 *config,
                        vdplet_pcm_state_v1 *state) {
    (void)context; (void)config; (void)state;
    unexpected_callback();
    return VDPL_E_UNSUPPORTED;
}
static int32_t pcm_enqueue(void *context, uint32_t generation,
                           const uint8_t *data, uint32_t bytes) {
    (void)context; (void)generation; (void)data; (void)bytes;
    unexpected_callback();
    return VDPL_E_UNSUPPORTED;
}
static int32_t pcm_state(void *context, uint32_t generation,
                         vdplet_pcm_state_v1 *state) {
    (void)context; (void)generation; (void)state;
    unexpected_callback();
    return VDPL_E_UNSUPPORTED;
}
static int32_t pcm_control(void *context, uint32_t generation, uint32_t operation) {
    (void)context; (void)generation; (void)operation;
    unexpected_callback();
    return VDPL_E_UNSUPPORTED;
}
static int32_t pcm_close(void *context, uint32_t generation) {
    (void)context; (void)generation;
    unexpected_callback();
    return VDPL_E_UNSUPPORTED;
}

static vdplet_host_v1 *prefix(vdplet_host_v1_3 *table) {
    return &table->base.base.base;
}

static vdplet_host_v1_3 complete_table(void) {
    vdplet_host_v1_3 table = {0};
    vdplet_host_v1 *base = prefix(&table);
    base->struct_bytes = (uint32_t)sizeof(table);
    base->abi_major = VDPL_ABI_MAJOR;
    base->abi_minor = VDPL_ABI_MINOR_MEDIA;
    base->capabilities = VDPL_CAP_PIXELS | VDPL_CAP_PCM;
    table.surface_info = surface_info;
    table.pixels_write = pixels_write;
    table.pcm_open = pcm_open;
    table.pcm_enqueue = pcm_enqueue;
    table.pcm_state = pcm_state;
    table.pcm_control = pcm_control;
    table.pcm_close = pcm_close;
    /* All unrelated callbacks remain null: these helpers must not require them. */
    return table;
}

static void expect_pointer(const char *label, const char *accessor,
                           const void *actual, const void *expected) {
    ++checks;
    if (actual != expected) {
        fprintf(stderr, "FAIL: %s / %s returned %p, expected %p\n",
                label, accessor, actual, expected);
        exit(EXIT_FAILURE);
    }
}

static void expect_views(const char *label, const vdplet_host_v1 *base,
                         const vdplet_host_v1_3 *pixels,
                         const vdplet_host_v1_3 *pcm,
                         const vdplet_host_v1_3 *media) {
    expect_pointer(label, "pixels", vdplet_pixels_host(base), pixels);
    expect_pointer(label, "pcm", vdplet_pcm_host(base), pcm);
    expect_pointer(label, "media", vdplet_media_host(base), media);
}

static void capability_combinations(void) {
    static const uint32_t combinations[] = {
        0, VDPL_CAP_PIXELS, VDPL_CAP_PCM, VDPL_CAP_PIXELS | VDPL_CAP_PCM,
        VDPL_CAP_BUFFERS | VDPL_CAP_PCM_SINK,
        VDPL_CAP_PIXELS | UINT32_C(0x80000000),
        VDPL_CAP_PCM | UINT32_C(0x80000000),
        VDPL_CAP_PIXELS | VDPL_CAP_PCM | UINT32_C(0x80000000)
    };
    unsigned i;
    for (i = 0; i < sizeof(combinations) / sizeof(combinations[0]); ++i) {
        vdplet_host_v1_3 table = complete_table();
        const uint32_t caps = combinations[i];
        char label[64];
        prefix(&table)->capabilities = caps;
        snprintf(label, sizeof(label), "capabilities 0x%08lx", (unsigned long)caps);
        expect_views(label, prefix(&table),
                     (caps & VDPL_CAP_PIXELS) ? &table : NULL,
                     (caps & VDPL_CAP_PCM) ? &table : NULL,
                     ((caps & (VDPL_CAP_PIXELS | VDPL_CAP_PCM)) ==
                       (VDPL_CAP_PIXELS | VDPL_CAP_PCM)) ? &table : NULL);
    }

    {
        vdplet_host_v1_3 table = complete_table();
        prefix(&table)->capabilities = VDPL_CAP_PIXELS;
        table.pcm_open = NULL;
        table.pcm_enqueue = NULL;
        table.pcm_state = NULL;
        table.pcm_control = NULL;
        table.pcm_close = NULL;
        expect_views("pixels-only table, absent PCM callbacks", prefix(&table),
                     &table, NULL, NULL);
    }
    {
        vdplet_host_v1_3 table = complete_table();
        prefix(&table)->capabilities = VDPL_CAP_PCM;
        table.surface_info = NULL;
        table.pixels_write = NULL;
        expect_views("PCM-only table, absent pixel callbacks", prefix(&table),
                     NULL, &table, NULL);
    }
}

static void version_and_extent_guards(void) {
    static const uint16_t wrong_majors[] = {0, VDPL_ABI_MAJOR + 1, UINT16_MAX};
    static const uint32_t short_extents[] = {
        0, sizeof(vdplet_host_v1) - 1, sizeof(vdplet_host_v1),
        sizeof(vdplet_host_v1_1), sizeof(vdplet_host_v1_2),
        sizeof(vdplet_host_v1_3) - 1
    };
    unsigned i;
    char label[64];
    expect_views("null host", NULL, NULL, NULL, NULL);
    for (i = 0; i < sizeof(wrong_majors) / sizeof(wrong_majors[0]); ++i) {
        vdplet_host_v1_3 table = complete_table();
        prefix(&table)->abi_major = wrong_majors[i];
        snprintf(label, sizeof(label), "wrong major %u", (unsigned)wrong_majors[i]);
        expect_views(label, prefix(&table), NULL, NULL, NULL);
    }
    for (i = 0; i < VDPL_ABI_MINOR_MEDIA; ++i) {
        vdplet_host_v1_3 table = complete_table();
        prefix(&table)->abi_minor = (uint16_t)i;
        snprintf(label, sizeof(label), "old minor %u", i);
        expect_views(label, prefix(&table), NULL, NULL, NULL);
    }
    for (i = 0; i < sizeof(short_extents) / sizeof(short_extents[0]); ++i) {
        vdplet_host_v1_3 table = complete_table();
        prefix(&table)->struct_bytes = short_extents[i];
        snprintf(label, sizeof(label), "short extent %lu", (unsigned long)short_extents[i]);
        expect_views(label, prefix(&table), NULL, NULL, NULL);
    }
    {
        /* Real short objects, rather than a short count over a large allocation:
           ASan will catch extension reads before validating struct_bytes. */
        vdplet_host_v1 old = {0};
        vdplet_host_v1_2 lines = {0};
        old.struct_bytes = sizeof(old);
        old.abi_major = VDPL_ABI_MAJOR;
        old.abi_minor = VDPL_ABI_MINOR_PCM_SINK;
        old.capabilities = VDPL_CAP_PIXELS | VDPL_CAP_PCM;
        expect_views("actual base allocation, later minor", &old, NULL, NULL, NULL);
        lines.base.base = old;
        lines.base.base.struct_bytes = sizeof(lines);
        expect_views("actual line allocation, later minor", &lines.base.base,
                     NULL, NULL, NULL);
    }
}

static void callback_guards(void) {
    static const char *const names[] = {
        "surface_info", "pixels_write", "pcm_open", "pcm_enqueue",
        "pcm_state", "pcm_control", "pcm_close"
    };
    unsigned i;
    for (i = 0; i < sizeof(names) / sizeof(names[0]); ++i) {
        vdplet_host_v1_3 table = complete_table();
        char label[64];
        switch (i) {
            case 0: table.surface_info = NULL; break;
            case 1: table.pixels_write = NULL; break;
            case 2: table.pcm_open = NULL; break;
            case 3: table.pcm_enqueue = NULL; break;
            case 4: table.pcm_state = NULL; break;
            case 5: table.pcm_control = NULL; break;
            case 6: table.pcm_close = NULL; break;
            default: abort();
        }
        snprintf(label, sizeof(label), "missing %s", names[i]);
        expect_views(label, prefix(&table), i < 2 ? NULL : &table,
                     i < 2 ? &table : NULL, NULL);
    }
}

static void later_views(void) {
    vdplet_host_v1_3 exact = complete_table();
    vdplet_host_v1_4 queue = {0};
    vdplet_host_v1_5 sink = {0};
    expect_views("exact media view", prefix(&exact), &exact, &exact, &exact);
    queue.base = complete_table();
    prefix(&queue.base)->abi_minor = VDPL_ABI_MINOR_AUDIO_QUEUE;
    prefix(&queue.base)->struct_bytes = sizeof(queue);
    expect_views("later queue view, unrelated queue callback null",
                 prefix(&queue.base), &queue.base, &queue.base, &queue.base);
    sink.base.base = complete_table();
    prefix(&sink.base.base)->abi_minor = VDPL_ABI_MINOR_PCM_SINK;
    prefix(&sink.base.base)->struct_bytes = sizeof(sink);
    expect_views("later sink view, unrelated optional callbacks null",
                 prefix(&sink.base.base), &sink.base.base, &sink.base.base,
                 &sink.base.base);
    prefix(&exact)->abi_minor = UINT16_MAX;
    expect_views("future minor, sufficient media prefix", prefix(&exact),
                 &exact, &exact, &exact);
}

int main(void) {
    capability_combinations();
    version_and_extent_guards();
    callback_guards();
    later_views();
    printf("PASS: %u SDK accessor admission checks (%lu-bit host pointers).\n",
           checks, (unsigned long)(sizeof(void *) * 8));
    return EXIT_SUCCESS;
}
