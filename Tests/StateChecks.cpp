#include "PluginProcessor.h"
#include "Parameters.h"
class StateChecks : public juce::UnitTest {
public:
 StateChecks():UnitTest("Guitar parameters and state","Hendrix"){}
 void runTest() override {
  HendrixGuitarAudioProcessor original,restored;
  const char* ids[]={"guitarVolume","master","fuzz","wahEnabled","wahPosition","vibeRate","vibeDepth","ampDrive","ampTone","cabTone","octave","feedback","legato","glideMs","bendRange","vibratoRate","vibratoDepth"};
  beginTest("All stable parameter IDs round-trip after recreation");
  for(auto id:ids) {
   auto* p=original.parameters.getParameter(id);expect(p!=nullptr);
   if(p)p->setValueNotifyingHost(.8f);
  }
  juce::MemoryBlock state;original.getStateInformation(state);expect(state.getSize()>0);
  restored.setStateInformation(state.getData(),int(state.getSize()));
  for(auto id:ids) {
   auto* a=original.parameters.getRawParameterValue(id);auto* b=restored.parameters.getRawParameterValue(id);
   expect(b!=nullptr);if(a&&b)expectWithinAbsoluteError(b->load(),a->load(),1e-5f);
  }
  auto value=[&](const char* id){return restored.parameters.getRawParameterValue(id)->load();};
  auto set=[&](const char* id,float v){auto* p=restored.parameters.getParameter(id);p->setValueNotifyingHost(p->convertTo0to1(v));};
  set("wahPosition",.8f);
  beginTest("Malformed bytes preserve current state");
  restored.setStateInformation("bad",3);expectWithinAbsoluteError(value("wahPosition"),.8f,1e-6f);
  auto restoreXML=[&](const juce::String& xml){juce::MemoryBlock bytes;auto x=juce::parseXML(xml);juce::AudioProcessor::copyXmlToBinary(*x,bytes);restored.setStateInformation(bytes.getData(),int(bytes.getSize()));};
  beginTest("Wrong root and unsupported version preserve current state");
  restoreXML("<Other schemaVersion='1'/>");expectWithinAbsoluteError(value("wahPosition"),.8f,1e-6f);
  restoreXML("<HendrixGuitarState schemaVersion='2'/>");expectWithinAbsoluteError(value("wahPosition"),.8f,1e-6f);
  beginTest("Valid partial state supplies defaults and clamps known finite values");
  restoreXML("<HendrixGuitarState schemaVersion='1'><PARAM id='wahPosition' value='0.3'/><PARAM id='master' value='20'/><PARAM id='fuzz' value='NaN'/><PARAM id='unknown' value='1'/></HendrixGuitarState>");
  expectWithinAbsoluteError(value("wahPosition"),.3f,1e-6f);
  expectWithinAbsoluteError(value("master"),1.0f,1e-6f);
  expectWithinAbsoluteError(value("fuzz"),.35f,1e-6f);
  expectWithinAbsoluteError(value("vibeRate"),2.5f,1e-6f);
  beginTest("Settings snapshots reflect parameters");
  set("legato",1);set("bendRange",12);set("glideMs",150);set("vibratoRate",8);set("vibratoDepth",80);set("feedback",.6f);
  auto perf=readPerformanceSettings(restored.parameters);expect(perf.legato);
  expectWithinAbsoluteError(perf.bendRangeSemitones,12.0f,1e-6f);expectWithinAbsoluteError(perf.glideMs,150.0f,1e-5f);
  expectWithinAbsoluteError(perf.vibratoRateHz,8.0f,1e-5f);expectWithinAbsoluteError(perf.vibratoDepthCents,80.0f,1e-5f);expectWithinAbsoluteError(perf.feedback,.6f,1e-5f);
  set("wahEnabled",1);set("fuzz",.9f);auto tone=readToneSettings(restored.parameters);expect(tone.wahEnabled);expectWithinAbsoluteError(tone.fuzz,.9f,1e-6f);
  beginTest("Three presets use intended tone and reset other controls to defaults");
  const float fuzz[]={0,.65f,.7f},drive[]={.2f,.65f,.6f};
  for(int i=0;i<3;++i) {applyPreset(restored.parameters,i);expectWithinAbsoluteError(value("fuzz"),fuzz[i],1e-6f);expectWithinAbsoluteError(value("ampDrive"),drive[i],1e-6f);expectEquals(value("wahEnabled"),i==2?1.0f:0.0f);expectEquals(value("feedback"),0.0f);}
  beginTest("Editor labels are readable and controls show concise values");
  std::unique_ptr<juce::AudioProcessorEditor> editor(original.createEditor());
  for(auto* child:editor->getChildren()) {
   if(auto* label=dynamic_cast<juce::Label*>(child))for(auto ch:label->getText())expect(ch<128);
   if(auto* slider=dynamic_cast<juce::Slider*>(child)) {
    auto text=slider->getTextFromValue(slider->getValue());auto point=text.indexOfChar('.');
    expect(point<0||text.length()-point-1<=2);
    for(auto* display:slider->getChildren())if(auto* label=dynamic_cast<juce::Label*>(display)){auto shown=label->getText();auto dot=shown.indexOfChar('.');expect(dot<0||shown.length()-dot-1<=2);}
   }
  }
  beginTest("Preset changes during playback keep note-off ownership");
  restored.prepareToPlay(48000,256);juce::AudioBuffer<float> audio(2,256);juce::MidiBuffer midi;
  midi.addEvent(juce::MidiMessage::noteOn(1,57,juce::uint8(100)),0);restored.processBlock(audio,midi);
  expect(audio.getMagnitude(0,256)>0);applyPreset(restored.parameters,1);
  midi.clear();midi.addEvent(juce::MidiMessage::noteOff(1,57),0);restored.processBlock(audio,midi);midi.clear();
  for(int i=0;i<200;++i)restored.processBlock(audio,midi);
  expect(audio.getMagnitude(0,256)<1e-5f);
 }
};
static StateChecks stateChecks;
