#include "PluginProcessor.h"
#include <dlfcn.h>
static juce::File guitarDirectory() {
 Dl_info info{};
 if (dladdr(reinterpret_cast<const void*>(&guitarDirectory), &info) == 0) return {};
 return juce::File(info.dli_fname).getParentDirectory().getParentDirectory().getChildFile("Resources/Guitar");
}
HendrixGuitarAudioProcessor::HendrixGuitarAudioProcessor()
: AudioProcessor(BusesProperties().withOutput("Output",juce::AudioChannelSet::stereo(),true)) { loadStatus=engine.load(guitarDirectory()); }
void HendrixGuitarAudioProcessor::prepareToPlay(double sr,int n) {engine.prepare(sr,n);}
void HendrixGuitarAudioProcessor::releaseResources() {engine.reset();}
void HendrixGuitarAudioProcessor::processBlock(juce::AudioBuffer<float>& b,juce::MidiBuffer& m) {juce::ScopedNoDenormals no; engine.render(b,m,{});}
bool HendrixGuitarAudioProcessor::isBusesLayoutSupported(const BusesLayout& l) const {return l.getMainOutputChannelSet()==juce::AudioChannelSet::stereo() && l.getMainInputChannelSet().isDisabled();}
juce::AudioProcessorEditor* HendrixGuitarAudioProcessor::createEditor() {return new juce::GenericAudioProcessorEditor(*this);}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {return new HendrixGuitarAudioProcessor();}
