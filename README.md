# EcoConnect IoT Gateway Firmware

An industrial IoT gateway firmware implementation for the STM32F407 platform, bridging environmental sensor nodes with IP networks via SPI Ethernet, developed as part of the System Design Masterclass.

---

## 1. Hardware Architecture & Board Profile

* **Host Microcontroller**: STMicroelectronics STM32F407VGT6 (ARM Cortex-M4 with FPU @ 168 MHz)
* **Development Platform**: STM32F407G-DISC1 (Discovery Board MB997)
* **On-Chip Flash**: 1024 KB @ `0x08000000`
* **On-Chip SRAM**: 192 KB total (128 KB continuous SRAM1/2 @ `0x20000000` + 64 KB CCM @ `0x10000000`)
* **Ethernet Controller**: Microchip ENC28J60 (Standalone 10Base-T Ethernet controller with 8 KB dual-port SRAM)
* **Sensor Bus**: I2C1 (BME280 / MPU6050 environmental/motion telemetry)

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

## 4. Reproducible Build Instructions

### Toolchain Prerequisites
* **ARM GCC Toolchain**: `arm-none-eabi-gcc` (Version 12.x or 14.x)
* **Build System**: GNU Make (`mingw32-make` or `make`)
* **Flash Programmer**: OpenOCD (with ST-Link v2 / v2-1 configuration)
* **RTOS Environment**: Zephyr West meta-tool (`west` version 1.2+)

---

### Target 1: Building & Flashing Bare-Metal Bring-Up

The bare-metal target provides zero-dependency register verification of the MCU, vector table, clock gating, and linker script layout.

```bash
# Navigate to the baremetal workspace
cd baremetal

# Build with GNU Make
make all

# Output footprint:
# text: ~380 bytes, data: 0 bytes, bss: 0 bytes

# Flash to silicon via OpenOCD
make flash
```

Alternatively on Windows PowerShell:
```powershell
.\build.bat
.\flash.bat
```

---

### Target 2: Building & Flashing Zephyr RTOS Application

This repository functions as a self-bootstrapping West manifest repository (`west.yml`). Anyone can clone and build the complete stack in two commands:

#### Option A: Quickstart from Scratch (Self-Bootstrapping Manifest)
```bash
# 1. Initialize a new workspace using this repository's West manifest
west init -m https://github.com/rohith-bn/eco-connect-gateway.git eco_workspace
cd eco_workspace

# 2. Pull all matching Zephyr dependencies (kernel, STM32 HAL, CMSIS)
west update

# 3. Build for STM32F4-Discovery
west build -b stm32f4_disco eco-connect-gateway/zephyr_app

# 4. Flash to silicon via OpenOCD / ST-LINK
west flash
```

#### Option B: Building Inside an Existing Zephyr Workspace
If you already have a Zephyr development workspace:
```bash
# Clone directly into your application directory
git clone https://github.com/rohith-bn/eco-connect-gateway.git

# Build and flash
west build -b stm32f4_disco eco-connect-gateway/zephyr_app
west flash
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

---

## 6. License & Authorship

Authored by **Rohith B Narasimhamurthy** as part of the System Design Masterclass.  
Licensed under the Apache-2.0 License.
