#include <Arduino.h>
#include "ModulinoDistance.h"
#include "Presence.h"

// Confirmed 28BYJ family; configured for 5 V 28BYJ-48 + ULN2003. Check voltage label.
// D8..D11 connect in order to IN1..IN4, not the reordered Arduino Stepper convention.
const uint8_t MOTOR_PINS[] = {8, 9, 10, 11};
const uint8_t HOME_PIN = 2; // Normally-open closed-position switch to GND.
const int8_t OPEN_DIRECTION = 1; // Reverse to -1 if winding runs backwards.
const long MAX_TRAVEL = 20000;
const long CLOSE_EXTRA = 400;
const uint32_t STEP_US = 4000; // Slow half-stepping; sensor polls can extend intervals.
const uint8_t PATTERN[8] = {1, 3, 2, 6, 4, 12, 8, 9};
ModulinoDistance sensor;
Presence presence;
long position = 0, target = 0, openSteps = 0;
uint8_t phase = 0;
bool zeroed = false, automatic = false, faulted = false;
bool homeRaw = false, home = false;
uint32_t homeChanged = 0, lastStep = 0, lastPoll = 0, lastValid = 0, moveStarted = 0;

void coilsOff() {
    for (uint8_t i = 0; i < 4; ++i) digitalWrite(MOTOR_PINS[i], LOW);
}
void stopFault(const __FlashStringHelper* message) {
    target = position;
    automatic = false;
    faulted = true;
    coilsOff(); // Elastic can close the book: use a weak return, never a strong spring.
    Serial.println(message);
}
void setTarget(long next) {
    if (next == target) return;
    target = next;
    moveStarted = millis();
}
void updateHome(uint32_t now) {
    const bool raw = digitalRead(HOME_PIN) == LOW;
    if (raw != homeRaw) { homeRaw = raw; homeChanged = now; }
    if (uint32_t(now - homeChanged) >= 25) home = homeRaw;
}
void motorTick(uint32_t now) {
    if (!zeroed || faulted) return;
    // Stop paying out immediately on raw switch contact; debounce before re-zeroing.
    if (target < position && homeRaw) {
        if (home) { position = target = 0; coilsOff(); }
        return;
    }
    if (target == position) {
        if (target < 0) stopFault(F("FAULT: closed switch not reached"));
        return;
    }
    if (uint32_t(now - moveStarted) > 120000UL) {
        stopFault(F("FAULT: motion timeout")); return;
    }
    if (target > position && home && position > 500) {
        stopFault(F("FAULT: closed switch did not release")); return;
    }
    if (uint32_t(micros() - lastStep) < STEP_US) return;
    lastStep = micros();
    const int8_t direction = target > position ? 1 : -1;
    phase = (phase + (direction * OPEN_DIRECTION > 0 ? 1 : 7)) & 7;
    for (uint8_t i = 0; i < 4; ++i)
        digitalWrite(MOTOR_PINS[i], (PATTERN[phase] >> i) & 1);
    position += direction;
}
void commands() {
    if (!Serial.available()) return;
    const char c = Serial.read();
    if (c == '!') { stopFault(F("STOPPED: close manually, then z")); return; }
    if (c == 'z') {
        if (!home) { Serial.println(F("Close book onto switch first")); return; }
        coilsOff(); position = target = openSteps = 0;
        zeroed = true; faulted = automatic = false; presence.reset();
        Serial.println(F("Zero set. + / - jog; s saves open position; c closes; a arms."));
        return;
    }
    if (c == 'p') {
        Serial.print(F("steps=")); Serial.print(position);
        Serial.print(F(" open=")); Serial.print(openSteps);
        Serial.print(F(" home=")); Serial.println(home);
        return;
    }
    if (!zeroed || faulted) return;
    if (c == '+' || c == '-') {
        automatic = false;
        if (target != position) return; // One deliberate jog at a time.
        long next = position + (c == '+' ? 200 : -200);
        if (next < 0) next = 0;
        if (next > MAX_TRAVEL) next = MAX_TRAVEL;
        setTarget(next);
    } else if (c == 's' && position == target && position > 0) {
        openSteps = position; Serial.print(F("Saved opening steps: ")); Serial.println(openSteps);
    } else if (c == 'c') {
        automatic = false;
        if (!home) setTarget(-CLOSE_EXTRA);
    } else if (c == 'a') {
        if (!home || openSteps <= 0 || target != position ||
            uint32_t(millis() - lastValid) > 500) {
            Serial.println(F("Need saved open position, closed switch, fresh distance")); return;
        }
        position = target = 0; presence.reset(); automatic = true;
        Serial.println(F("Automatic mode armed"));
    }
}
void setup() {
    Serial.begin(115200);
    for (uint8_t i = 0; i < 4; ++i) pinMode(MOTOR_PINS[i], OUTPUT);
    coilsOff();
    pinMode(HOME_PIN, INPUT_PULLUP);
    pinMode(LED_BUILTIN, OUTPUT);
    auto status = sensor.begin(50, 0);
    if (status) status = sensor.startRanging();
    if (!status) { stopFault(F("FAULT: distance initialization")); return; }
    lastValid = millis();
    Serial.println(F("Book opener: manually close, then send z. No motion at power-on."));
}
void loop() {
    const uint32_t now = millis();
    updateHome(now);
    commands();
    if (!faulted && uint32_t(now - lastPoll) >= 20) {
        lastPoll = now;
        auto reading = sensor.readMeasurement();
        if (reading) {
            presence.sample(now, reading->valid(), reading->distanceMm);
            if (reading->valid()) lastValid = now;
        } else if (reading.error() != ModulinoDistance::Error::NotReady) {
            stopFault(F("FAULT: distance I2C; check wiring and reset"));
        }
    }
    if (automatic && uint32_t(now - lastValid) > 1500)
        stopFault(F("FAULT: no valid distance; aim at a background within range"));
    if (automatic && !faulted)
        setTarget(presence.wantsOpen() ? openSteps : (home ? 0 : -CLOSE_EXTRA));
    motorTick(now);
    digitalWrite(LED_BUILTIN, faulted ? ((now / 250) & 1) : automatic);
}
