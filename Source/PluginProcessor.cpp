#include "PluginProcessor.h"
#include "PluginEditor.h"

static juce::AudioParameterFloat* floatParam(juce::AudioProcessorValueTreeState& s, const char* id)
{ return dynamic_cast<juce::AudioParameterFloat*>(s.getParameter(id)); }

MengicYeatBacksAudioProcessor::MengicYeatBacksAudioProcessor()
 : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                  .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
   parameters(*this, nullptr, "PARAMETERS", createParameterLayout()) {}

juce::AudioProcessorValueTreeState::ParameterLayout MengicYeatBacksAudioProcessor::createParameterLayout()
{
    using F = juce::AudioParameterFloat;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    auto add = [&p](const char* id, const char* name, float lo, float hi, float def, float step = 0.01f)
    { p.push_back(std::make_unique<F>(juce::ParameterID{id, 1}, name, juce::NormalisableRange<float>(lo, hi, step), def)); };
    add("pitch", "PITCH", -12.0f, 12.0f, 5.0f, 0.1f);
    add("formant", "FORMANT", 0.0f, 100.0f, 28.0f);
    add("drive", "DRIVE", 0.0f, 100.0f, 32.0f);
    add("width", "WIDTH", 0.0f, 200.0f, 145.0f);
    add("delay", "DELAY", 0.0f, 100.0f, 18.0f);
    add("feedback", "FEEDBACK", 0.0f, 85.0f, 28.0f);
    add("reverb", "REVERB", 0.0f, 100.0f, 24.0f);
    add("mix", "MIX", 0.0f, 100.0f, 75.0f);
    add("output", "OUTPUT", -18.0f, 6.0f, 0.0f);
    return { p.begin(), p.end() };
}

bool MengicYeatBacksAudioProcessor::isBusesLayoutSupported(const BusesLayout& l) const
{
    return l.getMainInputChannelSet() == l.getMainOutputChannelSet()
        && (l.getMainInputChannelSet() == juce::AudioChannelSet::mono()
            || l.getMainInputChannelSet() == juce::AudioChannelSet::stereo());
}

void MengicYeatBacksAudioProcessor::prepareToPlay(double sampleRate, int)
{
    sr = sampleRate;
    pitchShifter.prepare(sr);
    for (auto* smoother : { &pitchRatioSmooth, &delayTimeSmooth, &formantSmooth,
                           &driveSmooth, &widthSmooth, &delaySmooth, &feedbackSmooth })
        smoother->reset(sr, 0.05);
    pitchRatioSmooth.setCurrentAndTargetValue(std::pow(2.0f, parameters.getRawParameterValue("pitch")->load() / 12.0f));
    const float delay = parameters.getRawParameterValue("delay")->load() / 100.0f;
    delayTimeSmooth.setCurrentAndTargetValue((float)sr * (0.18f + delay * 0.32f));
    formantSmooth.setCurrentAndTargetValue(parameters.getRawParameterValue("formant")->load() / 100.0f);
    driveSmooth.setCurrentAndTargetValue(parameters.getRawParameterValue("drive")->load() / 100.0f);
    widthSmooth.setCurrentAndTargetValue(parameters.getRawParameterValue("width")->load() / 100.0f);
    delaySmooth.setCurrentAndTargetValue(delay);
    feedbackSmooth.setCurrentAndTargetValue(parameters.getRawParameterValue("feedback")->load() / 100.0f * 0.82f);
    std::fill(std::begin(lowState), std::end(lowState), 0.0f);
    delayBuffer.setSize(maxChannels, (int)(sr * 1.2) + 8); delayBuffer.clear(); delayWrite = 0;
    wetSmooth.reset(sr, 0.03); outputSmooth.reset(sr, 0.03);
    wetSmooth.setCurrentAndTargetValue(parameters.getRawParameterValue("mix")->load() / 100.0f);
    outputSmooth.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(parameters.getRawParameterValue("output")->load()));
    reverb.reset();
}
void MengicYeatBacksAudioProcessor::releaseResources() {}

void MengicYeatBacksAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int channels = juce::jmin(buffer.getNumChannels(), maxChannels), n = buffer.getNumSamples();
    const float pitch = parameters.getRawParameterValue("pitch")->load();
    const float formant = parameters.getRawParameterValue("formant")->load() / 100.0f;
    const float drive = parameters.getRawParameterValue("drive")->load() / 100.0f;
    const float width = parameters.getRawParameterValue("width")->load() / 100.0f;
    const float delayAmt = parameters.getRawParameterValue("delay")->load() / 100.0f;
    const float feedback = parameters.getRawParameterValue("feedback")->load() / 100.0f * 0.82f;
    const float rev = parameters.getRawParameterValue("reverb")->load() / 100.0f;
    wetSmooth.setTargetValue(parameters.getRawParameterValue("mix")->load() / 100.0f);
    outputSmooth.setTargetValue(juce::Decibels::decibelsToGain(parameters.getRawParameterValue("output")->load()));
    pitchRatioSmooth.setTargetValue(std::pow(2.0f, pitch / 12.0f));
    delayTimeSmooth.setTargetValue((float)sr * (0.18f + delayAmt * 0.32f));
    formantSmooth.setTargetValue(formant);
    driveSmooth.setTargetValue(drive);
    widthSmooth.setTargetValue(width);
    delaySmooth.setTargetValue(delayAmt);
    feedbackSmooth.setTargetValue(feedback);
    reverbParams.roomSize = 0.35f + rev * 0.58f; reverbParams.damping = 0.45f;
    reverbParams.wetLevel = rev * 0.38f; reverbParams.dryLevel = 1.0f; reverbParams.width = 1.0f;
    reverb.setParameters(reverbParams);

    for (int i=0; i<n; ++i)
    {
        const float dryL = buffer.getSample(0,i);
        const float dryR = channels > 1 ? buffer.getSample(1,i) : dryL;
        const float formantValue = formantSmooth.getNextValue();
        const float driveValue = driveSmooth.getNextValue();
        const float widthValue = widthSmooth.getNextValue();
        const float delayValue = delaySmooth.getNextValue();
        const float feedbackValue = feedbackSmooth.getNextValue();
        const float delaySamples = delayTimeSmooth.getNextValue();
        float wet[2]{dryL, dryR};
        pitchShifter.process(wet, channels, pitchRatioSmooth.getNextValue());
        for (int ch=0; ch<channels; ++ch)
        {
            const float shifted = wet[ch];
            // Formant-style tone shaping: variable resonant emphasis plus low-mid thinning.
            const float hp = shifted - lowState[ch];
            lowState[ch] += 0.025f * (shifted - lowState[ch]);
            float shaped = shifted + hp * (formantValue * 1.8f);
            shaped = std::tanh(shaped * (1.0f + driveValue * 11.0f));
            wet[ch] = shaped;
        }
        if (channels == 2)
        {
            const float mid = (wet[0] + wet[1]) * 0.5f, side = (wet[0] - wet[1]) * 0.5f * widthValue;
            wet[0] = mid + side; wet[1] = mid - side;
        }
        for (int ch=0; ch<channels; ++ch)
        {
            const int delayLen = delayBuffer.getNumSamples();
            float position = (float)delayWrite - delaySamples;
            if (position < 0.0f) position += (float)delayLen;
            const int index = (int)position;
            const float fraction = position - (float)index;
            const float delayed = delayBuffer.getSample(ch, index) * (1.0f - fraction)
                                + delayBuffer.getSample(ch, (index + 1) % delayLen) * fraction;
            delayBuffer.setSample(ch, delayWrite, wet[ch] + delayed * feedbackValue);
            wet[ch] += delayed * delayValue * 0.8f;
        }
        float revL = wet[0], revR = channels > 1 ? wet[1] : wet[0];
        if (channels == 2) { reverb.processStereo(&revL, &revR, 1); }
        else { reverb.processMono(&revL, 1); }
        const float mix = wetSmooth.getNextValue(), gain = outputSmooth.getNextValue();
        buffer.setSample(0,i, (dryL * (1.0f-mix) + revL * mix) * gain);
        if (channels > 1) buffer.setSample(1,i, (dryR * (1.0f-mix) + revR * mix) * gain);
        delayWrite = (delayWrite + 1) % delayBuffer.getNumSamples();
    }
}

juce::AudioProcessorEditor* MengicYeatBacksAudioProcessor::createEditor() { return new MengicYeatBacksAudioProcessorEditor(*this); }
void MengicYeatBacksAudioProcessor::getStateInformation(juce::MemoryBlock& dest) { auto state = parameters.copyState(); std::unique_ptr<juce::XmlElement> xml(state.createXml()); copyXmlToBinary(*xml, dest); }
void MengicYeatBacksAudioProcessor::setStateInformation(const void* data, int size) { std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data,size)); if (xml && xml->hasTagName(parameters.state.getType())) parameters.replaceState(juce::ValueTree::fromXml(*xml)); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new MengicYeatBacksAudioProcessor(); }
