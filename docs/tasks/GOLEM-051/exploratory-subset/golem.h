/* Golem on-Agon compiler prototype. Copyright (c) 2026 Brandon R. Gates. MIT. */
#ifndef GOLEM_ON_AGON_H
#define GOLEM_ON_AGON_H
#include <stdint.h>
#include <stdio.h>

#define G_SOURCE_MAX 16384u
#define G_RECORD_MAX 192u
#define G_BUFFER_MAX 8192u
#define G_PAYLOAD_MAX 49152ul
#define G_GLOBAL_MAX 64u
#define G_SUB_MAX 16u
#define G_DEFAULT_BASE 4096u

typedef struct { uint8_t *data; unsigned size, capacity; } GBuffer;
typedef struct {
    GBuffer records[G_RECORD_MAX];
    unsigned count, base, entry, status, flags;
} GPackage;

/* Both functions leave ownership of every allocated record with the caller. */
int g_compile(const char *source, unsigned base, GPackage *out,
              char *error, unsigned error_size);
int g_read(FILE *file, GPackage *out, char *error, unsigned error_size);
int g_write(FILE *file, const GPackage *package);
int g_write_map(FILE *file, const GPackage *package);
void g_free(GPackage *package);
unsigned g_word(const uint8_t *bytes);
void g_putword(uint8_t *bytes, unsigned value);
#ifdef AGONDEV
int g_run(const GPackage *package);
#endif
#endif
