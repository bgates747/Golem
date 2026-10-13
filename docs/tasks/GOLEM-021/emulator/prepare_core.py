#!/usr/bin/env python3
"""Export pinned read-only Flexe and apply bounded integration corrections."""
from __future__ import annotations
import argparse
import hashlib
import io
import json
from pathlib import Path
import subprocess
import tarfile

PIN='fbda8e905ccff36a090e4e33b0ffc2337dd79c52'
here=Path(__file__).resolve().parent
p=argparse.ArgumentParser()
p.add_argument('--reference',type=Path,default=Path('/Agon/others/levkropp/flexe'))
p.add_argument('--source',type=Path,default=here/'build/flexe-source')
a=p.parse_args()
marker=a.source/'.golem-flexe-source.json'
if not marker.exists():
    if a.source.exists() and any(a.source.iterdir()):raise SystemExit('Refusing unmarked nonempty source export')
    a.source.mkdir(parents=True,exist_ok=True)
    archive=subprocess.check_output(['git','-C',str(a.reference),'archive',PIN])
    with tarfile.open(fileobj=io.BytesIO(archive)) as tar:tar.extractall(a.source,filter='data')
    marker.write_text(json.dumps({'upstream_commit':PIN,'patch_revision':0},indent=2)+'\n')
identity=json.loads(marker.read_text())
if identity['upstream_commit']!=PIN:raise SystemExit('Unexpected source identity')
if identity['patch_revision']==0:
    def replace(file,old,new):
        path=a.source/file
        text=path.read_text()
        if text.count(old)!=1:raise SystemExit(f'Patch context differs: {file}')
        path.write_text(text.replace(old,new))
    replace('src/jit.c',
'''    sys_icache_invalidate(start, len);
#else
    (void)start; (void)len;
#endif
}''',
'''    sys_icache_invalidate(start, len);
#elif defined(__aarch64__)
    __builtin___clear_cache((char *)start, (char *)start + len);
#else
    (void)start; (void)len;
#endif
}''')
    replace('src/jit.c',
'''#  if defined(JIT_NEEDS_WX)
    sys_icache_invalidate(jmp_site, 4);
#  endif''',
'''#  if defined(JIT_NEEDS_WX)
    sys_icache_invalidate(jmp_site, 4);
#  else
    __builtin___clear_cache((char *)jmp_site, (char *)jmp_site + 4);
#  endif''')
    replace('src/xtensa.h',
'''    xtensa_code_invalidate_fn code_invalidate;
    void *code_invalidate_ctx;''',
'''    xtensa_code_invalidate_fn code_invalidate;
    void *code_invalidate_ctx;

    /* Optional ISYNC observer: host translation coherence for changed RAM. */
    void (*instruction_sync)(xtensa_cpu_t *cpu, void *ctx);
    void *instruction_sync_ctx;''')
    replace('src/xtensa.c',
'''                /* Fast mode has a single strongly ordered host timeline, so
                 * ISYNC/RSYNC/ESYNC/DSYNC/EXTW/MEMW/EXCW need no additional
                 * operation here. Accurate schedulers will use these as
                 * synchronization boundaries. */
                break;''',
'''                /* ISYNC is a translation-coherence boundary for code written
                 * to executable RAM. Other barriers retain ordered-host semantics. */
                if (XT_T(insn) == 0u && cpu->instruction_sync)
                    cpu->instruction_sync(cpu, cpu->instruction_sync_ctx);
                break;''')
    replace('src/jit.c',
'''                if (r == 2)
                    return xtensa_sync_encoding_valid(insn) ? 0 : 2;''',
'''                if (r == 2)
                    return xtensa_sync_encoding_valid(insn) && XT_T(insn) != 0u ? 0 : 2;''')
    replace('src/flexe_session.c',
'''    cpu->code_invalidate = old.code_invalidate;
    cpu->code_invalidate_ctx = old.code_invalidate_ctx;''',
'''    cpu->code_invalidate = old.code_invalidate;
    cpu->code_invalidate_ctx = old.code_invalidate_ctx;
    cpu->instruction_sync = old.instruction_sync;
    cpu->instruction_sync_ctx = old.instruction_sync_ctx;''')
    replace('src/xtensa.c',
'''            if (r == 2 && xtensa_sync_encoding_valid(insn)) return true;''',
'''            if (r == 2 && xtensa_sync_encoding_valid(insn)) {
                if (XT_T(insn) == 0u && cpu->instruction_sync)
                    cpu->instruction_sync(cpu, cpu->instruction_sync_ctx);
                return true;
            }''')
    identity['patch_revision']=2
    identity['patch_files']={file:hashlib.sha256((a.source/file).read_bytes()).hexdigest() for file in ['src/jit.c','src/xtensa.h','src/xtensa.c','src/flexe_session.c']}
    marker.write_text(json.dumps(identity,indent=2)+'\n')
elif identity['patch_revision']==1:
    for file,digest in identity['patch_files'].items():
        if hashlib.sha256((a.source/file).read_bytes()).hexdigest()!=digest:raise SystemExit(f'Export changed: {file}')
    path=a.source/'src/xtensa.c'
    text=path.read_text()
    old='            if (r == 2 && xtensa_sync_encoding_valid(insn)) return true;'
    new='''            if (r == 2 && xtensa_sync_encoding_valid(insn)) {
                if (XT_T(insn) == 0u && cpu->instruction_sync)
                    cpu->instruction_sync(cpu, cpu->instruction_sync_ctx);
                return true;
            }'''
    if text.count(old)!=1:raise SystemExit('Cached ISYNC patch context differs')
    path.write_text(text.replace(old,new))
    identity['patch_revision']=2
    identity['patch_files']['src/xtensa.c']=hashlib.sha256(path.read_bytes()).hexdigest()
    marker.write_text(json.dumps(identity,indent=2)+'\n')
elif identity['patch_revision']not in (2,3,4):raise SystemExit('Unknown integration patch revision')
if identity['patch_revision']==2:
    for file,digest in identity['patch_files'].items():
        if hashlib.sha256((a.source/file).read_bytes()).hexdigest()!=digest:raise SystemExit(f'Export changed: {file}')
    path=a.source/'src/xtensa.c';text=path.read_text()
    old='                else if (r == 2 && xtensa_sync_encoding_valid(insn)) kind = PD_NOP;'
    new='                else if (r == 2 && xtensa_sync_encoding_valid(insn)) kind = XT_T(insn) == 0u ? PD_SLOW : PD_NOP;'
    if text.count(old)!=1:raise SystemExit('Predecoded ISYNC patch context differs')
    path.write_text(text.replace(old,new))
    identity['patch_revision']=3
    identity['patch_files']['src/xtensa.c']=hashlib.sha256(path.read_bytes()).hexdigest()
    marker.write_text(json.dumps(identity,indent=2)+'\n')
if identity['patch_revision']==3:
    for file,digest in identity['patch_files'].items():
        if hashlib.sha256((a.source/file).read_bytes()).hexdigest()!=digest:raise SystemExit(f'Export changed: {file}')
    path=a.source/'src/peripherals.c';text=path.read_text()
    def replace(old,new):
        global text
        if text.count(old)!=1:raise SystemExit('UART timeout patch context differs')
        text=text.replace(old,new)
    replace('''    uint16_t rx_count;
    uart_tx_cb cb;''','''    uint16_t rx_count;
    esp32_periph_t *owner;
    int number;
    uart_tx_cb cb;''')
    replace('''static void uart_refresh_level_conditions(esp32_periph_t *p, int uart_num) {''','''/* Classic ESP32 timeout runs on the baud clock after incoming data stops.
 * Immediate host-injection-only timeout loses tail bytes when an ISR clears
 * TOUT while more bytes remain in the FIFO. Use the existing event clock so
 * interpreter, JIT and WAITI all see the same hardware-level condition. */
static void uart_rx_timeout_fire(void *ctx) {
    uart_state_t *uart = ctx;
    esp32_periph_t *p = uart->owner;
    const flexe_uart_ip_desc_t *ip = &p->target->uart_ip;
    if (uart->clock_enabled && !uart->reset_asserted && uart->rx_count &&
        (uart->shadow[0x24 / 4] & ip->rx_timeout_enable_mask)) {
        uart->int_raw |= UART_RXFIFO_TOUT_INT;
        uart_intr_update(p, uart->number);
    }
}

static void uart_arm_rx_timeout(esp32_periph_t *p, int uart_num) {
    if (!(p->target->capabilities & FLEXE_TARGET_CAP_ESP32_CLASSIC_PERIPHERALS))
        return;
    uart_state_t *uart = &p->uart[uart_num];
    periph_cancel_deferred(p, uart_rx_timeout_fire, uart);
    uint32_t conf1 = uart->shadow[0x24 / 4];
    if (!uart->clock_enabled || uart->reset_asserted || !uart->rx_count ||
        !(conf1 & p->target->uart_ip.rx_timeout_enable_mask)) return;
    uint64_t threshold = (conf1 >> 24) & 127u;
    uint32_t clkdiv = uart->shadow[0x14 / 4];
    uint64_t divider16 = ((uint64_t)(clkdiv & 0xfffffu) << 4) |
                         ((clkdiv >> 20) & 15u);
    /* ESP-IDF 4.4 uart_ll_set_rx_tout: APB threshold counts baud/8;
     * REF_TICK threshold uses an eightfold hardware workaround. */
    bool apb = (uart->shadow[0x20 / 4] & (1u << 27)) != 0u;
    uint64_t numerator = divider16 * threshold * (apb ? 8u : 1u);
    uint64_t denominator = apb ? (16u * 80u) : (16u * 8u);
    uint64_t delay_us = (numerator + denominator - 1u) / denominator;
    if (!delay_us) delay_us = 1u;
    (void)periph_schedule_deferred_us(p, delay_us, uart_rx_timeout_fire, uart);
}

static void uart_refresh_level_conditions(esp32_periph_t *p, int uart_num) {''')
    replace('''    uart->rx_count = 0u;
    memset(uart->shadow, 0, sizeof(uart->shadow));''','''    uart->rx_count = 0u;
    periph_cancel_deferred(p, uart_rx_timeout_fire, uart);
    memset(uart->shadow, 0, sizeof(uart->shadow));''')
    replace('''        uart->rx_count--;
        return byte;''','''        uart->rx_count--;
        if (!uart->rx_count)
            periph_cancel_deferred(p, uart_rx_timeout_fire, uart);
        return byte;''')
    replace('''        uart->int_raw &= ~(val & ip->interrupt_valid_mask);
        /* FIFO threshold and TX empty are level-triggered. */''','''        uart->int_raw &= ~(val & ip->interrupt_valid_mask);
        if (val & UART_RXFIFO_TOUT_INT) uart_arm_rx_timeout(p, uart_num);
        /* FIFO threshold and TX empty are level-triggered. */''')
    replace('''            if (uart->rx_count > 0 &&
                (val & ip->rx_timeout_enable_mask))
                uart->int_raw |= UART_RXFIFO_TOUT_INT;''','''            if (p->target->capabilities & FLEXE_TARGET_CAP_ESP32_CLASSIC_PERIPHERALS)
                uart_arm_rx_timeout(p, uart_num);
            else if (uart->rx_count > 0 && (val & ip->rx_timeout_enable_mask))
                uart->int_raw |= UART_RXFIFO_TOUT_INT;''')
    replace('''        p->uart[uart_num].clock_enabled = true;
        uart_reset_state(p, (unsigned)uart_num);''','''        p->uart[uart_num].owner = p;
        p->uart[uart_num].number = uart_num;
        p->uart[uart_num].clock_enabled = true;
        uart_reset_state(p, (unsigned)uart_num);''')
    replace('''    if (accepted > dma_accepted &&
        (conf1 & ip->rx_timeout_enable_mask))
        uart->int_raw |= UART_RXFIFO_TOUT_INT;''','''    if (accepted > dma_accepted) {
        if (p->target->capabilities & FLEXE_TARGET_CAP_ESP32_CLASSIC_PERIPHERALS)
            uart_arm_rx_timeout(p, uart_num);
        else if (conf1 & ip->rx_timeout_enable_mask)
            uart->int_raw |= UART_RXFIFO_TOUT_INT;
    }''')
    path.write_text(text)
    identity['patch_revision']=4
    identity['patch_files']['src/peripherals.c']=hashlib.sha256(path.read_bytes()).hexdigest()
    marker.write_text(json.dumps(identity,indent=2)+'\n')
for file,digest in identity['patch_files'].items():
    if hashlib.sha256((a.source/file).read_bytes()).hexdigest()!=digest:raise SystemExit(f'Export changed: {file}')
print(json.dumps(identity,indent=2))
