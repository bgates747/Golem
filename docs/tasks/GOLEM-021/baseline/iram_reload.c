/* Synthetic translation-cache regression, independent of VDP firmware. */
#include "xtensa.h"
#include "memory.h"
#include "jit.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

volatile int emu_app_running = 1;
static const uint32_t target = 0x40080000u;
static const uint32_t writer = 0x40080100u;
static const uint32_t returned = 0x40080200u;
static const uint32_t sync_pc = 0x40080300u;

#ifdef WITH_ISYNC_OBSERVER
typedef struct {
    uint8_t shadow[8];
    unsigned callbacks;
    unsigned invalidations;
} coherence_t;

static void instruction_sync(xtensa_cpu_t *cpu, void *opaque) {
    coherence_t *state = opaque;
    state->callbacks++;
    const uint8_t *current = mem_get_ptr(cpu->mem, target);
    if (memcmp(current, state->shadow, sizeof state->shadow)) {
        memcpy(state->shadow, current, sizeof state->shadow);
        xtensa_invalidate_code(cpu, target, sizeof state->shadow);
        state->invalidations++;
    }
}

static int first_load(void) {
    xtensa_cpu_t cpu;
    xtensa_cpu_init(&cpu);
    cpu.mem = mem_create();
    if (!cpu.mem) return 2;
    coherence_t coherence = {0};
    cpu.instruction_sync = instruction_sync;
    cpu.instruction_sync_ctx = &coherence;
    const uint8_t barrier[] = {0x00, 0x20, 0x00, 0x0d, 0xf0};
    for (unsigned i = 0; i < sizeof barrier; ++i)
        mem_write8(cpu.mem, sync_pc + i, barrier[i]);
    xtensa_predecode_build(&cpu);
    if (!cpu.predecode[target - PREDECODE_BASE]) return 10;
    uint8_t bytes[] = {0x22, 0xa0, 42, 0x3d, 0xf0, 0x0d, 0xf0, 0};
    memcpy(mem_get_ptr_w(cpu.mem, target), bytes, sizeof bytes);
    cpu.running = true;
    cpu.irq_check = false;
    cpu.next_timer_event = UINT32_MAX;
    ar_write(&cpu, 0, returned);
    cpu.pc = sync_pc;
    if (xtensa_run(&cpu, 2) != 2 || coherence.callbacks != 1 ||
        coherence.invalidations != 1 || cpu.predecode[target - PREDECODE_BASE]) return 11;
    cpu.pc = target;
    cpu._pc_written = false;
    if (xtensa_run(&cpu, 3) != 3 || ar_read(&cpu, 2) != 42 || cpu.exception) return 12;
    printf("{\"case\":\"predecoded_unused_iram_first_load\",\"interpreter\":42,"
           "\"sync_callbacks\":%u,\"invalidations\":%u,\"pass\":true}\n",
           coherence.callbacks, coherence.invalidations);
    free(cpu.predecode);
    mem_destroy(cpu.mem);
    return 0;
}
#endif

static jit_block_fn compile(jit_state_t *jit, xtensa_cpu_t *cpu, uint32_t pc) {
    cpu->pc = pc;
    for (unsigned i = 0; i <= JIT_HOT_THRESHOLD; ++i)
        (void)jit_get_block(jit, cpu, pc);
    return jit_get_block(jit, cpu, pc);
}

static bool execute(jit_state_t *jit, xtensa_cpu_t *cpu, uint32_t pc) {
    cpu->pc = pc;
    ar_write(cpu, 0, returned);
    jit_block_fn fn = compile(jit, cpu, pc);
    return fn && fn(cpu) >= 2 && cpu->pc == returned;
}

static int scenario(const char *name, unsigned mode, bool require_coherent, bool use_sync) {
    xtensa_cpu_t cpu;
    xtensa_cpu_init(&cpu);
    cpu.mem = mem_create();
    jit_state_t *jit = jit_init();
    if (!cpu.mem || !jit) return 2;
    jit_install_hook(jit, &cpu);
    /* MOVI a2,42; NOP.N; RET.N. Replacement changes only the immediate. */
    uint8_t bytes[] = {0x22, 0xa0, 42, 0x3d, 0xf0, 0x0d, 0xf0, 0};
    for (unsigned i = 0; i < sizeof bytes; ++i)
        mem_write8(cpu.mem, target + i, bytes[i]);
    uint8_t code[] = {0x42, 0x63, 0x00, 0x3d, 0xf0, 0x0d, 0xf0, 0, 0, 0};
    if (mode == 6) {
        const uint8_t synchronized[] = {0x42, 0x63, 0x00, 0x3d, 0xf0,
                                         0x00, 0x20, 0x00, 0x0d, 0xf0};
        memcpy(code, synchronized, sizeof code);
    }
    for (unsigned i = 0; i < sizeof code; ++i)
        mem_write8(cpu.mem, writer + i, code[i]);
#ifdef WITH_ISYNC_OBSERVER
    coherence_t coherence = {0};
    if (use_sync) {
        memcpy(coherence.shadow, bytes, sizeof bytes);
        cpu.instruction_sync = instruction_sync;
        cpu.instruction_sync_ctx = &coherence;
        /* ISYNC; RET.N, predecoded before the target changes. */
        const uint8_t barrier[] = {0x00, 0x20, 0x00, 0x0d, 0xf0};
        for (unsigned i = 0; i < sizeof barrier; ++i)
            mem_write8(cpu.mem, sync_pc + i, barrier[i]);
        xtensa_predecode_build(&cpu);
    }
#else
    (void)use_sync;
#endif
    if (!execute(jit, &cpu, target) || ar_read(&cpu, 2) != 42) return 3;
    bytes[2] = 43;
    uint32_t word;
    memcpy(&word, bytes, sizeof word);
    if (mode == 0) mem_write8(cpu.mem, target + 2, 43);
    else if (mode == 1) mem_write16(cpu.mem, target + 2, 0x3d2b);
    else if (mode == 2) mem_write32(cpu.mem, target, word);
    else if (mode == 3) memcpy(mem_get_ptr_w(cpu.mem, target), bytes, sizeof bytes);
    else {
        /* S32I a4,a3,0; NOP.N; RET.N. This store is performed by guest code. */
        ar_write(&cpu, 3, target);
        ar_write(&cpu, 4, word);
        if (mode == 6) {
            cpu.pc = writer;
            cpu._pc_written = true;
            cpu.running = true;
            if (!compile(jit, &cpu, writer)) return 13;
            uint64_t native_before = jit_get_stats(jit)->insns_jitted;
            if (xtensa_run(&cpu, 4) != 4 || cpu.pc != returned ||
                jit_get_stats(jit)->insns_jitted <= native_before) return 14;
        } else if (mode == 4) {
            if (!execute(jit, &cpu, writer)) return 4;
        } else {
            cpu.pc = writer;
            ar_write(&cpu, 0, returned);
            for (unsigned i = 0; i < 3; ++i) xtensa_step(&cpu);
            if (cpu.pc != returned) return 5;
        }
    }
#ifdef WITH_ISYNC_OBSERVER
    if (use_sync && mode != 6) {
        cpu.pc = sync_pc;
        cpu._pc_written = false;
        cpu.running = true;
        /* Exercise the actual cached interpreter path, which JIT's ordinary
         * record_branch_targets setting deliberately suppresses. */
        xtensa_pc_hook_fn saved_hook = cpu.pc_hook;
        void *saved_hook_ctx = cpu.pc_hook_ctx;
        const uint64_t *saved_bitmap = cpu.pc_hook_bitmap;
        cpu.pc_hook = NULL;
        cpu.pc_hook_ctx = NULL;
        cpu.pc_hook_bitmap = NULL;
        cpu.record_branch_targets = false;
        cpu.accelerated_blocks = false;
        cpu.irq_check = false;
        cpu.next_timer_event = UINT32_MAX;
        if (xtensa_run(&cpu, 2) != 2 || coherence.callbacks != 1 ||
            coherence.invalidations != 1) {
            fprintf(stderr, "DIAG ISYNC callbacks=%u invalidations=%u pc=%08x\n",
                    coherence.callbacks, coherence.invalidations, cpu.pc);
            return 9;
        }
        cpu.pc_hook = saved_hook;
        cpu.pc_hook_ctx = saved_hook_ctx;
        cpu.pc_hook_bitmap = saved_bitmap;
        cpu.record_branch_targets = true;
        cpu.accelerated_blocks = true;
    } else if (use_sync &&
               (coherence.callbacks != 1 || coherence.invalidations != 1)) {
        return 15;
    }
#endif
    xtensa_cpu_t reference = cpu;
    reference.pc = target;
    reference.pc_hook = NULL;
    reference.pc_hook_ctx = NULL;
    reference.pc_hook_bitmap = NULL;
    reference.accelerated_blocks = false;
    reference.predecode = NULL;
    ar_write(&reference, 0, returned);
    for (unsigned i = 0; i < 3; ++i) xtensa_step(&reference);
    uint32_t interpreted = ar_read(&reference, 2);
    if (interpreted != 43 || !execute(jit, &cpu, target)) {
        fprintf(stderr, "DIAG interpreted=%u reference_pc=%08x pc=%08x exception=%d cause=%u bytes=%08x\n",
                interpreted, reference.pc, cpu.pc, reference.exception,
                reference.exccause, mem_read32(cpu.mem, target));
        return 6;
    }
    uint32_t before_invalidation = ar_read(&cpu, 2);
    if (before_invalidation != (require_coherent ? 43u : 42u)) return 7;
    uint64_t old_flushes = jit_get_stats(jit)->cache_flushes;
    xtensa_invalidate_code(&cpu, target, sizeof bytes);
    bool replaced = execute(jit, &cpu, target);
    if (jit_get_stats(jit)->cache_flushes <= old_flushes ||
        !replaced || ar_read(&cpu, 2) != 43) {
        fprintf(stderr, "DIAG replacement=%d value=%u pc=%08x flushes=%llu old=%llu\n",
                replaced, ar_read(&cpu, 2), cpu.pc,
                (unsigned long long)jit_get_stats(jit)->cache_flushes,
                (unsigned long long)old_flushes);
        return 8;
    }
    printf("{\"case\":\"%s\",\"interpreter\":%u,\"jit_before_invalidation\":%u,"
           "\"jit_after_invalidation\":%u,\"expected_stale\":%s,\"pass\":true}\n",
           name, interpreted, before_invalidation, ar_read(&cpu, 2),
           require_coherent ? "false" : "true");
    jit_destroy(jit);
    free(cpu.predecode);
    mem_destroy(cpu.mem);
    return 0;
}

int main(int argc, char **argv) {
    bool coherent = argc == 2 && strcmp(argv[1], "--require-coherent") == 0;
    bool use_sync = argc == 2 && strcmp(argv[1], "--instruction-sync") == 0;
    coherent = coherent || use_sync;
    if (argc > 2 || (argc == 2 && !coherent)) return 2;
#ifndef WITH_ISYNC_OBSERVER
    if (use_sync) return 2;
#else
    if (use_sync) {
        int result = first_load();
        if (result) {
            fprintf(stderr, "FAIL first_load check=%d\n", result);
            return result;
        }
    }
#endif
    const char *names[] = {"host_mem_write8", "host_mem_write16", "host_mem_write32",
                           "direct_backing_memcpy", "jit_guest_s32i", "interpreter_guest_s32i"};
    for (unsigned i = 0; i < sizeof names / sizeof names[0]; ++i) {
        int result = scenario(names[i], i, coherent, use_sync);
        if (result) {
            fprintf(stderr, "FAIL case=%s check=%d\n", names[i], result);
            return result;
        }
    }
    if (use_sync) {
        int result = scenario("jit_store_isync_fallthrough", 6, true, true);
        if (result) {
            fprintf(stderr, "FAIL jit_store_isync_fallthrough check=%d\n", result);
            return result;
        }
    }
    return 0;
}
