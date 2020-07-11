#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

#if JUCE_IOS

//==============================================================================
/**
*/
class PAPUAudioProcessorEditor  : public gin::GinAudioProcessorEditor
{
public:
    PAPUAudioProcessorEditor (PAPUAudioProcessor&);
    ~PAPUAudioProcessorEditor() override;

    //==============================================================================
    void resized() override;
    void paint (Graphics& g) override;

    PAPUAudioProcessor& proc;
    
    gin::TriggeredScope scope { proc.fifo };
    Image logo;

    MidiKeyboardComponent keyboard { proc.state, MidiKeyboardComponent::horizontalKeyboard };
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PAPUAudioProcessorEditor)
};

#endif
