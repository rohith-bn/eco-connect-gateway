/**
 * @file main.c
 * @brief Pure Bare-Metal LED Blinky for STM32F407G-DISC1 (ARM Cortex-M4)
 * @author Rohith B Narasimhamurthy
 * 
 * @details
 * This driver demonstrates zero-abstraction bring-up of the STM32F407
 * microcontroller by directly configuring memory-mapped registers:
 *   1. RCC_AHB1ENR: Opens clock gating to GPIO Port D.
 *   2. GPIOD_MODER: Configures pins PD12, PD13, PD14, PD15 as general-purpose outputs.
 *   3. GPIOD_ODR: Drives pin voltages High (3.3V) and Low (0V) in an infinite loop.
 * 
 * Hardware Target: STM32F407G-DISC1 Discovery Board
 * Clocks: 16 MHz internal HSI RC oscillator (default state after power-on reset).
 */

#include "registers.h"

/**
 * @brief Accurate software delay loop.
 * @param count Number of loop cycles to execute.
 * @note Marked volatile to prevent compiler optimization (-O2/-O3) from stripping the loop.
 */
static void delay_cycles(volatile uint32_t count)
{
    while (count > 0U)
    {
        __asm__ volatile ("nop");
        count--;
    }
}

/**
 * @brief Configures onboard User LEDs (PD12=Green, PD13=Orange, PD14=Red, PD15=Blue).
 */
static void gpio_leds_init(void)
{
    /* Step 1: Enable AHB1 bus clock gate for GPIO Port D */
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIODEN;

    /* 
     * Small dummy read to ensure clock is settled on the bus before 
     * accessing peripheral registers (standard ARM silicon synchronization barrier).
     */
    volatile uint32_t dummy = RCC_AHB1ENR;
    (void)dummy;

    /* 
     * Step 2: Configure PD12 (Green) as General Purpose Output
     * Each pin occupies 2 bits in GPIOD_MODER:
     * Pin 12: bits [25:24]. Reset value is 00 (Input).
     * Rule: Always clear target bits before setting the new mask.
     */
    GPIOD_MODER &= ~(0x3U << (LED_GREEN_PIN * 2U));
    GPIOD_MODER |=  (GPIO_MODE_OUTPUT << (LED_GREEN_PIN * 2U));

    /* Configure PD13 (Orange), PD14 (Red), and PD15 (Blue) as outputs */
    GPIOD_MODER &= ~(0x3U << (LED_ORANGE_PIN * 2U));
    GPIOD_MODER |=  (GPIO_MODE_OUTPUT << (LED_ORANGE_PIN * 2U));

    GPIOD_MODER &= ~(0x3U << (LED_RED_PIN * 2U));
    GPIOD_MODER |=  (GPIO_MODE_OUTPUT << (LED_RED_PIN * 2U));

    GPIOD_MODER &= ~(0x3U << (LED_BLUE_PIN * 2U));
    GPIOD_MODER |=  (GPIO_MODE_OUTPUT << (LED_BLUE_PIN * 2U));

    /* Step 3: Turn all LEDs OFF initially (write 0 to ODR bits 12-15) */
    GPIOD_ODR &= ~((1U << LED_GREEN_PIN)  | 
                   (1U << LED_ORANGE_PIN) | 
                   (1U << LED_RED_PIN)    | 
                   (1U << LED_BLUE_PIN));
}

int main(void)
{
    /* Initialize the GPIO hardware registers */
    gpio_leds_init();

    /* Infinite LED toggle loop */
    while (1)
    {
        /* 
         * Option A: Toggle Green LED (PD12)
         * XOR bit 12: If 0 becomes 1 (3.3V, ON); if 1 becomes 0 (0V, OFF).
         */
        GPIOD_ODR ^= (1U << LED_GREEN_PIN);

        /* 
         * Software delay:
         * At 16 MHz HSI, each loop takes approx. 3 to 4 CPU cycles.
         * 1,600,000 iterations ≈ 400 milliseconds.
         */
        delay_cycles(1600000U);
    }

    /* Code should never reach here in bare-metal embedded execution */
    return 0;
}
