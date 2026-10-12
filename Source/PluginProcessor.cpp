#include "PluginProcessor.h"
#include <dlfcn.h>
static juce::File guitarDirectory() {
 Dl_info info{};
 if (dladdr(reinterpret_cast<const void*>(&guitarDirectory), &info) == 0) return {};
 return juce::File(info.dli_fname).getParentDirectory().getParentDirectory().getChildFile("Resources/Guitar");
}
HendrixGuitarAudioProcessor::HendrixGuitarAudioProcessor()
: AudioProcessor(BusesProperties().withOutput("Output",juce::AudioChannelSet::stereo(),true)) { loadStatus=engine.load(guitarDirectory()); }
void HendrixGuitarAudioProcessor::prepareToPlay(double sr,int n) {engine.prepare(sr,n);tone.prepare(sr,n);setLatencySamples(tone.latencySamples());}
void HendrixGuitarAudioProcessor::releaseResources() {engine.reset();tone.reset();}
void HendrixGuitarAudioProcessor::processBlock(juce::AudioBuffer<float>& b,juce::MidiBuffer& m) {
 juce::ScopedNoDenormals no;engine.render(b,m,{});
 auto processRange=[&](int offset,int length){
  if(length<=0||b.getNumChannels()!=2)return;
  float* channels[]={b.getWritePointer(0,offset),b.getWritePointer(1,offset)};
  juce::AudioBuffer<float> view(channels,2,length);tone.process(view,{});
 };
 int cursor=0;
 for(const auto item:m)if(item.numBytes==3&&(item.data[0]&0xf0)==0xb0&&item.data[1]==120) {
  int offset=juce::jlimit(cursor,b.getNumSamples(),item.samplePosition);
  processRange(cursor,offset-cursor);tone.reset();cursor=offset;
 }
 processRange(cursor,b.getNumSamples()-cursor);
}
bool HendrixGuitarAudioProcessor::isBusesLayoutSupported(const BusesLayout& l) const {return l.getMainOutputChannelSet()==juce::AudioChannelSet::stereo() && l.getMainInputChannelSet().isDisabled();}
juce::AudioProcessorEditor* HendrixGuitarAudioProcessor::createEditor() {return new juce::GenericAudioProcessorEditor(*this);}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {return new HendrixGuitarAudioProcessor();}
