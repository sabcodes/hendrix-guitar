#include "PluginProcessor.h"
#include <CommonCrypto/CommonDigest.h>
class ReleaseChecks : public juce::UnitTest {
public:
 ReleaseChecks():UnitTest("Release regressions","Hendrix"){}
 void runTest() override {
  beginTest("Host reset clears sounding voices, controllers and effect history");
  HendrixGuitarAudioProcessor processor;processor.prepareToPlay(48000,256);
  processor.parameters.getParameter("feedback")->setValueNotifyingHost(1);
  juce::AudioBuffer<float> audio(2,256);juce::MidiBuffer midi;
  midi.addEvent(juce::MidiMessage::noteOn(1,57,juce::uint8(100)),0);
  midi.addEvent(juce::MidiMessage::controllerEvent(1,64,127),0);
  processor.processBlock(audio,midi);midi.clear();
  for(int i=0;i<30;++i)processor.processBlock(audio,midi);
  expect(audio.getMagnitude(0,256)>0);
  processor.reset();processor.processBlock(audio,midi);
  expectEquals(audio.getMagnitude(0,256),0.0f);
  for(int i=0;i<30;++i)processor.processBlock(audio,midi);
  expectEquals(audio.getMagnitude(0,256),0.0f);

  auto directory=juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("hendrix-integrity-"+juce::Uuid().toString());
  directory.createDirectory();auto sample=directory.getChildFile("sample.flac");
  juce::AudioBuffer<float> source(1,48000*3);
  for(int i=0;i<source.getNumSamples();++i)source.setSample(0,i,.2f*std::sin(float(i*220.0/48000*juce::MathConstants<double>::twoPi)));
  juce::FlacAudioFormat format;auto output=sample.createOutputStream();
  std::unique_ptr<juce::AudioFormatWriter> writer(format.createWriterFor(output.release(),48000,1,16,{},5));
  expect(writer!=nullptr);if(!writer){directory.deleteRecursively();return;}
  expect(writer->writeFromAudioSampleBuffer(source,0,source.getNumSamples()));writer.reset();
  juce::MemoryBlock intact;expect(sample.loadFileAsData(intact));
  unsigned char digest[CC_SHA256_DIGEST_LENGTH];CC_SHA256(intact.getData(),static_cast<CC_LONG>(intact.getSize()),digest);
  auto hash=juce::String::toHexString(digest,CC_SHA256_DIGEST_LENGTH).removeCharacters(" ");
  auto mapping=juce::JSON::parse(R"({"regions":[{"sample":"sample.flac","low":0,"high":127,"root":57,"lowVelocity":1,"highVelocity":127,"release":0.2}]})");
  auto* entry=mapping["regions"].getArray()->getReference(0).getDynamicObject();
  entry->setProperty("bytes",static_cast<juce::int64>(intact.getSize()));entry->setProperty("sha256",hash);
  auto writeMapping=[&]{directory.getChildFile("mapping.json").replaceWithText(juce::JSON::toString(mapping));};
  writeMapping();GuitarEngine engine;engine.prepare(48000,256);
  beginTest("Intact FLAC with recorded integrity loads");
  expect(engine.load(directory).wasOk());
  beginTest("Truncated FLAC with intact metadata fails atomically");
  sample.replaceWithData(intact.getData(),intact.getSize()/2);
  expect(engine.load(directory).failed());midi.addEvent(juce::MidiMessage::noteOn(1,57,juce::uint8(100)),0);
  engine.render(audio,midi,{});expectEquals(audio.getMagnitude(0,256),0.0f);
  beginTest("Same-size corrupt FLAC fails integrity");
  auto corrupt=intact;static_cast<unsigned char*>(corrupt.getData())[corrupt.getSize()/2]^=0x20;
  sample.replaceWithData(corrupt.getData(),corrupt.getSize());expect(engine.load(directory).failed());
  beginTest("Missing or malformed integrity is rejected");
  sample.replaceWithData(intact.getData(),intact.getSize());entry->removeProperty("sha256");writeMapping();expect(engine.load(directory).failed());
  entry->setProperty("sha256","invalid");writeMapping();expect(engine.load(directory).failed());
  entry->setProperty("sha256",hash);entry->setProperty("bytes","not a size");writeMapping();expect(engine.load(directory).failed());
  directory.deleteRecursively();
 }
};
static ReleaseChecks releaseChecks;
