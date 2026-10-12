#pragma once
#include <juce_dsp/juce_dsp.h>
struct ToneSettings {
 float guitarVolume=.85f,master=.65f,fuzz=.35f;
 bool wahEnabled=false;
 float wahPosition=.5f,vibeRateHz=2.5f,vibeDepth=.15f;
 float ampDrive=.4f,ampTone=.55f,cabTone=.5f,octave=0;
};
class VintageTone {
public:
 void prepare(double,int);
 void process(juce::AudioBuffer<float>&,const ToneSettings&);
 void reset();
 int latencySamples() const;
private:
 using Filter=juce::dsp::StateVariableTPTFilter<float>;
 juce::dsp::Oversampling<float> oversampling{2,2,juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,true,true};
 std::array<juce::SmoothedValue<float>,11> controls;
 Filter wah,fuzzHighpass,octaveHighpass,ampLowpass,cabLowpass,cabHighpass;
 std::array<std::array<float,4>,2> vibeState{};
 double rate=192000,phase=0;
 int capacity=256;
 bool prepared=false;
};
