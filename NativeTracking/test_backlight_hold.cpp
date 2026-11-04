#include <cassert>
#include <queue>
#include <iostream>
const int HIGH=1, LOW=0, OUTPUT=1;
int pins[32]={};
struct SerialStub { std::queue<int> q; void begin(int) {} int available(){return !q.empty();} int read(){int x=q.front();q.pop();return x;} } Serial;
void pinMode(int,int){}
void digitalWrite(int p,int v){pins[p]=v;}
void delay(int){}
void delayMicroseconds(unsigned int){}
#include "../ProjectPB/Arduino.txt"
void send(int c){Serial.q.push(c);loop();}
void expectPair(int phase){
 for(int i=0;i<8;i++)assert(pins[OUTPUT_PINS[i]]==((i==phase||i==phase+4)?HIGH:LOW));
}
void expectOff(){for(int p:OUTPUT_PINS)assert(pins[p]==LOW);}
int main(){
 setup();
 for(int phase=0;phase<4;phase++){
  send(HOLD_FRAME_0+phase);expectPair(phase);
  for(int i=0;i<50;i++)loop();
  expectPair(phase);
  send(TIME_DIV_0+((phase+1)%4));expectPair(phase);
 }
 send(RESET_SYNC);expectOff();assert(cnt==0 && PIN_PAST==-1);
 send(ENABLE_TIMEDIVISION);assert(running);expectOff();
 for(int phase=0;phase<4;phase++){send(TIME_DIV_0+phase);assert(cnt==phase);expectOff();}
 send(HOLD_FRAME_2);expectPair(2);
 send(RESET_SYNC);send(ENABLE_TIMEDIVISION);send(TIME_DIV_0);assert(cnt==0);expectOff();
 std::cout<<"PASS: hold all 4 pairs, persist without commands, ignore timed pulses while held, reset/resume, one phase per frame\n";
}
