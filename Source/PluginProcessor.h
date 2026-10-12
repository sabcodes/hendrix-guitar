#pragma once
#include "GuitarEngine.h"
#include "Parameters.h"
class HendrixGuitarAudioProcessor final : public juce::AudioProcessor {
public:
 HendrixGuitarAudioProcessor();
 const juce::String getName() const override { return "Hendrix Guitar"; }
 void prepareToPlay(double,int) override;
 void releaseResources() override;
 void reset() override;
 void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
 bool isBusesLayoutSupported(const BusesLayout&) const override;
 juce::AudioProcessorEditor* createEditor() override;
 bool hasEditor() const override { return true; }
 bool acceptsMidi() const override { return true; }
 bool producesMidi() const override { return false; }
 double getTailLengthSeconds() const override { return .25; }
 int getNumPrograms() override { return 1; }
 int getCurrentProgram() override { return 0; }
 void setCurrentProgram(int) override {}
 const juce::String getProgramName(int) override { return {}; }
 void changeProgramName(int,const juce::String&) override {}
 void getStateInformation(juce::MemoryBlock&) override;
 void setStateInformation(const void*,int) override;
 juce::AudioProcessorValueTreeState parameters;
 juce::Result loadStatus=juce::Result::ok();
private:
 GuitarEngine engine;
 VintageTone tone;
};
