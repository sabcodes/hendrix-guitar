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
  float release=.2f,sustainLevel=.1f;
 };
 struct Tail {
  const Region* region=nullptr;
  double position=0,increment=1,phase=0,frequency=220;
  float gain=0,feedback=0;
  int left=0,total=1,channel=1;
 };
 struct Voice {
  const Region* region=nullptr;
  double position=0,increment=1,pitch=60,pitchStep=0,phase=0,frequency=220;
  float gain=0,envelope=0,releaseStep=0,feedback=0;
  bool keyDown=true,released=false;
  int note=0,channel=1,glideLeft=0;
  juce::uint64 age=0,press=0,frames=0;
  Tail tail;
 };
 struct Channel {
  std::array<juce::uint64,128> presses{},releases{},order{};
  bool sustain=false;
  float bend=0,bendTarget=0,mod=0,modTarget=0;
 };
 std::vector<Region> regions;
 std::array<Voice,6> voices{};
 std::array<Channel,16> channels{};
 std::array<unsigned,128> alternatives{};
 double sampleRate=48000,vibratoPhase=0;
 float smoothing=.01f;
 juce::uint64 age=0;
 void handle(const juce::MidiMessage&,const PerformanceSettings&);
 void renderFrames(juce::AudioBuffer<float>&,int,int,const PerformanceSettings&);
 void release(Voice&);
 void glide(Voice&,int,const PerformanceSettings&);
};
