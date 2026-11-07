/*
 * EcoConnect IoT Gateway - Dual Hardware Diagnostics (I2C + SPI)
 * Target: STM32F407G-DISC1 (ARM Cortex-M4 @ 168 MHz)
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/printk.h>

#define SLEEP_TIME_MS   1000

/* On-board Discovery LEDs */
static const struct gpio_dt_spec led_green  = GPIO_DT_SPEC_GET(DT_NODELABEL(green_led_4), gpios);
static const struct gpio_dt_spec led_orange = GPIO_DT_SPEC_GET(DT_NODELABEL(orange_led_3), gpios);
static const struct gpio_dt_spec led_red    = GPIO_DT_SPEC_GET(DT_NODELABEL(red_led_5), gpios);
static const struct gpio_dt_spec led_blue   = GPIO_DT_SPEC_GET(DT_NODELABEL(blue_led_6), gpios);

/* CS Pin for ENC28J60 on PB12 */
static const struct gpio_dt_spec enc_cs = {
	.port = DEVICE_DT_GET(DT_NODELABEL(gpiob)),
	.pin = 12,
	.dt_flags = GPIO_ACTIVE_LOW
};

/* Global Telemetry Variables */
volatile uint8_t g_bme280_id   = 0;
volatile uint8_t g_enc28j60_id = 0;

/* SPI Configuration (Mode 0,0, 5 MHz safe speed) */
static const struct spi_config spi_cfg = {
	.frequency = 5000000,
	.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB,
	.slave = 0,
};

static inline void enc_cs_assert(void)
{
	gpio_pin_set_dt(&enc_cs, 1);
}

static inline void enc_cs_deassert(void)
{
	gpio_pin_set_dt(&enc_cs, 0);
}

/* ENC28J60 Opcodes & Register Addresses */
#define ENC_SOFT_RESET     0xFF
#define ENC_CMD_BFS        0x80  /* Bit Field Set: 100xxxxx */
#define ENC_CMD_RCR        0x00  /* Read Control Register: 000xxxxx */
#define ENC_REG_ECON1      0x1F  /* Control Register 1 (common to all banks) */
#define ENC_REG_EREVID     0x12  /* Silicon Revision ID in Bank 3 */

uint8_t probe_enc28j60(const struct device *spi_dev)
{
	uint8_t tx_buf[2];
	uint8_t rx_buf[2];
	struct spi_buf tx_sbuf = { .buf = tx_buf, .len = 2 };
	struct spi_buf rx_sbuf = { .buf = rx_buf, .len = 2 };
	struct spi_buf_set tx_set = { .buffers = &tx_sbuf, .count = 1 };
	struct spi_buf_set rx_set = { .buffers = &rx_sbuf, .count = 1 };

	/* 1. Soft Reset: Send opcode 0xFF */
	tx_buf[0] = ENC_SOFT_RESET;
	tx_sbuf.len = 1;
	enc_cs_assert();
	spi_write(spi_dev, &spi_cfg, &tx_set);
	enc_cs_deassert();

	k_msleep(2); /* Wait 2 ms for oscillator startup */

	/* 2. Select Bank 3: Set bits [1:0] of ECON1 (BFS opcode 0x9F with value 0x03) */
	tx_buf[0] = (uint8_t)(ENC_CMD_BFS | ENC_REG_ECON1);
	tx_buf[1] = 0x03;
	tx_sbuf.len = 2;
	enc_cs_assert();
	spi_write(spi_dev, &spi_cfg, &tx_set);
	enc_cs_deassert();

	/* 3. Read EREVID (Address 0x12 in Bank 3) */
	/* Opcode 0x00 | 0x12 = 0x12, followed by dummy byte to clock in data */
	tx_buf[0] = (uint8_t)(ENC_CMD_RCR | ENC_REG_EREVID);
	tx_buf[1] = 0x00;
	tx_sbuf.len = 2;
	rx_sbuf.len = 2;
	rx_buf[0] = 0;
	rx_buf[1] = 0;

	enc_cs_assert();
	spi_transceive(spi_dev, &spi_cfg, &tx_set, &rx_set);
	enc_cs_deassert();

	return rx_buf[1]; /* Data byte clocked in on 2nd cycle */
}

int main(void)
{
	/* Configure LEDs */
	gpio_pin_configure_dt(&led_green, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&led_orange, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&led_red, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&led_blue, GPIO_OUTPUT_INACTIVE);

	/* Configure ENC28J60 CS Pin */
	gpio_pin_configure_dt(&enc_cs, GPIO_OUTPUT_INACTIVE);
	enc_cs_deassert();

	/* Initialize I2C1 and SPI1 buses */
	const struct device *const i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c1));
	const struct device *const spi_dev = DEVICE_DT_GET(DT_NODELABEL(spi1));

	if (!device_is_ready(i2c_dev) || !device_is_ready(spi_dev)) {
		gpio_pin_set_dt(&led_red, 1);
		return -1;
	}

	while (1) {
		gpio_pin_toggle_dt(&led_green); /* Green pulses on heartbeat */

		/* ---------------------------------------------------- */
		/* Subsystem 1: Check BME280 on I2C1                    */
		/* ---------------------------------------------------- */
		uint8_t reg = 0xD0;
		uint8_t bme_id = 0;
		if (i2c_write_read(i2c_dev, 0x76, &reg, 1, &bme_id, 1) == 0 ||
		    i2c_write_read(i2c_dev, 0x77, &reg, 1, &bme_id, 1) == 0) {
			g_bme280_id = bme_id;
			if (bme_id == 0x60 || bme_id == 0x58) {
				gpio_pin_set_dt(&led_blue, 1); /* Blue ON = BME280 Verified! */
			}
		} else {
			gpio_pin_set_dt(&led_blue, 0);
		}

		/* ---------------------------------------------------- */
		/* Subsystem 2: Check ENC28J60 on SPI1                  */
		/* ---------------------------------------------------- */
		uint8_t enc_rev = probe_enc28j60(spi_dev);
		g_enc28j60_id = enc_rev;

		/* EREVID valid values: 0x02, 0x04, 0x05, 0x06 (non-zero and non-0xFF) */
		if (enc_rev > 0x00 && enc_rev < 0xFF) {
			gpio_pin_set_dt(&led_orange, 1); /* Orange ON = ENC28J60 Verified! */
		} else {
			gpio_pin_set_dt(&led_orange, 0);
		}

		/* Red indicates if either subsystem failed */
		if (gpio_pin_get_dt(&led_blue) && gpio_pin_get_dt(&led_orange)) {
			gpio_pin_set_dt(&led_red, 0); /* Both Passed! */
		} else {
			gpio_pin_set_dt(&led_red, 1); /* One or both missing */
		}

		k_msleep(SLEEP_TIME_MS);
	}

	return 0;
}
