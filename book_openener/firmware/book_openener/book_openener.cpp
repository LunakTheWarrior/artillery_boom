#include <Arduino.h>
#include "ModulinoDistance.h"
#include "Presence.h"
#include "stepper_motor.h"

namespace {

// The Arduino layer owns pins and timing; StepperMotor owns the shared coil sequence.
class BookOpener final {
public:
    void begin() {
        Serial.begin(115200);
        for (const auto pin : motorPins_) pinMode(pin, OUTPUT);
        coilsOff();
        pinMode(homePin_, INPUT_PULLUP);
        pinMode(LED_BUILTIN, OUTPUT);

        auto status = sensor_.begin(50, 0);
        if (status) status = sensor_.startRanging();
        if (!status) {
            stopFault(F("FAULT: distance initialization"));
            return;
        }

        lastValid_ = millis();
        Serial.println(F("Book opener: manually close, then send z. No motion at power-on."));
    }

    void update() {
        const uint32_t now = millis();
        updateHome(now);
        readCommand();
        updateDistance(now);

        if (automatic_ && uint32_t(now - lastValid_) > 1500)
            stopFault(F("FAULT: no valid distance; aim at a background within range"));

        if (automatic_ && !faulted_)
            setTarget(presence_.wantsOpen() ? openSteps_ : (home_ ? 0 : -closeExtra_));

        moveMotor(now);
        digitalWrite(LED_BUILTIN, faulted_ ? ((now / 250) & 1) : automatic_);
    }

    bool zeroed() const { return zeroed_; }
    bool automatic() const { return automatic_; }
    bool faulted() const { return faulted_; }
    long position() const { return position_; }
    long target() const { return target_; }
    long openSteps() const { return openSteps_; }

private:
    static const uint8_t motorPins_[4];
    static const uint8_t homePin_ = 2;
    static const int8_t openDirection_ = 1;
    static const long maxTravel_ = 20000;
    static const long closeExtra_ = 400;
    static const uint32_t stepIntervalUs_ = 4000;

    ModulinoDistance sensor_;
    Presence presence_;
    StepperMotor stepper_;
    long position_ = 0;
    long target_ = 0;
    long openSteps_ = 0;
    bool zeroed_ = false;
    bool automatic_ = false;
    bool faulted_ = false;
    bool homeRaw_ = false;
    bool home_ = false;
    uint32_t homeChanged_ = 0;
    uint32_t lastStep_ = 0;
    uint32_t lastPoll_ = 0;
    uint32_t lastValid_ = 0;
    uint32_t moveStarted_ = 0;

    void coilsOff() const {
        for (const auto pin : motorPins_) digitalWrite(pin, LOW);
    }

    void stopFault(const __FlashStringHelper* message) {
        target_ = position_;
        automatic_ = false;
        faulted_ = true;
        coilsOff(); // Elastic can close the book: use a weak return, never a strong spring.
        Serial.println(message);
    }

    void setTarget(long next) {
        if (next == target_) return;
        target_ = next;
        moveStarted_ = millis();
    }

    void updateHome(uint32_t now) {
        const bool raw = digitalRead(homePin_) == LOW;
        if (raw != homeRaw_) {
            homeRaw_ = raw;
            homeChanged_ = now;
        }
        if (uint32_t(now - homeChanged_) >= 25) home_ = homeRaw_;
    }

    void moveMotor(uint32_t now) {
        if (!zeroed_ || faulted_) return;

        // Stop paying out immediately on raw switch contact; debounce before re-zeroing.
        if (target_ < position_ && homeRaw_) {
            if (home_) {
                position_ = target_ = 0;
                coilsOff();
            }
            return;
        }
        if (target_ == position_) {
            if (target_ < 0) stopFault(F("FAULT: closed switch not reached"));
            return;
        }
        if (uint32_t(now - moveStarted_) > 120000UL) {
            stopFault(F("FAULT: motion timeout"));
            return;
        }
        if (target_ > position_ && home_ && position_ > 500) {
            stopFault(F("FAULT: closed switch did not release"));
            return;
        }
        if (uint32_t(micros() - lastStep_) < stepIntervalUs_) return;

        lastStep_ = micros();
        const int8_t direction = target_ > position_ ? 1 : -1;
        const auto& coils = direction * openDirection_ > 0
            ? stepper_.stepForward() : stepper_.stepBackward();
        for (uint8_t index = 0; index < 4; ++index)
            digitalWrite(motorPins_[index], coils[index] ? HIGH : LOW);
        position_ += direction;
    }

    void readCommand() {
        if (!Serial.available()) return;
        const char command = Serial.read();
        if (command == '!') {
            stopFault(F("STOPPED: close manually, then z"));
            return;
        }
        if (command == 'z') {
            setZero();
            return;
        }
        if (command == 'p') {
            printStatus();
            return;
        }
        if (!zeroed_ || faulted_) return;

        switch (command) {
        case '+':
        case '-': jog(command); break;
        case 's': saveOpenPosition(); break;
        case 'c': closeBook(); break;
        case 'a': armAutomatic(); break;
        default: break;
        }
    }

    void setZero() {
        if (!home_) {
            Serial.println(F("Close book onto switch first"));
            return;
        }
        coilsOff();
        position_ = target_ = openSteps_ = 0;
        zeroed_ = true;
        faulted_ = automatic_ = false;
        presence_.reset();
        Serial.println(F("Zero set. + / - jog; s saves open position; c closes; a arms."));
    }

    void printStatus() const {
        Serial.print(F("steps=")); Serial.print(position_);
        Serial.print(F(" open=")); Serial.print(openSteps_);
        Serial.print(F(" home=")); Serial.println(home_);
    }

    void jog(char command) {
        automatic_ = false;
        if (target_ != position_) return; // One deliberate jog at a time.
        long next = position_ + (command == '+' ? 200 : -200);
        if (next < 0) next = 0;
        if (next > maxTravel_) next = maxTravel_;
        setTarget(next);
    }

    void saveOpenPosition() {
        if (position_ <= 0 || position_ != target_) return;
        openSteps_ = position_;
        Serial.print(F("Saved opening steps: "));
        Serial.println(openSteps_);
    }

    void closeBook() {
        automatic_ = false;
        if (!home_) setTarget(-closeExtra_);
    }

    void armAutomatic() {
        if (!home_ || openSteps_ <= 0 || target_ != position_ ||
            uint32_t(millis() - lastValid_) > 500) {
            Serial.println(F("Need saved open position, closed switch, fresh distance"));
            return;
        }
        position_ = target_ = 0;
        presence_.reset();
        automatic_ = true;
        Serial.println(F("Automatic mode armed"));
    }

    void updateDistance(uint32_t now) {
        if (faulted_ || uint32_t(now - lastPoll_) < 20) return;
        lastPoll_ = now;
        const auto reading = sensor_.readMeasurement();
        if (reading) {
            presence_.sample(now, reading->valid(), reading->distanceMm);
            if (reading->valid()) lastValid_ = now;
        } else if (reading.error() != ModulinoDistance::Error::NotReady) {
            stopFault(F("FAULT: distance I2C; check wiring and reset"));
        }
    }
};

const uint8_t BookOpener::motorPins_[4] = {8, 9, 10, 11};
BookOpener bookOpener;

} // namespace

void setup() { bookOpener.begin(); }
void loop() { bookOpener.update(); }
