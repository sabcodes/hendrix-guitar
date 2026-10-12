#include "VintageTone.h"
#include <algorithm>
static float safe(float x,float lo,float hi,float fallback){return std::isfinite(x)?juce::jlimit(lo,hi,x):fallback;}
void VintageTone::prepare(double sr,int maxBlock) {
 if(!std::isfinite(sr)||sr<8000)sr=48000;
 capacity=std::max(1,maxBlock);rate=sr*4;
 oversampling.initProcessing(static_cast<size_t>(capacity));
 juce::dsp::ProcessSpec spec{rate,static_cast<juce::uint32>(capacity*4),2};
 for(auto* f:{&wah,&fuzzHighpass,&octaveHighpass,&ampLowpass,&cabLowpass,&cabHighpass}){f->prepare(spec);f->setResonance(.7071f);}
 wah.setType(Filter::Type::bandpass);wah.setResonance(2.2f);
 for(auto* f:{&fuzzHighpass,&octaveHighpass,&cabHighpass})f->setType(Filter::Type::highpass);
 fuzzHighpass.setCutoffFrequency(35);octaveHighpass.setCutoffFrequency(35);cabHighpass.setCutoffFrequency(80);
 ampLowpass.setType(Filter::Type::lowpass);cabLowpass.setType(Filter::Type::lowpass);
 const float defaults[]={.85f,.65f,.35f,0,.5f,2.5f,.15f,.4f,.55f,.5f,0};
 for(size_t i=0;i<controls.size();++i){controls[i].reset(rate,.02);controls[i].setCurrentAndTargetValue(defaults[i]);}
 prepared=true;reset();
}
void VintageTone::reset() {
 oversampling.reset();for(auto* f:{&wah,&fuzzHighpass,&octaveHighpass,&ampLowpass,&cabLowpass,&cabHighpass})f->reset();
 for(auto& channel:vibeState)channel.fill(0);phase=0;
 for(auto& control:controls)control.setCurrentAndTargetValue(control.getTargetValue());
}
int VintageTone::latencySamples() const{return prepared?juce::roundToInt(oversampling.getLatencyInSamples()):0;}
void VintageTone::process(juce::AudioBuffer<float>& b,const ToneSettings& s) {
 if(!prepared||b.getNumChannels()!=2){b.clear();return;}
 const float targets[]={safe(s.guitarVolume,0,1,.85f),safe(s.master,0,1,.65f),safe(s.fuzz,0,1,.35f),
  s.wahEnabled?1.0f:0.0f,safe(s.wahPosition,0,1,.5f),safe(s.vibeRateHz,.1f,10,2.5f),safe(s.vibeDepth,0,1,.15f),
  safe(s.ampDrive,0,1,.4f),safe(s.ampTone,0,1,.55f),safe(s.cabTone,0,1,.5f),safe(s.octave,0,1,0)};
 for(size_t i=0;i<controls.size();++i)controls[i].setTargetValue(targets[i]);
 for(int offset=0;offset<b.getNumSamples();offset+=capacity) {
  int length=std::min(capacity,b.getNumSamples()-offset);
  auto native=juce::dsp::AudioBlock<float>(b).getSubBlock(static_cast<size_t>(offset),static_cast<size_t>(length));
  for(size_t c=0;c<2;++c)for(int i=0;i<length;++i) {
   auto& value=native.getChannelPointer(c)[i];value=std::isfinite(value)?juce::jlimit(-8.0f,8.0f,value):0;
  }
  auto up=oversampling.processSamplesUp(native);
  for(size_t i=0;i<up.getNumSamples();++i) {
   std::array<float,11> p{};for(size_t k=0;k<p.size();++k)p[k]=controls[k].getNextValue();
   wah.setCutoffFrequency(300*std::pow(9.0f,p[4]));
   ampLowpass.setCutoffFrequency(std::min(static_cast<float>(rate*.4),1200+7600*p[8]));
   cabLowpass.setCutoffFrequency(std::min(static_cast<float>(rate*.4),2500+4500*p[9]));
   const float centres[]={150,450,1200,2800};std::array<float,4> a{};
   for(size_t k=0;k<4;++k) {
    double hz=centres[k]*(1+.65*p[6]*std::sin(phase+.6*k));
    auto tangent=std::tan(juce::MathConstants<double>::pi*hz/rate);
    a[k]=static_cast<float>((1-tangent)/(1+tangent));
   }
   phase=std::fmod(phase+juce::MathConstants<double>::twoPi*p[5]/rate,juce::MathConstants<double>::twoPi);
   for(size_t c=0;c<2;++c) {
    float x=up.getChannelPointer(c)[i]*p[0];
    auto filtered=wah.processSample(static_cast<int>(c),x)*2.2f;x+=p[3]*(filtered-x);
    x=fuzzHighpass.processSample(static_cast<int>(c),x);
    float bias=.08f*p[2];float fuzz=std::tanh(x*(1+40*p[2])+bias)-std::tanh(bias);x+=p[2]*(fuzz-x);
    x=x*(1-p[10])+std::abs(x)*p[10];
    x=octaveHighpass.processSample(static_cast<int>(c),x);
    float wet=x;
    for(size_t k=0;k<4;++k){float y=-a[k]*wet+vibeState[c][k];vibeState[c][k]=wet+a[k]*y;wet=y;}
    x+=.5f*p[6]*(wet-x);
    x=.7f*std::tanh(x*(2+18*p[7]));
    x=ampLowpass.processSample(static_cast<int>(c),x);
    x=cabHighpass.processSample(static_cast<int>(c),x);
    x=cabLowpass.processSample(static_cast<int>(c),x);
    up.getChannelPointer(c)[i]=.9f*std::tanh(x*p[1]);
   }
  }
  oversampling.processSamplesDown(native);
  for(size_t c=0;c<2;++c)for(int i=0;i<length;++i) {
   auto& value=native.getChannelPointer(c)[i];value=std::isfinite(value)?juce::jlimit(-.98f,.98f,value):0;
  }
 }
}
