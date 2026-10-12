#include "Parameters.h"
juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() {
 juce::AudioProcessorValueTreeState::ParameterLayout layout;
 auto add=[&](const char* id,const char* name,float low,float high,float value) {
  layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id,1},name,juce::NormalisableRange<float>{low,high},value));
 };
 add("guitarVolume","Guitar volume",0,1,.85f);add("master","Output",0,1,.65f);
 add("fuzz","Fuzz",0,1,.35f);
 layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"wahEnabled",1},"Wah",false));
 add("wahPosition","Wah position",0,1,.5f);add("vibeRate","Vibe rate",.1f,10,2.5f);add("vibeDepth","Vibe depth",0,1,.15f);
 add("ampDrive","Amp drive",0,1,.4f);add("ampTone","Amp tone",0,1,.55f);add("cabTone","Cab tone",0,1,.5f);
 add("octave","Octave",0,1,0);add("feedback","Harmonic sustain",0,1,0);
 layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"legato",1},"Mono legato",false));
 add("glideMs","Glide (ms)",5,250,70);add("bendRange","Bend (semitones)",1,12,2);
 add("vibratoRate","Vibrato rate (Hz)",.5f,10,5.5f);add("vibratoDepth","Vibrato depth (cents)",0,100,35);
 return layout;
}
static float read(const juce::AudioProcessorValueTreeState& p,const char* id){return p.getRawParameterValue(id)->load(std::memory_order_relaxed);}
PerformanceSettings readPerformanceSettings(const juce::AudioProcessorValueTreeState& p) {
 PerformanceSettings s;s.legato=read(p,"legato")>=.5f;s.bendRangeSemitones=read(p,"bendRange");s.glideMs=read(p,"glideMs");
 s.vibratoRateHz=read(p,"vibratoRate");s.vibratoDepthCents=read(p,"vibratoDepth");s.feedback=read(p,"feedback");return s;
}
ToneSettings readToneSettings(const juce::AudioProcessorValueTreeState& p) {
 ToneSettings s;s.guitarVolume=read(p,"guitarVolume");s.master=read(p,"master");s.fuzz=read(p,"fuzz");s.wahEnabled=read(p,"wahEnabled")>=.5f;
 s.wahPosition=read(p,"wahPosition");s.vibeRateHz=read(p,"vibeRate");s.vibeDepth=read(p,"vibeDepth");s.ampDrive=read(p,"ampDrive");
 s.ampTone=read(p,"ampTone");s.cabTone=read(p,"cabTone");s.octave=read(p,"octave");return s;
}
void applyPreset(juce::AudioProcessorValueTreeState& state,int index) {
 if(index<0||index>2)return;
 auto current=state.copyState();
 for(auto entry:current) {
  auto* p=state.getParameter(entry.getProperty("id").toString());if(!p)continue;p->beginChangeGesture();p->setValueNotifyingHost(p->getDefaultValue());p->endChangeGesture();
 }
 auto set=[&](const char* id,float value){auto* p=state.getParameter(id);p->beginChangeGesture();p->setValueNotifyingHost(p->convertTo0to1(value));p->endChangeGesture();};
 const float fuzz[]={0,.65f,.7f},drive[]={.2f,.65f,.6f};set("fuzz",fuzz[index]);set("ampDrive",drive[index]);
 if(index==2){set("wahEnabled",1);set("wahPosition",.6f);}
}
