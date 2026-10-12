#include "GuitarEngine.h"
namespace checks {
juce::File writeSineBank(const juce::File& dir,double sr=48000,double hz=220) {
 dir.createDirectory();
 juce::AudioBuffer<float> b(1,static_cast<int>(sr*3));
 for(int i=0;i<b.getNumSamples();++i) b.setSample(0,i,.2f*std::sin(float(juce::MathConstants<double>::twoPi*hz*i/sr)));
 juce::WavAudioFormat wav;
 auto out=dir.getChildFile("sine.wav").createOutputStream();
 std::unique_ptr<juce::AudioFormatWriter> writer(wav.createWriterFor(out.release(),sr,1,16,{},0));
 writer->writeFromAudioSampleBuffer(b,0,b.getNumSamples()); writer.reset();
 dir.getChildFile("mapping.json").replaceWithText(R"({"regions":[{"sample":"sine.wav","low":0,"high":127,"root":57,"lowVelocity":1,"highVelocity":127,"release":0.2}]})");
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
