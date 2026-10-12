#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <array>
#include <vector>
struct PerformanceSettings {
 bool legato=false;
 float bendRangeSemitones=2, glideMs=70, vibratoRateHz=5.5f, vibratoDepthCents=35, feedback=0;
};
class GuitarEngine {
public:
 juce::Result load(const juce::File&);
 void prepare(double,int);
 void render(juce::AudioBuffer<float>&,const juce::MidiBuffer&,const PerformanceSettings&);
 void reset();
private:
 struct Region {
  juce::AudioBuffer<float> audio;
  double sampleRate=48000;
  int low=0,high=127,root=60,lowVelocity=1,highVelocity=127;
  float release=.2f;
 };
 struct Voice {
  const Region* region=nullptr;
  double position=0,increment=1;
  float gain=0,envelope=0,releaseStep=0;
  int note=0,channel=1;
  juce::uint64 age=0;
 };
 std::vector<Region> regions;
 std::array<Voice,6> voices{};
 std::array<unsigned,128> alternatives{};
 double sampleRate=48000;
 juce::uint64 age=0;
 void handle(const juce::MidiMessage&);
 void renderFrames(juce::AudioBuffer<float>&,int,int);
};
