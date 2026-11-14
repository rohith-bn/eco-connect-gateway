#include "bme280_sensor.h"

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bme280_sensor, CONFIG_BME280_MOD_LOG_LEVEL);

#define BME280_ADDR_PRIMARY     0x76
#define BME280_ADDR_SECONDARY   0x77
#define BME280_CHIP_ID_BME280   0x60
#define BME280_CHIP_ID_BMP280   0x58

#define BME280_REG_DIG_T1       0x88
#define BME280_REG_CHIP_ID      0xD0
#define BME280_REG_CTRL_MEAS    0xF4
#define BME280_REG_TEMP_MSB     0xFA

/* osrs_t = 001 (1x), osrs_p = 001 (1x), mode = 11 (Normal) */
#define BME280_CTRL_MEAS_TEMP_NORMAL  0x27

struct bme280_calib_t {
	uint16_t dig_T1;
	int16_t  dig_T2;
	int16_t  dig_T3;
};

static uint8_t g_active_addr = BME280_ADDR_PRIMARY;
static struct bme280_calib_t g_calib;

int bme280_sensor_init(const struct device *i2c_dev, uint8_t *chip_id_out)
{
	uint8_t reg = BME280_REG_CHIP_ID;
	uint8_t id = 0;
	int ret;

	if (!device_is_ready(i2c_dev)) {
		LOG_ERR("I2C1 device not ready");
		return -ENODEV;
	}

	ret = i2c_write_read(i2c_dev, BME280_ADDR_PRIMARY, &reg, 1, &id, 1);
	if (ret == 0) {
		g_active_addr = BME280_ADDR_PRIMARY;
	} else {
		ret = i2c_write_read(i2c_dev, BME280_ADDR_SECONDARY, &reg, 1, &id, 1);
		if (ret != 0) {
			LOG_ERR("BME280 not found on 0x76 or 0x77 (err=%d)", ret);
			return ret;
		}
		g_active_addr = BME280_ADDR_SECONDARY;
	}

	if (chip_id_out != NULL) {
		*chip_id_out = id;
	}

	if (id != BME280_CHIP_ID_BME280 && id != BME280_CHIP_ID_BMP280) {
		LOG_WRN("Unexpected chip ID 0x%02X at 0x%02X", id, g_active_addr);
		return -EINVAL;
	}

	/* Read 6 bytes of factory temperature calibration (dig_T1, dig_T2, dig_T3) */
	uint8_t trim_reg = BME280_REG_DIG_T1;
	uint8_t trim[6] = {0};

	ret = i2c_write_read(i2c_dev, g_active_addr, &trim_reg, 1, trim, sizeof(trim));
	if (ret != 0) {
		LOG_ERR("Failed to read calibration trims (err=%d)", ret);
		return ret;
	}

	g_calib.dig_T1 = (uint16_t)(((uint16_t)trim[1] << 8) | trim[0]);
	g_calib.dig_T2 = (int16_t)(((uint16_t)trim[3] << 8) | trim[2]);
	g_calib.dig_T3 = (int16_t)(((uint16_t)trim[5] << 8) | trim[4]);

	/* Soft reset sensor to ensure clean register state */
	(void)i2c_reg_write_byte(i2c_dev, g_active_addr, 0xE0, 0xB6);
	k_msleep(10);

	/* Configure standby 0.5ms / IIR off, then Normal mode (0xF4 = 0x27) */
	(void)i2c_reg_write_byte(i2c_dev, g_active_addr, 0xF5, 0x00);
	ret = i2c_reg_write_byte(i2c_dev, g_active_addr, BME280_REG_CTRL_MEAS, BME280_CTRL_MEAS_TEMP_NORMAL);
	if (ret != 0) {
		LOG_ERR("Failed to configure ctrl_meas (err=%d)", ret);
		return ret;
	}

	uint8_t ctrl_imm = 0;
	(void)i2c_reg_read_byte(i2c_dev, g_active_addr, BME280_REG_CTRL_MEAS, &ctrl_imm);

	k_msleep(50); /* Allow first Normal-mode ADC conversion to complete */

	uint8_t ctrl_rb = 0;
	uint8_t stat_rb = 0;
	(void)i2c_reg_read_byte(i2c_dev, g_active_addr, BME280_REG_CTRL_MEAS, &ctrl_rb);
	(void)i2c_reg_read_byte(i2c_dev, g_active_addr, 0xF3, &stat_rb);

	LOG_INF("BMP/BME280 init @0x%02X (ID:0x%02X, imm:0x%02X, after50ms:0x%02X, stat:0x%02X)",
		g_active_addr, id, ctrl_imm, ctrl_rb, stat_rb);
	LOG_DBG("Trims: T1=%u, T2=%d, T3=%d", g_calib.dig_T1, g_calib.dig_T2, g_calib.dig_T3);
	return 0;
}

int bme280_sensor_read_temp(const struct device *i2c_dev, int32_t *temp_c_x100)
{
	uint8_t reg = BME280_REG_TEMP_MSB;
	uint8_t raw[3] = {0};
	int ret;

	if (temp_c_x100 == NULL) {
		return -EINVAL;
	}

	ret = i2c_write_read(i2c_dev, g_active_addr, &reg, 1, raw, sizeof(raw));
	if (ret != 0) {
		LOG_ERR("I2C temp read failed (err=%d)", ret);
		return ret;
	}

	/* 20-bit unsigned raw ADC temperature */
	int32_t adc_t = ((int32_t)raw[0] << 12) | ((int32_t)raw[1] << 4) | ((int32_t)raw[2] >> 4);

	/* Bosch BME280 datasheet 32-bit integer compensation formula */
	int32_t var1 = ((((adc_t >> 3) - ((int32_t)g_calib.dig_T1 << 1))) *
			((int32_t)g_calib.dig_T2)) >> 11;
	int32_t var2 = (((((adc_t >> 4) - ((int32_t)g_calib.dig_T1)) *
			  ((adc_t >> 4) - ((int32_t)g_calib.dig_T1))) >> 12) *
			((int32_t)g_calib.dig_T3)) >> 14;
	int32_t t_fine = var1 + var2;
	int32_t temp = (t_fine * 5 + 128) >> 8;

	*temp_c_x100 = temp;
	LOG_INF("Temperature: %d.%02d C (raw_adc=0x%05X)", temp / 100,
		(temp < 0 ? -temp : temp) % 100, (uint32_t)adc_t);
	return 0;
}
