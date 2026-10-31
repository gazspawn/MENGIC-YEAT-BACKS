#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class MengicYeatBacksAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit MengicYeatBacksAudioProcessorEditor(MengicYeatBacksAudioProcessor&);
    ~MengicYeatBacksAudioProcessorEditor() override = default;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    MengicYeatBacksAudioProcessor& processor;
    struct Knob : juce::Component
    {
        juce::Slider slider; juce::Label label;
        Knob(const juce::String& name);
        void resized() override;
    };
    std::unique_ptr<Knob> knobs[9];
    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<Attachment> attachments[9];
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MengicYeatBacksAudioProcessorEditor)
};
