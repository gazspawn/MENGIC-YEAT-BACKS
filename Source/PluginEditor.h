#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class MengicYeatBacksAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit MengicYeatBacksAudioProcessorEditor(MengicYeatBacksAudioProcessor&);
    ~MengicYeatBacksAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    struct AlienLook : juce::LookAndFeel_V4
    {
        AlienLook();
        void drawRotarySlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider&) override;
        void drawToggleButton(juce::Graphics&,juce::ToggleButton&,bool,bool) override;
        juce::Font getLabelFont(juce::Label&) override {return juce::Font(15.0f);}
    };
    struct Knob : juce::Component
    {
        juce::Slider slider;
        juce::String name;
        explicit Knob(juce::String,bool=false);
        void resized() override;
        void paint(juce::Graphics&) override;
    };
    void timerCallback() override {phase+=.045f;repaint();}
    void drawTube(juce::Graphics&,juce::Rectangle<float>);
    MengicYeatBacksAudioProcessor& processor;
    AlienLook look;
    juce::Component canvas;
    std::array<std::unique_ptr<Knob>,12> knobs;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>,12> sliders;
    std::array<juce::ToggleButton,6> switches;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>,6> buttons;
    juce::ComboBox presets,convType,revType,flangusMode;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> convChoice,revChoice,flangusChoice;
    juce::TooltipWindow tips;
    float phase=0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MengicYeatBacksAudioProcessorEditor)
};
