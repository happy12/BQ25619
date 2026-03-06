#pragma once
#include <Arduino.h>
#include <Wire.h>

// ============================================================
//  BQ25619 Arduino/ESP32 Library
//  Based on TI BQ25619 Datasheet SLUSDF8 – JUNE 2019
//  I2C address: 0x6A (ADDR=GND) or 0x6B (ADDR=VCC)
// ============================================================

// --- Register Addresses (from datasheet Table 1) ---
#define BQ25619_REG00   0x00   // Input Source Control
#define BQ25619_REG01   0x01   // Power-On Configuration
#define BQ25619_REG02   0x02   // Charge Current Control
#define BQ25619_REG03   0x03   // Pre-Charge / Termination Current
#define BQ25619_REG04   0x04   // Charge Voltage Control
#define BQ25619_REG05   0x05   // Charge Termination / Timer
#define BQ25619_REG06   0x06   // Thermal Regulation / VINDPM
#define BQ25619_REG07   0x07   // Misc Operation Control
#define BQ25619_REG08   0x08   // System Status (read-only)
#define BQ25619_REG09   0x09   // Fault Register (read-only, clears on read)
#define BQ25619_REG0A   0x0A   // Vendor / Part / Revision (read-only)

// --- REG00: IINDPM input current limit (bits[5:0]), step = 100mA, offset = 100mA ---
// range: 100mA to 3200mA
#define BQ25619_IINDPM_MIN_MA    100
#define BQ25619_IINDPM_MAX_MA   3200
#define BQ25619_IINDPM_STEP_MA   100

// --- REG02: ICHG fast charge current (bits[6:0]), step = 25mA, offset = 0 ---
// range: 0mA to 1500mA (Note: values above 1500mA not supported)
#define BQ25619_ICHG_MIN_MA      0
#define BQ25619_ICHG_MAX_MA   1500
#define BQ25619_ICHG_STEP_MA    25

// --- REG03: IPRECHG precharge current (bits[7:4]), step = 25mA, offset = 25mA ---
// range: 25mA to 400mA
#define BQ25619_IPRECHG_MIN_MA    25
#define BQ25619_IPRECHG_MAX_MA   400
#define BQ25619_IPRECHG_STEP_MA   25

// --- REG03: ITERM termination current (bits[3:0]), step = 5mA, offset = 5mA ---
// range: 5mA to 80mA
#define BQ25619_ITERM_MIN_MA     5
#define BQ25619_ITERM_MAX_MA    80
#define BQ25619_ITERM_STEP_MA    5

// --- REG04: VREG charge voltage (bits[7:2]), step = 10mV, offset = 3856mV ---
// range: 3856mV to 4608mV
#define BQ25619_VREG_MIN_MV   3856
#define BQ25619_VREG_MAX_MV   4608
#define BQ25619_VREG_STEP_MV    10

// --- REG06: VINDPM threshold (bits[3:0]), step = 100mV, offset = 3900mV ---
// range: 3900mV to 5400mV
#define BQ25619_VINDPM_MIN_MV   3900
#define BQ25619_VINDPM_MAX_MV   5400
#define BQ25619_VINDPM_STEP_MV   100

// --- REG08 Status bits ---
#define BQ25619_VBUS_STAT_MASK    0xE0
#define BQ25619_VBUS_STAT_SHIFT   5
#define BQ25619_CHRG_STAT_MASK    0x18
#define BQ25619_CHRG_STAT_SHIFT   3
#define BQ25619_PG_STAT_BIT       0x04
#define BQ25619_THERM_STAT_BIT    0x02
#define BQ25619_VSYS_STAT_BIT     0x01

// --- REG09 Fault bits ---
#define BQ25619_WATCHDOG_FAULT_BIT   0x80
#define BQ25619_BOOST_FAULT_BIT      0x40
#define BQ25619_CHRG_FAULT_MASK      0x30
#define BQ25619_CHRG_FAULT_SHIFT     4
#define BQ25619_BAT_FAULT_BIT        0x08
#define BQ25619_NTC_FAULT_MASK       0x07

// --- Charge status values (REG08 CHRG_STAT) ---
enum BQ25619_ChargeStatus {
    CHARGE_STATUS_NOT_CHARGING  = 0,
    CHARGE_STATUS_PRE_CHARGE    = 1,
    CHARGE_STATUS_FAST_CHARGING = 2,
    CHARGE_STATUS_COMPLETE      = 3
};

// --- VBUS status values (REG08 VBUS_STAT) ---
enum BQ25619_VBUSStatus {
    VBUS_NO_INPUT    = 0,
    VBUS_USB_SDP     = 1,
    VBUS_ADAPTER     = 3,
    VBUS_OTG         = 7
};

// --- Charge fault values (REG09 CHRG_FAULT) ---
enum BQ25619_ChargeFault {
    CHRG_FAULT_NONE          = 0,
    CHRG_FAULT_INPUT_OVP     = 1,
    CHRG_FAULT_THERMAL       = 2,
    CHRG_FAULT_SAFETY_TIMER  = 3
};

// --- NTC fault values (REG09 NTC_FAULT) ---
enum BQ25619_NTCFault {
    NTC_FAULT_NONE    = 0,
    NTC_FAULT_WARM    = 1,
    NTC_FAULT_COOL    = 2,
    NTC_FAULT_COLD    = 3,
    NTC_FAULT_HOT     = 5,
    NTC_FAULT_VERY_HOT = 6
};

// --- Watchdog timer options (REG05 bits[4:3]) ---
enum BQ25619_WatchdogTimer {
    WATCHDOG_DISABLED = 0,
    WATCHDOG_40S      = 1,
    WATCHDOG_80S      = 2,
    WATCHDOG_160S     = 3
};

// --- Safety timer options (REG05 bits[2:1]) ---
enum BQ25619_SafetyTimer {
    SAFETY_TIMER_5H  = 0,
    SAFETY_TIMER_8H  = 1,
    SAFETY_TIMER_12H = 2,
    SAFETY_TIMER_20H = 3
};

// --- Thermal regulation threshold (REG06 bits[1:0]) ---
enum BQ25619_ThermalThreshold {
    TREG_60C  = 0,
    TREG_80C  = 1,
    TREG_100C = 2,
    TREG_120C = 3
};

// --- Status struct returned by getStatus() ---
struct BQ25619_Status {
    BQ25619_VBUSStatus   vbusStatus;
    BQ25619_ChargeStatus chargeStatus;
    bool                 powerGood;
    bool                 inThermalRegulation;
    bool                 inVSYSRegulation;
};

// --- Fault struct returned by getFaults() ---
struct BQ25619_Faults {
    bool                 watchdogFault;
    bool                 boostFault;
    BQ25619_ChargeFault  chargeFault;
    bool                 batteryFault;
    BQ25619_NTCFault     ntcFault;
    bool                 anyFault;
};

// ============================================================
class BQ25619 {
public:
    BQ25619(uint8_t address = 0x6A, TwoWire &wire = Wire);

    // Initialisation
    // All arguments optional:
    //   charger.begin()                  — use address + Wire set in constructor
    //   charger.begin(&Wire1)            — override Wire bus only
    //   charger.begin(&Wire1, 0x6B)      — override both Wire bus and address
    bool begin(TwoWire *wire = nullptr, uint8_t address = 0x00);

    // --- Configuration setters ---
    // All return true on successful I2C write

    // REG00: Input current limit (100mA to 3200mA, clamped to range)
    bool setInputCurrentLimit(uint16_t mA);

    // REG02: Fast charge current (0mA to 1500mA, clamped, 25mA steps)
    bool setChargeCurrent(uint16_t mA);

    // REG03: Precharge current (25mA to 400mA, clamped, 25mA steps)
    bool setPrechargeCurrent(uint16_t mA);

    // REG03: Termination current (5mA to 80mA, clamped, 5mA steps)
    bool setTerminationCurrent(uint16_t mA);

    // REG04: Charge voltage (3856mV to 4608mV, clamped, 10mV steps)
    bool setChargeVoltage(uint16_t mV);

    // REG06: Input voltage limit / VINDPM (3900mV to 5400mV, 100mV steps)
    bool setInputVoltageLimit(uint16_t mV);

    // REG06: Thermal regulation threshold
    bool setThermalThreshold(BQ25619_ThermalThreshold threshold);

    // REG05: Watchdog timer (use WATCHDOG_DISABLED to turn off)
    bool setWatchdogTimer(BQ25619_WatchdogTimer setting);

    // REG05: Safety timer enable/disable and duration
    bool setSafetyTimer(bool enable, BQ25619_SafetyTimer duration = SAFETY_TIMER_5H);

    // REG07: Enable or disable charging
    bool enableCharging(bool enable);

    // REG07: Enable or disable charge termination detection
    bool enableTermination(bool enable);

    // REG07: Enable or disable STAT pin (LED control)
    bool enableStatPin(bool enable);

    // REG01: Reset all registers to default (sets REG_RST bit, auto-clears)
    bool resetRegisters();

    // --- Status / fault readers ---
    BQ25619_Status  getStatus();
    BQ25619_Faults  getFaults();

    // Convenience wrappers
    bool isCharging();
    bool isChargeComplete();
    bool isPowerGood();
    bool hasFault();

    // --- Configuration readers (read back from registers) ---
    uint16_t getInputCurrentLimit();   // mA
    uint16_t getChargeCurrent();       // mA
    uint16_t getChargeVoltage();       // mV

    // --- Part identification (REG0A) ---
    // getICversion(): returns (PN[2:0] << 8) | DEV_REV[3:0]
    uint16_t getICversion();
    // getChipID(): returns PN[2:0] (part number field of REG0A)
    uint8_t  getChipID();

    // --- Raw register access (advanced use) ---
    bool    writeReg(uint8_t reg, uint8_t value);
    uint8_t readReg(uint8_t reg);
    bool    updateReg(uint8_t reg, uint8_t mask, uint8_t value);

    // Print full register state to Serial (for debugging)
    void dumpRegisters();

private:
    uint8_t   _address;
    TwoWire  *_wire;

    uint16_t _clamp(uint16_t value, uint16_t minVal, uint16_t maxVal);
};
