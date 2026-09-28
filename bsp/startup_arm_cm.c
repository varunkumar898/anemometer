#include <stdint.h>

extern uint32_t _sidata; /* Start of .data in FLASH */
extern uint32_t _sdata;  /* Start of .data in RAM */
extern uint32_t _edata;  /* End of .data in RAM */
extern uint32_t _sbss;   /* Start of .bss in RAM */
extern uint32_t _ebss;   /* End of .bss in RAM */
extern uint32_t _estack; /* Top of Stack */

extern int main(void);
extern void SysTick_Handler(void);

void Reset_Handler(void);
void Default_Handler(void);

void NMI_Handler(void)        __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler(void)  __attribute__((weak, alias("Default_Handler")));
void MemManage_Handler(void)  __attribute__((weak, alias("Default_Handler")));
void BusFault_Handler(void)   __attribute__((weak, alias("Default_Handler")));
void UsageFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SVC_Handler(void)        __attribute__((weak, alias("Default_Handler")));
void DebugMon_Handler(void)   __attribute__((weak, alias("Default_Handler")));
void PendSV_Handler(void)     __attribute__((weak, alias("Default_Handler")));

__attribute__((section(".isr_vector"), used))
const void *g_pfnVectors[] = {
    &_estack,
    Reset_Handler,
    NMI_Handler,
    HardFault_Handler,
    MemManage_Handler,
    BusFault_Handler,
    UsageFault_Handler,
    0, 0, 0, 0,
    SVC_Handler,
    DebugMon_Handler,
    0,
    PendSV_Handler,
    SysTick_Handler,
};

void Reset_Handler(void) {
    uint32_t *src = &_sidata;
    uint32_t *dst = &_sdata;

    /* Copy initialized data from Flash to SRAM */
    while (dst < &_edata) {
        *dst++ = *src++;
    }

    /* Zero-fill the bss section in SRAM */
    dst = &_sbss;
    while (dst < &_ebss) {
        *dst++ = 0;
    }

    /* Jump to main application entry */
    main();

    /* Trap if main returns */
    while (1) {
        __asm volatile("nop");
    }
}

void Default_Handler(void) {
    while (1) {
        __asm volatile("nop");
    }
}
