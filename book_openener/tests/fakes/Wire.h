#pragma once
#include "Arduino.h"
#include <vector>
#include <array>
struct TwoWire {
    std::array<uint8_t,512> memory{};
    std::vector<uint8_t> tx;
    std::vector<uint16_t> writes;
    uint16_t selected = 0;
    uint8_t left = 0;
    bool timeout = false, injectTimeout = false, shortRead = false, neverReady = false;
    uint8_t nack = 0;
    uint32_t busHz = 0, timeoutUs = 0;
    void begin() {}
    void setClock(uint32_t n) { busHz = n; }
    void setWireTimeout(uint32_t n, bool) { timeoutUs = n; }
    void clearWireTimeoutFlag() { timeout = false; }
    bool getWireTimeoutFlag() { return timeout; }
    void beginTransmission(uint8_t address) { if(address != 0x29) nack = 2; tx.clear(); }
    size_t write(uint8_t value) { tx.push_back(value); return 1; }
    uint8_t endTransmission(bool stop) {
        if (injectTimeout) { timeout = true; return 5; }
        if (nack) return nack;
        selected = (uint16_t(tx.at(0)) << 8) | tx.at(1);
        if (tx.size() > 2) {
            if (!stop) return 4;
            for (size_t n=2;n<tx.size();++n) {
                memory.at(selected) = tx[n]; writes.push_back(selected++);
            }
        } else if (stop) return 4; // Reads require repeated START.
        return 0;
    }
    uint8_t requestFrom(uint8_t, uint8_t count, uint8_t stop) {
        if (!stop) return 0;
        left = count - (shortRead ? 1 : 0); return left;
    }
    int available() { return left; }
    int read() {
        if (!left) return -1;
        --left;
        if (selected == 0x31 && neverReady) { ++selected; return 1; }
        return memory.at(selected++);
    }
};
extern TwoWire Wire;
