#include "BQ25619.h"

// ============================================================
//  BQ25619 Arduino/ESP32 Library - Implementation
//  Based on TI BQ25619 Datasheet SLUSDF8 – JUNE 2019
// ============================================================

BQ25619::BQ25619(uint8_t address, TwoWire &wire)
    : _address(address), _wire(&wire) {}

// ------------------------------------------------------------
bool BQ25619::begin(TwoWire *wire, uint8_t address) {
    // Allow Wire bus and address to be overridden at begin() time.
    // If not provided, the values set in the constructor are kept.
    if (wire    != nullptr) _wire    = wire;
    if (address != 0x00)    _address = address;

    // Note: Wire.begin() must be called by the caller before begin(),
    // typically with Wire.begin(SDA, SCL) on ESP32. The library does not
    // call Wire.begin() internally to avoid overriding custom pin assignments.

    // Verify we can communicate by reading the part info register (REG0A).
    // 0xFF indicates I2C NACK / no device present.
    uint8_t val = readReg(BQ25619_REG0A);
    return (val != 0xFF);
}

// ------------------------------------------------------------
// REG00: Input current limit
// bits[5:0] = (mA - 100) / 100, range 100–3200 mA
bool BQ25619::setInputCurrentLimit(uint16_t mA) {
    mA = _clamp(mA, BQ25619_IINDPM_MIN_MA, BQ25619_IINDPM_MAX_MA);
    uint8_t code = (mA - BQ25619_IINDPM_MIN_MA) / BQ25619_IINDPM_STEP_MA;
    // bits[5:0], preserve bits[7:6]
    return updateReg(BQ25619_REG00, 0x3F, code & 0x3F);
}

// ------------------------------------------------------------
// REG02: Fast charge current
// bits[6:0] = mA / 25, range 0–1500 mA
bool BQ25619::setChargeCurrent(uint16_t mA) {
    mA = _clamp(mA, BQ25619_ICHG_MIN_MA, BQ25619_ICHG_MAX_MA);
    uint8_t code = mA / BQ25619_ICHG_STEP_MA;
    // bits[6:0], preserve bit[7]
    return updateReg(BQ25619_REG02, 0x7F, code & 0x7F);
}

// ------------------------------------------------------------
// REG03: Precharge current (high nibble bits[7:4])
// bits[7:4] = (mA - 25) / 25, range 25–400 mA
bool BQ25619::setPrechargeCurrent(uint16_t mA) {
    mA = _clamp(mA, BQ25619_IPRECHG_MIN_MA, BQ25619_IPRECHG_MAX_MA);
    uint8_t code = (mA - BQ25619_IPRECHG_MIN_MA) / BQ25619_IPRECHG_STEP_MA;
    return updateReg(BQ25619_REG03, 0xF0, (code & 0x0F) << 4);
}

// ------------------------------------------------------------
// REG03: Termination current (low nibble bits[3:0])
// bits[3:0] = (mA - 5) / 5, range 5–80 mA
bool BQ25619::setTerminationCurrent(uint16_t mA) {
    mA = _clamp(mA, BQ25619_ITERM_MIN_MA, BQ25619_ITERM_MAX_MA);
    uint8_t code = (mA - BQ25619_ITERM_MIN_MA) / BQ25619_ITERM_STEP_MA;
    return updateReg(BQ25619_REG03, 0x0F, code & 0x0F);
}

// ------------------------------------------------------------
// REG04: Charge voltage
// bits[7:2] = (mV - 3856) / 10, range 3856–4608 mV
bool BQ25619::setChargeVoltage(uint16_t mV) {
    mV = _clamp(mV, BQ25619_VREG_MIN_MV, BQ25619_VREG_MAX_MV);
    uint8_t code = (mV - BQ25619_VREG_MIN_MV) / BQ25619_VREG_STEP_MV;
    // bits[7:2], preserve bits[1:0]
    return updateReg(BQ25619_REG04, 0xFC, (code & 0x3F) << 2);
}

// ------------------------------------------------------------
// REG06: Input voltage limit / VINDPM
// bits[3:0] = (mV - 3900) / 100, range 3900–5400 mV
bool BQ25619::setInputVoltageLimit(uint16_t mV) {
    mV = _clamp(mV, BQ25619_VINDPM_MIN_MV, BQ25619_VINDPM_MAX_MV);
    uint8_t code = (mV - BQ25619_VINDPM_MIN_MV) / BQ25619_VINDPM_STEP_MV;
    return updateReg(BQ25619_REG06, 0x0F, code & 0x0F);
}

// ------------------------------------------------------------
// REG06: Thermal regulation threshold
// bits[1:0] = threshold enum
bool BQ25619::setThermalThreshold(BQ25619_ThermalThreshold threshold) {
    return updateReg(BQ25619_REG06, 0x03, (uint8_t)threshold & 0x03);
}

// ------------------------------------------------------------
// REG05: Watchdog timer bits[4:3]
bool BQ25619::setWatchdogTimer(BQ25619_WatchdogTimer setting) {
    return updateReg(BQ25619_REG05, 0x18, ((uint8_t)setting & 0x03) << 3);
}

// ------------------------------------------------------------
// REG05: Safety timer
// bit[3] = EN_TIMER, bits[2:1] = CHG_TIMER
bool BQ25619::setSafetyTimer(bool enable, BQ25619_SafetyTimer duration) {
    uint8_t val = enable ? (0x08 | (((uint8_t)duration & 0x03) << 1)) : 0x00;
    return updateReg(BQ25619_REG05, 0x0E, val);
}

// ------------------------------------------------------------
// REG01: Enable/disable charging - bit[4] CHG_CONFIG
bool BQ25619::enableCharging(bool enable) {
    return updateReg(BQ25619_REG01, 0x10, enable ? 0x10 : 0x00);
}

// ------------------------------------------------------------
// REG05: Enable/disable termination detection - bit[7] EN_TERM
bool BQ25619::enableTermination(bool enable) {
    return updateReg(BQ25619_REG05, 0x80, enable ? 0x80 : 0x00);
}

// ------------------------------------------------------------
// REG07: STAT pin disable - bit[6] STAT_DIS (0 = enabled, 1 = disabled)
bool BQ25619::enableStatPin(bool enable) {
    return updateReg(BQ25619_REG07, 0x40, enable ? 0x00 : 0x40);
}

// ------------------------------------------------------------
// REG01: Reset all registers bit[7] REG_RST
bool BQ25619::resetRegisters() {
    return updateReg(BQ25619_REG01, 0x80, 0x80);
}

// ------------------------------------------------------------
// REG08: Read system status
BQ25619_Status BQ25619::getStatus() {
    BQ25619_Status s;
    uint8_t reg = readReg(BQ25619_REG08);
    s.vbusStatus          = (BQ25619_VBUSStatus)  ((reg & BQ25619_VBUS_STAT_MASK) >> BQ25619_VBUS_STAT_SHIFT);
    s.chargeStatus        = (BQ25619_ChargeStatus)((reg & BQ25619_CHRG_STAT_MASK) >> BQ25619_CHRG_STAT_SHIFT);
    s.powerGood           = (reg & BQ25619_PG_STAT_BIT)    != 0;
    s.inThermalRegulation = (reg & BQ25619_THERM_STAT_BIT) != 0;
    s.inVSYSRegulation    = (reg & BQ25619_VSYS_STAT_BIT)  != 0;
    return s;
}

// ------------------------------------------------------------
// REG09: Read fault register (clears on read per datasheet)
BQ25619_Faults BQ25619::getFaults() {
    BQ25619_Faults f;
    uint8_t reg = readReg(BQ25619_REG09);
    f.watchdogFault = (reg & BQ25619_WATCHDOG_FAULT_BIT) != 0;
    f.boostFault    = (reg & BQ25619_BOOST_FAULT_BIT)    != 0;
    f.chargeFault   = (BQ25619_ChargeFault)((reg & BQ25619_CHRG_FAULT_MASK) >> BQ25619_CHRG_FAULT_SHIFT);
    f.batteryFault  = (reg & BQ25619_BAT_FAULT_BIT) != 0;
    f.ntcFault      = (BQ25619_NTCFault)(reg & BQ25619_NTC_FAULT_MASK);
    f.anyFault      = (reg != 0x00);
    return f;
}

// ------------------------------------------------------------
bool BQ25619::isCharging() {
    BQ25619_ChargeStatus s = getStatus().chargeStatus;
    return (s == CHARGE_STATUS_PRE_CHARGE || s == CHARGE_STATUS_FAST_CHARGING);
}

bool BQ25619::isChargeComplete() {
    return getStatus().chargeStatus == CHARGE_STATUS_COMPLETE;
}

bool BQ25619::isPowerGood() {
    return getStatus().powerGood;
}

bool BQ25619::hasFault() {
    return getFaults().anyFault;
}

// ------------------------------------------------------------
// Read back charge current from REG02 in mA
uint16_t BQ25619::getChargeCurrent() {
    uint8_t reg = readReg(BQ25619_REG02);
    return (uint16_t)(reg & 0x7F) * BQ25619_ICHG_STEP_MA;
}

// Read back input current limit from REG00 in mA
uint16_t BQ25619::getInputCurrentLimit() {
    uint8_t reg = readReg(BQ25619_REG00);
    return ((uint16_t)(reg & 0x3F) * BQ25619_IINDPM_STEP_MA) + BQ25619_IINDPM_MIN_MA;
}

// Read back charge voltage from REG04 in mV
uint16_t BQ25619::getChargeVoltage() {
    uint8_t reg = readReg(BQ25619_REG04);
    return ((uint16_t)((reg & 0xFC) >> 2) * BQ25619_VREG_STEP_MV) + BQ25619_VREG_MIN_MV;
}

// ------------------------------------------------------------
// REG0A: Part identification
// Bits[7:5] = PN (part number), Bits[3:0] = DEV_REV
uint16_t BQ25619::getICversion() {
    uint8_t val = readReg(BQ25619_REG0A);
    uint8_t pn  = (val >> 5) & 0x07;
    uint8_t rev =  val       & 0x0F;
    return ((uint16_t)pn << 8) | rev;
}

uint8_t BQ25619::getChipID() {
    return (readReg(BQ25619_REG0A) >> 5) & 0x07;
}

// ------------------------------------------------------------
// Debug: dump all registers to Serial
void BQ25619::dumpRegisters() {
    Serial.println("=== BQ25619 Register Dump ===");
    for (uint8_t reg = 0x00; reg <= 0x0A; reg++) {
        uint8_t val = readReg(reg);
        Serial.print("REG0");
        Serial.print(reg, HEX);
        Serial.print(": 0x");
        if (val < 0x10) Serial.print("0");
        Serial.println(val, HEX);
    }

    // Decode key fields
    BQ25619_Status s = getStatus();
    BQ25619_Faults f = getFaults();

    Serial.println("\n=== Decoded Status ===");
    Serial.print("Power Good:     "); Serial.println(s.powerGood ? "YES" : "NO");
    Serial.print("Charge State:   ");
    switch(s.chargeStatus) {
        case CHARGE_STATUS_NOT_CHARGING:  Serial.println("Not Charging");  break;
        case CHARGE_STATUS_PRE_CHARGE:    Serial.println("Pre-Charge");    break;
        case CHARGE_STATUS_FAST_CHARGING: Serial.println("Fast Charging"); break;
        case CHARGE_STATUS_COMPLETE:      Serial.println("Complete");      break;
    }
    Serial.print("VBUS Status:    ");
    switch(s.vbusStatus) {
        case VBUS_NO_INPUT: Serial.println("No Input");     break;
        case VBUS_USB_SDP:  Serial.println("USB SDP");      break;
        case VBUS_ADAPTER:  Serial.println("Adapter");      break;
        case VBUS_OTG:      Serial.println("OTG");          break;
        default:            Serial.println("Unknown");      break;
    }
    if (s.inThermalRegulation) Serial.println("WARNING: In thermal regulation");
    if (s.inVSYSRegulation)    Serial.println("WARNING: In VSYS regulation");

    Serial.println("\n=== Decoded Faults ===");
    if (!f.anyFault) {
        Serial.println("No faults");
    } else {
        if (f.watchdogFault) Serial.println("FAULT: Watchdog timer expired");
        if (f.boostFault)    Serial.println("FAULT: Boost mode fault");
        switch (f.chargeFault) {
            case CHRG_FAULT_INPUT_OVP:    Serial.println("FAULT: Input overvoltage");     break;
            case CHRG_FAULT_THERMAL:      Serial.println("FAULT: Thermal shutdown");       break;
            case CHRG_FAULT_SAFETY_TIMER: Serial.println("FAULT: Safety timer expired");  break;
            default: break;
        }
        if (f.batteryFault) Serial.println("FAULT: Battery overvoltage");
        switch (f.ntcFault) {
            case NTC_FAULT_WARM:    Serial.println("FAULT: NTC warm");     break;
            case NTC_FAULT_COOL:    Serial.println("FAULT: NTC cool");     break;
            case NTC_FAULT_COLD:    Serial.println("FAULT: NTC cold");     break;
            case NTC_FAULT_HOT:     Serial.println("FAULT: NTC hot");      break;
            case NTC_FAULT_VERY_HOT:Serial.println("FAULT: NTC very hot"); break;
            default: break;
        }
    }

    Serial.println("\n=== Decoded Config ===");
    Serial.print("Charge Voltage:      "); Serial.print(getChargeVoltage());       Serial.println(" mV");
    Serial.print("Charge Current:      "); Serial.print(getChargeCurrent());       Serial.println(" mA");
    Serial.print("Input Current Limit: "); Serial.print(getInputCurrentLimit());   Serial.println(" mA");
}

// ------------------------------------------------------------
// Raw I2C helpers
bool BQ25619::writeReg(uint8_t reg, uint8_t value) {
    _wire->beginTransmission(_address);
    _wire->write(reg);
    _wire->write(value);
    return (_wire->endTransmission() == 0);
}

uint8_t BQ25619::readReg(uint8_t reg) {
    _wire->beginTransmission(_address);
    _wire->write(reg);
    if (_wire->endTransmission(false) != 0) return 0xFF;
    _wire->requestFrom(_address, (uint8_t)1);
    return _wire->available() ? _wire->read() : 0xFF;
}

// Read-modify-write: clears bits in mask, then sets value
bool BQ25619::updateReg(uint8_t reg, uint8_t mask, uint8_t value) {
    uint8_t current = readReg(reg);
    if (current == 0xFF) return false;
    uint8_t updated = (current & ~mask) | (value & mask);
    return writeReg(reg, updated);
}

uint16_t BQ25619::_clamp(uint16_t value, uint16_t minVal, uint16_t maxVal) {
    if (value < minVal) return minVal;
    if (value > maxVal) return maxVal;
    return value;
}
