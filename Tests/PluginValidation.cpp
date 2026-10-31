#include "../Source/PluginProcessor.h"
#include <iostream>
int main()
{
    juce::ScopedJuceInitialiser_GUI gui;
    MengicYeatBacksAudioProcessor p;
    p.prepareToPlay(48000,256);
    for(int i=0;i<4;++i)p.applyPreset(i);
    auto* parameter=p.parameters.getParameter("transpose");
    parameter->setValueNotifyingHost(parameter->convertTo0to1(5.0f));
    juce::MemoryBlock state;p.getStateInformation(state);
    MengicYeatBacksAudioProcessor restored;
    restored.setStateInformation(state.getData(),(int)state.getSize());
    if(std::abs(restored.parameters.getRawParameterValue("transpose")->load()-5.f)>.01f)return 1;
    if(p.getLatencySamples()<=0)return 5;
    p.applyPreset(0);
    juce::AudioBuffer<float> buffer(2,256);juce::MidiBuffer midi;
    buffer.clear();p.processBlock(buffer,midi);
    if(buffer.getMagnitude(0,256)!=0)return 2;
    std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());
    editor->setSize(1100,720);
    editor->setVisible(true);
    juce::Image image(juce::Image::RGB,1100,720,true,juce::SoftwareImageType());
    juce::Graphics graphics(image);editor->paintEntireComponent(graphics,true);
    if(image.getPixelAt(100,100)==juce::Colours::black){std::cerr<<"UI render is blank";return 4;}
    juce::File output=juce::File::getCurrentWorkingDirectory().getChildFile("MENGIC_ALIEN_UI.png");
    juce::FileOutputStream stream(output);
    juce::PNGImageFormat png;
    if(!png.writeImageToStream(image,stream))return 3;
    editor.reset();
    std::cout<<"PASS: presets, state roundtrip, silence, UI render\n";
}
