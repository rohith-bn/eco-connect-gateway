# Silicon Architecture & Driver Mechanics

Deep-dive documentation covering silicon-level hardware mitigation, memory buffer partitioning, network flow control, and system error conventions implemented in the EcoConnect Gateway.

---

## 1. ENC28J60 8 KB SRAM Ring Buffer Partitioning & Errata #5

The Microchip ENC28J60 standalone Ethernet controller features an internal 8 KB dual-port SRAM shared between packet reception and transmission.

```text
 0x0000 ┌───────────────────────────────────────────────┐ ◄── ERXST (Fixed at 0x0000 per Errata #5)
        │                                               │
        │      Circular Receive Buffer (ERX)            │
        │      (Capacity: 6,144 Bytes / 6 KB)           │
        │                                               │
 0x17FF ├───────────────────────────────────────────────┤ ◄── ERXND (End of Receive Ring)
 0x1800 │                                               │ ◄── ETXST (Start of Transmit Buffer)
        │      Linear Transmit Buffer (ETX)             │
        │      (Capacity: 2,048 Bytes / 2 KB)           │
 0x1FFF └───────────────────────────────────────────────┘ ◄── ETXND (End of Transmit Buffer)
```

### Microchip Silicon Errata #5 Mitigation
- **The Silicon Bug**: When `ERXST` is set to any non-zero address, the internal receive hardware pointer calculation occasionally miscalculates the wrap-around boundary during circular buffer overflow, corrupting adjacent packet memory.
- **The Firmware Fix**: In our driver configuration, `ERXST` is permanently fixed at hardware address `0x0000`. The receive ring runs from `0x0000` to `0x17FF`, while the linear transmit buffer is allocated from `0x1800` to `0x1FFF`.

---

## 2. Speed-Matching & Layer 4 TCP Flow Control

An asymmetric speed bottleneck exists between the network wire and the host bus:
* **Physical Wire Rate**: 10BASE-T Ethernet transmits at **10 Mbps** (1.25 MB/s).
* **Host Interface**: STM32F407 communicates over SPI1 at **10.5 MHz half-duplex** (~0.5 MB/s effective throughput when factoring in protocol command bytes).

### Overrun Prevention Mechanism
If an external client bursts large UDP or TCP streams, the ENC28J60's internal 6 KB receive buffer would overflow before the host CPU could drain it across the SPI bus.

To prevent buffer exhaustion:
1. **TCP Receive Window Throttling**: The gateway advertises a constrained TCP sliding receive window (`CONFIG_NET_TCP_MAX_REQ_WIN=2048`).
2. **Backpressure**: As the internal `net_pkt` and `net_buf` pools fill during processing, the TCP layer throttles the sender, forcing the remote client to pace its transmissions to match the SPI bus throughput.
3. **Interrupt-Driven DMA Draining**: The ENC28J60 asserts `PB11` (`EXTI11`) on packet arrival, immediately waking Zephyr's dedicated Ethernet RX thread to drain SRAM frames into RAM.

---

## 3. POSIX Negative Errno Conventions

All sensor and communication drivers strictly return standard POSIX negative errno values defined in `<errno.h>`:

| Return Code | Macro | Operational Meaning |
| :---: | :--- | :--- |
| `0` | `SUCCESS` | Transfer completed nominally. |
| `-22` | `-EINVAL` | Invalid argument passed by caller (e.g. `NULL` buffer pointer). |
| `-19` | `-ENODEV` | Target peripheral or chip not responding on bus (e.g. I2C NACK on address). |
| `-5` | `-EIO` | Mid-transfer electrical communication or CRC checksum error. |
| `-16` | `-EBUSY` | Bus or DMA controller locked by another RTOS thread. |
| `-110`| `-ETIMEDOUT` | Hardware operation or network ACK missed deadline. |

This standard enables calling state machines to execute intelligent self-healing routines (e.g., retrying on `-EBUSY` vs triggering hardware failover on `-ENODEV`).
