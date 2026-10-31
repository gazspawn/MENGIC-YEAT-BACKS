#pragma once
#include <array>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdint>

// Streaming DSP: prepare allocates all storage; tick performs no allocations.
class AlienDSP
{
public:
    struct Settings
    {
        float convDry=1, convWet=.25f, revDry=1, revWet=.4f, revMix=.3f, revSize=.6f;
        float enhance=.2f, delayMs=280, delayWet=.18f, transparency=.25f;
        bool convOn=true, revOn=true, enhanceOn=true, delayOn=true, transparencyOn=true, bypass=false;
        int convType=0, revType=0;
    };
    void prepare(double sampleRate)
    {
        sr=sampleRate;
        ringLength=(int)(sr*1.1)+8;
        for(auto& r:history) r.assign((size_t)ringLength,0);
        for(auto& r:echo) r.assign((size_t)ringLength,0);
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
        target={s.convDry,s.convWet,s.revDry,s.revWet,s.revMix,s.revSize,s.enhance,
                s.delayMs,s.delayWet,s.transparency,s.convOn?1.f:0.f,s.revOn?1.f:0.f,
                s.enhanceOn?1.f:0.f,s.delayOn?1.f:0.f,s.transparencyOn?1.f:0.f,s.bypass?1.f:0.f,
                s.convType==0?1.f:0.f,s.convType==1?1.f:0.f,s.convType==2?1.f:0.f,
                s.revType==0?0.f:(s.revType==1?.5f:1.f)};
        if(!ready) {current=target; ready=true;}
    }
    void tick(float& left,float& right,int channels)
    {
        for(size_t i=0;i<current.size();++i) current[i]+=smoothing*(target[i]-current[i]);
        const auto& p=current;
        const float dry[2]={finite(left),finite(channels==1?left:right)};
        float result[2]{};
        for(int c=0;c<2;++c) {
            const size_t ch=(size_t)c;
            float x=dry[c];
            const float g=1.0f+7.0f*p[6];
            const float saturated=std::tanh(x*g)/std::sqrt(g);
            x=blend(x,saturated,p[12]*p[6]);
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
            // Transparency means original signal: 100% is unprocessed.
            x=blend(x,dry[c],p[9]*p[14]);
            result[c]=blend(x,dry[c],p[15]);
        }
        left=channels==1?(result[0]+result[1])*.5f:result[0];
        right=result[1];
        if(++write==ringLength) write=0;
    }
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
    std::array<float,20> target{},current{};
    std::array<std::vector<float>,2> history,echo;
    std::array<std::array<Delay,4>,2> comb;
    std::array<std::array<Delay,2>,2> allpass;
    std::array<std::array<std::array<int,24>,2>,3> taps{};
    std::array<std::array<std::array<float,24>,2>,3> weights{};
};
