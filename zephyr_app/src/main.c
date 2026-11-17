/*
 * EcoConnect IoT Gateway - Week 2 Stage 2 (ENC28J60 Zephyr Driver + TCP/IP + HTTP)
 * Target: STM32F407G-DISC1 (ARM Cortex-M4 @ 168 MHz)
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_context.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/dhcpv4.h>

#include "bme280_sensor.h"
#if !defined(CONFIG_ETH_ENC28J60)
#include "enc28j60_spi.h"
#endif
#include "http_server.h"

LOG_MODULE_REGISTER(app_main, CONFIG_APP_MAIN_LOG_LEVEL);

#define SAMPLE_PERIOD_MS 2000

static const struct gpio_dt_spec led_green  = GPIO_DT_SPEC_GET(DT_NODELABEL(green_led_4), gpios);
static const struct gpio_dt_spec led_orange = GPIO_DT_SPEC_GET(DT_NODELABEL(orange_led_3), gpios);
static const struct gpio_dt_spec led_red    = GPIO_DT_SPEC_GET(DT_NODELABEL(red_led_5), gpios);
static const struct gpio_dt_spec led_blue   = GPIO_DT_SPEC_GET(DT_NODELABEL(blue_led_6), gpios);

static struct net_mgmt_event_callback mgmt_cb;

static void net_event_handler(struct net_mgmt_event_callback *cb,
			      uint64_t mgmt_event, struct net_if *iface)
{
	ARG_UNUSED(cb);

	if (mgmt_event == NET_EVENT_IF_UP) {
		LOG_INF("Ethernet interface UP (Carrier detected)");
		gpio_pin_set_dt(&led_orange, 1);
		return;
	}

	if (mgmt_event == NET_EVENT_IF_DOWN) {
		LOG_WRN("Ethernet interface DOWN (Carrier lost)");
		gpio_pin_set_dt(&led_orange, 0);
		return;
	}

	if (mgmt_event == NET_EVENT_IPV4_ADDR_ADD) {
		char buf[NET_IPV4_ADDR_LEN];

		for (int i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
			struct net_if_addr *if_addr = &iface->config.ip.ipv4->unicast[i].ipv4;

			if (if_addr->addr_type != NET_ADDR_DHCP) {
				continue;
			}

			LOG_INF("DHCPv4 IPv4 Address: %s",
				net_addr_ntop(AF_INET, &if_addr->address.in_addr,
					      buf, sizeof(buf)));
			LOG_INF("DHCPv4 Netmask:      %s",
				net_addr_ntop(AF_INET,
					      &iface->config.ip.ipv4->unicast[i].netmask,
					      buf, sizeof(buf)));
			LOG_INF("DHCPv4 Gateway:      %s",
				net_addr_ntop(AF_INET, &iface->config.ip.ipv4->gw,
					      buf, sizeof(buf)));
		}
	}
}

int main(void)
{
	gpio_pin_configure_dt(&led_green, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&led_orange, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&led_red, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&led_blue, GPIO_OUTPUT_INACTIVE);

	const struct device *const i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c1));

	LOG_INF("=== EcoConnect Gateway Booting (STM32F407G-DISC1) ===");

#if !defined(CONFIG_ETH_ENC28J60)
	const struct device *const spi_dev = DEVICE_DT_GET(DT_NODELABEL(spi1));
	uint8_t enc_rev = 0;
	bool enc_ok = (enc28j60_spi_probe_version(spi_dev, &enc_rev) == 0);
	gpio_pin_set_dt(&led_orange, enc_ok ? 1 : 0);
#else
	/* Zephyr's eth_enc28j60 driver owns SPI1 and initializes ENC28J60 at boot */
	net_mgmt_init_event_callback(&mgmt_cb, net_event_handler,
				     NET_EVENT_IF_UP | NET_EVENT_IF_DOWN |
				     NET_EVENT_IPV4_ADDR_ADD);
	net_mgmt_add_event_callback(&mgmt_cb);

	struct net_if *iface = net_if_get_default();
	if (iface != NULL) {
		if (net_if_is_carrier_ok(iface)) {
			LOG_INF("ENC28J60 Link UP at boot");
			gpio_pin_set_dt(&led_orange, 1);
		} else {
			LOG_INF("ENC28J60 initialized, waiting for Ethernet link...");
		}
		net_dhcpv4_start(iface);
	} else {
		LOG_ERR("No default network interface found!");
		gpio_pin_set_dt(&led_red, 1);
	}

	http_server_start();
#endif

	uint8_t bme_id = 0;
	bool bme_ok = (bme280_sensor_init(i2c_dev, &bme_id) == 0);
	gpio_pin_set_dt(&led_blue, bme_ok ? 1 : 0);

	while (1) {
		gpio_pin_toggle_dt(&led_green);

		if (bme_ok) {
			int32_t temp_c_x100 = 0;
			if (bme280_sensor_read_temp(i2c_dev, &temp_c_x100) == 0) {
				http_server_set_temp_x100(temp_c_x100);
			}
		}

		k_msleep(SAMPLE_PERIOD_MS);
	}

	return 0;
}
