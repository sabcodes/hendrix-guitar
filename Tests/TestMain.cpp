#include <juce_audio_utils/juce_audio_utils.h>
int main() {
 juce::ScopedJuceInitialiser_GUI gui;
 juce::UnitTestRunner runner; runner.runTestsInCategory("Hendrix",12345);
 int failures=0; for(int i=0;i<runner.getNumResults();++i) failures+=runner.getResult(i)->failures;
 return failures==0?0:1;
}
