#ifndef ENC28J60_SPI_H
#define ENC28J60_SPI_H

#include <stdint.h>
#include <zephyr/device.h>

/**
 * @brief Soft-reset ENC28J60 over SPI1, switch to Bank 3, and read silicon
 *        revision ID (EREVID register 0x12).
 *
 * @param spi_dev Pointer to ready Zephyr SPI1 device.
 * @param rev_id_out Output pointer for EREVID byte.
 * @return 0 on valid revision ID (0x01..0xFE), negative errno on failure.
 */
int enc28j60_spi_probe_version(const struct device *spi_dev, uint8_t *rev_id_out);

#endif /* ENC28J60_SPI_H */
