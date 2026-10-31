#pragma once
#include <array>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdint>
#include "PitchShifter.h"

// Streaming DSP: prepare allocates all storage; tick performs no allocations.
class AlienDSP
{
public:
    struct Settings
    {
        float convDry=1, convWet=.25f, revDry=1, revWet=.4f, revMix=.3f, revSize=.6f;
        float flangusRate=.4f, delayMs=280, delayWet=.18f, flangusDepth=.5f;
        float flangusMix=.35f, transpose=0;
        bool convOn=true, revOn=true, flangusOn=true, delayOn=true, transposeOn=true, bypass=false;
        int convType=0, revType=0, flangusMode=0;
    };
    void prepare(double sampleRate)
    {
        sr=sampleRate;
        ringLength=(int)(sr*1.1)+8;
        for(auto& r:history) r.assign((size_t)ringLength,0);
        for(auto& r:echo) r.assign((size_t)ringLength,0);
        for(auto& r:flange) r.assign((size_t)ringLength,0);
        for(auto& r:inputHistory) r.assign((size_t)ringLength,0);
        pitch.prepare(sr);
        latency=(int)std::round(sr*.1025);
        lfo=0;
        const double times[4]={.0297,.0371,.0411,.0437};
        for(int c=0;c<2;++c) {
            for(int k=0;k<4;++k) comb[(size_t)c][(size_t)k].prepare((int)(sr*(times[k]+c*.0013)));
            for(int k=0;k<2;++k) allpass[(size_t)c][(size_t)k].prepare((int)(sr*(.005+k*.0021+c*.0003)));
        }
        // Original sparse impulse responses, in seconds. Three small acoustic spaces.
        for(int m=0;m<3;++m) for(int c=0;c<2;++c) {
            float norm=0;
            for(int k=0;k<24;++k) {
                const double t=k==0?0.0:(.0018+k*(.0017+.00065*m)+c*.00041*(k%3));
                taps[(size_t)m][(size_t)c][(size_t)k]=(int)(sr*t);
                float a=k==0?.7f:std::exp(-k*(.16f-.025f*m))*.13f;
                if(k%3==1) a=-a;
                weights[(size_t)m][(size_t)c][(size_t)k]=a;
                norm+=std::abs(a);
            }
            for(auto& w:weights[(size_t)m][(size_t)c]) w/=norm;
        }
        write=0; ready=false;
        smoothing=1.0f-std::exp(-1.0f/(float)(sr*.025));
    }
    void set(const Settings& s)
    {
        target={s.convDry,s.convWet,s.revDry,s.revWet,s.revMix,s.revSize,s.flangusRate,
                s.delayMs,s.delayWet,s.flangusDepth,s.convOn?1.f:0.f,s.revOn?1.f:0.f,
                s.flangusOn?1.f:0.f,s.delayOn?1.f:0.f,s.transposeOn?1.f:0.f,s.bypass?1.f:0.f,
                s.convType==0?1.f:0.f,s.convType==1?1.f:0.f,s.convType==2?1.f:0.f,
                s.revType==0?0.f:(s.revType==1?.5f:1.f),
                s.flangusMix,(float)std::pow(2.0,s.transpose/12.0),s.flangusMode==0?0.f:1.f};
        if(!ready) {current=target; ready=true;}
    }
    void tick(float& left,float& right,int channels)
    {
        for(size_t i=0;i<current.size();++i) current[i]+=smoothing*(target[i]-current[i]);
        const auto& p=current;
        const float input[2]={finite(left),finite(channels==1?left:right)};
        float pitched[2]={input[0],input[1]};
        pitch.process(pitched,2,p[21]);
        float dry[2]{};
        for(int c=0;c<2;++c) {
            inputHistory[(size_t)c][(size_t)write]=input[c];
            int pos=write-latency;if(pos<0)pos+=ringLength;
            dry[c]=inputHistory[(size_t)c][(size_t)pos];
        }
        const float shiftAmount=std::min(1.f,std::abs(p[21]-1.f)*200.f);
        lfo+=p[6]/sr;
        lfo-=std::floor(lfo);
        float result[2]{};
        for(int c=0;c<2;++c) {
            const size_t ch=(size_t)c;
            float x=blend(dry[c],pitched[c],p[14]*shiftAmount);
            const float mode=p[22];
            const float centreMs=blend(15.f,2.0f,mode);
            const float depthMs=blend(7.f,1.7f,mode)*p[9];
            float voice=0;
            for(int v=0;v<3;++v) {
                const double phase=lfo+(c==0?0.0:.25)+(double)v/3.0;
                const float modMs=centreMs+(1-mode)*v*.7f+depthMs*(float)std::sin(6.283185307179586*phase);
                float fp=(float)write-(float)sr*modMs*.001f;
                if(fp<0)fp+=(float)ringLength;
                const int fi=(int)fp;
                const float ff=fp-(float)fi;
                voice+=(flange[ch][(size_t)fi]*(1-ff)+flange[ch][(size_t)((fi+1)%ringLength)]*ff)/3.f;
            }
            flange[ch][(size_t)write]=x+voice*(.15f+.3f*mode);
            const float flanged=x*(1-.5f*p[20])+voice*.5f*p[20];
            x=blend(x,flanged,p[12]);
            history[ch][(size_t)write]=x;
            float convolved=0;
            for(int m=0;m<3;++m) {
                float sum=0;
                for(int k=0;k<24;++k) {
                    int pos=write-taps[(size_t)m][ch][(size_t)k];
                    if(pos<0) pos+=ringLength;
                    sum+=history[ch][(size_t)pos]*weights[(size_t)m][ch][(size_t)k];
                }
                convolved+=sum*p[(size_t)(16+m)];
            }
            x=blend(x,x*p[0]+convolved*p[1],p[10]);
            float pos=(float)write-(float)sr*p[7]*.001f;
            if(pos<0) pos+=(float)ringLength;
            const int j=(int)pos;
            const float frac=pos-(float)j;
            const float delayed=echo[ch][(size_t)j]*(1-frac)+echo[ch][(size_t)((j+1)%ringLength)]*frac;
            echo[ch][(size_t)write]=x+delayed*.32f;
            x+=delayed*p[8]*p[13]*.7f;
            float space=0;
            for(int k=0;k<4;++k)
                space+=comb[ch][(size_t)k].process(x,.45f+.45f*p[5],.2f+.35f*p[19]);
            space*=.25f;
            for(int k=0;k<2;++k) space=allpass[ch][(size_t)k].diffuse(space);
            const float reverbed=blend(x,x*p[2]+space*p[3],p[4]);
            x=blend(x,reverbed,p[11]);
            result[c]=blend(x,dry[c],p[15]);
        }
        left=channels==1?(result[0]+result[1])*.5f:result[0];
        right=result[1];
        if(++write==ringLength) write=0;
    }
int getLatencySamples() const {return latency;}
private:
    struct Delay
    {
        std::vector<float> data;
        int index=0;
        float low=0;
        void prepare(int n) {data.assign((size_t)std::max(1,n),0);index=0;low=0;}
        float process(float x,float feedback,float damping) {
            float y=data[(size_t)index];
            low=(1-damping)*y+damping*low;
            data[(size_t)index]=x+low*feedback;
            advance();return y;
        }
        float diffuse(float x) {
            float delayed=data[(size_t)index];
            float y=delayed-x;
            data[(size_t)index]=x+delayed*.5f;
            advance();return y;
        }
        void advance() {if(++index==(int)data.size())index=0;}
    };
    static float blend(float a,float b,float t) {return t<=0?a:(t>=1?b:a+(b-a)*t);}
    static float finite(float x) {return std::isfinite(x)?x:0.f;}
    double sr=44100;
    int ringLength=0,write=0;
    bool ready=false;
    float smoothing=0;
    std::array<float,23> target{},current{};
    std::array<std::vector<float>,2> history,echo,flange,inputHistory;
    MengicPitchShifter pitch;
    int latency=0;
    double lfo=0;
    std::array<std::array<Delay,4>,2> comb;
    std::array<std::array<Delay,2>,2> allpass;
    std::array<std::array<std::array<int,24>,2>,3> taps{};
    std::array<std::array<std::array<float,24>,2>,3> weights{};
};
