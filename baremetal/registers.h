/**
 * @file registers.h
 * @brief Direct Memory-Mapped Register Definitions for STM32F407 (ARM Cortex-M4)
 * @author Rohith B Narasimhamurthy
 * @note Derived directly from STM32F407 Reference Manual (RM0090).
 */

#ifndef REGISTERS_H_
#define REGISTERS_H_

#include <stdint.h>

/* ========================================================================== */
/* Bus Memory Base Addresses                                                  */
/* ========================================================================== */
#define PERIPH_BASE             (0x40000000UL)
#define APB1PERIPH_BASE         (PERIPH_BASE + 0x00000000UL)
#define APB2PERIPH_BASE         (PERIPH_BASE + 0x00010000UL)
#define AHB1PERIPH_BASE         (PERIPH_BASE + 0x00020000UL)

/* ========================================================================== */
/* Reset and Clock Control (RCC) Registers                                    */
/* ========================================================================== */
#define RCC_BASE                (AHB1PERIPH_BASE + 0x3800UL)

/* RCC AHB1 Peripheral Clock Enable Register (Address: 0x4002 3830) */
#define RCC_AHB1ENR             (*(volatile uint32_t *)(RCC_BASE + 0x30UL))
#define RCC_AHB1ENR_GPIOAEN     (1U << 0)
#define RCC_AHB1ENR_GPIOBEN     (1U << 1)
#define RCC_AHB1ENR_GPIOCEN     (1U << 2)
#define RCC_AHB1ENR_GPIODEN     (1U << 3)  /* Port D Clock Gate Enable */

/* ========================================================================== */
/* General Purpose I/O Port D (GPIOD) Registers                               */
/* ========================================================================== */
#define GPIOD_BASE              (AHB1PERIPH_BASE + 0x0C00UL)

/* GPIO Port Mode Register (Address: 0x4002 0C00) - 2 bits per pin */
#define GPIOD_MODER             (*(volatile uint32_t *)(GPIOD_BASE + 0x00UL))

/* GPIO Port Output Type Register (Address: 0x4002 0C04) - 1 bit per pin */
#define GPIOD_OTYPER            (*(volatile uint32_t *)(GPIOD_BASE + 0x04UL))

/* GPIO Port Output Speed Register (Address: 0x4002 0C08) - 2 bits per pin */
#define GPIOD_OSPEEDR           (*(volatile uint32_t *)(GPIOD_BASE + 0x08UL))

/* GPIO Port Pull-Up/Pull-Down Register (Address: 0x4002 0C0C) - 2 bits per pin */
#define GPIOD_PUPDR             (*(volatile uint32_t *)(GPIOD_BASE + 0x0CUL))

/* GPIO Port Input Data Register (Address: 0x4002 0C10) */
#define GPIOD_IDR               (*(volatile uint32_t *)(GPIOD_BASE + 0x10UL))

/* GPIO Port Output Data Register (Address: 0x4002 0C14) */
#define GPIOD_ODR               (*(volatile uint32_t *)(GPIOD_BASE + 0x14UL))

/* GPIO Port Bit Set/Reset Register (Address: 0x4002 0C18) - Atomic bit writing */
#define GPIOD_BSRR              (*(volatile uint32_t *)(GPIOD_BASE + 0x18UL))

/* ========================================================================== */
/* STM32F4-Discovery Onboard User LED Pin Definitions                         */
/* ========================================================================== */
#define LED_GREEN_PIN           (12U)  /* LD4 (Green)  */
#define LED_ORANGE_PIN          (13U)  /* LD3 (Orange) */
#define LED_RED_PIN             (14U)  /* LD5 (Red)    */
#define LED_BLUE_PIN            (15U)  /* LD6 (Blue)   */

/* Mode bit positions in MODER (2 bits per pin) */
#define GPIO_MODE_INPUT         (0x0U)
#define GPIO_MODE_OUTPUT        (0x1U)
#define GPIO_MODE_ALTFUNC       (0x2U)
#define GPIO_MODE_ANALOG        (0x3U)

#endif /* REGISTERS_H_ */
