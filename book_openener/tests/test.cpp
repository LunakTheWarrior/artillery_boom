#include <cassert>
#include <iostream>
#include <algorithm>
#include "Arduino.h"
#include "Wire.h"
uint32_t fakeMs = 0;
int pins[32]{};
FakeSerial Serial;
TwoWire Wire;
#include "../firmware/book_openener/book_openener.ino"

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
int main() {
    fixture();
    ModulinoDistance d;
    assert(d.begin());
    assert(Wire.busHz == 100000 && Wire.timeoutUs == 10000);
    for (uint16_t address=0x2D;address<=0x87;++address)
        assert(std::find(Wire.writes.begin(),Wire.writes.end(),address)!=Wire.writes.end());
    assert(!d.setRangeTiming(9,0));
    assert(d.setRangeTiming(50,100));
    assert(d.startRanging());
    assert(!d.setRangeTiming(50,0));
    sample(321);
    auto r = d.readMeasurement();
    assert(r && r->valid() && r->distanceMm == 321 && r->signalKcps == 8000);
    sample(300,9,0); r=d.readMeasurement(); assert(r && !r->valid());
    sample(300,4); r=d.readMeasurement(); assert(r && !r->valid());
    Wire.shortRead=true; assert(!d.readMeasurement()); Wire.shortRead=false;
    Wire.injectTimeout=true;
    assert(d.readMeasurement().error()==ModulinoDistance::Error::I2cTimeout);
    Wire.injectTimeout=false;
    Wire.neverReady=true;
    assert(d.readMeasurement().error()==ModulinoDistance::Error::NotReady);
    assert(d.readDistance(10).error()==ModulinoDistance::Error::Timeout);
    fixture(); word(0x10F,0); ModulinoDistance wrong;
    assert(wrong.begin().error()==ModulinoDistance::Error::WrongDevice);

    Presence p;
    p.sample(0,true,200); p.sample(499,true,200); assert(!p.wantsOpen());
    p.sample(500,true,200); assert(p.wantsOpen());
    p.sample(600,true,400); assert(p.wantsOpen());
    p.sample(700,true,600); p.sample(2699,true,600); assert(p.wantsOpen());
    p.sample(2700,true,600); assert(!p.wantsOpen());
    p.sample(3000,true,200); p.sample(3499,false,0); p.sample(3500,true,200);
    assert(!p.wantsOpen()); p.sample(4000,true,200); assert(p.wantsOpen());
    p.reset(); p.sample(UINT32_MAX-200,true,200); p.sample(299,true,200);
    assert(p.wantsOpen()); // millis rollover

    fixture(); sample(600); setup();
    assert(!zeroed && !automatic && position==0);
    home=homeRaw=true; Serial.command='z'; commands(); assert(zeroed);
    Serial.command='a'; commands(); assert(!automatic); // No calibrated open position.
    home=homeRaw=false; Serial.command='+'; commands(); assert(target==200);
    for(int n=0;n<200;++n) { fakeMs+=4; motorTick(fakeMs); }
    assert(position==200);
    Serial.command='s'; commands(); assert(openSteps==200);
    Serial.command='c'; commands();
    fakeMs+=4; motorTick(fakeMs); assert(position==199);
    homeRaw=true; fakeMs+=4; motorTick(fakeMs); assert(position==199);
    home=true; motorTick(fakeMs); assert(position==0 && target==0);
    lastValid=fakeMs; Serial.command='a'; commands(); assert(automatic);
    // A valid background permits close detection; persistent invalid ranges fault.
    sample(0,4);
    fakeMs+=1600; loop(); assert(faulted && !automatic);
    for(auto pin : MOTOR_PINS) assert(pins[pin]==LOW);
    // Bounded close travel faults instead of unwinding forever.
    faulted=false; zeroed=true; home=homeRaw=false;
    position=target=-CLOSE_EXTRA; motorTick(fakeMs); assert(faulted);
    std::cout << "Driver, presence and motion tests passed\n";
}
