#include <gtest/gtest.h>
#include <algorithm>
#include "Arduino.h"
#include "Wire.h"
uint32_t fakeMs = 0;
int pins[32]{};
FakeSerial Serial;
TwoWire Wire;
#include "../firmware/book_openener/book_openener.cpp"

void word(uint16_t addr, uint16_t value) {
    Wire.memory[addr] = value >> 8; Wire.memory[addr+1] = value;
}
void fixture() {
    Wire = TwoWire{};
    Wire.memory[0xE5] = 3;
    word(0x10F,0xEBAA); word(6,0xA000); word(0xDE,0x0200);
}
void sample(uint16_t mm, uint8_t raw=9, uint16_t spads=0x0400) {
    Wire.memory[0x89] = raw;
    word(0x8C,spads); word(0x96,mm); word(0x8E,1000);
}
TEST(BookOpenerTest, SensorPresenceAndMotorControl) {
    fixture();
    ModulinoDistance d;
    EXPECT_TRUE(d.begin());
    EXPECT_TRUE(Wire.busHz == 100000 && Wire.timeoutUs == 10000);
    for (uint16_t address=0x2D;address<=0x87;++address)
        EXPECT_TRUE(std::find(Wire.writes.begin(),Wire.writes.end(),address)!=Wire.writes.end());
    EXPECT_TRUE(!d.setRangeTiming(9,0));
    EXPECT_TRUE(d.setRangeTiming(50,100));
    EXPECT_TRUE(d.startRanging());
    EXPECT_TRUE(!d.setRangeTiming(50,0));
    sample(321);
    auto r = d.readMeasurement();
    EXPECT_TRUE(r && r->valid() && r->distanceMm == 321 && r->signalKcps == 8000);
    sample(300,9,0); r=d.readMeasurement(); EXPECT_TRUE(r && !r->valid());
    sample(300,4); r=d.readMeasurement(); EXPECT_TRUE(r && !r->valid());
    Wire.shortRead=true; EXPECT_TRUE(!d.readMeasurement()); Wire.shortRead=false;
    Wire.injectTimeout=true;
    EXPECT_TRUE(d.readMeasurement().error()==ModulinoDistance::Error::I2cTimeout);
    Wire.injectTimeout=false;
    Wire.neverReady=true;
    EXPECT_TRUE(d.readMeasurement().error()==ModulinoDistance::Error::NotReady);
    EXPECT_TRUE(d.readDistance(10).error()==ModulinoDistance::Error::Timeout);
    fixture(); word(0x10F,0); ModulinoDistance wrong;
    EXPECT_TRUE(wrong.begin().error()==ModulinoDistance::Error::WrongDevice);

    Presence p;
    p.sample(0,true,200); p.sample(499,true,200); EXPECT_TRUE(!p.wantsOpen());
    p.sample(500,true,200); EXPECT_TRUE(p.wantsOpen());
    p.sample(600,true,400); EXPECT_TRUE(p.wantsOpen());
    p.sample(700,true,600); p.sample(2699,true,600); EXPECT_TRUE(p.wantsOpen());
    p.sample(2700,true,600); EXPECT_TRUE(!p.wantsOpen());
    p.sample(3000,true,200); p.sample(3499,false,0); p.sample(3500,true,200);
    EXPECT_TRUE(!p.wantsOpen()); p.sample(4000,true,200); EXPECT_TRUE(p.wantsOpen());
    p.reset(); p.sample(UINT32_MAX-200,true,200); p.sample(299,true,200);
    EXPECT_TRUE(p.wantsOpen()); // millis rollover

    fixture(); sample(600); setup();
    EXPECT_TRUE(!bookOpener.zeroed() && !bookOpener.automatic() && bookOpener.position()==0);
    bookOpener.update();
    fakeMs += 25; bookOpener.update(); // Debounce the closed-position switch.
    Serial.command='z'; bookOpener.update(); EXPECT_TRUE(bookOpener.zeroed());
    Serial.command='a'; bookOpener.update();
    EXPECT_TRUE(!bookOpener.automatic()); // No calibrated open position.
    pins[2] = HIGH; fakeMs += 25; bookOpener.update(); // Release the home switch.
    Serial.command='+'; bookOpener.update(); EXPECT_TRUE(bookOpener.target()==200);
    for(int n=0;n<200;++n) { fakeMs+=4; bookOpener.update(); }
    EXPECT_TRUE(bookOpener.position()==200);
    Serial.command='s'; bookOpener.update(); EXPECT_TRUE(bookOpener.openSteps()==200);
    Serial.command='c'; bookOpener.update();
    fakeMs+=4; bookOpener.update(); EXPECT_TRUE(bookOpener.position() <= 199 && bookOpener.position() >= 198);
    pins[2] = LOW; fakeMs+=4; bookOpener.update(); EXPECT_TRUE(bookOpener.position() <= 199 && bookOpener.position() >= 198);
    fakeMs+=25; bookOpener.update(); EXPECT_TRUE(bookOpener.position()==0 && bookOpener.target()==0);
    Serial.command='a'; bookOpener.update(); EXPECT_TRUE(bookOpener.automatic());
    // A valid background permits close detection; persistent invalid ranges fault.
    sample(0,4);
    fakeMs+=1600; bookOpener.update();
    EXPECT_TRUE(bookOpener.faulted() && !bookOpener.automatic());
    for(const auto pin : {8, 9, 10, 11}) EXPECT_TRUE(pins[pin]==LOW);
}
