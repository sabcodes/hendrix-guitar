#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <dlfcn.h>
#include <cstdlib>
static juce::File guitarDirectory() {
 Dl_info info{};
 if (dladdr(reinterpret_cast<const void*>(&guitarDirectory), &info) == 0) return {};
 return juce::File(info.dli_fname).getParentDirectory().getParentDirectory().getChildFile("Resources/Guitar");
}
HendrixGuitarAudioProcessor::HendrixGuitarAudioProcessor()
: AudioProcessor(BusesProperties().withOutput("Output",juce::AudioChannelSet::stereo(),true)), parameters(*this,nullptr,"HendrixGuitarState",createParameterLayout()) { loadStatus=engine.load(guitarDirectory()); }
void HendrixGuitarAudioProcessor::prepareToPlay(double sr,int n) {engine.prepare(sr,n);tone.prepare(sr,n);setLatencySamples(tone.latencySamples());}
void HendrixGuitarAudioProcessor::reset() {engine.reset();tone.reset();}
void HendrixGuitarAudioProcessor::releaseResources() {reset();}
void HendrixGuitarAudioProcessor::processBlock(juce::AudioBuffer<float>& b,juce::MidiBuffer& m) {
 juce::ScopedNoDenormals no;engine.render(b,m,readPerformanceSettings(parameters));
 const auto settings=readToneSettings(parameters);
 auto processRange=[&](int offset,int length){
  if(length<=0||b.getNumChannels()!=2)return;
  float* channels[]={b.getWritePointer(0,offset),b.getWritePointer(1,offset)};
  juce::AudioBuffer<float> view(channels,2,length);tone.process(view,settings);
 };
 int cursor=0;
 for(const auto item:m)if(item.numBytes==3&&(item.data[0]&0xf0)==0xb0&&item.data[1]==120) {
  int offset=juce::jlimit(cursor,b.getNumSamples(),item.samplePosition);
  processRange(cursor,offset-cursor);tone.reset();cursor=offset;
 }
 processRange(cursor,b.getNumSamples()-cursor);
}
bool HendrixGuitarAudioProcessor::isBusesLayoutSupported(const BusesLayout& l) const {return l.getMainOutputChannelSet()==juce::AudioChannelSet::stereo() && l.getMainInputChannelSet().isDisabled();}
juce::AudioProcessorEditor* HendrixGuitarAudioProcessor::createEditor() {return new HendrixGuitarEditor(*this);}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {return new HendrixGuitarAudioProcessor();}

void HendrixGuitarAudioProcessor::getStateInformation(juce::MemoryBlock& output) {
 auto tree=parameters.copyState();tree.setProperty("schemaVersion",1,nullptr);auto xml=tree.createXml();copyXmlToBinary(*xml,output);
}
void HendrixGuitarAudioProcessor::setStateInformation(const void* data,int size) {
 if(data==nullptr||size<=0||size>4*1024*1024)return;
 auto xml=getXmlFromBinary(data,size);
 if(!xml||!xml->hasTagName("HendrixGuitarState")||xml->getStringAttribute("schemaVersion")!="1")return;
 juce::ValueTree merged("HendrixGuitarState");merged.setProperty("schemaVersion",1,nullptr);
 for(auto* base:getParameters()) {
  auto* p=dynamic_cast<juce::RangedAudioParameter*>(base);if(!p)continue;
  float value=p->convertFrom0to1(p->getDefaultValue());
  for(auto* child:xml->getChildIterator())if(child->hasTagName("PARAM")&&child->getStringAttribute("id")==p->getParameterID()) {
   auto text=child->getStringAttribute("value").trim();auto utf8=text.toRawUTF8();char* end=nullptr;
   double parsed=std::strtod(utf8,&end);
   if(end!=utf8&&*end==0&&std::isfinite(parsed)) {
    auto range=p->getNormalisableRange();value=range.snapToLegalValue(juce::jlimit(range.start,range.end,float(parsed)));
   }
  }
  juce::ValueTree entry("PARAM");entry.setProperty("id",p->getParameterID(),nullptr);entry.setProperty("value",value,nullptr);merged.addChild(entry,-1,nullptr);
 }
 parameters.replaceState(merged);
}
