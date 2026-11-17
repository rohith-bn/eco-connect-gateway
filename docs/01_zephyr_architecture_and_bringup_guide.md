# EcoConnect IoT Gateway - Zephyr Architecture & Hardware Bring-Up Guide

## 1. System Overview

The **EcoConnect IoT Gateway** is an industrial-grade embedded gateway built on the **STM32F407G-DISC1** board running **Zephyr RTOS**. It reads high-precision environmental data from a Bosch BME280 sensor over I2C1, manages an Ethernet link via a Microchip ENC28J60 controller over SPI1, and serves real-time sensor metrics through an onboard HTTP web server on TCP port 80.

---

## 2. Hardware Pinout & Wiring

| Peripheral | Board Pin | Peripheral Signal | Description |
| :--- | :--- | :--- | :--- |
| **I2C1** | `PB6` | `SCL` | I2C Clock (100 kHz Standard Mode, Header P1) |
| **I2C1** | `PB7` | `SDA` | I2C Data (Header P1) |
| **SPI1** | `PA5` | `SCK` | SPI Clock (4 MHz, Header P1) |
| **SPI1** | `PA6` | `MISO` | SPI Master In / Slave Out (Header P1) |
| **SPI1** | `PA7` | `MOSI` | SPI Master Out / Slave In (Header P1) |
| **ENC28J60** | `PB12` | `CS` | Active Low Chip Select (Header P2) |
| **ENC28J60** | `PB11` | `INT` | Active Low Interrupt Line (EXTI11, Header P2) |
| **LED Green** | `PD12` | `GPIO` | Sampling Heartbeat Indicator |
| **LED Orange** | `PD13` | `GPIO` | Ethernet Carrier Link Indicator (ON when link is UP) |
| **LED Red** | `PD14` | `GPIO` | Error Indicator |
| **LED Blue** | `PD15` | `GPIO` | Network Activity Indicator |

---

## 3. Firmware Structure (`zephyr_app`)

```
zephyr_app/
├── CMakeLists.txt        # Links Zephyr and compiles C sources
├── prj.conf              # Kconfig: Enables I2C, SPI, ENC28J60 driver, TCP/IP, Sockets, Shell
├── app.overlay           # Devicetree: Configures I2C1, SPI1, pinmux, and ENC28J60 node
├── include/
│   ├── bme280_sensor.h   # BME280 driver API (init, read_temp)
│   ├── enc28j60_spi.h    # Low-level SPI probe API (silicon revision test)
│   └── http_server.h     # HTTP server API (thread spawn, set_temp)
└── src/
    ├── main.c            # Master coordinator, LEDs, Net Mgmt callbacks, sample loop
    ├── bme280_sensor.c   # I2C auto-detect, calibration trims, 32-bit integer polynomial
    ├── enc28j60_spi.c    # SPI reset, Bank 3 selection, EREVID validation
    └── http_server.c     # Zephyr BSD Sockets TCP server on port 80
```

---

## 4. Build, Flash & IDE Navigation

### 4.1 Building Firmware
```powershell
cd zephyr_app
west build -b stm32f4_disco
```

### 4.2 Flashing Hardware
```powershell
west flash
```

### 4.3 VS Code Navigation
The workspace configuration at `.vscode/c_cpp_properties.json` maps all Zephyr SDK header paths.
- **Go to Definition**: `F12` or `Ctrl + Click`
- **Navigate Back**: `Alt + Left Arrow`
- **Navigate Forward**: `Alt + Right Arrow`
