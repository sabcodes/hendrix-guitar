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
  r.sustainLevel=std::min(.5f,r.audio.getMagnitude(0,r.audio.getNumSamples())*.5f);
  loaded.push_back(std::move(r));
 }
 regions=std::move(loaded);
 return juce::Result::ok();
}
void GuitarEngine::prepare(double sr,int) {
 sampleRate=std::isfinite(sr)&&sr>=8000?sr:48000;
 smoothing=static_cast<float>(1-std::exp(-1/(.005*sampleRate)));reset();
}
void GuitarEngine::reset() {for(auto& v:voices)v=Voice{};for(auto& c:channels)c=Channel{};vibratoPhase=0;age=0;}
void GuitarEngine::release(Voice& v) {
 v.keyDown=false;
 if(v.envelope<=0){v.region=nullptr;return;}
 v.released=true;v.releaseStep=v.envelope/static_cast<float>(sampleRate*v.region->release);
}
void GuitarEngine::glide(Voice& v,int note,const PerformanceSettings& s) {
 v.press=channels[static_cast<size_t>(v.channel-1)].presses[static_cast<size_t>(note)];
 v.note=note;v.glideLeft=std::max(1,static_cast<int>(juce::jlimit(5.0f,250.0f,s.glideMs)*sampleRate/1000));
 v.pitchStep=(note-v.pitch)/v.glideLeft;v.keyDown=true;v.released=false;
}
void GuitarEngine::handle(const juce::MidiMessage& m,const PerformanceSettings& s) {
 if(!m.isNoteOnOrOff()&&!m.isController()&&!m.isPitchWheel())return;
 const int ch=m.getChannel();auto& state=channels[static_cast<size_t>(ch-1)];
 if(m.isNoteOn()) {
  int note=m.getNoteNumber(),velocity=m.getVelocity();auto index=static_cast<size_t>(note);
  auto press=++state.presses[index];state.order[index]=++age;
  if(s.legato) {
   Voice* active=nullptr;
   for(auto& v:voices)if(v.region&&v.channel==ch&&!v.released&&(!active||v.age>active->age))active=&v;
   if(active) {
    for(auto& v:voices)if(&v!=active&&v.region&&v.channel==ch)release(v);
    glide(*active,note,s);active->age=age;return;
   }
  }
  int distance=128,count=0;
  for(const auto& r:regions)if(velocity>=r.lowVelocity&&velocity<=r.highVelocity){
   int d=note<r.low?r.low-note:note>r.high?note-r.high:0;
   if(d<distance){distance=d;count=1;}else if(d==distance)++count;
  }
  if(!count)return;
  int choice=static_cast<int>(alternatives[index]++%static_cast<unsigned>(count));
  const Region* region=nullptr;
  for(const auto& r:regions)if(velocity>=r.lowVelocity&&velocity<=r.highVelocity){
   int d=note<r.low?r.low-note:note>r.high?note-r.high:0;
   if(d==distance&&choice--==0){region=&r;break;}
  }
  Voice* voice=&voices.front();
  for(auto& v:voices){if(!v.region){voice=&v;break;}if(v.age<voice->age)voice=&v;}
  Tail tail=voice->tail;
  if(voice->region) {
   tail.channel=voice->channel;tail.region=voice->region;tail.position=voice->position;tail.increment=voice->increment;
   tail.phase=voice->phase;tail.frequency=voice->frequency;tail.gain=voice->gain*voice->envelope;
   tail.feedback=voice->feedback;tail.left=tail.total=std::max(1,static_cast<int>(sampleRate*.008));
  }
  *voice=Voice{};voice->tail=tail;voice->region=region;voice->note=note;voice->pitch=note;
  voice->channel=ch;voice->age=age;voice->press=press;voice->gain=m.getFloatVelocity()*.85f;
  voice->increment=region->sampleRate/sampleRate*std::pow(2.0,(note-region->root)/12.0);
 } else if(m.isNoteOff()) {
  int note=m.getNoteNumber();auto index=static_cast<size_t>(note);
  if(state.releases[index]>=state.presses[index])return;
  auto press=++state.releases[index];
  if(state.releases[index]==state.presses[index])state.order[index]=0;
  for(auto& v:voices)if(v.region&&v.channel==ch) {
   if(s.legato&&v.note==note) {
    int last=-1;juce::uint64 order=0;
    for(int key=0;key<128;++key)if(state.order[static_cast<size_t>(key)]>order){last=key;order=state.order[static_cast<size_t>(key)];}
    if(last>=0)glide(v,last,s);else {v.keyDown=false;if(!state.sustain)release(v);}
   } else if(!s.legato&&v.note==note&&v.press==press) {
    v.keyDown=false;if(!state.sustain)release(v);
   }
  }
 } else if(m.isPitchWheel()) {
  int value=m.getPitchWheelValue()-8192;
  state.bendTarget=static_cast<float>(value)/static_cast<float>(value>=0?8191:8192);
 } else if(m.isController()) {
  int controller=m.getControllerNumber(),value=m.getControllerValue();
  if(controller==1)state.modTarget=value/127.0f;
  else if(controller==64) {
   state.sustain=value>=64;
   if(!state.sustain)for(auto& v:voices)if(v.region&&v.channel==ch&&!v.keyDown&&!v.released)release(v);
  } else if(controller==120) {
   for(auto& v:voices) {
    if(v.tail.channel==ch)v.tail=Tail{};
    if(v.channel==ch){auto tail=v.tail;v=Voice{};v.tail=tail;}
   }
   state=Channel{};
  } else if(controller==123) {
   state.releases=state.presses;state.order.fill(0);state.sustain=false;
   for(auto& v:voices)if(v.region&&v.channel==ch)release(v);
  }
 }
}
static float interpolated(const juce::AudioBuffer<float>& b,int channel,double position) {
 auto i=static_cast<int>(position);
 if(i<0||i>=b.getNumSamples()-1)return 0;
 auto* p=b.getReadPointer(std::min(channel,b.getNumChannels()-1));
 float fraction=static_cast<float>(position-i);return p[i]+fraction*(p[i+1]-p[i]);
}
static float harmonics(double phase){return static_cast<float>((std::sin(phase)+.25*std::sin(2*phase)+.15*std::sin(3*phase))/1.4);}
void GuitarEngine::renderFrames(juce::AudioBuffer<float>& b,int start,int end,const PerformanceSettings& s) {
 auto bendRange=juce::jlimit(1.0f,12.0f,s.bendRangeSemitones);
 auto rate=juce::jlimit(.5f,10.0f,s.vibratoRateHz),depth=juce::jlimit(0.0f,100.0f,s.vibratoDepthCents);
 auto feedback=juce::jlimit(0.0f,1.0f,s.feedback);
 for(int i=start;i<end;++i) {
  for(auto& c:channels){c.bend+=(c.bendTarget-c.bend)*smoothing;c.mod+=(c.modTarget-c.mod)*smoothing;}
  auto vibrato=std::sin(vibratoPhase);vibratoPhase=std::fmod(vibratoPhase+juce::MathConstants<double>::twoPi*rate/sampleRate,juce::MathConstants<double>::twoPi);
  for(auto& v:voices) {
   auto& tail=v.tail;
   if(tail.region&&tail.left>0) {
    float fade=static_cast<float>(tail.left)/tail.total;
    for(int c=0;c<b.getNumChannels();++c) {
     auto body=interpolated(tail.region->audio,c,tail.position);
     auto value=body*(1-.25f*tail.feedback)+harmonics(tail.phase)*tail.region->sustainLevel*.25f*tail.feedback;
     b.addSample(c,i,value*tail.gain*fade);
    }
    tail.position+=tail.increment;tail.phase=std::fmod(tail.phase+juce::MathConstants<double>::twoPi*tail.frequency/sampleRate,juce::MathConstants<double>::twoPi);
    if(--tail.left==0)tail.region=nullptr;
   }
   if(!v.region)continue;
   const auto& r=*v.region;
   if(v.released){v.envelope=std::max(0.0f,v.envelope-v.releaseStep);if(v.envelope<=0){v.region=nullptr;continue;}}
   else v.envelope=std::min(1.0f,v.envelope+static_cast<float>(1/(.001*sampleRate)));
   if(v.glideLeft>0){v.pitch+=v.pitchStep;if(--v.glideLeft==0)v.pitch=v.note;}
   const auto& state=channels[static_cast<size_t>(v.channel-1)];
   auto pitch=v.pitch+state.bend*bendRange+vibrato*state.mod*depth/100;
   v.frequency=440*std::pow(2.0,(pitch-69)/12);
   v.increment=r.sampleRate/sampleRate*std::pow(2.0,(pitch-r.root)/12);
   v.feedback+=(feedback-v.feedback)*smoothing;
   if(v.position>=r.audio.getNumSamples()-1&&feedback==0&&v.feedback<1e-5f){v.region=nullptr;continue;}
   float fade=juce::jlimit(0.0f,1.0f,static_cast<float>((r.audio.getNumSamples()-v.position)/(.008*r.sampleRate)));
   float mix=.25f*v.feedback*juce::jlimit(0.0f,1.0f,static_cast<float>((v.frames/sampleRate-.04)/.1));
   for(int c=0;c<b.getNumChannels();++c) {
    float body=interpolated(r.audio,c,v.position)*fade;
    float value=body*(1-mix)+harmonics(v.phase)*r.sustainLevel*mix;
    b.addSample(c,i,value*v.gain*v.envelope);
   }
   v.position+=v.increment;++v.frames;
   v.phase=std::fmod(v.phase+juce::MathConstants<double>::twoPi*v.frequency/sampleRate,juce::MathConstants<double>::twoPi);
  }
 }
}
void GuitarEngine::render(juce::AudioBuffer<float>& b,const juce::MidiBuffer& midi,const PerformanceSettings& s) {
 b.clear();int cursor=0;
 for(const auto metadata:midi) {
  int offset=juce::jlimit(cursor,b.getNumSamples(),metadata.samplePosition);
  renderFrames(b,cursor,offset,s);
  if(metadata.numBytes<=3)handle(metadata.getMessage(),s);
  cursor=offset;
 }
 renderFrames(b,cursor,b.getNumSamples(),s);
}
