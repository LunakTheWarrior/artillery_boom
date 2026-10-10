#pragma once
#include <stdint.h>
#include <stddef.h>
struct __FlashStringHelper {};
#define F(s) reinterpret_cast<const __FlashStringHelper*>(s)
#define LOW 0
#define HIGH 1
#define OUTPUT 1
#define INPUT_PULLUP 2
#define LED_BUILTIN 13
extern uint32_t fakeMs;
extern int pins[32];
inline uint32_t millis() { return fakeMs; }
inline uint32_t micros() { return fakeMs * 1000; }
inline void delay(uint32_t n) { fakeMs += n; }
inline void pinMode(int, int) {}
inline int digitalRead(int p) { return pins[p]; }
inline void digitalWrite(int p, int v) { pins[p] = v; }
struct FakeSerial {
    char command = 0;
    void begin(int) {}
    bool available() { return command != 0; }
    char read() { char c = command; command = 0; return c; }
    template<class T> void print(T) {}
    template<class T> void println(T) {}
};
extern FakeSerial Serial;
