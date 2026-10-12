#include "GuitarEngine.h"
#include <algorithm>
juce::Result GuitarEngine::load(const juce::File& dir) {
 reset(); regions.clear(); alternatives.fill(0);
 auto document=juce::JSON::parse(dir.getChildFile("mapping.json").loadFileAsString());
 auto* array=document["regions"].getArray();
 if(array==nullptr || array->isEmpty() || array->size()>512) return juce::Result::fail("Guitar sample mapping is missing or invalid.");
 juce::AudioFormatManager formats; formats.registerBasicFormats();
 std::vector<Region> loaded; loaded.reserve(static_cast<size_t>(array->size()));
 juce::int64 totalFrames=0;
 for(const auto& item:*array) {
  auto path=item["sample"].toString();
  auto parts=juce::StringArray::fromTokens(path,"/",{});
  if(path.isEmpty() || juce::File::isAbsolutePath(path) || parts.contains("..") || path.containsChar('\\'))
   return juce::Result::fail("Unsafe sample path.");
  Region r;
  auto integer=[&](const char* name,int& target) {
   auto value=item[name];
   if(!value.isInt() && !value.isInt64()) return false;
   auto n=static_cast<juce::int64>(value);
   if(n<0 || n>127) return false;
   target=static_cast<int>(n); return true;
  };
  if(!integer("low",r.low)||!integer("high",r.high)||!integer("root",r.root)||
     !integer("lowVelocity",r.lowVelocity)||!integer("highVelocity",r.highVelocity)||
     r.low>r.high || r.lowVelocity<1 || r.lowVelocity>r.highVelocity)
   return juce::Result::fail("Invalid sample region.");
  r.release=static_cast<float>(item["release"]);
  if(!std::isfinite(r.release)||r.release<=0||r.release>10) return juce::Result::fail("Invalid sample release.");
  std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(dir.getChildFile(path)));
  if(!reader || reader->lengthInSamples<2 || reader->lengthInSamples>10000000 ||
     !std::isfinite(reader->sampleRate) || reader->sampleRate<8000 ||
     reader->numChannels<1 || reader->numChannels>2)
   return juce::Result::fail("Cannot decode guitar sample: "+path);
  totalFrames+=reader->lengthInSamples*reader->numChannels;
  if(totalFrames>180000000) return juce::Result::fail("Guitar bank is too large.");
  r.sampleRate=reader->sampleRate;
  r.audio.setSize(static_cast<int>(reader->numChannels),static_cast<int>(reader->lengthInSamples));
  if(!reader->read(&r.audio,0,r.audio.getNumSamples(),0,true,true))
   return juce::Result::fail("Cannot read guitar sample: "+path);
  for(int c=0;c<r.audio.getNumChannels();++c)
   for(int i=0;i<r.audio.getNumSamples();++i)
    if(!std::isfinite(r.audio.getSample(c,i))) return juce::Result::fail("Invalid guitar sample data.");
  loaded.push_back(std::move(r));
 }
 regions=std::move(loaded);
 return juce::Result::ok();
}
void GuitarEngine::prepare(double sr,int) {sampleRate=std::isfinite(sr)&&sr>=8000?sr:48000;reset();}
void GuitarEngine::reset() {for(auto& v:voices) v=Voice{};}
void GuitarEngine::handle(const juce::MidiMessage& m) {
 if(m.isNoteOn()) {
  const int note=m.getNoteNumber(), velocity=m.getVelocity();
  int distance=128, count=0;
  for(const auto& r:regions) if(velocity>=r.lowVelocity&&velocity<=r.highVelocity) {
   int d=note<r.low?r.low-note:note>r.high?note-r.high:0;
   if(d<distance){distance=d;count=1;} else if(d==distance) ++count;
  }
  if(count==0) return;
  auto choice=static_cast<int>(alternatives[static_cast<size_t>(note)]++%static_cast<unsigned>(count));
  const Region* region=nullptr;
  for(const auto& r:regions) if(velocity>=r.lowVelocity&&velocity<=r.highVelocity) {
   int d=note<r.low?r.low-note:note>r.high?note-r.high:0;
   if(d==distance && choice--==0){region=&r;break;}
  }
  auto* voice=&voices.front();
  for(auto& v:voices) {if(v.region==nullptr){voice=&v;break;} if(v.age<voice->age) voice=&v;}
  *voice=Voice{}; voice->region=region;voice->note=note;voice->channel=m.getChannel();voice->age=++age;
  voice->gain=m.getFloatVelocity()*.85f;
  voice->increment=region->sampleRate/sampleRate*std::pow(2.0,(note-region->root)/12.0);
 } else if(m.isNoteOff()) {
  for(auto& v:voices) if(v.region && v.note==m.getNoteNumber() && v.channel==m.getChannel())
   v.releaseStep=v.envelope/static_cast<float>(sampleRate*v.region->release);
 }
}
void GuitarEngine::renderFrames(juce::AudioBuffer<float>& b,int start,int end) {
 for(int i=start;i<end;++i) for(auto& v:voices) if(v.region) {
  const auto& r=*v.region;
  int position=static_cast<int>(v.position);
  if(position>=r.audio.getNumSamples()-1){v=Voice{};continue;}
  if(v.releaseStep>0) {v.envelope=std::max(0.0f,v.envelope-v.releaseStep);if(v.envelope<=0){v=Voice{};continue;}}
  else v.envelope=std::min(1.0f,v.envelope+static_cast<float>(1.0/(.001*sampleRate)));
  float fraction=static_cast<float>(v.position-position);
  auto endFade=std::min(1.0f,static_cast<float>((r.audio.getNumSamples()-v.position)/(.008*r.sampleRate)));
  for(int c=0;c<b.getNumChannels();++c) {
   const float* source=r.audio.getReadPointer(std::min(c,r.audio.getNumChannels()-1));
   float value=source[position]+fraction*(source[position+1]-source[position]);
   b.addSample(c,i,value*v.gain*v.envelope*endFade);
  }
  v.position+=v.increment;
 }
}
void GuitarEngine::render(juce::AudioBuffer<float>& b,const juce::MidiBuffer& midi,const PerformanceSettings&) {
 b.clear(); int cursor=0;
 for(const auto metadata:midi) {
  int offset=juce::jlimit(cursor,b.getNumSamples(),metadata.samplePosition);
  renderFrames(b,cursor,offset);handle(metadata.getMessage());cursor=offset;
 }
 renderFrames(b,cursor,b.getNumSamples());
}
