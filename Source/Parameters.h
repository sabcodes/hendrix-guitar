#pragma once
#include "GuitarEngine.h"
#include "VintageTone.h"
juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
PerformanceSettings readPerformanceSettings(const juce::AudioProcessorValueTreeState&);
ToneSettings readToneSettings(const juce::AudioProcessorValueTreeState&);
void applyPreset(juce::AudioProcessorValueTreeState&,int);
