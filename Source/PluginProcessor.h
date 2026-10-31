#pragma once
#include <JuceHeader.h>

class MengicYeatBacksAudioProcessor : public juce::AudioProcessor
{
public:
    MengicYeatBacksAudioProcessor();
    ~MengicYeatBacksAudioProcessor() override = default;
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "MENGIC - YEAT BACKS"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    juce::AudioProcessorValueTreeState parameters;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    static constexpr int maxChannels = 2;
    juce::AudioBuffer<float> pitchBuffer;
    int pitchWrite = 0;
    double sr = 44100.0;
    juce::Reverb reverb;
    juce::Reverb::Parameters reverbParams;
    juce::AudioBuffer<float> delayBuffer;
    int delayWrite = 0;
    juce::SmoothedValue<float> wetSmooth, outputSmooth;
    float lowState[maxChannels]{};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MengicYeatBacksAudioProcessor)
};
