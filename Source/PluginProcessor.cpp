#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace {
const char* ids[]={"convDry","convWet","revDry","revWet","revMix","revSize","enhance",
 "delayMs","delayWet","transparency","convOn","revOn","enhanceOn","delayOn","transparencyOn",
 "bypass","convType","revType"};
}
MengicYeatBacksAudioProcessor::MengicYeatBacksAudioProcessor()
 :AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true)
 .withOutput("Output",juce::AudioChannelSet::stereo(),true)),
 parameters(*this,nullptr,"ALIEN_PARAMETERS",createParameterLayout())
{
    for(size_t i=0;i<values.size();++i)values[i]=parameters.getRawParameterValue(ids[i]);
}
juce::AudioProcessorValueTreeState::ParameterLayout MengicYeatBacksAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout p;
    auto f=[&](const char* id,const char* name,float def) {
        p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id,1},name,
            juce::NormalisableRange<float>(0,100,.1f),def));
    };
    f("convDry","Convolver Dry",100); f("convWet","Convolver Wet",25);
    f("revDry","Reverb Dry",100);f("revWet","Reverb Wet",40);
    f("revMix","Reverb Mix",30);f("revSize","Reverb Size",60);
    f("enhance","Enhancer Amount",20);
    p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"delayMs",1},"Delay Time",
        juce::NormalisableRange<float>(40,1000,1,.5f),280));
    f("delayWet","Delay Wet",18);f("transparency","Transparency",25);
    for(const char* id:{"convOn","revOn","enhanceOn","delayOn","transparencyOn","bypass"})
        p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{id,1},juce::String(id),juce::String(id)!="bypass"));
    p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"convType",1},"Convolver Space",
        juce::StringArray{"Vocal Chamber","Glass Cell","Alien Vessel"},0));
    p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"revType",1},"Reverb Colour",
        juce::StringArray{"Acid Hall","Dark Orbit","Soft Nebula"},0));
    return p;
}
bool MengicYeatBacksAudioProcessor::isBusesLayoutSupported(const BusesLayout& l) const
{
    auto in=l.getMainInputChannelSet();
    return in==l.getMainOutputChannelSet() && (in==juce::AudioChannelSet::mono() || in==juce::AudioChannelSet::stereo());
}
void MengicYeatBacksAudioProcessor::prepareToPlay(double rate,int) {dsp.prepare(rate);}
void MengicYeatBacksAudioProcessor::processBlock(juce::AudioBuffer<float>& b,juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    AlienDSP::Settings s;
    s.convDry=values[0]->load()*.01f;s.convWet=values[1]->load()*.01f;
    s.revDry=values[2]->load()*.01f;s.revWet=values[3]->load()*.01f;
    s.revMix=values[4]->load()*.01f;s.revSize=values[5]->load()*.01f;
    s.enhance=values[6]->load()*.01f;s.delayMs=values[7]->load();
    s.delayWet=values[8]->load()*.01f;s.transparency=values[9]->load()*.01f;
    s.convOn=values[10]->load()>.5f;s.revOn=values[11]->load()>.5f;
    s.enhanceOn=values[12]->load()>.5f;s.delayOn=values[13]->load()>.5f;
    s.transparencyOn=values[14]->load()>.5f;s.bypass=values[15]->load()>.5f;
    s.convType=(int)values[16]->load();s.revType=(int)values[17]->load();
    dsp.set(s);
    const int channels=b.getNumChannels();
    if(channels<1)return;
    auto* l=b.getWritePointer(0);
    auto* r=channels>1?b.getWritePointer(1):nullptr;
    for(int i=0;i<b.getNumSamples();++i) {
        float left=l[i],right=r?r[i]:left;
        dsp.tick(left,right,channels);
        l[i]=left;if(r)r[i]=right;
    }
}
void MengicYeatBacksAudioProcessor::applyPreset(int index)
{
    const float presets[4][10]={
        {100,25,100,40,30,60,20,280,18,25},
        {100,38,100,58,48,78,38,360,28,15},
        {100,14,100,28,20,38,12,180,10,55},
        {100,0,100,0,0,50,0,280,0,0}};
    index=juce::jlimit(0,3,index);
    auto set=[&](const char* id,float value) {
        auto* p=parameters.getParameter(id);
        p->beginChangeGesture();p->setValueNotifyingHost(p->convertTo0to1(value));p->endChangeGesture();
    };
    for(int i=0;i<10;++i)set(ids[i],presets[index][i]);
    for(int i=10;i<15;++i)set(ids[i],1);
    set("bypass",0);
    set("convType",index==1?2.f:0.f);set("revType",index==2?2.f:0.f);
}
juce::AudioProcessorEditor* MengicYeatBacksAudioProcessor::createEditor()
{return new MengicYeatBacksAudioProcessorEditor(*this);}
void MengicYeatBacksAudioProcessor::getStateInformation(juce::MemoryBlock& dest)
{auto state=parameters.copyState();auto xml=state.createXml();copyXmlToBinary(*xml,dest);}
void MengicYeatBacksAudioProcessor::setStateInformation(const void* data,int size)
{auto xml=getXmlFromBinary(data,size);if(xml && xml->hasTagName(parameters.state.getType()))parameters.replaceState(juce::ValueTree::fromXml(*xml));}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new MengicYeatBacksAudioProcessor();}
