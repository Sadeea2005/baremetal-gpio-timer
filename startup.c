/*
 * startup.c  --  Minimal C startup + vector table for STM32F103 (Cortex-M3).
 * ---------------------------------------------------------------------------
 * Before main() can run, SOMEONE has to: set the initial stack pointer, copy
 * initialised globals from flash to RAM (.data), zero uninitialised globals
 * (.bss), and place the reset vector. On a "real" project the vendor ships
 * this; writing it yourself proves you understand the C runtime and the
 * memory map. The linker script (stm32f103.ld) provides the symbols below.
 */
#include <stdint.h>

extern uint32_t _sidata;   /* .data init values, stored in flash */
extern uint32_t _sdata;    /* .data start in RAM                 */
extern uint32_t _edata;    /* .data end in RAM                   */
extern uint32_t _sbss;     /* .bss start in RAM                  */
extern uint32_t _ebss;     /* .bss end in RAM                    */
extern uint32_t _estack;   /* top of stack (end of RAM)          */

int main(void);

void Reset_Handler(void) {
    /* Copy .data (initialised globals) from flash to RAM */
    uint32_t *src = &_sidata;
    uint32_t *dst = &_sdata;
    while (dst < &_edata) {
        *dst++ = *src++;
    }
    /* Zero .bss */
    for (dst = &_sbss; dst < &_ebss; ) {
        *dst++ = 0u;
    }
    main();
    for (;;) { }               /* never return */
}

void Default_Handler(void) {
    for (;;) { }               /* trap unexpected exceptions here */
}

/* The vector table. Entry 0 is the initial stack pointer, entry 1 is the
 * reset handler. We alias the rest to Default_Handler. The linker places
 * this array at the very start of flash (0x08000000) via the .isr_vector
 * section (see the linker script). */
__attribute__((section(".isr_vector")))
void (* const g_vectors[])(void) = {
    (void (*)(void))(&_estack),   /* 0x00: initial SP        */
    Reset_Handler,                /* 0x04: reset             */
    Default_Handler,              /* NMI                     */
    Default_Handler,              /* HardFault               */
    Default_Handler,              /* MemManage               */
    Default_Handler,              /* BusFault                */
    Default_Handler,              /* UsageFault              */
    0, 0, 0, 0,                   /* reserved                */
    Default_Handler,              /* SVCall                  */
    Default_Handler,              /* Debug Monitor           */
    0,                            /* reserved                */
    Default_Handler,              /* PendSV                  */
    Default_Handler,              /* SysTick                 */
    /* Peripheral IRQs would follow here; polling demo doesn't need them. */
};
