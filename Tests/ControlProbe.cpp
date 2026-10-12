#include "GuitarEngine.h"
#include "VintageTone.h"
#include "PluginProcessor.h"
#include <iostream>
static double rms(const juce::AudioBuffer<float>& audio,int skip=4800) {
 double power=0;for(int i=skip;i<audio.getNumSamples();++i)power+=audio.getSample(0,i)*audio.getSample(0,i);
 return std::sqrt(power/(audio.getNumSamples()-skip));
}
static double brightness(const juce::AudioBuffer<float>& audio) {
 juce::dsp::FFT fft(11);std::array<float,4096> data{};double low=0,high=0;
 for(int offset=4800;offset+2048<audio.getNumSamples();offset+=2048) {
  data.fill(0);for(int i=0;i<2048;++i)data[size_t(i)]=audio.getSample(0,offset+i)*float(.5-.5*std::cos(juce::MathConstants<double>::twoPi*i/2047));
  fft.performFrequencyOnlyForwardTransform(data.data());
  for(int k=1;k<400;++k) {auto energy=double(data[size_t(k)])*data[size_t(k)];if(k*48000.0/2048<1500)low+=energy;else high+=energy;}
 }
 return 10*std::log10((high+1e-20)/(low+1e-20));
}
static double difference(const juce::AudioBuffer<float>& a,const juce::AudioBuffer<float>& b) {
 double ra=rms(a),rb=rms(b),sum=0;
 for(int i=4800;i<a.getNumSamples();++i){double x=a.getSample(0,i)/(ra+1e-12)-b.getSample(0,i)/(rb+1e-12);sum+=x*x;}
 return std::sqrt(sum/(a.getNumSamples()-4800));
}
int probeControls(const juce::File& output) {
 output.createDirectory();
 HendrixGuitarAudioProcessor processor;std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
 juce::Slider* volume=nullptr;
 for(auto* child:editor->getChildren())if(auto* slider=dynamic_cast<juce::Slider*>(child))if(slider->getName()=="Guitar volume")volume=slider;
 if(!volume)return 2;volume->setValue(0,juce::sendNotificationSync);
 if(processor.parameters.getRawParameterValue("guitarVolume")->load()!=0)return 2;
 processor.prepareToPlay(48000,256);juce::AudioBuffer<float> live(2,256);juce::MidiBuffer events;
 events.addEvent(juce::MidiMessage::noteOn(1,57,juce::uint8(100)),0);processor.processBlock(live,events);events.clear();
 for(int i=0;i<40;++i)processor.processBlock(live,events);
 if(live.getMagnitude(0,256)>1e-6f){std::cout<<"UI volume mute did not reach audio"<<std::endl;return 2;}
 volume->setValue(.85,juce::sendNotificationSync);for(int i=0;i<40;++i)processor.processBlock(live,events);
 if(live.getMagnitude(0,256)<.001f){std::cout<<"UI volume unmute did not reach audio"<<std::endl;return 2;}
 std::cout<<"UI slider -> saved parameter -> rendered audio wiring verified."<<std::endl;
 GuitarEngine engine;
 if(engine.load(juce::File(__FILE__).getParentDirectory().getParentDirectory().getChildFile("Resources/Guitar")).failed())return 2;
 engine.prepare(48000,256);juce::MidiBuffer midi;midi.addEvent(juce::MidiMessage::noteOn(1,57,juce::uint8(100)),0);
 juce::AudioBuffer<float> input(2,96000);engine.render(input,midi,{});
 const char* names[]={"guitarVolume","master","fuzz","ampDrive","ampTone","cabTone","vibeRate","vibeDepth","octave","wahPosition-off","wahPosition-on","fuzz-mid","fuzz-full-volume","fuzz-mid-full-volume"};
 bool weak=false;
 for(int control=0;control<14;++control) {
  std::array<juce::AudioBuffer<float>,2> pair;
  for(int side=0;side<2;++side) {
   ToneSettings settings;settings.guitarVolume=.14f;settings.fuzz=.65f;settings.ampDrive=.65f;
   const float value=float(side);
   switch(control) {
    case 0:settings.guitarVolume=side?.85f:.14f;break;
    case 1:settings.master=side?1.0f:.1f;break;
    case 2:settings.fuzz=value;break;
    case 3:settings.ampDrive=value;break;
    case 4:settings.ampTone=value;break;
    case 5:settings.cabTone=value;break;
    case 6:settings.vibeRateHz=side?8.0f:.5f;break;
    case 7:settings.vibeDepth=value;break;
    case 8:settings.octave=value;break;
    case 9:settings.wahPosition=value;break;
    case 10:settings.wahEnabled=true;settings.wahPosition=value;break;
    case 11:settings.fuzz=side?.85f:.35f;break;
    case 12:settings.guitarVolume=.85f;settings.fuzz=value;break;
    case 13:settings.guitarVolume=.85f;settings.fuzz=side?.85f:.35f;break;
   }
   pair[size_t(side)].makeCopyOf(input);VintageTone tone;tone.prepare(48000,256);tone.process(pair[size_t(side)],settings);
   auto file=output.getChildFile(juce::String(names[control])+(side?"-high.wav":"-low.wav"));file.deleteFile();
   juce::WavAudioFormat wav;auto stream=file.createOutputStream();std::unique_ptr<juce::AudioFormatWriter> writer(wav.createWriterFor(stream.release(),48000,2,24,{},0));
   if(!writer||!writer->writeFromAudioSampleBuffer(pair[size_t(side)],0,input.getNumSamples()))return 2;
  }
  double level=20*std::log10((rms(pair[1])+1e-20)/(rms(pair[0])+1e-20));
  double bright=brightness(pair[1])-brightness(pair[0]),shape=difference(pair[0],pair[1]);
  std::cout<<names[control]<<": level delta "<<level<<" dB, brightness delta "<<bright<<" dB, level-matched difference "<<shape<<std::endl;
  if((control==4||control==5)&&std::abs(bright)<6)weak=true;
  if(control==0&&level<6)weak=true;
  if(control==11&&(shape<.15||level<6))weak=true;
  if(control==6&&shape<.1)weak=true;
  if(control==13&&shape<.15)weak=true;
  if(!std::isfinite(level)||!std::isfinite(bright)||!std::isfinite(shape))return 2;
 }
 if(weak)std::cout<<"WEAK: insufficient guitar-volume/fuzz contrast or less than 6 dB amp/cab brightness range."<<std::endl;
 return weak?1:0;
}
