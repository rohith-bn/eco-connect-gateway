# EcoConnect IoT Gateway Firmware

A deterministic, resource-constrained edge telemetry gateway built on the **STM32F407** (ARM Cortex-M4 @ 168 MHz) running **Zephyr RTOS**. Engineered as an advanced systems engineering and edge architecture showcase bridging physical sensor nodes with IP networks via SPI Ethernet.

```text
 ┌──────────────────────────┐
 │   Bosch BME280 Sensor    │
 │ (Temperature / Pressure) │
 └─────────────┬────────────┘
               │ I2C1 (PB6/PB7 @ 400 kHz)
               ▼
 ┌────────────────────────────────────────────────────────┐
 │        STM32F407G-DISC1 (ARM Cortex-M4 @ 168 MHz)      │
 │  - Zephyr RTOS Multi-Threaded Kernel                   │
 │  - Native BSD Sockets HTTP Web Server (TCP Port 80)    │
 └─────────────┬──────────────────────────────────────────┘
               │ SPI1 (PA5/PA6/PA7 + CS PB12 @ 10.5 MHz)
               ▼
 ┌──────────────────────────┐         RJ45 Cat5 Cable
 │   Microchip ENC28J60     │─────────────────────────► Local LAN / Switch
 │  Standalone Ethernet MAC │                           (DHCPv4 Assigned)
 └──────────────────────────┘
```

---

## What This Project Contains

* **Dual-Stage Architecture**: Register-level bare-metal bringup (`baremetal/`) and production RTOS application (`zephyr_app/`).
* **Hardware-Independent Sensor Driver**: Bosch BME280/BMP280 I2C driver with factory trim NVM parsing and 32-bit integer polynomial compensation.
* **Industrial Network Subsystem**: Microchip ENC28J60 SPI Ethernet controller with silicon errata #5 mitigation and TCP sliding-window flow control.
* **Embedded HTTP Server**: Multi-threaded BSD Sockets server delivering live JSON telemetry on TCP port 80.
* **Self-Bootstrapping Manifest**: Single-command workspace and dependency management via `west.yml`.

---

## 4-Step Quickstart Guide

### 1. Hardware Connections
* **BME280 (I2C1)**: `PB6` (SCL), `PB7` (SDA), 3.3V, GND (with 4.7k pull-up resistors).
* **ENC28J60 (SPI1)**: `PA5` (SCK), `PA6` (MISO), `PA7` (MOSI), `PB12` (CS), `PB1` (INT), 3.3V, GND.
* **Network & PC**: Connect RJ45 cable to local router/switch; connect Mini-USB ST-LINK port to PC.
* *(Full pinout details: [`docs/01_zephyr_architecture_and_bringup_guide.md`](docs/01_zephyr_architecture_and_bringup_guide.md))*.

### 2. Build & Flash (Two Commands)
```bash
# 1. Initialize workspace and pull all Zephyr dependencies
west init -m https://github.com/rohith-bn/eco-connect-gateway.git eco_workspace
cd eco_workspace && west update

# 2. Build and flash the STM32F4-Discovery target
west build -b stm32f4_disco eco-connect-gateway/zephyr_app
west flash
```

### 3. Monitor Live Boot Telemetry
Connect any serial terminal to the ST-LINK COM port at **`115200 baud, 8-N-1`**:
```text
[00:00:00.010] <inf> app_main: === EcoConnect Gateway Booting (STM32F407G-DISC1) ===
[00:00:00.052] <inf> bme280_sensor: BMP/BME280 init @0x76 (ID:0x60)
[00:00:01.420] <inf> app_main: ENC28J60 Link UP at boot
[00:00:02.150] <inf> net_dhcpv4: Received IPv4 address: 192.168.1.145
[00:00:02.160] <inf> http_srv: HTTP Server listening on port 80...
```

### 4. View Live Telemetry in Browser
Open any web browser on your network and navigate to:
```text
http://192.168.1.145/
```
The gateway serves live JSON telemetry directly from STM32 SRAM:
```json
{
  "device": "EcoConnect-STM32F407",
  "status": "HEALTHY",
  "temperature_c": 24.35,
  "chip_id": "0x60",
  "uptime_sec": 42
}
```

---

## Engineering Documentation

For deep technical specifications, refer to the documentation suite in [`docs/`](docs/):

* **[Hardware Bring-Up & Pinout Guide](docs/01_zephyr_architecture_and_bringup_guide.md)** — Complete pin mapping, clock domains, and IDE navigation.
* **[Silicon Architecture & Driver Mechanics](docs/02_silicon_architecture_and_driver_mechanics.md)** — ENC28J60 8 KB SRAM circular ring buffer errata #5, TCP flow control, and POSIX errno conventions.
* **[Roadmap & System Evolution](docs/03_roadmap_and_system_evolution.md)** — Completed milestones, LittleFS flash logging, power-save state machine, and multi-radio expansion.

---

## License & Authorship

Authored by **Rohith B Narasimhamurthy** as an advanced embedded systems engineering and edge architecture showcase.  
Licensed under the Apache-2.0 License.
