#pragma once
#include "PluginProcessor.h"
class HendrixGuitarEditor final : public juce::AudioProcessorEditor {
public:
 explicit HendrixGuitarEditor(HendrixGuitarAudioProcessor&);
 ~HendrixGuitarEditor() override;
 void paint(juce::Graphics&) override;
 void resized() override;
private:
 struct Knob {
  juce::Label label;
  juce::Slider slider;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
 };
 juce::LookAndFeel_V4 look;
 juce::ComboBox presets;
 juce::Label presetLabel,status,help;
 juce::ToggleButton wah{"Wah enabled"},legato{"Mono legato"};
 std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> wahAttachment,legatoAttachment;
 std::array<Knob,15> knobs;
 juce::TooltipWindow tooltips{this,600};
};
