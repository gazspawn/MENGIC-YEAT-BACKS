#include "PluginEditor.h"

MengicYeatBacksAudioProcessorEditor::Knob::Knob(const juce::String& name)
{
    addAndMakeVisible(slider);
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 72, 20);
    slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xffb6ff36));
    slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff303a29));
    slider.setColour(juce::Slider::thumbColourId, juce::Colour(0xffe8ffc0));
    slider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
    slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff151a14));
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff303a29));
    label.setText(name, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colour(0xffd8e4ce));
    label.setFont(juce::Font(12.0f, juce::Font::bold));
    addAndMakeVisible(label);
}
void MengicYeatBacksAudioProcessorEditor::Knob::resized()
{
    auto r = getLocalBounds(); label.setBounds(r.removeFromTop(22)); slider.setBounds(r);
}

MengicYeatBacksAudioProcessorEditor::MengicYeatBacksAudioProcessorEditor(MengicYeatBacksAudioProcessor& p)
 : AudioProcessorEditor(&p), processor(p)
{
    const char* ids[] = {"pitch","formant","drive","width","delay","feedback","reverb","mix","output"};
    const char* names[] = {"PITCH","FORMANT","DRIVE","WIDTH","DELAY","FEEDBACK","REVERB","MIX","OUTPUT"};
    for (int i=0;i<9;++i)
    {
        knobs[i] = std::make_unique<Knob>(names[i]); addAndMakeVisible(*knobs[i]);
        attachments[i] = std::make_unique<Attachment>(processor.parameters, ids[i], knobs[i]->slider);
    }
    setSize(720, 430);
}
void MengicYeatBacksAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff090d09));
    g.setColour(juce::Colour(0xff141d12)); g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(12), 18.0f);
    g.setColour(juce::Colour(0xffb6ff36));
    g.setFont(juce::Font(27.0f, juce::Font::bold)); g.drawText("MENGIC", 30, 20, 190, 38, juce::Justification::left);
    g.setColour(juce::Colours::white.withAlpha(0.9f)); g.setFont(juce::Font(13.0f, juce::Font::bold));
    g.drawText("YEAT BACKS  /  VOCAL FX", 32, 56, 260, 20, juce::Justification::left);
    g.setColour(juce::Colour(0xffb6ff36).withAlpha(0.5f)); g.drawLine(30.0f, 87.0f, 690.0f, 87.0f, 1.0f);
    g.setColour(juce::Colour(0xff82917b)); g.setFont(11.0f);
    g.drawText("RAGE VOCAL PROCESSOR  •  PITCH / FORMANT / SATURATION / SPACE", 32, 392, 650, 18, juce::Justification::left);
}
void MengicYeatBacksAudioProcessorEditor::resized()
{
    const int left=28, top=112, w=664, h=260, cellW=w/5, cellH=130;
    for (int i=0;i<9;++i)
    {
        const int row=i/5, col=i%5;
        knobs[i]->setBounds(left+col*cellW, top+row*cellH, cellW, cellH-4);
    }
}
