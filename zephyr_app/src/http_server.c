#include "http_server.h"

#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>

LOG_MODULE_REGISTER(http_srv, CONFIG_HTTP_SRV_MOD_LOG_LEVEL);

#define HTTP_PORT        80
#define HTTP_STACK_SIZE  4096
#define HTTP_PRIORITY    7

static volatile int32_t g_latest_temp_x100 = 0;

void http_server_set_temp_x100(int32_t temp_c_x100)
{
	g_latest_temp_x100 = temp_c_x100;
}

static void http_server_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	int srv_sock = zsock_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (srv_sock < 0) {
		LOG_ERR("Failed to create TCP socket (err=%d)", errno);
		return;
	}

	struct sockaddr_in bind_addr = {
		.sin_family = AF_INET,
		.sin_addr.s_addr = htonl(INADDR_ANY),
		.sin_port = htons(HTTP_PORT),
	};

	if (zsock_bind(srv_sock, (struct sockaddr *)&bind_addr, sizeof(bind_addr)) < 0) {
		LOG_ERR("TCP bind on port %d failed (err=%d)", HTTP_PORT, errno);
		zsock_close(srv_sock);
		return;
	}

	if (zsock_listen(srv_sock, 2) < 0) {
		LOG_ERR("TCP listen failed (err=%d)", errno);
		zsock_close(srv_sock);
		return;
	}

	LOG_INF("HTTP server listening on TCP port %d", HTTP_PORT);

	while (1) {
		struct sockaddr_in client_addr;
		socklen_t client_len = sizeof(client_addr);
		int client = zsock_accept(srv_sock, (struct sockaddr *)&client_addr, &client_len);
		if (client < 0) {
			LOG_WRN("zsock_accept error (err=%d)", errno);
			k_msleep(100);
			continue;
		}

		char addr_str[NET_IPV4_ADDR_LEN] = {0};
		zsock_inet_ntop(AF_INET, &client_addr.sin_addr, addr_str, sizeof(addr_str));
		LOG_DBG("TCP client connected from %s:%u", addr_str, ntohs(client_addr.sin_port));

		char rx_buf[256];
		int rx_len = zsock_recv(client, rx_buf, sizeof(rx_buf) - 1, 0);
		if (rx_len > 0) {
			rx_buf[rx_len] = '\0';
			LOG_DBG("HTTP request (%d bytes) received", rx_len);
		}

		int32_t t = g_latest_temp_x100;
		int32_t whole = t / 100;
		int32_t frac = (t < 0 ? -t : t) % 100;

		char body[384];
		int body_len = snprintf(body, sizeof(body),
			"<!DOCTYPE html><html><head><title>EcoConnect Gateway</title>"
			"<meta charset=\"utf-8\"></head>"
			"<body style=\"font-family:sans-serif;margin:2rem;\">"
			"<h1>EcoConnect IoT Gateway</h1>"
			"<p><b>Platform:</b> STM32F407G-DISC1 + Zephyr RTOS</p>"
			"<p><b>Network:</b> Microchip ENC28J60 (SPI1) + TCP/IP</p>"
			"<p><b>Sensor Temp:</b> %d.%02d &deg;C</p>"
			"<p><b>Status:</b> Link Active</p>"
			"</body></html>",
			whole, frac);

		char hdr[160];
		int hdr_len = snprintf(hdr, sizeof(hdr),
			"HTTP/1.1 200 OK\r\n"
			"Content-Type: text/html\r\n"
			"Content-Length: %d\r\n"
			"Connection: close\r\n\r\n",
			body_len);

		zsock_send(client, hdr, hdr_len, 0);
		zsock_send(client, body, body_len, 0);
		zsock_close(client);
	}
}

K_THREAD_STACK_DEFINE(http_srv_stack, HTTP_STACK_SIZE);
static struct k_thread http_srv_thread_data;

void http_server_start(void)
{
	k_thread_create(&http_srv_thread_data, http_srv_stack,
			K_THREAD_STACK_SIZEOF(http_srv_stack),
			http_server_thread, NULL, NULL, NULL,
			HTTP_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&http_srv_thread_data, "http_srv");
}
