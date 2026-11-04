#include <cassert>
#include <queue>
#include <iostream>
const int HIGH=1,LOW=0,OUTPUT=1;
int pins[32]={}; unsigned long clockUs=0;
unsigned long micros(){return clockUs;}
void pinMode(int,int){}
void digitalWrite(int p,int v){pins[p]=v;}
struct SerialStub {
 std::queue<int> q; void begin(int){} int available(){return !q.empty();}
 int read(){int c=q.front();q.pop();return c;}
} Serial;
#include "../ProjectPB/Arduino.txt"
void send(int c){Serial.q.push(c);loop();}
void advance(unsigned long t){clockUs=t;loop();}
void off(){for(int p:OUTPUT_PINS)assert(pins[p]==LOW);}
void allOn(){for(int p:OUTPUT_PINS)assert(pins[p]==HIGH);}
int expectedPhysical(int logical){const int expected[4]={0,3,2,1};return expected[logical&3];}
void pair(int phase){phase=expectedPhysical(phase);for(int i=0;i<8;i++)assert(pins[OUTPUT_PINS[i]]==((i==phase||i==phase+4)?HIGH:LOW));}
int main(){
 setup();allOn();assert(!running && !pulseActive);
 advance(1000000);allOn();
 send(TIME_DIV_0);allOn();
 send(CONNECTION_BEGIN);off();
 send(CONNECTION_END);allOn();advance(2000000);allOn();
 send(ENABLE_TIMEDIVISION);send(TIME_DIV_0);
 send(LIGHT_EXIT);allOn();advance(3000000);allOn();
 send(ENABLE_TIMEDIVISION);off();
 for(int phase=0;phase<4;phase++){
  send(RESET_SYNC);send(ENABLE_TIMEDIVISION);
  const unsigned long base=clockUs+10000;advance(base);
  send(TIME_DIV_0+phase);off();
  advance(base+TOP_ON_US-1);off();
  advance(base+TOP_ON_US);assert(pins[OUTPUT_PINS[expectedPhysical(phase)+4]]==HIGH);assert(pins[OUTPUT_PINS[expectedPhysical(phase)]]==LOW);
  advance(base+BOTTOM_SWITCH_US);pair(phase);
  advance(base+ALL_OFF_US);off();
  int next=(phase+1)&3; unsigned long nextBase=base+10000;advance(nextBase);
  send(TIME_DIV_0+next);assert(pins[OUTPUT_PINS[expectedPhysical(phase)]]==HIGH);
  advance(nextBase+TOP_ON_US);assert(pins[OUTPUT_PINS[expectedPhysical(next)+4]]==HIGH);assert(pins[OUTPUT_PINS[expectedPhysical(phase)]]==HIGH);
  advance(nextBase+BOTTOM_SWITCH_US);pair(next);
  send(HOLD_FRAME_0+phase);pair(phase);
  advance(nextBase+50000);pair(phase);
  send(TIME_DIV_0+next);pair(phase);
  send(RESET_SYNC);off();send(ENABLE_TIMEDIVISION);assert(running);off();
 }
 // Exit during hold restores standalone all-on, without stale scheduled off.
 send(HOLD_FRAME_0+1);pair(1);send(CONNECTION_END);allOn();
 advance(clockUs+100000);allOn();
 send(ENABLE_TIMEDIVISION);
 // Hold interrupts an in-progress pulse immediately, without an 8ms delay.
 send(TIME_DIV_0);send(HOLD_FRAME_0+3);pair(3);
 std::cout<<"PASS "<<REFRESH_RATE<<"Hz: 4 phases, timing boundaries, previous bottom, off interval, hold, reset/resume\n";
}
