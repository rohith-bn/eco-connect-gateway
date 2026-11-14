cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(eco_connect_gateway)

target_include_directories(app PRIVATE include)

target_sources(app PRIVATE
	src/main.c
	src/bme280_sensor.c
	src/enc28j60_spi.c
	src/http_server.c
)
