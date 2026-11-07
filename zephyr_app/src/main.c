/*
 * EcoConnect IoT Gateway - Main Application
 * Board: STM32F407G-DISC1
 * OS: Zephyr RTOS
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/net/net_if.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(gateway_main, LOG_LEVEL_INF);

/* 1000 msec = 1 sec blink period */
#define SLEEP_TIME_MS   1000

/* Devicetree Node Identifiers */
#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

int main(void)
{
	int ret;

	LOG_INF("====================================================");
	LOG_INF(" EcoConnect IoT Gateway - Zephyr RTOS Initializing");
	LOG_INF(" Board : STM32F407G-DISC1 (ARM Cortex-M4 @ 168 MHz)");
	LOG_INF("====================================================");

	/* 1. Initialize User LED */
	if (!gpio_is_ready_dt(&led)) {
		LOG_ERR("Error: LED device %s is not ready", led.port->name);
		return -1;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		LOG_ERR("Error %d: Failed to configure LED pin", ret);
		return -1;
	}
	LOG_INF("[OK] GPIO Subsystem: Green LED (PD12) configured.");

	/* 2. Check I2C Sensor Bus */
	const struct device *const i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c1));
	if (!device_is_ready(i2c_dev)) {
		LOG_WRN("[WARN] I2C1 bus not ready or not initialized.");
	} else {
		LOG_INF("[OK] I2C1 Bus: Initialized on PB6 (SCL) / PB7 (SDA).");
	}

	/* 3. Check Ethernet Interface */
	struct net_if *iface = net_if_get_default();
	if (iface == NULL) {
		LOG_WRN("[WARN] Default network interface not yet online.");
	} else {
		LOG_INF("[OK] Network Interface: Online via ENC28J60 driver.");
	}

	LOG_INF("Starting main gateway heartbeat loop...");

	while (1) {
		ret = gpio_pin_toggle_dt(&led);
		if (ret < 0) {
			LOG_ERR("Failed to toggle LED");
		}
		k_msleep(SLEEP_TIME_MS);
	}

	return 0;
}
