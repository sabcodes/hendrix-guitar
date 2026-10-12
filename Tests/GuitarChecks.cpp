#include "GuitarEngine.h"
#include <CommonCrypto/CommonDigest.h>
namespace checks {
juce::File writeSineBank(const juce::File& dir,double sr=48000,double hz=220) {
 dir.createDirectory();
 juce::AudioBuffer<float> b(1,static_cast<int>(sr*3));
 for(int i=0;i<b.getNumSamples();++i) b.setSample(0,i,.2f*std::sin(float(juce::MathConstants<double>::twoPi*hz*i/sr)));
 juce::WavAudioFormat wav;
 auto out=dir.getChildFile("sine.wav").createOutputStream();
 std::unique_ptr<juce::AudioFormatWriter> writer(wav.createWriterFor(out.release(),sr,1,16,{},0));
 writer->writeFromAudioSampleBuffer(b,0,b.getNumSamples()); writer.reset();
 auto mapping=juce::JSON::parse(R"({"regions":[{"sample":"sine.wav","low":0,"high":127,"root":57,"lowVelocity":1,"highVelocity":127,"release":0.2}]})");
 auto sample=dir.getChildFile("sine.wav");juce::MemoryBlock bytes;sample.loadFileAsData(bytes);
 unsigned char digest[CC_SHA256_DIGEST_LENGTH];CC_SHA256(bytes.getData(),static_cast<CC_LONG>(bytes.getSize()),digest);
 auto* entry=mapping["regions"].getArray()->getReference(0).getDynamicObject();
 entry->setProperty("bytes",sample.getSize());entry->setProperty("sha256",juce::String::toHexString(digest,CC_SHA256_DIGEST_LENGTH).removeCharacters(" "));
 dir.getChildFile("mapping.json").replaceWithText(juce::JSON::toString(mapping));
 return dir;
}
bool finiteAndBounded(const juce::AudioBuffer<float>& b,float limit=1) {
 for(int c=0;c<b.getNumChannels();++c) for(int i=0;i<b.getNumSamples();++i)
  if(!std::isfinite(b.getSample(c,i)) || std::abs(b.getSample(c,i))>limit) return false;
 return true;
}
}
class GuitarChecks : public juce::UnitTest {
public:
 GuitarChecks():UnitTest("Guitar engine","Hendrix") {}
 void runTest() override {
  auto dir=juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("hendrix-check-"+juce::Uuid().toString());
  checks::writeSineBank(dir);
  GuitarEngine engine; engine.prepare(48000,256);
  juce::AudioBuffer<float> audio(2,256); juce::MidiBuffer midi;
  beginTest("Invalid assets fail atomically and remain silent");
  expect(engine.load(dir.getChildFile("missing")).failed());
  engine.render(audio,midi,{});
  expectEquals(audio.getMagnitude(0,256),0.0f);
  beginTest("Valid sample bank plays at MIDI event offset");
  expect(engine.load(dir).wasOk());
  midi.addEvent(juce::MidiMessage::noteOn(1,57,juce::uint8(100)),32);
  engine.render(audio,midi,{});
  expectEquals(audio.getMagnitude(0,32),0.0f);
  expect(audio.getMagnitude(32,224)>0.01f);
  expect(checks::finiteAndBounded(audio));
  beginTest("Reset produces silence");
  engine.reset(); midi.clear(); engine.render(audio,midi,{});
  expectEquals(audio.getMagnitude(0,256),0.0f);
  beginTest("Bundled guitar bank decodes all regions");
  expect(engine.load(juce::File(__FILE__).getParentDirectory().getParentDirectory().getChildFile("Resources/Guitar")).wasOk());
  beginTest("Corrupt data cannot leave a partially loaded bank");
  dir.getChildFile("sine.wav").replaceWithText("bad audio");
  expect(engine.load(dir).failed()); engine.render(audio,midi,{});
  expectEquals(audio.getMagnitude(0,256),0.0f);
  dir.deleteRecursively();
 }
};
static GuitarChecks guitarChecks;
namespace checks {
juce::AudioBuffer<float> render(GuitarEngine& e,const juce::MidiBuffer& m,int n,const PerformanceSettings& s={}) {
 juce::AudioBuffer<float> b(2,n);e.render(b,m,s);return b;
}
juce::AudioBuffer<float> tail(const juce::AudioBuffer<float>& b,int start) {
 juce::AudioBuffer<float> t(b.getNumChannels(),b.getNumSamples()-start);
 for(int c=0;c<t.getNumChannels();++c) t.copyFrom(c,0,b,c,start,t.getNumSamples());return t;
}
double dominantFrequency(const juce::AudioBuffer<float>& b,double sr) {
 double first=-1,last=-1;int crossings=0;
 for(int i=1;i<b.getNumSamples();++i) if(b.getSample(0,i-1)<=0 && b.getSample(0,i)>0) {
  auto x=i-1-b.getSample(0,i-1)/(b.getSample(0,i)-b.getSample(0,i-1));
  if(first<0)first=x;last=x;++crossings;
 }
 return crossings>1?(crossings-1)*sr/(last-first):0;
}
double powerAt(const juce::AudioBuffer<float>& b,double sr,double hz) {
 double re=0,im=0;
 for(int i=0;i<b.getNumSamples();++i){auto a=juce::MathConstants<double>::twoPi*hz*i/sr; re+=b.getSample(0,i)*std::cos(a);im+=b.getSample(0,i)*std::sin(a);}
 return (re*re+im*im)/(b.getNumSamples()*double(b.getNumSamples()));
}
}
class ExpressionChecks : public juce::UnitTest {
public:
 ExpressionChecks():UnitTest("Guitar expression","Hendrix"){}
 void runTest() override {
  for(double sr:{44100.0,48000.0,96000.0}) {
   auto dir=juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("hendrix-expression-"+juce::Uuid().toString());
   checks::writeSineBank(dir,sr);GuitarEngine e;expect(e.load(dir).wasOk());e.prepare(sr,256);
   juce::MidiBuffer m;const int n=static_cast<int>(sr);PerformanceSettings s;
   auto note=[&](int key=57,int ch=1){m.addEvent(juce::MidiMessage::noteOn(ch,key,juce::uint8(100)),0);};
   beginTest("Pitch wheel bends by selected range at "+juce::String(sr));
   note();m.addEvent(juce::MidiMessage::pitchWheel(1,16383),0);
   auto b=checks::render(e,m,n,s);
   expectWithinAbsoluteError(checks::dominantFrequency(checks::tail(b,n/4),sr)/220.0,1.122462,0.01);
   beginTest("Channel expression is isolated");e.reset();m.clear();note();m.addEvent(juce::MidiMessage::pitchWheel(2,16383),0);
   b=checks::render(e,m,n,s);expectWithinAbsoluteError(checks::dominantFrequency(checks::tail(b,n/4),sr),220.0,1.0);
   beginTest("Sustain holds note then releases");e.reset();m.clear();note();m.addEvent(juce::MidiMessage::controllerEvent(1,64,127),50);m.addEvent(juce::MidiMessage::noteOff(1,57),100);
   b=checks::render(e,m,n,s);expect(checks::tail(b,n/2).getMagnitude(0,n/2)>.01f);
   m.clear();m.addEvent(juce::MidiMessage::controllerEvent(1,64,0),0);b=checks::render(e,m,n,s);expect(b.getMagnitude(0,n/2,n/2)<1e-5f);
   beginTest("All sound off is immediate");e.reset();m.clear();note();checks::render(e,m,1000,s);
   m.clear();m.addEvent(juce::MidiMessage::controllerEvent(1,120,0),32);b=checks::render(e,m,256,s);expectEquals(b.getMagnitude(0,32,224),0.0f);
   beginTest("All notes off releases");e.reset();m.clear();note();checks::render(e,m,1000,s);
   m.clear();m.addEvent(juce::MidiMessage::controllerEvent(1,123,0),0);b=checks::render(e,m,n,s);expect(b.getMagnitude(0,n/2,n/2)<1e-5f);
   beginTest("Repeated note-offs release one press at a time");e.reset();m.clear();note();m.addEvent(juce::MidiMessage::noteOn(1,57,juce::uint8(100)),100);m.addEvent(juce::MidiMessage::noteOff(1,57),200);
   b=checks::render(e,m,n,s);expect(b.getMagnitude(0,n/2,n/2)>.01f);
   m.clear();m.addEvent(juce::MidiMessage::noteOff(1,57),0);b=checks::render(e,m,n,s);expect(b.getMagnitude(0,n/2,n/2)<1e-5f);
   beginTest("Zero-duration note cannot stick");e.reset();m.clear();note();m.addEvent(juce::MidiMessage::noteOff(1,57),0);
   b=checks::render(e,m,n,s);expectEquals(b.getMagnitude(0,n),0.0f);
   beginTest("Legato glides and returns to held note");e.reset();m.clear();s.legato=true;note();m.addEvent(juce::MidiMessage::noteOn(1,60,juce::uint8(100)),1000);
   b=checks::render(e,m,n,s);expectWithinAbsoluteError(checks::dominantFrequency(checks::tail(b,n/2),sr),261.6256,1.0);
   auto legatoTail=checks::tail(b,n/2);expect(checks::powerAt(legatoTail,sr,220)<checks::powerAt(legatoTail,sr,261.6256)*.001);
   m.clear();m.addEvent(juce::MidiMessage::noteOff(1,60),0);b=checks::render(e,m,n,s);
   expectWithinAbsoluteError(checks::dominantFrequency(checks::tail(b,n/2),sr),220.0,1.0);
   beginTest("Modulation activates vibrato");s.legato=false;e.reset();m.clear();note();auto dry=checks::render(e,m,n,s);
   e.reset();m.clear();note();m.addEvent(juce::MidiMessage::controllerEvent(1,1,127),0);auto wet=checks::render(e,m,n,s);
   wet.addFrom(0,0,dry,0,0,n,-1);expect(wet.getRMSLevel(0,n/4,n/2)>.01f);
   beginTest("Feedback extends source then releases");e.reset();m.clear();s.feedback=1;note();b=checks::render(e,m,4*n,s);
   expect(b.getMagnitude(0,3*n,n)>.0005f);expect(checks::finiteAndBounded(b));
   m.clear();m.addEvent(juce::MidiMessage::noteOff(1,57),0);b=checks::render(e,m,n,s);expect(b.getMagnitude(0,n/2,n/2)<1e-5f);
   beginTest("Seventh note steals oldest with a fade");s.feedback=0;e.reset();m.clear();
   for(int key=57;key<=62;++key)note(key);
   auto before=checks::render(e,m,1234,s);m.clear();note(63);b=checks::render(e,m,n/2,s);
   expect(std::abs(b.getSample(0,0)-before.getSample(0,1233))<.05f);
   auto after=checks::tail(b,static_cast<int>(sr*.02));
   expect(checks::powerAt(after,sr,220)<checks::powerAt(after,sr,311.127)*.01);
   beginTest("Block partitioning preserves expression timing");
   juce::MidiBuffer events;events.addEvent(juce::MidiMessage::noteOn(1,57,juce::uint8(100)),32);events.addEvent(juce::MidiMessage::pitchWheel(1,13000),1000);events.addEvent(juce::MidiMessage::noteOff(1,57),4000);
   e.reset();auto full=checks::render(e,events,8000,s);
   for(int block:{1,64,257,1024}) {
    e.reset();juce::AudioBuffer<float> result(2,8000);
    for(int pos=0;pos<8000;pos+=block) {
     auto len=std::min(block,8000-pos);juce::MidiBuffer part;
     for(const auto item:events)if(item.samplePosition>=pos&&item.samplePosition<pos+len)part.addEvent(item.getMessage(),item.samplePosition-pos);
     auto chunk=checks::render(e,part,len,s);for(int c=0;c<2;++c)result.copyFrom(c,pos,chunk,c,0,len);
    }
    for(int c=0;c<2;++c)result.addFrom(c,0,full,c,0,8000,-1);expect(result.getMagnitude(0,8000)<1e-6f);
   }
   beginTest("All sound off removes stolen tails on their original channel");
   auto stolenScenario=[&](juce::uint8 velocity) {
    e.reset();juce::MidiBuffer x;x.addEvent(juce::MidiMessage::noteOn(1,57,velocity),0);
    for(int key=58;key<=62;++key)x.addEvent(juce::MidiMessage::noteOn(2,key,juce::uint8(100)),0);
    checks::render(e,x,1234,s);x.clear();x.addEvent(juce::MidiMessage::noteOn(2,63,juce::uint8(100)),0);
    x.addEvent(juce::MidiMessage::controllerEvent(1,120,0),0);return checks::render(e,x,512,s);
   };
   auto loudTail=stolenScenario(100),quietTail=stolenScenario(1);
   loudTail.addFrom(0,0,quietTail,0,0,512,-1);expect(loudTail.getMagnitude(0,0,512)<1e-6f);
   beginTest("Leaving legato preserves repeated-note ownership");
   e.reset();s.legato=true;m.clear();note();note();m.addEvent(juce::MidiMessage::noteOn(1,60,juce::uint8(100)),1000);checks::render(e,m,n,s);
   m.clear();m.addEvent(juce::MidiMessage::noteOff(1,60),0);checks::render(e,m,n/4,s);
   s.legato=false;m.clear();m.addEvent(juce::MidiMessage::noteOff(1,57),0);b=checks::render(e,m,n/2,s);
   expect(b.getMagnitude(0,n/4,n/4)>.01f);
   m.clear();m.addEvent(juce::MidiMessage::noteOff(1,57),0);b=checks::render(e,m,n/2,s);
   expect(b.getMagnitude(0,n/4,n/4)<1e-5f);
   juce::AudioBuffer<float> zero(2,0);m.clear();e.render(zero,m,s);
   dir.deleteRecursively();
  }
 }
};
static ExpressionChecks expressionChecks;
#include "VintageTone.h"
#include "PluginProcessor.h"
class ToneChecks : public juce::UnitTest {
public:
 ToneChecks():UnitTest("Vintage tone","Hendrix"){}
 void runTest() override {
  for(double sr:{44100.0,48000.0,96000.0}) {
   int n=static_cast<int>(sr);
   auto input=[&](float level=.2f) {
    juce::AudioBuffer<float> b(2,n);
    for(int i=0;i<n;++i)for(int c=0;c<2;++c)b.setSample(c,i,level*std::sin(static_cast<float>(juce::MathConstants<double>::twoPi*220*i/sr)));
    return b;
   };
   auto processed=[&](const ToneSettings& s) {VintageTone f;f.prepare(sr,64);auto b=input();f.process(b,s);return checks::tail(b,n/4);};
   ToneSettings s;VintageTone f;f.prepare(sr,64);
   beginTest("Silent/reset tone remains silent");
   juce::AudioBuffer<float> silence(2,128);silence.clear();f.process(silence,s);expectEquals(silence.getMagnitude(0,128),0.0f);
   auto driven=input();f.process(driven,s);f.reset();silence.clear();f.process(silence,s);expectEquals(silence.getMagnitude(0,128),0.0f);
   beginTest("Extreme controls and input are bounded");
   for(int size:{0,1,64,257,1024,4096}) {
    juce::AudioBuffer<float> b(2,size);for(int c=0;c<2;++c)for(int i=0;i<size;++i)b.setSample(c,i,(i%2?10.0f:-10.0f));
    s.guitarVolume=10;s.master=10;s.fuzz=10;s.wahEnabled=true;s.wahPosition=10;s.vibeDepth=10;s.vibeRateHz=100;s.ampDrive=10;s.ampTone=10;s.cabTone=10;s.octave=10;
    f.process(b,s);expect(checks::finiteAndBounded(b));
   }
   beginTest("Invalid controls cannot poison the DSP");
   s=ToneSettings{};s.fuzz=std::numeric_limits<float>::quiet_NaN();auto b=input();b.setSample(0,12,std::numeric_limits<float>::quiet_NaN());
   f.process(b,s);expect(checks::finiteAndBounded(b));f.reset();
   beginTest("Guitar volume changes harmonic distortion");
   s=ToneSettings{};s.guitarVolume=.15f;s.fuzz=.5f;s.ampDrive=.7f;
   auto low=processed(s);s.guitarVolume=1;auto high=processed(s);
   expect(checks::powerAt(low,sr,660)/checks::powerAt(low,sr,220)<checks::powerAt(high,sr,660)/checks::powerAt(high,sr,220));
   beginTest("Octave-up raises second harmonic");
   s=ToneSettings{};s.fuzz=0;s.vibeDepth=0;s.ampDrive=.2f;s.octave=0;auto off=processed(s);
   s.octave=1;auto on=processed(s);expect(checks::powerAt(on,sr,440)>checks::powerAt(off,sr,440)*2);
   beginTest("Wah position changes spectrum");
   s=ToneSettings{};s.wahEnabled=true;s.wahPosition=0;auto wahA=processed(s);s.wahPosition=1;auto wahB=processed(s);
   wahA.addFrom(0,0,wahB,0,0,wahA.getNumSamples(),-1);expect(wahA.getRMSLevel(0,0,wahA.getNumSamples())>.005f);
   beginTest("Vibe depth zero bypasses modulation");
   s=ToneSettings{};s.vibeDepth=0;auto dry=processed(s),dryAgain=processed(s);
   dryAgain.addFrom(0,0,dry,0,0,dry.getNumSamples(),-1);expect(dryAgain.getMagnitude(0,0,dryAgain.getNumSamples())<1e-6f);
   s.vibeDepth=1;auto wet=processed(s);wet.addFrom(0,0,dry,0,0,dry.getNumSamples(),-1);
   expect(wet.getRMSLevel(0,0,wet.getNumSamples())>.005f);
   beginTest("Oversampling latency is positive and integer");expect(f.latencySamples()>0);
  }
  beginTest("Processor reports tone latency");
  HendrixGuitarAudioProcessor processor;processor.prepareToPlay(48000,256);
  VintageTone f;f.prepare(48000,256);expectEquals(processor.getLatencySamples(),f.latencySamples());expect(processor.getLatencySamples()>0);
  beginTest("Processor sound-off clears effect history at event offset");
  juce::AudioBuffer<float> b(2,256);juce::MidiBuffer m;
  m.addEvent(juce::MidiMessage::noteOn(1,60,juce::uint8(100)),0);processor.processBlock(b,m);
  m.clear();m.addEvent(juce::MidiMessage::controllerEvent(1,120,0),32);processor.processBlock(b,m);
  expectEquals(b.getMagnitude(0,32,224),0.0f);
 }
};
static ToneChecks toneChecks;
