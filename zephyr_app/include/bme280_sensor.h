#ifndef BME280_SENSOR_H
#define BME280_SENSOR_H

#include <stdint.h>
#include <zephyr/device.h>

/**
 * @brief Initialize BME280 on the given I2C bus (auto-detects 0x76 or 0x77),
 *        verifies Chip ID, reads factory temperature calibration trims, and
 *        places the sensor in Normal sampling mode.
 *
 * @param i2c_dev Pointer to ready Zephyr I2C device.
 * @param chip_id_out Optional output pointer for the detected Chip ID (0x60).
 * @return 0 on success, negative errno on failure.
 */
int bme280_sensor_init(const struct device *i2c_dev, uint8_t *chip_id_out);

/**
 * @brief Read and compensate ambient temperature from BME280.
 *
 * @param i2c_dev Pointer to ready Zephyr I2C device.
 * @param temp_c_x100 Output temperature in hundredths of deg C (e.g. 2435 = 24.35 C).
 * @return 0 on success, negative errno on failure.
 */
int bme280_sensor_read_temp(const struct device *i2c_dev, int32_t *temp_c_x100);

#endif /* BME280_SENSOR_H */
