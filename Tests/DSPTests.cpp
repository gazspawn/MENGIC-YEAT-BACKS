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
            const int j=i-d.getLatencySamples();
            const float expected=j<0?0.f:.1f*(float)std::sin(j*.1);
            check(l==expected,"Bypass latency mismatch");
            if(channels==2)check(r==-expected,"Stereo bypass changed input");
        }
        d.prepare(rate);s=AlienDSP::Settings{};d.set(s);
        check(render(d,10000,channels,0)==0,"Silence generated noise");
        d.prepare(rate);s.flangusDepth=1;s.flangusMix=1;s.transpose=12;s.convWet=1;s.delayWet=1;s.revWet=1;s.revMix=1;s.revSize=1;d.set(s);
        render(d,(int)(rate*2),channels,.8f);
        for(int i=0;i<200;++i) {
            s.convType=i%3;s.revType=i%3;s.bypass=i%2==0;s.convOn=i%3==0;
            s.delayMs=i%2==0?40.f:1000.f;s.transpose=i%2==0?-12.f:12.f;s.flangusMode=i%2;s.flangusRate=i%2==0?.05f:5.f;
            d.set(s);render(d,128,channels);
        }
        d.prepare(rate);d.set(s);
        check(render(d,10000,channels,0)==0,"prepare did not reset DSP");
    }
    // Each audible module must change the signal relative to an all-off chain.
    for(int module=0;module<5;++module) {
        AlienDSP a,b;a.prepare(48000);b.prepare(48000);
        AlienDSP::Settings off;off.convOn=off.revOn=off.flangusOn=off.delayOn=false;off.transposeOn=false;
        auto on=off;
        if(module==0){on.convOn=true;on.convWet=.8f;}
        if(module==1){on.revOn=true;on.revWet=1;on.revMix=1;}
        if(module==2){on.flangusOn=true;on.flangusMix=1;}
        if(module==3){on.delayOn=true;on.delayWet=1;}
        if(module==4){on.transposeOn=true;on.transpose=7;}
        a.set(off);b.set(on);double difference=0;
        for(int i=0;i<96000;++i) {
            float l=.1f*(float)std::sin(i*.083),r=l,al=l,ar=r;
            a.tick(al,ar,2);b.tick(l,r,2);difference+=std::abs(l-al);
        }
        check(difference>1,"Module had no audible effect");
    }
    // Frequency direction and absence of discontinuities across repeated pitch-buffer wraps.
    for(float semitones:{-12.f,0.f,5.f,12.f}) {
        AlienDSP d;d.prepare(48000);
        AlienDSP::Settings s;s.convOn=s.revOn=s.flangusOn=s.delayOn=false;s.transpose=semitones;d.set(s);
        float previous=0,maxStep=0;int crossings=0;
        for(int i=0;i<144000;++i) {
            float l=.5f*(float)std::sin(6.283185307179586*440*i/48000),r=l;
            d.tick(l,r,2);
            if(i>=48000) {
                if(previous<=0 && l>0)++crossings;
                maxStep=std::max(maxStep,std::abs(l-previous));
            }
            previous=l;
        }
        const double measured=crossings/2.0,expected=440*std::pow(2.0,semitones/12.0);
        check(std::abs(measured-expected)<15,"Transpose frequency incorrect");
        check(maxStep<.08f,"Transpose discontinuity");
    }
    std::cout<<"PASS: sample rates, mono/stereo, bypass latency, transpose frequency and continuity, silence, reset, extreme settings, automation, all effects\n";
}
