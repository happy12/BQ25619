// =============================================================
//  BQ25619 BasicCharger Example
//  1S LiPo 650mAh, 5V DC adapter, STAT LED on STAT pin
// =============================================================

#include <Wire.h>
#include "BQ25619.h"

// --- Custom I2C pins (ESP32) ---
// Change these to match your board wiring.
// If you are using the default Wire pins, you can remove these
// defines and the Wire.begin(SDA, SCL) call, and just call
// charger.begin() with no arguments.
#define I2C_SDA 21
#define I2C_SCL 22

// Use 0x6A if ADDR pin is tied to GND (most common)
// Use 0x6B if ADDR pin is tied to VCC
BQ25619 charger(0x6A);

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("BQ25619 BasicCharger Example");

    // Initialise Wire with custom pins BEFORE passing to charger.begin().
    // Wire.begin(SDA, SCL) is ESP32-specific.
    Wire.begin(I2C_SDA, I2C_SCL);

    if (!charger.begin(&Wire)) {
        Serial.println("ERROR: BQ25619 not found. Check I2C wiring and address.");
        while (true) delay(1000);
    }
    Serial.println("BQ25619 found.");

    // --- Configure for 1S LiPo 650mAh from 5V DC adapter ---

    // Input current limit: 500mA (safe for both 1A and 2A adapters)
    charger.setInputCurrentLimit(500);

    // Fast charge current: 325mA (0.5C of 650mAh for longevity)
    charger.setChargeCurrent(325);

    // Precharge current: 65mA (~10% of fast charge)
    charger.setPrechargeCurrent(65);

    // Termination current: 32mA (~5% of fast charge, signals charge complete)
    charger.setTerminationCurrent(32);

    // Charge voltage: 4200mV (standard 1S LiPo full charge — do not exceed)
    charger.setChargeVoltage(4200);

    // Input voltage limit: 4500mV (just below 5V, protects against sag)
    charger.setInputVoltageLimit(4500);

    // Thermal regulation threshold: 100°C
    charger.setThermalThreshold(TREG_100C);

    // Watchdog: disabled (set-and-forget, no MCU petting required)
    charger.setWatchdogTimer(WATCHDOG_DISABLED);

    // Safety timer: enabled, 5 hours (fault backstop — should never fire normally)
    charger.setSafetyTimer(true, SAFETY_TIMER_5H);

    // Termination detection: enabled
    charger.enableTermination(true);

    // STAT pin: enabled (drives your LED)
    charger.enableStatPin(true);

    // Enable charging
    charger.enableCharging(true);

    Serial.println("Charger configured.");
    Serial.println();

    // Print full state on boot
    charger.dumpRegisters();
}

void loop() {
    delay(30000); // Check every 30 seconds

    Serial.println("\n--- Periodic Status Check ---");

    BQ25619_Status status = charger.getStatus();
    BQ25619_Faults faults = charger.getFaults(); // clears fault register on read

    Serial.print("Power Good: ");
    Serial.println(status.powerGood ? "YES" : "NO");

    Serial.print("Charge State: ");
    switch (status.chargeStatus) {
        case CHARGE_STATUS_NOT_CHARGING:  Serial.println("Not Charging");  break;
        case CHARGE_STATUS_PRE_CHARGE:    Serial.println("Pre-Charge");    break;
        case CHARGE_STATUS_FAST_CHARGING: Serial.println("Fast Charging"); break;
        case CHARGE_STATUS_COMPLETE:      Serial.println("Charge Complete"); break;
    }

    if (status.inThermalRegulation) {
        Serial.println("WARNING: Thermal regulation active — charge current reduced");
    }

    if (faults.anyFault) {
        Serial.println("*** FAULT DETECTED ***");
        if (faults.watchdogFault)                            Serial.println("  Watchdog expired");
        if (faults.boostFault)                               Serial.println("  Boost fault");
        if (faults.chargeFault == CHRG_FAULT_INPUT_OVP)     Serial.println("  Input overvoltage");
        if (faults.chargeFault == CHRG_FAULT_THERMAL)        Serial.println("  Thermal shutdown");
        if (faults.chargeFault == CHRG_FAULT_SAFETY_TIMER)  Serial.println("  Safety timer expired");
        if (faults.batteryFault)                             Serial.println("  Battery overvoltage");
    }
}
