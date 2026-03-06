# BQ25619 Arduino / ESP32 Library

Arduino and ESP32 library for the **TI BQ25619** I2C-controlled 1-cell 1.5A buck battery charger.

Based on TI datasheet **SLUSDF8 – June 2019**.

---

## Installation (PlatformIO)

Copy the `BQ25619` folder into your project's `lib/` directory:

```
your_project/
  lib/
    BQ25619/
      src/
        BQ25619.h
        BQ25619.cpp
      library.properties
```

No `lib_deps` entry needed — PlatformIO auto-discovers libraries in `lib/`.

---

## Quick Start

```cpp
#include <Wire.h>
#include "BQ25619.h"

BQ25619 charger(0x6A); // 0x6A = ADDR pin to GND, 0x6B = ADDR pin to VCC

void setup() {
    // ESP32: configure custom SDA/SCL pins before calling begin().
    // Wire.begin(SDA, SCL) must be called first — the library does not
    // call it internally so you retain full control over pin assignment.
    Wire.begin(21, 22); // SDA=21, SCL=22
    charger.begin(&Wire);
    charger.setInputCurrentLimit(500);   // mA
    charger.setChargeCurrent(325);       // mA
    charger.setPrechargeCurrent(65);     // mA
    charger.setTerminationCurrent(32);   // mA
    charger.setChargeVoltage(4200);      // mV — DO NOT exceed 4200 for LiPo
    charger.setInputVoltageLimit(4500);  // mV
    charger.setThermalThreshold(TREG_100C);
    charger.setWatchdogTimer(WATCHDOG_DISABLED);
    charger.setSafetyTimer(true, SAFETY_TIMER_5H);
    charger.enableTermination(true);
    charger.enableStatPin(true);
    charger.enableCharging(true);
}
```

---

## API Reference

### Initialisation
| Method | Description |
|--------|-------------|
| `begin()` | Initialises I2C and verifies device is present. Returns `false` if not found. |

### Configuration
| Method | Parameters | Description |
|--------|-----------|-------------|
| `setInputCurrentLimit(mA)` | 100–3200 | Input current limit (IINDPM), 100mA steps |
| `setChargeCurrent(mA)` | 0–1500 | Fast charge current (ICHG), 25mA steps |
| `setPrechargeCurrent(mA)` | 25–400 | Precharge current (IPRECHG), 25mA steps |
| `setTerminationCurrent(mA)` | 5–80 | Charge termination current (ITERM), 5mA steps |
| `setChargeVoltage(mV)` | 3856–4608 | Charge regulation voltage (VREG), 10mV steps |
| `setInputVoltageLimit(mV)` | 3900–5400 | VINDPM threshold, 100mV steps |
| `setThermalThreshold(t)` | enum | TREG_60C / 80C / 100C / 120C |
| `setWatchdogTimer(t)` | enum | WATCHDOG_DISABLED / 40S / 80S / 160S |
| `setSafetyTimer(enable, duration)` | bool + enum | SAFETY_TIMER_5H / 8H / 12H / 20H |
| `enableCharging(bool)` | — | Enable or disable charging |
| `enableTermination(bool)` | — | Enable termination detection |
| `enableStatPin(bool)` | — | Enable STAT LED output |
| `resetRegisters()` | — | Reset all registers to datasheet defaults |

### Status
| Method | Returns | Description |
|--------|---------|-------------|
| `getStatus()` | `BQ25619_Status` | Full status struct (charge state, VBUS, PG, etc.) |
| `getFaults()` | `BQ25619_Faults` | Fault struct — **clears fault register on read** |
| `isCharging()` | bool | True if pre-charge or fast-charging |
| `isChargeComplete()` | bool | True if charge complete |
| `isPowerGood()` | bool | True if valid input present |
| `hasFault()` | bool | True if any fault bit set |

### Read-back
| Method | Returns | Description |
|--------|---------|-------------|
| `getChargeCurrent()` | uint16_t mA | Current ICHG setting |
| `getChargeVoltage()` | uint16_t mV | Current VREG setting |
| `getInputCurrentLimit()` | uint16_t mA | Current IINDPM setting |

### Debug
| Method | Description |
|--------|-------------|
| `dumpRegisters()` | Prints all registers + decoded fields to Serial |

### Raw access
| Method | Description |
|--------|-------------|
| `writeReg(reg, value)` | Write raw byte to register |
| `readReg(reg)` | Read raw byte from register (returns 0xFF on I2C error) |
| `updateReg(reg, mask, value)` | Read-modify-write |

---

## STAT Pin LED Behaviour
| State | STAT Pin |
|-------|----------|
| Actively charging | LOW (LED on) |
| Charge complete / no input | HIGH-Z (LED off) |
| Fault | Blinking 1Hz |

---

## Notes
- **VREG = 4200mV is critical.** Never set higher for 1S LiPo.
- The fault register (REG09) clears on every read. Read once per check cycle and store the result.
- With watchdog disabled, `begin()` only needs to be called once on boot.
- Values out of range are automatically clamped to min/max.
