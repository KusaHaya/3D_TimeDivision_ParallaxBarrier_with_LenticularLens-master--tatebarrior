#include "projectpb/Barrier.h"
#include "projectpb/Tracking.h"
#include <iostream>
#include <limits>
#include <stdexcept>

static void Check(bool condition, const char* what) { if (!condition) throw std::runtime_error(what); }
int main() {
    try {
        using namespace projectpb;
        Check(Quantize(1.79f,.8f)==1 && Quantize(-1.79f,.8f)==-1,"symmetric lower boundary");
        Check(Quantize(1.8f,.8f)==2 && Quantize(-1.8f,.8f)==-2,"symmetric upper boundary");
        // Original integer modulo oracle, independently expressed for integer
        // slope/shift: check all RGB subpixels and phases, including q < 0.
        for (float middle : {3420.f, 20000.f}) {
            Barrier p; p.middle=middle;
            for(unsigned phase=0;phase<4;++phase) for(int y=0;y<1080;y+=7)
                for(int x=0;x<1920;x+=3) for(int c=0;c<3;++c) {
                    const int s=3*x+c+1079-y;
                    const double q=(s-(middle-48*330))/330.0;
                    const double magnitude=std::abs(q);
                    const int rounded=static_cast<int>(std::floor(magnitude))+
                        (magnitude >= std::floor(magnitude)+.8 ? 1:0);
                    const int corr=q<0 ? -rounded:rounded;
                    const int value=s-corr+2*static_cast<int>(phase)+1;
                    Check(SelectLeft(x,y,1080,c,phase,p)==(((value%8+8)%8)>=4),"legacy mask oracle");
                }
        }
        CameraTransform transform; transform.tilt=0; transform.translation={0,0,0};
        EyePairBuffer buffer(transform);
        Check(!buffer.Accept("/eye/right",{.03f,0,1},1000000000),"one eye is not a pair");
        Check(buffer.Accept("/eye/left",{-.03f,0,1},1000000001),"named eye pairing");
        auto sample=buffer.Read(1000000001);
        Check(sample.left[0]<0 && sample.right[0]>0 && sample.fresh && sample.sequence==1,"display eye order");
        Check(!buffer.Read(999999999).valid,"never use a future received sample");
        Check(!buffer.Read(1300000001).fresh && buffer.Read(1300000001).valid,"held sample freshness");
        Check(!buffer.Accept("/eye/left",{-.03f,0,1},1400000000),"new pair left only");
        Check(!buffer.Accept("/eye/right",{.03f,0,1},1600000000),"reject skewed pair");
        Check(buffer.Accept("/eye/left",{-.03f,0,1},1600000001),"recover after skew");
        Check(!buffer.Accept("/eye/left",{std::numeric_limits<float>::quiet_NaN(),0,1},1700000000),"reject NaN");
        Check(!buffer.Accept("/eye/center",{0,0,1},1700000000),"ignore center");
        const auto point=transform.Apply({.1f,.2f,1});
        Check(point[0]==.1f && point[1]==-.2f && point[2]==1,"raw camera axes and metres");
        Barrier dynamic;
        Check(!TrackedBarrier(0,.3245f,0,.65f,.7f,dynamic),"optical singularity guard");
        Check(TrackedBarrier(0,.65f,0,.65f,.7f,dynamic) && dynamic.middle==3420 && dynamic.haba==330,"reference calibration");
        Barrier custom; custom.haba=400; custom.middle=3500;
        Check(TrackedBarrier(.01f,.65f,0,.65f,.7f,custom) && custom.haba==400 && custom.middle>3500,
              "custom reference preserved and X-right optical correction sign");
        std::cout << "PASS: optical oracle, quantization, named eyes, units, history, skew, stale transport\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
