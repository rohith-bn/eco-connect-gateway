# EcoConnect IoT Gateway Firmware

An industrial IoT gateway firmware implementation for the STM32F407 platform, bridging environmental sensor nodes with IP networks via SPI Ethernet, developed as part of the System Design Masterclass.

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

## 1. Hardware Architecture & Board Profile

* **Host Microcontroller**: STMicroelectronics STM32F407VGT6 (ARM Cortex-M4 with FPU @ 168 MHz)
* **Development Platform**: STM32F407G-DISC1 (Discovery Board MB997)
* **On-Chip Flash**: 1024 KB @ `0x08000000`
* **On-Chip SRAM**: 192 KB total (128 KB continuous SRAM1/2 @ `0x20000000` + 64 KB CCM @ `0x10000000`)
* **Ethernet Controller**: Microchip ENC28J60 (Standalone 10Base-T Ethernet controller with 8 KB dual-port SRAM)
* **Sensor Bus**: I2C1 (BME280 / BMP280 environmental telemetry)

---

## 2. Hardware Pinout & Signal Mapping

| Subsystem | Signal Name | MCU Pin | Alternate Function | Bus Domain | Physical Connection |
| :--- | :--- | :---: | :---: | :---: | :--- |
| **System Heartbeat**| LED4 (Green) | `PD12` | None (GPIO Output) | AHB1 (168 MHz)| On-board Discovery LED |
| **I2C Sensor** | `I2C1_SCL` | `PB6` | `AF04` (Open-Drain)| APB1 (42 MHz) | External 4.7k Pull-up to 3.3V |
| **I2C Sensor** | `I2C1_SDA` | `PB7` | `AF04` (Open-Drain)| APB1 (42 MHz) | External 4.7k Pull-up to 3.3V |
| **SPI Ethernet** | `SPI1_SCK` | `PA5` | `AF05` (Push-Pull) | APB2 (84 MHz) | ENC28J60 SCK (10.5 MHz) |
| **SPI Ethernet** | `SPI1_MISO`| `PA6` | `AF05` (Floating) | APB2 (84 MHz) | ENC28J60 SO |
| **SPI Ethernet** | `SPI1_MOSI`| `PA7` | `AF05` (Push-Pull) | APB2 (84 MHz) | ENC28J60 SI |
| **SPI Ethernet** | `SPI_CS` | `PB12` | None (GPIO Output) | AHB1 (168 MHz)| ENC28J60 CS (Active Low) |
| **SPI Ethernet** | `ENC_INT` | `PB1` | None (EXTI Input) | AHB1 (168 MHz)| ENC28J60 INT (Active Low) |
| **Power** | `VDD` / `GND`| `3V3`/`GND` | N/A | Power Plane | Clean 3.3V power (180 mA) |

---

## 3. Repository Structure

```text
eco-connect-gateway/
├── .clangd                          # LLVM Clangd driver flag filtering
├── .gitignore                       # Clean Git exclusion rules
├── west.yml                         # Self-bootstrapping West manifest
├── README.md                        # Master architectural documentation
├── docs/                            # Deep engineering specifications
│   └── 01_zephyr_architecture_and_bringup_guide.md
├── baremetal/                       # Stage 1: Bare-Metal Silicon Bring-Up
│   ├── Makefile                     # Standalone GCC build script
│   ├── build.bat                    # Windows convenience build script
│   ├── flash.bat                    # OpenOCD programming script
│   ├── linker.ld                    # Cortex-M4 linker memory map
│   ├── startup.c                    # Vector table, reset handler, CRT0
│   ├── registers.h                  # Direct MMIO register definitions
│   └── main.c                       # Register-level GPIO blinky
└── zephyr_app/                      # Stage 2: Production Zephyr RTOS Application
    ├── CMakeLists.txt               # Zephyr application CMake definition
    ├── prj.conf                     # Kconfig kernel, network, and driver selection
    ├── app.overlay                  # Devicetree hardware pin mapping
    ├── Kconfig                      # Application-level Kconfig logging menus
    ├── include/                     # Public driver contracts
    │   ├── bme280_sensor.h          # Bosch BME280 I2C API
    │   ├── enc28j60_spi.h           # Microchip ENC28J60 SPI API
    │   └── http_server.h            # BSD Sockets HTTP server API
    └── src/                         # Modular C implementations
        ├── bme280_sensor.c          # I2C driver & 32-bit polynomial compensation
        ├── enc28j60_spi.c           # SPI hardware probe & revision reader
        ├── http_server.c            # Multithreaded TCP HTTP server
        └── main.c                   # System coordinator & network event listener
```

---

## 4. Step-by-Step Quickstart Guide

This repository functions as a **self-bootstrapping West manifest repository (`west.yml`)**. Anyone can clone, build, flash, and verify the gateway in four sequential steps.

### Step 1: Hardware Connections
1. Wire the **BME280** to MCU pins `PB6` (SCL) and `PB7` (SDA) with 4.7k pull-up resistors to 3.3V.
2. Wire the **ENC28J60** SPI bus to MCU pins `PA5` (SCK), `PA6` (MISO), `PA7` (MOSI), and `PB12` (CS).
3. Connect an Ethernet cable from the ENC28J60 module into your local router or network switch.
4. Connect the STM32F4-Discovery board to your PC via Mini-USB (ST-LINK port).

### Step 2: Build & Flash Firmware
Open your terminal and run:

```bash
# 1. Initialize a new workspace using this repository's West manifest
west init -m https://github.com/rohith-bn/eco-connect-gateway.git eco_workspace
cd eco_workspace

# 2. Pull all matching Zephyr dependencies (kernel, STM32 HAL, CMSIS)
west update

# 3. Build the Zephyr application for STM32F4-Discovery
west build -b stm32f4_disco eco-connect-gateway/zephyr_app

# 4. Flash to target silicon via OpenOCD / ST-LINK
west flash
```

*(Note: For standalone bare-metal bringup without RTOS, see [`baremetal/`](baremetal/) where running `make all && make flash` flashes the direct register target).*

### Step 3: Monitor Live Boot Telemetry
Connect any serial terminal (e.g., VS Code Serial Monitor, PuTTY, or Minicom) to your ST-LINK virtual COM port:
* **Baud Rate**: `115200`
* **Data Bits**: `8`
* **Parity**: `None`
* **Stop Bits**: `1`

You will observe structured, per-module boot diagnostics:
```text
*** Booting Zephyr OS build v4.5.0-1994-g0c63b6bfeb0d ***
[00:00:00.010] <inf> app_main: === EcoConnect Gateway Booting (STM32F407G-DISC1) ===
[00:00:00.052] <inf> bme280_sensor: BMP/BME280 init @0x76 (ID:0x60, imm:0x27, stat:0x00)
[00:00:00.053] <dbg> bme280_sensor: Trims: T1=27845, T2=26481, T3=-1000
[00:00:00.060] <inf> eth_enc28j60: MAC 00:04:a3:11:22:33 initialized
[00:00:01.420] <inf> app_main: ENC28J60 Link UP at boot
[00:00:02.150] <inf> net_dhcpv4: Received IPv4 address: 192.168.1.145
[00:00:02.155] <inf> app_main: IPv4 address acquired: 192.168.1.145
[00:00:02.160] <inf> http_srv: HTTP Server thread started on TCP port 80
[00:00:04.050] <inf> bme280_sensor: Temperature: 24.35 C (raw_adc=0x0825F0)
```

### Step 4: Access the Live Web Server
Open any web browser on your local network (laptop or phone) and navigate to:
```text
http://192.168.1.145/
```
The embedded gateway responds with live JSON telemetry:
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

## 5. Architectural Highlights

1. **Dual Bring-Up Verification**:
   * *Stage 1 (Bare Metal)*: Direct memory-mapped I/O to verify physical silicon, clocks, and GPIO pins with zero operating system overhead.
   * *Stage 2 (Zephyr RTOS)*: Modular driver model with Devicetree overlays decoupling hardware from business logic.
2. **ENC28J60 Circular Buffer Mechanics**:
   * Dual-port 8 KB SRAM partitioned into a circular receive ring buffer (`0x0000` to `0x17FF`) and linear transmit buffer (`0x1800` to `0x1FFF`).
   * Adherence to Microchip silicon errata #5 by fixing `ERXST` at `0x0000` to prevent pointer wrap memory corruption.
3. **Flow Control Architecture**:
   * Bridges the speed mismatch between the 10 Mbps wire rate and the 10.5 MHz half-duplex SPI host bus using Layer 4 TCP sliding receive window (`rcv_wnd`) throttling to prevent internal SRAM overrun.
4. **POSIX System Error Conventions**:
   * Sensor and network drivers strictly return standard POSIX negative errno values (`-EINVAL`, `-ENODEV`, `-EIO`), enabling caller state machines to execute autonomous recovery routines.

---

## 6. Project Roadmap & Implementation Milestones

```text
 [Phase 1: Bare Metal Bringup] ──► [Phase 2: RTOS & Networking] ──► [Phase 3: Cloud & Security]
           COMPLETED                            COMPLETED                         PLANNED
```

### Completed Milestones
* [x] **Milestone 1 (Stage 1 Bring-Up)**: Bare-metal Cortex-M4 startup file (`startup.c`), linker script (`linker.ld`), and register-level MMIO GPIO blinky with custom Makefile.
* [x] **Milestone 2 (Devicetree & Pinout)**: Zephyr RTOS overlay (`app.overlay`) mapping I2C1 (PB6/PB7) and SPI1 (PA5/PA6/PA7 + PB12 CS).
* [x] **Milestone 3 (I2C Sensor Driver)**: Bosch BME280/BMP280 auto-detect driver with 32-bit fixed-point integer polynomial compensation.
* [x] **Milestone 4 (Ethernet Subsystem)**: ENC28J60 driver integration over SPI with asynchronous link status callbacks.
* [x] **Milestone 5 (TCP/IP & DHCPv4)**: Native Zephyr IP stack integration with dynamic IP acquisition.
* [x] **Milestone 6 (Embedded HTTP Server)**: Multithreaded BSD Sockets HTTP server serving JSON telemetry.
* [x] **Milestone 7 (West Manifest & CI Ready)**: Automated multi-repository dependency management via `west.yml`.

### Future Scope & Planned Next Steps
* [ ] **MQTT-SN / MQTT Telemetry Client**: Publish sensor telemetry to AWS IoT Core or local Mosquitto broker via lightweight publish-subscribe.
* [ ] **TLS 1.3 / HTTPS Encryption**: Integrate mbedTLS socket credentials for authenticated cloud ingestion.
* [ ] **LittleFS Local Flash Logging**: Persistent ring buffer logging of sensor readings during network disconnections.
* [ ] **Low-Power Tickless Idle**: Enable STM32 Stop mode during sleep intervals, waking via EXTI line interrupts.

---

## 7. License & Authorship

Authored by **Rohith B Narasimhamurthy** as part of the System Design Masterclass.  
Licensed under the Apache-2.0 License.
