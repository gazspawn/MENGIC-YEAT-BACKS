#include "../Source/AlienDSP.h"
#include <iostream>
#include <limits>
#include <cstdlib>
static void check(bool ok,const char* message)
{if(!ok){std::cerr<<message<<"\n";std::exit(1);}}
static double render(AlienDSP& d,int samples,int channels,float amplitude=.1f)
{
    double energy=0;
    for(int i=0;i<samples;++i) {
        float l=amplitude*(float)std::sin(i*.063),r=l;
        d.tick(l,r,channels);
        check(std::isfinite(l)&&std::isfinite(r),"Non-finite output");
        check(std::abs(l)<20&&std::abs(r)<20,"Unstable feedback");
        energy+=l*l+r*r;
    }
    return energy;
}
int main()
{
    for(double rate:{44100.,48000.,96000.})for(int channels:{1,2}) {
        AlienDSP d;d.prepare(rate);AlienDSP::Settings s;s.bypass=true;d.set(s);
        for(int i=0;i<10000;++i) {
            float input=.1f*(float)std::sin(i*.1),l=input,r=-input;
            d.tick(l,r,channels);
            check(l==input,"Bypass changed input");
            if(channels==2)check(r==-input,"Stereo bypass changed input");
        }
        d.prepare(rate);s.bypass=false;s.transparency=1;d.set(s);
        for(int i=0;i<10000;++i) {
            float l=.1f,r=.1f;d.tick(l,r,channels);
            check(std::abs(l-.1f)<1e-6f,"100% transparency is not dry");
        }
        d.prepare(rate);s=AlienDSP::Settings{};d.set(s);
        check(render(d,10000,channels,0)==0,"Silence generated noise");
        d.prepare(rate);s.enhance=1;s.convWet=1;s.delayWet=1;s.revWet=1;s.revMix=1;s.revSize=1;s.transparency=0;d.set(s);
        render(d,(int)(rate*2),channels,.8f);
        for(int i=0;i<200;++i) {
            s.convType=i%3;s.revType=i%3;s.bypass=i%2==0;s.convOn=i%3==0;
            s.delayMs=i%2==0?40.f:1000.f;s.transparency=(i%2==0)?0.f:1.f;
            d.set(s);render(d,128,channels);
        }
        d.prepare(rate);d.set(s);
        check(render(d,10000,channels,0)==0,"prepare did not reset DSP");
    }
    // Each audible module must change the signal relative to an all-off chain.
    for(int module=0;module<4;++module) {
        AlienDSP a,b;a.prepare(48000);b.prepare(48000);
        AlienDSP::Settings off;off.convOn=off.revOn=off.enhanceOn=off.delayOn=false;off.transparency=0;
        auto on=off;
        if(module==0){on.convOn=true;on.convWet=.8f;}
        if(module==1){on.revOn=true;on.revWet=1;on.revMix=1;}
        if(module==2){on.enhanceOn=true;on.enhance=1;}
        if(module==3){on.delayOn=true;on.delayWet=1;}
        a.set(off);b.set(on);double difference=0;
        for(int i=0;i<96000;++i) {
            float l=.1f*(float)std::sin(i*.083),r=l,al=l,ar=r;
            a.tick(al,ar,2);b.tick(l,r,2);difference+=std::abs(l-al);
        }
        check(difference>1,"Module had no audible effect");
    }
    std::cout<<"PASS: sample rates, mono/stereo, bypass, transparency, silence, reset, extreme settings, automation, all effects\n";
}
