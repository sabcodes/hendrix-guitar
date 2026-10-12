#include <juce_audio_utils/juce_audio_utils.h>
#include "GuitarEngine.h"
#include "VintageTone.h"
#include "PluginProcessor.h"
#include <iostream>
static int renderReferences(const juce::File& output) {
 output.createDirectory();
 const char* names[]={"Clean Touch","Driven Lead","Wah Fuzz"};
 for(int preset=0;preset<3;++preset) {
  GuitarEngine engine;
  auto status=engine.load(juce::File(__FILE__).getParentDirectory().getParentDirectory().getChildFile("Resources/Guitar"));
  if(status.failed())return 1;
  engine.prepare(48000,256);VintageTone tone;tone.prepare(48000,256);
  ToneSettings settings;settings.fuzz=preset==0?0:preset==1?.65f:.7f;
  settings.ampDrive=preset==0?.2f:preset==1?.65f:.6f;settings.wahEnabled=preset==2;
  juce::AudioBuffer<float> full(2,48000*8);full.clear();
  juce::MidiBuffer events;const int notes[]={52,55,57,59,57,55,52,64,67,69,71,69};
  for(int i=0;i<12;++i) {
   int start=i*24000;events.addEvent(juce::MidiMessage::noteOn(1,notes[i],juce::uint8(80+i%3*20)),start);
   events.addEvent(juce::MidiMessage::noteOff(1,notes[i]),start+19000);
  }
  float peak=0;
  for(int at=0;at<full.getNumSamples();at+=256) {
   int n=juce::jmin(256,full.getNumSamples()-at);juce::AudioBuffer<float> block(2,n);juce::MidiBuffer midi;
   midi.addEvents(events,at,n,-at);engine.render(block,midi,{});
   settings.wahPosition=.5f+.4f*std::sin(float(at/48000.0*juce::MathConstants<double>::twoPi*.6));
   tone.process(block,settings);peak=juce::jmax(peak,block.getMagnitude(0,n));
   for(int ch=0;ch<2;++ch)full.copyFrom(ch,at,block,ch,0,n);
  }
  auto file=output.getChildFile(juce::String(names[preset])+".wav");file.deleteFile();
  juce::WavAudioFormat format;auto stream=file.createOutputStream();
  std::unique_ptr<juce::AudioFormatWriter> writer(format.createWriterFor(stream.release(),48000,2,24,{},0));
  if(!writer||!writer->writeFromAudioSampleBuffer(full,0,full.getNumSamples()))return 1;
  std::cout<<names[preset]<<": peak "<<peak<<"; "<<file.getFullPathName()<<std::endl;
 }
 return 0;
}
int main(int argc,char** argv) {
 juce::ScopedJuceInitialiser_GUI gui;
 if(argc==3&&juce::String(argv[1])=="--editor") {
  HendrixGuitarAudioProcessor processor;std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
  auto image=editor->createComponentSnapshot(editor->getLocalBounds(),true,2.0f);juce::PNGImageFormat png;
  auto file=juce::File(argv[2]);file.deleteFile();auto output=file.createOutputStream();return output&&png.writeImageToStream(image,*output)?0:1;
 }
 if(argc==3&&juce::String(argv[1])=="--render")return renderReferences(juce::File(argv[2]));
 juce::UnitTestRunner runner; runner.runTestsInCategory("Hendrix",12345);
 int failures=0; for(int i=0;i<runner.getNumResults();++i) failures+=runner.getResult(i)->failures;
 return failures==0?0:1;
}
