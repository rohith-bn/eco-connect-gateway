# Roadmap & System Evolution

Multi-phase engineering progression, verified milestones, and future architectural expansion planned for the EcoConnect Gateway.

---

## 1. Multi-Phase Roadmap

```text
 [Phase 1: Bare Metal Bringup] ──► [Phase 2: RTOS & Networking] ──► [Phase 3: Storage & Control] ──► [Phase 4: Cloud & Energy]
           COMPLETED                            COMPLETED                         PLANNED                        PLANNED
```

---

## 2. Completed Milestones

### Phase 1: Silicon Bring-Up & Direct Register Verification
- [x] **Cortex-M4 CRT0**: Hand-written vector table (`startup.c`), memory map (`linker.ld`), and C runtime initialization.
- [x] **Direct MMIO Verification**: Standalone GCC Makefile building direct-register GPIO Blinky verifying clock gating and silicon integrity.

### Phase 2: Production RTOS & Network Integration
- [x] **Zephyr Devicetree Layer**: Hardware pinmux decoupled via `app.overlay` (I2C1 PB6/PB7, SPI1 PA5/PA6/PA7 + PB12 CS, EXTI PB11 INT).
- [x] **I2C Sensor Driver**: Bosch BME280/BMP280 driver with factory calibration NVM parsing and 32-bit integer polynomial compensation.
- [x] **SPI Ethernet Subsystem**: ENC28J60 driver integration over SPI with asynchronous link status callbacks.
- [x] **TCP/IP & DHCPv4**: Native Zephyr L2/L3 network stack integration with dynamic IP acquisition.
- [x] **Embedded HTTP Server**: Multi-threaded BSD Sockets web server serving live JSON sensor telemetry on port 80.
- [x] **Self-Bootstrapping Manifest**: Hermetic multi-repository dependency management via `west.yml`.

---

## 3. Future Scope & System Evolution

### Phase 3: Persistent Storage & Interactive Control (Next)
- [ ] **LittleFS Flash Storage**: Mount LittleFS partition on STM32 on-chip flash for `INI` configuration and offline sensor spooling during network dropouts.
- [ ] **Interactive HTTP REST API**: Expand HTTP server with endpoints (`/api/telemetry`, `/api/config`) for dynamic sample rate adjustment.
- [ ] **Interactive UART Shell**: Zephyr console shell providing runtime inspection (`net iface`, `net ping`, `sensor read`).

### Phase 4: Energy Optimization & Multi-Radio Ingestion (Advanced)
- [ ] **Low-Power Tickless Idle**: Autonomous power-save state machine entering STM32 Stop mode between 2-second sample intervals (< 2 mA), waking via RTC timer or Ethernet EXTI interrupt.
- [ ] **Pluggable Sensor Ingestion**: Abstract polymorphic vtable (`sensor_ingest_if_t`) enabling hot-swapping local I2C for remote Wi-Fi, BLE, or LoRa sensor nodes.
- [ ] **Crash-Safe Dual-Slot OTA Bootloader**: Dual-bank flash staging, CRC32/SHA-256 validation, and watchdog auto-rollback.
- [ ] **Cloud Telemetry (MQTT-SN / TLS)**: Streaming JSON telemetry to an MQTT broker with mbedTLS encryption.
