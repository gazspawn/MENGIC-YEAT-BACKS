#include "PluginEditor.h"
namespace {
const juce::Colour lime(0xffaaff24), text(0xffe3e6e2), muted(0xff929b91);
}
MengicYeatBacksAudioProcessorEditor::AlienLook::AlienLook()
{
    setColour(juce::Slider::textBoxTextColourId,text);
    setColour(juce::Slider::textBoxBackgroundColourId,juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);
    setColour(juce::ComboBox::backgroundColourId,juce::Colour(0xff111510));
    setColour(juce::ComboBox::textColourId,text);
    setColour(juce::ComboBox::outlineColourId,juce::Colour(0xff3a4338));
    setColour(juce::PopupMenu::backgroundColourId,juce::Colour(0xff151b13));
    setColour(juce::PopupMenu::highlightedBackgroundColourId,juce::Colour(0xff344525));
}
void MengicYeatBacksAudioProcessorEditor::AlienLook::drawRotarySlider(
 juce::Graphics& g,int x,int y,int w,int h,float pos,float start,float end,juce::Slider&)
{
    auto bounds=juce::Rectangle<float>((float)x,(float)y,(float)w,(float)h).reduced(9);
    float radius=juce::jmin(bounds.getWidth(),bounds.getHeight())*.5f;
    auto centre=bounds.getCentre();
    const float angle=start+pos*(end-start);
    juce::Path back,front;
    back.addCentredArc(centre.x,centre.y,radius,radius,0,start,end,true);
    front.addCentredArc(centre.x,centre.y,radius,radius,0,start,angle,true);
    g.setColour(juce::Colour(0xff30392c));g.strokePath(back,juce::PathStrokeType(4));
    g.setColour(lime);g.strokePath(front,juce::PathStrokeType(4));
    auto disk=juce::Rectangle<float>(radius*1.66f,radius*1.66f).withCentre(centre);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff343a32),disk.getTopLeft(),
        juce::Colour(0xff10140f),disk.getBottomRight(),false));
    g.fillEllipse(disk);
    g.setColour(juce::Colour(0xff50594a));g.drawEllipse(disk,1);
    const juce::Point<float> a(centre.x+std::sin(angle)*radius*.52f,centre.y-std::cos(angle)*radius*.52f);
    const juce::Point<float> b(centre.x+std::sin(angle)*radius*.74f,centre.y-std::cos(angle)*radius*.74f);
    g.setColour(text);g.drawLine({a,b},2.5f);
}
void MengicYeatBacksAudioProcessorEditor::AlienLook::drawToggleButton(
 juce::Graphics& g,juce::ToggleButton& b,bool,bool)
{
    const bool on=b.getToggleState();
    const bool bypass=b.getButtonText()=="BYPASS";
    if(bypass) {
        g.setColour(on?lime:muted);g.drawEllipse(3,9,20,20,1.5f);
        if(on)g.fillEllipse(8,14,10,10);
        g.setFont(13.0f);g.drawText("BYPASS",30,0,b.getWidth()-30,b.getHeight(),juce::Justification::centredLeft);
        return;
    }
    auto r=b.getLocalBounds().toFloat().reduced(2,9);
    g.setColour(juce::Colour(0xff0c110b));g.fillRoundedRectangle(r,r.getHeight()*.5f);
    g.setColour(on?juce::Colour(0xff598522):juce::Colour(0xff3b4338));
    g.drawRoundedRectangle(r,r.getHeight()*.5f,1);
    float d=r.getHeight()-6;
    g.setColour(on?lime:muted);
    g.fillEllipse(on?r.getRight()-d-3:r.getX()+3,r.getY()+3,d,d);
}
MengicYeatBacksAudioProcessorEditor::Knob::Knob(juce::String title,bool time):name(title)
{
    addAndMakeVisible(slider);
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setRotaryParameters(juce::MathConstants<float>::pi*1.25f,
        juce::MathConstants<float>::pi*2.75f,true);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow,false,92,24);
    slider.setTextValueSuffix(time?" ms":" %");
    slider.setNumDecimalPlacesToDisplay(0);
    slider.setDoubleClickReturnValue(true,time?280.0:0.0);
}
void MengicYeatBacksAudioProcessorEditor::Knob::paint(juce::Graphics& g)
{
    g.setColour(text);g.setFont(13.0f);
    g.drawText(name,getLocalBounds().removeFromTop(24),juce::Justification::centred);
}
void MengicYeatBacksAudioProcessorEditor::Knob::resized()
{slider.setBounds(getLocalBounds().withTrimmedTop(26));}
MengicYeatBacksAudioProcessorEditor::MengicYeatBacksAudioProcessorEditor(MengicYeatBacksAudioProcessor& p)
 :AudioProcessorEditor(&p),processor(p),tips(this,650)
{
    setLookAndFeel(&look);
    addAndMakeVisible(canvas);
    const char* ids[]={"convDry","convWet","revDry","revWet","revMix","revSize","flangusRate","delayMs","delayWet","flangusDepth","flangusMix","transpose"};
    const char* names[]={"DRY","WET","DRY","WET","MIX","SIZE","RATE","TIME","WET","DEPTH","MIX","SEMITONES"};
    for(size_t i=0;i<knobs.size();++i) {
        knobs[i]=std::make_unique<Knob>(names[i],i==7);
        canvas.addAndMakeVisible(*knobs[i]);
        sliders[i]=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            p.parameters,ids[i],knobs[i]->slider);
    }
    knobs[7]->slider.setTooltip("Delay time in milliseconds. Feedback is fixed at 32%.");
    knobs[6]->slider.setTextValueSuffix(" Hz");
    knobs[6]->slider.setDoubleClickReturnValue(true,.4);
    knobs[6]->slider.setTooltip("Modulation speed in Hz.");
    knobs[11]->slider.setTextValueSuffix(" st");
    knobs[11]->slider.setDoubleClickReturnValue(true,0);
    knobs[11]->slider.setTooltip("Pitch shift -12 to +12 semitones. Changes timbre too; no automatic tuning or formant preservation.");
    knobs[9]->slider.setTooltip("Chorus / flanger modulation depth.");
    knobs[10]->slider.setTooltip("Blend of dry and modulated signal.");
    const char* toggles[]={"convOn","revOn","flangusOn","delayOn","transposeOn","bypass"};
    for(size_t i=0;i<switches.size();++i) {
        canvas.addAndMakeVisible(switches[i]);
        buttons[i]=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.parameters,toggles[i],switches[i]);
        switches[i].setTooltip(i==5?"Bypass all processing":"Enable / disable this module");
    }
    switches[5].setButtonText("BYPASS");
    presets.addItemList({"Vocal - Green Chamber","Adlibs - Deep Orbit","Vocal - Clear Mist","Init - Clean"},1);
    presets.setTextWhenNothingSelected("PRESETS / edited");
    presets.onChange=[this]{const int n=presets.getSelectedId();if(n>0) {
        processor.applyPreset(n-1);
        // No stale preset name after parameter automation / edits.
        presets.setSelectedId(0,juce::dontSendNotification);
    }};
    convType.addItemList({"Vocal Chamber","Glass Cell","Alien Vessel"},1);
    revType.addItemList({"Acid Hall","Dark Orbit","Soft Nebula"},1);
    flangusMode.addItemList({"Chorus","Flanger"},1);
    for(auto* combo:{&presets,&convType,&revType,&flangusMode})canvas.addAndMakeVisible(*combo);
    convChoice=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.parameters,"convType",convType);
    revChoice=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.parameters,"revType",revType);
    flangusChoice=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.parameters,"flangusMode",flangusMode);
    setResizable(true,true);
    setResizeLimits(825,540,1650,1080);
    if(auto* c=getConstrainer())c->setFixedAspectRatio(1100.0/720.0);
    setSize(1100,720);
    startTimerHz(20);
}
MengicYeatBacksAudioProcessorEditor::~MengicYeatBacksAudioProcessorEditor()
{stopTimer();setLookAndFeel(nullptr);}
void MengicYeatBacksAudioProcessorEditor::resized()
{
    canvas.setBounds(0,0,1100,720);
    canvas.setTransform(juce::AffineTransform::scale((float)getWidth()/1100.0f));
    presets.setBounds(535,27,320,37);switches[5].setBounds(895,27,130,38);
    convType.setBounds(80,145,360,35);revType.setBounds(520,145,500,35);
    flangusMode.setBounds(220,432,145,27);
    const juce::Rectangle<int> knobBounds[]={
        {100,200,145,170},{275,200,145,170},
        {500,210,130,160},{635,210,130,160},{770,210,130,160},{905,210,130,160},
        {70,485,125,170},{505,485,125,170},{650,485,125,170},{200,485,125,170},
        {330,485,125,170},{845,480,170,175}};
    for(size_t i=0;i<knobs.size();++i)knobs[i]->setBounds(knobBounds[i]);
    const juce::Rectangle<int> toggleBounds[]={{395,97,55,37},{985,97,55,37},{395,425,55,37},{720,425,55,37},{985,425,55,37}};
    for(size_t i=0;i<5;++i)switches[i].setBounds(toggleBounds[i]);
}
void MengicYeatBacksAudioProcessorEditor::drawTube(juce::Graphics& g,juce::Rectangle<float> r)
{
    g.setColour(juce::Colour(0xff070c06));g.fillRoundedRectangle(r,11);
    g.setColour(juce::Colour(0xff485541));g.drawRoundedRectangle(r,11,1);
    auto inside=r.reduced(5,7);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff213710),inside.getTopLeft(),
        juce::Colour(0xff7ccf11),inside.getBottomRight(),false));
    g.fillRoundedRectangle(inside,7);
    for(int i=0;i<6;++i) {
        float t=std::fmod(phase*.1f+(float)i*.167f,1.0f);
        float cy=inside.getBottom()-t*inside.getHeight();
        float cx=inside.getCentreX()+std::sin(phase+i*2.f)*2;
        float d=i%2==0?4.f:2.f;
        g.setColour(lime.withAlpha(.7f));g.drawEllipse(cx-d*.5f,cy-d*.5f,d,d,.7f);
    }
    g.setColour(juce::Colours::white.withAlpha(.16f));
    g.fillRoundedRectangle(r.getX()+4,r.getY()+9,2,r.getHeight()-18,1);
}
void MengicYeatBacksAudioProcessorEditor::paint(juce::Graphics& graphics)
{
    juce::Graphics::ScopedSaveState save(graphics);
    graphics.addTransform(juce::AffineTransform::scale((float)getWidth()/1100.f));
    auto& g=graphics;
    g.fillAll(juce::Colour(0xff0a0d09));
    g.setColour(juce::Colour(0xff151a13));g.fillRoundedRectangle(9,9,1082,702,20);
    g.setColour(juce::Colour(0xff3b4535));g.drawRoundedRectangle(9,9,1082,702,20,1);
    g.setColour(text);g.setFont(juce::Font(29.f,juce::Font::bold));
    g.drawText("MENGIC",60,27,165,40,juce::Justification::centredLeft);
    g.setColour(lime);g.setFont(juce::Font(24.f,juce::Font::bold));
    g.drawText("ALIEN",250,27,105,40,juce::Justification::centredLeft);
    g.setColour(muted);g.setFont(16.f);g.drawText("/ VOCAL LAB",355,27,150,40,juce::Justification::centredLeft);
    const juce::Rectangle<float> panels[]={{55,92,410,305},{485,92,565,305},{55,418,410,263},{485,418,305,263},{810,418,240,263}};
    const char* titles[]={"CONVOLVER","REVERB","FLANGUS","DELAY","TRANSPOSE"};
    for(size_t i=0;i<5;++i) {
        auto r=panels[i];
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff1c2119),r.getTopLeft(),juce::Colour(0xff10150e),r.getBottomRight(),false));
        g.fillRoundedRectangle(r,12);
        g.setColour(juce::Colour(0xff3c4536));g.drawRoundedRectangle(r,12,1);
        g.setColour(text);g.setFont(juce::Font(18.f,juce::Font::bold));
        g.drawText(titles[i],(int)r.getX()+22,(int)r.getY()+12,(int)r.getWidth()-88,30,juce::Justification::centredLeft);
        g.setColour(juce::Colour(0xff323d2c));
        g.drawLine(r.getX()+22,r.getY()+55,r.getRight()-22,r.getY()+55,.7f);
    }
    drawTube(g,{21,115,19,264});drawTube(g,{1060,478,18,160});
    g.setColour(muted);g.setFont(10.f);
    g.drawText("MENGIC ALIEN  /  2.1",60,687,500,18,juce::Justification::centredLeft);
}
