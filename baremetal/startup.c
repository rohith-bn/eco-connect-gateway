/**
 * @file startup.c
 * @brief Minimal Bare-Metal Startup Code and Vector Table for STM32F407 (Cortex-M4)
 * @author Rohith B Narasimhamurthy
 */

#include <stdint.h>

/* Symbols exported by the linker script */
extern uint32_t _estack;
extern uint32_t _sidata;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;

/* Prototype of main function */
int main(void);

/* Default Handler for unexpected interrupts */
void Default_Handler(void)
{
    while (1)
    {
        /* Loop indefinitely if an unhandled fault occurs */
    }
}

/* Reset Handler: The first code executed when silicon powers on */
void Reset_Handler(void)
{
    /* 1. Copy initialized .data section from FLASH to SRAM */
    uint32_t *pSrc = &_sidata;
    uint32_t *pDst = &_sdata;

    while (pDst < &_edata)
    {
        *pDst++ = *pSrc++;
    }

    /* 2. Zero-initialize the .bss section in SRAM */
    uint32_t *pBss = &_sbss;
    while (pBss < &_ebss)
    {
        *pBss++ = 0U;
    }

    /* 3. Branch to the C application entry point */
    main();

    /* 4. Safety catch if main ever returns */
    while (1);
}

/* Cortex-M4 System Exception Handlers with weak aliases */
void NMI_Handler(void)        __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler(void)  __attribute__((weak, alias("Default_Handler")));
void MemManage_Handler(void)  __attribute__((weak, alias("Default_Handler")));
void BusFault_Handler(void)   __attribute__((weak, alias("Default_Handler")));
void UsageFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SVC_Handler(void)        __attribute__((weak, alias("Default_Handler")));
void DebugMon_Handler(void)   __attribute__((weak, alias("Default_Handler")));
void PendSV_Handler(void)     __attribute__((weak, alias("Default_Handler")));
void SysTick_Handler(void)    __attribute__((weak, alias("Default_Handler")));

/* Vector Table: Positioned at 0x08000000 by linker script */
__attribute__((section(".isr_vector")))
const void *vector_table[] = {
    &_estack,             /* 0x00: Initial Main Stack Pointer (MSP) */
    Reset_Handler,        /* 0x04: Reset Handler Address            */
    NMI_Handler,          /* 0x08: Non-Maskable Interrupt           */
    HardFault_Handler,    /* 0x0C: Hard Fault Handler               */
    MemManage_Handler,    /* 0x10: Memory Management Fault          */
    BusFault_Handler,     /* 0x14: Bus Fault                        */
    UsageFault_Handler,   /* 0x18: Usage Fault                      */
    0, 0, 0, 0,           /* 0x1C - 0x28: Reserved                  */
    SVC_Handler,          /* 0x2C: SVCall Handler                   */
    DebugMon_Handler,     /* 0x30: Debug Monitor Handler            */
    0,                    /* 0x34: Reserved                         */
    PendSV_Handler,       /* 0x38: PendSV Handler                   */
    SysTick_Handler       /* 0x3C: SysTick Timer Handler            */
};
