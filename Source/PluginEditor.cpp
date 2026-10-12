#include "PluginEditor.h"
HendrixGuitarEditor::HendrixGuitarEditor(HendrixGuitarAudioProcessor& processor):AudioProcessorEditor(processor) {
 look.setColour(juce::Slider::rotarySliderFillColourId,juce::Colour(0xffe2b873));
 look.setColour(juce::Slider::rotarySliderOutlineColourId,juce::Colour(0xff55465a));
 look.setColour(juce::Slider::thumbColourId,juce::Colour(0xffffead1));
 look.setColour(juce::Slider::textBoxTextColourId,juce::Colour(0xffffead1));
 look.setColour(juce::Slider::textBoxBackgroundColourId,juce::Colour(0xff251c29));
 look.setColour(juce::ComboBox::backgroundColourId,juce::Colour(0xff39283e));
 look.setColour(juce::ComboBox::textColourId,juce::Colour(0xffffead1));
 look.setColour(juce::Label::textColourId,juce::Colour(0xffffead1));
 look.setColour(juce::ToggleButton::textColourId,juce::Colour(0xffffead1));
 look.setColour(juce::ToggleButton::tickColourId,juce::Colour(0xffe2b873));
 setLookAndFeel(&look);
 const char* ids[]={"guitarVolume","master","fuzz","wahPosition","octave","vibeRate","vibeDepth","ampDrive","ampTone","cabTone","feedback","glideMs","bendRange","vibratoRate","vibratoDepth"};
 const char* names[]={"Guitar volume","Output","Fuzz","Wah position","Octave up","Vibe rate / Hz","Vibe depth","Amp drive","Amp tone","Cab tone","Harmonic sustain","Glide / ms","Bend / semitones","Vibrato rate / Hz","Vibrato / cents"};
 const char* tips[]={"Turn down to clean up the fuzz and amp.","Final output level.","Vintage asymmetric fuzz intensity.","Wah sweep; enable Wah first. Automate this knob in Logic.","Octave fuzz blend; single notes work best.","Speed of the swirling vibe.","Amount of the swirling vibe.","Amplifier gain and saturation.","Amplifier brightness.","Cabinet brightness.","Bounded harmonic sustain. Notes still release normally.","Glide time between overlapping notes in Mono legato.","Pitch bend range for Musical Typing keys 1 and 2.","Vibrato speed; Musical Typing keys 3-8 set modulation.","Maximum vibrato depth when modulation is full."};
 for(size_t i=0;i<knobs.size();++i) {
  auto& k=knobs[i];k.label.setText(names[i],juce::dontSendNotification);k.label.setJustificationType(juce::Justification::centred);
  k.slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);k.slider.setTextBoxStyle(juce::Slider::TextBoxBelow,false,90,22);
  k.slider.setTooltip(tips[i]);k.slider.setName(names[i]);k.slider.setWantsKeyboardFocus(true);
  addAndMakeVisible(k.label);addAndMakeVisible(k.slider);
  k.attachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.parameters,ids[i],k.slider);
  k.slider.textFromValueFunction=[i](double v){return juce::String(v,(i==11||i==12||i==14)?0:2);};k.slider.updateText();
 }
 presetLabel.setText("STARTING TONE",juce::dontSendNotification);addAndMakeVisible(presetLabel);
 presets.addItem("Clean Touch",1);presets.addItem("Driven Lead",2);presets.addItem("Wah Fuzz",3);
 presets.setTextWhenNothingSelected("Choose a preset");presets.setTooltip("Selecting a starting tone resets all controls to its settings.");
 presets.onChange=[this,&processor]{applyPreset(processor.parameters,presets.getSelectedId()-1);};
 addAndMakeVisible(presets);addAndMakeVisible(wah);addAndMakeVisible(legato);
 wah.setTooltip("Enable the resonant wah sweep.");legato.setTooltip("Overlapping notes glide with last-note priority. Disable for six-note chords.");
 wahAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.parameters,"wahEnabled",wah);
 legatoAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.parameters,"legato",legato);
 status.setText(processor.loadStatus.wasOk()?"GUITAR READY / 6 voices / bundled direct samples":"SAMPLE ERROR: "+processor.loadStatus.getErrorMessage(),juce::dontSendNotification);
 status.setColour(juce::Label::textColourId,processor.loadStatus.wasOk()?juce::Colour(0xffcbb9cf):juce::Colours::salmon);
 addAndMakeVisible(status);
 help.setText("PLAY IN LOGIC  /  Command-K opens Musical Typing\nBend: 1 / 2    Vibrato: 3-8    Sustain: Tab    Octave: Z / X\nSampled notes: MIDI 35-86. Use overlapping notes for legato; disable legato for chords.",juce::dontSendNotification);
 help.setColour(juce::Label::textColourId,juce::Colour(0xffcbb9cf));addAndMakeVisible(help);
 setWantsKeyboardFocus(false);setSize(880,630);
}
HendrixGuitarEditor::~HendrixGuitarEditor(){setLookAndFeel(nullptr);}
void HendrixGuitarEditor::paint(juce::Graphics& g) {
 g.fillAll(juce::Colour(0xff211924));
 g.setColour(juce::Colour(0xff39283e));g.fillRoundedRectangle(16,92,float(getWidth()-32),420,12);
 g.setColour(juce::Colour(0xffe2b873));g.setFont(juce::FontOptions(30.0f,juce::Font::bold));
 g.drawText("HENDRIX GUITAR",24,16,440,40,juce::Justification::centredLeft);
 g.setFont(juce::FontOptions(13.0f));g.setColour(juce::Colour(0xffcbb9cf));
 g.drawText("DIRECT STRINGS  /  VINTAGE TONE",25,57,430,20,juce::Justification::centredLeft);
}
void HendrixGuitarEditor::resized() {
 presetLabel.setBounds(540,16,300,20);presets.setBounds(540,40,312,32);
 const int width=(getWidth()-48)/5;
 for(size_t i=0;i<knobs.size();++i) {
  const int x=24+int(i%5)*width,y=100+int(i/5)*136;
  knobs[i].label.setBounds(x,y,width,24);knobs[i].slider.setBounds(x+8,y+24,width-16,108);
 }
 wah.setBounds(26,523,170,26);legato.setBounds(207,523,170,26);status.setBounds(392,523,getWidth()-418,26);
 help.setBounds(24,561,getWidth()-48, 62);
}
