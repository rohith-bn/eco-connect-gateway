#include "enc28j60_spi.h"

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(enc28j60_spi, CONFIG_ENC_SPI_MOD_LOG_LEVEL);

#define ENC_SOFT_RESET     0xFF
#define ENC_CMD_BFS        0x80  /* Bit Field Set: 100xxxxx */
#define ENC_CMD_RCR        0x00  /* Read Control Register: 000xxxxx */
#define ENC_REG_ECON1      0x1F  /* Control Register 1 (common to all banks) */
#define ENC_REG_EREVID     0x12  /* Silicon Revision ID in Bank 3 */

static const struct gpio_dt_spec enc_cs = {
	.port = DEVICE_DT_GET(DT_NODELABEL(gpiob)),
	.pin = 12,
	.dt_flags = GPIO_ACTIVE_LOW
};

static const struct spi_config spi_cfg = {
	.frequency = 1000000, /* 1 MHz for reliable breadboard signal integrity */
	.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB,
	.slave = 0,
};

int enc28j60_spi_probe_version(const struct device *spi_dev, uint8_t *rev_id_out)
{
	uint8_t tx_buf[2];
	uint8_t rx_buf[2];
	struct spi_buf tx_sbuf = { .buf = tx_buf, .len = 1 };
	struct spi_buf rx_sbuf = { .buf = rx_buf, .len = 2 };
	struct spi_buf_set tx_set = { .buffers = &tx_sbuf, .count = 1 };
	struct spi_buf_set rx_set = { .buffers = &rx_sbuf, .count = 1 };
	int ret;

	if (!device_is_ready(spi_dev) || !gpio_is_ready_dt(&enc_cs)) {
		LOG_ERR("SPI1 or CS GPIO not ready");
		return -ENODEV;
	}

	gpio_pin_configure_dt(&enc_cs, GPIO_OUTPUT_INACTIVE);
	k_msleep(50); /* Allow ENC28J60 25 MHz crystal and VCAP to stabilize after boot */

	/* 1. Soft Reset (0xFF) */
	tx_buf[0] = ENC_SOFT_RESET;
	tx_sbuf.len = 1;
	gpio_pin_set_dt(&enc_cs, 1);
	ret = spi_write(spi_dev, &spi_cfg, &tx_set);
	gpio_pin_set_dt(&enc_cs, 0);
	if (ret != 0) {
		LOG_ERR("SPI soft reset failed (err=%d)", ret);
		return ret;
	}
	LOG_DBG("Sent ENC28J60 Soft Reset (0xFF), waiting 2 ms for OST");
	k_msleep(2);

	/* 2. Select Bank 3 via ECON1[1:0] = 0b11 */
	tx_buf[0] = (uint8_t)(ENC_CMD_BFS | ENC_REG_ECON1);
	tx_buf[1] = 0x03;
	tx_sbuf.len = 2;
	gpio_pin_set_dt(&enc_cs, 1);
	ret = spi_write(spi_dev, &spi_cfg, &tx_set);
	gpio_pin_set_dt(&enc_cs, 0);
	if (ret != 0) {
		LOG_ERR("SPI Bank 3 select failed (err=%d)", ret);
		return ret;
	}
	LOG_DBG("Selected Bank 3 via ECON1 BFS (0x%02X 0x03)", tx_buf[0]);

	/* 3. Read EREVID (0x12 in Bank 3) */
	tx_buf[0] = (uint8_t)(ENC_CMD_RCR | ENC_REG_EREVID);
	tx_buf[1] = 0x00;
	tx_sbuf.len = 2;
	rx_buf[0] = 0;
	rx_buf[1] = 0;

	gpio_pin_set_dt(&enc_cs, 1);
	ret = spi_transceive(spi_dev, &spi_cfg, &tx_set, &rx_set);
	gpio_pin_set_dt(&enc_cs, 0);
	if (ret != 0) {
		LOG_ERR("SPI EREVID read failed (err=%d)", ret);
		return ret;
	}

	uint8_t rev = rx_buf[1];
	LOG_DBG("SPI RX bytes: [0]=0x%02X, [1]=0x%02X", rx_buf[0], rx_buf[1]);

	if (rev == 0x00 || rev == 0xFF) {
		LOG_ERR("Invalid ENC28J60 EREVID=0x%02X (check MISO/CS wiring)", rev);
		return -EIO;
	}

	if (rev_id_out != NULL) {
		*rev_id_out = rev;
	}

	LOG_INF("ENC28J60 Silicon Rev ID (EREVID): 0x%02X", rev);
	return 0;
}
